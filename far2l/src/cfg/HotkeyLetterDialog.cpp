#include "headers.hpp"
#include "dialog.hpp"
#include "lang.hpp"
#include "strmix.hpp"

#include "HotkeyLetterDialog.hpp"

bool HotkeyLetterDialog(const wchar_t *Title, const wchar_t *Object, wchar_t &Letter)
{
	/*
	г================ Assign plugin hot key =================¬
	¦ Enter hot key (letter or digit)                        ¦
	¦ _                                                      ¦
	L========================================================-
	*/
	DialogDataEx HkDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,60,4,{},0,Title},
		{DI_TEXT,5,2,0,2,{},0,Msg::HelpHotKey},
		{DI_FIXEDIT,5,3,5,3,{},DIF_FOCUS|DIF_DEFAULT,L""},
		{DI_TEXT,8,3,58,3,{},0,Object}
	};
	MakeDialogItemsEx(HkDlgData, HkDlg);
	if (Letter) {
		HkDlg[2].strData = Letter;
	}

	{
		Dialog Dlg(HkDlg, ARRAYSIZE(HkDlg));
		Dlg.SetPosition(-1,-1,64,6);
		Dlg.Process();
		int ExitCode = Dlg.GetExitCode();
		if (ExitCode != 2)
			return false;
	}

	RemoveLeadingSpaces(HkDlg[2].strData);
	Letter = HkDlg[2].strData.IsEmpty() ? 0 : *HkDlg[2].strData.CPtr();
	return true;
}
