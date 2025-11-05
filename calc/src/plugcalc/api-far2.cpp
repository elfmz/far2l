//
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include "api.h"

#include <memory>

#include <farplug-wide.h>
#include <farkeys.h>
#include <farcolor.h>

#include <KeyFileHelper.h>
#include <utils.h>


static const wchar_t *PluginMenuStrings, *PluginConfigStrings;
static CalcDialog::CalcDialogCallback *msg_table = NULL;


/////////////////////////////////////////////////////////////////////////////////


class CalcDialogFuncsFar2 : public CalcDialogFuncs
{
public:
	CalcDialogFuncsFar2(PluginStartupInfo *Info)
	{
		this->Info = Info;
	}

	virtual void EnableRedraw(DLGHANDLE hdlg, bool en)
	{
		Info->SendDlgMessage(hdlg, DM_ENABLEREDRAW, en ? TRUE : FALSE, 0);
	}
	virtual void ResizeDialog(DLGHANDLE hdlg, const CalcCoord & dims)
	{
		Info->SendDlgMessage(hdlg, DM_RESIZEDIALOG, 0, (LONG_PTR)&dims);
	}
	virtual void RedrawDialog(DLGHANDLE hdlg)
	{
		Info->SendDlgMessage(hdlg, DM_REDRAW, 0, 0);
	}
	virtual void Close(DLGHANDLE hdlg, int exitcode)
	{
		Info->SendDlgMessage(hdlg, DM_CLOSE, exitcode, 0);
	}
	virtual void GetDlgRect(DLGHANDLE hdlg, CalcRect *rect)
	{
		Info->SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (LONG_PTR)rect);
	}

	virtual void GetDlgItemShort(DLGHANDLE hdlg, int id, FarDialogItem *item)
	{
		Info->SendDlgMessage(hdlg, DM_GETDLGITEMSHORT, id, (LONG_PTR)item);
	}
	virtual void SetDlgItemShort(DLGHANDLE hdlg, int id, const FarDialogItem & item)
	{
		Info->SendDlgMessage(hdlg, DM_SETDLGITEMSHORT, id, (LONG_PTR)&item);
	}
	virtual void SetItemPosition(DLGHANDLE hdlg, int id, const CalcRect & rect)
	{
		Info->SendDlgMessage(hdlg, DM_SETITEMPOSITION, id, (LONG_PTR)&rect);
	}
	virtual int  GetFocus(DLGHANDLE hdlg)
	{
		return (int)Info->SendDlgMessage(hdlg, DM_GETFOCUS, 0, 0);
	}
	virtual void SetFocus(DLGHANDLE hdlg, int id)
	{
		Info->SendDlgMessage(hdlg, DM_SETFOCUS, id, 0);
	}
	virtual void EditChange(DLGHANDLE hdlg, int id, const FarDialogItem & item)
	{
		Info->SendDlgMessage(hdlg, DN_EDITCHANGE, id, (LONG_PTR)&item);
	}
	virtual void SetSelection(DLGHANDLE hdlg, int id, const EditorSelect & sel)
	{
		Info->SendDlgMessage(hdlg, DM_SETSELECTION, id, (LONG_PTR)&sel);
	}
	virtual void SetCursorPos(DLGHANDLE hdlg, int id, const CalcCoord & pos)
	{
		Info->SendDlgMessage(hdlg, DM_SETCURSORPOS, id, (LONG_PTR)&pos);
	}
	virtual void GetText(DLGHANDLE hdlg, int id, std::wstring &str)
	{
		int len = (int)Info->SendDlgMessage(hdlg, DM_GETTEXTPTR, id, 0);
		str.clear();
		str.resize(len);
		Info->SendDlgMessage(hdlg, DM_GETTEXTPTR, id, (LONG_PTR)str.data());
	}
	virtual void SetText(DLGHANDLE hdlg, int id, const std::wstring & str)
	{
		Info->SendDlgMessage(hdlg, DM_SETTEXTPTR, id, (LONG_PTR)str.c_str());
	}
	virtual void AddHistory(DLGHANDLE hdlg, int id, const std::wstring & str)
	{
		Info->SendDlgMessage(hdlg, DM_ADDHISTORY, id, (LONG_PTR)str.c_str());
	}
	virtual bool IsChecked(DLGHANDLE hdlg, int id)
	{
		return ((int)Info->SendDlgMessage(hdlg, DM_GETCHECK, id, 0) == BSTATE_CHECKED);
	}

	////////////////////////////////////////////////////////////////////////////////////

	virtual DLGHANDLE DialogInit(int id, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic,
								struct FarDialogItem *Items, unsigned int ItemsNumber,
								CALCDLGPROC dlgProc)
	{
		return Info->DialogInit(Info->ModuleNumber, X1, Y1, X2, Y2, HelpTopic, Items, ItemsNumber, 0, 0,
								(FARWINDOWPROC)dlgProc, 0);
	}

	virtual intptr_t DialogRun(DLGHANDLE hdlg)
	{
		return (intptr_t)Info->DialogRun(hdlg);
	}

	virtual void DialogFree(DLGHANDLE hdlg)
	{
		Info->DialogFree(hdlg);
	}

	virtual CALC_INT_PTR DefDlgProc(DLGHANDLE hdlg, int msg, int param1, void *param2)
	{
		return Info->DefDlgProc(hdlg, msg, param1, (LONG_PTR)param2);
	}

	virtual CalcDialog::CalcDialogCallback *GetMessageTable()
	{
		if (msg_table == NULL)
		{
			const int max_msg_id = DM_USER + 32;
			msg_table = (CalcDialog::CalcDialogCallback *)malloc(max_msg_id * sizeof(CalcDialog::CalcDialogCallback));
			if (msg_table == NULL)
				return NULL;

			memset(msg_table, 0, max_msg_id * sizeof(CalcDialog::CalcDialogCallback));
			// fill the LUT
			msg_table[DN_INITDIALOG]      = &CalcDialog::OnInitDialog;
			msg_table[DN_CLOSE]           = &CalcDialog::OnClose;
			msg_table[DN_RESIZECONSOLE]   = &CalcDialog::OnResizeConsole;
			msg_table[DN_DRAWDIALOG]      = &CalcDialog::OnDrawDialog;
			msg_table[DN_BTNCLICK]        = &CalcDialog::OnButtonClick;
			msg_table[DN_GOTFOCUS]        = &CalcDialog::OnGotFocus;
			msg_table[DN_EDITCHANGE]      = &CalcDialog::OnEditChange;
			msg_table[DN_KEY]             = &CalcDialog::OnKey;
			msg_table[DN_CTLCOLORDLGITEM] = &CalcDialog::OnCtrlColorDlgItem;
		}
		return msg_table;
	}

protected:
	PluginStartupInfo *Info;
};

static CalcDialogFuncsFar2 *dlg_funcs = NULL;

/////////////////////////////////////////////////////////////////////////////////

class CalcApiFar2 : public CalcApi
{
	std::unique_ptr<KeyFileHelper> _settings_kfh;

public:

	virtual void GetPluginInfo(void *pinfo, const wchar_t *name)
	{
		struct PluginInfo *pInfo = (struct PluginInfo *)pinfo;
		pInfo->StructSize = sizeof(*pInfo);
		pInfo->Flags = PF_EDITOR | PF_VIEWER;
		pInfo->DiskMenuStringsNumber = 0;

		PluginMenuStrings = name;
		pInfo->PluginMenuStrings = &PluginMenuStrings;
		pInfo->PluginMenuStringsNumber = 1;

		PluginConfigStrings = name;
		pInfo->PluginConfigStrings = &PluginConfigStrings;
		pInfo->PluginConfigStringsNumber = 1;

		pInfo->CommandPrefix = CALC_PREFIX;
	}

	virtual const wchar_t *GetMsg(int MsgId)
	{
		return Info.GetMsg(Info.ModuleNumber, MsgId);
	}

	virtual int GetMinVersion(void *ver)
	{
		return MAKEFARVERSION(2, 2);
	}

	virtual CalcDialogFuncs *GetDlgFuncs()
	{
		if (!dlg_funcs)
			dlg_funcs = new CalcDialogFuncsFar2(&Info);
		return dlg_funcs;
	}

	virtual intptr_t Message(unsigned long Flags, const wchar_t *HelpTopic, const wchar_t * const *Items,
						int ItemsNumber, int ButtonsNumber)
	{
		return (intptr_t)Info.Message(Info.ModuleNumber, Flags, HelpTopic, Items, ItemsNumber, ButtonsNumber);
	}

	virtual intptr_t Menu(int X, int Y, int MaxHeight, unsigned long long Flags,
						const wchar_t *Title, const wchar_t *HelpTopic,
						const std::vector<FarMenuItem> & Items)
	{
		/// \TODO: find less ugly impl.
		const struct FarMenuItem *items = (const struct FarMenuItem *)&(*Items.begin());

		return (intptr_t)Info.Menu(Info.ModuleNumber, X, Y, MaxHeight, (DWORD)Flags, Title, NULL, HelpTopic,
							NULL, NULL, items, (int)Items.size());
	}

	virtual void EditorGet(EditorGetString *str, EditorInfo *info)
	{
		Info.EditorControl(ECTL_GETSTRING, str);
		Info.EditorControl(ECTL_GETINFO, info);
	}

	virtual void SetSelection(const EditorSelect & sel)
	{
		Info.EditorControl(ECTL_SELECT, (void *)&sel);
	}

	virtual void EditorInsert(const wchar_t *text)
	{
		Info.EditorControl(ECTL_INSERTTEXT, (void *)text);
	}

	virtual void EditorRedraw()
	{
		Info.EditorControl(ECTL_REDRAW, 0);
	}

	virtual void GetDlgColors(uint64_t *edit_color, uint64_t *sel_color, uint64_t *highlight_color)
	{
		Info.AdvControl(Info.ModuleNumber, ACTL_GETCOLOR, (void *)COL_DIALOGEDIT, edit_color);
		Info.AdvControl(Info.ModuleNumber, ACTL_GETCOLOR, (void *)COL_DIALOGEDITSELECTED, sel_color);
		Info.AdvControl(Info.ModuleNumber, ACTL_GETCOLOR, (void *)COL_DIALOGHIGHLIGHTTEXT, highlight_color);
	}

	virtual int GetCmdLine(std::wstring &cmd)
	{
		int len = Info.Control(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, 0, 0);
		cmd.clear();
		if (len > 0)
		{
			cmd.resize(len);
			Info.Control(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, len, (LONG_PTR)cmd.data());
			// rtrim
			if (cmd[len - 1] == '\0')
				cmd.resize(len - 1);
		}
		return len;
	}

	virtual void SetCmdLine(const std::wstring & cmd)
	{
		Info.Control(INVALID_HANDLE_VALUE, FCTL_SETCMDLINE, (int)cmd.size(), (LONG_PTR)cmd.data());
	}

	virtual bool SettingsBegin()
	{
		_settings_kfh.reset(new KeyFileHelper(InMyConfig("plugins/calc/config.ini")));
		return true;
	}

	virtual bool SettingsEnd()
	{
		if (!_settings_kfh->Save())
			return false;

		_settings_kfh.reset();
		return true;
	}

	virtual bool SettingsGet(const char *name, std::wstring *sval, int *ival)
	{
		if (!_settings_kfh->HasKey("Settings", name))
			return false;

		if (sval)
			StrMB2Wide(_settings_kfh->GetString("Settings", name), *sval);
		else if (ival)
			*ival = _settings_kfh->GetInt("Settings", name);
		else
			return false;

		return true;
	}

	virtual bool SettingsSet(const char *name, const std::wstring *sval, const int *ival)
	{
		if (sval)
			_settings_kfh->SetString("Settings", name, StrWide2MB(*sval).c_str());
		else if (ival)
			_settings_kfh->SetInt("Settings", name, *ival);
		else
			return false;

		return true;
	}

	virtual const wchar_t *GetModuleName()
	{
		return Info.ModuleName;
	}

protected:
	friend CalcApi *CreateApiFar2(const struct PluginStartupInfo *Info);

	PluginStartupInfo Info;
	HKEY hReg;
};

/////////////////////////////////////////////////////////////////////////////////

CalcApi *CreateApiFar2(const struct PluginStartupInfo *psi)
{
	if (psi == NULL
		|| (size_t)psi->StructSize < sizeof(PluginStartupInfo) // far container is too old
		|| psi->AdvControl == NULL)
	{
		return NULL;
	}

	CalcApiFar2 *api = new(std::nothrow) CalcApiFar2();

	if (api == NULL)
		return NULL;

	memset(&api->Info, 0, sizeof(api->Info));
	memmove(&api->Info, psi, std::min((size_t)psi->StructSize, sizeof(api->Info)));

	return api;
}

