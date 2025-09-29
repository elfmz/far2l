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


class settings
{
public:
	/**
	 * Load settings.
	 */
	static void load();

	/**
	 * Configure settings
	 */
	static void configure();

private:
	/**
	 * Save settings.
	 */
	static void save();

	//Far dialog's callback
	static LONG_PTR WINAPI dlg_proc(HANDLE dlg, int Msg, int Param1, LONG_PTR Param2);

public:
	static bool add_to_panel_menu;		///< Add plugin to the panel plugin menu flag
	static bool add_to_editor_menu;		///< Add plugin to the editor plugin menu flag
	static bool add_to_viewer_menu;		///< Add plugin to the viewer plugin menu flag
	static wstring cmd_prefix;			///< Plugin command prefix
	static bool save_file_pos;			///< Save file position
	static bool show_dword_seps;		///< Show separators between DWORDs
	static bool move_inside_byte;		///< Move cursor inside the byte
	static bool std_cursor_size;		///< Use standard cursor size
	static FarColor clr_active;			///< Highlight color for active position
	static FarColor clr_updated;			///< Highlight color for updated position
	static FarColor clr_offset;			///< Highlight color for offset and separators
	static void *Dialog;
};
