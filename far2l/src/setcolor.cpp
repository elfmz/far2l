/*
setcolor.cpp

Установка фаровских цветов
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
#include <wchar.h>

#include "setcolor.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "colors.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "ctrlobj.hpp"
#include "savescr.hpp"
#include "scrbuf.hpp"
#include "panel.hpp"
#include "chgmmode.hpp"
#include "interf.hpp"
#include "palette.hpp"
#include "config.hpp"

static void SetItemColors(MenuDataEx *Items, int *PaletteItems, int Size, int TypeSub);
void GetColor(int PaletteIndex);
static VMenu *MenuToRedraw1 = nullptr, *MenuToRedraw2 = nullptr, *MenuToRedraw3 = nullptr;

// 0,1 - dialog,warn List
// 2,3 - dialog,warn Combobox
static int ListPaletteItems[4][13] = {
	// Listbox
	{
		// normal
		COL_DIALOGLISTTEXT, COL_DIALOGLISTHIGHLIGHT, COL_DIALOGLISTSELECTEDTEXT,
		COL_DIALOGLISTSELECTEDHIGHLIGHT, COL_DIALOGLISTDISABLED, COL_DIALOGLISTBOX,
		COL_DIALOGLISTTITLE, COL_DIALOGLISTSCROLLBAR,
		COL_DIALOGLISTARROWS,				// Arrow
		COL_DIALOGLISTARROWSSELECTED,		// Выбранный - Arrow
		COL_DIALOGLISTARROWSDISABLED,		// Arrow disabled
		COL_DIALOGLISTGRAY,					// "серый"
		COL_DIALOGLISTSELECTEDGRAYTEXT,		// выбранный "серый"
	},
	{
		// warn
		COL_WARNDIALOGLISTTEXT, COL_WARNDIALOGLISTHIGHLIGHT, COL_WARNDIALOGLISTSELECTEDTEXT,
		COL_WARNDIALOGLISTSELECTEDHIGHLIGHT, COL_WARNDIALOGLISTDISABLED, COL_WARNDIALOGLISTBOX,
		COL_WARNDIALOGLISTTITLE, COL_WARNDIALOGLISTSCROLLBAR,
		COL_WARNDIALOGLISTARROWS,				// Arrow
		COL_WARNDIALOGLISTARROWSSELECTED,		// Выбранный - Arrow
		COL_WARNDIALOGLISTARROWSDISABLED,		// Arrow disabled
		COL_WARNDIALOGLISTGRAY,					// "серый"
		COL_WARNDIALOGLISTSELECTEDGRAYTEXT,		// выбранный "серый"
	},
	// Combobox
	{
		// normal
		COL_DIALOGCOMBOTEXT, COL_DIALOGCOMBOHIGHLIGHT, COL_DIALOGCOMBOSELECTEDTEXT,
		COL_DIALOGCOMBOSELECTEDHIGHLIGHT, COL_DIALOGCOMBODISABLED, COL_DIALOGCOMBOBOX,
		COL_DIALOGCOMBOTITLE, COL_DIALOGCOMBOSCROLLBAR,
		COL_DIALOGCOMBOARROWS,				// Arrow
		COL_DIALOGCOMBOARROWSSELECTED,		// Выбранный - Arrow
		COL_DIALOGCOMBOARROWSDISABLED,		// Arrow disabled
		COL_DIALOGCOMBOGRAY,				// "серый"
		COL_DIALOGCOMBOSELECTEDGRAYTEXT,	// выбранный "серый"
	},
	{
		// warn
		COL_WARNDIALOGCOMBOTEXT, COL_WARNDIALOGCOMBOHIGHLIGHT, COL_WARNDIALOGCOMBOSELECTEDTEXT,
		COL_WARNDIALOGCOMBOSELECTEDHIGHLIGHT, COL_WARNDIALOGCOMBODISABLED, COL_WARNDIALOGCOMBOBOX,
		COL_WARNDIALOGCOMBOTITLE, COL_WARNDIALOGCOMBOSCROLLBAR,
		COL_WARNDIALOGCOMBOARROWS,				// Arrow
		COL_WARNDIALOGCOMBOARROWSSELECTED,		// Выбранный - Arrow
		COL_WARNDIALOGCOMBOARROWSDISABLED,		// Arrow disabled
		COL_WARNDIALOGCOMBOGRAY,				// "серый"
		COL_WARNDIALOGCOMBOSELECTEDGRAYTEXT,	// выбранный "серый"
	},
};

void SetColors()
{
	MenuDataEx Groups[] = {
		{(const wchar_t *)Msg::SetColorPanel,       LIF_SELECTED,  0},
		{(const wchar_t *)Msg::SetColorDialog,      0,             0},
		{(const wchar_t *)Msg::SetColorWarning,     0,             0},
		{(const wchar_t *)Msg::SetColorMenu,        0,             0},
		{(const wchar_t *)Msg::SetColorHMenu,       0,             0},
		{(const wchar_t *)Msg::SetColorKeyBar,      0,             0},
		{(const wchar_t *)Msg::SetColorCommandLine, 0,             0},
		{(const wchar_t *)Msg::SetColorClock,       0,             0},
		{(const wchar_t *)Msg::SetColorViewer,      0,             0},
		{(const wchar_t *)Msg::SetColorEditor,      0,             0},
		{(const wchar_t *)Msg::SetColorHelp,        0,             0},
		{L"",                                       LIF_SEPARATOR, 0},
		{(const wchar_t *)Msg::SetDefaultColors,    0,             0},
		{(const wchar_t *)Msg::SetBW,               0,             0}
	};
	MenuDataEx PanelItems[] = {
		{(const wchar_t *)Msg::SetColorPanelNormal,          LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorPanelSelected,        0,            0},
		{(const wchar_t *)Msg::SetColorPanelHighlightedInfo, 0,            0},
		{(const wchar_t *)Msg::SetColorPanelDragging,        0,            0},
		{(const wchar_t *)Msg::SetColorPanelBox,             0,            0},
		{(const wchar_t *)Msg::SetColorPanelNormalCursor,    0,            0},
		{(const wchar_t *)Msg::SetColorPanelSelectedCursor,  0,            0},
		{(const wchar_t *)Msg::SetColorPanelNormalTitle,     0,            0},
		{(const wchar_t *)Msg::SetColorPanelSelectedTitle,   0,            0},
		{(const wchar_t *)Msg::SetColorPanelColumnTitle,     0,            0},
		{(const wchar_t *)Msg::SetColorPanelTotalInfo,       0,            0},
		{(const wchar_t *)Msg::SetColorPanelSelectedInfo,    0,            0},
		{(const wchar_t *)Msg::SetColorPanelScrollbar,       0,            0},
		{(const wchar_t *)Msg::SetColorPanelScreensNumber,   0,            0}
	};
	int PanelPaletteItems[] = {COL_PANELTEXT, COL_PANELSELECTEDTEXT, COL_PANELINFOTEXT, COL_PANELDRAGTEXT,
			COL_PANELBOX, COL_PANELCURSOR, COL_PANELSELECTEDCURSOR, COL_PANELTITLE, COL_PANELSELECTEDTITLE,
			COL_PANELCOLUMNTITLE, COL_PANELTOTALINFO, COL_PANELSELECTEDINFO, COL_PANELSCROLLBAR,
			COL_PANELSCREENSNUMBER};
	MenuDataEx DialogItems[] = {
		{(const wchar_t *)Msg::SetColorDialogNormal,                           LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorDialogHighlighted,                      0,            0},
		{(const wchar_t *)Msg::SetColorDialogDisabled,                         0,            0},
		{(const wchar_t *)Msg::SetColorDialogBox,                              0,            0},
		{(const wchar_t *)Msg::SetColorDialogBoxTitle,                         0,            0},
		{(const wchar_t *)Msg::SetColorDialogHighlightedBoxTitle,              0,            0},
		{(const wchar_t *)Msg::SetColorDialogTextInput,                        0,            0},
		{(const wchar_t *)Msg::SetColorDialogUnchangedTextInput,               0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedTextInput,                0,            0},
		{(const wchar_t *)Msg::SetColorDialogEditDisabled,                     0,            0},
		{(const wchar_t *)Msg::SetColorDialogButtons,                          0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedButtons,                  0,            0},
		{(const wchar_t *)Msg::SetColorDialogHighlightedButtons,               0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedHighlightedButtons,       0,            0},
		{(const wchar_t *)Msg::SetColorDialogDefaultButton,                    0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedDefaultButton,            0,            0},
		{(const wchar_t *)Msg::SetColorDialogHighlightedDefaultButton,         0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedHighlightedDefaultButton, 0,            0},
		{(const wchar_t *)Msg::SetColorDialogListBoxControl,                   0,            0},
		{(const wchar_t *)Msg::SetColorDialogComboBoxControl,                  0,            0}
	};
	int DialogPaletteItems[] = {
		COL_DIALOGTEXT,
		COL_DIALOGHIGHLIGHTTEXT,
		COL_DIALOGDISABLED,
		COL_DIALOGBOX,
		COL_DIALOGBOXTITLE,
		COL_DIALOGHIGHLIGHTBOXTITLE,
		COL_DIALOGEDIT,
		COL_DIALOGEDITUNCHANGED,
		COL_DIALOGEDITSELECTED,
		COL_DIALOGEDITDISABLED,
		COL_DIALOGBUTTON,
		COL_DIALOGSELECTEDBUTTON,
		COL_DIALOGHIGHLIGHTBUTTON,
		COL_DIALOGHIGHLIGHTSELECTEDBUTTON,
		COL_DIALOGDEFAULTBUTTON,
		COL_DIALOGSELECTEDDEFAULTBUTTON,
		COL_DIALOGHIGHLIGHTDEFAULTBUTTON,
		COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON,
		0,
		2,
	};
	MenuDataEx WarnDialogItems[] = {
		{(const wchar_t *)Msg::SetColorDialogNormal,                           LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorDialogHighlighted,                      0,            0},
		{(const wchar_t *)Msg::SetColorDialogDisabled,                         0,            0},
		{(const wchar_t *)Msg::SetColorDialogBox,                              0,            0},
		{(const wchar_t *)Msg::SetColorDialogBoxTitle,                         0,            0},
		{(const wchar_t *)Msg::SetColorDialogHighlightedBoxTitle,              0,            0},
		{(const wchar_t *)Msg::SetColorDialogTextInput,                        0,            0},
		{(const wchar_t *)Msg::SetColorDialogUnchangedTextInput,               0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedTextInput,                0,            0},
		{(const wchar_t *)Msg::SetColorDialogEditDisabled,                     0,            0},
		{(const wchar_t *)Msg::SetColorDialogButtons,                          0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedButtons,                  0,            0},
		{(const wchar_t *)Msg::SetColorDialogHighlightedButtons,               0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedHighlightedButtons,       0,            0},
		{(const wchar_t *)Msg::SetColorDialogDefaultButton,                    0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedDefaultButton,            0,            0},
		{(const wchar_t *)Msg::SetColorDialogHighlightedDefaultButton,         0,            0},
		{(const wchar_t *)Msg::SetColorDialogSelectedHighlightedDefaultButton, 0,            0},
		{(const wchar_t *)Msg::SetColorDialogListBoxControl,                   0,            0},
		{(const wchar_t *)Msg::SetColorDialogComboBoxControl,                  0,            0}
	};
	int WarnDialogPaletteItems[] = {
		COL_WARNDIALOGTEXT,
		COL_WARNDIALOGHIGHLIGHTTEXT,
		COL_WARNDIALOGDISABLED,
		COL_WARNDIALOGBOX,
		COL_WARNDIALOGBOXTITLE,
		COL_WARNDIALOGHIGHLIGHTBOXTITLE,
		COL_WARNDIALOGEDIT,
		COL_WARNDIALOGEDITUNCHANGED,
		COL_WARNDIALOGEDITSELECTED,
		COL_WARNDIALOGEDITDISABLED,
		COL_WARNDIALOGBUTTON,
		COL_WARNDIALOGSELECTEDBUTTON,
		COL_WARNDIALOGHIGHLIGHTBUTTON,
		COL_WARNDIALOGHIGHLIGHTSELECTEDBUTTON,
		COL_WARNDIALOGDEFAULTBUTTON,
		COL_WARNDIALOGSELECTEDDEFAULTBUTTON,
		COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON,
		COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON,
		1,
		3,
	};
	MenuDataEx MenuItems[] = {
		{(const wchar_t *)Msg::SetColorMenuNormal,              LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorMenuSelected,            0,            0},
		{(const wchar_t *)Msg::SetColorMenuHighlighted,         0,            0},
		{(const wchar_t *)Msg::SetColorMenuSelectedHighlighted, 0,            0},
		{(const wchar_t *)Msg::SetColorMenuDisabled,            0,            0},
		{(const wchar_t *)Msg::SetColorMenuBox,                 0,            0},
		{(const wchar_t *)Msg::SetColorMenuTitle,               0,            0},
		{(const wchar_t *)Msg::SetColorMenuScrollBar,           0,            0},
		{(const wchar_t *)Msg::SetColorMenuArrows,              0,            0},
		{(const wchar_t *)Msg::SetColorMenuArrowsSelected,      0,            0},
		{(const wchar_t *)Msg::SetColorMenuArrowsDisabled,      0,            0},
		{(const wchar_t *)Msg::SetColorMenuGrayed,              0,            0},
		{(const wchar_t *)Msg::SetColorMenuSelectedGrayed,      0,            0}
	};
	int MenuPaletteItems[] = {
		COL_MENUTEXT, COL_MENUSELECTEDTEXT, COL_MENUHIGHLIGHT, COL_MENUSELECTEDHIGHLIGHT,
		COL_MENUDISABLEDTEXT, COL_MENUBOX, COL_MENUTITLE, COL_MENUSCROLLBAR,
		COL_MENUARROWS,					// Arrow
		COL_MENUARROWSSELECTED,			// Выбранный - Arrow
		COL_MENUARROWSDISABLED,
		COL_MENUGRAYTEXT,				// "серый"
		COL_MENUSELECTEDGRAYTEXT,		// выбранный "серый"
	};
	MenuDataEx HMenuItems[] = {
		{(const wchar_t *)Msg::SetColorHMenuNormal,              LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorHMenuSelected,            0,            0},
		{(const wchar_t *)Msg::SetColorHMenuHighlighted,         0,            0},
		{(const wchar_t *)Msg::SetColorHMenuSelectedHighlighted, 0,            0}
	};
	int HMenuPaletteItems[] = {COL_HMENUTEXT, COL_HMENUSELECTEDTEXT, COL_HMENUHIGHLIGHT,
			COL_HMENUSELECTEDHIGHLIGHT};
	MenuDataEx KeyBarItems[] = {
		{(const wchar_t *)Msg::SetColorKeyBarNumbers,    LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorKeyBarNames,      0,            0},
		{(const wchar_t *)Msg::SetColorKeyBarBackground, 0,            0}
	};
	int KeyBarPaletteItems[] = {COL_KEYBARNUM, COL_KEYBARTEXT, COL_KEYBARBACKGROUND};
	MenuDataEx CommandLineItems[] = {
		{(const wchar_t *)Msg::SetColorCommandLineNormal,     LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorCommandLineSelected,   0,            0},
		{(const wchar_t *)Msg::SetColorCommandLinePrefix,     0,            0},
		{(const wchar_t *)Msg::SetColorCommandLineUserScreen, 0,            0}
	};
	int CommandLinePaletteItems[] = {COL_COMMANDLINE, COL_COMMANDLINESELECTED, COL_COMMANDLINEPREFIX,
			COL_COMMANDLINEUSERSCREEN};
	MenuDataEx ClockItems[] = {
		{(const wchar_t *)Msg::SetColorClockNormal,       LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorClockNormalEditor, 0,            0},
		{(const wchar_t *)Msg::SetColorClockNormalViewer, 0,            0}
	};
	int ClockPaletteItems[] = { COL_CLOCK, COL_EDITORCLOCK, COL_VIEWERCLOCK };
	MenuDataEx ViewerItems[] = {
		{(const wchar_t *)Msg::SetColorViewerNormal,    LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorViewerSelected,  0,            0},
		{(const wchar_t *)Msg::SetColorViewerStatus,    0,            0},
		{(const wchar_t *)Msg::SetColorViewerArrows,    0,            0},
		{(const wchar_t *)Msg::SetColorViewerScrollbar, 0,            0}
	};
	int ViewerPaletteItems[] = {COL_VIEWERTEXT, COL_VIEWERSELECTEDTEXT, COL_VIEWERSTATUS, COL_VIEWERARROWS,
			COL_VIEWERSCROLLBAR};
	MenuDataEx EditorItems[] = {
		{(const wchar_t *)Msg::SetColorEditorNormal,    LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorEditorSelected,  0,            0},
		{(const wchar_t *)Msg::SetColorEditorStatus,    0,            0},
		{(const wchar_t *)Msg::SetColorEditorScrollbar, 0,            0}
	};
	int EditorPaletteItems[] = {COL_EDITORTEXT, COL_EDITORSELECTEDTEXT, COL_EDITORSTATUS,
			COL_EDITORSCROLLBAR};
	MenuDataEx HelpItems[] = {
		{(const wchar_t *)Msg::SetColorHelpNormal,            LIF_SELECTED, 0},
		{(const wchar_t *)Msg::SetColorHelpHighlighted,       0,            0},
		{(const wchar_t *)Msg::SetColorHelpReference,         0,            0},
		{(const wchar_t *)Msg::SetColorHelpSelectedReference, 0,            0},
		{(const wchar_t *)Msg::SetColorHelpBox,               0,            0},
		{(const wchar_t *)Msg::SetColorHelpBoxTitle,          0,            0},
		{(const wchar_t *)Msg::SetColorHelpScrollbar,         0,            0}
	};
	int HelpPaletteItems[] = {COL_HELPTEXT, COL_HELPHIGHLIGHTTEXT, COL_HELPTOPIC, COL_HELPSELECTEDTOPIC,
			COL_HELPBOX, COL_HELPBOXTITLE, COL_HELPSCROLLBAR};
	{
		int GroupsCode;
		VMenu GroupsMenu(Msg::SetColorGroupsTitle, Groups, ARRAYSIZE(Groups), 0);
		MenuToRedraw1 = &GroupsMenu;

		for (;;) {
			GroupsMenu.SetPosition(2, 1, 0, 0);
			GroupsMenu.SetFlags(VMENU_WRAPMODE | VMENU_NOTCHANGE);
			GroupsMenu.ClearDone();
			GroupsMenu.Process();

			if ((GroupsCode = GroupsMenu.Modal::GetExitCode()) < 0)
				break;

			if (GroupsCode == 12) {
				//                   было sizeof(Palette)
				memcpy(Palette, DefaultPalette, SIZE_ARRAY_PALETTE);
				break;
			}

			if (GroupsCode == 13) {
				memcpy(Palette, BlackPalette, SIZE_ARRAY_PALETTE);
				break;
			}

			switch (GroupsCode) {
				case 0:
					SetItemColors(PanelItems, PanelPaletteItems, ARRAYSIZE(PanelItems), 0);
					break;
				case 1:
					SetItemColors(DialogItems, DialogPaletteItems, ARRAYSIZE(DialogItems), 1);
					break;
				case 2:
					SetItemColors(WarnDialogItems, WarnDialogPaletteItems, ARRAYSIZE(WarnDialogItems), 1);
					break;
				case 3:
					SetItemColors(MenuItems, MenuPaletteItems, ARRAYSIZE(MenuItems), 0);
					break;
				case 4:
					SetItemColors(HMenuItems, HMenuPaletteItems, ARRAYSIZE(HMenuItems), 0);
					break;
				case 5:
					SetItemColors(KeyBarItems, KeyBarPaletteItems, ARRAYSIZE(KeyBarItems), 0);
					break;
				case 6:
					SetItemColors(CommandLineItems, CommandLinePaletteItems, ARRAYSIZE(CommandLineItems), 0);
					break;
				case 7:
					SetItemColors(ClockItems, ClockPaletteItems, ARRAYSIZE(ClockItems), 0);
					break;
				case 8:
					SetItemColors(ViewerItems, ViewerPaletteItems, ARRAYSIZE(ViewerItems), 0);
					break;
				case 9:
					SetItemColors(EditorItems, EditorPaletteItems, ARRAYSIZE(EditorItems), 0);
					break;
				case 10:
					SetItemColors(HelpItems, HelpPaletteItems, ARRAYSIZE(HelpItems), 0);
					break;
			}
		}
	}
	CtrlObject->Cp()->SetScreenPosition();
	CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
	CtrlObject->Cp()->LeftPanel->Redraw();
	CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
	CtrlObject->Cp()->RightPanel->Redraw();
}

static void SetItemColors(MenuDataEx *Items, int *PaletteItems, int Size, int TypeSub)
{
	MenuDataEx ListItems[] = {
		{Msg::SetColorDialogListText,              LIF_SELECTED, 0},
		{Msg::SetColorDialogListHighLight,         0,            0},
		{Msg::SetColorDialogListSelectedText,      0,            0},
		{Msg::SetColorDialogListSelectedHighLight, 0,            0},
		{Msg::SetColorDialogListDisabled,          0,            0},
		{Msg::SetColorDialogListBox,               0,            0},
		{Msg::SetColorDialogListTitle,             0,            0},
		{Msg::SetColorDialogListScrollBar,         0,            0},
		{Msg::SetColorDialogListArrows,            0,            0},
		{Msg::SetColorDialogListArrowsSelected,    0,            0},
		{Msg::SetColorDialogListArrowsDisabled,    0,            0},
		{Msg::SetColorDialogListGrayed,            0,            0},
		{Msg::SetColorDialogSelectedListGrayed,    0,            0}
	};

	int ItemsCode;
	VMenu ItemsMenu(Msg::SetColorItemsTitle, Items, Size, 0);

	if (TypeSub == 2)
		MenuToRedraw3 = &ItemsMenu;
	else
		MenuToRedraw2 = &ItemsMenu;

	for (;;) {
		ItemsMenu.SetPosition(17 - (TypeSub == 2 ? 7 : 0), 5 + (TypeSub == 2 ? 2 : 0), 0, 0);
		ItemsMenu.SetFlags(VMENU_WRAPMODE | VMENU_NOTCHANGE);
		ItemsMenu.ClearDone();
		ItemsMenu.Process();

		if ((ItemsCode = ItemsMenu.Modal::GetExitCode()) < 0)
			break;

		// 0,1 - dialog,warn List
		// 2,3 - dialog,warn Combobox
		if (TypeSub == 1 && PaletteItems[ItemsCode] < 4) {
			SetItemColors(ListItems, ListPaletteItems[PaletteItems[ItemsCode]], ARRAYSIZE(ListItems), 2);
			MenuToRedraw3 = nullptr;
		} else
			GetColor(PaletteItems[ItemsCode]);
	}
}

void GetColor(int PaletteIndex)
{
	ChangeMacroMode chgMacroMode(MACRO_MENU);
	WORD NewColor = Palette[PaletteIndex - COL_FIRSTPALETTECOLOR];

	if (GetColorDialog(NewColor)) {
		Palette[PaletteIndex - COL_FIRSTPALETTECOLOR] = static_cast<BYTE>(NewColor);
		ScrBuf.Lock();	// отменяем всякую прорисовку
		CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
		CtrlObject->Cp()->LeftPanel->Redraw();
		CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
		CtrlObject->Cp()->RightPanel->Redraw();

		if (MenuToRedraw3)
			MenuToRedraw3->Hide();

		MenuToRedraw2->Hide();			// гасим
		MenuToRedraw1->Hide();
		FrameManager->RefreshFrame();	// рефрешим
		FrameManager->PluginCommit();	// коммитим.
		MenuToRedraw1->SetColors();
		MenuToRedraw1->Show();			// кажем
		MenuToRedraw2->SetColors();
		MenuToRedraw2->Show();

		if (MenuToRedraw3) {
			MenuToRedraw3->SetColors();
			MenuToRedraw3->Show();
		}

		if (Opt.Clock)
			ShowTime(1);

		ScrBuf.Unlock();				// разрешаем прорисовку
		FrameManager->PluginCommit();	// коммитим.
	}
}

//////////////////////////////////////
static wchar_t ColorDialogForeRGB[16], ColorDialogBackRGB[16];

static void UpdateRGBFromDialog(HANDLE hDlg)
{
	SendDlgMessage(hDlg, DM_GETTEXTPTR, 36, (LONG_PTR)ColorDialogForeRGB);
	SendDlgMessage(hDlg, DM_GETTEXTPTR, 38, (LONG_PTR)ColorDialogBackRGB);
}

static DWORD ReverseColorBytes(DWORD Color)
{
	return (Color & 0x00ff00) | ((Color >> 16) & 0xff) | ((Color & 0xff) << 16);
}

static DWORD ColorDialogForeRGBValue()
{
	return ReverseColorBytes(((DWORD)wcstoul(ColorDialogForeRGB, nullptr, 16)));
}

static DWORD ColorDialogBackRGBValue()
{
	return (ReverseColorBytes((DWORD)wcstoul(ColorDialogBackRGB, nullptr, 16)));
}

static DWORD64 ColorDialogForeRGBMask()
{
	return DWORD64(ColorDialogForeRGBValue()) << 16;
}

static DWORD64 ColorDialogBackRGBMask()
{
	return DWORD64(ColorDialogBackRGBValue()) << 40;
}

static void GetColorDlgProc_EnsureColorsAreInverted(SHORT x, SHORT y)
{
	CHAR_INFO ci{};
	SMALL_RECT Rect = {x, y, x, y};
	WINPORT(ReadConsoleOutput)(0, &ci, COORD{1, 1}, COORD{0, 0}, &Rect);
	if (ci.Attributes & COMMON_LVB_REVERSE_VIDEO)
		return;	// this cell is already tweaked during prev paint

	DWORD64 InvColors = COMMON_LVB_REVERSE_VIDEO;

	InvColors|= ((ci.Attributes & 0x0f) << 4) | ((ci.Attributes & 0xf0) >> 4);

	InvColors|= (ci.Attributes & (COMMON_LVB_UNDERSCORE | COMMON_LVB_STRIKEOUT));

	if (ci.Attributes & FOREGROUND_TRUECOLOR) {
		SET_RGB_BACK(InvColors, GET_RGB_FORE(ci.Attributes));
	}

	if (ci.Attributes & BACKGROUND_TRUECOLOR) {
		SET_RGB_FORE(InvColors, GET_RGB_BACK(ci.Attributes));
	}

	DWORD NumberOfAttrsWritten{};
	WINPORT(FillConsoleOutputAttribute) (0, InvColors, 1, COORD{x, y}, &NumberOfAttrsWritten);
}

static void GetColorDlgProc_OnDrawn(HANDLE hDlg)
{
	UpdateRGBFromDialog(hDlg);

	// Trick to fix #1392:
	// For foreground-colored boxes invert Fg&Bg colors and add COMMON_LVB_REVERSE_VIDEO attribute
	// this will put real colors on them if mapping of colors is different for Fg and Bg indexes

	// Ensure everything is on screen and will use console API then
	ScrBuf.Flush();

	SMALL_RECT DlgRect{};
	SendDlgMessage(hDlg, DM_GETDLGRECT, 0, (LONG_PTR)&DlgRect);
	SMALL_RECT ItemRect{};

	for (int ID = 2; ID <= 17; ++ID) {
		if (SendDlgMessage(hDlg, DM_GETITEMPOSITION, ID, (LONG_PTR)&ItemRect)) {
			ItemRect.Left+= DlgRect.Left;
			ItemRect.Right+= DlgRect.Left;
			ItemRect.Top+= DlgRect.Top;
			ItemRect.Bottom+= DlgRect.Top;
			for (auto x = ItemRect.Left; x <= ItemRect.Right; ++x) {
				for (auto y = ItemRect.Top; y <= ItemRect.Bottom; ++y) {
					GetColorDlgProc_EnsureColorsAreInverted(x, y);
				}
			}
		}
	}
}

static LONG_PTR GetColorDlgProc_CtlColorDlgItem(HANDLE hDlg, int ID, LONG_PTR DefaultColor, bool UseTrueColor)
{
	if (UseTrueColor) {
		DialogItemTrueColors ditc{};
		FarTrueColorFromRGB(ditc.Normal.Fore, ColorDialogForeRGBValue());
		FarTrueColorFromRGB(ditc.Normal.Back, ColorDialogBackRGBValue());
		SendDlgMessage(hDlg, DM_SETTRUECOLOR, ID, (LONG_PTR)&ditc);
	}

	int *CurColor = (int *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
	return (DefaultColor & 0xFFFFFF00U) | ((*CurColor) & 0xFF);
}

static LONG_PTR WINAPI GetColorDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_CTLCOLORDLGITEM:
			if (Param1 >= 41 && Param1 <= 43) {
				return GetColorDlgProc_CtlColorDlgItem(hDlg, Param1, Param2, Param1 >= 42);
			}
			break;

		case DN_BTNCLICK:

			if ( (Param1 >= 2 && Param1 <= 17) || (Param1 >= 19 && Param1 <= 34)) {
				int NewColor;
				int *CurColor = (int *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
				FarDialogItem *DlgItem =
						(FarDialogItem *)malloc(SendDlgMessage(hDlg, DM_GETDLGITEM, Param1, 0));
				SendDlgMessage(hDlg, DM_GETDLGITEM, Param1, (LONG_PTR)DlgItem);
				NewColor = *CurColor;

				if (Param1 <= 17)		// Fore
				{
					NewColor&= ~0x0F;
					NewColor|= (DlgItem->Flags & B_MASK) >> 4;
				} else // if (Param1 >= 19) // Back 
				{
					NewColor&= ~0xF0;
					NewColor|= DlgItem->Flags & B_MASK;
				}

				if (NewColor != *CurColor)
					*CurColor = NewColor;

				free(DlgItem);
				return TRUE;
			}
			break;

		case DN_DRAWDIALOGDONE:
			GetColorDlgProc_OnDrawn(hDlg);
			break;

		case DN_EDITCHANGE:
			if (Param1 == 36 || Param1 == 38) {
				GetColorDlgProc_OnDrawn(hDlg);
			}
			break;

		case DN_CLOSE:
			UpdateRGBFromDialog(hDlg);
			break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool GetColorDialogInner(bool bForFileFilter, DWORD64 &Color, bool bCentered)
{
	const wchar_t *HexMask = L"HHHHHH";
	swprintf(ColorDialogForeRGB, ARRAYSIZE(ColorDialogForeRGB), L"%06X", ReverseColorBytes((Color >> 16) & 0xffffff));
	swprintf(ColorDialogBackRGB, ARRAYSIZE(ColorDialogBackRGB), L"%06X", ReverseColorBytes((Color >> 40) & 0xffffff));

	DialogDataEx ColorDlgData[] = {
		/*   0 */ {DI_DOUBLEBOX, 3, 1, 39, 14, {}, 0, Msg::SetColorTitle},

		/*   1 */ {DI_SINGLEBOX, 5, 2, 20, 7, {}, 0, Msg::SetColorForeground},
		/*   2 */ {DI_RADIOBUTTON, 7, 3, 0, 3, {}, F_LIGHTGRAY | B_BLACK | DIF_GROUP | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*   3 */ {DI_RADIOBUTTON, 7, 4, 0, 4, {}, F_BLACK | B_RED | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*   4 */ {DI_RADIOBUTTON, 7, 5, 0, 5, {}, F_LIGHTGRAY | B_DARKGRAY | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*   5 */ {DI_RADIOBUTTON, 7, 6, 0, 6, {}, F_BLACK | B_LIGHTRED | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*   6 */ {DI_RADIOBUTTON, 10, 3, 0, 3, {}, F_LIGHTGRAY | B_BLUE | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*   7 */ {DI_RADIOBUTTON, 10, 4, 0, 4, {}, F_BLACK | B_MAGENTA | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*   8 */ {DI_RADIOBUTTON, 10, 5, 0, 5, {}, F_BLACK | B_LIGHTBLUE | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*   9 */ {DI_RADIOBUTTON, 10, 6, 0, 6, {}, F_BLACK | B_LIGHTMAGENTA | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  10 */ {DI_RADIOBUTTON, 13, 3, 0, 3, {}, F_BLACK | B_GREEN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  11 */ {DI_RADIOBUTTON, 13, 4, 0, 4, {}, F_BLACK | B_BROWN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  12 */ {DI_RADIOBUTTON, 13, 5, 0, 5, {}, F_BLACK | B_LIGHTGREEN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  13 */ {DI_RADIOBUTTON, 13, 6, 0, 6, {}, F_BLACK | B_YELLOW | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  14 */ {DI_RADIOBUTTON, 16, 3, 0, 3, {}, F_BLACK | B_CYAN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  15 */ {DI_RADIOBUTTON, 16, 4, 0, 4, {}, F_BLACK | B_LIGHTGRAY | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  16 */ {DI_RADIOBUTTON, 16, 5, 0, 5, {}, F_BLACK | B_LIGHTCYAN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  17 */ {DI_RADIOBUTTON, 16, 6, 0, 6, {}, F_BLACK | B_WHITE | DIF_SETCOLOR | DIF_MOVESELECT, L""},

		/*  18 */ {DI_SINGLEBOX, 22, 2, 37, 7, {}, 0, Msg::SetColorBackground},
		/*  19 */ {DI_RADIOBUTTON, 24, 3, 0, 3, {}, F_LIGHTGRAY | B_BLACK | DIF_GROUP | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  20 */ {DI_RADIOBUTTON, 24, 4, 0, 4, {}, F_BLACK | B_RED | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  21 */ {DI_RADIOBUTTON, 24, 5, 0, 5, {}, F_LIGHTGRAY | B_DARKGRAY | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  22 */ {DI_RADIOBUTTON, 24, 6, 0, 6, {}, F_BLACK | B_LIGHTRED | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  23 */ {DI_RADIOBUTTON, 27, 3, 0, 3, {}, F_LIGHTGRAY | B_BLUE | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  24 */ {DI_RADIOBUTTON, 27, 4, 0, 4, {}, F_BLACK | B_MAGENTA | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  25 */ {DI_RADIOBUTTON, 27, 5, 0, 5, {}, F_BLACK | B_LIGHTBLUE | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  26 */ {DI_RADIOBUTTON, 27, 6, 0, 6, {}, F_BLACK | B_LIGHTMAGENTA | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  27 */ {DI_RADIOBUTTON, 30, 3, 0, 3, {}, F_BLACK | B_GREEN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  28 */ {DI_RADIOBUTTON, 30, 4, 0, 4, {}, F_BLACK | B_BROWN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  29 */ {DI_RADIOBUTTON, 30, 5, 0, 5, {}, F_BLACK | B_LIGHTGREEN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  30 */ {DI_RADIOBUTTON, 30, 6, 0, 6, {}, F_BLACK | B_YELLOW | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  31 */ {DI_RADIOBUTTON, 33, 3, 0, 3, {}, F_BLACK | B_CYAN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  32 */ {DI_RADIOBUTTON, 33, 4, 0, 4, {}, F_BLACK | B_LIGHTGRAY | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  33 */ {DI_RADIOBUTTON, 33, 5, 0, 5, {}, F_BLACK | B_LIGHTCYAN | DIF_SETCOLOR | DIF_MOVESELECT, L""},
		/*  34 */ {DI_RADIOBUTTON, 33, 6, 0, 6, {}, F_BLACK | B_WHITE | DIF_SETCOLOR | DIF_MOVESELECT, L""},

		/*  35 */ {DI_TEXT,         7,   7,  11, 7,  {}, DIF_HIDDEN, L"RGB#:"},
		/*  36 */ {DI_FIXEDIT,     13,   7,  18, 7,  {(DWORD_PTR)HexMask}, DIF_HIDDEN | DIF_MASKEDIT, ColorDialogForeRGB},
		/*  37 */ {DI_TEXT,        24,   7,  28, 7,  {}, DIF_HIDDEN, L"RGB#:"},
		/*  38 */ {DI_FIXEDIT,     30,   7,  35, 7,  {(DWORD_PTR)HexMask}, DIF_HIDDEN | DIF_MASKEDIT, ColorDialogBackRGB},

		/*  39 */ {DI_CHECKBOX,  5, 9, 0, 9, {}, DIF_HIDDEN, Msg::SetColorForeTransparent},
		/*  40 */ {DI_CHECKBOX, 22, 9, 0, 9, {}, DIF_HIDDEN, Msg::SetColorBackTransparent},

		/*  41 */ {DI_TEXT,   5,   9, 37,  9, {}, DIF_SETCOLOR, Msg::SetColorSample},
		/*  42 */ {DI_TEXT,   5,  10, 37, 10, {}, DIF_SETCOLOR, Msg::SetColorSample},
		/*  43 */ {DI_TEXT,   5,  11, 37, 11, {}, DIF_SETCOLOR, Msg::SetColorSample},
		/*  44 */ {DI_TEXT,   0,  12,  0, 12, {}, DIF_SEPARATOR, L""},
		/*  45 */ {DI_BUTTON, 0,  13,  0, 13, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::SetColorSet},
		/*  46 */ {DI_BUTTON, 0,  13,  0, 13, {}, DIF_CENTERGROUP, Msg::SetColorCancel},
	};
	MakeDialogItemsEx(ColorDlgData, ColorDlg);
	int ExitCode;
	WORD CurColor = Color;

	for (size_t i = 2; i <= 17; i++) {
		if (static_cast<WORD>((ColorDlg[i].Flags & B_MASK) >> 4) == (Color & F_MASK)) {
			ColorDlg[i].Selected = 1;
			ColorDlg[i].Focus = TRUE;
			break;
		}
	}

	for (size_t i = 19; i <= 34; i++) {
		if (static_cast<WORD>(ColorDlg[i].Flags & B_MASK) == (Color & B_MASK)) {
			ColorDlg[i].Selected = 1;
			break;
		}
	}

	for (size_t i = 41; i <= 43; i++) {
		ColorDlg[i].Flags = (ColorDlg[i].Flags & ~DIF_COLORMASK) | (Color & 0xffff);
	}

	if (bForFileFilter) {
		ColorDlg[0].Y2+= 2;
		ColorDlg[1].Y2++;
		ColorDlg[18].Y2++;

		for (size_t i = 35; i <= 40; ++i) {
			ColorDlg[i].Flags&= ~DIF_HIDDEN;
		}
		for (size_t i = 41; i <= 46; i++) {
			ColorDlg[i].Y1+= 2;
			ColorDlg[i].Y2+= 2;
		}
		ColorDlg[39].Selected = (Color & 0x0F00 ? 1 : 0);
		ColorDlg[40].Selected = (Color & 0xF000 ? 1 : 0);
	}

	{
		Dialog Dlg(ColorDlg, ARRAYSIZE(ColorDlg), GetColorDlgProc, (LONG_PTR)&CurColor);

		if (bCentered)
			Dlg.SetPosition(-1, -1, 39 + 4, 15 + (bForFileFilter ? 3 : 2));
		else
			Dlg.SetPosition(37, 2, 75 + 4, 16 + (bForFileFilter ? 3 : 2));

		Dlg.Process();
		ExitCode = Dlg.GetExitCode();
	}

	if (ExitCode == 45) {
		Color = CurColor & 0xffff;

		if (ColorDlg[39].Selected)
			Color|= 0x0F00;
		else
			Color&= 0xF0FF;

		if (ColorDlg[40].Selected)
			Color|= 0xF000;
		else
			Color&= 0x0FFF;

		Color|= ColorDialogForeRGBMask();
		Color|= ColorDialogBackRGBMask();

		return true;
	}

	return false;
}

bool GetColorDialogForFileFilter(DWORD64 &Color)
{
	return GetColorDialogInner(true, Color, true);
}

bool GetColorDialog(WORD &Color, bool bCentered)
{
	DWORD64 ColorRGB = Color;
	bool out = GetColorDialogInner(false, ColorRGB, bCentered);
	Color = ColorRGB & 0xffff;
	return out;
}

