/**
 * @file   test-segmented_stream.cpp
 * @brief  Test code for segmented_stream class.
 *
 * Copyright (C) 2010 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <boost/algorithm/string.hpp> // for case-insensitive string compare
#include <boost/iostreams/copy.hpp>
#include <iostream>
#include <iomanip>

#include <camoto/substream.hpp>
#include <camoto/segmented_stream.hpp>
#include <camoto/iostream_helpers.hpp>

#include "tests.hpp"

// Allow a string constant to be passed around with embedded nulls
#define makeString(x)  std::string((x), sizeof((x)) - 1)

struct segstream_sample: public default_sample {

	std::stringstream *psstrBase;
	void *_do; // unused var, but allows a statement to run in constructor init
	camoto::iostream_sptr psBase;
	camoto::segstream_sptr pss;

	segstream_sample() :
		psstrBase(new std::stringstream),
		_do((*this->psstrBase) << "ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
		psBase(this->psstrBase),
		pss(new camoto::segmented_stream(psBase))
	{
		// Make sure the data went in correctly to begin the test
		BOOST_REQUIRE(this->psstrBase->str().compare("ABCDEFGHIJKLMNOPQRSTUVWXYZ") == 0);
		this->psstrBase->exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
		this->psBase->exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
		this->pss->exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
	}

	boost::test_tools::predicate_result is_equal(int pos, const std::string& strExpected)
	{
		// Write the segmented_stream out to the underlying stringstream
		camoto::FN_TRUNCATE tr = boost::bind<void>(stringStreamTruncate, this->psstrBase, _1);
		this->pss->commit(tr);

		// Make sure the file offset hasn't changed after a commit (-1 is passed
		// in pos for "don't care" to skip this test.)
		if (pos >= 0) BOOST_CHECK_EQUAL(this->pss->tellp(), pos);

		// See if the stringstream now matches what we expected
		std::string strResult = this->psstrBase->str();
		strResult = strResult.substr(0, strResult.find_last_not_of('\0') + 1); /* TEMP: trim off trailing nulls */
		if (strExpected.compare(strResult)) {
			boost::test_tools::predicate_result res(false);
			this->print_wrong(res, strExpected, strResult);
			return res;
		}

		return true;
	}

};

BOOST_FIXTURE_TEST_SUITE(segmented_stream_suite, segstream_sample)

BOOST_AUTO_TEST_CASE(segstream_no_change)
{
	BOOST_TEST_MESSAGE("Flush with no change");

	BOOST_CHECK_MESSAGE(is_equal(0, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
		"Flush with no change failed");
}

BOOST_AUTO_TEST_CASE(segstream_streamMove_back)
{
	BOOST_TEST_MESSAGE("Overlapping stream move backwards (segmented_stream this time)");

	camoto::streamMove(*pss.get(), 10, 5, 10);

	BOOST_CHECK_MESSAGE(is_equal(-1, "ABCDEKLMNOPQRSTPQRSTUVWXYZ"),
		"Overlapping stream move backwards (segmented_stream this time) failed");
}

BOOST_AUTO_TEST_CASE(segstream_streamMove_forward)
{
	BOOST_TEST_MESSAGE("Overlapping stream move forward (segmented_stream this time)");

	camoto::streamMove(*pss.get(), 10, 15, 10);

	BOOST_CHECK_MESSAGE(is_equal(-1, "ABCDEFGHIJKLMNOKLMNOPQRSTZ"),
		"Overlapping stream move forward (segmented_stream this time) failed");
}

BOOST_AUTO_TEST_CASE(segstream_seek_write)
{
	BOOST_TEST_MESSAGE("Seek and write");

	pss->seekp(5);
	pss->write("123456", 6);

	BOOST_CHECK_MESSAGE(is_equal(11, "ABCDE123456LMNOPQRSTUVWXYZ"),
		"Seek and write failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_write)
{
	BOOST_TEST_MESSAGE("Insert and write into inserted space only");

	pss->seekp(4);
	pss->insert(5);
	pss->write("12345", 5);

	BOOST_CHECK_MESSAGE(is_equal(9, "ABCD12345EFGHIJKLMNOPQRSTUVWXYZ"),
		"Insert and write failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_write_src3)
{
	BOOST_TEST_MESSAGE("Insert and write over into third source");

	pss->seekp(4);
	pss->insert(4);
	pss->write("123456", 6);

	BOOST_CHECK_MESSAGE(is_equal(10, "ABCD123456GHIJKLMNOPQRSTUVWXYZ"),
		"Insert and write over into third source failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_within_insert)
{
	BOOST_TEST_MESSAGE("Insert within inserted segment");

	pss->seekp(5);
	pss->insert(10);
	pss->write("0123456789", 10);
	pss->seekp(-5, std::ios::cur);
	pss->insert(4);
	pss->write("!@#$", 4);

	BOOST_CHECK_MESSAGE(is_equal(14, "ABCDE01234!@#$56789FGHIJKLMNOPQRSTUVWXYZ"),
		"Insert within inserted segment failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_twice)
{
	BOOST_TEST_MESSAGE("Insert and insert again in third part");

	pss->seekp(5);
	pss->insert(5);
	pss->write("12345", 5);
	pss->seekp(5, std::ios::cur);
	pss->insert(5);
	pss->write("67890", 5);

	BOOST_CHECK_MESSAGE(is_equal(20, "ABCDE12345FGHIJ67890KLMNOPQRSTUVWXYZ"),
		"Insert and insert again in third part failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_twice_no_seek)
{
	BOOST_TEST_MESSAGE("Write into third stream then insert with no seek");

	pss->seekp(5);
	pss->insert(4);
	pss->write("123456", 6);
	pss->insert(4);
	pss->write("123456", 6);

	BOOST_CHECK_MESSAGE(is_equal(17, "ABCDE123456123456JKLMNOPQRSTUVWXYZ"),
		"Write into third stream then insert with no seek failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_at_eof)
{
	BOOST_TEST_MESSAGE("Insert at EOF");

	pss->seekp(0, std::ios::end);
	pss->insert(4);
	pss->write("1234", 4);

	BOOST_CHECK_MESSAGE(is_equal(30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234"),
		"Insert at EOF failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_at_eof_overwrite)
{
	BOOST_TEST_MESSAGE("Insert at EOF and overwrite");

	pss->seekp(0, std::ios::end);
	pss->insert(8);
	pss->write("12345678", 8);
	pss->seekp(-8, std::ios::cur);
	pss->write("!@#$", 4);

	BOOST_CHECK_MESSAGE(is_equal(30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$5678"),
		"Insert at EOF and overwrite failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_from_eof)
{
	BOOST_TEST_MESSAGE("Remove data from EOF, reducing file size");

	pss->seekp(21);
	pss->remove(5);

	BOOST_CHECK_MESSAGE(is_equal(21, "ABCDEFGHIJKLMNOPQRSTU"),
		"Remove data from EOF, reducing file size failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_write)
{
	BOOST_TEST_MESSAGE("Remove data from middle of stream, then write before it");

	pss->seekp(20);
	pss->remove(5);
	pss->seekp(10);
	pss->remove(5);
	pss->seekp(3);
	pss->write("1234", 4);

	BOOST_CHECK_MESSAGE(is_equal(7, "ABC1234HIJPQRSTZ"),
		"Remove data from middle of stream, then write before it failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_remove_before)
{
	BOOST_TEST_MESSAGE("Insert block, then remove just before new block");

	pss->seekp(4);
	pss->insert(5);
	pss->write("12345", 5);
	pss->seekp(2);
	pss->remove(2);

	BOOST_CHECK_MESSAGE(is_equal(2, "AB12345EFGHIJKLMNOPQRSTUVWXYZ"),
		"Insert block, then remove just before new block failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_remove_start)
{
	BOOST_TEST_MESSAGE("Insert block, then remove start of new block");

	pss->seekp(4);
	pss->insert(5);
	pss->write("12345", 5);
	pss->seekp(4);
	pss->remove(3);

	BOOST_CHECK_MESSAGE(is_equal(4, "ABCD45EFGHIJKLMNOPQRSTUVWXYZ"),
		"Insert block, then remove start of new block failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_remove_within)
{
	BOOST_TEST_MESSAGE("Insert block, then remove within new block");

	pss->seekp(4);
	pss->insert(5);
	pss->write("12345", 5);
	pss->seekp(5);
	pss->remove(3);

	BOOST_CHECK_MESSAGE(is_equal(5, "ABCD15EFGHIJKLMNOPQRSTUVWXYZ"),
		"Insert block, then remove within new block failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_remove_entirely)
{
	BOOST_TEST_MESSAGE("Insert block, then remove around (including) new block");

	pss->seekp(4);
	pss->insert(5);
	pss->write("12345", 5);
	pss->seekp(2);
	pss->remove(9);

	BOOST_CHECK_MESSAGE(is_equal(2, "ABGHIJKLMNOPQRSTUVWXYZ"),
		"Insert block, then remove around (including) new block failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_remove_across_sources_small)
{
	BOOST_TEST_MESSAGE("Insert block, then remove across block boundary (< inserted block size)");

	pss->seekp(4);
	pss->insert(5);
	pss->write("12345", 5);
	pss->seekp(7);
	pss->remove(4);

	BOOST_CHECK_MESSAGE(is_equal(7, "ABCD123GHIJKLMNOPQRSTUVWXYZ"),
		"Insert block, then remove across block boundary (< inserted block size) failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_remove_across_sources_large)
{
	BOOST_TEST_MESSAGE("Insert block, then remove across block boundary (> inserted block size)");

	pss->seekp(4);
	pss->insert(5);
	pss->write("12345", 5);
	pss->seekp(7);
	pss->remove(8);

	BOOST_CHECK_MESSAGE(is_equal(7, "ABCD123KLMNOPQRSTUVWXYZ"),
		"Insert block, then remove across block boundary (> inserted block size) failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_remove_src3)
{
	BOOST_TEST_MESSAGE("Insert block, then remove from third source");

	pss->seekp(5);
	pss->insert(5);
	pss->write("12345", 5);
	pss->seekp(15);
	pss->remove(6);

	BOOST_CHECK_MESSAGE(is_equal(15, "ABCDE12345FGHIJQRSTUVWXYZ"),
		"Insert block, then remove from third source failed");
}

BOOST_AUTO_TEST_CASE(segstream_large_insert)
{
	BOOST_TEST_MESSAGE("Insert large block so third source is pushed past EOF");

	pss->seekp(20);
	pss->insert(10);
	pss->write("1234567890", 10);

	BOOST_CHECK_MESSAGE(is_equal(30, "ABCDEFGHIJKLMNOPQRST1234567890UVWXYZ"),
		"Insert large block so third source is pushed past EOF failed");
}

BOOST_AUTO_TEST_CASE(segstream_large_insert_gap)
{
	BOOST_TEST_MESSAGE("Insert large block so third source is pushed past EOF (with gap)");

	pss->seekp(20);
	pss->insert(15);
	pss->write("1234567890", 10);

	BOOST_CHECK_MESSAGE(is_equal(30,
		makeString(
			"ABCDEFGHIJKLMNOPQRST1234567890\0\0\0\0\0UVWXYZ"
		)),
		"Insert large block so third source is pushed past EOF (with gap) failed");
}

void substreamTruncate(camoto::substream_sptr& sub, camoto::segstream_sptr parent, int len)
{
	boost::iostreams::stream_offset off = sub->getOffset();
	boost::iostreams::stream_offset oldLen = sub->getSize();
	// Enlarge the end of the substream (in the parent)
	std::streamsize delta = len - oldLen+0; //TODO: +1 is temp

	if (delta < 0) {
		// TODO: untested
		parent->seekp(off + oldLen + delta);
		parent->remove(-delta);
	} else {
		parent->seekp(off + oldLen);
		parent->insert(delta);
	}
	// Update the substream with its new size
	sub->setSize(len);
	return;
}

// This reproduces a crash discovered by the fmt-rff-blood archive handler
BOOST_AUTO_TEST_CASE(segstream_insert_past_parent_eof)
{
	BOOST_TEST_MESSAGE("Make segstream commit past parent's EOF");

	pss->seekp(0);

	camoto::substream_sptr base(new camoto::substream(pss, 15, 10));
	camoto::segstream_sptr child(new camoto::segmented_stream(base));
	base->exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
	child->exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

	child->seekp(8);
	child->insert(5);
	child->commit(boost::bind(substreamTruncate, base, pss, _1));

	BOOST_CHECK_MESSAGE(is_equal(-1,
		makeString(
			"ABCDEFGHIJKLMNOPQRSTUVW\0\0\0\0\0XYZ"
		)),
		"Make segstream commit past parent's EOF");
}

BOOST_AUTO_TEST_CASE(segstream_insert_past_parent_eof2)
{
	BOOST_TEST_MESSAGE("Make segstream commit past parent's EOF");

	pss->seekp(0);

	camoto::substream_sptr base(new camoto::substream(pss, 15, 10));
	camoto::segstream_sptr child(new camoto::segmented_stream(base));
	base->exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
	child->exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

	child->seekp(0); // will seek to @15 out of 25 (15+10)
	child->insert(5);
	child->commit(boost::bind(substreamTruncate, base, pss, _1));

	BOOST_CHECK_MESSAGE(is_equal(-1,
		makeString(
			"ABCDEFGHIJKLMNO\0\0\0\0\0PQRSTUVWXYZ"
		)),
		"Make segstream commit past parent's EOF");
}

BOOST_AUTO_TEST_CASE(segstream_insert_c01)
{
	BOOST_TEST_MESSAGE("Insert into first source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(4);
	BOOST_CHECKPOINT("02 About to insert");
	pss->insert(5);
	BOOST_CHECKPOINT("03 About to write");
	pss->write("12345", 5);

	BOOST_REQUIRE_MESSAGE(is_equal(9, "ABCD12345EFGHIJKLMNOPQRSTUVWXYZ"),
		"Insert into first source failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_c02)
{
	BOOST_TEST_MESSAGE("Insert into second source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(4);
	BOOST_CHECKPOINT("02 About to insert");
	pss->insert(5);
	BOOST_CHECKPOINT("03 About to write");
	pss->write("12345", 5);

	// Can't check here or the commit flattens the data and the third source
	// no longer exists.
	/*BOOST_REQUIRE_MESSAGE(is_equal(9, "ABCD12345EFGHIJKLMNOPQRSTUVWXYZ"),
		"Insert into second source failed");*/

	BOOST_CHECKPOINT("04 About to seek");
	pss->seekp(6);
	BOOST_CHECKPOINT("05 About to insert");
	pss->insert(3);
	BOOST_CHECKPOINT("06 About to write");
	pss->write("!@#", 3);

	BOOST_REQUIRE_MESSAGE(is_equal(9, "ABCD12!@#345EFGHIJKLMNOPQRSTUVWXYZ"),
		"Insert into second source failed");
}

BOOST_AUTO_TEST_CASE(segstream_insert_c03)
{
	BOOST_TEST_MESSAGE("Insert into third source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(4);
	BOOST_CHECKPOINT("02 About to insert");
	pss->insert(5);
	BOOST_CHECKPOINT("03 About to write");
	pss->write("12345", 5);

	// Can't check here or the commit flattens the data and the third source
	// no longer exists.
	/*BOOST_REQUIRE_MESSAGE(is_equal(9, "ABCD12345EFGHIJKLMNOPQRSTUVWXYZ"),
		"Insert into second source failed");*/

	BOOST_CHECKPOINT("04 About to seek");
	pss->seekp(15);
	BOOST_CHECKPOINT("05 About to insert");
	pss->insert(3);
	BOOST_CHECKPOINT("06 About to write");
	pss->write("!@#", 3);

	// Do it again (this time it'll be the third source's third source.)
	BOOST_CHECKPOINT("07 About to seek");
	pss->seekp(20);
	BOOST_CHECKPOINT("08 About to insert");
	pss->insert(3);
	BOOST_CHECKPOINT("09 About to write");
	pss->write("$%^", 3);

	BOOST_REQUIRE_MESSAGE(is_equal(23, "ABCD12345EFGHIJ!@#KL$%^MNOPQRSTUVWXYZ"),
		"Insert into third source failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_c01)
{
	BOOST_TEST_MESSAGE("Remove from start of first source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(0);
	BOOST_CHECKPOINT("02 About to remove");
	pss->remove(5);

	BOOST_REQUIRE_MESSAGE(is_equal(0,
		makeString(
			"FGHIJKLMNOPQRSTUVWXYZ"
		)),
		"Remove from start of first source failed");

	BOOST_CHECKPOINT("03 About to remove");
	pss->remove(5);

	BOOST_REQUIRE_MESSAGE(is_equal(0,
		makeString(
			"KLMNOPQRSTUVWXYZ"
		)),
		"Second removal from start of first source failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_c02)
{
	BOOST_TEST_MESSAGE("Remove data from middle of stream");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(20);
	BOOST_CHECKPOINT("02 About to remove");
	pss->remove(5);

	BOOST_REQUIRE_MESSAGE(is_equal(20, "ABCDEFGHIJKLMNOPQRSTZ"),
		"Remove data from middle of stream failed");

	BOOST_CHECKPOINT("03 About to seek");
	pss->seekp(5);
	BOOST_CHECKPOINT("04 About to remove");
	pss->remove(6);

	BOOST_REQUIRE_MESSAGE(is_equal(5, "ABCDELMNOPQRSTZ"),
		"Remove data from middle of stream failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_c03)
{
	BOOST_TEST_MESSAGE("Remove data within third source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(10);
	BOOST_CHECKPOINT("02 About to remove");
	pss->remove(5);

	// Can't check here or the commit flattens the data and the third source
	// no longer exists.
	/*BOOST_REQUIRE_MESSAGE(is_equal(10, "ABCDEFGHIJPQRSTUVWXYZ"),
		"Remove data from middle of first source failed");*/

	BOOST_CHECKPOINT("03 About to seek");
	pss->seekp(15);
	BOOST_CHECKPOINT("04 About to remove");
	pss->remove(5);

	BOOST_REQUIRE_MESSAGE(is_equal(15, "ABCDEFGHIJPQRSTZ"),
		"Remove data within third source failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_c04)
{
	BOOST_TEST_MESSAGE("Remove data up to end of first source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(20);
	BOOST_CHECKPOINT("02 About to remove");
	pss->remove(6);

	BOOST_REQUIRE_MESSAGE(is_equal(20, "ABCDEFGHIJKLMNOPQRST"),
		"Remove data up to end of first source failed");

	BOOST_CHECKPOINT("03 About to seek");
	pss->seekp(15);
	BOOST_CHECKPOINT("04 About to remove");
	pss->remove(5);

	BOOST_REQUIRE_MESSAGE(is_equal(15, "ABCDEFGHIJKLMNO"),
		"Second removal up to end of first source failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_c05)
{
	BOOST_TEST_MESSAGE("Remove entire second source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(10);
	BOOST_CHECKPOINT("02 About to insert");
	pss->insert(5);
	BOOST_CHECKPOINT("03 About to write into inserted space");
	pss->write("12345", 5);

	// Can't check here or the commit flattens the data and the third source
	// no longer exists.
	/*BOOST_REQUIRE_MESSAGE(is_equal(15, "ABCDEFGHIJ12345KLMNOPQRSTUVWXYZ"),
		"Removing entire second source failed during set up phase");*/

	BOOST_CHECKPOINT("04 About to seek");
	pss->seekp(10);
	BOOST_CHECKPOINT("05 About to remove");
	pss->remove(5);

	BOOST_REQUIRE_MESSAGE(is_equal(10, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
		"Removing entire second source failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_c06)
{
	BOOST_TEST_MESSAGE("Remove start of second source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(10);
	BOOST_CHECKPOINT("02 About to insert");
	pss->insert(5);
	BOOST_CHECKPOINT("03 About to write into inserted space");
	pss->write("12345", 5);

	// Can't check here or the commit flattens the data and the third source
	// no longer exists.
	/*BOOST_REQUIRE_MESSAGE(is_equal(15, "ABCDEFGHIJ12345KLMNOPQRSTUVWXYZ"),
		"Removing entire second source failed during set up phase");*/

	BOOST_CHECKPOINT("04 About to seek");
	pss->seekp(10);
	BOOST_CHECKPOINT("05 About to remove");
	pss->remove(3);

	BOOST_REQUIRE_MESSAGE(is_equal(10, "ABCDEFGHIJ45KLMNOPQRSTUVWXYZ"),
		"Removing start of second source failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_c07)
{
	BOOST_TEST_MESSAGE("Remove end of second source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(10);
	BOOST_CHECKPOINT("02 About to insert");
	pss->insert(5);
	BOOST_CHECKPOINT("03 About to write into inserted space");
	pss->write("12345", 5);

	// Can't check here or the commit flattens the data and the third source
	// no longer exists.
	/*BOOST_REQUIRE_MESSAGE(is_equal(15, "ABCDEFGHIJ12345KLMNOPQRSTUVWXYZ"),
		"Removing entire second source failed during set up phase");*/

	BOOST_CHECKPOINT("04 About to seek");
	pss->seekp(12);
	BOOST_CHECKPOINT("05 About to remove");
	pss->remove(3);

	// Do it again
	BOOST_CHECKPOINT("04 About to seek");
	pss->seekp(11);
	BOOST_CHECKPOINT("05 About to remove");
	pss->remove(1);

	BOOST_REQUIRE_MESSAGE(is_equal(11, "ABCDEFGHIJ1KLMNOPQRSTUVWXYZ"),
		"Removing end of second source failed");
}

BOOST_AUTO_TEST_CASE(segstream_remove_c08)
{
	BOOST_TEST_MESSAGE("Remove middle of second source");

	BOOST_CHECKPOINT("01 About to seek");
	pss->seekp(10);
	BOOST_CHECKPOINT("02 About to insert");
	pss->insert(5);
	BOOST_CHECKPOINT("03 About to write into inserted space");
	pss->write("12345", 5);

	// Can't check here or the commit flattens the data and the third source
	// no longer exists.
	/*BOOST_REQUIRE_MESSAGE(is_equal(15, "ABCDEFGHIJ12345KLMNOPQRSTUVWXYZ"),
		"Removing entire second source failed during set up phase");*/

	BOOST_CHECKPOINT("04 About to seek");
	pss->seekp(11);
	BOOST_CHECKPOINT("05 About to remove");
	pss->remove(2);
	// Do it again
	BOOST_CHECKPOINT("06 About to remove");
	pss->remove(1);

	BOOST_REQUIRE_MESSAGE(is_equal(11, "ABCDEFGHIJ15KLMNOPQRSTUVWXYZ"),
		"Removing middle of second source failed");
}

BOOST_AUTO_TEST_SUITE_END() // segmented_stream_suite
