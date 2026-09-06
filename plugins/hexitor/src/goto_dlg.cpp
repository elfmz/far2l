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
#include "i18nindex.h"

LONG_PTR WINAPI goto_dlg::dlg_proc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	if (msg == DN_CLOSE && param1 == myDialog->getID("bn_ok")) {
		//Check parameters
		if ( get_val() >= _file_size) {
			wchar_t scv[80];
			swprintf(scv, ARRAYSIZE(scv), I18N(ps_goto_spec), _file_size-1);
			const wchar_t* err_msg[] = { I18N(ps_goto_title), I18N(ps_goto_out_ofr), scv };
			_PSI.Message(_PSI.ModuleNumber, FMSG_MB_OK | FMSG_WARNING, nullptr, err_msg, ARRAYSIZE(err_msg), 0);
			return 0;
		}
	}
	return _PSI.DefDlgProc(dlg, msg, param1, param2);
}

bool goto_dlg::show(const UINT64 file_size, UINT64& offset)
{
	_file_size = file_size;

	fardialog::DlgTEXT toffset(nullptr, I18N(ps_goto_offset));
	fardialog::DlgEDIT eoffset("tb_offset", 10, 0, DIF_HISTORY | DIF_MASKEDIT);
	fardialog::DlgHLine hline0(nullptr, nullptr);
	fardialog::DlgBUTTON button1("bn_ok", I18N(ps_ok), DIF_CENTERGROUP | DIF_DEFAULT, 0, 1);
	fardialog::DlgBUTTON button2("bn_cancel", I18N(ps_cancel), DIF_CENTERGROUP);

	fardialog::DlgHSizer hbox1({&toffset, &eoffset});
	fardialog::DlgHSizer hbox2({&button1, &button2});
	fardialog::DlgVSizer vbox1({
		&hbox1,
		&hline0,
		&hbox2
	});

	auto dlg = fardialog::CreateDialog(
		I18N(ps_goto_title),
		I18N(ps_goto_topic),
		0,
		*this,
		&goto_dlg::dlg_proc,
		vbox1
	);

	myDialog = &dlg;

	int rc;
	while(true) {
		rc = dlg.DialogRun();
		if (rc == dlg.getID("bn_ok") ){
			UINT64 off = get_val();
			if ( off >= _file_size) {
				wchar_t scv[80];
				swprintf(scv, ARRAYSIZE(scv), I18N(ps_goto_spec), _file_size-1);
				const wchar_t* err_msg[] = { I18N(ps_goto_title), I18N(ps_goto_out_ofr), scv };
				_PSI.Message(_PSI.ModuleNumber, FMSG_MB_OK | FMSG_WARNING, nullptr, err_msg, ARRAYSIZE(err_msg), 0);
				continue;
			}
			offset = off;
			break;
		} else
			break;
	}
	dlg.DialogFree();
	myDialog = nullptr;
	return rc == dlg.getID("bn_ok");
}

UINT64 goto_dlg::get_val() const
{
	UINT64 offset = 0xffffffffffffffffULL;
	std::wstring val(myDialog->GetText(myDialog->getID("tb_offset")));
	if( val.length() > 0 ){
		if (val[val.length() - 1] == L'%'){
			UINT64 percent = 0;
			swscanf(val.c_str(), L"%lld%%", &percent);
			offset = static_cast<UINT64>(_file_size * (percent / 100.0));
			if( offset == _file_size )
				offset = _file_size - 1;
		}else if (val[0] == L'0' && val[1] == L'x')
			swscanf(val.c_str(), L"0x%llx", &offset);
		else
			swscanf(val.c_str(), L"%lld", &offset);
	}
	return offset;
}

