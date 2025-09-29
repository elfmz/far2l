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
#include "file.h"
#include "statusbar_ctl.h"
#include "hex_ctl.h"
#include "keybar_ctl.h"


class editor
{
public:
	editor();

	/**
	 * Edit file.
	 * \param file_name source file name
	 * \param file_offset file offset to set cursor
	 * \return true if file content has been updated
	 */
	bool edit(const wchar_t* file_name, const UINT64 file_offset = 0);

private:
	/**
	 * Window resize handler.
	 * \param coord new window size
	 */
	void on_resize(const COORD& coord);

	void set_keys_state(int keystate);
	bool handle_key_down(int key);

	/**
	 * Save handler
	 * \return true if file saved or user say 'no' on save
	 */
	bool save();

	/**
	 * Save as handler
	 */
	void save_as();

	/**
	 * View/cursor move message handler
	 * \param rec event
	 * \return true if event has been handled
	 */
	bool move_handle_key(int key);
	bool move_handle_mouse(int msg, int ctrl, MOUSE_EVENT_RECORD *rec);

	/**
	 * Keyboard shortcut message handler
	 * \param rec keyboard event
	 * \return true if event has been handled
	 */
	bool sckey_handle(const int key);

	/**
	 * Editor keyboard message handler
	 * \param rec keyboard event
	 * \return true if event has been handled
	 */
	bool edkey_handle(const int key);

	/**
	 * Switch RW/RO mode
	 * \return false if error
	 */
	bool switch_mode();

	/**
	 * Move cursor to the right
	 * \param ingnore_settings flag to ignore settings::move_inside_byte flag
	 */
	void move_right(const bool ingnore_settings);

	/**
	 * Move Far cursor to correct position
	 */
	void move_far_cursor();

	/**
	 * Update edited data
	 * \param offset data offset
	 * \param new_val new value
	 */
	void update_data(const UINT64 offset, const BYTE new_val);

	/**
	 * Undo operation handler
	 */
	void undo();

	/**
	 * Redo operation handler
	 */
	void redo();

	/**
	 * Paste from clipboard
	 */
	void paste();

	/**
	 * Find sequence
	 * \param forward search direction (true=forward, false=backward)
	 * \param force_dlg true to force show find initial dialog
	 */
	void find(const bool forward, const bool force_dlg);

	/**
	 * Get current byte value (original or updated)
	 * \param offset value offset
	 * \return byte value
	 */
	BYTE get_current_value(const UINT64 offset) const;

	/**
	 * Update internal view buffer
	 * \param offset needed offset
	 * \return false if error
	 */
	bool update_buffer(const UINT64 offset);

	/**
	 * Update screen data buffer
	 */
	void update_screen();

	//Far dialog's callback
	static LONG_PTR WINAPI dlg_proc(HANDLE dlg, int msg, int param1, LONG_PTR param2);

	//CopyProgressRoutine Callback Function (see MSDN for more info)
	static DWORD CALLBACK copy_progress_routine(LARGE_INTEGER, LARGE_INTEGER total_transferred, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID lp_data);

private:
	HANDLE			_dialog;		///< Dialog window handle
	statusbar_ctl	_statusbar;		///< Status bar screen control
	hex_ctl			_hexeditor;		///< Hex editor screen control
	keybar_ctl		_keybar;		///< Key bar screen control

	file			_file;			///< Edited file

	UINT64			_cursor_offset;	///< Cursor offset (absolute value)
	bool			_cursor_fbp;	///< Cursor position inside byte (true = first part, false = second part)
	bool			_cursor_iha;	///< Cursor inside hex area (true = hex area, false = text area)

	UINT64			_view_offset;	///< Current view offset

	vector<BYTE>		_ori_data;	///< Original file data array
	map<UINT64, BYTE>	_upd_data;	///< Updated file data map (offset, new value)

	//Undo description
	struct undo_t {
		undo_t(const UINT64	off, const BYTE ov, const BYTE nv) : offset(off), old_val(ov), new_val(nv) {}
		UINT64	offset;				///< Offset
		BYTE	old_val;			///< Old value
		BYTE	new_val;			///< New value
	};
	vector<undo_t>	_undo;			///< Available undo operations array
	size_t			_undo_pos;		///< Undo position

	vector<BYTE>	_search_seq;	///< Search sequence
	bool			_search_fwd;	///< Search direction flag (true = forward, false = backward)
};

void CreateEditor(const wchar_t *file_name, const UINT64 offset);
