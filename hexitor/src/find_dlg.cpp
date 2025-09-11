/**************************************************************************
 *  Hexitor plug-in for FAR 3.0 modifed by m32 2024 for far2l             *
 *  Copyright (C) 2010-2013 by Artem Senichev <artemsen@gmail.com>        *
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

#include "find_dlg.h"
#include "string_rc.h"

static constexpr size_t MAX_SEQ_SIZE = 128;
//
static constexpr intptr_t //Dialog item ids
DLGID_HEX_EDIT = 2, DLGID_ANS_EDIT = 4, DLGID_OEM_EDIT = 6, DLGID_UTF_EDIT = 8, DLGID_BTN_CANCEL = 13;
//
static const wchar_t HHist[]{ L"HexitorFindHex"  };
static const wchar_t aHist[]{ L"HexitorFindAnsi" };
static const wchar_t oHist[]{ L"HexitorFindOem"  };
static const wchar_t uHist[]{ L"HexitorFindUtf"  };
//
bool find_dlg::show(vector<unsigned char>& seq, bool& forward_search)
{
	wstring mask_edit(MAX_SEQ_SIZE * 3, L'H' );
	for (size_t i = 0; i < MAX_SEQ_SIZE; ++i) mask_edit[3*i + 2] = L' '; // 128 x "HH "

	_seq.reserve(MAX_SEQ_SIZE);
	_seq = seq;
	const auto f = forward_search ? 0 : 1;

	#define LIF_NONE 0
	#define FDLG_NONE 0

	FarDialogItem dlg_items[] = {
	/*  0 */ { DI_DOUBLEBOX, 3,  1, 56, 13, 0, (DWORD_PTR)nullptr, LIF_NONE, 0, _PSI.GetMsg(_PSI.ModuleNumber, ps_find_title) },
	/*  1 */ { DI_TEXT,      5,  2, 11,  2, 0, (DWORD_PTR)nullptr, LIF_NONE, 0, L"&Hex:" },
	/*  2 */ { DI_FIXEDIT,  12,  2, 54,  2, 0, (DWORD_PTR)mask_edit.c_str(), 0, DIF_MASKEDIT | DIF_HISTORY },
	/*  3 */ { DI_TEXT,      5,  4, 11,  4, 0, (DWORD_PTR)nullptr, LIF_NONE, 0, L"&ANSI:" },
	/*  4 */ { DI_EDIT,     12,  4, 54,  4, 0, (DWORD_PTR)nullptr, LIF_NONE | DIF_HISTORY},
	/*  5 */ { DI_TEXT,      5,  6, 11,  6, 0, (DWORD_PTR)nullptr, LIF_NONE, 0, L"&OEM:" },
	/*  6 */ { DI_EDIT,     12,  6, 54,  6, 0, (DWORD_PTR)nullptr, LIF_NONE | DIF_HISTORY},
	/*  7 */ { DI_TEXT,      5,  8, 11,  8, 0, (DWORD_PTR)nullptr, LIF_NONE, 0, L"&UTF16:" },
	/*  8 */ { DI_EDIT,     12,  8, 54,  8, 0, (DWORD_PTR)nullptr, LIF_NONE | DIF_HISTORY},
	/*  9 */ { DI_TEXT,      0,  9,  0,  9, 0, (DWORD_PTR)nullptr, DIF_SEPARATOR },
	/* 10 */ { DI_CHECKBOX, 12, 10, 54, 10, f, (DWORD_PTR)nullptr, LIF_NONE, 0, _PSI.GetMsg(_PSI.ModuleNumber, ps_find_backward) },
	/* 11 */ { DI_TEXT,      0, 11,  0, 11, 0, (DWORD_PTR)nullptr, DIF_SEPARATOR },
	/* 12 */ { DI_BUTTON,    0, 12,  0, 12, 0, (DWORD_PTR)nullptr, DIF_CENTERGROUP | DIF_DEFAULT, 0, _PSI.GetMsg(_PSI.ModuleNumber, ps_ok) },
	/* 13 */ { DI_BUTTON,    0, 12,  0, 12, 0, (DWORD_PTR)nullptr, DIF_CENTERGROUP, 0, _PSI.GetMsg(_PSI.ModuleNumber, ps_cancel) }
	};

	_dialog = _PSI.DialogInit(
		_PSI.ModuleNumber,
		-1, -1, 60, 15,
		nullptr,
		dlg_items,
		_countof(dlg_items),
		0,
		FDLG_NONE,
		(FARWINDOWPROC)&find_dlg::dlg_proc,
		(LONG_PTR)this
	);
	const intptr_t rc = _PSI.DialogRun(_dialog);
	if (rc >= 0 && rc != DLGID_BTN_CANCEL) {
		_seq.swap(seq);
		forward_search = !(_PSI.SendDlgMessage(_dialog, DM_GETCHECK, 10, (LONG_PTR)nullptr) == BSTATE_CHECKED);
		_PSI.DialogFree(_dialog);
		return true;
	}

	_PSI.DialogFree(_dialog);
	return false;
}

void find_dlg::fill_hex()
{
	wchar_t txt[MAX_SEQ_SIZE*3 + 1]={};
	const size_t len = std::min(_seq.size(), MAX_SEQ_SIZE);
	for (size_t i = 0; i < len; ++i) swprintf_ws2ls(&txt[i*3], sizeof(txt), L"%02X ", (unsigned)_seq[i]);
	FarDialogItemData item_data{ len*3, txt };
	_PSI.SendDlgMessage(_dialog, DM_SETTEXT, DLGID_HEX_EDIT, (LONG_PTR)&item_data);
}


void find_dlg::fill_mb(const UINT cp, const uintptr_t item_id) noexcept
{
	wchar_t txt[MAX_SEQ_SIZE * 2]={};
	size_t ncw = 0;
	const auto len = _seq.size();
	if (len) {
		const auto cw = MultiByteToWideChar(cp, 0, (LPCSTR)&_seq.front(), (int)len, &txt[0], _countof(txt));
		if (cw > 0) ncw = cw;
	}
	FarDialogItemData item_data{ ncw, txt };
	_PSI.SendDlgMessage(_dialog, DM_SETTEXT, item_id, (LONG_PTR)&item_data);
}
//
void find_dlg::fill_ans() noexcept { fill_mb(CP_ACP,   DLGID_ANS_EDIT); }
void find_dlg::fill_oem() noexcept { fill_mb(CP_OEMCP, DLGID_OEM_EDIT); }


void find_dlg::fill_u16()
{
	FarDialogItemData item_data{ sizeof(item_data) };
	bool padded{ false };
	const size_t len = _seq.size();
	if (len) {
		if (len & 1) {
			_seq.push_back(0x00); padded = true;
		}
		item_data.PtrLength = (len + 1) / 2;
		item_data.PtrData = (wchar_t*)&_seq.front();
	}
	_PSI.SendDlgMessage(_dialog, DM_SETTEXT, DLGID_UTF_EDIT, (LONG_PTR)&item_data);
	if (padded)
		_seq.pop_back();
}


intptr_t WINAPI find_dlg::dlg_proc(HANDLE dlg, intptr_t msg, intptr_t param1, void* param2)
{
	find_dlg* instance = nullptr;
	if (msg != DN_INITDIALOG)
		instance = reinterpret_cast<find_dlg*>(_PSI.SendDlgMessage(dlg, DM_GETDLGDATA, 0, (LONG_PTR)nullptr));
	else {
		instance = reinterpret_cast<find_dlg*>(param2);
		_PSI.SendDlgMessage(dlg, DM_SETDLGDATA, 0, (LONG_PTR)instance);
	}
	assert(instance);

	if (msg == DN_INITDIALOG) {
		instance->_can_update = true;
		instance->fill_hex();
		instance->fill_ans();
		instance->fill_oem();
		instance->fill_u16();
		_PSI.SendDlgMessage(dlg, DM_SETMAXTEXTLENGTH, DLGID_HEX_EDIT, (LONG_PTR)(MAX_SEQ_SIZE * 3));
		_PSI.SendDlgMessage(dlg, DM_SETMAXTEXTLENGTH, DLGID_ANS_EDIT, (LONG_PTR)MAX_SEQ_SIZE);
		_PSI.SendDlgMessage(dlg, DM_SETMAXTEXTLENGTH, DLGID_OEM_EDIT, (LONG_PTR)MAX_SEQ_SIZE);
		_PSI.SendDlgMessage(dlg, DM_SETMAXTEXTLENGTH, DLGID_UTF_EDIT, (LONG_PTR)(MAX_SEQ_SIZE / 2));
		return 1;
	}
	else if (msg == DN_CLOSE && param1 >= 0 && param1 != DLGID_BTN_CANCEL && instance->_seq.empty()) {
		const wchar_t* err_msg[] = { _PSI.GetMsg(_PSI.ModuleNumber, ps_find_title), _PSI.GetMsg(_PSI.ModuleNumber, ps_find_empty) };
		_PSI.Message(_PSI.ModuleNumber, FMSG_MB_OK | FMSG_WARNING, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
		return 0;
	}
	else if (msg == DN_EDITCHANGE) {
		if (!instance->_can_update)
			return 1;
		instance->_can_update = false;

		const auto val = (const wchar_t*)_PSI.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, param1, (LONG_PTR)nullptr);
		const auto val_len = wcslen(val);

		if (param1 == DLGID_HEX_EDIT) {
			instance->_seq.clear();
			size_t pos = 0;
			while (pos < val_len) {
				if (val[pos] <= L' ')
					break;
				const auto v = wcstoul(val + pos, nullptr, 16);
				instance->_seq.push_back(static_cast<unsigned char>(v));
				pos += 3;
			}
			instance->fill_ans();
			instance->fill_oem();
			instance->fill_u16();
		}
		else if (param1 == DLGID_ANS_EDIT) {
			instance->_seq.resize(val_len);
			if (val_len)
				WideCharToMultiByte(CP_ACP, 0, val, (int)val_len, (LPSTR)&instance->_seq.front(), (int)instance->_seq.size(), nullptr, nullptr);
			instance->fill_hex();
			instance->fill_oem();
			instance->fill_u16();
		}
		else if (param1 == DLGID_OEM_EDIT) {
			instance->_seq.resize(val_len);
			if (val_len)
				WideCharToMultiByte(CP_OEMCP, 0, val, (int)val_len, (LPSTR)&instance->_seq.front(), (int)instance->_seq.size(), nullptr, nullptr);
			instance->fill_hex();
			instance->fill_ans();
			instance->fill_u16();
		}
		else if (param1 == DLGID_UTF_EDIT) {
			instance->_seq.resize(val_len * 2);
			if (val_len)
				memcpy(&instance->_seq.front(), val, val_len * 2);
			instance->fill_hex();
			instance->fill_ans();
			instance->fill_oem();
		}
		instance->_can_update = true;

		return 1;
	}

	return _PSI.DefDlgProc(dlg, msg, param1, (LONG_PTR)param2);
}
