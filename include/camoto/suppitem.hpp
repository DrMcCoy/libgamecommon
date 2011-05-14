/**
 * @file   suppitem.hpp
 * @brief  Supplemental files required to open other files.
 *
 * Copyright (C) 2010-2011 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef _CAMOTO_SUPPITEM_HPP_
#define _CAMOTO_SUPPITEM_HPP_

#include <vector>
#include <map>

#include <camoto/types.hpp>

/// Main namespace
namespace camoto {

/// Supplementary item.
/**
 * This class contains data about a supplementary item required to open a
 * particular type of file.
 *
 * @see TilesetType::getRequiredSupps(), ImageType::getRequiredSupps()
 */
struct SuppItem {

	/// Type of supplemental file.
	enum Type {

		// Common
		Dictionary,  ///< Compression dictionary is external

		// Archives
		FAT,         ///< FAT is stored externally

		// Images
		Palette,     ///< Palette data

		// Music
		Instruments, ///< Instrument patches/settings

	};

	/// The stream containing the supplemental data.
	iostream_sptr stream;

	/// The truncate callback (required)
	FN_TRUNCATE fnTruncate;

};

/// A list of required supplemental files and their filenames.
typedef std::map<SuppItem::Type, std::string> SuppFilenames;

/// A list of the supplemental file types mapped to open file streams.
typedef std::map<SuppItem::Type, SuppItem> SuppData;

} // namespace camoto

#endif // _CAMOTO_SUPPITEM_HPP_