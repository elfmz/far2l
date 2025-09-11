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

	const size_t length = ori_data.size();

	//Fill buffer
	const size_t total_row = length / 16;
	for (size_t row = 0; row <= total_row && row * 16 < length; ++row) {

		//Offset address
		wchar_t offset_txt[16];
		const int offset_len = swprintf(offset_txt, sizeof(offset_txt) / sizeof(offset_txt[0]), L"%012llX:", offset + static_cast<UINT64>(row) * 0x10);
		write(row, 0, offset_txt, offset_len);

		//Edit area
		for (size_t col = 0; col < 16 && row * 16 + col < length; ++col) {
			map<UINT64, BYTE>::const_iterator itup = upd_data.find(offset + static_cast<UINT64>(row) * 0x10 + col);
			const BYTE val = itup == upd_data.end() ? ori_data[row * 0x10 + col] : itup->second;

			//Hex value
			wchar_t hex_val[3];
			const int hex_val_len = swprintf(hex_val, sizeof(hex_val) / sizeof(offset_txt[0]), L"%02X", val);
			write(row, 15 + col * 3, hex_val, hex_val_len);

			//Print value
			if (_codepage != CP_UTF16LE) {
				wchar_t pval = L' ';
				MultiByteToWideChar(_codepage, 0, reinterpret_cast<LPCSTR>(&val), 1, &pval, 1);
				write(row, 64 + col, pval);
			}
			else if (col % 2 == 0) {
				map<UINT64, BYTE>::const_iterator itup2 = upd_data.find(offset + static_cast<UINT64>(row) * 0x10 + col + 1);
				const BYTE val_second = (itup2 != upd_data.end() ? itup2->second : row * 16 + col + 1 < ori_data.size() ? ori_data[row * 16 + col + 1] : 0);
				write(row, 64 + col / 2, (uint64_t)MAKEWORD(val, val_second));
			}

			 //Set color for updated data
			if (itup != upd_data.end()) {
				write(row, 15 + col * 3 + 0, settings::clr_updated);
				write(row, 15 + col * 3 + 1, settings::clr_updated);
				write(row, 64 + col / (_codepage == CP_UTF16LE ? 2 : 1), settings::clr_updated);
			}
		}

		//Separators
		if (settings::show_dword_seps) {
			for (size_t i = 0; i < 3; ++i)
				write(row, 26 + i * 12, (uint64_t)0x2502);
		}
	}

	//Highlight cursor position
	const COORD pos_hex = cursor_from_offset(offset, cursor, true);
	write(pos_hex.Y, pos_hex.X + 0, settings::clr_active);
	write(pos_hex.Y, pos_hex.X + 1, settings::clr_active);
	if (_codepage == CP_UTF16LE && !hex_area) {
		write(pos_hex.Y, pos_hex.X + 3, settings::clr_active);
		write(pos_hex.Y, pos_hex.X + 4, settings::clr_active);
	}
	const COORD pos_txt = cursor_from_offset(offset, cursor, false);
	write(pos_txt.Y, pos_txt.X, settings::clr_active);
}


COORD hex_ctl::cursor_from_offset(const UINT64 start_offset, const UINT64 cursor_offset, const bool hex_area) const
{
	assert(cursor_offset >= start_offset && cursor_offset < start_offset + _height * 0x10);

	COORD coord = { 0, 0 };

	const SHORT top_offset = static_cast<SHORT>(cursor_offset - start_offset);
	coord.Y = top_offset / 0x10;
	const SHORT col = top_offset - coord.Y * 0x10;
	coord.X = hex_area ? 15 + 3 * col : 64 + col / (_codepage == CP_UTF16LE ? 2 : 1);

	return coord;
}


bool hex_ctl::offset_from_cursor(const UINT64 start_offset, const COORD& coord, UINT64& offset, bool& first_part, bool& hex_area) const
{
	if (coord.X < 15 || coord.X > HEX_CTL_WIDTH || (coord.X >= 62 && coord.X <= 63))
		return false;

	hex_area = (coord.X <= 61);
	if (!hex_area) {
		offset = start_offset + (coord.X - 64) * (_codepage == CP_UTF16LE ? 2 : 1) + coord.Y * 0x10;
		first_part = true;
	}
	else {
		offset = start_offset + (coord.X - 15) / 3 + coord.Y * 0x10;
		first_part = (coord.X % 3 == 0);
	}

	return true;
}


UINT hex_ctl::switch_codepage()
{
	if (_codepage == CP_UTF16LE)
		_codepage = GetACP();
	else if (_codepage == GetACP())
		_codepage = GetOEMCP();
	else
		_codepage = CP_UTF16LE;

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
			write(row, col, settings::clr_offset);
		for (size_t col = 0; col < 3; ++col)		//Separators
			write(row, 26 + col * 12, settings::clr_offset);
	}
}
