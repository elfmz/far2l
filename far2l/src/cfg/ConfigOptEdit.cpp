/*
config.cpp

Конфигурация
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include <algorithm>
#include "config.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "panel.hpp"
#include "help.hpp"
#include "filefilter.hpp"
#include "poscache.hpp"
#include "findfile.hpp"
#include "hilight.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "panelmix.hpp"
#include "strmix.hpp"
#include "udlist.hpp"
#include "datetime.hpp"
#include "DialogBuilder.hpp"
#include "vtshell.h"
#include "ConfigRW.hpp"
#include "AllXLats.hpp"
#include "ConfigOpt.hpp"

class ConfigOptProps
{
	const ConfigOpt &_opt;

public:
	inline ConfigOptProps(const ConfigOpt &opt) : _opt(opt)
	{
	}

	//  0 - is default
	//  1 - not default
	// -1 - error or has not default
	int IsNotDefault() const
	{
		switch (_opt.type)
		{
			case ConfigOpt::T_BOOL:
				return (*_opt.value.b != _opt.def.b);
			case ConfigOpt::T_INT:
				return (*_opt.value.i != _opt.def.i);
			case ConfigOpt::T_DWORD:
				return (*_opt.value.dw != _opt.def.dw);
			case ConfigOpt::T_STR:
				return (*_opt.value.str != _opt.def.str);
			case ConfigOpt::T_BIN:
				return (_opt.def.bin == nullptr || _opt.value.bin == nullptr ? -1
						: ( memcmp(_opt.value.bin, _opt.def.bin, _opt.bin_size) == 0 ? 0 : 1 ));
			default:
				return -1; // can not process unknown type
		}
	}

	// 0 - was default, not changed
	// 1 - changed to default
	// -1 - error or has not default
	int ToDefault() const
	{
		switch (_opt.type)
		{
			case ConfigOpt::T_BOOL:
				if (*_opt.value.b == _opt.def.b)
					return 0;
				*_opt.value.b = _opt.def.b;
				return 1;
			case ConfigOpt::T_INT:
				if (*_opt.value.i == _opt.def.i)
					return 0;
				*_opt.value.i = _opt.def.i;
				return 1;
			case ConfigOpt::T_DWORD:
				if (*_opt.value.dw == _opt.def.dw)
					return 0;
				*_opt.value.dw = _opt.def.dw;
				return 1;
			case ConfigOpt::T_STR:
				if (*_opt.value.str == _opt.def.str)
					return 0;
				*_opt.value.str = _opt.def.str;
				return 1;
			case ConfigOpt::T_BIN:
				return -1; // can not process binary
			default:
				return -1; // can not process unknown type
		}
	}

	void GetMaxLengthSectKeys(size_t &len_sections, size_t &len_keys, size_t &len_sections_keys) const
	{
		const size_t len_section = strlen(_opt.section);
		if (len_section > len_sections)
			len_sections = len_section;
		const size_t len_key = strlen(_opt.key);
		if (len_key > len_keys )
			len_keys = len_key;
		const size_t len_section_key = len_section + len_key + 1;
		if (len_section_key > len_sections_keys)
			len_sections_keys = len_section_key;
	}

	void MenuListAppend(VMenu &vm,
					size_t len_sections, size_t len_keys, size_t len_sections_keys,
					bool hide_unchanged, bool align_dot,
					int update_id = -1) const
	{
		MenuItemEx mi;
		FARString fsn, fssave;
		if (align_dot)
		 fsn.Format(L"%*s.%-*s", len_sections, _opt.section, len_keys, _opt.key);
		else {
			mi.strName.Format(L"%s.%s", _opt.section, _opt.key);
			fsn.Format(L"%-*ls", len_sections_keys, mi.strName.CPtr());
		}
		fssave = (_opt.save ? "s" : "-");
		switch (_opt.type)
		{
			case ConfigOpt::T_BOOL:
				mi.strName.Format(L"%s %ls |  bool|%ls|%s",
					(*_opt.value.b == _opt.def.b ? " " : "*"), fsn.CPtr(), fssave.CPtr(), (*_opt.value.b ? "true" : "false"));
				break;
			case ConfigOpt::T_INT:
				mi.strName.Format(L"%s %ls |   int|%ls|%ld = 0x%lx",
					(*_opt.value.i == _opt.def.i ? " " : "*"), fsn.CPtr(), fssave.CPtr(), *_opt.value.i, *_opt.value.i);
				break;
			case ConfigOpt::T_DWORD:
				mi.strName.Format(L"%s %ls | dword|%ls|%lu = 0x%lx",
					(*_opt.value.dw == _opt.def.dw ? " " : "*"), fsn.CPtr(), fssave.CPtr(), *_opt.value.dw, *_opt.value.dw);
				break;
			case ConfigOpt::T_STR:
				mi.strName.Format(L"%s %ls |string|%ls|%ls",
					(*_opt.value.str == _opt.def.str ? " " : "*"), fsn.CPtr(), fssave.CPtr(), _opt.value.str->CPtr());
				break;
			case ConfigOpt::T_BIN:
				mi.strName.Format(L"%s %ls |binary|%ls|(binary has length %u bytes)",
					(_opt.def.bin == nullptr || _opt.value.bin == nullptr ? "?"
						: ( memcmp(_opt.value.bin, _opt.def.bin, _opt.bin_size) == 0 ? " " : "*")),
					fsn.CPtr(), fssave.CPtr(), _opt.bin_size );
				break;
			default:
				mi.strName.Format(L"? %ls |unknown type ???", fsn.CPtr());
		}
		if (update_id < 0) {
			if (hide_unchanged && mi.strName.At(0)==L' ') // no hide after change item to default value
				mi.Flags |= LIF_HIDDEN;
			vm.AddItem(&mi);
		}
		else {
			vm.DeleteItem(update_id);
			vm.AddItem(&mi, update_id);
			vm.SetSelectPos(update_id, 0);
		}
	}

	int Msg(const wchar_t *title) const
	{
		const char *type_psz;
		FARString def_str, val_str;
		switch (_opt.type) {
			case ConfigOpt::T_BOOL:
				type_psz = "bool";
				def_str = _opt.def.b ? L"true" : L"false";
				val_str = (*_opt.value.b) ? L"true" : L"false";
				break;
			case ConfigOpt::T_INT:
				type_psz = "int";
				def_str.Format(L"%ld = 0x%lx", _opt.def.i, _opt.def.i);
				val_str.Format(L"%ld = 0x%lx", *_opt.value.i, *_opt.value.i);
				break;
			case ConfigOpt::T_DWORD:
				type_psz = "dword";
				def_str.Format(L"%lu = 0x%lx", _opt.def.dw, _opt.def.dw);
				val_str.Format(L"%lu = 0x%lx", *_opt.value.dw, *_opt.value.dw);
				break;
			case ConfigOpt::T_STR:
				type_psz = "string";
				def_str = _opt.def.str ? _opt.def.str : L"(null)";
				val_str = *_opt.value.str;
				break;
			case ConfigOpt::T_BIN:
				type_psz = "binary";
				if (_opt.def.bin) {
					def_str.Format(L"(binary has length %u bytes)", _opt.bin_size);
				} else {
					def_str = L"(no default value set)";
				}
				val_str.Format(L"(binary has length %u bytes)", _opt.bin_size);
				break;
			default:
				type_psz = "???";
				def_str = L"???";
				val_str = L"???";
		}

		ExMessager em;
		em.AddFormat(L"%ls - %s.%s", title, _opt.section, _opt.key);
		em.AddFormat(L"        Section: %s", _opt.section);
		em.AddFormat(L"            Key: %s", _opt.key);
		em.AddFormat(L" to config file: %s", (_opt.save ? "saved" : "never"));
		em.AddFormat(L"           Type: %s", type_psz);
		em.AddFormat(L"  Default value: %ls", def_str.CPtr());
		em.AddFormat(L"  Current value: %ls", val_str.CPtr());
		if (IsNotDefault()==1) {
			em.Add(L"");
			em.Add(L"Note: some parameters after update/reset");
			em.Add(L"      not applied immediatly in FAR2L");
			em.Add(L"      and need relaunch feature");
			em.Add(L"      or may be need save config & restart FAR2L");
		}
		em.Add(L"Continue");
		SetMessageHelp(L"FarConfig");
		if (IsNotDefault()==1) {
			em.Add(L"Reset to default");
			return em.Show(MSG_LEFTALIGN, 2);
		}
		return em.Show(MSG_LEFTALIGN, 1);
	}

	static LONG_PTR WINAPI EditDlgDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
	{
		if (Msg == DN_BTNCLICK) {
			if ( Param1 == 32 && !SendDlgMessage(hDlg, DM_ENABLE, 29, -1) ) { // to decimal
				SendDlgMessage(hDlg, DM_ENABLE, 29, TRUE);
				SendDlgMessage(hDlg, DM_ENABLE, 31, FALSE);
			}
			else if ( Param1 == 33 && !SendDlgMessage(hDlg, DM_ENABLE, 31, -1) ) { // to hex
				SendDlgMessage(hDlg, DM_ENABLE, 29, FALSE);
				SendDlgMessage(hDlg, DM_ENABLE, 31, TRUE);
			}
		}

		return DefDlgProc(hDlg, Msg, Param1, Param2);
	}

	bool EditDlg(const wchar_t *title) const
	{
		bool is_editable = false, is_def = true;
		const wchar_t *type_pwsz;
		FARString fs_title,
				def_str, cur_str, new_str, def_str_hex, cur_str_hex, new_str_hex,
				fs_section = _opt.section, fs_key = _opt.key;
		fs_title.Format(L"%ls - %s.%s", title, _opt.section, _opt.key);

		switch (_opt.type) {
			case ConfigOpt::T_BOOL:
				type_pwsz = L"bool";
				is_editable = true;
				break;
			case ConfigOpt::T_INT:
				type_pwsz = L"int";
				is_editable = true;
				def_str.Format(L"%ld", _opt.def.i);
				cur_str.Format(L"%ld", *_opt.value.i);
				def_str_hex.Format(L"%lx", _opt.def.i);
				cur_str_hex.Format(L"%lx", *_opt.value.i);
				new_str = cur_str;
				new_str_hex = cur_str_hex;
				break;
			case ConfigOpt::T_DWORD:
				type_pwsz = L"dword";
				is_editable = true;
				def_str.Format(L"%lu", _opt.def.dw);
				cur_str.Format(L"%lu", *_opt.value.dw);
				def_str_hex.Format(L"%lx", _opt.def.dw);
				cur_str_hex.Format(L"%lx", *_opt.value.dw);
				new_str = cur_str;
				new_str_hex = cur_str_hex;
				break;
			case ConfigOpt::T_STR:
				type_pwsz = L"string";
				is_editable = true;
				is_def = (bool) _opt.def.str;
				def_str = _opt.def.str ? _opt.def.str : L"";
				cur_str = *_opt.value.str;
				new_str = cur_str;
				break;
			case ConfigOpt::T_BIN:
				type_pwsz = L"binary";
				if (_opt.def.bin) {
					def_str.Format(L"(binary has length %u bytes)", _opt.bin_size);
				} else {
					is_def = false;
					def_str = L"(no default value set)";
				}
				cur_str.Format(L"(binary has length %u bytes)", _opt.bin_size);
				new_str	= L"(can not process binary)";
				break;
			default:
				type_pwsz = L"???";
				def_str = L"???";
				cur_str = L"???";
				new_str	= L"(can not process unknown type)";
		}

		const short DLG_HEIGHT = 20, DLG_WIDTH = 76;
		const wchar_t *mask_int = L"#9999999999";
		const wchar_t *mask_dword = L"9999999999";
		const wchar_t *HexMask = L"HHHHHHHH";
		DialogDataEx AdvancedConfigDlgData[] = {
			/*   0 */ {DI_DOUBLEBOX,	 3,  1, DLG_WIDTH - 4, DLG_HEIGHT - 2, {}, 0, fs_title.CPtr()},
			/*   1 */ {DI_TEXT,			 5,  2, 20,             2, {}, 0, L"       Section:"},
			/*   2 */ {DI_TEXT,			21,  2, DLG_WIDTH - 6,  2, {}, 0, fs_section.CPtr()},
			/*   3 */ {DI_TEXT,			 5,  3, 20,             3, {}, 0, L"           Key:"},
			/*   4 */ {DI_TEXT,			21,  3, DLG_WIDTH - 6,  3, {}, 0, fs_key.CPtr()},
			/*   5 */ {DI_TEXT,			 5,  4, 20,             4, {}, 0, L"to config file:"},
			/*   6 */ {DI_TEXT,			21,  4, DLG_WIDTH - 6,  4, {}, 0, (_opt.save ? L"saved" : L"never")},
			/*   7 */ {DI_TEXT,			 5,  5, 20,             5, {}, 0, L"          Type:"},
			/*   8 */ {DI_TEXT,			21,  5, DLG_WIDTH - 6,  5, {}, 0, type_pwsz},
			/*   9 */ {DI_TEXT,			 3,  6, 20,             6, {}, DIF_SEPARATOR, L" Values "},
			/*  10 */ {DI_TEXT,			 5,  7, 13,             7, {}, 0, L"Default:"},
			/*  11 */ {DI_RADIOBUTTON,	14,  7,  0,             7, {}, DIF_DISABLE | DIF_GROUP, L"false"},
			/*  12 */ {DI_RADIOBUTTON,	29,  7,  0,             7, {}, DIF_DISABLE, L"true"},
			/*  13 */ {DI_TEXT,			14,  7, 21,             7, {}, 0, L"Decimal="},
			/*  14 */ {DI_EDIT,			22,  7, 32,             7, {}, DIF_READONLY | DIF_SELECTONENTRY, def_str.CPtr()},
			/*  15 */ {DI_TEXT,			35,  7, 40,             7, {}, 0, L"Hex=0x"},
			/*  16 */ {DI_EDIT,			41,  7, 49,             7, {}, DIF_READONLY | DIF_SELECTONENTRY, def_str_hex.CPtr()},
			/*  17 */ {DI_TEXT,			 5,  8, 13,             8, {}, 0, L"Current:"},
			/*  18 */ {DI_RADIOBUTTON,	14,  8,  0,             8, {}, DIF_DISABLE | DIF_GROUP, L"false"},
			/*  19 */ {DI_RADIOBUTTON,	29,  8,  0,             8, {}, DIF_DISABLE, L"true"},
			/*  20 */ {DI_TEXT,			14,  8, 21,             8, {}, 0, L"Decimal="},
			/*  21 */ {DI_EDIT,			22,  8, 32,             8, {}, DIF_READONLY | DIF_SELECTONENTRY, cur_str.CPtr()},
			/*  22 */ {DI_TEXT,			35,  8, 40,             8, {}, 0, L"Hex=0x"},
			/*  23 */ {DI_EDIT,			41,  8, 49,             8, {}, DIF_READONLY | DIF_SELECTONENTRY, cur_str_hex.CPtr()},
			/*  24 */ {DI_TEXT,			 3,  9, 20,             9, {}, DIF_SEPARATOR, L" New value "},
			/*  25 */ {DI_TEXT,			 5, 10, 13,            10, {}, (is_editable ? 0 : DIF_DISABLE), L"    New:"},
			/*  26 */ {DI_RADIOBUTTON,	14, 10, 14,            10, {}, (is_editable ? DIF_FOCUS : DIF_DISABLE) | DIF_GROUP, L"false"},
			/*  27 */ {DI_RADIOBUTTON,	29, 10, 14,            10, {}, (is_editable ? 0 : DIF_DISABLE), L"true"},
			/*  28 */ {DI_TEXT,			14, 10, 21,            10, {}, 0, L"Decimal="},
			/*  29 */ {DI_EDIT,			22, 10, 32,            10, {}, (is_editable ? DIF_FOCUS : DIF_DISABLE) | DIF_SELECTONENTRY, new_str.CPtr()},
			/*  30 */ {DI_TEXT,			35, 10, 40,            10, {}, 0, L"Hex=0x"},
			/*  31 */ {DI_FIXEDIT,		41, 10, 49,            10, {(DWORD_PTR)HexMask}, DIF_MASKEDIT | DIF_DISABLE | DIF_SELECTONENTRY, new_str_hex.CPtr()},
			/*  32 */ {DI_RADIOBUTTON,	51, 10, 58,            10, {1}, (is_editable ? 0 : DIF_DISABLE) | DIF_GROUP, L"dec"},
			/*  33 */ {DI_RADIOBUTTON,	59, 10, 65,            10, {}, (is_editable ? 0 : DIF_DISABLE), L"hex"},
			/*  34 */ {DI_TEXT,		3, 11, 20,            11, {}, DIF_SEPARATOR, L""},
			/*  35 */ {DI_TEXT,		5, 12, DLG_WIDTH - 6, 12, {}, DIF_SHOWAMPERSAND, L"Note: some parameters after update/reset"},
			/*  36 */ {DI_TEXT,		5, 13, DLG_WIDTH - 6, 13, {}, DIF_SHOWAMPERSAND, L"      not applied immediatly in FAR2L"},
			/*  37 */ {DI_TEXT,		5, 14, DLG_WIDTH - 6, 14, {}, DIF_SHOWAMPERSAND, L"      and need relaunch feature"},
			/*  38 */ {DI_TEXT,		5, 15, DLG_WIDTH - 6, 15, {}, DIF_SHOWAMPERSAND, L"      or may be need save config & restart FAR2L"},
			/*  39 */ {DI_TEXT,		3, 16, 20, 16, {}, DIF_SEPARATOR, L""},
			/*  40 */ {DI_BUTTON,	0, 17, 0,  17, {}, DIF_DEFAULT | DIF_CENTERGROUP | (is_editable ? 0 : DIF_DISABLE), Msg::Change},
			/*  41 */ {DI_BUTTON,	0, 17, 0,  17, {}, DIF_CENTERGROUP | (is_editable ? 0 : DIF_FOCUS), Msg::Cancel}
		};
		if (!is_def) {
			AdvancedConfigDlgData[10].Flags	|= DIF_DISABLE;
			AdvancedConfigDlgData[11].Flags	|= DIF_DISABLE;
			AdvancedConfigDlgData[12].Flags	|= DIF_DISABLE;
			AdvancedConfigDlgData[13].Flags	|= DIF_DISABLE;
			AdvancedConfigDlgData[14].Flags	|= DIF_DISABLE;
			AdvancedConfigDlgData[15].Flags	|= DIF_DISABLE;
			AdvancedConfigDlgData[16].Flags	|= DIF_DISABLE;
		}
		if (_opt.type == ConfigOpt::T_BOOL) {
			AdvancedConfigDlgData[_opt.def.b ? 12 : 11].Selected = 1;
			AdvancedConfigDlgData[(*_opt.value.b) ? 19 : 18].Selected = 1;
			AdvancedConfigDlgData[(*_opt.value.b) ? 27 : 26].Selected = 1;
			AdvancedConfigDlgData[13].Flags =
			AdvancedConfigDlgData[14].Flags =
			AdvancedConfigDlgData[15].Flags =
			AdvancedConfigDlgData[16].Flags =
			AdvancedConfigDlgData[20].Flags =
			AdvancedConfigDlgData[21].Flags =
			AdvancedConfigDlgData[22].Flags =
			AdvancedConfigDlgData[23].Flags =
			AdvancedConfigDlgData[28].Flags =
			AdvancedConfigDlgData[29].Flags =
			AdvancedConfigDlgData[30].Flags =
			AdvancedConfigDlgData[31].Flags =
			AdvancedConfigDlgData[32].Flags =
			AdvancedConfigDlgData[33].Flags = DIF_HIDDEN;
		}
		else {
			AdvancedConfigDlgData[11].Flags	=
			AdvancedConfigDlgData[12].Flags	=
			AdvancedConfigDlgData[18].Flags =
			AdvancedConfigDlgData[19].Flags =
			AdvancedConfigDlgData[26].Flags =
			AdvancedConfigDlgData[27].Flags	= DIF_HIDDEN;
			if (_opt.type==ConfigOpt::T_DWORD) {
				AdvancedConfigDlgData[29].Type = DI_FIXEDIT;
				AdvancedConfigDlgData[29].Flags |= DIF_MASKEDIT;
				AdvancedConfigDlgData[29].Mask = mask_dword;
			}
			else if (_opt.type==ConfigOpt::T_INT) {
				AdvancedConfigDlgData[29].Type = DI_FIXEDIT;
				AdvancedConfigDlgData[29].Flags |= DIF_MASKEDIT;
				AdvancedConfigDlgData[29].Mask = mask_int;
			}
			else { // T_STR & T_BIN
				AdvancedConfigDlgData[14].X1 = AdvancedConfigDlgData[21].X1 = AdvancedConfigDlgData[29].X1 = 14;
				AdvancedConfigDlgData[14].X2 = AdvancedConfigDlgData[21].X2 = AdvancedConfigDlgData[29].X2 = DLG_WIDTH - 6;
				AdvancedConfigDlgData[13].Flags =
				AdvancedConfigDlgData[15].Flags =
				AdvancedConfigDlgData[16].Flags =
				AdvancedConfigDlgData[20].Flags =
				AdvancedConfigDlgData[22].Flags =
				AdvancedConfigDlgData[23].Flags =
				AdvancedConfigDlgData[28].Flags =
				AdvancedConfigDlgData[30].Flags =
				AdvancedConfigDlgData[31].Flags =
				AdvancedConfigDlgData[32].Flags =
				AdvancedConfigDlgData[33].Flags = DIF_HIDDEN;
			}
		}
		MakeDialogItemsEx(AdvancedConfigDlgData, AdvancedConfigDlg);
		Dialog Dlg(AdvancedConfigDlg, ARRAYSIZE(AdvancedConfigDlg), EditDlgDlgProc);
		Dlg.SetPosition(-1, -1, DLG_WIDTH, DLG_HEIGHT);
		Dlg.SetHelp(L"FarConfig");
		Dlg.Process();

		if (Dlg.GetExitCode() == 40) {
			switch (_opt.type) {
				case ConfigOpt::T_BOOL:
					if (*_opt.value.b != (bool) AdvancedConfigDlg[27].Selected ) {
						*_opt.value.b = (bool) AdvancedConfigDlg[27].Selected;
						return true;
					}
					return false;
				case ConfigOpt::T_INT:
					{
						int from_edit, base, i;
						wchar_t *endptr;
						if (AdvancedConfigDlg[32].Selected)
							from_edit = 29, base = 10; // decimal
						else
							from_edit = 31, base = 16; // hex
						i = (int) wcstol(AdvancedConfigDlg[from_edit].strData.CPtr(), &endptr, base);
						if (AdvancedConfigDlg[from_edit].strData.CPtr() != endptr && *_opt.value.i != i) {
							*_opt.value.i = i;
							return true;
						}
					}
					return false;
				case ConfigOpt::T_DWORD:
					{
						int from_edit, base;
						DWORD dw;
						wchar_t *endptr;
						if (AdvancedConfigDlg[32].Selected)
							from_edit = 29, base = 10; // decimal
						else
							from_edit = 31, base = 16; // hex
						dw = (DWORD) wcstoul(AdvancedConfigDlg[from_edit].strData.CPtr(), &endptr, base);
						if (AdvancedConfigDlg[from_edit].strData.CPtr() != endptr && *_opt.value.dw != dw) {
							*_opt.value.dw = dw;
							return true;
						}
					}
					return false;
				case ConfigOpt::T_STR:
					if (AdvancedConfigDlg[29].strData != cur_str) {
						*_opt.value.str = AdvancedConfigDlg[29].strData.CPtr();
						return true;
					}
					return false;
				case ConfigOpt::T_BIN: // TODO
					return false;
				default:
					return false;
			}
		}
		return false;
	}
};

static FARString ConfigOptEditTitle(bool hide_unchanged = false)
{
	FARString title = L"far:config";
	if (hide_unchanged) {
		title+= L" *";
	}
	return title;
}

void ConfigOptEdit()
{
	size_t len_sections = 0, len_keys = 0, len_sections_keys = 0;
	bool hide_unchanged = false, align_dot = false;
	int sel_pos = 0;

	VMenu ListConfig(ConfigOptEditTitle(hide_unchanged), nullptr, 0, ScrY-4);
	ListConfig.SetFlags(VMENU_SHOWAMPERSAND | VMENU_IGNORE_SINGLECLICK);
	ListConfig.ClearFlags(VMENU_MOUSEREACTION);
	//ListConfig.SetFlags(VMENU_WRAPMODE);
	ListConfig.SetHelp(L"FarConfig");

	ListConfig.SetBottomTitle(L"F1, Esc or F10, Enter or F4, Del, Ctrl-Alt-F, Ctrl-H, Ctrl-A");

	for (size_t i = ConfigOptCount(); i--;) {
		ConfigOptProps(g_cfg_opts[i])
			.GetMaxLengthSectKeys(len_sections, len_keys, len_sections_keys);
	}
	for (size_t i = 0; i < ConfigOptCount(); ++i) {
		ConfigOptProps(g_cfg_opts[i])
			.MenuListAppend(ListConfig, len_sections, len_keys, len_sections_keys, hide_unchanged, align_dot);
	}

	ListConfig.SetPosition(-1, -1, 0, 0);
	//ListConfig.Process();
	ListConfig.Show();
	do {
		while (!ListConfig.Done()) {
			FarKey Key = ListConfig.ReadInput();
			switch (Key) {
				case KEY_CTRLH:
					hide_unchanged = !hide_unchanged;
					ListConfig.SetTitle(ConfigOptEditTitle(hide_unchanged));
					break;
				case KEY_CTRLA:
					align_dot = !align_dot;
					break;
				case KEY_NUMDEL:
				case KEY_DEL:
					sel_pos = ListConfig.GetSelectPos();
					if (sel_pos >= 0) {
						ConfigOptProps cop(g_cfg_opts[sel_pos]);
						if (cop.IsNotDefault()==1
								&& cop.Msg(ConfigOptEditTitle())==1
								&& cop.ToDefault()) {
							cop.MenuListAppend(
								ListConfig,
								len_sections, len_keys, len_sections_keys,
								hide_unchanged, align_dot,
								sel_pos);
							ListConfig.FastShow();
						}
					}
					continue;
				case KEY_ALTF4:
				case KEY_SHIFTF4:
				case KEY_F4:
					ListConfig.ProcessKey(KEY_ENTER);
					continue;
				default:
					ListConfig.ProcessInput();
					continue;
			}

			// regenerate items in loop only if not was contunue
			sel_pos = ListConfig.GetSelectPos();
			ListConfig.DeleteItems();
			for (size_t i = 0; i < ConfigOptCount(); ++i) {
				ConfigOptProps(g_cfg_opts[i])
					.MenuListAppend(ListConfig, len_sections, len_keys, len_sections_keys, hide_unchanged, align_dot);
			}
			ListConfig.SetSelectPos(sel_pos,0);
			ListConfig.SetPosition(-1, -1, 0, 0);
			ListConfig.Show();
		}

		sel_pos = ListConfig.GetExitCode();
		if (sel_pos < 0) // exit from loop by ESC or F10 or click outside vmenu
			break;
		ListConfig.ClearDone(); // no close after select item by ENTER or dbl mouse click
		ConfigOptProps cop(g_cfg_opts[sel_pos]);
		if (cop.EditDlg(ConfigOptEditTitle()) ) { // by ENTER - show edit dialog
			cop.MenuListAppend( // if was change value then regenerate item
				ListConfig,
				len_sections, len_keys, len_sections_keys,
				hide_unchanged, align_dot,
				sel_pos);
			ListConfig.FastShow();
		}
	} while(1);
}
