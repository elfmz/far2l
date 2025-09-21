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

#include "statusbar_ctl.h"
#include "version.h"


void statusbar_ctl::initialize()
{
	//Get color from Far settings
	_PSI.AdvControl(_PSI.ModuleNumber, ACTL_GETCOLOR, (void*)COL_EDITORSTATUS, &_default_color);
}


void statusbar_ctl::resize(const size_t width)
{
	resize_buffer(width, 1 /* Always one line only */);

	for (size_t i = 80; i < _width; ++i)
		write(0, i, L' ');
}


void statusbar_ctl::write_filename(const wchar_t* file_name)
{
	assert(file_name && *file_name);

	const size_t max_len = 48;
	wstring showed_file_name = file_name;
	if (showed_file_name.length() > max_len)
		showed_file_name = wstring(L"...") + showed_file_name.substr(showed_file_name.length() - max_len + 3);
	write(0, 0, showed_file_name.c_str(), showed_file_name.length());
}


void statusbar_ctl::write_mode_flag(const bool rw)
{
	write(0, 50, rw ? L"RW" : L"RO", 2);
}


void statusbar_ctl::write_update_flag(const bool upd)
{
	write(0, 53, upd ? L'*' : L' ');
}


void statusbar_ctl::write_codepage(const UINT cp)
{
	wchar_t num[8];
	const int num_len = swprintf(num, sizeof(num) / sizeof(num[0]), L"%d", cp);
	for (size_t i = 56; i < 60; ++i)
		write(0, i, L' ');
	write(0, 60 - num_len, num, num_len);
}


void statusbar_ctl::write_offset(const UINT64 pos)
{
	wchar_t num[16];
	const int num_len = swprintf(num, sizeof(num) / sizeof(num[0]), L"0x%012llX", pos);
	write(0, 61, num, num_len);
}


void statusbar_ctl::write_position(const unsigned char percent)
{
	assert(percent <= 100);

	wchar_t num[8];
	const int num_len = swprintf(num, sizeof(num) / sizeof(num[0]), L"%d%%", percent);
	for (size_t i = 76; i < 80; ++i)
		write(0, i, L' ');
	write(0, 80 - num_len, num, num_len);
}
