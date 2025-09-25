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

#include "settings.h"
#include "string_rc.h"
#include <utils.h>
#include <KeyFileHelper.h>
//#include <src/pick_color.hpp>
#include "fardialog.h"

#define MAKEDWORD(low, high) ((DWORD)(((WORD)(low)) | ((DWORD)((WORD)(high))) << 16))

#define INI_LOCATION InMyConfig("plugins/hexitor/config.ini")
#define INI_SECTION "Settings"

bool settings::add_to_panel_menu = true;
bool settings::add_to_editor_menu = true;
bool settings::add_to_viewer_menu = true;
wstring settings::cmd_prefix = L"hex";
bool settings::save_file_pos = true;
bool settings::show_dword_seps = true;
bool settings::move_inside_byte = true;
bool settings::std_cursor_size = false;
FarColor settings::clr_active = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
FarColor settings::clr_updated = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
FarColor settings::clr_offset = FOREGROUND_BLUE | FOREGROUND_GREEN | BACKGROUND_BLUE;
void *settings::Dialog = nullptr;

const char* param_add_to_panel_menu = "add_to_panel_menu";
const char* param_add_to_editor_menu = "add_to_editor_menu";
const char* param_add_to_viewer_menu = "add_to_viewer_menu";
const char* param_cmd_prefix = "cmd_prefix";
const char* param_save_file_pos = "save_file_pos";
const char* param_show_dword_seps = "show_dword_seps";
const char* param_move_inside_byte = "move_inside_byte";
const char* param_clr_active = "clr_active";
const char* param_clr_updated = "clr_updated";
const char* param_clr_offset = "clr_offset";
const char* param_std_cursor_size = "std_cursor_size";

void settings::load()
{
	KeyFileReadSection kfr(INI_LOCATION, INI_SECTION);
	if (!kfr.SectionLoaded())
		return;
	add_to_panel_menu = kfr.GetInt(param_add_to_panel_menu, 0) != 0;
	add_to_editor_menu = kfr.GetInt(param_add_to_editor_menu, 0) != 0;
	add_to_viewer_menu = kfr.GetInt(param_add_to_viewer_menu, 0) != 0;
	cmd_prefix = kfr.GetString(param_cmd_prefix, L"hex");
	save_file_pos = kfr.GetInt(param_save_file_pos, 0) != 0;
	show_dword_seps = kfr.GetInt(param_show_dword_seps, 0) != 0;
	move_inside_byte = kfr.GetInt(param_move_inside_byte, 0) != 0;
	std_cursor_size = kfr.GetInt(param_std_cursor_size, 0) != 0;
	clr_active = static_cast<FarColor>(kfr.GetInt(param_clr_active, 0));
	clr_updated = static_cast<FarColor>(kfr.GetInt(param_clr_updated, 0));
	clr_offset = static_cast<FarColor>(kfr.GetInt(param_clr_offset, 0));
}


void settings::save()
{
	KeyFileHelper kfh(INI_LOCATION);
	kfh.SetInt(INI_SECTION, param_add_to_panel_menu, add_to_panel_menu ? 1 : 0);
	kfh.SetInt(INI_SECTION, param_add_to_editor_menu, add_to_editor_menu ? 1 : 0);
	kfh.SetInt(INI_SECTION, param_add_to_viewer_menu, add_to_viewer_menu ? 1 : 0);
	kfh.SetString(INI_SECTION, param_cmd_prefix, cmd_prefix.c_str());
	kfh.SetInt(INI_SECTION, param_save_file_pos, save_file_pos ? 1 : 0);
	kfh.SetInt(INI_SECTION, param_show_dword_seps, show_dword_seps ? 1 : 0);
	kfh.SetInt(INI_SECTION, param_move_inside_byte, move_inside_byte ? 1 : 0);
	kfh.SetInt(INI_SECTION, param_std_cursor_size, std_cursor_size ? 1 : 0);
	kfh.SetInt(INI_SECTION, param_clr_active, clr_active);
	kfh.SetInt(INI_SECTION, param_clr_updated, clr_updated);
	kfh.SetInt(INI_SECTION, param_clr_offset, clr_offset);
}

void settings::configure()
{
	fardialog::DlgCHECKBOX checkbox0("add_pm", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_add_pm), add_to_panel_menu);
	fardialog::DlgCHECKBOX checkbox1("add_em", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_add_em), add_to_editor_menu);
	fardialog::DlgCHECKBOX checkbox2("add_vm", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_add_vm), add_to_viewer_menu);
	
	fardialog::DlgTEXT text1(nullptr, _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_prefix));
	fardialog::DlgEDIT edit1("prefix", 10);
	fardialog::DlgHLine hline1;
	fardialog::DlgCHECKBOX checkbox3("save_pos", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_save_pos), save_file_pos);
	fardialog::DlgCHECKBOX checkbox4("move_ib", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_move_ib), move_inside_byte);
	fardialog::DlgCHECKBOX checkbox5("std_csize", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_std_csize), std_cursor_size);
	fardialog::DlgCHECKBOX checkbox6("show_dd", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_show_dd), show_dword_seps);
	fardialog::DlgHLine hline2(nullptr, _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_clr_title));
	fardialog::DlgBUTTON button1("clr_offset", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_clr_offset), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
	fardialog::DlgBUTTON button2("clr_active", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_clr_active), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
	fardialog::DlgBUTTON button3("clr_updated", _PSI.GetMsg(_PSI.ModuleNumber, ps_cfg_clr_updated), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
	fardialog::DlgHLine hline3;
	fardialog::DlgBUTTON button4("bn_ok", _PSI.GetMsg(_PSI.ModuleNumber, ps_ok), DIF_CENTERGROUP | DIF_DEFAULT, 0, 1);
	fardialog::DlgBUTTON button5("bn_cancel", _PSI.GetMsg(_PSI.ModuleNumber, ps_cancel), DIF_CENTERGROUP);

	std::vector<fardialog::Window*> hbox1c = {&text1, &edit1};
	fardialog::DlgHSizer hbox1(hbox1c);
	std::vector<fardialog::Window*> hbox2c = {&checkbox4, &checkbox5, &checkbox6};
	fardialog::DlgVSizer hbox2(hbox2c);
	std::vector<fardialog::Window*> hbox3c = {&button2, &button3};
	fardialog::DlgHSizer hbox3(hbox3c);
	std::vector<fardialog::Window*> hbox4c = {&button4, &button5};
	fardialog::DlgHSizer hbox4(hbox4c);

	std::vector<fardialog::Window*> vbox1c = {
		&checkbox0,
		&checkbox1,
		&checkbox2,
		&hbox1,
		&hline1,
		&checkbox3,
		&hbox2,
		&hline2,
		&button1,
		&hbox3,
		&hline3,
		&hbox4
	};
	fardialog::DlgVSizer vbox1(vbox1c);
	fardialog::Dialog dlg(&_PSI, _PSI.GetMsg(_PSI.ModuleNumber, ps_title), _PSI.GetMsg(_PSI.ModuleNumber, ps_helptopic), 0, &settings::dlg_proc, 0);
	dlg.buildFDI(&vbox1);

	settings::Dialog = &dlg;

	const HANDLE hDlg = dlg.DialogInit();
	const intptr_t rc = _PSI.DialogRun(hDlg);
	if (rc >= 0 && rc == dlg.getID("bn_ok") ) {
		add_to_panel_menu = dlg.GetCheck(dlg.getID("add_pm")) != 0;
		add_to_editor_menu = dlg.GetCheck(dlg.getID("add_em")) != 0;
		add_to_viewer_menu = dlg.GetCheck(dlg.getID("add_vm")) != 0;
		cmd_prefix = dlg.GetText(dlg.getID("prefix"));
		save_file_pos = dlg.GetCheck(dlg.getID("save_pos")) != 0;
		move_inside_byte = dlg.GetCheck(dlg.getID("move_ib")) != 0;
		std_cursor_size = dlg.GetCheck(dlg.getID("std_csize")) != 0;
		show_dword_seps = dlg.GetCheck(dlg.getID("show_dd")) != 0;
		save();
	}
	_PSI.DialogFree(hDlg);
}


LONG_PTR WINAPI settings::dlg_proc(HANDLE dlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (Msg == DN_BTNCLICK) {
		FarColor *fc = nullptr;
		fardialog::Dialog *dlg = static_cast<fardialog::Dialog*>(settings::Dialog);
		if (Param1 == dlg->getID("clr_offset"))
			fc = &settings::clr_offset;
		else if (Param1 == dlg->getID("clr_active"))
			fc = &settings::clr_active;
		else if (Param1 == dlg->getID("clr_updated"))
			fc = &settings::clr_updated;
		if (fc)
			_PSI.ColorDialog(0, fc);
	}
	return _PSI.DefDlgProc(dlg, Msg, Param1, Param2);
}
