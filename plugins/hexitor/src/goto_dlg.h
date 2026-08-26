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
#include "fardialog.h"


class goto_dlg
{
public:
	/**
	 * Show 'goto' dialog
	 * \param file_size file size
	 * \param offset offset value
	 * \return false if dialog canceled
	 */
	bool show(const UINT64 file_size, UINT64& offset);

private:
	/**
	 * Get current offset value
	 * \return current offset value
	 */
	UINT64 get_val() const;

	//Far dialog's callback
	LONG_PTR WINAPI dlg_proc(HANDLE dlg, int msg, int param1, LONG_PTR param2);

private:
	UINT64	_file_size;
	fardialog::Dialog *myDialog = nullptr;
};
