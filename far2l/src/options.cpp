/*
options.cpp

Фаровское горизонтальное меню (вызов hmenu.cpp с конкретными параметрами)
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
#include "filepanels.hpp"
#include "filediff.hpp"
#include "panel.hpp"
#include "chgmmode.hpp"
#include "filelist.hpp"
#include "hilight.hpp"
#include "cmdline.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "history.hpp"
#include "message.hpp"
#include "config.hpp"
#include "ConfigOptEdit.hpp"
#include "ConfigOptSaveLoad.hpp"
#include "usermenu.hpp"
#include "datetime.hpp"
#include "setcolor.hpp"
#include "plist.hpp"
#include "filetype.hpp"
#include "Bookmarks.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "codepage.hpp"
#include "MaskGroups.hpp"

enum enumMenus
{
	MENU_LEFT,
	MENU_FILES,
	MENU_COMMANDS,
	MENU_OBJECT,
	MENU_OPTIONS,
	MENU_NAVIGATE,
	MENU_VIEW,
	MENU_RIGHT
};

enum enumLeftMenu
{
	MENU_LEFT_BRIEFVIEW,
	MENU_LEFT_MEDIUMVIEW,
	MENU_LEFT_FULLVIEW,
	MENU_LEFT_WIDEVIEW,
	MENU_LEFT_DETAILEDVIEW,
	MENU_LEFT_DIZVIEW,
	MENU_LEFT_LONGVIEW,
	MENU_LEFT_OWNERSVIEW,
	MENU_LEFT_LINKSVIEW,
	MENU_LEFT_ALTERNATIVEVIEW,
	MENU_LEFT_SEPARATOR1,
	MENU_LEFT_INFOPANEL,
	MENU_LEFT_TREEPANEL,
	MENU_LEFT_QUICKVIEW,
	MENU_LEFT_SEPARATOR2,
	MENU_LEFT_SORTMODES,
	MENU_LEFT_TOGGLEPANEL,
	MENU_LEFT_REREAD,
	MENU_LEFT_CHANGEDRIVE
};

// currently left == right

enum enumFilesMenu
{
	MENU_FILES_VIEW,
	MENU_FILES_EDIT,
	MENU_FILES_COPY,
	MENU_FILES_MOVE,
	MENU_FILES_LINK,
	MENU_FILES_CREATEFOLDER,
	MENU_FILES_DELETE,
	MENU_FILES_WIPE,
	MENU_FILES_SEPARATOR1,
	MENU_FILES_ADD,
	MENU_FILES_EXTRACT,
	MENU_FILES_ARCHIVECOMMANDS,
	MENU_FILES_SEPARATOR2,
	MENU_FILES_ATTRIBUTES,
	MENU_FILES_CHATTR,
	MENU_FILES_APPLYCOMMAND,
	MENU_FILES_DESCRIBE,
	MENU_FILES_SEPARATOR3,
	MENU_FILES_SELECTGROUP,
	MENU_FILES_UNSELECTGROUP,
	MENU_FILES_INVERTSELECTION,
	MENU_FILES_RESTORESELECTION
};

enum enumCommandsMenu
{
	MENU_COMMANDS_FINDFILE,
	MENU_COMMANDS_HISTORY,
	MENU_COMMANDS_VIDEOMODE,
	MENU_COMMANDS_FINDFOLDER,
	MENU_COMMANDS_VIEWHISTORY,
	MENU_COMMANDS_FOLDERHISTORY,
	MENU_COMMANDS_SEPARATOR1,
	MENU_COMMANDS_SWAPPANELS,
	MENU_COMMANDS_HORZPANELS,
	MENU_COMMANDS_TOGGLEPANELS,
	MENU_COMMANDS_COMPAREFOLDERS,
	MENU_COMMANDS_COMPAREFILES,
	MENU_COMMANDS_SEPARATOR2,
	MENU_COMMANDS_EDITUSERMENU,
	MENU_COMMANDS_FILEASSOCIATIONS,
	MENU_COMMANDS_FOLDERSHORTCUTS,
	MENU_COMMANDS_FILTER,
	MENU_COMMANDS_SEPARATOR3,
	MENU_COMMANDS_PLUGINCOMMANDS,
	MENU_COMMANDS_WINDOWSLIST,
	MENU_COMMANDS_PROCESSLIST,
	MENU_COMMANDS_SEPARATOR4,
	MENU_COMMANDS_FARCONFIG,
	MENU_COMMANDS_MACROBROWSER,
	MENU_COMMANDS_ABOUTFAR,
	MENU_COMMANDS_HOTPLUGLIST,
	MENU_PANELHELP,
	MENU_PANELEXIT
};

enum enumOptionsMenu
{
	MENU_OPTIONS_SYSTEMSETTINGS,
	MENU_OPTIONS_PANELSETTINGS,
	MENU_OPTIONS_INTERFACESETTINGS,
	MENU_OPTIONS_LANGUAGES,
	MENU_OPTIONS_INPUTSETTINGS,
	MENU_OPTIONS_PLUGINSCONFIG,
	MENU_OPTIONS_PLUGINSMANAGERSETTINGS,
	MENU_OPTIONS_DIALOGSETTINGS,
	MENU_OPTIONS_VMENUSETTINGS,
	MENU_OPTIONS_CMDLINESETTINGS,
	MENU_OPTIONS_AUTOCOMPLETESETTINGS,
	MENU_OPTIONS_MASKGROUPS,
	//	MENU_OPTIONS_INFOPANELSETTINGS,
	MENU_OPTIONS_SEPARATOR1,
	MENU_OPTIONS_CONFIRMATIONS,
	MENU_OPTIONS_FILEPANELMODES,
	MENU_OPTIONS_FILEDESCRIPTIONS,
	MENU_OPTIONS_FOLDERINFOFILES,
	MENU_OPTIONS_SEPARATOR2,
	MENU_OPTIONS_VIEWERSETTINGS,
	MENU_OPTIONS_EDITORSETTINGS,
	MENU_OPTIONS_CODEPAGESSETTINGS,
	MENU_OPTIONS_SEPARATOR3,
	MENU_OPTIONS_COLORS,
	MENU_OPTIONS_FILESHIGHLIGHTING,
	MENU_OPTIONS_NOTIFICATIONSSETTINGS,
	MENU_OPTIONS_SEPARATOR4,
	MENU_OPTIONS_SAVESETUP
};

enum enumNavigateMenu {
	MENU_PANELGOTOHOME,
	MENU_PANELGOTOTOP,
	MENU_PANELUPTO,
	MENU_PANELGOTOMOUNTPOINT,
	MENU_PANELGODEEPER,
	MENU_PANELREVERTSYMLINKTRAVERSE,
	MENU_PANELNAVSEPARATOR1,
	MENU_PANELPINSHORTCUT0,
	MENU_PANELPINSHORTCUT1,
	MENU_PANELPINSHORTCUT2,
	MENU_PANELPINSHORTCUT3,
	MENU_PANELPINSHORTCUT4,
	MENU_PANELPINSHORTCUT5,
	MENU_PANELPINSHORTCUT6,
	MENU_PANELPINSHORTCUT7,
	MENU_PANELPINSHORTCUT8,
	MENU_PANELPINSHORTCUT9,
	MENU_PANELNAVSEPARATOR2,
	MENU_PANELGOTOSHORTCUT0,
	MENU_PANELGOTOSHORTCUT1,
	MENU_PANELGOTOSHORTCUT2,
	MENU_PANELGOTOSHORTCUT3,
	MENU_PANELGOTOSHORTCUT4,
	MENU_PANELGOTOSHORTCUT5,
	MENU_PANELGOTOSHORTCUT6,
	MENU_PANELGOTOSHORTCUT7,
	MENU_PANELGOTOSHORTCUT8,
	MENU_PANELGOTOSHORTCUT9
};

enum enumObjectsMenu {
	MENU_PANELSELECTFILE,
	MENU_PANELUNSELECT,
	MENU_PANELSELECT,
	MENU_PANELUNSELECTBYEXT,
	MENU_PANELSELECTBYEXT,
	MENU_PANELUNSELECTBYNAME,
	MENU_PANELSELECTBYNAME,
	//MENU_FOLDERTREENEXT,
	//MENU_FOLDERTREEPREV,
	MENU_PANELOBJSEPARATOR1,
	MENU_PANELEXECUTE,
	MENU_PANELUSERMENU,
	MENU_PANELCOPYTO,
	MENU_PANELRENAMETO,
	MENU_PANELREMOVECURRENT,
	MENU_PANELREMOVEPERMANENTLY,
	MENU_PANELPRINT,
	MENU_PANELOBJSEPARATOR2,
	MENU_PANELCOPYTOCMDLINE,
	//MENU_PANELCOPYCLIPBOARD,
	MENU_PANELCOPYNAMES,
	MENU_PANELCOPYFULLNAMES,
	MENU_PANELCOPYNAMESACTIVE,
	MENU_PANELCOPYNAMESPASSIVE,
	MENU_PANELCOPYUNCNAMES,
	MENU_PANELCOPYFNACTIVEREAL,
	MENU_PANELCOPYFNPASSIVEREAL,
	MENU_PANELCOPYFNLEFT,
	MENU_PANELCOPYFNRIGHT
};

enum enumViewMenu {
	MENU_PANELSHOWHIDDEN,
	MENU_PANELSHOWFILEMARKS,
	MENU_PANELSHOWAUTOCOLUMNWIDTH,
	MENU_PANELSHOWFILEMARKSSTSTUS,
	MENU_PANELSHIFTLONGTEXTLEFT,
	MENU_PANELSHIFTLONGTEXTRIGHT,
	MENU_PANELVIEWSEPARATOR1,
	MENU_PANELSORTBYNAME,
	MENU_PANELSORTBYEXT,
	MENU_PANELSORTBYMTIME,
	MENU_PANELSORTBYSIZE,
	MENU_PANELUNSORT,
	MENU_PANELSORTBYCTIME,
	MENU_PANELSORTBYATIME,
	MENU_PANELSORTBYDIZ,
	MENU_PANELSORTBYOWNER,
	MENU_PANELSORTBYGROUPS,
	MENU_PANELSORTBYSELECTED,
	MENU_PANELVIEWSEPARATOR2,
	MENU_PANELTOGGLEFULLSCREEN,
	MENU_PANELDIRECTORYNAMESETTINGS
};

void SetLeftRightMenuChecks(MenuDataEx *pMenu, bool bLeft)
{
	Panel *pPanel = bLeft ? CtrlObject->Cp()->LeftPanel : CtrlObject->Cp()->RightPanel;

	switch (pPanel->GetType()) {
		case FILE_PANEL: {
			int MenuLine = pPanel->GetViewMode() - VIEW_0;

			if (MenuLine <= MENU_LEFT_ALTERNATIVEVIEW) {
				if (!MenuLine)
					pMenu[MENU_LEFT_ALTERNATIVEVIEW].SetCheck(1);
				else
					pMenu[MenuLine - 1].SetCheck(1);
			}
		} break;
		case INFO_PANEL:
			pMenu[MENU_LEFT_INFOPANEL].SetCheck(1);
			break;
		case TREE_PANEL:
			pMenu[MENU_LEFT_TREEPANEL].SetCheck(1);
			break;
		case QVIEW_PANEL:
			pMenu[MENU_LEFT_QUICKVIEW].SetCheck(1);
			break;
	}
}

void ShellOptions(int LastCommand, MOUSE_EVENT_RECORD *MouseEvent)
{
	MenuDataEx LeftMenu[] = {
		{Msg::MenuBriefView,       LIF_SELECTED,  KEY_CTRL1  },
		{Msg::MenuMediumView,      0,             KEY_CTRL2  },
		{Msg::MenuFullView,        0,             KEY_CTRL3  },
		{Msg::MenuWideView,        0,             KEY_CTRL4  },
		{Msg::MenuDetailedView,    0,             KEY_CTRL5  },
		{Msg::MenuDizView,         0,             KEY_CTRL6  },
		{Msg::MenuLongDizView,     0,             KEY_CTRL7  },
		{Msg::MenuOwnersView,      0,             KEY_CTRL8  },
		{Msg::MenuLinksView,       0,             KEY_CTRL9  },
		{Msg::MenuAlternativeView, 0,             KEY_CTRL0  },
		{L"",                      LIF_SEPARATOR, 0          },
		{Msg::MenuInfoPanel,       0,             KEY_CTRLL  },
		{Msg::MenuTreePanel,       0,             KEY_CTRLT  },
		{Msg::MenuQuickView,       0,             KEY_CTRLQ  },
		{L"",                      LIF_SEPARATOR, 0          },
		{Msg::MenuSortModes,       0,             KEY_CTRLF12},
		{Msg::MenuTogglePanel,     0,             KEY_CTRLF1 },
		{Msg::MenuReread,          0,             KEY_CTRLR  },
		{Msg::MenuChangeDrive,     0,             KEY_ALTF1  }
	};
	MenuDataEx FilesMenu[] = {
		{Msg::MenuView,             LIF_SELECTED,  KEY_F3      },
		{Msg::MenuEdit,             0,             KEY_F4      },
		{Msg::MenuCopy,             0,             KEY_F5      },
		{Msg::MenuMove,             0,             KEY_F6      },
		{Msg::MenuLink,             0,             KEY_ALTF6   },
		{Msg::MenuCreateFolder,     0,             KEY_F7      },
		{Msg::MenuDelete,           0,             KEY_F8      },
		{Msg::MenuWipe,             0,             KEY_ALTDEL  },
		{L"",                       LIF_SEPARATOR, 0           },
		{Msg::MenuAdd,              0,             KEY_SHIFTF1 },
		{Msg::MenuExtract,          0,             KEY_SHIFTF2 },
		{Msg::MenuArchiveCommands,  0,             KEY_SHIFTF3 },
		{L"",                       LIF_SEPARATOR, 0           },
		{Msg::MenuAttributes,       0,             KEY_CTRLA   },
		{Msg::MenuChattr,           0,             KEY_CTRLALTA},
		{Msg::MenuApplyCommand,     0,             KEY_CTRLG   },
		{Msg::MenuDescribe,         0,             KEY_CTRLZ   },
		{L"",                       LIF_SEPARATOR, 0           },
		{Msg::MenuSelectGroup,      0,             KEY_ADD     },
		{Msg::MenuUnselectGroup,    0,             KEY_SUBTRACT},
		{Msg::MenuInvertSelection,  0,             KEY_MULTIPLY},
		{Msg::MenuRestoreSelection, 0,             KEY_CTRLM   }
	};
	MenuDataEx CmdMenu[] = {
		{Msg::MenuFindFile,         LIF_SELECTED,  KEY_ALTF7 },
		{Msg::MenuHistory,          0,             KEY_ALTF8 },
		{Msg::MenuVideoMode,        0,             KEY_ALTF9 },
		{Msg::MenuFindFolder,       0,             KEY_ALTF10},
		{Msg::MenuViewHistory,      0,             KEY_ALTF11},
		{Msg::MenuFoldersHistory,   0,             KEY_ALTF12},
		{L"",                       LIF_SEPARATOR, 0         },
		{Msg::MenuSwapPanels,       0,             KEY_CTRLU },
		{ Opt.PanelsDisposition ? Msg::MenuVerticalPanels : Msg::MenuHorizontalPanels, 0,(KEY_CTRL + KEY_COMMA) },
		{Msg::MenuTogglePanels,     0,             KEY_CTRLO },
		{Msg::MenuCompareFolders,   0,             0         },
		{Msg::MenuCompareFiles,     0,             KEY_CTRLD},
		{L"",                       LIF_SEPARATOR, 0         },
		{Msg::MenuUserMenu,         0,             0         },
		{Msg::MenuFileAssociations, 0,             0         },
		{Msg::MenuBookmarks,        0,             0         },
		{Msg::MenuFilter,           0,             KEY_CTRLI },
		{L"",                       LIF_SEPARATOR, 0         },
		{Msg::MenuPluginCommands,   0,             KEY_F11   },
		{Msg::MenuWindowsList,      0,             KEY_F12   },
		{Msg::MenuProcessList,      0,             KEY_CTRLW },
		{L"",                       LIF_SEPARATOR, 0         },
		{Msg::MenuFarConfig,        0,             0         },
		{Msg::MenuMacroBrowser,     0,             0         },
		{Msg::MenuAboutFar,         0,             0         },
		{Msg::PanelHelp,	0,	KEY_F1  },
		{Msg::PanelExit,	0,	KEY_F10  }
	};
	MenuDataEx OptionsMenu[] = {
		{Msg::MenuSystemSettings,         LIF_SELECTED,  0          },
		{Msg::MenuPanelSettings,          0,             0          },
		{Msg::MenuInterface,              0,             0          },
		{Msg::MenuLanguages,              0,             0          },
		{Msg::MenuInput,                  0,             0          },
		{Msg::MenuPluginsConfig,          0,             0          },
		{Msg::MenuPluginsManagerSettings, 0,             0          },
		{Msg::MenuDialogSettings,         0,             0          },
		{Msg::MenuVMenuSettings,          0,             0          },
		{Msg::MenuCmdlineSettings,        0,             0          },
		{Msg::MenuAutoCompleteSettings,   0,             0          },
		{Msg::MenuMaskGroups,             0,             0          },
		{L"",                             LIF_SEPARATOR, 0          },
		{Msg::MenuConfirmation,           0,             0          },
		{Msg::MenuFilePanelModes,         0,             0          },
		{Msg::MenuFileDescriptions,       0,             0          },
		{Msg::MenuFolderInfoFiles,        0,             0          },
		{L"",                             LIF_SEPARATOR, 0          },
		{Msg::MenuViewer,                 0,             0          },
		{Msg::MenuEditor,                 0,             0          },
		{Msg::MenuCodePages,              0,             0          },
		{L"",                             LIF_SEPARATOR, 0          },
		{Msg::MenuColors,                 0,             0          },
		{Msg::MenuFilesHighlighting,      0,             0          },
		{Msg::MenuNotifications,          0,             0          },
		{L"",                             LIF_SEPARATOR, 0          },
		{Msg::MenuSaveSetup,              0,             KEY_SHIFTF9}
	};
	MenuDataEx ObjectsMenu[] = {
		{Msg::PanelSelectFile,	0,	KEY_INS  },
		{Msg::PanelUnselect,	0,	KEY_SHIFTSUBTRACT  },
		{Msg::PanelSelect,	0,	KEY_SHIFTADD  },
		{Msg::PanelUnselectByExt,	0,	KEY_CTRLSUBTRACT  },
		{Msg::PanelSelectByExt,	0,	KEY_CTRLADD  },
		{Msg::PanelUnselectByName,	0,	KEY_ALTSUBTRACT  },
		{Msg::PanelSelectByName,	0,	KEY_ALTADD  },
		//{Msg::FolderTreeNext,	0,	KEY_CTRLENTER  },
		//{Msg::FolderTreePrev,	0,	KEY_CTRLSHIFTENTER  },
		{L"",	LIF_SEPARATOR,	0  },
		{Msg::PanelExecute,	0,	KEY_SHIFTENTER  },
		{Msg::PanelLaunchUserMenu, 0, KEY_F2 },
		{Msg::PanelCopyTo,	0,	KEY_SHIFTF5  },
		{Msg::PanelRenameTo,	0,	KEY_SHIFTF6  },
		{Msg::PanelRemoveCurrent,	0,	KEY_SHIFTF8  },
		{Msg::PanelRemovePermanently,	0,	KEY_SHIFTDEL  },
		{Msg::PanelPrint,	0,	KEY_ALTF5  },
		{L"",	LIF_SEPARATOR,	0  },
		{Msg::PanelCopyToCmdLine,	0,	KEY_CTRLENTER  },
		//{Msg::PanelCopyClipboard,	0,	KEY_CTRLALTINS  },
		{Msg::PanelCopyNames,	0,	KEY_CTRLINS  },
		{Msg::PanelCopyFullNames,	0,	KEY_CTRLALTINS  },
		{Msg::PanelCopyNamesActive,	0,	KEY_CTRLF  },
		{Msg::PanelCopyNamesPassive,	0,	KEY_CTRL | KEY_SEMICOLON  },
		{Msg::PanelCopyUncNames,	0,	KEY_CTRLALTF  },
		{Msg::PanelCopyFnActiveReal,	0,	KEY_ALTSHIFTBRACKET  },
		{Msg::PanelCopyFnPassiveReal,	0,	KEY_ALTSHIFTBACKBRACKET  },
		{Msg::PanelCopyFnLeft,	0,	KEY_CTRLBRACKET  },
		{Msg::PanelCopyFnRight,	0,	KEY_CTRLBACKBRACKET  }
	};
	MenuDataEx NavigateMenu[] = {
		{Msg::PanelGoToHome,	0,	KEY_CTRL | '`'  },
		{Msg::PanelGoToTop,	0,	KEY_CTRLBACKSLASH  },
		{Msg::PanelUpTo,	0,	KEY_CTRLPGUP  },
		{Msg::PanelGoToMountPoint,	0,	KEY_ALT | KEY_CTRLBACKSLASH  },
		{Msg::PanelGoDeeper,	0,	KEY_CTRLPGDN  },
		{Msg::PanelRevertSymlinkTraverse,	0,	KEY_CTRLSHIFTPGUP  },
		{L"",	LIF_SEPARATOR,	0  },
		{Msg::PanelPinShortcut0,	0,	KEY_CTRLSHIFT0  },
		{Msg::PanelPinShortcut1,	0,	KEY_CTRLSHIFT1  },
		{Msg::PanelPinShortcut2,	0,	KEY_CTRLSHIFT2  },
		{Msg::PanelPinShortcut3,	0,	KEY_CTRLSHIFT3  },
		{Msg::PanelPinShortcut4,	0,	KEY_CTRLSHIFT4  },
		{Msg::PanelPinShortcut5,	0,	KEY_CTRLSHIFT5  },
		{Msg::PanelPinShortcut6,	0,	KEY_CTRLSHIFT6  },
		{Msg::PanelPinShortcut7,	0,	KEY_CTRLSHIFT7  },
		{Msg::PanelPinShortcut8,	0,	KEY_CTRLSHIFT8  },
		{Msg::PanelPinShortcut9,	0,	KEY_CTRLSHIFT9  },
		{L"",	LIF_SEPARATOR,	0  },
		{Msg::PanelGotoShortcut0,	0,	KEY_RCTRL0  },
		{Msg::PanelGotoShortcut1,	0,	KEY_RCTRL1  },
		{Msg::PanelGotoShortcut2,	0,	KEY_RCTRL2  },
		{Msg::PanelGotoShortcut3,	0,	KEY_RCTRL3  },
		{Msg::PanelGotoShortcut4,	0,	KEY_RCTRL4  },
		{Msg::PanelGotoShortcut5,	0,	KEY_RCTRL5  },
		{Msg::PanelGotoShortcut6,	0,	KEY_RCTRL6  },
		{Msg::PanelGotoShortcut7,	0,	KEY_RCTRL7  },
		{Msg::PanelGotoShortcut8,	0,	KEY_RCTRL8  },
		{Msg::PanelGotoShortcut9,	0,	KEY_RCTRL9  }
	};
	MenuDataEx ViewMenu[] = {
		{Msg::PanelShowHidden,	0,	KEY_CTRLH  },
		{Msg::PanelShowFileMarks,	0,	KEY_CTRLALTM  },
		{Msg::PanelShowAutoColumnWidth,	0,	KEY_CTRLALTL  },
		{Msg::PanelShowFileMarksStstus,	0,	KEY_CTRLALTN  },
		{Msg::PanelShiftLongTextLeft,	0,	KEY_ALTLEFT  },
		{Msg::PanelShiftLongTextRight,	0,	KEY_ALTRIGHT  },
		{L"",	LIF_SEPARATOR,	0  },
		{Msg::PanelSortByName,	0,	KEY_CTRLF3  },
		{Msg::PanelSortByExt,	0,	KEY_CTRLF4  },
		{Msg::PanelSortByMTime,	0,	KEY_CTRLF5  },
		{Msg::PanelSortBySize,	0,	KEY_CTRLF6  },
		{Msg::PanelUnsort,	0,	KEY_CTRLF7  },
		{Msg::PanelSortByCTime,	0,	KEY_CTRLF8  },
		{Msg::PanelSortByATime,	0,	KEY_CTRLF9  },
		{Msg::PanelSortByDiz,	0,	KEY_CTRLF10  },
		{Msg::PanelSortByOwner,	0,	KEY_CTRLF11  },
		{Msg::PanelSortByGroups,	0,	KEY_SHIFTF11  },
		{Msg::PanelSortBySelected,	0,	KEY_SHIFTF12  },
		{L"",	LIF_SEPARATOR,	0  },
		{Msg::PanelToggleFullScreen,	0,	KEY_ALTF9  },
		{Msg::PanelDirectoryNameSettings,	0,	KEY_CTRLALTD  }
	};
	MenuDataEx RightMenu[] = {
		{Msg::MenuBriefView,        LIF_SELECTED,  KEY_CTRL1  },
		{Msg::MenuMediumView,       0,             KEY_CTRL2  },
		{Msg::MenuFullView,         0,             KEY_CTRL3  },
		{Msg::MenuWideView,         0,             KEY_CTRL4  },
		{Msg::MenuDetailedView,     0,             KEY_CTRL5  },
		{Msg::MenuDizView,          0,             KEY_CTRL6  },
		{Msg::MenuLongDizView,      0,             KEY_CTRL7  },
		{Msg::MenuOwnersView,       0,             KEY_CTRL8  },
		{Msg::MenuLinksView,        0,             KEY_CTRL9  },
		{Msg::MenuAlternativeView,  0,             KEY_CTRL0  },
		{L"",                       LIF_SEPARATOR, 0          },
		{Msg::MenuInfoPanel,        0,             KEY_CTRLL  },
		{Msg::MenuTreePanel,        0,             KEY_CTRLT  },
		{Msg::MenuQuickView,        0,             KEY_CTRLQ  },
		{L"",                       LIF_SEPARATOR, 0          },
		{Msg::MenuSortModes,        0,             KEY_CTRLF12},
		{Msg::MenuTogglePanelRight, 0,             KEY_CTRLF2 },
		{Msg::MenuReread,           0,             KEY_CTRLR  },
		{Msg::MenuChangeDriveRight, 0,             KEY_ALTF2  }
	};
	
	HMenuData MainMenu[] = {
		{Msg::MenuLeftTitle,     1, LeftMenu,    ARRAYSIZE(LeftMenu),    L"LeftRightMenu"},
		{Msg::MenuFilesTitle,    0, FilesMenu,   ARRAYSIZE(FilesMenu),   L"FilesMenu"    },
		{Msg::MenuCommandsTitle, 0, CmdMenu,     ARRAYSIZE(CmdMenu),     L"CmdMenu"      },
		{Msg::MenuObjectTitle, 0, ObjectsMenu,     ARRAYSIZE(ObjectsMenu),     L"ObjectsMenu"      },
		{Msg::MenuOptionsTitle,  0, OptionsMenu, ARRAYSIZE(OptionsMenu), L"OptMenu"      },
		{Msg::MenuNavigateTitle, 0, NavigateMenu,     ARRAYSIZE(NavigateMenu),     L"NavigateMenu"      },
		{Msg::MenuViewTitle, 0, ViewMenu,     ARRAYSIZE(ViewMenu),     L"ViewMenu"      },
		{Msg::MenuRightTitle,    0, RightMenu,   ARRAYSIZE(RightMenu),   L"LeftRightMenu"}
	};

	static int LastHItem = -1, LastVItem = 0;
	int HItem, VItem;

	if (Opt.Policies.DisabledOptions) {
		for (size_t I = 0; I < ARRAYSIZE(OptionsMenu); ++I) {
			if (I >= MENU_OPTIONS_CONFIRMATIONS)
				OptionsMenu[I].SetGrayed((Opt.Policies.DisabledOptions >> (I - 1)) & 1);
			else
				OptionsMenu[I].SetGrayed((Opt.Policies.DisabledOptions >> I) & 1);
		}
	}

	SetLeftRightMenuChecks(LeftMenu, true);
	SetLeftRightMenuChecks(RightMenu, false);
	// Навигация по меню
	{
		int lastHMenuPos = ARRAYSIZE(MainMenu) - 1;
		HMenu HOptMenu(MainMenu, ARRAYSIZE(MainMenu));
		HOptMenu.SetHelp(L"Menus");
		HOptMenu.SetPosition(0, 0, ScrX, 0);

		if (LastCommand) {
			MenuDataEx *VMenuTable[] = {LeftMenu, FilesMenu, CmdMenu, ObjectsMenu, OptionsMenu, NavigateMenu, ViewMenu, RightMenu};
			int HItemToShow = LastHItem;

			if (HItemToShow == -1) {
				if (CtrlObject->Cp()->ActivePanel == CtrlObject->Cp()->RightPanel
						&& CtrlObject->Cp()->ActivePanel->IsVisible())
					HItemToShow = lastHMenuPos;
				else
					HItemToShow = 0;
			}

			MainMenu[0].Selected = 0;
			MainMenu[HItemToShow].Selected = 1;
			VMenuTable[HItemToShow][0].SetSelect(0);
			VMenuTable[HItemToShow][LastVItem].SetSelect(1);
			HOptMenu.Show();
			{
				ChangeMacroMode MacroMode(MACRO_MAINMENU);
				HOptMenu.ProcessKey(KEY_DOWN);
			}
		} else {
			if (CtrlObject->Cp()->ActivePanel == CtrlObject->Cp()->RightPanel
					&& CtrlObject->Cp()->ActivePanel->IsVisible()) {
				MainMenu[0].Selected = 0;
				MainMenu[lastHMenuPos].Selected = 1;
			}
		}

		if (MouseEvent) {
			ChangeMacroMode MacroMode(MACRO_MAINMENU);
			HOptMenu.Show();
			HOptMenu.ProcessMouse(MouseEvent);
		}

		{
			ChangeMacroMode MacroMode(MACRO_MAINMENU);
			HOptMenu.Process();
		}

		HOptMenu.GetExitCode(HItem, VItem);
	}

	FarKey key = 0;

	if (HItem >= 0 && VItem >= 0) {
		key = MainMenu[HItem].SubMenu[VItem].AccelKey;
	}

	// "Исполнятор команд меню"
	switch (HItem) {
		case MENU_LEFT:
		case MENU_RIGHT: {
			Panel *pPanel = (HItem == MENU_LEFT) ? CtrlObject->Cp()->LeftPanel : CtrlObject->Cp()->RightPanel;

			if (VItem >= MENU_LEFT_BRIEFVIEW && VItem <= MENU_LEFT_ALTERNATIVEVIEW) {
				CtrlObject->Cp()->ChangePanelToFilled(pPanel, FILE_PANEL);
				pPanel = (HItem == MENU_LEFT) ? CtrlObject->Cp()->LeftPanel : CtrlObject->Cp()->RightPanel;
				pPanel->SetViewMode((VItem == MENU_LEFT_ALTERNATIVEVIEW) ? VIEW_0 : VIEW_1 + VItem);
			} else {
				switch (VItem) {
					case MENU_LEFT_INFOPANEL:	// Info panel
						CtrlObject->Cp()->ChangePanelToFilled(pPanel, INFO_PANEL);
						break;
					case MENU_LEFT_TREEPANEL:	// Tree panel
						CtrlObject->Cp()->ChangePanelToFilled(pPanel, TREE_PANEL);
						break;
					case MENU_LEFT_QUICKVIEW:	// Quick view
						CtrlObject->Cp()->ChangePanelToFilled(pPanel, QVIEW_PANEL);
						break;
					case MENU_LEFT_SORTMODES:	// Sort modes
						pPanel->ProcessKey(KEY_CTRLF12);
						break;
					case MENU_LEFT_TOGGLEPANEL:		// Panel On/Off
						FrameManager->ProcessKey((HItem == MENU_LEFT) ? KEY_CTRLF1 : KEY_CTRLF2);
						break;
					case MENU_LEFT_REREAD:	// Re-read
						pPanel->ProcessKey(KEY_CTRLR);
						break;
					case MENU_LEFT_CHANGEDRIVE:		// Change drive
						pPanel->ChangeDisk();
						break;
				}
			}

			break;
		}
		case MENU_FILES: {
			switch (VItem) {
				case MENU_FILES_VIEW:	// View
					FrameManager->ProcessKey(KEY_F3);
					break;
				case MENU_FILES_EDIT:	// Edit
					FrameManager->ProcessKey(KEY_F4);
					break;
				case MENU_FILES_COPY:	// Copy
					FrameManager->ProcessKey(KEY_F5);
					break;
				case MENU_FILES_MOVE:	// Rename or move
					FrameManager->ProcessKey(KEY_F6);
					break;
				case MENU_FILES_LINK:	// Make link
					FrameManager->ProcessKey(KEY_ALTF6);
					break;
				case MENU_FILES_CREATEFOLDER:	// Make folder
					FrameManager->ProcessKey(KEY_F7);
					break;
				case MENU_FILES_DELETE:		// Delete
					FrameManager->ProcessKey(KEY_F8);
					break;
				case MENU_FILES_WIPE:	// Wipe
					FrameManager->ProcessKey(KEY_ALTDEL);
					break;
				case MENU_FILES_ADD:	// Add to archive
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_SHIFTF1);
					break;
				case MENU_FILES_EXTRACT:	// Extract files
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_SHIFTF2);
					break;
				case MENU_FILES_ARCHIVECOMMANDS:	// Archive commands
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_SHIFTF3);
					break;
				case MENU_FILES_ATTRIBUTES:		// File attributes
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_CTRLA);
					break;
				case MENU_FILES_CHATTR:		// chattr
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_CTRLALTA);
					break;
				case MENU_FILES_APPLYCOMMAND:	// Apply command
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_CTRLG);
					break;
				case MENU_FILES_DESCRIBE:	// Describe files
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_CTRLZ);
					break;
				case MENU_FILES_SELECTGROUP:	// Select group
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_ADD);
					break;
				case MENU_FILES_UNSELECTGROUP:	// Unselect group
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_SUBTRACT);
					break;
				case MENU_FILES_INVERTSELECTION:	// Invert selection
					CtrlObject->Cp()->ActivePanel->ProcessKey(KEY_MULTIPLY);
					break;
				case MENU_FILES_RESTORESELECTION:	// Restore selection
					CtrlObject->Cp()->ActivePanel->RestoreSelection();
					break;
			}

			break;
		}
		case MENU_COMMANDS: {
			switch (VItem) {
				case MENU_COMMANDS_FINDFILE:	// Find file
					FrameManager->ProcessKey(KEY_ALTF7);
					break;
				case MENU_COMMANDS_HISTORY:		// History
					FrameManager->ProcessKey(KEY_ALTF8);
					break;
				case MENU_COMMANDS_VIDEOMODE:	// Video mode
					FrameManager->ProcessKey(KEY_ALTF9);
					break;
				case MENU_COMMANDS_FINDFOLDER:	// Find folder
					FrameManager->ProcessKey(KEY_ALTF10);
					break;
				case MENU_COMMANDS_VIEWHISTORY:		// File view history
					FrameManager->ProcessKey(KEY_ALTF11);
					break;
				case MENU_COMMANDS_FOLDERHISTORY:	// Folders history
					FrameManager->ProcessKey(KEY_ALTF12);
					break;
				case MENU_COMMANDS_SWAPPANELS:	// Swap panels
					FrameManager->ProcessKey(KEY_CTRLU);
					break;
				case MENU_COMMANDS_HORZPANELS:	// Hrz/Vert panels disposition
					FrameManager->ProcessKey(KEY_CTRL + KEY_COMMA);
					break;
				case MENU_COMMANDS_TOGGLEPANELS:	// Panels On/Off
					FrameManager->ProcessKey(KEY_CTRLO);
					break;
				case MENU_COMMANDS_COMPAREFOLDERS:	// Compare folders
					CtrlObject->Cp()->ActivePanel->CompareDir();
					break;
				case MENU_COMMANDS_COMPAREFILES:	// Compare files
					PresentFileDiff();
					break;
				case MENU_COMMANDS_EDITUSERMENU:	// Edit user menu
				{
					UserMenu::Present(true);
				} break;
				case MENU_COMMANDS_FILEASSOCIATIONS:	// File associations
					EditFileTypes();
					break;
				case MENU_COMMANDS_FOLDERSHORTCUTS:		// Folder shortcuts
					ShowBookmarksMenu();
					break;
				case MENU_COMMANDS_FILTER:	// File panel filter
					CtrlObject->Cp()->ActivePanel->EditFilter();
					break;
				case MENU_COMMANDS_PLUGINCOMMANDS:	// Plugin commands
					FrameManager->ProcessKey(KEY_F11);
					break;
				case MENU_COMMANDS_WINDOWSLIST:		// Screens list
					FrameManager->ProcessKey(KEY_F12);
					break;
				case MENU_COMMANDS_PROCESSLIST:		// Task list
					ShowProcessList();
					break;
				case MENU_COMMANDS_FARCONFIG:		// far:config
					ConfigOptEdit();
					break;
				case MENU_COMMANDS_MACROBROWSER:
					CtrlObject->Macro.MacroBrowser();
					break;
				case MENU_COMMANDS_ABOUTFAR:		// far:about
					void FarAbout(PluginManager &Plugins);
					FarAbout(CtrlObject->Plugins);
					break;
				case MENU_COMMANDS_HOTPLUGLIST:		// HotPlug list
													//					ShowHotplugDevice();
					break;
				default:
					if (key) FrameManager->ProcessKey(key);
					break;
			}

			break;
		}
		case MENU_OPTIONS: {
			switch (VItem) {
				case MENU_OPTIONS_SYSTEMSETTINGS:	// System settings
					SystemSettings();
					break;
				case MENU_OPTIONS_PANELSETTINGS:	// Panel settings
					PanelSettings();
					break;
				case MENU_OPTIONS_INTERFACESETTINGS:	// Interface settings
					InterfaceSettings();
					break;
				case MENU_OPTIONS_LANGUAGES:	// Languages
					LanguageSettings();
					break;
				case MENU_OPTIONS_INPUTSETTINGS:	// Input settings
					InputSettings();
					break;
				case MENU_OPTIONS_PLUGINSCONFIG:	// Plugins configuration
					CtrlObject->Plugins.Configure();
					break;
				case MENU_OPTIONS_PLUGINSMANAGERSETTINGS:
					PluginsManagerSettings();
					break;
				case MENU_OPTIONS_DIALOGSETTINGS:	// Dialog settings (police=5)
					DialogSettings();
					break;
				case MENU_OPTIONS_VMENUSETTINGS:	// VMenu settings
					VMenuSettings();
					break;
				case MENU_OPTIONS_CMDLINESETTINGS:	// Command line settings
					CmdlineSettings();
					break;
				case MENU_OPTIONS_AUTOCOMPLETESETTINGS:
					AutoCompleteSettings();
					break;
					//				case MENU_OPTIONS_INFOPANELSETTINGS: // InfoPanel Settings
					//					InfoPanelSettings();
					//					break;
				case MENU_OPTIONS_MASKGROUPS:
					MaskGroupsSettings();
					break;
				case MENU_OPTIONS_CONFIRMATIONS:	// Confirmations
					SetConfirmations();
					break;
				case MENU_OPTIONS_FILEPANELMODES:	// File panel modes
					FileList::SetFilePanelModes();
					break;
				case MENU_OPTIONS_FILEDESCRIPTIONS:		// File descriptions
					SetDizConfig();
					break;
				case MENU_OPTIONS_FOLDERINFOFILES:	// Folder description files
					SetFolderInfoFiles();
					break;
				case MENU_OPTIONS_VIEWERSETTINGS:	// Viewer settings
					ViewerConfig(Opt.ViOpt);
					break;
				case MENU_OPTIONS_EDITORSETTINGS:	// Editor settings
					EditorConfig(Opt.EdOpt);
					break;
				case MENU_OPTIONS_NOTIFICATIONSSETTINGS:	// Notifications settings
					NotificationsConfig(Opt.NotifOpt);
					break;
				case MENU_OPTIONS_CODEPAGESSETTINGS:	// Code pages
					SelectCodePage(CP_AUTODETECT, true, true, true);
					break;
				case MENU_OPTIONS_COLORS:	// Colors
					SetColors();
					break;
				case MENU_OPTIONS_FILESHIGHLIGHTING:	// Files highlighting
					CtrlObject->HiFiles->HiEdit(0);
					break;
				case MENU_OPTIONS_SAVESETUP:	// Save setup
					ConfigOptSave(true);
					break;
			}

			break;
		}
		default:
			if (key) FrameManager->ProcessKey(key);
			break;
	}

	int _CurrentFrame = FrameManager->GetCurrentFrame()->GetType();
	// TODO:Здесь как то нужно изменить, чтобы учесть будущие новые типы полноэкранных фреймов
	// или то, что, скажем редактор/вьювер может быть не полноэкранным

	if (!(_CurrentFrame == MODALTYPE_VIEWER || _CurrentFrame == MODALTYPE_EDITOR))
		CtrlObject->CmdLine->Show();

	if (HItem != -1 && VItem != -1) {
		LastHItem = HItem;
		LastVItem = VItem;
	}
}
