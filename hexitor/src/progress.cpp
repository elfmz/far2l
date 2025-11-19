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

#include "progress.h"
#include "i18nindex.h"

#define PROGRESS_WIDTH 50


progress::progress(const wchar_t* title, const uint64_t min_value, const uint64_t max_value)
:	_title(title),
	_visible(false),
	_min_value(min_value), _max_value(max_value)
{
	assert(_title);
	update(_min_value);
	show();
}


progress::~progress()
{
	hide();
}


void progress::show()
{
	if (!_visible) {
		_visible = true;
		_PSI.AdvControl(_PSI.ModuleNumber, ACTL_SETPROGRESSSTATE, (void *)PGS_INDETERMINATE, nullptr);
	}

	const wchar_t* msg[] = { _title, _bar.c_str() };
	_PSI.Message(_PSI.ModuleNumber, FMSG_NONE, nullptr, msg, ARRAYSIZE(msg), 0);
}


void progress::hide()
{
	if (_visible) {
		_PSI.AdvControl(_PSI.ModuleNumber, ACTL_PROGRESSNOTIFY, 0, nullptr);
		_PSI.AdvControl(_PSI.ModuleNumber, ACTL_SETPROGRESSSTATE, (void *)PGS_NOPROGRESS, nullptr);
		_PSI.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
		_PSI.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
		_visible = false;
	}
}


void progress::update(const uint64_t val)
{
	assert(val >= (_max_value < _min_value ? _max_value : _min_value) && val <= (_max_value > _min_value ? _max_value : _min_value));

	const size_t percent = static_cast<size_t>(((val - _min_value) * 100) / (_max_value == _min_value ? 1 : _max_value - _min_value));
	assert(percent <= 100);

	PROGRESSVALUE pv;
	ZeroMemory(&pv, sizeof(pv));
	pv.Completed = percent;
	pv.Total = 100;
	_PSI.AdvControl(_PSI.ModuleNumber, ACTL_SETPROGRESSVALUE, &pv, 0);

	if (_bar.empty())
		_bar.resize(PROGRESS_WIDTH);
	const size_t fill_length = percent * _bar.size() / 100;
	fill(_bar.begin() + fill_length, _bar.end(), L'\x2591');
	fill(_bar.begin(), _bar.begin() + fill_length, L'\x2588');

	show();
}


bool progress::aborted()
{
	HANDLE std_in = 0; // Incorrect emulation - GetStdHandle(STD_INPUT_HANDLE) return non-zero
	INPUT_RECORD rec;
	DWORD read_count = 0;
	while (PeekConsoleInput(std_in, &rec, 1, &read_count) && read_count != 0) {
		ReadConsoleInput(std_in, &rec, 1, &read_count);
		if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && rec.Event.KeyEvent.bKeyDown)
			return true;
	}
	return false;
}
