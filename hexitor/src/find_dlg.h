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


class find_dlg
{
public:
	/**
	 * Show 'find' dialog
	 * \param seq search sequence
	 * \param forward_search forward search flag
	 * \return false if dialog canceled
	 */
	bool show(vector<unsigned char>& seq, bool& forward_search);

private:
	//Field fillers
	void fill_hex();
	void fill_mb(const UINT cp, const uintptr_t item_id) noexcept;
	void fill_ans() noexcept;
	void fill_oem() noexcept;
	void fill_u16();
	void fill_u8() noexcept;

	//Far dialog's callback
	static intptr_t WINAPI dlg_proc(HANDLE dlg, intptr_t msg, int param1, void* param2);

private:
	HANDLE			_dialog;		///< Dialog window handle
	vector<unsigned char>	_seq;
	bool		_can_update;
};