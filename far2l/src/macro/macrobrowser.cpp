/*
macrobrowser.cpp

Просмотр списка клавиатурных макросов
*/
/*
Copyright (c) 2026- Far2l People
*/

#include "headers.hpp"
#include "macro.hpp"
#include "macrobrowser.hpp"
#include "keyboard.hpp"
#include "interf.hpp" // for RemoveChar()
#include "clipboard.hpp" // for CopyToClipboard()
#include "config.hpp"
#include "vmenu.hpp"
#include "message.hpp" // for ExMessager
#include "CachedCreds.hpp" // for CachedHomeDir()
#include "dialog.hpp"

class MacroBrowser
{
private:
	class KeyMacro *Macro;
	bool b_hide_empty_areas;
	FARString fs2copy;
	VMenu ListMacro;
	std::vector< std::pair<int,int> > v_menu_macroindex;

	int View(const MacroRecord *mr, int iarea, int index);
	bool Edit(int imacro);
	void PrepareVMenu();

public:
	MacroBrowser()
		:
		Macro(nullptr),
		b_hide_empty_areas(false),
		ListMacro(nullptr, nullptr, 0, 0),
		v_menu_macroindex {}
		{ };

	inline int MacroReplaceAdd(int imacro, int iarea,
			DWORD Flags, const wchar_t *pstrKey, const wchar_t *pstrSequence, const wchar_t *pstrDescription)
	{ return (Macro == nullptr)
		? MacroReplaceAddRes::Busy
		: Macro->MacroReplaceAdd(imacro, iarea, Flags, pstrKey, pstrSequence, pstrDescription); };

	inline DWORD AssignMacroKey(FARString& macroDescription)
	{ return (Macro == nullptr) ? KEY_INVALID : Macro->AssignMacroKey(macroDescription); };

	void Show(class KeyMacro *KMacro);
	FARString TitleStr(bool b_hide_empty_areas = false);
};

enum MACROEDITDLG
{
	ME_DOUBLEBOX,

	ME_TEXT_AREA,
	ME_TEXT_AREA_CHMARK,
	ME_COMBO_AREA,

	ME_TEXT_KEY,
	ME_TEXT_KEY_CHMARK,
	ME_EDIT_KEY,
	ME_BUTTON_KEY,

	ME_TEXT_DESCRIPTION,
	ME_TEXT_DESCRIPTION_CHMARK,
	ME_EDIT_DESCRIPTION,

	ME_SINGLEBOX_SEQUENCE,
	ME_TEXT_SEQUENCE_CHMARK,
	ME_MEMOEDIT_SEQUENCE,
	ME_TEXT_SINGLEBOX_SEQUENCE_TIPS,
	ME_BUTTON_MLIB_TO_SEQUENCE,
	ME_COMBO_MLIB,
	ME_MEMOEDIT_MLIB_DESC,

	ME_TEXT_OUTPUT_CHMARK,
	ME_CHECKBOX_OUTPUT,
	ME_TEXT_START_CHMARK,
	ME_CHECKBOX_START,

	ME_CHECKBOX_A_PANEL,
	ME_TEXT_A_PLUGINPANEL_CHMARK,
	ME_CHECKBOX_A_PLUGINPANEL,
	ME_TEXT_A_FOLDERS_CHMARK,
	ME_CHECKBOX_A_FOLDERS,
	ME_TEXT_A_SELECTION_CHMARK,
	ME_CHECKBOX_A_SELECTION,

	ME_CHECKBOX_P_PANEL,
	ME_TEXT_P_PLUGINPANEL_CHMARK,
	ME_CHECKBOX_P_PLUGINPANEL,
	ME_TEXT_P_FOLDERS_CHMARK,
	ME_CHECKBOX_P_FOLDERS,
	ME_TEXT_P_SELECTION_CHMARK,
	ME_CHECKBOX_P_SELECTION,

	ME_TEXT_CMDLINE_CHMARK,
	ME_CHECKBOX_CMDLINE,
	ME_TEXT_SELBLOCK_CHMARK,
	ME_CHECKBOX_SELBLOCK,

	ME_TEXT_SENDTOPLUGINS_CHMARK,
	ME_CHECKBOX_SENDTOPLUGINS,
	ME_TEXT_DEACTIVATE_CHMARK,
	ME_CHECKBOX_DEACTIVATE,

	ME_SEPARATOR,
	ME_BUTTON_OK,
	ME_BUTTON_CANCEL,
	ME_BUTTON_RESET,
};

// для диалога редактирования
struct MacroEditDlgParam
{
	MacroBrowser *mb;
	int iarea;
	int imacro;
	const wchar_t *pstrArea;
	const MacroRecord *mr;
	const wchar_t *pstrKeyName;
	std::vector<FARString> *Descriptions;
};

static int Set3State(DWORD Flags, DWORD Chk1, DWORD Chk2)
{
	DWORD Chk12 = Chk1 | Chk2, FlagsChk12 = Flags & Chk12;

	if (FlagsChk12 == Chk12 || !FlagsChk12)
		return (2);
	else
		return (Flags & Chk1 ? 1 : 0);
}

inline static bool IsEditChanged(HANDLE hDlg, int EditControl, const wchar_t *Original)
{
	const wchar_t *Cur = reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, EditControl, 0));
	if (!Cur && !Original)
		return false;
	if (!Cur || !Original)
		return true;
	return 0 != StrCmp(Cur, Original);
}

static bool IsMemoEditChanged(HANDLE hDlg, int EditControl, const wchar_t *Original)
{
	const wchar_t *Cur = reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, EditControl, 0));
	if (!Cur && !Original)
		return false;
	if (!Cur || !Original)
		return true;
	FARString strCur = Cur; // DI_MEMOEDIT always add trailing newline
	size_t last = strCur.GetLength();
	if (last > 0) {
		last--;
		if (strCur[last] == '\n')
			strCur.Truncate(last);
	}
	return 0 != StrCmp(strCur, Original);
}

inline static void DlgDefaultMark(HANDLE hDlg, int id, bool nodefault)
{
	// id - id of DI_TEXT for non-default marker
	SendDlgMessage(hDlg, DM_SETTEXTPTR, id, // mark (un)changed
		nodefault ? reinterpret_cast<LONG_PTR>(L"*") : reinterpret_cast<LONG_PTR>(L"") );
}

static void MacroEditDlg_SetDefault(HANDLE hDlg, const MacroEditDlgParam *MEParam)
{
	SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
	//SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_COMBO_AREA, reinterpret_cast<LONG_PTR>(MEParam->pstrArea) );
	{
		FarListPos lpos { MEParam->iarea, -1 };
		SendDlgMessage(hDlg, DM_LISTSETCURPOS, ME_COMBO_AREA, (LONG_PTR)&lpos);
	}
	SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_EDIT_KEY, reinterpret_cast<LONG_PTR>(NullToEmpty(MEParam->pstrKeyName)) );
	SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, ME_EDIT_KEY, 0 ); // for clear selection
	SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_EDIT_DESCRIPTION, reinterpret_cast<LONG_PTR>(NullToEmpty(MEParam->mr->Description)) );
	SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, ME_EDIT_DESCRIPTION, 0 ); // for clear selection
	SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_MEMOEDIT_SEQUENCE, reinterpret_cast<LONG_PTR>(NullToEmpty(MEParam->mr->Src)) );
	SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, ME_MEMOEDIT_SEQUENCE, 0 ); // for clear selection

	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_OUTPUT,
			MEParam->mr->Flags & MFLAGS_DISABLEOUTPUT ? 0 : 1);
	DlgDefaultMark(hDlg, ME_CHECKBOX_OUTPUT-1, false);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_START,
			MEParam->mr->Flags & MFLAGS_RUNAFTERFARSTART ? 1 : 0);
	DlgDefaultMark(hDlg, ME_CHECKBOX_START-1, false);

	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_A_PLUGINPANEL,
			Set3State(MEParam->mr->Flags, MFLAGS_NOFILEPANELS, MFLAGS_NOPLUGINPANELS));
	DlgDefaultMark(hDlg, ME_CHECKBOX_A_PLUGINPANEL-1, false);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_A_FOLDERS,
			Set3State(MEParam->mr->Flags, MFLAGS_NOFILES, MFLAGS_NOFOLDERS));
	DlgDefaultMark(hDlg, ME_CHECKBOX_A_FOLDERS-1, false);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_A_SELECTION,
			Set3State(MEParam->mr->Flags, MFLAGS_SELECTION, MFLAGS_NOSELECTION));
	DlgDefaultMark(hDlg, ME_CHECKBOX_A_SELECTION-1, false);

	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_P_PLUGINPANEL,
			Set3State(MEParam->mr->Flags, MFLAGS_PNOFILEPANELS, MFLAGS_PNOPLUGINPANELS));
	DlgDefaultMark(hDlg, ME_CHECKBOX_P_PLUGINPANEL-1, false);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_P_FOLDERS,
			Set3State(MEParam->mr->Flags, MFLAGS_PNOFILES, MFLAGS_PNOFOLDERS));
	DlgDefaultMark(hDlg, ME_CHECKBOX_P_FOLDERS-1, false);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_P_SELECTION,
			Set3State(MEParam->mr->Flags, MFLAGS_PSELECTION, MFLAGS_PNOSELECTION));
	DlgDefaultMark(hDlg, ME_CHECKBOX_P_SELECTION-1, false);

	bool bPanel = MEParam->mr->Flags
			& (MFLAGS_NOFILEPANELS | MFLAGS_NOPLUGINPANELS
				| MFLAGS_NOFILES | MFLAGS_NOFOLDERS
				| MFLAGS_SELECTION | MFLAGS_NOSELECTION);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_A_PANEL, bPanel);
	SendDlgMessage(hDlg, DM_ENABLE, ME_CHECKBOX_A_PLUGINPANEL, bPanel);
	SendDlgMessage(hDlg, DM_ENABLE, ME_CHECKBOX_A_FOLDERS, bPanel);
	SendDlgMessage(hDlg, DM_ENABLE, ME_CHECKBOX_A_SELECTION, bPanel);

	bPanel = MEParam->mr->Flags
			& (MFLAGS_PNOFILEPANELS | MFLAGS_PNOPLUGINPANELS
				| MFLAGS_PNOFILES | MFLAGS_PNOFOLDERS
				| MFLAGS_PSELECTION | MFLAGS_PNOSELECTION);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_P_PANEL, bPanel);
	SendDlgMessage(hDlg, DM_ENABLE, ME_CHECKBOX_P_PLUGINPANEL, bPanel);
	SendDlgMessage(hDlg, DM_ENABLE, ME_CHECKBOX_P_FOLDERS, bPanel);
	SendDlgMessage(hDlg, DM_ENABLE, ME_CHECKBOX_P_SELECTION, bPanel);

	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_CMDLINE,
			Set3State(MEParam->mr->Flags, MFLAGS_EMPTYCOMMANDLINE, MFLAGS_NOTEMPTYCOMMANDLINE));
	DlgDefaultMark(hDlg, ME_CHECKBOX_CMDLINE-1, false);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_SELBLOCK,
			Set3State(MEParam->mr->Flags, MFLAGS_EDITSELECTION, MFLAGS_EDITNOSELECTION));
	DlgDefaultMark(hDlg, ME_CHECKBOX_SELBLOCK-1, false);

	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_SENDTOPLUGINS,
			MEParam->mr->Flags & MFLAGS_NOSENDKEYSTOPLUGINS ? 0 : 1);
	DlgDefaultMark(hDlg, ME_CHECKBOX_SENDTOPLUGINS-1, false);
	SendDlgMessage(hDlg, DM_SETCHECK, ME_CHECKBOX_DEACTIVATE,
			MEParam->mr->Flags & MFLAGS_DISABLEMACRO ? 1 : 0);
	DlgDefaultMark(hDlg, ME_CHECKBOX_DEACTIVATE-1, false);

	SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
}

static LONG_PTR WINAPI MacroEditDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	const auto *MEParam = (Msg == DN_INITDIALOG)
			? reinterpret_cast<MacroEditDlgParam *>(Param2)
			: reinterpret_cast<MacroEditDlgParam *>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));

	if (MEParam) {

		switch (Msg) {
			case DN_INITDIALOG:
				MacroEditDlg_SetDefault(hDlg, MEParam);
				{
					FarListPos lpos {0, -1};
					SendDlgMessage(hDlg, DM_LISTSETCURPOS, ME_COMBO_MLIB, (LONG_PTR)&lpos);
					FarListTitles ListTitle {};
					ListTitle.Title = L"MacroLibrary";
					ListTitle.Bottom = L"Enter Ctrl-C Ctrl-Alt-F";
					SendDlgMessage(hDlg, DM_LISTSETTITLES, ME_COMBO_MLIB, (LONG_PTR)&ListTitle);

					size_t i = (size_t) SendDlgMessage(hDlg, DM_LISTGETCURPOS, ME_COMBO_MLIB, 0);
					if (i < MEParam->Descriptions->size())
						SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_MEMOEDIT_MLIB_DESC,
								reinterpret_cast<LONG_PTR>(MEParam->Descriptions->at(i).CPtr()));
				}
				break;

			case DN_KILLFOCUS:
			case DN_EDITCHANGE:
				switch (Param1) {
					case ME_COMBO_AREA:
						DlgDefaultMark(hDlg, ME_COMBO_AREA-1, // mark (un)changed
							IsEditChanged(hDlg, ME_COMBO_AREA, MEParam->pstrArea) );
						break;
					case ME_EDIT_KEY:
						DlgDefaultMark(hDlg, ME_EDIT_KEY-1, // mark (un)changed
							IsEditChanged(hDlg, ME_EDIT_KEY, MEParam->pstrKeyName) );
						break;
					case ME_EDIT_DESCRIPTION:
						DlgDefaultMark(hDlg, ME_EDIT_DESCRIPTION-1, // mark (un)changed
							IsEditChanged(hDlg, ME_EDIT_DESCRIPTION, MEParam->mr->Description) );
						break;
					case ME_MEMOEDIT_SEQUENCE:
						DlgDefaultMark(hDlg, ME_MEMOEDIT_SEQUENCE-1, // mark (un)changed
							IsMemoEditChanged(hDlg, ME_MEMOEDIT_SEQUENCE, MEParam->mr->Src) );
						break;
				}
				break;

			case DN_LISTCHANGE: {
					if (Param1 == ME_COMBO_MLIB) {
						SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_MEMOEDIT_MLIB_DESC,
							((size_t)Param2 < MEParam->Descriptions->size())
								? reinterpret_cast<LONG_PTR>(MEParam->Descriptions->at(Param2).CPtr())
								: reinterpret_cast<LONG_PTR>(L"")
						);
						return TRUE;
					}
				}
				break;

			case DN_BTNCLICK:
				switch (Param1) {
					case ME_CHECKBOX_A_PANEL:
					case ME_CHECKBOX_P_PANEL:
						for (int i = 2; i <= 6; i+=2)
							SendDlgMessage(hDlg, DM_ENABLE, Param1 + i, Param2);
						break;
					case ME_CHECKBOX_OUTPUT:
						DlgDefaultMark(hDlg, Param1-1, Param2 != (MEParam->mr->Flags & MFLAGS_DISABLEOUTPUT ? 0 : 1)); // mark (un)changed
						break;
					case ME_CHECKBOX_START:
						DlgDefaultMark(hDlg, Param1-1, Param2 != (MEParam->mr->Flags & MFLAGS_RUNAFTERFARSTART ? 1 : 0)); // mark (un)changed
						break;
					case ME_CHECKBOX_A_PLUGINPANEL:
						DlgDefaultMark(hDlg, Param1-1, Param2 != Set3State(MEParam->mr->Flags, MFLAGS_NOFILEPANELS, MFLAGS_NOPLUGINPANELS)); // mark (un)changed
						break;
					case ME_CHECKBOX_A_FOLDERS:
						DlgDefaultMark(hDlg, Param1-1, Param2 != Set3State(MEParam->mr->Flags, MFLAGS_NOFILES, MFLAGS_NOFOLDERS)); // mark (un)changed
						break;
					case ME_CHECKBOX_A_SELECTION:
						DlgDefaultMark(hDlg, Param1-1, Param2 != Set3State(MEParam->mr->Flags, MFLAGS_SELECTION, MFLAGS_NOSELECTION)); // mark (un)changed
						break;
					case ME_CHECKBOX_P_PLUGINPANEL:
						DlgDefaultMark(hDlg, Param1-1, Param2 != Set3State(MEParam->mr->Flags, MFLAGS_PNOFILEPANELS, MFLAGS_PNOPLUGINPANELS)); // mark (un)changed
						break;
					case ME_CHECKBOX_P_FOLDERS:
						DlgDefaultMark(hDlg, Param1-1, Param2 != Set3State(MEParam->mr->Flags, MFLAGS_PNOFILES, MFLAGS_PNOFOLDERS)); // mark (un)changed
						break;
					case ME_CHECKBOX_P_SELECTION:
						DlgDefaultMark(hDlg, Param1-1, Param2 != Set3State(MEParam->mr->Flags, MFLAGS_PSELECTION, MFLAGS_PNOSELECTION)); // mark (un)changed
						break;
					case ME_CHECKBOX_CMDLINE:
						DlgDefaultMark(hDlg, Param1-1, Param2 != Set3State(MEParam->mr->Flags, MFLAGS_EMPTYCOMMANDLINE, MFLAGS_NOTEMPTYCOMMANDLINE)); // mark (un)changed
						break;
					case ME_CHECKBOX_SELBLOCK:
						DlgDefaultMark(hDlg, Param1-1, Param2 != Set3State(MEParam->mr->Flags, MFLAGS_EDITSELECTION, MFLAGS_EDITNOSELECTION)); // mark (un)changed
						break;
					case ME_CHECKBOX_SENDTOPLUGINS:
						DlgDefaultMark(hDlg, Param1-1, Param2 != (MEParam->mr->Flags & MFLAGS_NOSENDKEYSTOPLUGINS ? 0 : 1)); // mark (un)changed
						break;
					case ME_CHECKBOX_DEACTIVATE:
						DlgDefaultMark(hDlg, Param1-1, Param2 != (MEParam->mr->Flags & MFLAGS_DISABLEMACRO ? 1 : 0)); // mark (un)changed
						break;
					case ME_BUTTON_KEY: {
							FARString strDescription = (const wchar_t*)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, ME_EDIT_DESCRIPTION, 0);
							DWORD newKey = MEParam->mb->AssignMacroKey(strDescription);
							if (newKey != KEY_INVALID) {
								FARString strKeyName;
								KeyToText(newKey, strKeyName);
								SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_EDIT_KEY, reinterpret_cast<LONG_PTR>(strKeyName.CPtr()) );
								SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_EDIT_DESCRIPTION, reinterpret_cast<LONG_PTR>(strDescription.CPtr()) );
								SendDlgMessage(hDlg, DM_SETFOCUS, ME_EDIT_KEY, 0);
							}
						}
						return TRUE;
					case ME_BUTTON_MLIB_TO_SEQUENCE: {
							FarListGetItem List{};
							List.ItemIndex = SendDlgMessage(hDlg, DM_LISTGETCURPOS, ME_COMBO_MLIB, 0);
							if (!SendDlgMessage(hDlg, DM_LISTGETITEM , ME_COMBO_MLIB, (LONG_PTR)&List)
									|| !List.Item.Text || !List.Item.Text[0])
								return TRUE;

							if (!SendDlgMessage(hDlg, DM_SETFOCUS, ME_MEMOEDIT_SEQUENCE, 0))
								return TRUE;

							// workaround for put text in current position of DI_MEMOEDIT
							const size_t n1 = wcslen(List.Item.Text);
							const size_t n2 = n1 + 2;
							DWORD *keys = new DWORD[n2];
							if (!keys)
								return TRUE;
							keys[0] = ' '; // surround space before
							for (size_t i = 0; i < n1; i++)
								keys[i + 1] = List.Item.Text[i];
							keys[n2 - 1] = ' '; // surround space after
							SendDlgMessage(hDlg, DM_KEY, n2, (LONG_PTR)keys);

							// workaround because DI_MEMOEDIT not process DN_EDITCHANGE properly
							DlgDefaultMark(hDlg, ME_MEMOEDIT_SEQUENCE-1, // mark (un)changed
								IsMemoEditChanged(hDlg, ME_MEMOEDIT_SEQUENCE, MEParam->mr->Src) );

							// workaround to activate cursor in DI_MEMOEDIT after change mark
							SendDlgMessage(hDlg, DM_SETFOCUS, ME_MEMOEDIT_SEQUENCE, 0);
							keys[0] = KEY_LEFT;
							SendDlgMessage(hDlg, DM_KEY, 1, (LONG_PTR)keys);

							delete[] keys;
						}
						return TRUE;
					case ME_BUTTON_RESET:
						MacroEditDlg_SetDefault(hDlg, MEParam);
						// workaround because DI_MEMOEDIT not process DN_EDITCHANGE properly
						DlgDefaultMark(hDlg, ME_MEMOEDIT_SEQUENCE-1, false);// mark (un)changed
						SendDlgMessage(hDlg, DM_SETFOCUS, ME_COMBO_AREA, 0);
						return TRUE;
				}
				break;

			case DN_CLOSE:
				if (Param1 == ME_BUTTON_OK) {

					// клавиатурная комбинация
					const wchar_t *pstrKeyName = (const wchar_t*)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, ME_EDIT_KEY, 0);
					if (!pstrKeyName || pstrKeyName[0] == 0 || pstrKeyName[wcsspn(pstrKeyName, L" \t\n\r\f\v")] == 0) {
						SendDlgMessage(hDlg, DM_SETFOCUS, ME_EDIT_KEY, 0);
						Message(MSG_WARNING, 1,
							L"Edit Macro",
							//L"Редактирование макрокоманды"/*Msg::MacroEditTitle*/,
							L"Empty keyboard shortcut field",
							//L"Пустое поле значения клавиатурной комбинации"/*Msg::MacroEditFieldKeyEmpty*/,
							Msg::Ok);
						return FALSE;
					}

					// последовательность макрокоманды
					FARString strSeq = (const wchar_t*)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, ME_MEMOEDIT_SEQUENCE, 0);
					{
						RemoveExternalSpaces(strSeq);
						while (strSeq.Contains(L" \n")) // removing TrailingSpaces in each string
							ReplaceStrings(strSeq, L" \n", L"\n");
						SendDlgMessage(hDlg, DM_SETTEXTPTR, ME_MEMOEDIT_SEQUENCE, reinterpret_cast<LONG_PTR>(strSeq.CPtr()) );
					}

					// описание макроса
					const wchar_t *pstrDescription = (const wchar_t*)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, ME_EDIT_DESCRIPTION, 0);

					// область действия
					int iArea = SendDlgMessage(hDlg, DM_LISTGETCURPOS, ME_COMBO_AREA, 0);

					// Флаги макропоследовательности
					DWORD Flags = SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_OUTPUT, 0) ? 0 : MFLAGS_DISABLEOUTPUT;
					Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_START, 0) ? MFLAGS_RUNAFTERFARSTART : 0;

					if (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_A_PANEL, 0)) {
						Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_A_PLUGINPANEL, 0) == 2
								? 0
								: (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_A_PLUGINPANEL, 0) == 0
												? MFLAGS_NOPLUGINPANELS : MFLAGS_NOFILEPANELS);
						Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_A_FOLDERS, 0) == 2
								? 0
								: (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_A_FOLDERS, 0) == 0
												? MFLAGS_NOFOLDERS : MFLAGS_NOFILES);
						Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_A_SELECTION, 0) == 2
								? 0
								: (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_A_SELECTION,0) == 0
												? MFLAGS_NOSELECTION : MFLAGS_SELECTION);
					}

					if (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_P_PANEL, 0)) {
						Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_P_PLUGINPANEL, 0) == 2
								? 0
								: (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_P_PLUGINPANEL, 0) == 0
												? MFLAGS_PNOPLUGINPANELS : MFLAGS_PNOFILEPANELS);
						Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_P_FOLDERS, 0) == 2
								? 0
								: (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_P_FOLDERS, 0) == 0
												? MFLAGS_PNOFOLDERS : MFLAGS_PNOFILES);
						Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_P_SELECTION, 0) == 2
								? 0
								: (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_P_SELECTION, 0) == 0
												? MFLAGS_PNOSELECTION : MFLAGS_PSELECTION);
					}

					Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_CMDLINE, 0) == 2
							? 0
							: (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_CMDLINE, 0) == 0
											? MFLAGS_NOTEMPTYCOMMANDLINE : MFLAGS_EMPTYCOMMANDLINE);
					Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_SELBLOCK, 0) == 2
							? 0
							: (SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_SELBLOCK, 0) == 0
											? MFLAGS_EDITNOSELECTION : MFLAGS_EDITSELECTION);

					Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_SENDTOPLUGINS, 0) ? 0 : MFLAGS_NOSENDKEYSTOPLUGINS;
					Flags|= SendDlgMessage(hDlg, DM_GETCHECK, ME_CHECKBOX_DEACTIVATE, 0) ? MFLAGS_DISABLEMACRO : 0;

					// собственно изменяем/добавляем макрос
					switch (MEParam->mb->MacroReplaceAdd(
								MEParam->imacro, iArea, Flags, pstrKeyName, strSeq.CPtr(), pstrDescription)) {
						case MacroReplaceAddRes::Success: // successfully
							return TRUE;
						case MacroReplaceAddRes::InvalidKey:
							SendDlgMessage(hDlg, DM_SETFOCUS, ME_EDIT_KEY, 0);
							Message(MSG_WARNING, 1,
								L"Edit Macro",
								//L"Редактирование макрокоманды"/*Msg::MacroEditTitle*/,
								L"Invalid Key",
								//L"Некорректная комбинация клавиш"/*Msg::MacroEditFieldKeyInvalid*/,
								Msg::Ok);
							return FALSE;
						case MacroReplaceAddRes::EmptySequence:
							SendDlgMessage(hDlg, DM_SETFOCUS, ME_MEMOEDIT_SEQUENCE, 0);
							Message(MSG_WARNING, 1,
								L"Edit Macro",
								//L"Редактирование макрокоманды"/*Msg::MacroEditTitle*/,
								L"Empty macro sequence field",
								//L"Пустое поле значения последовательности макрокоманды"/*Msg::MacroEditFieldSequenceEmpty*/,
								Msg::Ok);
							return FALSE;
						case MacroReplaceAddRes::InvalidSequence: // parse error show Message inside ParseMacroString()
							SendDlgMessage(hDlg, DM_SETFOCUS, ME_MEMOEDIT_SEQUENCE, 0);
							return FALSE;
						case MacroReplaceAddRes::NoChanges: // Изменения полностью эквивалентны исходному => не трогаем
							//SendDlgMessage(hDlg, DM_SETFOCUS, ME_COMBO_AREA, 0);
							//Message(MSG_WARNING, 1, L"Edit Macro",
							//L"Редактирование макрокоманды"/*Msg::MacroEditTitle*/,
							//	L"Изменения полностью эквивалентны исходному"/*Msg::MacroEditNoChanges*/,
							//	Msg::Ok);
							//return FALSE;
							return TRUE;
						case MacroReplaceAddRes::DuplicateAreaKey:
							SendDlgMessage(hDlg, DM_SETFOCUS, ME_EDIT_KEY, 0);
							Message(MSG_WARNING, 1,
								L"Edit Macro",
								//L"Редактирование макрокоманды"/*Msg::MacroEditTitle*/,
								L"There is already another macro with the same key combination in the selected area",
								//L"В выбранной области уже присутствует другой макрос с такой же комбинацией клавиш"/*Msg::MacroEditAreaKeyDup*/,
								Msg::Ok);
							return FALSE;
						default: // 1, 2, 3, 4, 10, 11: internal errors
							SendDlgMessage(hDlg, DM_SETFOCUS, ME_COMBO_AREA, 0);
							Message(MSG_WARNING, 1,
								L"Edit Macro",
								//L"Редактирование макрокоманды"/*Msg::MacroEditTitle*/,
								L"Internal error while editing/adding macro",
								//L"Внутренняя ошибка при изменении/добавлении макроса"/*Msg::MacroEditInternalError*/,
								Msg::Ok);
							return FALSE;
					} // switch( MacroReplaceAdd(...
				}
				break;
		}

		if (Param1 == ME_COMBO_MLIB) {
			if ((Msg == DN_KEY && (Param2 == KEY_ENTER || Param2 == KEY_NUMENTER))
					|| (Msg == DN_MOUSECLICK && ((MOUSE_EVENT_RECORD *)Param2)->dwEventFlags==DOUBLE_CLICK)) {
				// Enter or DblClk in Library list of Keywords/Functions make action from ME_BUTTON_MLIB_TO_SEQUENCE
				SendDlgMessage(hDlg, DM_SETFOCUS, ME_BUTTON_MLIB_TO_SEQUENCE, 0);
				DWORD Keys[1]={KEY_ENTER};
				SendDlgMessage(hDlg, DM_KEY, ARRAYSIZE(Keys), (LONG_PTR)Keys);
				return TRUE;
			}
			if(Msg == DN_KEY && (Param2 == KEY_CTRLC || Param2 == KEY_CTRLINS || Param2 == KEY_CTRLNUMPAD0)) {
				// copy current element from library to clipboard
				FarListGetItem List{};
				List.ItemIndex = SendDlgMessage(hDlg, DM_LISTGETCURPOS, ME_COMBO_MLIB, 0);
				if (SendDlgMessage(hDlg, DM_LISTGETITEM , ME_COMBO_MLIB, (LONG_PTR)&List)
						&& List.Item.Text)
					CopyToClipboard(List.Item.Text);
				return TRUE;
			}

		}

	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool MacroBrowser::Edit(int imacro)
{
	if (imacro >= Macro->MacroLIBCount || (imacro > 0 && !Macro->MacroLIB))
		return false;

	wchar_t strEmpty[] = L"";
	MacroRecord mr0{MACRO_COMMON, KEY_INVALID, 0, nullptr, strEmpty, strEmpty };
	const MacroRecord *mr = (imacro < 0) ? &mr0 : &Macro->MacroLIB[imacro];
	const int iarea =  mr->Flags & MFLAGS_MODEMASK;
	const wchar_t *pstrArea = Macro->GetSubKey(iarea);
	FARString strKeyName {};
	if (imacro >= 0)
		KeyToText(mr->Key, strKeyName);

	/*
			  1         2         3         4         5         6
	   3456789012345678901234567890123456789012345678901234567890123456789
	 1 г=============== Редактирование макрокоманды =====================¬­
	 2 | Область действия:    *_________________________________________ |
	 3 | Комбинация клавиш:   *___________________ [ << Ввести клавишу ] |
	 4 | Описание макроса:    *_________________________________________ |
	 5 |--- Последовательность: ----------------------------------------¬|
	 6 ||*______________________________________________________________||
	 7 || ______________________________________________________________||
	 8 ||  _____________________________________________________________||
	 9 |L[ ^^ << ] _____________________________________________________+|
	10 | *[ ] Разрешить во время выполнения вывод на экран               |
	11 | *[ ] Выполнять после запуска FAR                                |
	12 | [ ] Активная панель             [ ] Пассивная панель            |
	13 |   *[?] На панели плагина         *[?] На панели плагина         |
	14 |   *[?] Выполнять для папок       *[?] Выполнять для папок       |
	15 |   *[?] Отмечены файлы            *[?] Отмечены файлы            |
	16 | *[?] Пустая командная строка                                    |
	17 | *[?] Отмечен блок                                               |
    18 | *[x] Посылать макрокоманду в плагины                            |
    19 | *[ ] Отключить макрокоманду                                     |
	20 |-----------------------------------------------------------------|
	21 |                { Ok }  [ Отменить ]  [ Сбросить ]               |
	22 L=================================================================+

	*/
	int DLG_WIDTH = 73;
	const int DLG_HEIGHT = 24;
	DialogDataEx MacroEditDlgData[] = {
		{DI_DOUBLEBOX, 3,  1,  (SHORT)(DLG_WIDTH - 4), (SHORT)(DLG_HEIGHT - 2), {},  0,
				(imacro < 0) ? L"New Macro"/*L"Новая макрокоманда"*//*Msg::MacroEditNewTitle*/
							 : L"Edit Macro"/*L"Редактирование макрокоманды"*//*Msg::MacroEditTitle*/},

		{DI_TEXT,      5,  2,  25, 2,  {},  0,                                L"Macro Area:"/*L"Область действия:"*//*Msg::MacroArea*/ },
		{DI_TEXT,      26, 2,  26, 2,  {},  0,                                L""},
		{DI_COMBOBOX,  27, 2,  66, 2,  {},  DIF_FOCUS | DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE, L"" },

		{DI_TEXT,      5,  3,  25, 3,  {},  0,                                L"Macro Key:"/*L"Комбинация клавиш:"*//*Msg::MacroKey*/ },
		{DI_TEXT,      26, 3,  26, 3,  {},  0,                                L""},
		{DI_EDIT,      27, 3,  45, 3,  {},  DIF_FOCUS,                        L""                                      },
		{DI_BUTTON,    47, 3,  67, 3,  {},  DIF_FOCUS | DIF_BTNNOCLOSE,       L"<< Assign Key"/*L"<< Ввести клавишу"*//*Msg::MacroEditTitle*/ },

		{DI_TEXT,      5,  4,  25, 4,  {},  0,                                Msg::DefineMacroDescription              },
		{DI_TEXT,      26, 4,  26, 4,  {},  0,                                L""},
		{DI_EDIT,      27, 4,  67, 4,  {},  DIF_FOCUS,                        L""},

		{DI_SINGLEBOX, 4,  5,  68, 9,  {},  DIF_LEFTTEXT,                     Msg::MacroSequence                       },
		{DI_TEXT,      5,  6,  5,  6,  {},  0,                                L""},
		{DI_MEMOEDIT,  6,  6,  67, 8,  {},  DIF_FOCUS,                        L""},
		{DI_TEXT,      5,  9,  19, 9,  {},  DIF_HIDDEN,                       L" F3 Ctrl-F3 F5 "                       },
		{DI_BUTTON,    5,  9,  13, 9,  {},  DIF_FOCUS | DIF_BTNNOCLOSE,       L"^^ <<"                                 },
		{DI_COMBOBOX,  15, 9,  66, 9,  {},  DIF_FOCUS | DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE, L"" },
		{DI_MEMOEDIT,  15, 9,  66, 9,  {},  DIF_FOCUS | DIF_READONLY | DIF_HIDDEN, L""},

		{DI_TEXT,      5,  10, 5,  10, {},  0,                                L""},
		{DI_CHECKBOX,  6,  10, 0,  10, {},  0,                                Msg::MacroSettingsEnableOutput            },
		{DI_TEXT,      5,  11, 5,  11, {},  0,                                L""},
		{DI_CHECKBOX,  6,  11, 0,  11, {},  0,                                Msg::MacroSettingsRunAfterStart           },

		{DI_CHECKBOX,  5,  12, 0,  12, {},  0,                                Msg::MacroSettingsActivePanel             },
		{DI_TEXT,      6,  13, 6,  13, {},  0,                                L""},
		{DI_CHECKBOX,  7,  13, 0,  13, {2}, DIF_3STATE | DIF_DISABLE,         Msg::MacroSettingsPluginPanel             },
		{DI_TEXT,      6,  14, 6,  14, {},  0,                                L""},
		{DI_CHECKBOX,  7,  14, 0,  14, {2}, DIF_3STATE | DIF_DISABLE,         Msg::MacroSettingsFolders                 },
		{DI_TEXT,      6,  15, 6,  15, {},  0,                                L""},
		{DI_CHECKBOX,  7,  15, 0,  15, {2}, DIF_3STATE | DIF_DISABLE,         Msg::MacroSettingsSelectionPresent        },
		{DI_CHECKBOX,  37, 12, 0,  12, {},  0,                                Msg::MacroSettingsPassivePanel            },
		{DI_TEXT,      38, 13, 38, 13, {},  0,                                L""},
		{DI_CHECKBOX,  39, 13, 0,  13, {2}, DIF_3STATE | DIF_DISABLE,         Msg::MacroSettingsPluginPanel             },
		{DI_TEXT,      38, 14, 38, 14, {},  0,                                L""},
		{DI_CHECKBOX,  39, 14, 0,  14, {2}, DIF_3STATE | DIF_DISABLE,         Msg::MacroSettingsFolders                 },
		{DI_TEXT,      38, 15, 38, 15, {},  0,                                L""},
		{DI_CHECKBOX,  39, 15, 0,  15, {2}, DIF_3STATE | DIF_DISABLE,         Msg::MacroSettingsSelectionPresent        },

		{DI_TEXT,      5,  16, 5,  16, {},  0,                                L""},
		{DI_CHECKBOX,  6,  16, 0,  16, {2}, DIF_3STATE,                       Msg::MacroSettingsCommandLine             },
		{DI_TEXT,      5,  17, 5,  17, {},  0,                                L""},
		{DI_CHECKBOX,  6,  17, 0,  17, {2}, DIF_3STATE,                       Msg::MacroSettingsSelectionBlockPresent   },

		{DI_TEXT,      5,  18, 5,  18, {},  0,                                L""},
		{DI_CHECKBOX,  6,  18, 0,  18, {},  0,                                L"Send macro to plugins"/*L"Посылать макрокоманду в плагины"*//*Msg::MacroEditSendToPlugins*/},
		{DI_TEXT,      5,  19, 5,  19, {},  0,                                L""},
		{DI_CHECKBOX,  6,  19, 0,  19, {},  0,                                L"Deactivate macro"/*L"Макрос неактивен"*//*Msg::MacroEditDeactivate*/},

		{DI_TEXT,      3,  20, 0,  20, {},  DIF_SEPARATOR,                    L""},
		{DI_BUTTON,    0,  21, 0,  21, {},  DIF_DEFAULT | DIF_CENTERGROUP,    Msg::Ok                                   },
		{DI_BUTTON,    0,  21, 0,  21, {},  DIF_CENTERGROUP,                  Msg::Cancel                               },
		{DI_BUTTON,    0,  21, 0,  21, {},  DIF_CENTERGROUP | DIF_BTNNOCLOSE, Msg::Reset                                }
	};

	std::vector<FarListItem> ComboListMLib_Items;
	std::vector<FARString> ComboListMLib_Descriptions;
	const int x_mlib = (int)(MacroLib_KeywordsFunctions2Items(this->Macro, ComboListMLib_Items, ComboListMLib_Descriptions) + 3);

	if (ScrX - DLG_WIDTH > 16)
	{ // для широкого экрана показываем библиотеку ключевых слов/функций более обзорно справа
		MacroEditDlgData[ME_COMBO_MLIB].X1 = DLG_WIDTH - 4;
		DLG_WIDTH = ScrX - 4;
		MacroEditDlgData[ME_DOUBLEBOX].X2 = DLG_WIDTH - 4;
		MacroEditDlgData[ME_COMBO_MLIB].X2 = DLG_WIDTH - 6;
		MacroEditDlgData[ME_COMBO_MLIB].Y1 = 2;
		MacroEditDlgData[ME_COMBO_MLIB].Y2 = DLG_HEIGHT - 5 - 4;
		MacroEditDlgData[ME_COMBO_MLIB].Type = DI_LISTBOX;
		MacroEditDlgData[ME_COMBO_MLIB].Flags|= DIF_LISTNOCLOSE;
		if (MacroEditDlgData[ME_COMBO_MLIB].X2 - MacroEditDlgData[ME_COMBO_MLIB].X1 > x_mlib) {
			MacroEditDlgData[ME_COMBO_MLIB].X1 = MacroEditDlgData[ME_COMBO_MLIB].X2 - x_mlib;
			MacroEditDlgData[ME_SINGLEBOX_SEQUENCE].X2 = MacroEditDlgData[ME_COMBO_MLIB].X1 - 1;
			MacroEditDlgData[ME_MEMOEDIT_SEQUENCE].X2 = MacroEditDlgData[ME_EDIT_DESCRIPTION].X2
				= MacroEditDlgData[ME_SINGLEBOX_SEQUENCE].X2 - 1;
		}
		MacroEditDlgData[ME_BUTTON_MLIB_TO_SEQUENCE].X2 = MacroEditDlgData[ME_MEMOEDIT_SEQUENCE].X2;
		MacroEditDlgData[ME_BUTTON_MLIB_TO_SEQUENCE].X1 = MacroEditDlgData[ME_BUTTON_MLIB_TO_SEQUENCE].X2 - 9; // 9 =strlen("[ ^^ << ]")
		MacroEditDlgData[ME_TEXT_SINGLEBOX_SEQUENCE_TIPS].Flags = 0; // clear DIF_HIDDEN

		MacroEditDlgData[ME_MEMOEDIT_MLIB_DESC].X1 = MacroEditDlgData[ME_COMBO_MLIB].X1;
		MacroEditDlgData[ME_MEMOEDIT_MLIB_DESC].X2 = MacroEditDlgData[ME_COMBO_MLIB].X2;
		MacroEditDlgData[ME_MEMOEDIT_MLIB_DESC].Y1 = MacroEditDlgData[ME_COMBO_MLIB].Y2 + 1;
		MacroEditDlgData[ME_MEMOEDIT_MLIB_DESC].Y2 = MacroEditDlgData[ME_COMBO_MLIB].Y2 + 4;
		MacroEditDlgData[ME_MEMOEDIT_MLIB_DESC].Flags = DIF_FOCUS | DIF_READONLY; // clear DIF_HIDDEN
	}
	MakeDialogItemsEx(MacroEditDlgData, MacroEditDlg);

	FarList ComboListMacroAreas;
	FarListItem ComboListMacroAreasItems[MACRO_LAST] = {};
	ComboListMacroAreas.ItemsNumber = ARRAYSIZE(ComboListMacroAreasItems);
	ComboListMacroAreas.Items = ComboListMacroAreasItems;
	for (int i = 0; i < MACRO_LAST; i++)
		ComboListMacroAreasItems[i].Text = Macro->GetSubKey(i);
	MacroEditDlg[ME_COMBO_AREA].ListItems = &ComboListMacroAreas;

	FarList ComboListMLib;
	ComboListMLib.ItemsNumber = ComboListMLib_Items.size();
	ComboListMLib.Items = ComboListMLib_Items.data();
	MacroEditDlg[ME_COMBO_MLIB].ListItems = &ComboListMLib;

	static const wchar_t kMemoSequenceFilename[] = L"Far2MacroSequence.macro";
	MacroEditDlg[ME_MEMOEDIT_SEQUENCE].UserData = (DWORD_PTR)kMemoSequenceFilename;
	static const wchar_t kMemoMLibFilename[] = L"Far2MacroLib.txt";
	MacroEditDlg[ME_MEMOEDIT_MLIB_DESC].UserData = (DWORD_PTR)kMemoMLibFilename;

	MacroEditDlgParam Param = {this, iarea, imacro, pstrArea, mr, strKeyName.CPtr(), &ComboListMLib_Descriptions};

	Dialog Dlg(MacroEditDlg, ARRAYSIZE(MacroEditDlg), MacroEditDlgProc, (LONG_PTR)&Param);
	Dlg.SetPosition(-1, -1, DLG_WIDTH, DLG_HEIGHT);
	Dlg.SetHelp(L"KeyMacroSetting"/*KeyMacroLang"*/);
	{
		LockBottomFrame LBF;	// временно отменим прорисовку фрейма
		Dlg.Process();
	}

	return (Dlg.GetExitCode() == ME_BUTTON_OK);
}

static FARString &GetKeyMacrosIniSmart(FARString &str, bool bQuote)
{
	str = InMyConfig("settings/key_macros.ini", false);
	const FARString &strHome = CachedHomeDir();
	if (str.Begins(strHome))
		str = L"~" + str.SubStr(strHome.GetLength());
	if (bQuote)
		InsertQuote(str);
	return str;
}

int MacroBrowser::View(const MacroRecord *mr, int iarea, int index)
{
	ExMessager em;
	if (mr != nullptr) {
		em.AddDup(L"Macro Browser - Details of Macro");
		FARString strKeyName;
		if (mr->Key == KEY_INVALID)
			strKeyName = L"INVALID";
		else
			KeyToText(mr->Key, strKeyName);

		em.AddFormat(L"       Area: %ls", Macro->GetSubKey(mr->Flags & MFLAGS_MODEMASK));
		em.AddFormat(L"        Key: %ls", strKeyName.CPtr());
		em.AddFormat(L"Description: %ls", mr->Description ? mr->Description : L"");
		em.AddFormat(L"   Sequence: %ls", mr->Src ? mr->Src : L"");
		em.Add(L"\x2 Flags ");
		em.AddFormat(L"[%c] DisableOutput      [%c] NoSendKeysToPlugins",
			mr->Flags & MFLAGS_DISABLEOUTPUT ? 'x' : ' ',
			mr->Flags & MFLAGS_NOSENDKEYSTOPLUGINS ? 'x' : ' ');
		em.AddFormat(L"[%c] RunAfterFARStarted [%c] RunAfterFARStart",
			mr->Flags & MFLAGS_RUNAFTERFARSTARTED ? 'x' : ' ',
			mr->Flags & MFLAGS_RUNAFTERFARSTART ? 'x' : ' ');
		em.AddFormat(L"[%c] EmptyCommandLine   [%c] NotEmptyCommandLine",
			mr->Flags & MFLAGS_EMPTYCOMMANDLINE ? 'x' : ' ',
			mr->Flags & MFLAGS_NOTEMPTYCOMMANDLINE ? 'x' : ' ');
		em.AddFormat(L"[%c] EVSelection        [%c] NoEVSelection",
			mr->Flags & MFLAGS_EDITSELECTION ? 'x' : ' ',
			mr->Flags & MFLAGS_EDITNOSELECTION ? 'x' : ' ');
		em.Add(L"\x1");
		em.AddFormat(L"[%c] Selection          [%c] PSelection",
			mr->Flags & MFLAGS_SELECTION ? 'x' : ' ',
			mr->Flags & MFLAGS_PSELECTION ? 'x' : ' ');
		em.AddFormat(L"[%c] NoSelection        [%c] NoPSelection",
			mr->Flags & MFLAGS_NOSELECTION ? 'x' : ' ',
			mr->Flags & MFLAGS_PNOSELECTION ? 'x' : ' ');
		em.AddFormat(L"[%c] NoFilePanels       [%c] NoFilePPanels",
			mr->Flags & MFLAGS_NOFILEPANELS ? 'x' : ' ',
			mr->Flags & MFLAGS_PNOFILEPANELS ? 'x' : ' ');
		em.AddFormat(L"[%c] NoPluginPanels     [%c] NoPluginPPanels",
			mr->Flags & MFLAGS_NOPLUGINPANELS ? 'x' : ' ',
			mr->Flags & MFLAGS_PNOPLUGINPANELS ? 'x' : ' ');
		em.AddFormat(L"[%c] NoFolders          [%c] NoPFolders",
			mr->Flags & MFLAGS_NOFOLDERS ? 'x' : ' ',
			mr->Flags & MFLAGS_PNOFOLDERS ? 'x' : ' ');
		em.AddFormat(L"[%c] NoFiles            [%c] NoPFiles",
			mr->Flags & MFLAGS_NOFILES ? 'x' : ' ',
			mr->Flags & MFLAGS_PNOFILES ? 'x' : ' ');
		em.Add(L"\x1");
		em.AddFormat(L"[%c] NeedSaveMacro      [%c] DisableMacro",
			mr->Flags & MFLAGS_NEEDSAVEMACRO ? 'x' : ' ',
			mr->Flags & MFLAGS_DISABLEMACRO ? 'x' : ' ');
		em.Add(L"\x2 Note ");
	}
	else if(iarea == MACRO_FUNCS && index >= 0) {
		const TMacroFunction *mf = Macro->GetMacroFunction((size_t)index);
		if (mf && mf->Name && mf->Name[0]) {
			em.AddDup(L"Macro Browser - Details of MacroFunction");
			em.AddFormat(L"MacroFunction Name: %ls", mf->Name);
			em.AddFormat(L"        Parameters: %d (all), %d (optional)", mf->nParam, mf->oParam);
			em.AddFormat(L"              GUID: %ls", mf->fnGUID ? mf->fnGUID : L"");
			em.AddFormat(L"            Syntax: %ls", mf->Syntax ? mf->Syntax : L"");
			em.AddFormat(L"              Type: %ls", MacroLib_GetFunctionType(mf));
			//em.AddFormat(L"            OpCode: %u", mf->Code);
			em.Add(L"\x2 Flags ");
			em.AddFormat(L"[%c] UnlockScreen      [%c] DisableIntInput",
				mf->IntFlags & IMFF_UNLOCKSCREEN ? 'x' : ' ',
				mf->IntFlags & IMFF_DISABLEINTINPUT ? 'x' : ' ');
			em.Add(L"\x2 Note ");
		}
	}
	else
		em.AddDup(L"Macro Browser - Details of Macro");
	em.Add(L"All far2l saved macros editable in ini-file:");
	FARString str;
	GetKeyMacrosIniSmart(str, true);
	em.AddDup(L"   " + str);
	em.Add(L"About FAR2 Macro Language see in Help (F1) and in Encyclopedia:");
	em.Add(L"   https://api.farmanager.com/ru/macro/macrocmd/");
	em.Add(L"Continue");
	SetMessageHelp(L"KeyMacro");
	return em.Show(MSG_LEFTALIGN, 1);
}

FARString MacroBrowser::TitleStr(bool b_hide_empty_areas)
{
	FARString title (Msg::MenuMacroBrowser);
	if (Macro->MacroLIBCount > 0 || Macro->GetCountMacroFunction() > 0) {
		title.AppendFormat(L" (%d total macros, %zu total macrofunctions)",
			Macro->MacroLIBCount, Macro->GetCountMacroFunction());
	}
	if (b_hide_empty_areas) {
		title+= L" *";
	}
	RemoveChar(title, L'&');
	return title;
}

void MacroBrowser::PrepareVMenu()
{
	fs2copy.Clear();
	ListMacro.DeleteItems();
	v_menu_macroindex.clear();

	FARString fs_prefix, fs;
	FARString strKeyName;
	MenuItemEx mi;
	mi.Flags = 0;

	int stat_areas_not_empty = 0,
		stat_macros_enabled = 0, stat_macros_disabled = 0,
		stat_need_save = 0,
		stat_macros_deleted = 0,
		stat_glbConsts = 0, stat_glbVars = 0;

	fs.Format(L"Total Macros in all areas: %d,  MacroFunctions: %zu",
		Macro->MacroLIBCount, Macro->GetCountMacroFunction());
	ListMacro.AddItem(fs);
	fs2copy += fs;
	v_menu_macroindex.emplace_back(-1, -1);
	if (Opt.Macro.DisableMacro & MDOL_ALL) {
		fs = L" (macros loading was disabled by the command line parameter \"-m\")";
		ListMacro.AddItem(fs);
		fs2copy += "\n" + fs;
		v_menu_macroindex.emplace_back(-1, -1);
	}
	if (Opt.Macro.DisableMacro & MDOL_AUTOSTART) {
		fs = L" (macros autostart was disabled by the command line parameter \"-ma\")";
		ListMacro.AddItem(fs);
		fs2copy += "\n" + fs;
		v_menu_macroindex.emplace_back(-1, -1);
	}

	for (int ia = 0; ia < MACRO_LAST; ia++) {
		const wchar_t *macro_area = Macro->GetSubKey(ia);

		if (Macro->IndexMode[ia][1] > 0)
			stat_areas_not_empty++;

		mi.strName.Format(L"%ls (%d macros)", macro_area, Macro->IndexMode[ia][1]);
		mi.Flags = (b_hide_empty_areas && Macro->IndexMode[ia][1] <= 0) ? LIF_SEPARATOR | LIF_HIDDEN : LIF_SEPARATOR;
		ListMacro.AddItem(&mi);
		fs2copy += "\n--- " + mi.strName + " ---";
		v_menu_macroindex.emplace_back(-1, -1);

		if (!Macro->MacroLIB)
			continue;

		mi.Flags = 0;
		for (int imacro = Macro->IndexMode[ia][0], iam = 0;
				iam < Macro->IndexMode[ia][1] && imacro < Macro->MacroLIBCount;
				iam++, imacro++) {
			mi.strName.Format(L"%14ls#%3d: ", macro_area, iam+1);
			mi.PrefixLen = mi.strName.GetLength()-1;
			if (Macro->MacroLIB[imacro].Key == KEY_INVALID)
				strKeyName = L"INVALID";
			else
				KeyToText(Macro->MacroLIB[imacro].Key, strKeyName);
			mi.strName.AppendFormat(L"%-25ls %c%s%c%lc %ls",
				strKeyName.CPtr(),
				((Macro->MacroLIB[imacro].Flags & MFLAGS_DISABLEMACRO)  ? '-' : '+'),
				((Macro->MacroLIB[imacro].Flags & MFLAGS_NEEDSAVEMACRO) ? "NS" : "  "),
				((!Macro->MacroLIB[imacro].BufferSize || !Macro->MacroLIB[imacro].Src) ? 'D' : ' '),
				BoxSymbols[BS_V1],
				Macro->MacroLIB[imacro].Description ? Macro->MacroLIB[imacro].Description : L"");
			mi.Flags = (!Macro->MacroLIB[imacro].BufferSize || !Macro->MacroLIB[imacro].Src) ? LIF_GRAYED : 0;
			ListMacro.AddItem(&mi);
			fs2copy += "\n" + mi.strName;
			v_menu_macroindex.emplace_back(ia, imacro);

			if (Macro->MacroLIB[imacro].Flags & MFLAGS_DISABLEMACRO)
				stat_macros_disabled++;
			else
				stat_macros_enabled++;
			if (Macro->MacroLIB[imacro].Flags & MFLAGS_NEEDSAVEMACRO)
				stat_need_save++;
			if (!Macro->MacroLIB[imacro].BufferSize || !Macro->MacroLIB[imacro].Src)
				stat_macros_deleted++;
		}
	}

	for (int ia = MACRO_CONSTS; ia <= MACRO_VARS; ia++) {
		mi.PrefixLen = 0;
		mi.strName = (ia == MACRO_VARS) ? L"Global Variables" : L"Global Constants";
		mi.Flags = LIF_SEPARATOR;
		ListMacro.AddItem(&mi);
		fs2copy += "\n--- " + mi.strName + " ---";
		v_menu_macroindex.emplace_back(-1, -1);

		TVarTable *t = (ia == MACRO_VARS) ? &glbVarTable : &glbConstTable;
		const wchar_t *macro_area = Macro->GetSubKey(ia);

		for (int I = 0, iacv = 0; I < V_TABLE_SIZE; I++) {
			for (int J = 0;; ++J) {
				TVarSet *var = varEnum(*t, I, J);
				if (!var)
					break;
				iacv++;
				if (ia == MACRO_VARS)
					stat_glbVars++;
				else
					stat_glbConsts++;
				mi.strName.Format(L"%14ls#%3d: ", macro_area, iacv);
				mi.PrefixLen = mi.strName.GetLength()-1;
				mi.strName.AppendFormat(L"%-25ls%lc ", var->str, BoxSymbols[BS_V1]);
				mi.Flags = 0;
				switch (var->value.type()) {
					case vtInteger:
						mi.strName.AppendFormat(L"Integer %lc %ls", BoxSymbols[BS_V1], var->value.s());
						break;
					case vtDouble:
						mi.strName.AppendFormat(L"Double  %lc %ls", BoxSymbols[BS_V1], var->value.s());
						break;
					case vtString:
						mi.strName.AppendFormat(L"String  %lc \"%ls\"", BoxSymbols[BS_V1], var->value.s());
						break;
					case vtUnknown:
					default:
						mi.strName.Append(L"Unknown");
						break;
				}
				ListMacro.AddItem(&mi);
				fs2copy += "\n" + mi.strName;
				v_menu_macroindex.emplace_back(-1, -1);
			}
		}
	}

	mi.PrefixLen = 0;
	mi.strName.Format(L"MacroFunctions (%zu items)", Macro->GetCountMacroFunction());
	mi.Flags = LIF_SEPARATOR;
	ListMacro.AddItem(&mi);
	fs2copy += "\n--- " + mi.strName + " ---";
	v_menu_macroindex.emplace_back(-1, -1);
	if (Macro->GetCountMacroFunction() > 0 ) {
		const wchar_t *macro_area = Macro->GetSubKey(MACRO_FUNCS);

		for (size_t imf = 0; imf < Macro->GetCountMacroFunction(); imf++) {
				mi.strName.Format(L"%14ls#%3zu: ", macro_area, imf+1);
				mi.PrefixLen = mi.strName.GetLength()-1;
				const TMacroFunction *mf = Macro->GetMacroFunction(imf);
				if (!mf || !mf->Name || !mf->Name[0])
					continue;
				mi.strName.AppendFormat(L"%-25ls%lc %-12ls %lc ",
					mf->Name, BoxSymbols[BS_V1], MacroLib_GetFunctionType(mf), BoxSymbols[BS_V1] );
				if (mf->Syntax && mf->Syntax[0])
					mi.strName+= mf->Syntax;
				mi.Flags = 0;
				ListMacro.AddItem(&mi);
				fs2copy += "\n" + mi.strName;
				v_menu_macroindex.emplace_back(MACRO_FUNCS, (int)imf);
		}
	}

	mi.PrefixLen = 0;
	mi.strName = L"Statistics";
	mi.Flags = LIF_SEPARATOR;
	ListMacro.AddItem(&mi);
	fs2copy += "\n--- " + mi.strName + " ---";
	v_menu_macroindex.emplace_back(-1, -1);

	mi.strName.Format(L"  Areas: all=%d, with macros=%d, without any macro=%d",
		MACRO_LAST, stat_areas_not_empty, MACRO_LAST-stat_areas_not_empty);
	mi.Flags = 0;
	ListMacro.AddItem(&mi);
	fs2copy += "\n" + mi.strName;
	v_menu_macroindex.emplace_back(-1, -1);

	mi.strName.Format(L" Macros: all=%3d, (+) enabled=%d, (-) disabled=%d,",
		Macro->MacroLIBCount, stat_macros_enabled, stat_macros_disabled);
	mi.Flags = 0;
	ListMacro.AddItem(&mi);
	fs2copy += "\n" + mi.strName;
	v_menu_macroindex.emplace_back(-1, -1);

	mi.strName.Format(L"                  (NS) need save=%d, (D) marked as deleted=%d",
		stat_need_save, stat_macros_deleted);
	mi.Flags = 0;
	ListMacro.AddItem(&mi);
	fs2copy += "\n" + mi.strName;
	v_menu_macroindex.emplace_back(-1, -1);

	mi.strName.Format(L" Global: Constants=%d, Variables=%d, MacroFunctions=%d",
		stat_glbConsts, stat_glbVars, Macro->CMacroFunction);
	mi.Flags = 0;
	ListMacro.AddItem(&mi);
	fs2copy += "\n" + mi.strName;
	v_menu_macroindex.emplace_back(-1, -1);
}

void MacroBrowser::Show(class KeyMacro *KMacro)
{
	Macro = KMacro;

	if (Macro->IsRecording()) {
		ExMessager em;
		em.AddDup(TitleStr());
		em.Add(L"Macro Browser does not open while recording a macro");
		em.Add(L"Continue");
		SetMessageHelp(L"KeyMacro");
		em.Show(MSG_WARNING, 1);
		return;
	}

	ListMacro.SetTitle(TitleStr(b_hide_empty_areas));
	ListMacro.SetFlags(VMENU_SHOWAMPERSAND | VMENU_IGNORE_SINGLECLICK);
	ListMacro.ClearFlags(VMENU_MOUSEREACTION);
	ListMacro.SetFlags(VMENU_WRAPMODE);
	ListMacro.SetHelp(L"KeyMacroList");
	ListMacro.SetBottomTitle(L"F1, ESC or F10, F3, Enter or F4, Ins, Del, +, -, *, Ctrl-R, Ctrl-S, Ctrl-C or Ctrl-Ins, Ctrl-H, Ctrl-Alt-F");

	v_menu_macroindex.reserve(3 + MACRO_LAST + Macro->MacroLIBCount);

	PrepareVMenu();

	ListMacro.SetPosition(-1, -1, 0, 0);
	ListMacro.Show();
	do {
		while (!ListMacro.Done()) {
			FarKey Key = ListMacro.ReadInput();
			switch (Key) {
				case KEY_CTRLC:
				case KEY_CTRLINS:
				case KEY_CTRLNUMPAD0:
					CopyToClipboard(fs2copy.CPtr());
					break;
				case KEY_CTRLH: {
						struct MenuItemEx *mip;
						if (b_hide_empty_areas) {
							b_hide_empty_areas = false;
							for (int i = 0; i < ListMacro.GetItemCount(); i++) {
								mip = ListMacro.GetItemPtr(i);
								mip->Flags &= ~LIF_HIDDEN;
							}
						}
						else {
							b_hide_empty_areas = true;
							for (int i = 0; i < ListMacro.GetItemCount(); i++) {
								mip = ListMacro.GetItemPtr(i);
								if (mip->Flags & LIF_SEPARATOR && mip->strName.Ends(L" (0 macros)"))
									mip->Flags |= LIF_HIDDEN;
							}
						}
					}
					ListMacro.SetTitle(TitleStr(b_hide_empty_areas));
					ListMacro.Show();
					break;
				case KEY_CTRLR:
					if (!Macro->IsExecuting() && !Macro->IsRecording()) {
						FARString str;
						if (!Message(0, 2, Msg::MenuMacroBrowser,
									L"Reload all macros from",
									GetKeyMacrosIniSmart(str, true),
									L"and lose unsaved ones?",
									//L"Перезагрузить все макросы с потерей несохраненных?"/*Msg::MacroReload*/,
									Msg::Ok, Msg::Cancel)) {
							Macro->LoadMacros(!Macro->IsExecuting());
							b_hide_empty_areas = false;
							PrepareVMenu();
							ListMacro.SetTitle(TitleStr(b_hide_empty_areas));
							ListMacro.Show();
						}
					}
					break;
				case KEY_CTRLS:
					if (!Macro->IsExecuting() && !Macro->IsRecording()) {
						FARString str;
						if (!Message(0, 2, Msg::MenuMacroBrowser,
									L"Save all macros to",
									GetKeyMacrosIniSmart(str, true) + L"?",
									//L"Сохранить все макросы?"/*Msg::MacroSaveAll*/,
									Msg::Ok, Msg::Cancel)) {
							Macro->SaveMacros();
							// на всякий случай прочитаем что сохранили
							Macro->LoadMacros(!Macro->IsExecuting());
							b_hide_empty_areas = false;
							PrepareVMenu();
							ListMacro.SetTitle(TitleStr(b_hide_empty_areas));
							ListMacro.Show();
						}
					}
					break;
				case KEY_F3: {
						int i = ListMacro.GetSelectPos();
						if (i >= 0 && i < (int) v_menu_macroindex.size()) {
							if ( Macro->MacroLIB
									&& v_menu_macroindex[i].first >= 0
									&& v_menu_macroindex[i].second >= 0
									&& v_menu_macroindex[i].second < Macro->MacroLIBCount) {
								View(&Macro->MacroLIB[v_menu_macroindex[i].second],
									v_menu_macroindex[i].first, v_menu_macroindex[i].second);
							}
							else
								View(nullptr, v_menu_macroindex[i].first, v_menu_macroindex[i].second);
						}
					}
					break;
				case KEY_ENTER:
				case KEY_NUMENTER:
				case KEY_F4:
					if (!Macro->IsExecuting() && !Macro->IsRecording()) {
						int i = ListMacro.GetSelectPos();
						if (i >= 0 && i < (int) v_menu_macroindex.size()
								&& Macro->MacroLIB
								&& v_menu_macroindex[i].first >= 0
								&& v_menu_macroindex[i].second >= 0
								&& v_menu_macroindex[i].second < Macro->MacroLIBCount) {
							if (Edit(v_menu_macroindex[i].second)) {
								b_hide_empty_areas = false;
								PrepareVMenu();
								ListMacro.SetTitle(TitleStr(b_hide_empty_areas));
								ListMacro.Show();
							}
						}
						else
							View(nullptr, v_menu_macroindex[i].first, v_menu_macroindex[i].second);
					}
					break;
				case KEY_NUMDEL:
				case KEY_DEL:
					if (!Macro->IsExecuting() && !Macro->IsRecording()) {
						FarListPos lpos;
						int i = ListMacro.GetSelectPos(&lpos);
						if (i >= 0 && i < (int) v_menu_macroindex.size()
								&& Macro->MacroLIB
								&& v_menu_macroindex[i].first >= 0
								&& v_menu_macroindex[i].second >= 0
								&& v_menu_macroindex[i].second < Macro->MacroLIBCount
								&& !Message(0, 2, Msg::MenuMacroBrowser,
										L"Mark macro as deleted?",
										//L"Пометить макрос как удаленный?"/*Msg::MacroMarkDeleted*/,
										Msg::Ok, Msg::Cancel)) {
							Macro->MacroDelete(v_menu_macroindex[i].second);
							PrepareVMenu();
							ListMacro.SetSelectPos(&lpos);
							ListMacro.Show();
						}
					}
					break;
				case KEY_INS:
				case KEY_NUMPAD0:
					if (!Macro->IsExecuting() && !Macro->IsRecording()) {
							if (Edit(-1)) {
								b_hide_empty_areas = false;
								PrepareVMenu();
								ListMacro.SetTitle(TitleStr(b_hide_empty_areas));
								ListMacro.Show();
							}
					}
					break;
				case KEY_SUBTRACT:
				case '-':
				case KEY_ADD:
				case '+':
				case KEY_SPACE:
				case KEY_MULTIPLY:
				case '*':
					if (!Macro->IsExecuting() && !Macro->IsRecording()) {
						bool ch = false;
						FarListPos lpos;
						int i = ListMacro.GetSelectPos(&lpos);
						if (i >= 0 && i < (int) v_menu_macroindex.size()
								&& Macro->MacroLIB
								&& v_menu_macroindex[i].first >= 0
								&& v_menu_macroindex[i].second >= 0
								&& v_menu_macroindex[i].second < Macro->MacroLIBCount
								// if marked as removed we can't change disabled status
								&& Macro->MacroLIB[v_menu_macroindex[i].second].BufferSize
								&& Macro->MacroLIB[v_menu_macroindex[i].second].Src) {
							MacroRecord *mr = &Macro->MacroLIB[v_menu_macroindex[i].second];
							// make macros DISABLED
							if ((Key == KEY_SUBTRACT || Key == '-')
									&& !(mr->Flags & MFLAGS_DISABLEMACRO))
								ch = true, mr->Flags|= MFLAGS_DISABLEMACRO;
							// make macros ENABLED
							else if ((Key == KEY_ADD || Key == '+')
									&& (mr->Flags & MFLAGS_DISABLEMACRO))
								ch = true, mr->Flags&= ~MFLAGS_DISABLEMACRO;
							// toggle ENABLED/DISABLED
							else if (Key == KEY_SPACE || Key == KEY_MULTIPLY || Key == '*') {
								if (mr->Flags & MFLAGS_DISABLEMACRO)
									ch = true, mr->Flags&= ~MFLAGS_DISABLEMACRO;
								else
									ch = true, mr->Flags|= MFLAGS_DISABLEMACRO;
							}
							if (ch) {
								mr->Flags|=  MFLAGS_NEEDSAVEMACRO;
								PrepareVMenu();
								ListMacro.SetSelectPos(&lpos);
								ListMacro.Show();
							}
						}
					}
					break;
				default:
					ListMacro.ProcessInput();
					continue;
			}
		}
		if (ListMacro.GetExitCode() < 0) // exit from loop only by ESC or F10 or click outside vmenu
			break;
		ListMacro.ClearDone(); // no close after select item by ENTER or mouse click
	} while(1);
}

void KeyMacro::MacroBrowser()
{
	class MacroBrowser mb;
	mb.Show(this);
}
