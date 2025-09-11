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

#include "keybar_ctl.h"
#include "string_rc.h"
#include <farkeys.h>

#define BTN_LABEL_LEN	6	//Button label length


keybar_ctl::keybar_ctl() : _state(st_normal), _rw_mode(false)
{}


void keybar_ctl::initialize()
{
	//Get colors from Far settings
	_PSI.AdvControl(_PSI.ModuleNumber, ACTL_GETCOLOR, (void *)COL_KEYBARNUM, &_clr_num);
	_PSI.AdvControl(_PSI.ModuleNumber, ACTL_GETCOLOR, (void *)COL_KEYBARTEXT, &_clr_txt);
	_PSI.AdvControl(_PSI.ModuleNumber, ACTL_GETCOLOR, (void *)COL_KEYBARBACKGROUND, &_default_color);
}


void keybar_ctl::resize(const size_t width)
{
	resize_buffer(width, 1 /* Always one line only */);

	//Clear background
	for (size_t i = 0; i < _width; ++i) {
		write(0, i, _clr_txt);
		write(0, i, L' ');
	}

	//Draw buttons num
	for (size_t i = 0; i < 12; ++i) {
		const size_t pos = get_btn_pos(i + 1, false);
		if (pos + 2 >= _width)
			break;
		//Set color
		if (i)
			write(0, pos - 1, _default_color);
		wchar_t num[16]={0};
		std::to_wstring(i + 1).copy(num, 16);
		write(0, pos, num, wcslen(num));
		write(0, pos, _clr_num);
		if (i >= 9)
			write(0, pos + 1, _clr_num);
	}

	set_labels();
}


bool keybar_ctl::update(const bool rw_mode, const bool initialize, const int keystate)
{
	state new_state = st_normal;
	if (!initialize) {
		const bool state_shift = keystate & KEY_SHIFT;
		const bool state_ctrl  = keystate & KEY_CTRL;
		const bool state_alt   = keystate & KEY_ALT;
		const bool state_multiple = (state_shift || state_ctrl || state_alt) && !(state_shift ^ state_ctrl ^ state_alt);

		if (state_multiple)
			new_state = st_undef;
		else if (state_shift)
			new_state = st_shift;
		else if (state_ctrl)
			new_state = st_ctrl;
		else if (state_alt)
			new_state = st_alt;
	}

	if (initialize || new_state != _state || _rw_mode != rw_mode) {
		_rw_mode = rw_mode;
		_state = new_state;
		set_labels();
		return true;
	}

	return false;
}


WORD keybar_ctl::get_button(const SHORT x_coord) const
{
	WORD btn_id = 0;
	const size_t btn_width = get_btn_width();

	for (WORD i = 1; btn_id == 0 && i <= 12; ++i) {
		const size_t btn_pos = get_btn_pos(i, false);
		if (x_coord >= static_cast<SHORT>(btn_pos) && x_coord <= static_cast<SHORT>(btn_pos + btn_width))
			btn_id = i;
	}

	return btn_id;
}


void keybar_ctl::set_labels()
{
	//Clear all old values
	for (size_t i = 1; i <= 12; ++i)
		set_label(i, nullptr);

	switch (_state) {
		case st_normal:
			set_label( 1, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_help));
			if (_rw_mode)
				set_label( 2, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_save));
			set_label( 4, _PSI.GetMsg(_PSI.ModuleNumber, _rw_mode ? ps_fn_mode_ro : ps_fn_mode_rw));
			set_label( 5, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_goto));
			set_label( 7, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_find));
			set_label( 8, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_codepage));
			set_label( 9, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_setup));
			set_label(10, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_exit));
			break;
		case st_alt:
			set_label(7, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_prev));
			break;
		case st_shift:
			set_label(2, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_saveas));
			set_label(7, _PSI.GetMsg(_PSI.ModuleNumber, ps_fn_next));
			break;
		case st_ctrl:
		case st_undef:
			break;
		default:
			assert(false && "Undefined state");
	}
}

void keybar_ctl::set_label(const size_t num, const wchar_t* label)
{
	assert(num >= 1 && num <= 12);

	const size_t pos = get_btn_pos(num);

	if (label)
		write(0, pos, label, wcslen(label));
	else {
		//Clear old value
		for (size_t i = pos; i < pos + BTN_LABEL_LEN && i < _width; ++i)
			write(0, i, L' ');
	}
}


size_t keybar_ctl::get_btn_pos(const size_t num, const bool for_label /*= true*/) const
{
	assert(num >= 1 && num <= 12);

	const size_t btn_width = get_btn_width();
	size_t pos = (num - 1) * (btn_width + 2) + (num >= 10 ? num - 10 : 0);
	if (for_label)
		pos += num >= 10 ? 2 : 1;

	return pos;
}


size_t keybar_ctl::get_btn_width() const
{
	size_t btn_width = (_width - 11 /*spaces between buttons*/ - 15 /*buttons num label*/) / 12 /*buttons count*/;
	if (btn_width < BTN_LABEL_LEN)	//minimum button width
		btn_width = BTN_LABEL_LEN;
	return btn_width;
}
