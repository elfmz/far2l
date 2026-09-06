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


class statusbar_ctl : public screen_ctl
{
public:
	//From screen_ctl
	void initialize();

	/**
	 * Resize screen control buffer
	 * \param width new width size
	 */
	void resize(const size_t width);

	/**
	 * Write file name to the status line
	 * \param file_name file name
	 */
	void write_filename(const wchar_t* file_name);

	/**
	 * Write mode flag to the status line
	 * \param rw true for RW, false for RO file modes
	 */
	void write_mode_flag(const bool rw);

	/**
	 * Write 'updated' flag state to the status line
	 * \param upd new state flag
	 */
	void write_update_flag(const bool upd);

	/**
	 * Write code page number to the status line
	 * \param cp current code page number
	 */
	void write_codepage(const UINT cp);

	/**
	 * Write cursor position to the status line
	 * \param pos position as offset
	 */
	void write_offset(const UINT64 pos);

	/**
	 * Write position percent to the status line
	 * \param percent percent position
	 */
	void write_position(const unsigned char percent);
};
