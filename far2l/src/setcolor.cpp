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
	uint16_t NewColor = Palette[PaletteIndex - COL_FIRSTPALETTECOLOR];

	if (GetColorDialog16(&NewColor)) {
		Palette[PaletteIndex - COL_FIRSTPALETTECOLOR] = static_cast<uint16_t>(NewColor);
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

enum enumColorPanelElements
{
	ID_CP_CHECKBOX = 0,
	ID_CP_TEXT,
	ID_CP_COLORS_RECT,
	ID_CP_CHECKBOX_RGB,
	ID_CP_EDIT_RGB,
	ID_CP_BUTTON_256,
	ID_CP_BUTTON_RGB,
	ID_CP_RGB_SAMPLE,
	ID_CP_RGB_PREFIX,
	ID_CP_TOTAL
};

enum enumSetColorDialog
{
	ID_ST_TITLE = 0,

	ID_ST_SEPARATOR,
	ID_ST_SEPARATOR2,
	ID_ST_SEPARATOR3,
	ID_ST_SEPARATOR4,
	ID_ST_SEPARATOR5,

	ID_CP_FIRST,
	/// enumColorPanelElements foreground
	ID_ST_CHECKBOX_FOREGROUND = ID_CP_FIRST,
	ID_ST_TEXT_FOREGROUND,
	ID_ST_FG_COLORS_RECT,
	ID_ST_CHECKBOX_RGB1,
	ID_ST_EDIT_FORERGB,
	ID_ST_BUTTON_F256,
	ID_ST_BUTTON_FRGB,
	ID_ST_FRGB_SMAPLE,
	ID_ST_FRGB_PREFIX,

	/// enumColorPanelElements background
	ID_ST_CHECKBOX_BACKGROUND,
	ID_ST_TEXT_BACKGROUND,
	ID_ST_BG_COLORS_RECT,
	ID_ST_CHECKBOX_RGB2,
	ID_ST_EDIT_BACKRGB,
	ID_ST_BUTTON_B256,
	ID_ST_BUTTON_BRGB,
	ID_ST_BRGB_SMAPLE,
	ID_ST_BRGB_PREFIX,

	/// text style checkboxes
	ID_ST_TEXT_STYLE,
	ID_ST_CHECKBOX_STYLE_ENABLE,
	ID_ST_CHECKBOX_STYLE_FIRST,
	ID_ST_CHECKBOX_STYLE_BOLD = ID_ST_CHECKBOX_STYLE_FIRST,
	ID_ST_CHECKBOX_STYLE_ITALIC,
	ID_ST_CHECKBOX_STYLE_OVERLINE,
	ID_ST_CHECKBOX_STYLE_STRIKEOUT,
	ID_ST_CHECKBOX_STYLE_UNDERLINE,
	ID_ST_CHECKBOX_STYLE_BLINK,
	ID_ST_CHECKBOX_STYLE_INVERSE,
	ID_ST_CHECKBOX_STYLE_LAST = ID_ST_CHECKBOX_STYLE_INVERSE,

	ID_ST_COLOREXAMPLE,

	ID_ST_BUTTON_SETCOLOR,
	ID_ST_BUTTON_RESTORE,
	ID_ST_BUTTON_CANCEL
};

typedef struct st_font_style_bind_s {
	int32_t	id;
	int64_t	flags;
} st_font_style_bind_t;

static st_font_style_bind_t sup_styles[] = {

	{ID_ST_CHECKBOX_STYLE_STRIKEOUT, COMMON_LVB_STRIKEOUT},
	{ID_ST_CHECKBOX_STYLE_UNDERLINE, COMMON_LVB_UNDERSCORE},
	{ID_ST_CHECKBOX_STYLE_INVERSE,   COMMON_LVB_REVERSE_VIDEO},
};

#define ST_ALL_FONT_STYLES (\
							  COMMON_LVB_STRIKEOUT\
							| COMMON_LVB_UNDERSCORE\
							| COMMON_LVB_REVERSE_VIDEO\
)

struct color_panel_s
{
	CHAR_INFO vbuff[64];
	CHAR_INFO vbuff_rgb[8];
	uint64_t color;		// rgb
	uint32_t id;
	uint32_t offset;
	uint32_t index;		// foreground or background index 0 - 15
	bool bTransparent;
	bool bRGB;
	wchar_t wsRGB[16];

	void update_index(int32_t newindex) {
		newindex &= 15;
		vbuff[index * 3 + 1].Char.UnicodeChar = 32;
		vbuff[newindex * 3 + 1].Char.UnicodeChar = L'\x2022'; // DOT
		index = newindex;
	}

	void update_rgb_color_from_str( ) {
		uint32_t rgb = wcstoul(wsRGB, nullptr, 16);
		color = (rgb & 0x00FF00) | ((rgb >> 16) & 0xFF) | ((rgb & 0xFF) << 16);
		draw_rgb_sample( );
	}

	void update_str_from_rgb_color( ) {
		uint32_t rgb = (color & 0x00FF00) | ((color >> 16) & 0xFF) | ((color & 0xFF) << 16);
		swprintf(wsRGB, 16, L"%06X", rgb);
	}

	void draw_rgb_sample( ) {
		const uint64_t attr = ((color << 40) | BACKGROUND_TRUECOLOR) + (((~(color) & 0xFFFFFF) << 16) | FOREGROUND_TRUECOLOR) + 7;

		vbuff_rgb[0].Char.UnicodeChar = 32;
		vbuff_rgb[0].Attributes = attr;
		vbuff_rgb[1].Char.UnicodeChar = L'\x2022'; // DOT
		vbuff_rgb[1].Attributes = attr;
		vbuff_rgb[2].Char.UnicodeChar = 32;
		vbuff_rgb[2].Attributes = attr;
	}

	intptr_t WINAPI ColorPanelUserProc(HANDLE hDlg, int Msg, int Param1, intptr_t Param2);
};

struct set_color_s
{
	enum {
		IDC_FOREGROUND_PANEL = 0,
		IDC_BACKGROUND_PANEL = 1
	};
	color_panel_s cPanel[2];

	CHAR_INFO samplevbuff[132];
	uint64_t color;
	uint64_t style;
	uint64_t style_inherit;
	uint64_t smpcolor;
	uint64_t mask;
	uint64_t resetcolor;
	uint64_t resetmask;
	uint64_t flags;
	bool bTransparencyEnabled;
	bool bRGBEnabled;
	bool bStyleEnabled;
	bool bStyle;

	set_color_s() {

		memset(this, 0, sizeof(set_color_s));

		draw_panels_vbuff( );

		cPanel[IDC_FOREGROUND_PANEL].id = IDC_FOREGROUND_PANEL;
		cPanel[IDC_FOREGROUND_PANEL].offset = ID_CP_FIRST;
		cPanel[IDC_BACKGROUND_PANEL].id = IDC_BACKGROUND_PANEL;
		cPanel[IDC_BACKGROUND_PANEL].offset = ID_CP_FIRST + ID_CP_TOTAL;
	}

	inline void enable_RGB(const bool bEnable) {
		bRGBEnabled = bEnable;
	}

	inline void enable_transparency(const bool bEnable) {
		bTransparencyEnabled = bEnable;
	}

	inline void enable_font_styles(const bool bEnable) {
		bStyleEnabled = bEnable;
	}

	void update_panel_indexes(void)
	{
		cPanel[IDC_FOREGROUND_PANEL].update_index(color & 0xF);
		cPanel[IDC_BACKGROUND_PANEL].update_index((color & 0xFF) >> 4);
	}

	void set_color(const uint64_t color)
	{
		this->color = resetcolor = color;
		mask = 0xFFFFFFFFFFFFFFFF;
		flags = (color & 0x000000000000FF00) >> 8;

		cPanel[IDC_FOREGROUND_PANEL].bTransparent = false;
		cPanel[IDC_BACKGROUND_PANEL].bTransparent = false;

		update_panel_indexes();

		if (bRGBEnabled) {
			cPanel[IDC_FOREGROUND_PANEL].color = (color >> 16) & 0xFFFFFF;
			cPanel[IDC_BACKGROUND_PANEL].color = (color >> 40);

			if (color & 0x000000FFFFFF0000 || color & FOREGROUND_TRUECOLOR) {
				cPanel[IDC_FOREGROUND_PANEL].bRGB = true;
			}

			if (color & 0xFFFFFF0000000000 || color & BACKGROUND_TRUECOLOR) {
				cPanel[IDC_BACKGROUND_PANEL].bRGB = true;
			}

			cPanel[IDC_FOREGROUND_PANEL].update_str_from_rgb_color( );
			cPanel[IDC_BACKGROUND_PANEL].update_str_from_rgb_color( );

			cPanel[IDC_FOREGROUND_PANEL].draw_rgb_sample( );
			cPanel[IDC_BACKGROUND_PANEL].draw_rgb_sample( );
		}

		style = style_inherit = 0;

		if (bStyleEnabled) {
			style = color & ST_ALL_FONT_STYLES;
			if (style)
				bStyle = true;
		}
	}

	void set_mask(const uint64_t mask)
	{
		this->mask = resetmask = mask;

		if (bTransparencyEnabled) {
			if (!(mask & 0xF))
				cPanel[IDC_FOREGROUND_PANEL].bTransparent = true;

			if (!(mask & 0xF0))
				cPanel[IDC_BACKGROUND_PANEL].bTransparent = true;
		}

		style_inherit = 0;

		if (bStyleEnabled) {
			style_inherit = (~mask) & ST_ALL_FONT_STYLES;
			style_inherit &= ~style;

			if (style_inherit)
				bStyle = true;
		}
	}

	void reset_color(void)
	{
		set_color(resetcolor);
		if (bTransparencyEnabled) {
			set_mask(resetmask);
		}
		update_color( );
	}

	void update_color(void)
	{
		color = 0;
		color |= cPanel[IDC_FOREGROUND_PANEL].index;
		color |= cPanel[IDC_BACKGROUND_PANEL].index << 4;

		if (bRGBEnabled) {
			if (cPanel[IDC_FOREGROUND_PANEL].bRGB && cPanel[IDC_FOREGROUND_PANEL].color) {
				color |= (cPanel[IDC_FOREGROUND_PANEL].color << 16) & 0x000000FFFFFF0000;
				color |= FOREGROUND_TRUECOLOR;
			}

			if (cPanel[IDC_BACKGROUND_PANEL].bRGB && cPanel[IDC_BACKGROUND_PANEL].color) {
				color |= cPanel[IDC_BACKGROUND_PANEL].color << 40;
				color |= BACKGROUND_TRUECOLOR;
			}
		}

		mask = 0xFFFFFFFFFFFFFFFF;

		if (bStyleEnabled && bStyle)
			color |= style;

		smpcolor = color;

		if (bTransparencyEnabled) {

			if (cPanel[IDC_FOREGROUND_PANEL].bTransparent) {
				mask ^= (0x000000FFFFFF000F | FOREGROUND_TRUECOLOR);
				smpcolor &= (0xFFFFFF00000000F0 | BACKGROUND_TRUECOLOR);
			}

			if (cPanel[IDC_BACKGROUND_PANEL].bTransparent) {
				mask ^= (0xFFFFFF00000000F0 | BACKGROUND_TRUECOLOR);
				smpcolor &= (0x000000FFFFFF000F | FOREGROUND_TRUECOLOR);
			}

			if (bStyle)
				mask ^= style_inherit;
		}

		draw_sample_vbuff();
	}

	void draw_sample_vbuff(void);
	void draw_panels_vbuff(void);
};

void set_color_s::draw_sample_vbuff(void)
{
	static const wchar_t *sample_text_str = L"Text Text Text Text Text Text Text Text Text Text";

	for (int i = 0; i < 4; i++) {
		const uint64_t attr = (i > 0) ? smpcolor : smpcolor & 0xFF;

		for(int g = 0; g < 33; g++) {
			CHAR_INFO *const vbuff = samplevbuff + i * 33;

			vbuff[g].Char.UnicodeChar = sample_text_str[g];
			vbuff[g].Attributes = attr;
		}
	}
}

void set_color_s::draw_panels_vbuff(void)
{
	static const uint16_t dotfc[16] = { 15, 15, 15, 0, 0, 0, 0, 0,		// foreground dot colors
										15,  0,  0, 0, 0, 0, 0, 0 };

	// For foreground-colored boxes invert Fg&Bg colors and add COMMON_LVB_REVERSE_VIDEO attribute
	// this will put real colors on them if mapping of colors is different for Fg and Bg indexes
	// 0xBF

	// Fill foreground colors rect
	for (int i = 0; i < 16; i++) {
		CHAR_INFO *const vbuff = cPanel[IDC_FOREGROUND_PANEL].vbuff;
		const uint16_t attr = ((dotfc[i] << 4) + i) | COMMON_LVB_REVERSE_VIDEO;

		vbuff[i * 3 + 0].Char.UnicodeChar = 32;
		vbuff[i * 3 + 0].Attributes = attr;
		vbuff[i * 3 + 1].Char.UnicodeChar = 32;
		vbuff[i * 3 + 1].Attributes = attr;
		vbuff[i * 3 + 2].Char.UnicodeChar = 32;
		vbuff[i * 3 + 2].Attributes = attr;
	}

	// Fill background colors rect
	for (int i = 0; i < 16; i++) {
		CHAR_INFO *const vbuff = cPanel[IDC_BACKGROUND_PANEL].vbuff;
		const uint16_t attr = (i << 4) + dotfc[i];

		vbuff[i * 3 + 0].Char.UnicodeChar = 32;
		vbuff[i * 3 + 0].Attributes = attr;
		vbuff[i * 3 + 1].Char.UnicodeChar = 32;
		vbuff[i * 3 + 1].Attributes = attr;
		vbuff[i * 3 + 2].Char.UnicodeChar = 32;
		vbuff[i * 3 + 2].Attributes = attr;
	}
}

#define DM_UPDATECURSOR (DM_USER + 1)
#define DM_UPDATECOLORCODE (DM_USER + 2)

intptr_t WINAPI color_panel_s::ColorPanelUserProc(HANDLE hDlg, int Msg, int Param1, intptr_t Param2)
{
	auto update_cursor = [=]() {
		COORD coord;
		coord.X = (index & 7) * 3 + 1;
		coord.Y = (index > 7);
		SendDlgMessage(hDlg, DM_SETCURSORPOS, ID_CP_COLORS_RECT + offset, (intptr_t)&coord);
	};

	auto update_index_and_cursor = [&](uint32_t newindex) {
		update_index(newindex);
		update_cursor();
		SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
	};

	switch(Param1) {
	case ID_CP_RGB_PREFIX: {
		if (Msg == DN_CTLCOLORDLGITEM) {
			if (bRGB)
				return FarColorToReal(COL_DIALOGEDIT);
			else
				return FarColorToReal(COL_DIALOGEDITDISABLED);
		}
	}
	break;

	case ID_CP_RGB_SAMPLE:
		if (Msg == DN_DRAWDLGITEM) {
			if (!bRGB)
				return 0;
		}
	break;

	case ID_CP_CHECKBOX:
		if (Msg == DN_BTNCLICK) {
			bTransparent = (!SendDlgMessage(hDlg, DM_GETCHECK, ID_CP_CHECKBOX + offset, 0));
			SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
		}
	break;

	case ID_CP_CHECKBOX_RGB:
		if (Msg == DN_BTNCLICK) {
			bRGB = SendDlgMessage(hDlg, DM_GETCHECK, ID_CP_CHECKBOX_RGB + offset, 0);
			SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
		}
	break;

	case ID_CP_EDIT_RGB:
		if (Msg == DN_EDITCHANGE) {
			SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_CP_EDIT_RGB + offset, (uintptr_t)wsRGB);
			update_rgb_color_from_str( );
			SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
		}
	break;

	case ID_CP_COLORS_RECT:
		switch(Msg) {

		case DM_UPDATECURSOR:
			update_cursor();
		break;

//		case DN_BTNCLICK:
		case DN_MOUSECLICK: {
			MOUSE_EVENT_RECORD *mEv = (MOUSE_EVENT_RECORD *)Param2;
			uint32_t newindex = mEv->dwMousePosition.X / 3 + (mEv->dwMousePosition.Y & 1) * 8;
			update_index_and_cursor(newindex);
		}
		break;

		case DN_KEY:
			switch(Param2) {

			case KEY_LEFT:
			case KEY_NUMPAD4: 
			case KEY_MSWHEEL_LEFT:
				update_index_and_cursor(index - 1);
			break;

			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_MSWHEEL_RIGHT:
				update_index_and_cursor(index + 1);
			break;

			case KEY_UP:
			case KEY_NUMPAD8:
			case KEY_MSWHEEL_UP:
			case KEY_PGUP:
			case KEY_NUMPAD9:
				update_index_and_cursor(index - 8);
			break;

			case KEY_DOWN:
			case KEY_NUMPAD2:
			case KEY_MSWHEEL_DOWN:
			case KEY_PGDN:
			case KEY_NUMPAD3:
				update_index_and_cursor(index + 8);
			break;

			case KEY_HOME:
			case KEY_NUMPAD7:
				update_index_and_cursor(0);
			break;

			case KEY_END:
			case KEY_NUMPAD1:
				update_index_and_cursor(15);
			break;
			}

		break;
		}

	break;
	}

	return -1;
}

static intptr_t WINAPI GetColorDlgProc(HANDLE hDlg, int Msg, int Param1, intptr_t Param2)
{
	set_color_s *colorState = (set_color_s *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

	auto update_dialog_items = [=]() {

		if (colorState->bTransparencyEnabled) {

			for (int g = 0; g < 2; g++ ) {
				const size_t offset = colorState->cPanel[g].offset;
				bool bSet = !colorState->cPanel[g].bTransparent;
				SendDlgMessage(hDlg, DM_SETCHECK, ID_CP_CHECKBOX + offset, bSet);
				for (size_t i = ID_CP_COLORS_RECT + offset; i <= ID_CP_RGB_PREFIX + offset; i++)
					SendDlgMessage(hDlg, DM_SHOWITEM, i, bSet);
			}

			for (size_t i = 0; i < ARRAYSIZE(sup_styles); i++) {
				SendDlgMessage(hDlg, DM_SET3STATE, sup_styles[i].id, true);

				if (colorState->style_inherit & sup_styles[i].flags)
					SendDlgMessage(hDlg, DM_SETCHECK, sup_styles[i].id, BSTATE_3STATE);
				else
					SendDlgMessage(hDlg, DM_SETCHECK, sup_styles[i].id, (bool)(colorState->style & sup_styles[i].flags));
			}
		}

		for (int g = 0; g < 2; g++ ) {
			const size_t offset = colorState->cPanel[g].offset;

			bool bSet = colorState->cPanel[g].bRGB;
			SendDlgMessage(hDlg, DM_SETCHECK, ID_CP_CHECKBOX_RGB + offset, bSet);
			for (size_t i = ID_CP_EDIT_RGB + offset; i <= ID_CP_RGB_PREFIX + offset; i++)
				SendDlgMessage(hDlg, DM_ENABLE, i, bSet);

			SendDlgMessage(hDlg, DM_ENABLE, ID_CP_CHECKBOX_RGB + offset, colorState->bRGBEnabled);
		}

		if (colorState->bStyleEnabled) {

			for (size_t i = 0; i < ARRAYSIZE(sup_styles); i++)
				SendDlgMessage(hDlg, DM_ENABLE, sup_styles[i].id, colorState->bStyle);

			SendDlgMessage(hDlg, DM_SETCHECK, ID_ST_CHECKBOX_STYLE_ENABLE, colorState->bStyle);
		}
		SendDlgMessage(hDlg, DM_ENABLE, ID_ST_CHECKBOX_STYLE_ENABLE, colorState->bStyleEnabled);

	};

	auto set_focus = [=]() {

		SendDlgMessage(hDlg, DM_UPDATECURSOR, ID_ST_FG_COLORS_RECT, 0);
		SendDlgMessage(hDlg, DM_UPDATECURSOR, ID_ST_BG_COLORS_RECT, 0);

		if (!colorState->cPanel[set_color_s::IDC_FOREGROUND_PANEL].bTransparent)
			SendDlgMessage(hDlg, DM_SETFOCUS, ID_ST_FG_COLORS_RECT, 0);
		else if (!colorState->cPanel[set_color_s::IDC_BACKGROUND_PANEL].bTransparent)
			SendDlgMessage(hDlg, DM_SETFOCUS, ID_ST_BG_COLORS_RECT, 0);
		else
			SendDlgMessage(hDlg, DM_SETFOCUS, ID_ST_CHECKBOX_FOREGROUND, 0);
	};

	switch (Msg) {

	case DM_UPDATECOLORCODE: {
		colorState->update_color( );
		SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
	}
	break;

	case DN_INITDIALOG: {
		SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID_ST_FG_COLORS_RECT, Opt.CursorSize[1]);
		SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID_ST_BG_COLORS_RECT, Opt.CursorSize[1]);
		set_focus( );
		update_dialog_items( );
	}
	break;

	case DN_BTNCLICK: {
		if (Param1 == ID_ST_CHECKBOX_STYLE_ENABLE) {
			colorState->bStyle = (bool)(SendDlgMessage(hDlg, DM_GETCHECK, ID_ST_CHECKBOX_STYLE_ENABLE, 0));
			colorState->update_color( );
			SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			break;
		}

		if (Param1 >= ID_ST_CHECKBOX_STYLE_FIRST && Param1 <= ID_ST_CHECKBOX_STYLE_LAST) {
			for (size_t i = 0; i < ARRAYSIZE(sup_styles); i++) {
				const int &id = sup_styles[i].id;
				const int &stf = sup_styles[i].flags;
				if (id != Param1)
					continue;

				int iState = (int)SendDlgMessage(hDlg, DM_GETCHECK, id, 0);

				if (iState == BSTATE_UNCHECKED) {
					colorState->style &= (~stf);
					colorState->style_inherit &= (~stf);
				}
				else if (iState == BSTATE_CHECKED) {
					colorState->style |= stf;
					colorState->style_inherit &= (~stf);
				}
				else if (iState == BSTATE_3STATE) {
					colorState->style_inherit |= stf;
					colorState->style &= (~stf);
				}

				colorState->update_color( );
				SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			}
			break;
		}

		if (Param1 == ID_ST_BUTTON_RESTORE) {
			colorState->reset_color( );
			SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_ST_EDIT_FORERGB, (uintptr_t)colorState->cPanel[set_color_s::IDC_FOREGROUND_PANEL].wsRGB);
			SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_ST_EDIT_BACKRGB, (uintptr_t)colorState->cPanel[set_color_s::IDC_BACKGROUND_PANEL].wsRGB);
			set_focus( );
			update_dialog_items( );
			SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			break;
		}

	}
	break;

	case DN_MOUSECLICK: {
	}
	break;

	} // switch

	if (Param1 >= ID_CP_FIRST && Param1 < ID_CP_FIRST + ID_CP_TOTAL * 2) {
		const uint32_t id = (Param1 >= ID_CP_FIRST + ID_CP_TOTAL);
		const intptr_t rv = colorState->cPanel[id].ColorPanelUserProc(hDlg, Msg, (Param1 - id * ID_CP_TOTAL) - ID_CP_FIRST, Param2);

		if (rv != -1)
			return rv;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool GetColorDialogInner(uint64_t *color, uint64_t *mask, bool bRGB, bool bFontStyles, bool bCentered)
{
	const wchar_t VerticalLine[] = { BoxSymbols[BS_T_H2V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1],BoxSymbols[BS_V1], 
			BoxSymbols[BS_V1],BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], 
			BoxSymbols[BS_V1],BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_B_H1V1], 0};

	const wchar_t HorizontalLine[] = { BoxSymbols[BS_L_H1V2], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_R_H1V1], 0};

	const wchar_t HorizontalMini[] = { BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], 
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], 0 };

	static const wchar_t *HexMask = L"HHHHHH";

	if (!color) return false;

	set_color_s	colorState;
	colorState.enable_RGB(bRGB);
	colorState.enable_font_styles(bFontStyles);
	colorState.set_color(*color);
	if (mask) {
		colorState.enable_transparency(true);
		colorState.set_mask(*mask);
	}

	colorState.update_color( );

	DialogDataEx ColorDlgData[] = {

		{DI_DOUBLEBOX, 3, 1, 55, 18, {}, 0, Msg::SetColorTitle},

		{DI_TEXT,  0, 16,  0, 16, {}, DIF_SEPARATOR, L""},
		{DI_VTEXT, 39, 1, 39, 16, {}, DIF_BOXCOLOR, VerticalLine},
		{DI_TEXT, 3, 6,  39, 6, {}, DIF_BOXCOLOR, HorizontalLine},
		{DI_TEXT, 3, 11, 39, 11, {}, DIF_BOXCOLOR, HorizontalLine},
		{DI_TEXT, 41, 4, 54, 4, {}, DIF_BOXCOLOR, HorizontalMini},

		{DI_CHECKBOX, 5, 2, 20, 2, {}, DIF_AUTOMATION, Msg::SetColorForeground},
		{DI_TEXT, 5, 2, 20, 2, {}, DIF_HIDDEN, Msg::SetColorForeground},
		{DI_USERCONTROL, 5, 3, 28, 4, {}, 0, L"" },
		{DI_CHECKBOX, 30, 2, 36, 2, {}, DIF_AUTOMATION, L"RGB#"},
		{DI_FIXEDIT, 31, 3, 37, 3, {(DWORD_PTR)HexMask}, DIF_MASKEDIT, colorState.cPanel[set_color_s::IDC_FOREGROUND_PANEL].wsRGB},
		{DI_BUTTON, 30, 4, 36, 4, {}, DIF_BTNNOCLOSE, L"256‥"},
		{DI_BUTTON, 30, 5, 36, 5, {}, DIF_BTNNOCLOSE, L"RGB‥"},
		{DI_USERCONTROL, 26, 5, 28, 5, {}, DIF_NOFOCUS, L"" },
		{DI_TEXT, 30, 3, 30, 3, {}, 0, L"#"},

		{DI_CHECKBOX, 5, 7, 20, 7, {}, DIF_AUTOMATION, Msg::SetColorBackground},
		{DI_TEXT, 5, 7, 20, 7, {}, DIF_HIDDEN, Msg::SetColorBackground},
		{DI_USERCONTROL, 5, 8, 28, 9, {}, 0, L"" },
		{DI_CHECKBOX, 30, 7, 36, 7, {}, DIF_AUTOMATION, L"RGB#"},
		{DI_FIXEDIT, 31, 8, 37, 8, {(DWORD_PTR)HexMask}, DIF_MASKEDIT, colorState.cPanel[set_color_s::IDC_BACKGROUND_PANEL].wsRGB},
		{DI_BUTTON, 30, 9, 36, 9, {}, DIF_BTNNOCLOSE, L"256‥"},
		{DI_BUTTON, 30, 10, 36, 10, {}, DIF_BTNNOCLOSE, L"RGB‥"},
		{DI_USERCONTROL, 26, 10, 28, 10, {}, DIF_NOFOCUS, L"" },
		{DI_TEXT, 30, 8, 30, 8, {}, 0, L"#"},

		{DI_TEXT, 41, 2, 48, 2, {}, 0, L"Style:"},
		{DI_CHECKBOX, 41, 3, 48, 3, {}, DIF_AUTOMATION, L"Enable"},
		{DI_CHECKBOX, 41, 5, 48, 5, {}, DIF_DISABLE, L"Bold"},
		{DI_CHECKBOX, 41, 6, 48, 6, {}, DIF_DISABLE, L"Italic"},
		{DI_CHECKBOX, 41, 7, 48, 7, {}, DIF_DISABLE, L"Overline"},
		{DI_CHECKBOX, 41, 8, 48, 8, {}, DIF_DISABLE, L"Strikeout"},
		{DI_CHECKBOX, 41, 9, 48, 9, {}, DIF_DISABLE, L"Underline"},
		{DI_CHECKBOX, 41, 10, 48, 10, {}, DIF_DISABLE, L"Blink"},
		{DI_CHECKBOX, 41, 11, 48, 11, {}, DIF_DISABLE, L"Inverse"},

		{DI_USERCONTROL, 5, 12, 37, 15, {}, DIF_NOFOCUS, L"" },

		{DI_BUTTON, 0, 17, 0, 17, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::SetColorSet},
		{DI_BUTTON, 0, 17, 0, 17, {}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, Msg::SetColorReset},
		{DI_BUTTON, 0, 17, 0, 17, {}, DIF_CENTERGROUP, Msg::SetColorCancel},

	};

	MakeDialogItemsEx(ColorDlgData, ColorDlg);

	ColorDlg[ID_ST_FG_COLORS_RECT].VBuf = colorState.cPanel[set_color_s::IDC_FOREGROUND_PANEL].vbuff;
	ColorDlg[ID_ST_BG_COLORS_RECT].VBuf = colorState.cPanel[set_color_s::IDC_BACKGROUND_PANEL].vbuff;
	ColorDlg[ID_ST_FRGB_SMAPLE].VBuf = colorState.cPanel[set_color_s::IDC_FOREGROUND_PANEL].vbuff_rgb;
	ColorDlg[ID_ST_BRGB_SMAPLE].VBuf = colorState.cPanel[set_color_s::IDC_BACKGROUND_PANEL].vbuff_rgb;
	ColorDlg[ID_ST_COLOREXAMPLE].VBuf = colorState.samplevbuff;

	if (!mask) {
		ColorDlg[ID_ST_CHECKBOX_FOREGROUND].Flags |= (DIF_HIDDEN | DIF_DISABLE);
		ColorDlg[ID_ST_CHECKBOX_BACKGROUND].Flags |= (DIF_HIDDEN | DIF_DISABLE);
		ColorDlg[ID_ST_TEXT_FOREGROUND].Flags ^= DIF_HIDDEN;
		ColorDlg[ID_ST_TEXT_BACKGROUND].Flags ^= DIF_HIDDEN;
	}

	Dialog Dlg(ColorDlg, ARRAYSIZE(ColorDlg), GetColorDlgProc, (LONG_PTR)&colorState);
	Dlg.SetPosition(-1, -1, 59, 20);

	for (size_t i = ID_ST_FG_COLORS_RECT; i <= ID_ST_FRGB_PREFIX; i++)
		Dlg.SetAutomation(ID_ST_CHECKBOX_FOREGROUND, i, DIF_HIDDEN, DIF_NONE, DIF_NONE, DIF_HIDDEN);

	for (size_t i = ID_ST_BG_COLORS_RECT; i <= ID_ST_BRGB_PREFIX; i++)
		Dlg.SetAutomation(ID_ST_CHECKBOX_BACKGROUND, i, DIF_HIDDEN, DIF_NONE, DIF_NONE, DIF_HIDDEN);

	for (size_t i = ID_ST_EDIT_FORERGB; i <= ID_ST_FRGB_PREFIX; i++)
		Dlg.SetAutomation(ID_ST_CHECKBOX_RGB1, i, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);

	for (size_t i = ID_ST_EDIT_BACKRGB; i <= ID_ST_BRGB_PREFIX; i++)
		Dlg.SetAutomation(ID_ST_CHECKBOX_RGB2, i, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);

	for (size_t i = 0; i < ARRAYSIZE(sup_styles); i++) {
		Dlg.SetAutomation(ID_ST_CHECKBOX_STYLE_ENABLE, sup_styles[i].id, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	}

	Dlg.Process();
	int ExitCode = Dlg.GetExitCode();

	if (ExitCode == ID_ST_BUTTON_SETCOLOR) {

		colorState.update_color( );

		*color = colorState.color;
		if (mask) *mask = colorState.mask;

		return true;
	}

	return false;
}

bool GetColorDialogForFileFilter(uint64_t *color, uint64_t *mask)
{
	return GetColorDialogInner(color, mask, true, true, true);
}

bool GetColorDialog16(uint16_t *color, bool bCentered)
{
	if (!color) return false;

	uint64_t color64 = *color;
	bool out = GetColorDialogInner(&color64, NULL, false, false, bCentered);
	*color = (uint16_t)(color64 & 0xFFFF);

	return out;
}
