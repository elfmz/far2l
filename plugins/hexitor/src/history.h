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

#include "common.h"


//! Position history.
class history
{
public:
	history();

	/**
	 * Load last saved position in file.
	 * \param file_name edited file name
	 * \param view_offset last saved view position in file
	 * \param cursor_offset last saved cursor position in file
	 * \return false history not found for file
	 */
	bool load_last_position(const wchar_t* file_name, UINT64& view_offset, UINT64& cursor_offset);

	/**
	 * Save last position in file.
	 * \param file_name edited file name
	 * \param pos position in file
	 */
	void save_last_position(const wchar_t* file_name, const UINT64 view_offset, const UINT64 cursor_offset);

private:
	//! Load history.
	void load();

	//! Save history.
	void save() const;

private:
	//! History description.
	struct hist {
		time_t	datetime;
		UINT64	offset_view;
		UINT64	offset_cursor;
	};

	map<wstring, hist>	_history;	///< History map (file name to description)
};
