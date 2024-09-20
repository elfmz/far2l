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
#include "pick_color256.hpp"
#include "pick_colorRGB.hpp"
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

#include "pick_color.hpp"

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
		{(const wchar_t *)Msg::SetDefaultColorsRGB, 0,             0},
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

			// Set default 8 bit colors
			if (GroupsCode == 12) {
				for(size_t i = 0; i < SIZE_ARRAY_PALETTE; i++) {
					Palette[i] = DefaultPalette8bit[i];
				}
				break;
			}

			// Set default RGB
			if (GroupsCode == 13) {
				uint32_t basepalette[32];
				WINPORT(GetConsoleBasePalette)(NULL, basepalette);

				for(size_t i = 0; i < SIZE_ARRAY_PALETTE; i++) {
					uint8_t color = DefaultPalette8bit[i];
					Palette[i] = ((uint64_t)basepalette[16 + (color & 0xF)] << 16);
					Palette[i] += ((uint64_t)basepalette[color >> 4] << 40);
					Palette[i] += FOREGROUND_TRUECOLOR + BACKGROUND_TRUECOLOR;
					Palette[i] += color;
				}

				break;
			}

			// Set black & white 8 bit colors
			if (GroupsCode == 14) {
				for(size_t i = 0; i < SIZE_ARRAY_PALETTE; i++) {
					Palette[i] = BlackPalette8bit[i];
				}
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
	uint64_t NewColor = Palette[PaletteIndex];

	if (GetColorDialog(&NewColor, false)) {
		Palette[PaletteIndex] = NewColor;
		Palette8bit[PaletteIndex] = static_cast<uint8_t>(NewColor);

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
