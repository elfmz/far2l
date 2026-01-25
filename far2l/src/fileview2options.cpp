/*
fileмшуц2options.cpp

Фаровское горизонтальное меню (вызов hmenu.cpp с конкретными параметрами) для filewview
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

#include "options.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "hmenu.hpp"
#include "vmenu.hpp"
#include "config.hpp"
#include "ConfigOptEdit.hpp"
#include "ConfigOptSaveLoad.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "macro.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "fileview.hpp"

#include "fileview2options.hpp"

void ViewerShellOptions(int LastCommand, MOUSE_EVENT_RECORD *MouseEvent, FileViewer* fileView)
{
	MenuDataEx FileMenu[] = {
		{Msg::ViewerMenuNextFile,	0,	KEY_ADD  },
		{Msg::ViewerMenuPrevFile,	0,	KEY_SUBTRACT  },
		{Msg::ViewerMenuFileHistory,	0,	KEY_ALTF11  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuFilePrint,	0,	KEY_ALTF5  },
		{Msg::ViewerMenuFilePrinter,	0,	0  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuFileGoToPanel,	0,	KEY_CTRLF10  },
		{Msg::ViewerMenuFileExit,	0,	KEY_F3  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuFileHelp,	0,	KEY_F1  },
		{Msg::ViewerMenuFilePlugins,	0,	KEY_F11  },
		{Msg::ViewerMenuFileScreens,	0,	KEY_F12  },
		{Msg::ViewerMenuFileExitFar,	0,	KEY_ALTF10  },
	};

	MenuDataEx ToolsMenu[] = {
		{Msg::ViewerMenuUndo,	0,	KEY_CTRLZ  },
		{Msg::ViewerMenuRedo,	0,	KEY_CTRLSHIFTZ  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuSelect,	0,	KEY_SHIFTRIGHT  },
		{Msg::ViewerMenuCopy,	0,	KEY_CTRLC  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuGrep,	0,	KEY_CTRLF7  },
		{Msg::ViewerMenuFind,	0,	KEY_F7  },
		{Msg::ViewerMenuFindNext,	0,	KEY_SHIFTF7  },
		{Msg::ViewerMenuFindPrev,	0,	KEY_ALTF7  },
	};

	MenuDataEx NavigateMenu[] = {
		{Msg::ViewerMenuNavigateGoToLine,	0,	KEY_ALTF8  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuNavigateBegin,	0,	KEY_CTRLHOME  },
		{Msg::ViewerMenuNavigateEnd,	0,	KEY_CTRLEND  },
		{Msg::ViewerMenuNavigatePageUp,	0,	KEY_PGDN  },
		{Msg::ViewerMenuNavigatePageDown,	0,	KEY_PGUP  },
		{Msg::ViewerMenuNavigateAltPageUp,	0,	KEY_ALTPGDN  },
		{Msg::ViewerMenuNavigateAltPageDown,	0,	KEY_ALTPGUP  },
		{Msg::ViewerMenuNavigateToStartLines,	0,	KEY_CTRLSHIFTLEFT  },
		{Msg::ViewerMenuNavigateToEndLines,	0,	KEY_CTRLSHIFTRIGHT  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuNavigateBm0,	0,	KEY_CTRL0  },
		{Msg::ViewerMenuNavigateBm1,	0,	KEY_CTRL1  },
		{Msg::ViewerMenuNavigateBm2,	0,	KEY_CTRL2  },
		{Msg::ViewerMenuNavigateBm3,	0,	KEY_CTRL3  },
		{Msg::ViewerMenuNavigateBm4,	0,	KEY_CTRL4  },
		{Msg::ViewerMenuNavigateBm5,	0,	KEY_CTRL5  },
		{Msg::ViewerMenuNavigateBm6,	0,	KEY_CTRL6  },
		{Msg::ViewerMenuNavigateBm7,	0,	KEY_CTRL7  },
		{Msg::ViewerMenuNavigateBm8,	0,	KEY_CTRL8  },
		{Msg::ViewerMenuNavigateBm9,	0,	KEY_CTRL9  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuNavigatePin0,	0,	KEY_CTRLSHIFT0  },
		{Msg::ViewerMenuNavigatePin1,	0,	KEY_CTRLSHIFT1  },
		{Msg::ViewerMenuNavigatePin2,	0,	KEY_CTRLSHIFT2  },
		{Msg::ViewerMenuNavigatePin3,	0,	KEY_CTRLSHIFT3  },
		{Msg::ViewerMenuNavigatePin4,	0,	KEY_CTRLSHIFT4  },
		{Msg::ViewerMenuNavigatePin5,	0,	KEY_CTRLSHIFT5  },
		{Msg::ViewerMenuNavigatePin6,	0,	KEY_CTRLSHIFT6  },
		{Msg::ViewerMenuNavigatePin7,	0,	KEY_CTRLSHIFT7  },
		{Msg::ViewerMenuNavigatePin8,	0,	KEY_CTRLSHIFT8  },
		{Msg::ViewerMenuNavigatePin9,	0,	KEY_CTRLSHIFT9  },
	};

	MenuDataEx ViewMenu[] = {
		{Msg::ViewerMenuKeyBar,	0,	KEY_CTRLB  },
		{Msg::ViewerMenuTitleBar,	0,	KEY_CTRLSHIFTB  },
		{Msg::ViewerMenuScrollBar,	0,	KEY_CTRLS  },
		{Msg::ViewerMenuShowMenuBar,	0,	0  },
		{Msg::ViewerMenuConsole,	0,	KEY_CTRLO  },
		{Msg::ViewerMenuWrap,	0,	KEY_F2  },
		{Msg::ViewerMenuWordWrap,	0,	KEY_SHIFTF2  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuHex,	0,	KEY_F4  },
		{Msg::ViewerMenuExtl,	0,	KEY_F5  },
		{Msg::ViewerMenuEditor,	0,	KEY_F6  },
		{Msg::ViewerMenuTabSize,	0,	KEY_SHIFTF5  },
		{Msg::ViewerMenuOem,	0,	KEY_F8  },
		{Msg::ViewerMenuCodepage,	0,	KEY_SHIFTF8  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::ViewerMenuFileOptions,	0,	KEY_ALTSHIFTF9  },
	};

	HMenuData MainMenu[] = {
		{Msg::ViewerMenuFileTitle,     1, FileMenu,      ARRAYSIZE(FileMenu),       L"Viewer"},
		{Msg::ViewerMenuToolsTitle,    0, ToolsMenu,     ARRAYSIZE(ToolsMenu),      L"Viewer" },
		{Msg::ViewerMenuNavigateTitle, 0, NavigateMenu,  ARRAYSIZE(NavigateMenu),   L"Viewer" },
		{Msg::ViewerMenuViewTitle,     0, ViewMenu,      ARRAYSIZE(ViewMenu),       L"Viewer" }
	};

	static int LastHItem = -1, LastVItem = 0;
	int HItem, VItem;

	int size = (int)ARRAYSIZE(ViewMenu);
	for(int i = 0; i < size; ++i) {
		int check = fileView->IsOptionActive(MENU_VIEW_VIEW, i);
		if (check) ViewMenu[i].SetCheck(1);
	}

	// Навигация по меню
	{
		HMenu HOptMenu(MainMenu, ARRAYSIZE(MainMenu));
		HOptMenu.SetHelp(L"Viewer");
		int gap = fileView->MenuBarPosition();
		HOptMenu.SetPosition(0, gap, ScrX, gap);

		if (LastCommand) {
			MenuDataEx *VMenuTable[] = {FileMenu, ToolsMenu, NavigateMenu, ViewMenu};
			int HItemToShow = LastHItem;

			MainMenu[0].Selected = 0;
			MainMenu[HItemToShow].Selected = 1;
			VMenuTable[HItemToShow][0].SetSelect(0);
			VMenuTable[HItemToShow][LastVItem].SetSelect(1);
			HOptMenu.Show();
			HOptMenu.ProcessKey(KEY_DOWN);
		}
		else if (MouseEvent) {
			HOptMenu.Show();
			HOptMenu.ProcessMouse(MouseEvent);
		}
		else {
			HOptMenu.Show();
			HOptMenu.ProcessKey(KEY_DOWN);
		}

		HOptMenu.Process();

		HOptMenu.GetExitCode(HItem, VItem);
	}

	if (HItem == MENU_VIEW_FILE) {
		switch(VItem) {
		case MENU_VIEW_FILE_PLUGINS:	// Plugin commands
			FrameManager->ProcessKey(KEY_F11);
			break;
		case MENU_VIEW_FILE_SCREENS:		// Screens list
			FrameManager->ProcessKey(KEY_F12);
			break;
		}
	}

	if (HItem >= 0 && VItem >= 0) {
		FarKey key = MainMenu[HItem].SubMenu[VItem].AccelKey;
		fileView->ProcessMenuCommand(HItem, VItem, key);
	}

	if (HItem != -1 && VItem != -1) {
		LastHItem = HItem;
		LastVItem = VItem;
	}
}
