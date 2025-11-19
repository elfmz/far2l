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
#include "i18nindex.h"
#include <utils.h>
#include <KeyFileHelper.h>
//#include <src/pick_color.hpp>

#define MAKEDWORD(low, high) ((DWORD)(((WORD)(low)) | ((DWORD)((WORD)(high))) << 16))

#define INI_LOCATION InMyConfig("plugins/hexitor/config.ini")
#define INI_SECTION "Settings"

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

Settings::Settings()
	: add_to_panel_menu(true),
	  add_to_editor_menu(true),
	  add_to_viewer_menu(false),
	  cmd_prefix(L"hex"),
	  save_file_pos(false),
	  show_dword_seps(true),
	  move_inside_byte(true),
	  std_cursor_size(false),
	  clr_active(MAKEDWORD(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, 0)),
	  clr_updated(MAKEDWORD(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY, 0)),
	  clr_offset(MAKEDWORD(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, 0)),
	  myDialog(nullptr)
{
}
void Settings::load()
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
	clr_active = static_cast<FarColor>(kfr.GetULL(param_clr_active, 0));
	clr_updated = static_cast<FarColor>(kfr.GetULL(param_clr_updated, 0));
	clr_offset = static_cast<FarColor>(kfr.GetULL(param_clr_offset, 0));
}


void Settings::save()
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
	kfh.SetULL(INI_SECTION, param_clr_active, clr_active);
	kfh.SetULL(INI_SECTION, param_clr_updated, clr_updated);
	kfh.SetULL(INI_SECTION, param_clr_offset, clr_offset);
}

void Settings::configure()
{
	fardialog::DlgCHECKBOX checkbox0("add_pm", I18N(ps_cfg_add_pm), add_to_panel_menu);
	fardialog::DlgCHECKBOX checkbox1("add_em", I18N(ps_cfg_add_em), add_to_editor_menu);
	fardialog::DlgCHECKBOX checkbox2("add_vm", I18N(ps_cfg_add_vm), add_to_viewer_menu);
	
	fardialog::DlgTEXT text1(nullptr, I18N(ps_cfg_prefix));
	fardialog::DlgEDIT edit1("prefix", 10);
	fardialog::DlgHLine hline1(nullptr, nullptr);
	fardialog::DlgCHECKBOX checkbox3("save_pos", I18N(ps_cfg_save_pos), save_file_pos);
	fardialog::DlgCHECKBOX checkbox4("move_ib", I18N(ps_cfg_move_ib), move_inside_byte);
	fardialog::DlgCHECKBOX checkbox5("std_csize", I18N(ps_cfg_std_csize), std_cursor_size);
	fardialog::DlgCHECKBOX checkbox6("show_dd", I18N(ps_cfg_show_dd), show_dword_seps);
	fardialog::DlgHLine hline2(nullptr, I18N(ps_cfg_clr_title));
	fardialog::DlgBUTTON button1("clr_offset", I18N(ps_cfg_clr_offset), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
	fardialog::DlgBUTTON button2("clr_active", I18N(ps_cfg_clr_active), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
	fardialog::DlgBUTTON button3("clr_updated", I18N(ps_cfg_clr_updated), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
	fardialog::DlgHLine hline3(nullptr, nullptr);
	fardialog::DlgBUTTON button4("bn_ok", I18N(ps_ok), DIF_CENTERGROUP | DIF_DEFAULT, 0, 1);
	fardialog::DlgBUTTON button5("bn_cancel", I18N(ps_cancel), DIF_CENTERGROUP);

	fardialog::DlgHSizer hbox1({&text1, &edit1});
	fardialog::DlgVSizer vbox2({&checkbox4, &checkbox5, &checkbox6});
	fardialog::DlgHSizer hbox3({&button2, &button3});
	fardialog::DlgHSizer hbox4({&button4, &button5});

	fardialog::DlgVSizer vbox1({
		&checkbox0,
		&checkbox1,
		&checkbox2,
		&hbox1,
		&hline1,
		&checkbox3,
		&vbox2,
		&hline2,
		&button1,
		&hbox3,
		&hline3,
		&hbox4
	});
	auto dlg = fardialog::CreateDialog(
		I18N(ps_title),
		I18N(ps_helptopic),
		0,
		*this,
		&Settings::dlg_proc,
		vbox1
	);

	myDialog = &dlg;
	FarColor save_clr_active = clr_active;
	FarColor save_clr_updated = clr_updated;
	FarColor save_clr_offset = clr_offset;


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
	} else {
		clr_active = save_clr_active;
		clr_updated = save_clr_updated;
		clr_offset = save_clr_offset;
	}
	_PSI.DialogFree(hDlg);
}


LONG_PTR WINAPI Settings::dlg_proc(HANDLE dlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (Msg == DN_BTNCLICK) {
		FarColor *fc = nullptr;
		if (Param1 == myDialog->getID("clr_offset"))
			fc = &clr_offset;
		else if (Param1 == myDialog->getID("clr_active"))
			fc = &clr_active;
		else if (Param1 == myDialog->getID("clr_updated"))
			fc = &clr_updated;
		if (fc){
			_PSI.ColorDialog(0, fc);
			return 1;
		}
	}
	return _PSI.DefDlgProc(dlg, Msg, Param1, Param2);
}
