/**
 * @file   test-stream_filtered.cpp
 * @brief  Test code for filteredstream class.
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
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
#include <boost/bind.hpp>
#include <camoto/stream.hpp>
#include <camoto/stream_string.hpp>
#include <camoto/stream_filtered.hpp>
#include <camoto/filter_dummy.hpp>
#include "tests.hpp"

using namespace camoto;

BOOST_FIXTURE_TEST_SUITE(stream_filtered_suite, string_sample)

BOOST_AUTO_TEST_CASE(stream_filtered_read)
{
	BOOST_TEST_MESSAGE("Read from stream_filtered");

	this->in << "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	filter_sptr algo(new filter_dummy());
	stream::filtered_sptr f(new stream::filtered());
	f->open(this->in, algo, algo, NULL);

	f->seekg(10, stream::start);
	stream::copy(this->out, f);
	BOOST_REQUIRE_EQUAL(f->tellg(), 26);

	BOOST_CHECK_MESSAGE(is_equal("KLMNOPQRSTUVWXYZ"),
		"Read from stream_filtered failed");
}

BOOST_AUTO_TEST_CASE(stream_filtered_write)
{
	BOOST_TEST_MESSAGE("Write to stream_filtered");

	this->out << "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	filter_sptr algo(new filter_dummy());
	stream::filtered_sptr f(new stream::filtered());
	f->open(this->out, algo, algo, NULL);

	f->seekp(10, stream::start);
	f->write("1234567890", 10);
	f->flush();

	BOOST_CHECK_MESSAGE(is_equal("ABCDEFGHIJ1234567890UVWXYZ"),
		"Write to stream_filtered failed");
}

BOOST_AUTO_TEST_CASE(stream_filtered_postflush_write)
{
	BOOST_TEST_MESSAGE("Write, flush, write to stream_filtered");

	this->out << "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	filter_sptr algo(new filter_dummy());
	stream::filtered_sptr f(new stream::filtered());
	f->open(this->out, algo, algo, NULL);

	f->seekp(10, stream::start);
	f->write("1234567890", 10);
	f->flush();
	f->seekp(5, stream::start);
	f->write("!@#$%^&*()", 10);
	f->flush();

	BOOST_CHECK_MESSAGE(is_equal("ABCDE!@#$%^&*()67890UVWXYZ"),
		"Write, flush, write to stream_filtered failed");
}

BOOST_AUTO_TEST_CASE(stream_filtered_read_write)
{
	BOOST_TEST_MESSAGE("Write to stream_filtered");

	this->out << "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	filter_sptr algo(new filter_dummy());
	stream::filtered_sptr f(new stream::filtered());
	f->open(this->out, algo, algo, NULL);

	f->seekp(10, stream::start);
	f->write("1234567890", 10);
	f->flush();

	BOOST_CHECK_MESSAGE(is_equal("ABCDEFGHIJ1234567890UVWXYZ"),
		"Write to stream_filtered failed");

	f->seekg(5, stream::start);

	char buf[10];
	f->read(buf, 10);
	BOOST_REQUIRE_EQUAL(f->tellg(), 15);

	BOOST_CHECK_MESSAGE(is_equal(std::string("FGHIJ12345"), std::string(buf, 10)),
		"Read from stream_filtered failed");
}

void setVar(stream::len *var, stream::len val)
{
	*var = val;
	return;
}

BOOST_AUTO_TEST_CASE(double_stream_filtered_write)
{
	BOOST_TEST_MESSAGE("Write to double stream_filtered");

	this->out << "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	this->out->seekg(0, stream::start);

	stream::len lenF = 0, lenH = 0;

	stream::fn_truncate fnF = boost::bind<void>(setVar, &lenF, _1);
	stream::fn_truncate fnH = boost::bind<void>(setVar, &lenH, _1);

	filter_sptr algo(new filter_dummy());
	stream::filtered_sptr f(new stream::filtered());
	f->open(this->out, algo, algo, fnF);

	f->seekp(10, stream::start);
	f->write("1234567890", 10);
	f->truncate(25);
	f->flush();
	BOOST_REQUIRE_EQUAL(lenF, 25);
	BOOST_REQUIRE_EQUAL(f->size(), 25);

	filter_sptr algoH(new filter_dummy());
	stream::filtered_sptr h(new stream::filtered());
	h->open(f, algoH, algoH, fnH);

	h->seekp(11, stream::start);
	h->write("!@#$%", 5);
	h->truncate(24);
	h->flush();
	BOOST_REQUIRE_EQUAL(lenH, 24);
	BOOST_REQUIRE_EQUAL(h->size(), 24);

	// Make sure it's updated the parent stream
	BOOST_REQUIRE_EQUAL(lenF, 24);

	BOOST_CHECK_MESSAGE(is_equal("ABCDEFGHIJ1!@#$%7890UVWX"),
		"Write to double stream_filtered failed");
}

BOOST_AUTO_TEST_SUITE_END()
