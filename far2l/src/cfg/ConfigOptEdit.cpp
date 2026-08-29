/*
ConfigOptEdit.cpp

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
#include "farcolors.hpp"
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
#include "ConfigOptSaveLoad.hpp"
#include "strmix.hpp"

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
				return (_opt.def.str == nullptr ? -1
						: (*_opt.value.str != _opt.def.str));
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
				if (_opt.def.str == nullptr)
					return -1;
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

	bool IsVisibleInList(bool hide_unchanged) const
	{
		return !hide_unchanged || IsNotDefault() != 0;
	}

	const wchar_t *TypeName() const
	{
		switch (_opt.type) {
			case ConfigOpt::T_BOOL:
				return L"bool";
			case ConfigOpt::T_INT:
				return L"int";
			case ConfigOpt::T_DWORD:
				return L"dword";
			case ConfigOpt::T_STR:
				return L"string";
			case ConfigOpt::T_BIN:
				return L"binary";
			default:
				return L"unknown";
		}
	}

	const wchar_t *SaveName() const
	{
		switch (_opt.save) {
			case OST_COMMON:
				return L"common";
			case OST_PANELS:
				return L"panels";
			default:
				return L"never";
		}
	}

	const wchar_t *SaveNameShort() const
	{
		switch (_opt.save) {
			case OST_COMMON:
				return L"c";
			case OST_PANELS:
				return L"p";
			default:
				return L"-";
		}
	}

	void MenuListAppend(VMenu &vm,
					size_t len_sections, size_t len_keys, size_t len_sections_keys,
					bool align_dot,
					size_t option_index) const
	{
		MenuItemEx mi;
		FARString fsn;

		if (align_dot)
		 fsn.Format(L"%*s.%-*s", len_sections, _opt.section, len_keys, _opt.key);
		else {
			mi.strName.Format(L"%s.%s", _opt.section, _opt.key);
			fsn.Format(L"%-*ls", len_sections_keys, mi.strName.CPtr());
		}

		FormatString out1;
		FormatString out2;
		switch (_opt.type)
		{
			case ConfigOpt::T_BOOL: {
				out1 << (*_opt.value.b == _opt.def.b ? L" " : L"*")
					<< L' ' << fsn << L' ' << BoxSymbols[BS_V1] << L"  bool";
				out2 << (*_opt.value.b ? L"[x] true" : L"[ ] false");
				break;
			}
			case ConfigOpt::T_INT: {
				out1 << (*_opt.value.i == _opt.def.i ? L" " : L"*")
					<< L' ' << fsn << L' ' << BoxSymbols[BS_V1] << L"   int";
				out2 << *_opt.value.i << L" = " << fmt::Hex(static_cast<uint32_t>(*_opt.value.i), 0, true);
				break;
			}
			case ConfigOpt::T_DWORD: {
				out1 << (*_opt.value.dw == _opt.def.dw ? L" " : L"*")
					<< L' ' << fsn << L' ' << BoxSymbols[BS_V1] << L" dword";
				out2 << *_opt.value.dw << L" = " << fmt::Hex(static_cast<uint32_t>(*_opt.value.dw), 0, true);
				break;
			}
			case ConfigOpt::T_STR: {
				out1 << (_opt.def.str == nullptr ? L"?"
						: (*_opt.value.str == _opt.def.str ? L" " : L"*"))
					<< L' ' << fsn << L' ' << BoxSymbols[BS_V1] << L"string";
				out2 << _opt.value.str->CPtr();
				break;
			}
			case ConfigOpt::T_BIN: {
				out1 << (_opt.def.bin == nullptr || _opt.value.bin == nullptr ? L"?"
						: (memcmp(_opt.value.bin, _opt.def.bin, _opt.bin_size) == 0 ? L" " : L"*"))
					<< L' ' << fsn << L' ' << BoxSymbols[BS_V1] << L"binary";
				out2 << L"(binary has length " << static_cast<unsigned int>(_opt.bin_size) << L" bytes)";
				break;
			}
			default: {
				out1 << L"? " << fsn << L' ';
				out2 << L"unknown type ???";
			}
		}

		out1 << BoxSymbols[BS_V1] << SaveNameShort() << BoxSymbols[BS_V1];

		mi.strName = out1.strValue();
		mi.strName += out2.strValue();

		mi.strDescription = _opt.section;
		mi.strDescription += L'.';
		mi.strDescription += _opt.key;
		mi.strDescription += L" (type: ";
		mi.strDescription += TypeName();
		mi.strDescription += L", saved: ";
		mi.strDescription += SaveName();
		mi.strDescription += L")\nValue";
		if (_opt.type == ConfigOpt::T_STR)
			mi.strDescription.AppendFormat(L" (length: %zu): \"%ls\"", out2.strValue().GetLength(), out2.strValue().CPtr() );
		else
			mi.strDescription += L": " + out2.strValue();
		if (_opt.description && _opt.description[0]) {
			mi.strDescription += L"\n\n";
			mi.strDescription += _opt.description;
		}
		mi.UserData = reinterpret_cast<char *>(static_cast<DWORD_PTR>(option_index + 1));
		mi.UserDataSize = sizeof(option_index);
		vm.AddItem(&mi);
	}

	int Msg(const wchar_t *title) const
	{
		const char *type_psz;
		FormatString def_str, val_str;
		def_str << L"  Default value: ";
		val_str << L"  Current value: ";
		switch (_opt.type) {
			case ConfigOpt::T_BOOL:
				type_psz = "bool";
				def_str << (_opt.def.b ? L"true" : L"false");
				val_str << ((*_opt.value.b) ? L"true" : L"false");
				break;
			case ConfigOpt::T_INT:
				type_psz = "int";
				def_str << _opt.def.i << L" = " << fmt::Hex(static_cast<uint32_t>(_opt.def.i), 0, true);
				val_str << *_opt.value.i << L" = " << fmt::Hex(static_cast<uint32_t>(*_opt.value.i), 0, true);
				break;
			case ConfigOpt::T_DWORD:
				type_psz = "dword";
				def_str << _opt.def.dw << L" = " << fmt::Hex(static_cast<uint32_t>(_opt.def.dw), 0, true);
				val_str << *_opt.value.dw << L" = " << fmt::Hex(static_cast<uint32_t>(*_opt.value.dw), 0, true);
				break;
			case ConfigOpt::T_STR:
				type_psz = "string";
				def_str << (_opt.def.str ? _opt.def.str : L"(null)");
				val_str << *_opt.value.str;
				break;
			case ConfigOpt::T_BIN:
				type_psz = "binary";
				if (_opt.def.bin) {
					def_str << L"(binary has length" << _opt.bin_size << L"bytes)";
				} else {
					def_str << L"(no default value set)";
				}
				val_str << L"(binary has length" << _opt.bin_size << L"bytes)";
				break;
			default:
				type_psz = "???";
				def_str << L"???";
				val_str << L"???";
		}

		ExMessager em;
		em.AddFormat(L"%ls - %s.%s", title, _opt.section, _opt.key);
		em.AddFormat(L"        Section: %s", _opt.section);
		em.AddFormat(L"            Key: %s", _opt.key);
		em.AddFormat(L"       Saved in: %ls", (_opt.save == OST_COMMON ? L"common" : (_opt.save == OST_PANELS ? L"panels" : L"never")));
		em.AddFormat(L"           Type: %s", type_psz);
		em.AddDup(def_str.strValue());
		em.AddDup(val_str.strValue());
		if (_opt.description && _opt.description[0]) {
			em.AddDup(L"");
			em.AddDupWrap(_opt.description);
		}
		if (IsNotDefault()==1) {
			em.AddDup(L"");
			em.Add(L"Note: Some changes may not take effect immediately.");
			em.Add(L"      Save the configuration and restart FAR2L");
			em.Add(L"      if necessary.");
		}
		em.Add(L"Continue");
		SetMessageHelp(L"FarConfig");
		if (IsNotDefault()==1) {
			em.Add(L"Reset to default");
			return em.Show(MSG_LEFTALIGN, 2);
		}
		return em.Show(MSG_LEFTALIGN, 1);
	}

	enum EditDlgItem
	{
		EDIT_DLG_DEFAULT_LABEL = 10,
		EDIT_DLG_DEFAULT_FALSE = 11,
		EDIT_DLG_DEFAULT_TRUE,
		EDIT_DLG_DEFAULT_DEC_LABEL,
		EDIT_DLG_DEFAULT_DEC_VALUE,
		EDIT_DLG_DEFAULT_HEX_LABEL,
		EDIT_DLG_DEFAULT_HEX_VALUE,
		EDIT_DLG_CURRENT_FALSE = 18,
		EDIT_DLG_CURRENT_TRUE,
		EDIT_DLG_CURRENT_DEC_LABEL,
		EDIT_DLG_CURRENT_DEC_VALUE,
		EDIT_DLG_CURRENT_HEX_LABEL,
		EDIT_DLG_CURRENT_HEX_VALUE,
		EDIT_DLG_NEW_FALSE = 26,
		EDIT_DLG_NEW_TRUE,
		EDIT_DLG_NEW_DEC_LABEL,
		EDIT_DLG_NEW_DEC_VALUE,
		EDIT_DLG_NEW_HEX_LABEL,
		EDIT_DLG_NEW_HEX_VALUE,
		EDIT_DLG_RADIO_DEC,
		EDIT_DLG_RADIO_HEX,
		EDIT_DLG_DESCRIPTION_FIRST = 35,
		EDIT_DLG_CHANGE = 49,
		EDIT_DLG_HELP = 51,
	};

	static LONG_PTR WINAPI EditDlgDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
	{
		switch (Msg) {
			case DN_BTNCLICK:
				if ( Param1 == EDIT_DLG_RADIO_DEC && !SendDlgMessage(hDlg, DM_ENABLE, EDIT_DLG_NEW_DEC_VALUE, -1) ) { // to decimal
					SendDlgMessage(hDlg, DM_ENABLE, EDIT_DLG_NEW_DEC_VALUE, TRUE);
					SendDlgMessage(hDlg, DM_ENABLE, EDIT_DLG_NEW_HEX_VALUE, FALSE);
					SendDlgMessage(hDlg, DM_SETFOCUS, EDIT_DLG_NEW_DEC_VALUE, TRUE);
				}
				else if ( Param1 == EDIT_DLG_RADIO_HEX && !SendDlgMessage(hDlg, DM_ENABLE, EDIT_DLG_NEW_HEX_VALUE, -1) ) { // to hex
					SendDlgMessage(hDlg, DM_ENABLE, EDIT_DLG_NEW_DEC_VALUE, FALSE);
					SendDlgMessage(hDlg, DM_ENABLE, EDIT_DLG_NEW_HEX_VALUE, TRUE);
					SendDlgMessage(hDlg, DM_SETFOCUS, EDIT_DLG_NEW_HEX_VALUE, TRUE);
				}
				else if (Param1 == EDIT_DLG_HELP) {
					const wchar_t *help_topic = reinterpret_cast<const wchar_t *>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
					if (help_topic && help_topic[0])
						Help::Present(help_topic);
					return TRUE;
				}
				break;
			default:
				break;
		}

		return DefDlgProc(hDlg, Msg, Param1, Param2);
	}

	bool EditDlg(const wchar_t *title) const
	{
		bool is_editable = false, is_def = true;
		const wchar_t *type_pwsz;
		FormatString def_str, cur_str, new_str, def_str_hex, cur_str_hex, new_str_hex;
		FARString fs_title,
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
				def_str << _opt.def.i;
				cur_str << *_opt.value.i;
				def_str_hex << fmt::Hex(static_cast<uint32_t>(_opt.def.i));
				cur_str_hex << fmt::Hex(static_cast<uint32_t>(*_opt.value.i));
				new_str << cur_str;
				new_str_hex << cur_str_hex;
				break;
			case ConfigOpt::T_DWORD:
				type_pwsz = L"dword";
				is_editable = true;
				def_str << _opt.def.dw;
				cur_str << *_opt.value.dw;
				def_str_hex << fmt::Hex(static_cast<uint32_t>(_opt.def.dw));
				cur_str_hex << fmt::Hex(static_cast<uint32_t>(*_opt.value.dw));
				new_str << cur_str;
				new_str_hex << cur_str_hex;
				break;
			case ConfigOpt::T_STR:
				type_pwsz = L"string";
				is_editable = true;
				is_def = (bool) _opt.def.str;
				def_str << ( _opt.def.str ? _opt.def.str : L"" );
				cur_str << *_opt.value.str;
				new_str << cur_str;
				break;
			case ConfigOpt::T_BIN:
				type_pwsz = L"binary";
				if (_opt.def.bin) {
					def_str << L"(binary has length " << _opt.bin_size << L" bytes)";
				} else {
					is_def = false;
					def_str << L"(no default value set)";
				}
				cur_str << L"(binary has length " << _opt.bin_size << L" bytes)";
				new_str << L"(can not process binary)";
				break;
			default:
				type_pwsz = L"???";
				def_str << L"???";
				cur_str << L"???";
				new_str	<< L"(can not process unknown type)";
		}

		const wchar_t *mask_int = L"#9999999999";
		const wchar_t *mask_dword = L"9999999999";
		const wchar_t *HexMask = L"HHHHHHHH";
		const short DLG_WIDTH = 76;
		const int DESCRIPTION_WIDTH = DLG_WIDTH - 12;
		const size_t MIN_DESCRIPTION_LINES = 1;
		const size_t MAX_DESCRIPTION_LINES = 8;
		const size_t max_description_lines =
			std::min(MAX_DESCRIPTION_LINES, static_cast<size_t>(std::max(1, ScrY - 24)));
		FARString description_lines[MAX_DESCRIPTION_LINES];
		const wchar_t *description = _opt.description;
		if (!description || !*description)
			description = L"(no description)";
		const size_t wrapped_description_lines_count =
			WrapTextToLines(description, DESCRIPTION_WIDTH, description_lines, ARRAYSIZE(description_lines));
		const size_t description_lines_count = std::min(
			std::max(MIN_DESCRIPTION_LINES, wrapped_description_lines_count),
			max_description_lines);
		const short desc_separator_y = 12 + static_cast<short>(description_lines_count);
		const short note_y = desc_separator_y + 1;
		const short note_y1 = note_y + 1;
		const short note_y2 = note_y + 2;
		const short note_y3 = note_y + 3;
		const short bottom_separator_y = note_y + 4;
		const short button_y = bottom_separator_y + 1;
		const short DLG_HEIGHT = button_y + 3;
		const short DLG_X2 = DLG_WIDTH - 4;
		const short DLG_Y2 = DLG_HEIGHT - 2;
		const short TEXT_X2 = DLG_WIDTH - 6;
		DialogDataEx AdvancedConfigDlgData[] = {
			/*   0 */ {DI_DOUBLEBOX,	 3,  1, DLG_X2, DLG_Y2, {}, 0, fs_title.CPtr()},
			/*   1 */ {DI_TEXT,			 5,  2, 20,             2, {}, 0, L"       Section:"},
			/*   2 */ {DI_TEXT,			21,  2, TEXT_X2,  2, {}, 0, fs_section.CPtr()},
			/*   3 */ {DI_TEXT,			 5,  3, 20,             3, {}, 0, L"           Key:"},
			/*   4 */ {DI_TEXT,			21,  3, TEXT_X2,  3, {}, 0, fs_key.CPtr()},
			/*   5 */ {DI_TEXT,			 5,  4, 20,             4, {}, 0, L"      Saved in:"},
			/*   6 */ {DI_TEXT,			21,  4, TEXT_X2,  4, {}, 0, SaveName()},
			/*   7 */ {DI_TEXT,			 5,  5, 20,             5, {}, 0, L"          Type:"},
			/*   8 */ {DI_TEXT,			21,  5, TEXT_X2,  5, {}, 0, type_pwsz},
			/*   9 */ {DI_TEXT,			 3,  6, 20,             6, {}, DIF_SEPARATOR, L" Values "},
			/*  10 */ {DI_TEXT,			 5,  7, 13,             7, {}, 0, L"Default:"},
			/*  11 */ {DI_RADIOBUTTON,	14,  7,  0,             7, {}, DIF_DISABLE | DIF_GROUP, L"false"},
			/*  12 */ {DI_RADIOBUTTON,	29,  7,  0,             7, {}, DIF_DISABLE, L"true"},
			/*  13 */ {DI_TEXT,			14,  7, 21,             7, {}, 0, L"Decimal="},
			/*  14 */ {DI_EDIT,			22,  7, 32,             7, {}, DIF_READONLY | DIF_SELECTONENTRY, def_str.strValue()},
			/*  15 */ {DI_TEXT,			35,  7, 40,             7, {}, 0, L"Hex=0x"},
			/*  16 */ {DI_EDIT,			41,  7, 49,             7, {}, DIF_READONLY | DIF_SELECTONENTRY, def_str_hex.strValue()},
			/*  17 */ {DI_TEXT,			 5,  8, 13,             8, {}, 0, L"Current:"},
			/*  18 */ {DI_RADIOBUTTON,	14,  8,  0,             8, {}, DIF_DISABLE | DIF_GROUP, L"false"},
			/*  19 */ {DI_RADIOBUTTON,	29,  8,  0,             8, {}, DIF_DISABLE, L"true"},
			/*  20 */ {DI_TEXT,			14,  8, 21,             8, {}, 0, L"Decimal="},
			/*  21 */ {DI_EDIT,			22,  8, 32,             8, {}, DIF_READONLY | DIF_SELECTONENTRY, cur_str.strValue()},
			/*  22 */ {DI_TEXT,			35,  8, 40,             8, {}, 0, L"Hex=0x"},
			/*  23 */ {DI_EDIT,			41,  8, 49,             8, {}, DIF_READONLY | DIF_SELECTONENTRY, cur_str_hex.strValue()},
			/*  24 */ {DI_TEXT,			 3,  9, 20,             9, {}, DIF_SEPARATOR, L" New value "},
			/*  25 */ {DI_TEXT,			 5, 10, 13,            10, {}, (is_editable ? 0 : DIF_DISABLE), L"    New:"},
			/*  26 */ {DI_RADIOBUTTON,	14, 10, 14,            10, {}, (is_editable ? DIF_FOCUS : DIF_DISABLE) | DIF_GROUP, L"&false"},
			/*  27 */ {DI_RADIOBUTTON,	29, 10, 14,            10, {}, (is_editable ? 0 : DIF_DISABLE), L"t&rue"},
			/*  28 */ {DI_TEXT,			14, 10, 21,            10, {}, 0, L"Decimal="},
			/*  29 */ {DI_EDIT,			22, 10, 32,            10, {}, (is_editable ? DIF_FOCUS : DIF_DISABLE) | DIF_SELECTONENTRY, new_str.strValue()},
			/*  30 */ {DI_TEXT,			35, 10, 40,            10, {}, 0, L"Hex=0x"},
			/*  31 */ {DI_FIXEDIT,		41, 10, 49,            10, {(DWORD_PTR)HexMask}, DIF_MASKEDIT | DIF_DISABLE | DIF_SELECTONENTRY, new_str_hex.strValue()},
			/*  32 */ {DI_RADIOBUTTON,	51, 10, 58,            10, {1}, (is_editable ? 0 : DIF_DISABLE) | DIF_GROUP, L"&dec"},
			/*  33 */ {DI_RADIOBUTTON,	59, 10, 65,            10, {}, (is_editable ? 0 : DIF_DISABLE), L"&hex"},
			/*  34 */ {DI_TEXT,		3, 11, 20,            11, {}, DIF_SEPARATOR, L" Description "},
			/*  35 */ {DI_TEXT,		5, 12, TEXT_X2, 12, {}, DIF_SHOWAMPERSAND, description_lines[0].CPtr()},
			/*  36 */ {DI_TEXT,		5, 13, TEXT_X2, 13, {}, DIF_SHOWAMPERSAND, description_lines[1].CPtr()},
			/*  37 */ {DI_TEXT,		5, 14, TEXT_X2, 14, {}, DIF_SHOWAMPERSAND, description_lines[2].CPtr()},
			/*  38 */ {DI_TEXT,		5, 15, TEXT_X2, 15, {}, DIF_SHOWAMPERSAND, description_lines[3].CPtr()},
			/*  39 */ {DI_TEXT,		5, 16, TEXT_X2, 16, {}, DIF_SHOWAMPERSAND, description_lines[4].CPtr()},
			/*  40 */ {DI_TEXT,		5, 17, TEXT_X2, 17, {}, DIF_SHOWAMPERSAND, description_lines[5].CPtr()},
			/*  41 */ {DI_TEXT,		5, 18, TEXT_X2, 18, {}, DIF_SHOWAMPERSAND, description_lines[6].CPtr()},
			/*  42 */ {DI_TEXT,		5, 19, TEXT_X2, 19, {}, DIF_SHOWAMPERSAND, description_lines[7].CPtr()},
			/*  43 */ {DI_TEXT,		3, desc_separator_y, 20, desc_separator_y, {}, DIF_SEPARATOR, L""},
			/*  44 */ {DI_TEXT,		5, note_y, TEXT_X2, note_y, {}, DIF_SHOWAMPERSAND, L"Note: Some changes may not take effect immediately."},
			/*  45 */ {DI_TEXT,		5, note_y1, TEXT_X2, note_y1, {}, DIF_SHOWAMPERSAND, L"      Save the configuration and restart FAR2L"},
			/*  46 */ {DI_TEXT,		5, note_y2, TEXT_X2, note_y2, {}, DIF_SHOWAMPERSAND, L"      if necessary."},
			/*  47 */ {DI_TEXT,		5, note_y3, TEXT_X2, note_y3, {}, DIF_SHOWAMPERSAND, L""},
			/*  48 */ {DI_TEXT,		3, bottom_separator_y, 20, bottom_separator_y, {}, DIF_SEPARATOR, L""},
			/*  49 */ {DI_BUTTON,	0, button_y, 0,  button_y, {}, DIF_DEFAULT | DIF_CENTERGROUP | (is_editable ? 0 : DIF_DISABLE), Msg::Change},
			/*  50 */ {DI_BUTTON,	0, button_y, 0,  button_y, {}, DIF_CENTERGROUP | (is_editable ? 0 : DIF_FOCUS), Msg::Cancel},
			/*  51 */ {DI_BUTTON,	0, button_y, 0,  button_y, {}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, L"Closest Help &Topic"}
		};
		if (!is_def) {
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_LABEL].Flags |= DIF_DISABLE;
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_FALSE].Flags |= DIF_DISABLE;
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_TRUE].Flags |= DIF_DISABLE;
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_DEC_LABEL].Flags |= DIF_DISABLE;
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_DEC_VALUE].Flags |= DIF_DISABLE;
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_HEX_LABEL].Flags |= DIF_DISABLE;
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_HEX_VALUE].Flags |= DIF_DISABLE;
		}
		if (_opt.type == ConfigOpt::T_BOOL) {
			AdvancedConfigDlgData[_opt.def.b ? EDIT_DLG_DEFAULT_TRUE : EDIT_DLG_DEFAULT_FALSE].Selected = 1;
			AdvancedConfigDlgData[(*_opt.value.b) ? EDIT_DLG_CURRENT_TRUE : EDIT_DLG_CURRENT_FALSE].Selected = 1;
			AdvancedConfigDlgData[(*_opt.value.b) ? EDIT_DLG_NEW_TRUE : EDIT_DLG_NEW_FALSE].Selected = 1;
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_DEC_LABEL].Flags =
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_DEC_VALUE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_HEX_LABEL].Flags =
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_HEX_VALUE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_CURRENT_DEC_LABEL].Flags =
			AdvancedConfigDlgData[EDIT_DLG_CURRENT_DEC_VALUE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_CURRENT_HEX_LABEL].Flags =
			AdvancedConfigDlgData[EDIT_DLG_CURRENT_HEX_VALUE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_LABEL].Flags =
			AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_NEW_HEX_LABEL].Flags =
			AdvancedConfigDlgData[EDIT_DLG_NEW_HEX_VALUE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_RADIO_DEC].Flags =
			AdvancedConfigDlgData[EDIT_DLG_RADIO_HEX].Flags = DIF_HIDDEN;
		}
		else {
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_FALSE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_DEFAULT_TRUE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_CURRENT_FALSE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_CURRENT_TRUE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_NEW_FALSE].Flags =
			AdvancedConfigDlgData[EDIT_DLG_NEW_TRUE].Flags = DIF_HIDDEN;
			if (_opt.type==ConfigOpt::T_DWORD) {
				AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].Type = DI_FIXEDIT;
				AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].Flags |= DIF_MASKEDIT;
				AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].Mask = mask_dword;
			}
			else if (_opt.type==ConfigOpt::T_INT) {
				AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].Type = DI_FIXEDIT;
				AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].Flags |= DIF_MASKEDIT;
				AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].Mask = mask_int;
			}
			else { // T_STR & T_BIN
				AdvancedConfigDlgData[EDIT_DLG_DEFAULT_DEC_VALUE].X1 = AdvancedConfigDlgData[EDIT_DLG_CURRENT_DEC_VALUE].X1 = AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].X1 = 14;
				AdvancedConfigDlgData[EDIT_DLG_DEFAULT_DEC_VALUE].X2 = AdvancedConfigDlgData[EDIT_DLG_CURRENT_DEC_VALUE].X2 = AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_VALUE].X2 = TEXT_X2;
				AdvancedConfigDlgData[EDIT_DLG_DEFAULT_DEC_LABEL].Flags =
				AdvancedConfigDlgData[EDIT_DLG_DEFAULT_HEX_LABEL].Flags =
				AdvancedConfigDlgData[EDIT_DLG_DEFAULT_HEX_VALUE].Flags =
				AdvancedConfigDlgData[EDIT_DLG_CURRENT_DEC_LABEL].Flags =
				AdvancedConfigDlgData[EDIT_DLG_CURRENT_HEX_LABEL].Flags =
				AdvancedConfigDlgData[EDIT_DLG_CURRENT_HEX_VALUE].Flags =
				AdvancedConfigDlgData[EDIT_DLG_NEW_DEC_LABEL].Flags =
				AdvancedConfigDlgData[EDIT_DLG_NEW_HEX_LABEL].Flags =
				AdvancedConfigDlgData[EDIT_DLG_NEW_HEX_VALUE].Flags =
				AdvancedConfigDlgData[EDIT_DLG_RADIO_DEC].Flags =
				AdvancedConfigDlgData[EDIT_DLG_RADIO_HEX].Flags = DIF_HIDDEN;
			}
		}
		for (size_t i = description_lines_count; i < MAX_DESCRIPTION_LINES; ++i) {
			AdvancedConfigDlgData[EDIT_DLG_DESCRIPTION_FIRST + i].Flags|= DIF_HIDDEN;
		}
		if (!_opt.help_topic || !_opt.help_topic[0])
			AdvancedConfigDlgData[EDIT_DLG_HELP].Flags |= DIF_DISABLE;
		MakeDialogItemsEx(AdvancedConfigDlgData, AdvancedConfigDlg);
		Dialog Dlg(AdvancedConfigDlg, ARRAYSIZE(AdvancedConfigDlg), EditDlgDlgProc, (LONG_PTR)_opt.help_topic);
		Dlg.SetPosition(-1, -1, DLG_WIDTH, DLG_HEIGHT);
		Dlg.SetHelp(L"FarConfig");
		Dlg.Process();

		if (Dlg.GetExitCode() == EDIT_DLG_CHANGE) {
			switch (_opt.type) {
				case ConfigOpt::T_BOOL:
					if (*_opt.value.b != (bool) AdvancedConfigDlg[EDIT_DLG_NEW_TRUE].Selected ) {
						*_opt.value.b = (bool) AdvancedConfigDlg[EDIT_DLG_NEW_TRUE].Selected;
						return true;
					}
					return false;
				case ConfigOpt::T_INT:
					{
						int from_edit, base, i;
						wchar_t *endptr;
						if (AdvancedConfigDlg[EDIT_DLG_RADIO_DEC].Selected)
							from_edit = EDIT_DLG_NEW_DEC_VALUE, base = 10; // decimal
						else
							from_edit = EDIT_DLG_NEW_HEX_VALUE, base = 16; // hex
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
						if (AdvancedConfigDlg[EDIT_DLG_RADIO_DEC].Selected)
							from_edit = EDIT_DLG_NEW_DEC_VALUE, base = 10; // decimal
						else
							from_edit = EDIT_DLG_NEW_HEX_VALUE, base = 16; // hex
						dw = (DWORD) wcstoul(AdvancedConfigDlg[from_edit].strData.CPtr(), &endptr, base);
						if (AdvancedConfigDlg[from_edit].strData.CPtr() != endptr && *_opt.value.dw != dw) {
							*_opt.value.dw = dw;
							return true;
						}
					}
					return false;
				case ConfigOpt::T_STR:
					if (AdvancedConfigDlg[EDIT_DLG_NEW_DEC_VALUE].strData != cur_str) {
						*_opt.value.str = AdvancedConfigDlg[EDIT_DLG_NEW_DEC_VALUE].strData.CPtr();
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
	FARString title (Msg::MenuFarConfig);
	title+= L" - far:config";
	if (hide_unchanged) {
		title+= L" *";
	}
	RemoveChar(title, L'&');
	return title;
}

static void ConfigOptAppendHeader(VMenu &vm, size_t len_sections_keys)
{
	MenuItemEx mi;
	FormatString header;
	header << L"  " << fmt::Cells() << fmt::LeftAlign() << fmt::Size(len_sections_keys)
		<< L"Section.Parameter" << L' ' << BoxSymbols[BS_V1]
		<< L"  Type" << BoxSymbols[BS_V1]
		<< L" " << BoxSymbols[BS_V1]
		<< L"Value";
	mi.strName = header.strValue();
	mi.Flags = LIF_DISABLE;
	vm.AddItem(&mi);
}

static void ConfigOptPopulateMenu(VMenu &vm,
		size_t len_sections, size_t len_keys, size_t len_sections_keys,
		bool hide_unchanged, bool align_dot)
{
	vm.DeleteItems();
	ConfigOptAppendHeader(vm, len_sections_keys);

	for (size_t i = 0; i < ConfigOptCount(); ++i) {
		ConfigOptProps cop(g_cfg_opts[i]);
		if (!cop.IsVisibleInList(hide_unchanged))
			continue;

		cop.MenuListAppend(vm, len_sections, len_keys, len_sections_keys, align_dot, i);
	}
}

static size_t ConfigOptIndexFromMenu(VMenu &vm, int position = -1)
{
	const DWORD_PTR token = reinterpret_cast<DWORD_PTR>(vm.GetUserData(nullptr, 0, position));
	return token ? static_cast<size_t>(token - 1) : static_cast<size_t>(-1);
}

static bool ConfigOptSelectByIndex(VMenu &vm, size_t option_index)
{
	for (int i = 0; i < vm.GetItemCount(); ++i) {
		const size_t item_index = ConfigOptIndexFromMenu(vm, i);
		if (item_index != static_cast<size_t>(-1) && item_index == option_index) {
			vm.SetSelectPos(i, 0);
			return true;
		}
	}

	for (int i = 0; i < vm.GetItemCount(); ++i) {
		if (ConfigOptIndexFromMenu(vm, i) != static_cast<size_t>(-1)) {
			vm.SetSelectPos(i, 0);
			return true;
		}
	}
	return false;
}

void ConfigOptEdit()
{
	size_t len_sections = 0, len_keys = 0, len_sections_keys = 0;
	bool hide_unchanged = false, align_dot = false;
	int sel_pos = 0;

	VMenu ListConfig(ConfigOptEditTitle(hide_unchanged), nullptr, 0, ScrY-4);
	ListConfig.SetBottomTextLines(2);
	ListConfig.SetFlags(VMENU_SHOWAMPERSAND | VMENU_IGNORE_SINGLECLICK);
	ListConfig.ClearFlags(VMENU_MOUSEREACTION);
	//ListConfig.SetFlags(VMENU_WRAPMODE);
	ListConfig.SetHelp(L"FarConfig");

	ListConfig.SetBottomTitle(L"[Ctrl-Alt-F] Filter  [Enter/F4] Edit  [Del] Reset  [Ctrl-H] Changed  [Ctrl-A] Align  [Esc/F10] Exit");

	for (size_t i = ConfigOptCount(); i--;) {
		ConfigOptProps(g_cfg_opts[i])
			.GetMaxLengthSectKeys(len_sections, len_keys, len_sections_keys);
	}
	ConfigOptPopulateMenu(ListConfig, len_sections, len_keys, len_sections_keys, hide_unchanged, align_dot);

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
						const size_t option_index = ConfigOptIndexFromMenu(ListConfig, sel_pos);
						if (option_index == static_cast<size_t>(-1))
							continue;

						ConfigOptProps cop(g_cfg_opts[option_index]);
						if (cop.IsNotDefault()==1
								&& cop.Msg(ConfigOptEditTitle())==1
								&& cop.ToDefault()) {
							ConfigOptPopulateMenu(ListConfig, len_sections, len_keys, len_sections_keys, hide_unchanged, align_dot);
							ConfigOptSelectByIndex(ListConfig, option_index);
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
			const size_t option_index = ConfigOptIndexFromMenu(ListConfig);
			ConfigOptPopulateMenu(ListConfig, len_sections, len_keys, len_sections_keys, hide_unchanged, align_dot);
			ConfigOptSelectByIndex(ListConfig, option_index);
			ListConfig.SetPosition(-1, -1, 0, 0);
			ListConfig.Show();
		}

		sel_pos = ListConfig.GetExitCode();
		if (sel_pos < 0) // exit from loop by ESC or F10 or click outside vmenu
			break;
		ListConfig.ClearDone(); // no close after select item by ENTER or dbl mouse click
		const size_t option_index = ConfigOptIndexFromMenu(ListConfig, sel_pos);
		if (option_index == static_cast<size_t>(-1)) {
			ListConfig.FastShow();
			continue;
		}

		ConfigOptProps cop(g_cfg_opts[option_index]);
		if (cop.EditDlg(ConfigOptEditTitle()) ) { // by ENTER - show edit dialog
			ConfigOptPopulateMenu(ListConfig, len_sections, len_keys, len_sections_keys, hide_unchanged, align_dot);
			ConfigOptSelectByIndex(ListConfig, option_index);
			ListConfig.FastShow();
		}
	} while(1);
}
