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

#include "editor.h"
#include "progress.h"
#include "string_rc.h"
#include "goto_dlg.h"
#include "find_dlg.h"
#include "settings.h"
#include "history.h"
#include "version.h"
#include <farkeys.h>

#define MIN_WIDTH		80		//Minimum width width size of main edit window
#define MIN_HEIGHT	3		//Minimum height width size of main edit window

#define ID_TITLE		0		//Id of the fake title control
#define ID_STATUSBAR	1		//Id of the status bar control
#define ID_EDITOR		2		//Id of the hex editor control
#define ID_KEYBAR		3		//Id of the key bar control

#ifndef MAXUINT64
#define MAXUINT64   ((UINT64)~((UINT64)0))
#endif // MAXUINT64

//#############################################################################

static const auto get_msg(const int txt_id) noexcept {
	return _PSI.GetMsg(_PSI.ModuleNumber, txt_id);
}

static constexpr FARMESSAGEFLAGS MB_mask
= FMSG_MB_OK					// =
| FMSG_MB_OKCANCEL			// -
| FMSG_MB_ABORTRETRYIGNORE // -
| FMSG_MB_YESNO				// +
| FMSG_MB_YESNOCANCEL		// +
| FMSG_MB_RETRYCANCEL      // +
;

static auto msg_box(
	const FARMESSAGEFLAGS f, const wchar_t* const* i, const size_t ntext
) {
	auto flags(f);
	auto items(i);
	intptr_t nbutt(0);
	auto b1{ 0 }, b2{ 0 }, b3{ 0 };
	const wchar_t* my_items[20]; // big enough to append buttons

	switch (f & MB_mask) {
	case FMSG_MB_YESNO:       b1 = ps__yes;   b2 = ps__no;                  break;
	case FMSG_MB_YESNOCANCEL: b1 = ps__yes;   b2 = ps__no; b3 = ps__cancel; break;
	case FMSG_MB_RETRYCANCEL: b1 = ps__retry; b2 = ps__cancel;              break;
	}
	if (b1) {
		for (size_t j = 0; j < ntext; ++j) my_items[j] = i[j];
		items = &my_items[0];
		flags &= ~MB_mask;
		my_items[ntext + nbutt++] = get_msg(b1);
		if (b2) my_items[ntext + nbutt++] = get_msg(b2);
		if (b3) my_items[ntext + nbutt++] = get_msg(b3);
	}

	return _PSI.Message(_PSI.ModuleNumber, flags, nullptr, items, ntext+nbutt, nbutt);
}

template <const size_t N>
static inline auto msg_box(const FARMESSAGEFLAGS f, const wchar_t* (&msgs)[N]) {
	return msg_box(f, msgs, N);
}

//#############################################################################

editor::editor()
:	_cursor_offset(0), _cursor_fbp(true), _cursor_iha(true),
	_view_offset(0),
	_undo_pos(string::npos),
	_search_fwd(true)
{
}

//###

bool editor::edit(const wchar_t* file_name, const UINT64 file_offset /*= 0*/)
{
	assert(file_name && *file_name);

	//Open edited file
	const DWORD open_status = _file.open(file_name);
	if (open_status != ERROR_SUCCESS) {
		const wchar_t* err_msg[] = {
			get_msg(ps_title), get_msg(ps_err_open_file), file_name
		};
		msg_box(FMSG_WARNING | FMSG_ERRORTYPE | FMSG_MB_OK, err_msg);
		return false;
	}

	if (_file.size() == 0)
		return false;

	//Initialize screen controls
	_statusbar.initialize();
	_hexeditor.initialize();
	_keybar.initialize();

	//Prepare undo buffer
	_undo.reserve(1024);

	//Create edit window
	SMALL_RECT far_wnd_size = { 0, 0, MIN_WIDTH, MIN_HEIGHT };
	_PSI.AdvControl(_PSI.ModuleNumber, ACTL_GETFARRECT, &far_wnd_size, 0);
	const size_t wnd_width = max(far_wnd_size.Right - far_wnd_size.Left + 1, MIN_WIDTH);
	const size_t wnd_heigth = max(far_wnd_size.Bottom - far_wnd_size.Top + 1, MIN_HEIGHT);

	//Allocate screen buffers
	_statusbar.resize(wnd_width);
	_hexeditor.resize(wnd_width, wnd_heigth - 2 /* minus status and key bars */);
	_keybar.resize(wnd_width);

	_statusbar.write_filename(file_name);
	_statusbar.write_mode_flag(_file.writable());
	_statusbar.write_codepage(_hexeditor.get_codepage());
	_statusbar.write_offset(file_offset);
	_statusbar.write_position(0);
	_keybar.update(_file.writable(), true, 0);

	//Create controls
	wstring title = _FSF.PointToName(file_name);
	title += L": ";
	title += TEXT("Hex Editor");

	FarDialogItem dlg_items[4];
	ZeroMemory(dlg_items, sizeof(dlg_items));
	dlg_items[0].Type = DI_TEXT;
	dlg_items[0].PtrData = title.c_str();
	dlg_items[1].Type = DI_USERCONTROL;
	dlg_items[1].X2 = wnd_width - 1;
	dlg_items[1].Param.VBuf = _statusbar.buffer();
	dlg_items[2].Type = DI_USERCONTROL;
	dlg_items[2].Y1 = 1;
	dlg_items[2].X2 = wnd_width - 1;
	dlg_items[2].Y2 = wnd_heigth - 2;
	dlg_items[2].Param.VBuf = _hexeditor.buffer();
	dlg_items[2].Flags = DIF_FOCUS;
	dlg_items[3].Type = DI_USERCONTROL;
	dlg_items[3].Y1 = wnd_heigth - 1;
	dlg_items[3].X2 = wnd_width - 1;
	dlg_items[3].Y2 = wnd_heigth - 1;
	dlg_items[3].Param.VBuf = _keybar.buffer();

	_view_offset = file_offset - (file_offset % 0x10);
	_cursor_offset = _view_offset;

	//Try to find last position in history
	if (_cursor_offset == 0 && settings::save_file_pos && history().load_last_position(_file.name(), _view_offset, _cursor_offset)) {
		if (_cursor_offset < _view_offset || _cursor_offset >= _view_offset + _hexeditor.showed_data_size())
			_view_offset = _cursor_offset - _cursor_offset % 0x10;
	}

	if (_cursor_offset >= _file.size())
		_cursor_offset = _file.size() - 1;
	if (!update_buffer(_view_offset))
		return false;

	HANDLE hDlg = _PSI.DialogInit(
		_PSI.ModuleNumber,
		0,
		0,
		wnd_width,
		wnd_heigth,
		nullptr,
		dlg_items,
		sizeof(dlg_items) / sizeof(dlg_items[0]),
		0,
		FDLG_NODRAWSHADOW | FDLG_NODRAWPANEL | FDLG_NONMODAL,
		(FARWINDOWPROC)&editor::dlg_proc,
		(LONG_PTR)this);

	return hDlg != INVALID_HANDLE_VALUE;
}

//###

LONG_PTR WINAPI editor::dlg_proc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	editor* instance = reinterpret_cast<editor*>(_PSI.SendDlgMessage(dlg, DM_GETDLGDATA, 0, (LONG_PTR)nullptr));
	if( !instance )
		return _PSI.DefDlgProc(dlg, msg, param1, param2);
	instance->_dialog = dlg;

	if (msg == DN_INITDIALOG) {
		DWORD cursor_size = MAKELONG(1, 100);
		if (settings::std_cursor_size) {
			CONSOLE_CURSOR_INFO cci;
			if (GetConsoleCursorInfo(0, &cci))
				cursor_size = MAKELONG(1, cci.dwSize);
		}
		_PSI.SendDlgMessage(dlg, DM_SETCURSORSIZE, ID_EDITOR, (LONG_PTR)(cursor_size));
		_PSI.SendDlgMessage(dlg, DM_SETFOCUS, ID_EDITOR, 0);
		instance->move_far_cursor();
		instance->update_screen();
		return 1;
	}
	else if (msg == DN_KILLFOCUS) {
		return ID_EDITOR;
	}
	else if (msg == DN_RESIZECONSOLE) {
		instance->on_resize(*reinterpret_cast<COORD*>(param2));
		return 1;
	}
	else if (msg == DN_KEY){
		if( param1 == -1) 
			instance->set_keys_state((int)param2);
		else if( param1 == ID_EDITOR)
			instance->handle_key_down((int)param2);
		return 1;
	} else if (msg == DN_MOUSECLICK || msg == DN_MOUSEEVENT){
		instance->move_handle_mouse(msg, param1, (MOUSE_EVENT_RECORD *)param2);
		return 1;
	} else if( msg == DN_CLOSE) {
		//Save last position to history
		if (settings::save_file_pos)
			history().save_last_position(instance->_file.name(), instance->_view_offset, instance->_cursor_offset);
        delete instance;
        _PSI.SendDlgMessage(dlg, DM_SETDLGDATA, 0, 0);
	}
	//return instance->on_ctl_input(param1, *reinterpret_cast<const INPUT_RECORD*>(param2)) ? 1 : 0;

	return _PSI.DefDlgProc(dlg, msg, param1, (LONG_PTR)param2);
}

//###

void editor::on_resize(const COORD& coord)
{
	_PSI.SendDlgMessage(_dialog, DM_ENABLEREDRAW, FALSE, (LONG_PTR)nullptr);

	//Resize screen controls
	_statusbar.resize(coord.X);
	_hexeditor.resize(coord.X, coord.Y - 2 /* minus status and key bars */);
	_keybar.resize(coord.X);

	//Set controls coordinates
	FarDialogItem far_item_sb;
	_PSI.SendDlgMessage(_dialog, DM_GETDLGITEMSHORT, ID_STATUSBAR, (LONG_PTR)&far_item_sb);
	far_item_sb.Param.VBuf = _statusbar.buffer();
	far_item_sb.X2 = _statusbar.width() - 1;
	_PSI.SendDlgMessage(_dialog, DM_SETDLGITEMSHORT, ID_STATUSBAR, (LONG_PTR)&far_item_sb);

	FarDialogItem far_item_he;
	_PSI.SendDlgMessage(_dialog, DM_GETDLGITEMSHORT, ID_EDITOR, (LONG_PTR)&far_item_he);
	far_item_he.Param.VBuf = _hexeditor.buffer();
	far_item_he.Y2 = _hexeditor.height();
	far_item_he.X2 = _hexeditor.width() - 1;
	_PSI.SendDlgMessage(_dialog, DM_SETDLGITEMSHORT, ID_EDITOR, (LONG_PTR)&far_item_he);

	FarDialogItem far_item_kb;
	_PSI.SendDlgMessage(_dialog, DM_GETDLGITEMSHORT, ID_KEYBAR, (LONG_PTR)&far_item_kb);
	far_item_kb.Param.VBuf = _keybar.buffer();
	far_item_kb.Y1 = far_item_kb.Y2 = coord.Y - 1;
	far_item_kb.X2 = _keybar.width() - 1;
	_PSI.SendDlgMessage(_dialog, DM_SETDLGITEMSHORT, ID_KEYBAR, (LONG_PTR)&far_item_kb);

	_PSI.SendDlgMessage(_dialog, DM_RESIZEDIALOG, 0, (LONG_PTR)&coord);

	_PSI.SendDlgMessage(_dialog, DM_ENABLEREDRAW, TRUE, (LONG_PTR)nullptr);

	if (_cursor_offset > _view_offset + _hexeditor.showed_data_size())
		_view_offset = _cursor_offset - (_cursor_offset % 0x10) + 0x10 - _hexeditor.showed_data_size();

	update_buffer(_view_offset);	//Re-read
	move_far_cursor();
	update_screen();
}

//###

void editor::set_keys_state(int keystate)
{
	if( _keybar.update(_file.writable(), false, keystate) )
		_PSI.SendDlgMessage(_dialog, DM_REDRAW, 0, (LONG_PTR)nullptr);
}

bool editor::handle_key_down(int key){
	if (sckey_handle(key))
		return true;
	if(move_handle_key(key))
		return true;
	if( edkey_handle(key) )
		return true;

	return false;
}

//###

bool editor::save()
{
	if (_upd_data.empty())
		return true;

	if (_file.read_only()) {
		const wchar_t* msg[] = {
			get_msg(ps_sav_title), get_msg(ps_sav_file), _file.name(), get_msg(ps_sav_readonly), get_msg(ps_sav_overwrq)
		};
		if (msg_box(FMSG_WARNING | FMSG_MB_YESNO, msg) != 0)
			return false;
		DWORD setattr_status;
		while ((setattr_status = file::clear_read_only(_file.name())) != ERROR_SUCCESS) {
			const wchar_t* err_msg[] = {
				get_msg(ps_sav_title), get_msg(ps_err_save_file), _file.name()
			};
			if (msg_box(FMSG_WARNING | FMSG_ERRORTYPE | FMSG_MB_RETRYCANCEL, err_msg) != 0)
				return false;
		}
	}

	DWORD save_status;
	while ((save_status = _file.save(_upd_data)) != ERROR_SUCCESS) {
		const wchar_t* err_msg[] = {
			get_msg(ps_sav_title), get_msg(ps_err_save_file), _file.name()
		};
		if (msg_box(FMSG_WARNING | FMSG_ERRORTYPE | FMSG_MB_RETRYCANCEL, err_msg) != 0)
			return false;
	}

	_upd_data.clear();
	update_buffer(_view_offset);	//Re-read
	_statusbar.write_update_flag(false);
	update_screen();

	return true;
}

//###

void editor::save_as()
{
	wchar_t new_file_name[1024];
	if (!_PSI.InputBox(
		get_msg(ps_sav_title), get_msg(ps_sav_saveas),
		nullptr, _file.name(), new_file_name, sizeof(new_file_name), nullptr, FIB_EXPANDENV | FIB_BUTTONS | FIB_EDITPATH))
		return;

	if (file::file_exist(new_file_name)) {
		const wchar_t* msg[] = {
			get_msg(ps_sav_title), get_msg(ps_sav_file), _file.name(), get_msg(ps_sav_alrexist), get_msg(ps_sav_overwrq)
		};
		if (msg_box(FMSG_WARNING | FMSG_MB_YESNO, msg) != 0)
			return;
	}

	progress progress_wnd(get_msg(ps_sav_title), 0, _file.size());

	DWORD save_status;
	while ((save_status = _file.save_as(new_file_name, _upd_data, &editor::copy_progress_routine, &progress_wnd)) != ERROR_SUCCESS) {
		if (save_status != NO_ERROR)
			return;
		const wchar_t* err_msg[] = {
			get_msg(ps_sav_title), get_msg(ps_err_save_file), _file.name()
		};
		if (msg_box(FMSG_WARNING | FMSG_ERRORTYPE | FMSG_MB_RETRYCANCEL, err_msg) != 0)
			return;
	}

	_upd_data.clear();
	update_buffer(_view_offset);	//Re-read

	_statusbar.write_mode_flag(_file.writable());
	_statusbar.write_update_flag(false);
	_statusbar.write_filename(_file.name());
	_keybar.update(_file.writable(), false, 0);

	update_screen();
}

//###

bool editor::move_handle_key(int key)
{
	bool event_handled = true;
	keybar_ctl::state kb_state = _keybar.get_state();
	bool ctrl_pressed = (kb_state == keybar_ctl::st_ctrl);
	int keyx = key & ~(KEY_CTRL|KEY_ALT|KEY_SHIFT);

	switch (keyx) {
		case KEY_TAB:
			_cursor_iha = !_cursor_iha;
			//_cursor_fbp = true;
			break;

		case KEY_NUMPAD7:
		case KEY_HOME:
			_cursor_fbp = true;
			if (ctrl_pressed) {
				_cursor_offset = 0;
				if (_view_offset != 0)
					update_buffer(0);
			}
			else
				_cursor_offset = _cursor_offset - (_cursor_offset % 0x10);
			break;

		case KEY_NUMPAD1:
		case KEY_END:
			_cursor_fbp = true;
			if (ctrl_pressed) {
				_cursor_offset = _file.size() - 1;
				if (_cursor_offset > _hexeditor.showed_data_size()) {
					const UINT64 needed_offset = _cursor_offset - (_cursor_offset % 0x10) + 0x10 - _hexeditor.showed_data_size();
					if (_view_offset != needed_offset)
						update_buffer(needed_offset);
				}
			}
			else {
				_cursor_offset = _cursor_offset + 0x10 - (_cursor_offset % 0x10) - 1;
				if (_cursor_offset >= _file.size())
					_cursor_offset = _file.size() - 1;
				if (!_cursor_iha && _hexeditor.get_codepage() == CP_UTF16LE)
					--_cursor_offset;
			}
			break;

		case KEY_NUMPAD4:
		case KEY_LEFT:
			if (ctrl_pressed) {
				_cursor_fbp = true;
				if (_cursor_offset > 0){
					--_cursor_offset;
					_cursor_offset = _cursor_offset - (_cursor_offset % 0x4);
				}
			}
			else {
				if (_cursor_offset && !_cursor_iha)
					_cursor_fbp = true;
				if (_cursor_offset || (_cursor_iha && !_cursor_fbp)) {
					bool cursor_moved = false;
					if (_cursor_iha) {
						if (settings::move_inside_byte) {
							cursor_moved = _cursor_fbp;
							_cursor_fbp = !_cursor_fbp;
						}
						else {
							cursor_moved = true;
							_cursor_fbp = true;
						}
					}
					if (!_cursor_iha || cursor_moved)
						_cursor_offset -= (_hexeditor.get_codepage() == CP_UTF16LE && !_cursor_iha ? 2 : 1);
				}
			}
			if (_cursor_offset < _view_offset)
				update_buffer(_view_offset - 0x10);
			break;

		case KEY_NUMPAD6:
		case KEY_RIGHT:
			if (!ctrl_pressed)
				move_right(false);
			else {
				_cursor_fbp = true;
				_cursor_offset = _cursor_offset + 0x4 - (_cursor_offset % 0x4);
				if (_cursor_offset >= _file.size())
					_cursor_offset = _file.size() - 1;
			}
			if (_cursor_offset >= _view_offset + _hexeditor.showed_data_size())
				update_buffer(_view_offset + 0x10);
			break;

		case KEY_NUMPAD8:
		case KEY_UP:
			if (ctrl_pressed) {
				if (_cursor_offset >= 0x10 && _view_offset >= 0x10) {
					if (!_cursor_iha)
						_cursor_fbp = true;
					update_buffer(_view_offset - 0x10);
					_cursor_offset -= 0x10;
				}
			}
			else {
				if (_cursor_offset >= 0x10) {
					if (!_cursor_iha)
						_cursor_fbp = true;
					_cursor_offset -= 0x10;
					if (_view_offset > _cursor_offset)
						update_buffer(_view_offset - 0x10);
				}
			}
			break;

		case KEY_NUMPAD2:
		case KEY_DOWN:
			if (ctrl_pressed) {
				if (_view_offset + _hexeditor.showed_data_size() < _file.size()) {
					if (!_cursor_iha)
						_cursor_fbp = true;
					_cursor_offset += 0x10;
					update_buffer(_view_offset + 0x10);
				}
			}
			else {
				if (_cursor_offset  + 0x10 < _file.size()) {
					if (!_cursor_iha)
						_cursor_fbp = true;
					_cursor_offset += 0x10;
					if (_cursor_offset >= _view_offset + _hexeditor.showed_data_size())
						update_buffer(_view_offset + 0x10);
				}
			}
			break;

		case KEY_NUMPAD9:
		case KEY_PGUP:
			if (_view_offset < _hexeditor.showed_data_size()) {
				if (_view_offset != 0) {
					if (!_cursor_iha)
						_cursor_fbp = true;
					_cursor_offset -= _view_offset;
					update_buffer(0);
				}
			}
			else {
				if (!_cursor_iha)
					_cursor_fbp = true;
				update_buffer(_view_offset - _hexeditor.showed_data_size());
				_cursor_offset -= _hexeditor.showed_data_size();
			}
			break;

		case KEY_NUMPAD3:
		case KEY_PGDN:
			if (_view_offset + _hexeditor.showed_data_size() * 2 < _file.size()) {
				if (!_cursor_iha)
					_cursor_fbp = true;
				update_buffer(_view_offset + _hexeditor.showed_data_size());
				_cursor_offset += _hexeditor.showed_data_size();
			}
			else {
				const UINT64 needed_offset = _file.size() <= _hexeditor.showed_data_size() ? 0 : _file.size() - _hexeditor.showed_data_size() + ((_file.size() % 0x10) ? 0x10 - (_file.size() % 0x10) : 0);
				if (_view_offset != needed_offset) {
					if (!_cursor_iha)
						_cursor_fbp = true;
					_cursor_offset += needed_offset - _view_offset;
					if (_cursor_offset >= _file.size())
						_cursor_offset = _file.size() - 1;
					update_buffer(needed_offset);
				}
			}
			break;
		default:
			event_handled = false;
	}

	if (event_handled) {
		move_far_cursor();
		update_screen();
	}

	return event_handled;
}

bool editor::move_handle_mouse(int msg, int ctrl, MOUSE_EVENT_RECORD *rec)
{
	bool event_handled = false;
	if( ctrl == ID_KEYBAR && msg == DN_MOUSECLICK){
		//Mouse click on fake button?
		const WORD btn_id = _keybar.get_button(rec->dwMousePosition.X);
		if (btn_id > 0)
			event_handled = sckey_handle(VK_F1 + btn_id - 1);
	}
	else if( ctrl == ID_EDITOR ){
		if (msg == DN_MOUSECLICK) {
			UINT64 offset = 0;
			bool first_part = true;
			bool hex_area = true;
			if (_hexeditor.offset_from_cursor(_view_offset, rec->dwMousePosition, offset, first_part, hex_area)) {
				if (offset >= _file.size()) {
					offset = _file.size() - 1;
					if (hex_area)
						first_part = false;
				}
				_cursor_offset = offset;
				_cursor_fbp = first_part;
				_cursor_iha = hex_area;
				event_handled = true;
			}
		}
		else if (msg == DN_MOUSEEVENT) {
			if ((static_cast<int>(rec->dwButtonState) >> 16) > 0) {
				//Up wheel
				if (_cursor_offset >= 0x10 && _view_offset >= 0x10) {
					if (!_cursor_iha)
						_cursor_fbp = true;
					_cursor_offset -= 0x10;
					update_buffer(_view_offset - 0x10);
					event_handled = true;
				}
			}
			else {
				//Down wheel
				if (_view_offset + _hexeditor.showed_data_size() < _file.size()) {
					if (!_cursor_iha)
						_cursor_fbp = true;
					_cursor_offset += 0x10;
					update_buffer(_view_offset + 0x10);
					event_handled = true;
				}
			}
		}
	}

	if (event_handled) {
		move_far_cursor();
		update_screen();
	}

	return event_handled;

}

//###

bool editor::sckey_handle(const int key)
{
	bool even_handled = false;
	keybar_ctl::state kb_state = _keybar.get_state();
	bool ctrl_pressed = (kb_state == keybar_ctl::st_ctrl);
	bool alt_pressed = (kb_state == keybar_ctl::st_alt);
	bool shift_pressed = (kb_state == keybar_ctl::st_shift);

	int keyx = key & ~(KEY_CTRL|KEY_ALT|KEY_SHIFT);
	switch (keyx) {
		case KEY_ENTER:
			even_handled = /*!ctrl_pressed &&*/ !alt_pressed && !shift_pressed;
			break;

		case KEY_ESC:
		case KEY_F10:
			if (keyx == KEY_ESC || (!ctrl_pressed && !alt_pressed && !shift_pressed)) {
				even_handled = true;
				bool can_exit = _upd_data.empty();
				if (!can_exit) {
					const wchar_t* msg[] = {
						get_msg(ps_sav_title), get_msg(ps_sav_modifq)
					};
					const intptr_t ret = msg_box(FMSG_WARNING | FMSG_MB_YESNOCANCEL, msg);
					if (ret == 0)
						can_exit = save();
					else if (ret == 1)
						can_exit = true;
				}
				if (can_exit)
					_PSI.SendDlgMessage(_dialog, DM_CLOSE, -1, (LONG_PTR)nullptr);
			}
			break;
		case KEY_F1:
			if (!ctrl_pressed && !alt_pressed && !shift_pressed) {
				even_handled = true;
				_PSI.ShowHelp(_PSI.ModuleName, L"Contents", 0);
			}
			break;

		case KEY_F2:
			if (!ctrl_pressed && !alt_pressed) {
				even_handled = true;
				if (shift_pressed)
					save_as();
				else
					save();
			}
			break;

		case KEY_F4:
			if (!ctrl_pressed && !alt_pressed && !shift_pressed) {
				even_handled = true;
				switch_mode();
			}
			break;

		case KEY_F5:
		case 'G':
			if (keyx == KEY_F5 || (ctrl_pressed && !alt_pressed && !shift_pressed)) {
				even_handled = true;
				goto_dlg dlg;
				UINT64 offset = _cursor_offset;
				if (dlg.show(_file.size(), offset)) {
					_cursor_offset = offset;
					if (_cursor_offset < _view_offset || _cursor_offset >= _view_offset + _hexeditor.showed_data_size())
						update_buffer(_cursor_offset - _cursor_offset % 0x10);
					_cursor_fbp = true;
					move_far_cursor();
					update_screen();
				}
			}
			break;

		case 'F':
			if (ctrl_pressed && !alt_pressed && !shift_pressed) {
				even_handled = true;
				find(true, true);
			}
			break;

		case KEY_F7:
			if (!ctrl_pressed) {
				if (!alt_pressed && !shift_pressed) {
					even_handled = true;
					find(true, true);
				}
				else if (!alt_pressed && shift_pressed) {
					even_handled = true;
					find(true, false);
				}
				else if (alt_pressed && !shift_pressed) {
					even_handled = true;
					find(false, false);
				}
			}
			break;

		case KEY_F8:
			if (!ctrl_pressed && !alt_pressed && !shift_pressed) {
				even_handled = true;
				_statusbar.write_codepage(_hexeditor.switch_codepage());
				if (!_cursor_iha)
					_cursor_offset -= _cursor_offset % 2;
				move_far_cursor();
				update_screen();
			}
			break;

		case KEY_F9:
			if ((!ctrl_pressed && !alt_pressed && !shift_pressed) || (!ctrl_pressed && alt_pressed && shift_pressed)) {
				even_handled = true;
				settings::configure();
				update_screen();
			}
			break;

		case 'Z':
			if (ctrl_pressed && !shift_pressed && !alt_pressed) {
				even_handled = true;
				undo();
			}
			else if (ctrl_pressed && shift_pressed && !alt_pressed) {
				even_handled = true;
				redo();
			}
			break;

		case 'Y':
			if (ctrl_pressed && !shift_pressed && !alt_pressed) {
				even_handled = true;
				redo();
			}
			break;

		case KEY_INS:
			if (!ctrl_pressed && shift_pressed && !alt_pressed) {
				even_handled = true;
				paste();
			}
			break;

		case 'V':
			if (ctrl_pressed && !shift_pressed && !alt_pressed) {
				even_handled = true;
				paste();
			}
			break;
	}

	return even_handled;
}

//###

bool editor::edkey_handle(int key)
{
	keybar_ctl::state kb_state = _keybar.get_state();
	bool ctrl_pressed = (kb_state == keybar_ctl::st_ctrl);
	bool alt_pressed = (kb_state == keybar_ctl::st_alt);
	if (ctrl_pressed || alt_pressed)
		return false;
	
	if( key & KEY_CTRLMASK )
		return false;
	key &= KEY_MASKF;
	if( !key )
		return false;

	if (_cursor_iha) {
		unsigned char key_code = 0;
		if (key >= '0' && key <= '9')
			key_code = static_cast<BYTE>(key) - '0';
		else if (key >= 'A' && key <= 'F')
			key_code = 10 + static_cast<BYTE>(key) - 'A';
		else if (key >= 'a' && key <= 'f')
			key_code = 10 + static_cast<BYTE>(key) - 'a';
		else
			return false;	//Unhandled key

		if (!_file.writable()) {
			const wchar_t* msg[] = {
				get_msg(ps_title), get_msg(ps_swmod_warn), get_msg(ps_swmod_quest)
			};
			if (msg_box(FMSG_MB_YESNO, msg))
				return false;
			if (!switch_mode())
				return false;
		}

		const BYTE old_val = get_current_value(_cursor_offset);
		const BYTE new_val = _cursor_fbp ? ((old_val & 0x0f) | (key_code << 4)) : ((old_val & 0xf0) | key_code);
		update_data(_cursor_offset, new_val);
	}
	else {
		if (key < ' ')
			return false;

		if (!_file.writable()) {
			const wchar_t* msg[] = {
				get_msg(ps_title), get_msg(ps_swmod_warn), get_msg(ps_swmod_quest)
			};
			if (msg_box(FMSG_MB_YESNO, msg) != 0)
				return false;
			if (!switch_mode())
				return false;
		}
		const wchar_t key_value = static_cast<wchar_t>(key);

		if (_hexeditor.get_codepage() == CP_UTF16LE) {
			update_data(_cursor_offset, LOBYTE(key_value));
			if (_cursor_offset + 1 < _file.size())
				update_data(_cursor_offset + 1, HIBYTE(key_value));
		}
		else {
			BYTE new_val;
			WideCharToMultiByte(_hexeditor.get_codepage(), 0, &key_value, 1, reinterpret_cast<LPSTR>(&new_val), 1, nullptr, nullptr);
			update_data(_cursor_offset, new_val);
		}
	}

	move_right(true);
	move_far_cursor();
	update_screen();

	return true;
}

//###

bool editor::switch_mode()
{
	if (!_upd_data.empty()) {
		assert(_file.writable());
		const wchar_t* msg[] = {
			get_msg(ps_title), get_msg(ps_sav_modifq)
		};
		const intptr_t ret = msg_box(FMSG_MB_YESNOCANCEL | FMSG_WARNING, msg);

		if (ret < 0 || ret == 2)
			return false;
		else if (ret == 1) {
			_statusbar.write_update_flag(false);
			_upd_data.clear();
		}
		else if (ret == 0 && !save())
			return false;
	}

	DWORD swm_status;
	while ((swm_status = _file.switch_mode()) != ERROR_SUCCESS) {
		SetLastError(swm_status);
		const wchar_t* err_msg[] = {
			get_msg(ps_title), get_msg(ps_err_open_file), _file.name()
		};
		if (msg_box(FMSG_WARNING | FMSG_ERRORTYPE | FMSG_MB_RETRYCANCEL, err_msg) != 0)
			return false;
	}

	_statusbar.write_mode_flag(_file.writable());
	_keybar.update(_file.writable(), false, 0);
	update_screen();

	return true;
}

//###

void editor::move_right(const bool ingnore_settings)
{
	if (_cursor_iha && _cursor_fbp && (ingnore_settings || settings::move_inside_byte))
		_cursor_fbp = false;
	else if (_cursor_offset + (_hexeditor.get_codepage() == CP_UTF16LE && !_cursor_iha ? 2 : 1) < _file.size()) {
		_cursor_fbp = true;
		_cursor_offset += (_hexeditor.get_codepage() == CP_UTF16LE && !_cursor_iha ? 2 : 1);
	}
	if (_cursor_offset >= _view_offset + _hexeditor.showed_data_size())
		update_buffer(_view_offset + 0x10);
}

//###

void editor::move_far_cursor()
{
	COORD coord = _hexeditor.cursor_from_offset(_view_offset, _cursor_offset, _cursor_iha);
	coord.X += (!_cursor_fbp && _cursor_iha ? 1 : 0);
	_PSI.SendDlgMessage(_dialog, DM_SETCURSORPOS, ID_EDITOR, (LONG_PTR)&coord);
	_statusbar.write_offset(_cursor_offset);
}

//###

void editor::update_data(const UINT64 offset, const BYTE new_val)
{
	const BYTE old_val = get_current_value(offset);
	if (old_val == new_val)
		return;

	_upd_data[offset] = new_val;
	_statusbar.write_update_flag(true);

	//Prepare undo operation
	if (_undo_pos == string::npos)
		_undo.clear();
	else if (_undo_pos != _undo.size() - 1)
		_undo.erase(_undo.begin() + _undo_pos + 1, _undo.end());
	if (!_undo.empty() && _undo.back().offset == offset)
		_undo.back().new_val = new_val;	//Replace value
	else {
		_undo.push_back(undo_t(offset, old_val, new_val));
		_undo_pos = _undo.size() - 1;
	}
}

//###

void editor::undo()
{
	assert(_undo_pos == string::npos || _undo_pos < _undo.size());

	if (_undo_pos == string::npos)
		return;	//Start position

	const undo_t& undo_action = _undo[_undo_pos];

	map<UINT64, BYTE>::iterator itup = _upd_data.find(undo_action.offset);
	if (itup == _upd_data.end()) {
		_upd_data[undo_action.offset] = undo_action.old_val;
	}
	else {
		vector<BYTE> buff;
		if (_file.read(undo_action.offset, buff, 1) == ERROR_SUCCESS && buff.front() == undo_action.old_val)
			_upd_data.erase(itup);
		else
			itup->second = undo_action.old_val;
	}

	--_undo_pos;
	_cursor_offset = undo_action.offset;
	_cursor_fbp = true;

	if (_cursor_offset < _view_offset || _cursor_offset >= _view_offset + _hexeditor.showed_data_size())
		update_buffer(_cursor_offset - _cursor_offset % 0x10);

	if (_upd_data.empty())
		_statusbar.write_update_flag(false);

	move_far_cursor();
	update_screen();
}

//###

void editor::redo()
{
	if (_undo.empty())
		return;	//No action
	if (_undo_pos != string::npos && _undo_pos >= _undo.size() - 1)
		return;	//End position

	++_undo_pos;

	const undo_t& redo_action = _undo[_undo_pos];

	_upd_data[redo_action.offset] = redo_action.new_val;
	_statusbar.write_update_flag(true);

	_cursor_offset = redo_action.offset;
	_cursor_fbp = true;

	if (_cursor_offset < _view_offset || _cursor_offset >= _view_offset + _hexeditor.showed_data_size())
		update_buffer(_cursor_offset - _cursor_offset % 0x10);

	move_far_cursor();
	update_screen();
}

//###

void editor::paste()
{
	/*
	// TODO implement paste from keyboard
	const size_t paste_buff_size = _FSF.PasteFromClipboard(FCT_STREAM, nullptr, 0);
	if (!paste_buff_size)
		return;	//No data
	wstring paste_data(paste_buff_size, 0);
	_FSF.PasteFromClipboard(FCT_STREAM, &paste_data.front(), paste_buff_size);
	if (*paste_data.rbegin() == 0)
		paste_data.erase(paste_data.length() - 1);

	if (!_file.writable()) {
		const wchar_t* msg[] = {
			get_msg(ps_title), get_msg(ps_swmod_warn), get_msg(ps_swmod_quest)
		};
		if (msg_box(FMSG_MB_YESNO, msg) != 0)
			return;
		if (!switch_mode())
			return;
	}

	vector<BYTE> paste_array;
	if (_hexeditor.get_codepage() == CP_UTF16LE) {
		//Paste as Unicode
		for (wstring::const_iterator it = paste_data.begin(); it != paste_data.end(); ++it) {
			paste_array.push_back(LOBYTE(*it));
			paste_array.push_back(HIBYTE(*it));
		}
	}
	else {
		//Paste as encoded bytes
		string enc;
		const int req = WideCharToMultiByte(_hexeditor.get_codepage(), 0, paste_data.c_str(), static_cast<int>(paste_data.length()), 0, 0, NULL, NULL);
		if (req) {
			enc.resize(static_cast<size_t>(req));
			WideCharToMultiByte(_hexeditor.get_codepage(), 0, paste_data.c_str(), static_cast<int>(paste_data.length()), &enc.front(), req, NULL, NULL);
			paste_array.assign(enc.begin(), enc.end());
		}
	}

	for (vector<BYTE>::const_iterator it = paste_array.begin(); it != paste_array.end(); ++it) {
		update_data(_cursor_offset, *it);
		if (_cursor_offset + 1 >= _file.size())
			break;	//EOF
		++_cursor_offset;
	}

	//Set correct view offset
	if (_cursor_offset >= _view_offset + _hexeditor.showed_data_size())
		update_buffer(_cursor_offset - _hexeditor.showed_data_size() + 0x10 - (_cursor_offset % 0x10));

	move_far_cursor();
	update_screen();
	*/
}

//###

void editor::find(const bool forward, const bool force_dlg)
{
	bool dir_forward = forward;

	if (force_dlg || _search_seq.empty()) {
		find_dlg dlg;
		if (!dlg.show(_search_seq, dir_forward))
			return;
	}

	assert(!_search_seq.empty());

	UINT64 seq_offset = MAXUINT64;

	bool interrupted_by_user = false;
	const size_t init_buff_size = 4096;
	const size_t seq_size = _search_seq.size();
	const BYTE* seq_ptr = &_search_seq.front();

	if (dir_forward && _cursor_offset + 1 < _file.size()) {
		//Initialize progress bar window
		progress progress_wnd(get_msg(ps_find_title), _cursor_offset, _file.size());

		//Initial read position
		UINT64 read_offset = _cursor_offset + 1;

		//Let's search!
		while (seq_offset == MAXUINT64 && read_offset < _file.size()) {
			//Show progress window and check for Esc press
			progress_wnd.update(read_offset);
			if (progress::aborted()) {
				interrupted_by_user = true;
				break;
			}

			//Read file
			vector<BYTE> search_buff;
			if (_file.read(read_offset, search_buff, init_buff_size) != ERROR_SUCCESS)
				break;
			const size_t search_buff_sz = search_buff.size();
			if (search_buff_sz < seq_size)
				break;

			//Apply current updates
			for (map<UINT64, BYTE>::const_iterator it = _upd_data.begin(); it != _upd_data.end(); ++it) {
				if (it->first >= read_offset && it->first < read_offset + search_buff_sz)
					search_buff[static_cast<size_t>(it->first - read_offset)] = it->second;
			}

			//Search for sequence
			for (size_t i = 0; seq_offset == MAXUINT64 && i < search_buff_sz && search_buff_sz - i >= seq_size; ++i) {
				if (memcmp(seq_ptr, &search_buff[i], seq_size) == 0)
					seq_offset = read_offset + i;
			}

			//Calculate next read position
			if (seq_offset == MAXUINT64)
				read_offset += init_buff_size - seq_size;
		}

		progress_wnd.hide();
		_PSI.SendDlgMessage(_dialog, DM_REDRAW, 0, (LONG_PTR)nullptr);
	}
	else if (!dir_forward && _cursor_offset != 0) {
		//Initialize progress bar window
		progress progress_wnd(get_msg(ps_find_title), _cursor_offset, 0);

		//Initial read position
		UINT64 read_offset_start = _cursor_offset < init_buff_size ? 0 : _cursor_offset - init_buff_size;
		UINT64 read_offset_end = _cursor_offset + seq_size;

		//Let's search!
		while (seq_offset == MAXUINT64) {
			//Show progress window and check for Esc press
			progress_wnd.update(read_offset_start);
			if (progress::aborted()) {
				interrupted_by_user = true;
				break;
			}

			//Read file
			vector<BYTE> search_buff;
			if (_file.read(read_offset_start, search_buff, static_cast<size_t>(read_offset_end - read_offset_start)) != ERROR_SUCCESS)
				break;
			const size_t search_buff_sz = search_buff.size();
			if (search_buff_sz < seq_size)
				break;

			//Apply current updates
			for (map<UINT64, BYTE>::const_iterator it = _upd_data.begin(); it != _upd_data.end(); ++it) {
				if (it->first >= read_offset_start && it->first < read_offset_end)
					search_buff[static_cast<size_t>(it->first - read_offset_start)] = it->second;
			}

			//Search for sequence
			for (INT64 i = static_cast<INT64>(search_buff_sz - seq_size) - 1; seq_offset == MAXUINT64 && i > 0; --i) {
				if (memcmp(seq_ptr, &search_buff[static_cast<size_t>(i)], seq_size) == 0)
					seq_offset = read_offset_start + i;
			}

			//Calculate next read position
			if (seq_offset == MAXUINT64) {
				if (read_offset_start == 0)
					break;	//Start position already reached
				read_offset_start = read_offset_start > init_buff_size ? read_offset_start - init_buff_size : 0;
				read_offset_end = read_offset_start + search_buff_sz;
			}
		}

		progress_wnd.hide();
		_PSI.SendDlgMessage(_dialog, DM_REDRAW, 0, (LONG_PTR)nullptr);
	}

	if (seq_offset == MAXUINT64 && !interrupted_by_user) {
		const wchar_t* msg[] = {
			get_msg(ps_find_title), get_msg(ps_find_not_found)
		};
		msg_box(FMSG_MB_OK, msg);
	}
	else if (seq_offset != MAXUINT64) {
		_cursor_fbp = true;
		_cursor_offset = seq_offset;
		if (_cursor_offset < _view_offset || _cursor_offset >= _view_offset + _hexeditor.showed_data_size())
			update_buffer(_cursor_offset - _cursor_offset % 0x10);
		move_far_cursor();
		update_screen();
	}
}

//###

BYTE editor::get_current_value(const UINT64 offset) const
{
	assert(offset < _file.size());

	BYTE val = 0;

	if (offset < _file.size()) {
		map<UINT64, BYTE>::const_iterator itup = _upd_data.find(_cursor_offset);
		if (itup != _upd_data.end())
			val = itup->second;
		else {
			if (offset >= _view_offset && offset < _view_offset + _hexeditor.showed_data_size())
				val = _ori_data[static_cast<size_t>(offset - _view_offset)];
			else {
				//Read from file
				vector<BYTE> buff;
				if (_file.read(offset, buff, 1) == ERROR_SUCCESS && !buff.empty())
					val = buff.front();
			}
		}
	}

	return val;
}

//###

bool editor::update_buffer(const UINT64 offset)
{
	assert(offset < _file.size());
	assert(!(offset % 0x10));

	const DWORD read_status = _file.read(offset, _ori_data, _hexeditor.showed_data_size());
	if (read_status != ERROR_SUCCESS) {
		const wchar_t* msg[] = {
			get_msg(ps_title), get_msg(ps_err_read_file), _file.name()
		};
		msg_box(FMSG_MB_OK | FMSG_WARNING | FMSG_ERRORTYPE, msg);
		return false;
	}
	else {
		_view_offset = offset;

		//Calculate position
		unsigned char percent = 0;
		if (_view_offset + _hexeditor.showed_data_size() >= _file.size())
			percent = 100;
		else if (_view_offset != 0)
			percent = static_cast<unsigned char>(_view_offset * 100 / _file.size());
		_statusbar.write_position(percent);

		return true;
	}
}

//###

void editor::update_screen()
{
    _PSI.SendDlgMessage(_dialog, DM_ENABLEREDRAW, FALSE, 0);
	_hexeditor.update(_view_offset, _ori_data, _upd_data, _cursor_offset, _cursor_iha);
    _PSI.SendDlgMessage(_dialog, DM_ENABLEREDRAW, TRUE, 0);
}

//###

DWORD CALLBACK editor::copy_progress_routine(LARGE_INTEGER, LARGE_INTEGER total_transferred, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID lp_data)
{
	progress* pw = reinterpret_cast<progress*>(lp_data);
	pw->update(total_transferred.QuadPart);
	bool interrupted = progress::aborted();
	if (interrupted) {
		const wchar_t* msg[] = {
			get_msg(ps_sav_title), get_msg(ps_sav_cancelq)
		};
		interrupted = msg_box(FMSG_MB_YESNO | FMSG_WARNING, msg) == 0;
	}
	return interrupted ? PROGRESS_CANCEL : PROGRESS_CONTINUE;
}

void CreateEditor(const wchar_t *file_name, const UINT64 offset)
{
	editor* ed = new editor();
	if (!ed->edit(file_name, offset))
		delete ed;
}
//#############################################################################
