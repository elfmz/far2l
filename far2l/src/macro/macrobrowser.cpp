/*
macrobrowser.cpp

Просмотр списка клавиатурных макросов
*/
/*
Copyright (c) 2026- Far2l People
*/

#include "headers.hpp"
#include "macro.hpp"
#include "keyboard.hpp"
#include "interf.hpp" // for RemoveChar()
#include "clipboard.hpp" // for CopyToClipboard()
#include "config.hpp"
#include "vmenu.hpp"
#include "message.hpp" // for ExMessager
#include "CachedCreds.hpp" // for CachedHomeDir()

class MacroBrowser
{
private:
	class KeyMacro *Macro;
	bool b_hide_empty_areas = false;
	FARString fs2copy;
	VMenu ListMacro;
	std::vector< std::pair<int,int> > v_menu_macroindex;

	int View(const wchar_t *area, MacroRecord *mr);
	void PrepareVMenu();

public:
	MacroBrowser()
		:
		Macro(nullptr),
		b_hide_empty_areas(false),
		ListMacro(nullptr, nullptr, 0, 0),
		v_menu_macroindex {}
		{ };

	void Show(class KeyMacro *KMacro);
};

int MacroBrowser::View(const wchar_t *area, MacroRecord *mr)
{
	ExMessager em(L"Macro Browser - Details of Macro");
	if (area != nullptr && mr != nullptr) {
		FARString strKeyName;
		KeyToText(mr->Key, strKeyName);

		em.AddFormat(L"       Area: %ls", area);
		em.AddFormat(L"        Key: %ls", strKeyName.CPtr());
		em.AddFormat(L"Description: %ls", mr->Description);
		em.AddFormat(L"   Sequence: %ls", mr->Src);
		em.Add(L"\x1 Flags ");
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
		em.AddFormat(L"[%c] NeedSaveMacro      [%c] DisableMacro",
			mr->Flags & MFLAGS_NEEDSAVEMACRO ? 'x' : ' ',
			mr->Flags & MFLAGS_DISABLEMACRO ? 'x' : ' ');
		em.Add(L"\x1 Note ");
	}
	em.Add(L"All far2l saved macros editable in ini-file:");
	FARString str = InMyConfig("settings/key_macros.ini", false);
	const FARString &strHome = CachedHomeDir();
	if (str.Begins(strHome))
		str = L"~" + str.SubStr(strHome.GetLength());
	em.AddDup(L"   " + str);
	em.Add(L"About FAR2 Macro Language see in Help (F1) and in Encyclopedia:");
	em.Add(L"   https://api.farmanager.com/ru/macro/macrocmd/");
	em.Add(L"Continue");
	SetMessageHelp(L"KeyMacro");
	return em.Show(MSG_LEFTALIGN, 1);
}

static FARString MacroBrowser_Title(bool b_hide_empty_areas = false)
{
	FARString title (Msg::MenuMacroBrowser);
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

	fs.Format(L"Total macros in all areas: %d", Macro->MacroLIBCount);
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

		mi.strName.Format(L"%ls (%d macros)", macro_area, Macro->IndexMode[ia][1]);
		mi.Flags = (b_hide_empty_areas && Macro->IndexMode[ia][1] <= 0) ? LIF_SEPARATOR | LIF_HIDDEN : LIF_SEPARATOR;
		ListMacro.AddItem(&mi);
		fs2copy += "\n--- " + mi.strName + " ---";
		v_menu_macroindex.emplace_back(-1, -1);

		mi.Flags = 0;
		for (int iai = Macro->IndexMode[ia][0], iam = 0;
				iam < Macro->IndexMode[ia][1];
				iam++, iai++ ) {
			fs_prefix.Format(L"%14ls#%3d: ", macro_area, iam+1);
			mi.PrefixLen = fs_prefix.GetLength()-1;
			KeyToText(Macro->MacroLIB[iai].Key, strKeyName);
			fs.Format(L"%-25ls %lc %ls",
				strKeyName.CPtr(), BoxSymbols[BS_V1], Macro->MacroLIB[iai].Description);
			mi.strName = fs_prefix + fs;
			//mi.Flags = Macro->MacroLIB[iai].Flags & MFLAGS_DISABLEMACRO ? 0 : LIF_CHECKED;
			ListMacro.AddItem(&mi);
			fs2copy += "\n" + mi.strName;
			v_menu_macroindex.emplace_back(ia, iai);
		}
	}
}

void MacroBrowser::Show(class KeyMacro *KMacro)
{
	Macro = KMacro;

	if (Macro->IsRecording()) {
		ExMessager em;
		em.AddDup(MacroBrowser_Title());
		em.Add(L"Macro Browser does not open while recording a macro");
		em.Add(L"Continue");
		SetMessageHelp(L"KeyMacro");
		em.Show(MSG_WARNING, 1);
		return;
	}

	ListMacro.SetTitle(MacroBrowser_Title(b_hide_empty_areas));
	ListMacro.SetFlags(VMENU_SHOWAMPERSAND | VMENU_IGNORE_SINGLECLICK);
	ListMacro.ClearFlags(VMENU_MOUSEREACTION);
	ListMacro.SetFlags(VMENU_WRAPMODE);
	ListMacro.SetHelp(L"KeyMacroList");
	ListMacro.SetBottomTitle(L"F1, ESC or F10, Enter or F3, Ctrl-C or Ctrl-Ins, Ctrl-H, Ctrl-Alt-F");

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
					ListMacro.SetTitle(MacroBrowser_Title(b_hide_empty_areas));
					ListMacro.Show();
					break;
				case KEY_ENTER:
				case KEY_NUMENTER:
				case KEY_F3: {
						int i = ListMacro.GetSelectPos();
						if (i >= 0 && i < (int) v_menu_macroindex.size()) {
							if ( v_menu_macroindex[i].first >=0
									&& v_menu_macroindex[i].first < MACRO_LAST
									&& v_menu_macroindex[i].second >= 0
									&& v_menu_macroindex[i].second < Macro->MacroLIBCount) {
								View(Macro->GetSubKey(v_menu_macroindex[i].first),
									&Macro->MacroLIB[v_menu_macroindex[i].second]);
							}
							else
								View(nullptr, nullptr);
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
		// by ENTER - show edit dialog
		// TO DO .... edit macro dialog
	} while(1);
}

void KeyMacro::MacroBrowser()
{
	class MacroBrowser mb;
	mb.Show(this);
}