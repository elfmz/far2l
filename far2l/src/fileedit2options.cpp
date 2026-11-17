/*
fileedit2options.cpp

Фаровское горизонтальное меню (вызов hmenu.cpp с конкретными параметрами) для редактора
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
#include "fileedit.hpp"

#include "fileedit2options.hpp"
#include "options.hpp"

void initializeWindowMenuContext(WindowMenuContext& ctx);
void applyMenu(WindowMenuContext& ctx, int VItem);

void EditorShellOptions(int LastCommand, MOUSE_EVENT_RECORD *MouseEvent, FileEditor* fileEditor)
{
	MenuDataEx FileMenu[] = {
		{Msg::EditorMenuFileOpen,	0,	KEY_SHIFTF4  },
		{Msg::EditorMenuFileSave,	0,	KEY_F2  },
		{Msg::EditorMenuFileSaveAs,	0,	KEY_SHIFTF2  },
		{Msg::EditorMenuFileHistory,	0,	KEY_ALTF11  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuFilePrint,	0,	KEY_ALTF5  },
		{Msg::EditorMenuFilePrinter,	0,	0  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuFileGoToPanel,	0,	KEY_CTRLF10  },
		{Msg::EditorMenuFileExit,	0,	KEY_F10  },
		{Msg::EditorMenuFileSaveQ,	0,	KEY_SHIFTF10  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuFileHelp,	0,	0  },
		{Msg::EditorMenuFilePlugins,	0,	0  },
		{Msg::EditorMenuFileScreens,	0,	0  },
		{Msg::EditorMenuFileExitFar,	0,	KEY_ALTF10  },
	};

	MenuDataEx EditMenu[] = {
		{Msg::EditorMenuEditUndo,	0,	KEY_CTRLZ  },
		{Msg::EditorMenuEditRedo,	0,	KEY_CTRLSHIFTZ  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuEditSelectAll,	0,	KEY_CTRLA  },
		{Msg::EditorMenuEditSelectVertical,	0,	KEY_ALTSHIFTRIGHT  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuEditCut,	0,	KEY_CTRLX  },
		{Msg::EditorMenuEditCopy,	0,	KEY_CTRLC  },
		{Msg::EditorMenuEditPaste,	0,	KEY_CTRLV  },
		{Msg::EditorMenuEditDelete,	0,	KEY_CTRLD  },
		{Msg::EditorMenuEditDelLine,	0,	KEY_CTRLY  },
		{Msg::EditorMenuEditDeleteWordLeft,	0,	KEY_CTRLBS  },
		{Msg::EditorMenuEditDeleteWordRight,	0,	KEY_CTRLDEL  },
		{Msg::EditorMenuEditDeleteEOL,	0,	KEY_CTRLK  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuEditFind,	0,	KEY_F7  },
		{Msg::EditorMenuEditFindReplace,	0,	KEY_CTRLF7  },
		{Msg::EditorMenuEditFindNext,	0,	KEY_SHIFTF7  },
		{Msg::EditorMenuEditFindPrev,	0,	KEY_ALTF7  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuEditShortFName,	0,	KEY_SHIFTENTER  },
		{Msg::EditorMenuEditLongFName,	0,	KEY_CTRLF  },
		{Msg::EditorMenuEditLeftPaneName,	0,	KEY_CTRLALTBRACKET  },
		{Msg::EditorMenuEditRightPanel,	0,	KEY_CTRLALTBACKBRACKET  },
		{Msg::EditorMenuEditActivePanel,	0,	KEY_ALTSHIFTBRACKET  },
		{Msg::EditorMenuEditInactivePanel,	0,	KEY_ALTSHIFTBACKBRACKET  },
		{Msg::EditorMenuEditBlockIndent,	0,	KEY_ALTU  },
		{Msg::EditorMenuEditBlockUnindent,	0,	KEY_ALTI  },
	};

	MenuDataEx NavigateMenu[] = {
		{Msg::EditorMenuNavigateGoToLine,	0,	KEY_ALTF8  },
		{Msg::EditorMenuNavigateTop,	0,	KEY_CTRLN  },
		{Msg::EditorMenuNavigateBottom,	0,	KEY_CTRLE  },
		{Msg::EditorMenuNavigateBegin,	0,	KEY_CTRLHOME  },
		{Msg::EditorMenuNavigateEnd,	0,	KEY_CTRLEND  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuNavigateBm0,	0,	KEY_CTRL0  },
		{Msg::EditorMenuNavigateBm1,	0,	KEY_CTRL1  },
		{Msg::EditorMenuNavigateBm2,	0,	KEY_CTRL2  },
		{Msg::EditorMenuNavigateBm3,	0,	KEY_CTRL3  },
		{Msg::EditorMenuNavigateBm4,	0,	KEY_CTRL4  },
		{Msg::EditorMenuNavigateBm5,	0,	KEY_CTRL5  },
		{Msg::EditorMenuNavigateBm6,	0,	KEY_CTRL6  },
		{Msg::EditorMenuNavigateBm7,	0,	KEY_CTRL7  },
		{Msg::EditorMenuNavigateBm8,	0,	KEY_CTRL8  },
		{Msg::EditorMenuNavigateBm9,	0,	KEY_CTRL9  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuNavigatePin0,	0,	KEY_CTRLSHIFT0  },
		{Msg::EditorMenuNavigatePin1,	0,	KEY_CTRLSHIFT1  },
		{Msg::EditorMenuNavigatePin2,	0,	KEY_CTRLSHIFT2  },
		{Msg::EditorMenuNavigatePin3,	0,	KEY_CTRLSHIFT3  },
		{Msg::EditorMenuNavigatePin4,	0,	KEY_CTRLSHIFT4  },
		{Msg::EditorMenuNavigatePin5,	0,	KEY_CTRLSHIFT5  },
		{Msg::EditorMenuNavigatePin6,	0,	KEY_CTRLSHIFT6  },
		{Msg::EditorMenuNavigatePin7,	0,	KEY_CTRLSHIFT7  },
		{Msg::EditorMenuNavigatePin8,	0,	KEY_CTRLSHIFT8  },
		{Msg::EditorMenuNavigatePin9,	0,	KEY_CTRLSHIFT9  },
	};

	MenuDataEx ViewMenu[] = {
		{Msg::EditorMenuViewKeyBar,	0,	KEY_CTRLB  },
		{Msg::EditorMenuViewTitleBar,	0,	KEY_CTRLSHIFTB  },
		{Msg::EditorMenuViewMenuBar, 0, 0 },
		{Msg::EditorMenuViewWordWrap,	0,	KEY_F3  },
		{Msg::EditorMenuViewNumbers,	0,	KEY_CTRLF3  },
		{Msg::EditorMenuViewSpaces,	0,	KEY_F5  },
		{Msg::EditorMenuViewEOL,	0,	KEY_CTRLSHIFTF5  },
		{Msg::EditorMenuViewOvertype,	0,	KEY_INS  },
		{Msg::EditorMenuViewFullScreen,	0,	KEY_ALTF9  },
		{Msg::EditorMenuViewLock,	0,	KEY_CTRLL  },
		{Msg::EditorMenuViewConsole,	0,	KEY_CTRLO  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuViewTabSize,	0,	KEY_SHIFTF5  },
		{Msg::EditorMenuViewTabs,	0,	KEY_CTRLF5  },
		{Msg::EditorMenuViewOem,	0,	KEY_F8  },
		{Msg::EditorMenuViewCodepage,	0,	KEY_SHIFTF8  },
		{Msg::EditorMenuViewViewer,	0,	KEY_F6  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuFileOptions,	0,	KEY_ALTSHIFTF9  },
	};

	std::vector<FARString> v;
	for(size_t i = 0; ; ++i) {
		FARString keyName, description, s;
		if (CtrlObject->Macro.GetMacroKeyInfo(true, MACRO_EDITOR, i, description, keyName)	== -1) 
			break;
		s = s.Format(L"%-35ls %ls", keyName.GetBuffer(), description.GetBuffer());
		v.push_back(s);	
	}

	int MacroMenuSize = std::min(128, (int)v.size() + 2);
	MenuDataEx MacroMenu[128] = {
		{Msg::EditorMenuMacroRecord,	0,	KEY_CTRLB  },
		{L"", LIF_SEPARATOR, 0  }
	};

	for(size_t i = 0; i < v.size() && i < 128; ++i) {
		MacroMenu[i+2].Name = v[i].GetBuffer();
		MacroMenu[i+2].Flags = 0;
		MacroMenu[i+2].AccelKey = 0;
	}

	// plugins menu
	std::vector<MenuItemData> plugins = CtrlObject->Plugins.GetMenuItems(MODALTYPE_EDITOR);
	int PluginsMenuSize = std::min(128, (int)plugins.size());
	MenuDataEx PluginsMenu[128];

	for(size_t i = 0; i < plugins.size() && i < 128; ++i) {
		PluginsMenu[i].Name = plugins[i].name.c_str();
		PluginsMenu[i].Flags = 0;
		PluginsMenu[i].AccelKey = 0;
	}

	WindowMenuContext wc;
	initializeWindowMenuContext(wc);

	HMenuData MainMenu[] = {
		{Msg::EditorMenuFileTitle,     1, FileMenu,    ARRAYSIZE(FileMenu),    L"Editor"},
		{Msg::EditorMenuEditTitle,    0, EditMenu,   ARRAYSIZE(EditMenu),   L"Editor" },
		{Msg::EditorMenuNavigateTitle, 0, NavigateMenu,     ARRAYSIZE(NavigateMenu),     L"Editor" },
		{Msg::EditorMenuViewTitle,  0, ViewMenu, ARRAYSIZE(ViewMenu), L"Editor" },
		{Msg::EditorMenuMacroTitle, 0, MacroMenu, MacroMenuSize, L"Editor" },
		{Msg::EditorMenuPluginsTitle, 0, PluginsMenu, PluginsMenuSize, L"Editor" },
		{Msg::MenuWindowTitle, 0, wc.WindowMenu,     wc.WindowMenuCount,     L"Editor"      }
	};

	static int LastHItem = -1, LastVItem = 0;
	int HItem, VItem;

	int size = (int)ARRAYSIZE(ViewMenu);
	for(int i = 0; i < size; ++i) {
		int check = fileEditor->IsOptionActive(MENU_VIEW, i);
		if (check) ViewMenu[i].SetCheck(1);
	}

	// Bookmarks highlighting
	std::vector<FARString> bookmarks;

	InternalEditorBookMark* savepos = fileEditor->GetBookmark();
	for(int i = 0; i < 10; ++i) {
		if (savepos->Line[i] == POS_NONE) {
			bookmarks.push_back(L"-");
			// continue;
		}
		else {
			int line = static_cast<int>(savepos->Line[i]);
			int pos = static_cast<int>(savepos->Cursor[i]);
			FARString s = fileEditor->GetLine(line, pos, 35);
			if (s.GetLength() < 2) s = L"-";
			s = s.Format(L"[%d, %d] %ls", line, pos, s.GetBuffer());
			bookmarks.push_back(s);
		}
		// now we have bookmark (if any) and we can replace it to the actual text
		FARString s = bookmarks[i]; 
		s = s.Format(L"%-35.35ls Ctrl+%d", s.GetBuffer(), i);
		bookmarks[i] = s;
		NavigateMenu[MENU_NAV_BM_0 + i].Name = bookmarks[i].GetBuffer();
	}

	// Навигация по меню
	{
		HMenu HOptMenu(MainMenu, ARRAYSIZE(MainMenu));
		HOptMenu.SetHelp(L"Editor");
		int gap = fileEditor->MenuBarPosition();
		HOptMenu.SetPosition(0, gap, ScrX, gap);

		if (LastCommand) {
			MenuDataEx *VMenuTable[] = {FileMenu, EditMenu, NavigateMenu, ViewMenu, MacroMenu, PluginsMenu, wc.WindowMenu };
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

	if (HItem == MENU_FILE) {
		switch(VItem) {
		case MENU_FILE_PLUGINS:	// Plugin commands
			FrameManager->ProcessKey(KEY_F11);
			break;
		case MENU_FILE_SCREEN_LIST:		// Screens list
			FrameManager->ProcessKey(KEY_F12);
			break;
		}
	}
	else if (HItem == MENU_PLUGINS) {
		CtrlObject->Plugins.OpenPlugin(plugins[VItem].pluginItem.pPlugin, OPEN_EDITOR, plugins[VItem].pluginItem.nItem);
		return;
	}
	else if (HItem == MENU_WINDOW) {
		applyMenu(wc, VItem);
		return;
	}

	if (HItem >= 0 && VItem >= 0) {
		FarKey key = MainMenu[HItem].SubMenu[VItem].AccelKey;
		fileEditor->ProcessMenuCommand(HItem, VItem, key);
	}

	if (HItem != -1 && VItem != -1) {
		LastHItem = HItem;
		LastVItem = VItem;
	}
}

void initializeWindowMenuContext(WindowMenuContext& ctx) 
{
	for (int i = 0; ; ++i) {
		Frame* frame = FrameManager->getWindowByTypeAndIndex(-1, i);
		if (!frame) break;

		FARString strNumText;
		if (i < 10)	strNumText.Format(L"&%d ", i);
		else if (i - 10 < 'Z' - 'A') strNumText.Format(L"&%c ", char(i - 10 + 'A'));
		else strNumText.Format(L"&  ");

		FARString strType, strName;

		frame->GetTypeAndName(strType, strName);
		ReplaceStrings(strName, L"&", L"&&", -1);
		strName.Format(L"%ls%-10.10ls %ls", strNumText.CPtr(), strType.CPtr(), strName.CPtr());
		ctx.windowNames.push_back(strName.CPtr());

		ctx.WindowMenu[ctx.WindowMenuCount].Name = ctx.windowNames[ctx.windowNames.size() - 1].c_str();
		ctx.WindowMenu[ctx.WindowMenuCount].AccelKey = 0;
		ctx.WindowMenu[ctx.WindowMenuCount].Flags = 0;

		if (frame->IsFileModified()) ctx.WindowMenu[ctx.WindowMenuCount].SetCheck(L'*');

		ctx.frameIndexes.push_back(i);
		ctx.subframeIndexes.push_back(-1);
        ++ctx.WindowMenuCount;
	}
}

void applyMenu(WindowMenuContext& ctx, int VItem) 
{
	switch(VItem) {
	case MENU_PANELWINDOWNEWEDITOR:
		FrameManager->SwitchToPanels();
		FrameManager->ProcessKey(KEY_SHIFTF4);
		break;
	default:
		if (VItem > MENU_PANELWINDOW_SEPARATOR2) {
			int frame = VItem - MENU_PANELWINDOW_SEPARATOR2 - 1;

			if (frame >= 0 && frame < (int)ctx.frameIndexes.size()) {
				int frameI = ctx.frameIndexes[frame];

				FrameManager->ActivateFrame(frameI);
			}
		}
		break;
	}
}

