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

#include "goto_dlg.h"
#include "string_rc.h"

static const wchar_t* _hex_mask = L"0xHHHHHHHHHHHH";
static const wchar_t* _percent_mask = L"99";

static const wchar_t AHist[]{ L"HexitorGotoAddr" };

LONG_PTR WINAPI goto_dlg::dlg_proc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	if (msg == DN_BTNCLICK && (param1 == dlgptr->getID("rb_hex") || param1 == dlgptr->getID("rb_percent")) && reinterpret_cast<LONG_PTR>(param2) == BSTATE_CHECKED) {
		const size_t sz = _PSI.SendDlgMessage(dlg, DM_GETDLGITEM, dlgptr->getID("tb_mask"), 0);
		if (sz) {
			vector<unsigned char> buffer(sz);
			FarDialogItem gdi;
			ZeroMemory(&gdi, sizeof(gdi));
			gdi.PtrData = reinterpret_cast<const wchar_t*>(&buffer.front());
			if (_PSI.SendDlgMessage(dlg, DM_GETDLGITEM, dlgptr->getID("tb_mask"), (LONG_PTR)&gdi)) {
				FarDialogItem* di = reinterpret_cast<FarDialogItem*>(&buffer.front());
				di->Param.Mask = (param1 == dlgptr->getID("rb_percent") ? _hex_mask : _percent_mask);
				_PSI.SendDlgMessage(dlg, DM_SETDLGITEM, dlgptr->getID("tb_mask"), (LONG_PTR)di);
			}
		}
	}
	else if (msg == DN_CLOSE && param1 >= 0 && param1 != dlgptr->getID("bn_cancel")) {
		//Check parameters
		if ( get_val() >= _file_size) {
			wchar_t scv[80];
			swprintf(scv, sizeof(scv), _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_spec), _file_size);
			const wchar_t* err_msg[] = { _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_title), _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_out_ofr), scv };
			_PSI.Message(_PSI.ModuleNumber, FMSG_MB_OK | FMSG_WARNING, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
			return 0;
		}
	}
	return _PSI.DefDlgProc(dlg, msg, param1, param2);
}

bool goto_dlg::show(const UINT64 file_size, UINT64& offset)
{
	_file_size = file_size;

	fardialog::DlgMASKED maskoff("tb_mask", nullptr, _hex_mask, DIF_HISTORY | DIF_MASKEDIT);
	// TODO manage history in aHist
	fardialog::DlgRADIOBUTTON radiohex("rb_hex", _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_hex), DIF_GROUP, true);
	fardialog::DlgRADIOBUTTON radioper("rb_percent", _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_percent), 0, false);
	fardialog::DlgHLine hline0;
	fardialog::DlgBUTTON button1("bn_ok", _PSI.GetMsg(_PSI.ModuleNumber, ps_ok), DIF_CENTERGROUP | DIF_DEFAULT, 0, 1);
	fardialog::DlgBUTTON button2("bn_cancel", _PSI.GetMsg(_PSI.ModuleNumber, ps_cancel), DIF_CENTERGROUP);

	std::vector<fardialog::Window*> hbox1c = {&radiohex, &radioper};
	fardialog::DlgHSizer hbox1(hbox1c);
	std::vector<fardialog::Window*> hbox2c = {&button1, &button2};
	fardialog::DlgHSizer hbox2(hbox2c);
	std::vector<fardialog::Window*> vbox1c = {
		&maskoff,
		&hbox1,
		&hline0,
		&hbox2
	};
	fardialog::DlgVSizer vbox1(vbox1c);

	typedef LONG_PTR (WINAPI goto_dlg::*tdlgproc)(HANDLE dlg, int msg, int param1, LONG_PTR param2);

	fardialog::DialogT<goto_dlg, tdlgproc> dlg(
		&_PSI, _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_title),
		0,
		0,
		*this, &goto_dlg::dlg_proc
	);
	dlg.buildFDI(&vbox1);

	dlgptr = &dlg;

	const HANDLE hDlg = dlg.DialogInit();
	int rc;
	while(true) {
		rc = dlg.DialogRun();
		if (rc == dlg.getID("bn_ok") ){
			UINT64 off = get_val();
			if ( off >= _file_size) {
				wchar_t scv[80];
				swprintf(scv, sizeof(scv), _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_spec), _file_size);
				const wchar_t* err_msg[] = { _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_title), _PSI.GetMsg(_PSI.ModuleNumber, ps_goto_out_ofr), scv };
				_PSI.Message(_PSI.ModuleNumber, FMSG_MB_OK | FMSG_WARNING, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
				continue;
			}
			offset = off;
			break;
		} else
			break;
	}

	_PSI.DialogFree(hDlg);
	return rc == dlg.getID("bn_ok");
}

UINT64 goto_dlg::get_val() const
{
	UINT64 offset = 0;
	std::wstring val(dlgptr->GetText(dlgptr->getID("tb_mask")));
	if( val.length() > 0 ){
		if (val[val.length() - 1] == L'%'){
			UINT64 percent = 0;
			swscanf(val.c_str(), L"%lld%%", &percent);
			offset = static_cast<UINT64>(_file_size * (percent / 100.0));
		}else if (val[0] == L'0' && val[1] == L'x')
			swscanf(val.c_str(), L"0x%llx", &offset);
		else
			swscanf(val.c_str(), L"%lld", &offset);
	}
	return offset;
}

