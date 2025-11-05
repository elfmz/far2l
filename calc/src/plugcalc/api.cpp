//
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <cstdio>
#include <windows.h>

#include <unordered_map>

#include "api.h"
#include "calc.h"
#include "messages.h"


CalcApi *api = nullptr;
static CalcDialogFuncs *dlg_funcs = nullptr;

// exports


//////////////////////////////////////////////////////////////
// FAR exports
SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	if (!api) {
		api = CreateApiFar2(Info);
		if (api)
		{
			dlg_funcs = api->GetDlgFuncs();
			CalcStartup();
		}
	}
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	if (api)
		api->GetPluginInfo(Info, api->GetMsg(mName));
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	if (!api) {
		;

	} else if (OpenFrom == OPEN_EDITOR) {
		CalcOpenFromEditor();

	} else {
		const wchar_t *expression = (Item > 0xfff) ? (const wchar_t *)Item : nullptr;
		CalcOpen(expression);
	}

	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
	if (api)
		return CalcConfig();

	return 0;
}

SHAREDSYMBOL int WINAPI GetMinFarVersionW()
{
	#define MAKEFARVERSION(major,minor) ( ((major)<<16) | (minor))
	return MAKEFARVERSION(2, 2);
}

////////////////////////////////////////////////////////////////////

// dlg_hash.insert(std::make_pair(handle, dlgobj));
static std::unordered_map<DLGHANDLE, CalcDialog *> dlg_hash;

static CALC_INT_PTR __stdcall dlgProc(DLGHANDLE hdlg, int msg, int param1, void *param2)
{
	auto op = dlg_hash.find(hdlg);
	CALC_INT_PTR ret = -1;
	if (op != dlg_hash.end())
	{
		CalcDialog *dlg = op->second;

		if (dlg->msg_tbl[msg])
		{
			ret = (dlg->*(dlg->msg_tbl[msg]))(param1, param2);
		}
	}

	if (ret == -1)
		return dlg_funcs->DefDlgProc(hdlg, msg, param1, param2);
	return ret;
}

/////////////////////////////////////////////////////////////////////////

CalcDialog::CalcDialog()
{
	api->GetDlgColors(&editColor, &selColor, &highlightColor);

	msg_tbl = dlg_funcs->GetMessageTable();
	hdlg = nullptr;
}

CalcDialog::~CalcDialog()
{
	if (hdlg)
	{
		auto op = dlg_hash.find(hdlg);
		if (op != dlg_hash.end())
			dlg_hash.erase(op);
		dlg_funcs->DialogFree(hdlg);
	}
}

bool CalcDialog::Init(int id, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic,
							struct FarDialogItem *Item, unsigned int ItemsNumber)
{
	hdlg = dlg_funcs->DialogInit(id, X1, Y1, X2, Y2, HelpTopic, Item, ItemsNumber, dlgProc);
	if (hdlg == INVALID_HANDLE_VALUE)
		return false;
	dlg_hash.insert(std::make_pair(hdlg, this));
	return true;
}

intptr_t CalcDialog::Run()
{
	return dlg_funcs->DialogRun(hdlg);
}

void CalcDialog::EnableRedraw(bool en)
{
	dlg_funcs->EnableRedraw(hdlg, en);
}
void CalcDialog::ResizeDialog(const CalcCoord & dims)
{
	dlg_funcs->ResizeDialog(hdlg, dims);
}
void CalcDialog::RedrawDialog()
{
	dlg_funcs->RedrawDialog(hdlg);
}
void CalcDialog::GetDlgRect(CalcRect *rect)
{
	dlg_funcs->GetDlgRect(hdlg, rect);
}
void CalcDialog::Close(int exitcode)
{
	dlg_funcs->Close(hdlg, exitcode);
}

void CalcDialog::GetDlgItemShort(int id, FarDialogItem *item)
{
	dlg_funcs->GetDlgItemShort(hdlg, id, item);
}
void CalcDialog::SetDlgItemShort(int id, const FarDialogItem & item)
{
	dlg_funcs->SetDlgItemShort(hdlg, id, item);
}
void CalcDialog::SetItemPosition(int id, const CalcRect & rect)
{
	dlg_funcs->SetItemPosition(hdlg, id, rect);
}
int  CalcDialog::GetFocus()
{
	return dlg_funcs->GetFocus(hdlg);
}
void CalcDialog::SetFocus(int id)
{
	dlg_funcs->SetFocus(hdlg, id);
}
void CalcDialog::EditChange(int id, const FarDialogItem & item)
{
	dlg_funcs->EditChange(hdlg, id, item);
}
void CalcDialog::SetSelection(int id, const EditorSelect & sel)
{
	dlg_funcs->SetSelection(hdlg, id, sel);
}
void CalcDialog::SetCursorPos(int id, const CalcCoord & pos)
{
	dlg_funcs->SetCursorPos(hdlg, id, pos);
}
void CalcDialog::GetText(int id, std::wstring &str)
{
	dlg_funcs->GetText(hdlg, id, str);
}
void CalcDialog::SetText(int id, const std::wstring & str)
{
	dlg_funcs->SetText(hdlg, id, str);
}
void CalcDialog::AddHistory(int id, const std::wstring & str)
{
	dlg_funcs->AddHistory(hdlg, id, str);
}
bool CalcDialog::IsChecked(int id)
{
	return dlg_funcs->IsChecked(hdlg, id);
}