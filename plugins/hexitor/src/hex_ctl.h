/**************************************************************************
 *  Hexitor plug-in for FAR 3.0 modifed by m32 2024 for far2l             *
 *  Copyright (C) 2010-2014 by Artem Senichev <artemsen@gmail.com>        *
 *  https://sourceforge.net/projects/farplugs/                            *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#pragma once

#include "screen_ctl.h"

#define CP_UTF16LE		1200	//Unicode LE code page
#define CP_UTF8			65001	//UTF-8 code page

// Helper to get the length of a UTF-8 character from its first byte
static inline int get_utf8_char_len(BYTE b)
{
	if ((b & 0x80) == 0) return 1;
	if ((b & 0xE0) == 0xC0) return 2;
	if ((b & 0xF0) == 0xE0) return 3;
	if ((b & 0xF8) == 0xF0) return 4;
	return 1; // Invalid start byte, treat as 1 byte
}

class hex_ctl : public screen_ctl
{
public:
	hex_ctl();

	//From screen_ctl
	void initialize();

	/**
	 * Resize screen control buffer
	 * \param width new width size
	 * \param height new height size
	 */
	void resize(const size_t width, const size_t height);

	/**
	 * Update content buffer
	 * \param offset start offset value
	 * \param ori_data original data array
	 * \param upd_data updated data map
	 * \param cursor cursor position (offset)
	 * \param hex_area cursor in hex area flag
	 */
	void update(const UINT64 offset, const vector<BYTE>& ori_data, const map<UINT64, BYTE>& upd_data, const UINT64 cursor, const bool hex_area);

	/**
	 * Calculate display cursor coordinates from offset
	 * \param start_offset start (top visible) offset
	 * \param cursor_offset cursor offset
	 * \param hex_area true if cursor must be in hex editor area
	 * \return display coordinates (col, row)
	 */
	COORD cursor_from_offset(const UINT64 start_offset, const UINT64 cursor_offset, const bool hex_area) const;

	/**
	 * Calculate offset from cursor coordinates
	 * \param start_offset start (top visible) offset
	 * \param coord cursor coordinates
	 * \param offset offset
	 * \param first_part byte part (if it is a hex area)
	 * \param hex_area field type
	 * \return false if unable to calculate offset
	 */
	bool offset_from_cursor(const UINT64 start_offset, const COORD& coord, UINT64& offset, bool& first_part, bool& hex_area) const;

	/**
	 * Switch code page
	 * \return new code page number
	 */
	UINT switch_codepage();

	/**
	 * Get current code page
	 * \return code page number
	 */
	inline UINT get_codepage() const	{ return _codepage; }

	/**
	 * Get max data size for show
	 * \return max data
	 */
	inline size_t showed_data_size() const	{ return _height * 0x10; }

private:
	/**
	 * Reset buffer (set default content and colors)
	 */
	void reset();
private:
	UINT	_codepage;		///< Code page
	vector<int> _byte_to_col_map; ///< Map byte offset in view to text column for UTF-8
};