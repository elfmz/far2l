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

#include "history.h"
#include <utils.h>
#include <KeyFileHelper.h>
#include <time.h>

#define MAX_HISTORY_ITEMS 10

#define INI_LOCATION InMyConfig("plugins/hexitor/config.ini")


history::history()
{
	load();
}


bool history::load_last_position(const wchar_t* file_name, UINT64& view_offset, UINT64& cursor_offset)
{
	assert(file_name && *file_name);

	map<wstring, hist>::iterator it_existing = _history.find(file_name);
	if (it_existing == _history.end())
		return false;
	view_offset = it_existing->second.offset_view;
	cursor_offset = it_existing->second.offset_cursor;
	return true;
}


void history::save_last_position(const wchar_t* file_name, const UINT64 view_offset, const UINT64 cursor_offset)
{
	assert(file_name && *file_name);

	time_t curr_time;
	time(&curr_time);

	map<wstring, hist>::iterator it_existing = _history.find(file_name);
	if (it_existing != _history.end()) {
		it_existing->second.datetime = curr_time;
		it_existing->second.offset_view = view_offset;
		it_existing->second.offset_cursor = cursor_offset;
	}
	else {
		if (_history.size() >= MAX_HISTORY_ITEMS) {
			//Remove oldest file description
			time_t oldest_time = _history.begin()->second.datetime;
			const wchar_t* oldest_file = _history.begin()->first.c_str();
			for (map<wstring, hist>::const_iterator it = ++_history.begin(); it != _history.end(); ++it) {
				if (it->second.datetime < oldest_time) {
					oldest_file = it->first.c_str();
					oldest_time = it->second.datetime;
				}
			}
			_history.erase(oldest_file);
		}

		assert(_history.size() <= MAX_HISTORY_ITEMS);

		//Add record
		hist h;
		h.datetime = curr_time;
		h.offset_view = view_offset;
		h.offset_cursor = cursor_offset;
		_history.insert(make_pair(file_name, h));
	}

	save();
}


void history::load()
{
	int i;
	char buff[100];
	_history.clear();

	for (i=0; i<MAX_HISTORY_ITEMS; ++i) {
	  	snprintf(buff, sizeof(buff), "History%02i", i);
		std::string ini_section(buff);
		KeyFileReadSection kfr(INI_LOCATION, ini_section);
		if (!kfr.SectionLoaded())
			return;

		hist h;
		unsigned long long n;
		n = kfr.GetULL("datetime", 0);
		h.datetime = (n>0) ? static_cast<time_t>(n) : time_t();
		n = kfr.GetULL("offset_view", 0);
		h.offset_view = (n>0) ? static_cast<UINT64>(n) : 0ULL;
		n = kfr.GetULL("offset_cursor", 0);
		h.offset_cursor = (n>0) ? static_cast<UINT64>(n) : 0ULL;

		std::wstring file_name(StrMB2Wide(kfr.GetString("file_name", "")));
		_history.insert(make_pair(file_name.c_str(), h));
	}
}


void history::save() const
{
	assert(_history.size() <= MAX_HISTORY_ITEMS);
	char buff[100];
	int idx = 0;

	for (map<wstring, hist>::const_iterator it = _history.begin(); it != _history.end(); ++it, ++idx) {
		std::snprintf(buff, sizeof(buff), "History%02i", idx);
		std::string ini_section(buff);
		KeyFileHelper kfw(INI_LOCATION);

		kfw.SetString(ini_section, "file_name", StrWide2MB(it->first).c_str());
		kfw.SetULL(ini_section, "datetime", static_cast<unsigned long long>(it->second.datetime));
		kfw.SetULL(ini_section, "offset_view", static_cast<unsigned long long>(it->second.offset_view));
		kfw.SetULL(ini_section, "offset_cursor", static_cast<unsigned long long>(it->second.offset_cursor));
	}
}
