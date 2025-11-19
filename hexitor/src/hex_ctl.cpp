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

#include "hex_ctl.h"
#include "settings.h"

#define HEX_CTL_WIDTH	80		//Hex control width size


hex_ctl::hex_ctl()
:	_codepage(GetACP())
{
}


void hex_ctl::initialize()
{
	//Get colors from Far settings
	_PSI.AdvControl(_PSI.ModuleNumber, ACTL_GETCOLOR, (void*)COL_EDITORTEXT, &_default_color);
}


void hex_ctl::resize(const size_t width, const size_t height)
{
	resize_buffer(width, height);
}


void hex_ctl::update(const UINT64 offset, const vector<BYTE>& ori_data, const map<UINT64, BYTE>& upd_data, const UINT64 cursor, const bool hex_area)
{
	assert(_height * 0x10 >= ori_data.size());
	assert(!(offset % 0x10));
	assert(cursor >= offset && cursor < offset + ori_data.size());

	//Reset content and colors
	reset();

	if (_codepage == CP_UTF8) {
		_byte_to_col_map.assign(showed_data_size(), -1);
	}

	const size_t length = ori_data.size();

	//Fill buffer
	const size_t total_row = length / 16;
	for (size_t row = 0; row <= total_row && row * 16 < length; ++row) {

		//Offset address
		wchar_t offset_txt[16];
		const int offset_len = swprintf(offset_txt, ARRAYSIZE(offset_txt), L"%012llX:", offset + static_cast<UINT64>(row) * 0x10);
		write(row, 0, offset_txt, offset_len);

		//Hex values area (common for all codepages)
		for (size_t col = 0; col < 16 && row * 16 + col < length; ++col) {
			map<UINT64, BYTE>::const_iterator itup = upd_data.find(offset + static_cast<UINT64>(row) * 0x10 + col);
			const BYTE val = itup == upd_data.end() ? ori_data[row * 0x10 + col] : itup->second;

			//Hex value
			wchar_t hex_val[3];
			const int hex_val_len = swprintf(hex_val, ARRAYSIZE(hex_val), L"%02X", val);
			write(row, 15 + col * 3, hex_val, hex_val_len);

			 //Set color for updated data
			if (itup != upd_data.end()) {
				write(row, 15 + col * 3 + 0, settings.clr_updated);
				write(row, 15 + col * 3 + 1, settings.clr_updated);
			}
		}

		//Text values area (depends on codepage)
		if (_codepage == CP_UTF8) {
			size_t text_col = 0;
			for (size_t col = 0; col < 16 && row * 16 + col < length; ) {
				const UINT64 current_offset_abs = offset + static_cast<UINT64>(row) * 0x10 + col;
				const size_t current_offset_rel = row * 16 + col;

				BYTE sequence[4];
				bool is_updated = false;

				map<UINT64, BYTE>::const_iterator itup = upd_data.find(current_offset_abs);
				sequence[0] = (itup == upd_data.end()) ? ori_data[current_offset_rel] : itup->second;
				if (itup != upd_data.end()) is_updated = true;

				int expected_len = 0;
				if ((sequence[0] & 0x80) == 0)      expected_len = 1;
				else if ((sequence[0] & 0xE0) == 0xC0) expected_len = 2;
				else if ((sequence[0] & 0xF0) == 0xE0) expected_len = 3;
				else if ((sequence[0] & 0xF8) == 0xF0) expected_len = 4;

				if (expected_len == 0 || col + expected_len > 16 || current_offset_rel + expected_len > length) {
					expected_len = 1; // Invalid start byte or truncated sequence
				}

				for (int i = 1; i < expected_len; ++i) {
					itup = upd_data.find(current_offset_abs + i);
					sequence[i] = (itup == upd_data.end()) ? ori_data[current_offset_rel + i] : itup->second;
					if (itup != upd_data.end()) is_updated = true;
				}

				wchar_t pval[2] = {0};
				int converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)sequence, expected_len, pval, 1);

				wchar_t char_to_write = L'.';
				int bytes_consumed = 1;

				if (converted > 0 && pval[0] != 0 && !iswcntrl(pval[0])) {
					char_to_write = pval[0];
					bytes_consumed = expected_len;
				}

				if (text_col < 16) {
					write(row, 64 + text_col, char_to_write);
					if (is_updated) write(row, 64 + text_col, settings.clr_updated);
				}

				for (int i=0; i < bytes_consumed; ++i) {
					if (current_offset_rel + i < _byte_to_col_map.size()) {
						_byte_to_col_map[current_offset_rel + i] = text_col;
					}
				}

				col += bytes_consumed;
				text_col++;
			}
		} else { // Other codepages (ANSI, OEM, UTF-16)
			for (size_t col = 0; col < 16 && row * 16 + col < length; ++col) {
				const UINT64 current_offset_abs = offset + static_cast<UINT64>(row) * 0x10 + col;
				map<UINT64, BYTE>::const_iterator itup = upd_data.find(current_offset_abs);
				const BYTE val = itup == upd_data.end() ? ori_data[row * 16 + col] : itup->second;

				if (_codepage != CP_UTF16LE) {
					wchar_t pval = L' ';
					MultiByteToWideChar(_codepage, 0, reinterpret_cast<LPCSTR>(&val), 1, &pval, 1);
					write(row, 64 + col, pval);
				}
				else if (col % 2 == 0) {
					map<UINT64, BYTE>::const_iterator itup2 = upd_data.find(current_offset_abs + 1);
					const BYTE val_second = (itup2 != upd_data.end() ? itup2->second : row * 16 + col + 1 < ori_data.size() ? ori_data[row * 16 + col + 1] : 0);
					write(row, 64 + col / 2, (wchar_t)MAKEWORD(val, val_second));
				}
				if (itup != upd_data.end()) {
					write(row, 64 + col / (_codepage == CP_UTF16LE ? 2 : 1), settings.clr_updated);
				}
			}
		}

		//Separators
		if (settings.show_dword_seps) {
			for (size_t i = 0; i < 3; ++i)
				write(row, 26 + i * 12, (uint64_t)0x2502);
		}
	}

	//Highlight cursor position
	if (!hex_area && _codepage == CP_UTF8)
	{
		// UTF-8 multi-byte character highlighting
		UINT64 start_char_offset = cursor;
		size_t rel_offset = static_cast<size_t>(start_char_offset - offset);

		// Find the beginning of the character by moving backwards
		int limit = 6; // Sane limit for backward search
		while (rel_offset > 0 && limit-- > 0 && (ori_data[rel_offset] & 0xC0) == 0x80) {
			start_char_offset--;
			rel_offset--;
		}

		// Determine character length
		BYTE first_byte = (start_char_offset >= offset) ? ori_data[rel_offset] : 0;
		if (upd_data.count(start_char_offset)) {
			first_byte = upd_data.at(start_char_offset);
		}

		int char_len = get_utf8_char_len(first_byte);

		// Highlight all bytes of the character in the hex area
		for (int i = 0; i < char_len; ++i) {
			if (start_char_offset + i < offset + ori_data.size()) {
				const COORD pos_hex_byte = cursor_from_offset(offset, start_char_offset + i, true);
				write(pos_hex_byte.Y, pos_hex_byte.X + 0, settings.clr_active);
				write(pos_hex_byte.Y, pos_hex_byte.X + 1, settings.clr_active);
			}
		}

		// Highlight the character in the text area
		const COORD pos_txt = cursor_from_offset(offset, cursor, false);
		write(pos_txt.Y, pos_txt.X, settings.clr_active);
	}
	else
	{
		// Standard single-byte highlighting
		const COORD pos_hex = cursor_from_offset(offset, cursor, true);
		write(pos_hex.Y, pos_hex.X + 0, settings.clr_active);
		write(pos_hex.Y, pos_hex.X + 1, settings.clr_active);
		if (_codepage == CP_UTF16LE && !hex_area) {
			write(pos_hex.Y, pos_hex.X + 3, settings.clr_active);
			write(pos_hex.Y, pos_hex.X + 4, settings.clr_active);
		}
		const COORD pos_txt = cursor_from_offset(offset, cursor, false);
		write(pos_txt.Y, pos_txt.X, settings.clr_active);
	}
}


COORD hex_ctl::cursor_from_offset(const UINT64 start_offset, const UINT64 cursor_offset, const bool hex_area) const
{
	assert(cursor_offset >= start_offset && cursor_offset < start_offset + showed_data_size());

	COORD coord = { 0, 0 };

	const size_t rel_offset = static_cast<size_t>(cursor_offset - start_offset);
	coord.Y = static_cast<SHORT>(rel_offset / 0x10);
	const SHORT col_byte = static_cast<SHORT>(rel_offset % 0x10);

	if (hex_area) {
		coord.X = 15 + 3 * col_byte;
	} else {
		if (_codepage == CP_UTF8) {
			coord.X = 64;
			if (rel_offset < _byte_to_col_map.size() && _byte_to_col_map[rel_offset] != -1) {
				coord.X += _byte_to_col_map[rel_offset];
			}
		} else {
			coord.X = 64 + col_byte / (_codepage == CP_UTF16LE ? 2 : 1);
		}
	}

	return coord;
}


bool hex_ctl::offset_from_cursor(const UINT64 start_offset, const COORD& coord, UINT64& offset, bool& first_part, bool& hex_area) const
{
	if (coord.X < 15 || coord.X > HEX_CTL_WIDTH || (coord.X >= 62 && coord.X <= 63))
		return false;

	hex_area = (coord.X <= 61);
	if (!hex_area) {
		first_part = true;
		if (_codepage == CP_UTF8) {
			const size_t target_text_col = coord.X - 64;
			const size_t line_start_rel_offset = coord.Y * 16;
			bool found = false;
			for(size_t i = 0; i < 16; ++i) {
				const size_t rel_offset = line_start_rel_offset + i;
				if (rel_offset < _byte_to_col_map.size() && _byte_to_col_map[rel_offset] == static_cast<int>(target_text_col)) {
					offset = start_offset + rel_offset;
					found = true;
					break;
				}
			}
			if (!found) return false;
		} else {
			offset = start_offset + (coord.X - 64) * (_codepage == CP_UTF16LE ? 2 : 1) + coord.Y * 0x10;
		}
	}
	else {
		offset = start_offset + (coord.X - 15) / 3 + coord.Y * 0x10;
		first_part = (coord.X % 3 == 0);
	}

	return true;
}


UINT hex_ctl::switch_codepage()
{
	if (_codepage == GetACP())
		_codepage = GetOEMCP();
	else if (_codepage == GetOEMCP())
		_codepage = CP_UTF8;
	else if (_codepage == CP_UTF8)
		_codepage = CP_UTF16LE;
	else // CP_UTF16LE
		_codepage = GetACP();

	return _codepage;
}


void hex_ctl::reset()
{
	for (size_t row = 0; row < _height; ++row) {
		for (size_t col = 0; col < _width; ++col) {	//Content
			write(row, col, L' ');
			write(row, col, _default_color);
		}
		for (size_t col = 0; col < 13; ++col)		//Offsets
			write(row, col, settings.clr_offset);
		for (size_t col = 0; col < 3; ++col)		//Separators
			write(row, 26 + col * 12, settings.clr_offset);
	}
}