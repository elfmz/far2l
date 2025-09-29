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

#include "common.h"
#include "version.h"
#include "string_rc.h"
#include "editor.h"
#include "settings.h"
#include "farapi.h"

PluginStartupInfo    _PSI;
FarStandardFunctions _FSF;

SHAREDSYMBOL void WINAPI _export SetStartupInfoW(const PluginStartupInfo* psi)
{
	_PSI = *psi;
	_FSF = *psi->FSF;
	_PSI.FSF = &_FSF;

	settings::load();
}


SHAREDSYMBOL void WINAPI _export GetPluginInfoW(PluginInfo* info)
{
	assert(info);

	info->StructSize = sizeof(PluginInfo);
	if (settings::add_to_viewer_menu)
		info->Flags |= PF_VIEWER;
	if (settings::add_to_editor_menu)
		info->Flags |= PF_EDITOR;
	if (!settings::add_to_panel_menu)
		info->Flags |= PF_DISABLEPANELS;

	if (!settings::cmd_prefix.empty())
		info->CommandPrefix = settings::cmd_prefix.c_str();

	static const wchar_t* menu_strings[1];
	menu_strings[0] = _PSI.GetMsg(_PSI.ModuleNumber, ps_title);

	info->PluginConfigStrings = menu_strings;
	info->PluginConfigStringsNumber = sizeof(menu_strings) / sizeof(menu_strings[0]);

	info->PluginMenuStrings = menu_strings;
	info->PluginMenuStringsNumber = sizeof(menu_strings) / sizeof(menu_strings[0]);

#ifdef _DEBUG
	info->Flags |= PF_PRELOAD;
#endif // _DEBUG
}


SHAREDSYMBOL HANDLE WINAPI _export OpenPluginW(int openFrom, INT_PTR item)
{
	UINT64 file_offset = 0;
	std::wstring file_name;

	//Determine file name for open
	if (openFrom == OPEN_COMMANDLINE && item) {
		file_name = (const wchar_t *)item;
	}
	else if (openFrom == OPEN_PLUGINSMENU) {
		FarApi api;
		if( auto ppi = api.GetCurrentPanelItem() ) {
			if( _FSF.LStricmp(ppi->FindData.lpwszFileName, L"..") != 0 )
				file_name = ppi->FindData.lpwszFileName;
			api.FreePanelItem(ppi);
		}
	}
	else if (openFrom == OPEN_VIEWER) {
		ViewerInfo vi;
		ZeroMemory(&vi, sizeof(vi));
		vi.StructSize = sizeof(vi);
		if (_PSI.ViewerControl(VCTL_GETINFO, &vi)){
			file_name = vi.FileName;
			file_offset = vi.FilePos;
		}
	}
	else if (openFrom == OPEN_EDITOR) {
		const int buff_len = _PSI.EditorControl(ECTL_GETFILENAME, 0);
		if (buff_len) {
			file_name.resize(buff_len + 1, 0);
			_PSI.EditorControl(ECTL_GETFILENAME, static_cast<void *>(&file_name.front()));
		}
	}

	if (!file_name.empty()) {
		CreateEditor(file_name.c_str(), file_offset);
	}

	return INVALID_HANDLE_VALUE;
}


SHAREDSYMBOL int WINAPI _export ConfigureW(int /*itemNumber*/)
{
	settings::configure();
	return 0;
}
