/*
panel.cpp

Parent class для панелей
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

#include "panel.hpp"
#include "macroopcode.hpp"
#include "keyboard.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "vmenu.hpp"
#include "filepanels.hpp"
#include "cmdline.hpp"
#include "chgmmode.hpp"
#include "chgprior.hpp"
#include "edit.hpp"
#include "treelist.hpp"
#include "filelist.hpp"
#include "dialog.hpp"
#include "savescr.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "array.hpp"
#include "lockscrn.hpp"
#include "help.hpp"
#include "syslog.hpp"
#include "plugapi.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "Mounts.hpp"
#include "clipboard.hpp"
#include "config.hpp"
#include "scrsaver.hpp"
#include "execute.hpp"
#include "Bookmarks.hpp"
#include "options.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "constitle.hpp"
#include "DialogBuilder.hpp"
#include "setattr.hpp"
#include "palette.hpp"
#include "panel.hpp"
#include "drivemix.hpp"
#include "xlat.hpp"
#include "vt/vtshell.h"
#include <StackHeapArray.hpp>

static int DragX, DragY, DragMove;
static Panel *SrcDragPanel;
static SaveScreen *DragSaveScr = nullptr;
static FARString strDragName;

/*
	$ 21.08.2002 IS
	Класс для хранения пункта плагина в меню выбора дисков
*/
class ChDiskPluginItem
{
public:
	MenuItemEx Item;
	WCHAR HotKey;

	ChDiskPluginItem() { Clear(); }
	~ChDiskPluginItem() {}

	void Clear()
	{
		HotKey = 0;
		Item.Clear();
	}
	bool operator==(const ChDiskPluginItem &rhs) const
	{
		return HotKey == rhs.HotKey && !StrCmpI(Item.strName, rhs.Item.strName)
				&& Item.UserData == rhs.Item.UserData;
	}
	int operator<(const ChDiskPluginItem &rhs) const { return StrCmpI(Item.strName, rhs.Item.strName) < 0; }
	const ChDiskPluginItem &operator=(const ChDiskPluginItem &rhs);
};

const ChDiskPluginItem &ChDiskPluginItem::operator=(const ChDiskPluginItem &rhs)
{
	if (this != &rhs) {
		Item = rhs.Item;
		HotKey = rhs.HotKey;
	}

	return *this;
}

Panel::Panel()
	:
	Focus(0),
	EnableUpdate(TRUE),
	PanelMode(NORMAL_PANEL),
	PrevViewMode(VIEW_3),
	NumericSort(0),
	CaseSensitiveSort(0),
	DirectoriesFirst(1),
	ModalMode(0),
	ViewSettings(),
	ProcessingPluginCommand(0)
{
	_OT(SysLog(L"[%p] Panel::Panel()", this));
	SrcDragPanel = nullptr;
	DragX = DragY = -1;
};

Panel::~Panel()
{
	_OT(SysLog(L"[%p] Panel::~Panel()", this));
	EndDrag();
}

void Panel::SetViewMode(int ViewMode)
{
	PrevViewMode = ViewMode;
	Panel::ViewMode = ViewMode;
};

void Panel::ChangeDirToCurrent()
{
	FARString strNewDir;
	apiGetCurrentDirectory(strNewDir);
	SetCurDir(strNewDir, TRUE);
}

void Panel::ChangeDisk()
{
	int Pos = 0, FirstCall = TRUE;

	if (!strCurDir.IsEmpty() && strCurDir.At(1) == L':') {
		Pos = Upper(strCurDir.At(0)) - L'A';
	}

	while (Pos != -1) {
		Pos = ChangeDiskMenu(Pos, FirstCall);
		FirstCall = FALSE;
	}
}

struct PanelMenuItem
{
	enum Kind
	{
		UNSPECIFIED = 0,
		MOUNTPOINT,
		DIRECTORY,
		SHORTCUT,
		PLUGIN,
	} kind = UNSPECIFIED;

	Plugin *pPlugin = nullptr;

	INT_PTR nItem = -1;		// plugin item or shortcut index

	struct
	{
		wchar_t path[0x1000];
		int id;
	} location;
};

static void AddPluginItems(VMenu &ChDisk, int Pos)
{
	TArray<ChDiskPluginItem> MPItems;
	ChDiskPluginItem OneItem;
	// Список дополнительных хоткеев, для случая, когда плагинов, добавляющих пункт в меню, больше 9.
	int PluginItem, PluginNumber = 0;	// IS: счетчики - плагинов и пунктов плагина
	bool ItemPresent, Done = false;
	FARString strMenuText;
	FARString strPluginText;

	while (!Done) {
		for (PluginItem = 0;; ++PluginItem) {
			if (PluginNumber >= CtrlObject->Plugins.GetPluginsCount()) {
				Done = true;
				break;
			}

			Plugin *pPlugin = CtrlObject->Plugins.GetPlugin(PluginNumber);

			WCHAR HotKey = 0;
			if (!CtrlObject->Plugins.GetDiskMenuItem(pPlugin, PluginItem, ItemPresent, HotKey,
						strPluginText)) {
				Done = true;
				break;
			}

			if (!ItemPresent)
				break;

			strMenuText = strPluginText;

			if (!strMenuText.IsEmpty()) {
				OneItem.Clear();
				PanelMenuItem *item = new PanelMenuItem;
				item->kind = PanelMenuItem::PLUGIN;
				item->pPlugin = pPlugin;
				item->nItem = PluginItem;

				OneItem.Item.strName = strMenuText;
				OneItem.Item.UserDataSize = sizeof(PanelMenuItem);
				OneItem.Item.UserData = (char *)item;
				OneItem.HotKey = HotKey;
				ChDiskPluginItem *pResult = MPItems.addItem(OneItem);

				if (pResult) {
					pResult->Item.UserData = (char *)item;	// BUGBUG, это фантастика просто. Исправить!!!! связано с работой TArray
					pResult->Item.UserDataSize = sizeof(PanelMenuItem);
				}

				/*
				else BUGBUG, а вот это, похоже, лишнее
				{
					Done=TRUE;
					break;
				}
				*/
			}
		}	// END: for (PluginItem=0;;++PluginItem)

		++PluginNumber;
	}

	MPItems.Sort();
	MPItems.Pack();		// выкинем дубли
	size_t PluginMenuItemsCount = MPItems.getSize();

	if (PluginMenuItemsCount) {
		MenuItemEx ChDiskItem;

		ChDiskItem.Clear();
		ChDiskItem.strName = Msg::PluginsTitle;
		ChDiskItem.Flags|= LIF_SEPARATOR;
		ChDiskItem.UserDataSize = 0;
		ChDisk.AddItem(&ChDiskItem);
		ChDiskItem.Flags&= ~LIF_SEPARATOR;

		for (size_t I = 0; I < PluginMenuItemsCount; ++I) {
			MPItems.getItem(I)->Item.SetSelect(ChDisk.GetItemCount() == Pos);
			const wchar_t HotKeyStr[] = {MPItems.getItem(I)->HotKey ? L'&' : L' ',
					MPItems.getItem(I)->HotKey ? MPItems.getItem(I)->HotKey : L' ', L' ',
					MPItems.getItem(I)->HotKey ? L' ' : L'\0', L'\0'};
			MPItems.getItem(I)->Item.strName = FARString(HotKeyStr) + MPItems.getItem(I)->Item.strName;
			ChDisk.AddItem(&MPItems.getItem(I)->Item);

			delete (PanelMenuItem *)MPItems.getItem(I)->Item.UserData;	// ммда...
		}
	}
}

static void ConfigureChangeDriveMode()
{
	DialogBuilder Builder(Msg::ChangeDriveConfigure, L"ChangeLocationConfig");
	//	Builder.AddCheckbox(Msg::ChangeDriveShowDiskType, &Opt.ChangeDriveMode, DRIVE_SHOW_TYPE);
	//	Builder.AddCheckbox(Msg::ChangeDriveShowNetworkName, &Opt.ChangeDriveMode, DRIVE_SHOW_NETNAME);
	//	Builder.AddCheckbox(Msg::ChangeDriveShowLabel, &Opt.ChangeDriveMode, DRIVE_SHOW_LABEL);
	//	Builder.AddCheckbox(Msg::ChangeDriveShowFileSystem, &Opt.ChangeDriveMode, DRIVE_SHOW_FILESYSTEM);

	//	BOOL ShowSizeAny = Opt.ChangeDriveMode & (DRIVE_SHOW_SIZE | DRIVE_SHOW_SIZE_FLOAT);

	//	DialogItemEx *ShowSize = Builder.AddCheckbox(Msg::ChangeDriveShowSize, &ShowSizeAny);
	//	DialogItemEx *ShowSizeFloat = Builder.AddCheckbox(Msg::ChangeDriveShowSizeFloat, &Opt.ChangeDriveMode, DRIVE_SHOW_SIZE_FLOAT);
	//	ShowSizeFloat->Indent(3);
	//	Builder.LinkFlags(ShowSize, ShowSizeFloat, DIF_DISABLE);

	auto *ShowMountsItem =
			Builder.AddCheckbox(Msg::ChangeDriveShowMounts, &Opt.ChangeDriveMode, DRIVE_SHOW_MOUNTS);

	auto *EditItem = Builder.AddEditField(&Opt.ChangeDriveExceptions, 28);
	Builder.LinkFlags(ShowMountsItem, EditItem, DIF_DISABLE);
	Builder.AddTextBefore(EditItem, Msg::ChangeDriveExceptions);

	EditItem = Builder.AddEditField(&Opt.ChangeDriveColumn2, 28);
	Builder.LinkFlags(ShowMountsItem, EditItem, DIF_DISABLE);
	Builder.AddTextBefore(EditItem, Msg::ChangeDriveColumn2);

	EditItem = Builder.AddEditField(&Opt.ChangeDriveColumn3, 28);
	Builder.LinkFlags(ShowMountsItem, EditItem, DIF_DISABLE);
	Builder.AddTextBefore(EditItem, Msg::ChangeDriveColumn3);

	Builder.AddCheckbox(Msg::ChangeDriveShowShortcuts, &Opt.ChangeDriveMode, DRIVE_SHOW_BOOKMARKS);
	Builder.AddCheckbox(Msg::ChangeDriveShowPlugins, &Opt.ChangeDriveMode, DRIVE_SHOW_PLUGINS);
	//	Builder.AddCheckbox(Msg::ChangeDriveShowCD, &Opt.ChangeDriveMode, DRIVE_SHOW_CDROM);
	//	Builder.AddCheckbox(Msg::ChangeDriveShowNetworkDrive, &Opt.ChangeDriveMode, DRIVE_SHOW_REMOTE);

	Builder.AddOKCancel();
	if (Builder.ShowDialog()) {
		//		if (ShowSizeAny)
		//		{
		//			bool ShowSizeFloat = (Opt.ChangeDriveMode & DRIVE_SHOW_SIZE_FLOAT) ? true : false;
		//			if (ShowSizeFloat)
		//				Opt.ChangeDriveMode &= ~DRIVE_SHOW_SIZE;
		//			else
		//				Opt.ChangeDriveMode |= DRIVE_SHOW_SIZE;
		//		}
		//		else
		//			Opt.ChangeDriveMode &= ~(DRIVE_SHOW_SIZE | DRIVE_SHOW_SIZE_FLOAT);
	}
}

/*

LONG_PTR WINAPI ChDiskDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_CTLCOLORDLGITEM: {
			if (Param1 == 1)	// BUGBUG, magic number
			{
				uint64_t *ItemColor = reinterpret_cast<uint64_t *>(Param2);
				uint64_t color = FarColorToReal(COL_WARNDIALOGTEXT);
				ItemColor[0] = color;
				ItemColor[2] = color;
				return 1;
//				int Color = FarColorToReal(COL_WARNDIALOGTEXT);
//				return ((Param2 & 0xFF00FF00) | (Color << 16) | Color);
			}
		} break;
	}
	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

*/

static void AddBookmarkItems(VMenu &ChDisk, int Pos)
{
	Bookmarks b;
	for (int SCPos = 0, AddedCount = 0;; ++SCPos) {
		FARString Folder, Plugin, PluginFile, ShortcutPath;
		if (b.Get(SCPos, &Folder, &Plugin, &PluginFile, nullptr)) {
			MenuItemEx ChDiskItem;

			if (!AddedCount++) {
				ChDiskItem.Clear();
				ChDiskItem.strName = Msg::BookmarksTitle;
				ChDiskItem.Flags|= LIF_SEPARATOR;
				ChDiskItem.UserDataSize = 0;
				ChDisk.AddItem(&ChDiskItem);
				ChDiskItem.Flags&= ~LIF_SEPARATOR;
			}

			ChDiskItem.Clear();
			ChDiskItem.SetSelect(ChDisk.GetItemCount() == Pos);

			if (!PluginFile.IsEmpty()) {
				ShortcutPath+= PluginFile;
				ShortcutPath+= L"/";
			}
			ShortcutPath+= Folder;
			if (ShortcutPath.IsEmpty()) {
				ShortcutPath = L"@";
				ShortcutPath+= Plugin;
			}

			if (SCPos <= 9)
				ChDiskItem.strName.Format(L"&%d  ", SCPos);
			else
				ChDiskItem.strName = L"   ";

			ChDiskItem.strName+= TruncPathStr(ShortcutPath, 64);

			PanelMenuItem item;
			item.kind = PanelMenuItem::SHORTCUT;
			item.nItem = SCPos;
			ChDisk.SetUserData(&item, sizeof(item), ChDisk.AddItem(&ChDiskItem));
		} else if (SCPos > 10) {
			break;
		}
	}
}

int Panel::ChangeDiskMenu(int Pos, int FirstCall)
{
	/*Events.DeviceArchivalEvent.Reset();
	Events.DeviceRemoveEvent.Reset();
	Events.MediaArchivalEvent.Reset();
	Events.MediaRemoveEvent.Reset();*/
	class Guard_Macro_DskShowPosType	// фигня какая-то
	{
	public:
		Guard_Macro_DskShowPosType(Panel *curPanel)
		{
			Macro_DskShowPosType = (curPanel == CtrlObject->Cp()->LeftPanel) ? 1 : 2;
		}
		~Guard_Macro_DskShowPosType() { Macro_DskShowPosType = 0; }
	};
	Guard_Macro_DskShowPosType _guard_Macro_DskShowPosType(this);
	MenuItemEx ChDiskItem;
	FARString strDiskType, strRootDir, strDiskLetter;

	FARString curdir, another_curdir;
	GetCurDir(curdir);

	auto another_panel = CtrlObject->Cp()->GetAnotherPanel(this);
	another_panel->GetCurDirPluginAware(another_curdir);
	if (another_panel->GetPluginHandle() != INVALID_HANDLE_VALUE) {
		another_curdir.Insert(0, L"{");
		another_curdir.Append(L"}");
	}

	PanelMenuItem Item, *mitem = 0;
	{	// эта скобка надо, см. M#605
		VMenu ChDisk(Msg::ChangeDriveTitle, nullptr, 0, ScrY - Y1 - 3);
		ChDisk.SetBottomTitle(Msg::ChangeDriveMenuFooter);
		ChDisk.SetFlags(VMENU_NOTCENTER);

		if (this == CtrlObject->Cp()->LeftPanel)
			ChDisk.SetFlags(VMENU_LEFTMOST);

		ChDisk.SetHelp(L"DriveDlg");
		ChDisk.SetFlags(VMENU_WRAPMODE);
		Mounts::Enum mounts(another_curdir);
		for (const auto &m : mounts) {
			ChDiskItem.Clear();

			if (m.path == L"-") {
				ChDiskItem.strName = m.col3;
				ChDiskItem.Flags|= LIF_SEPARATOR;
				ChDisk.AddItem(&ChDiskItem);
				ChDiskItem.Flags&= ~LIF_SEPARATOR;

			} else {
				ChDiskItem.SetSelect(ChDisk.GetItemCount() == Pos);
				const wchar_t HotKeyStr[] = {m.hotkey ? L'&' : L' ', m.hotkey ? m.hotkey : L' ',
						m.hotkey ? L' ' : 0, 0};
				ChDiskItem.strName = HotKeyStr;
				ChDiskItem.strName+= FixedSizeStr(m.path, std::min(mounts.max_path, (size_t)48), true, false);
				if (mounts.max_col2) {
					ChDiskItem.strName+= L' ';
					ChDiskItem.strName+= BoxSymbols[BS_V1];
					ChDiskItem.strName+= L' ';
					ChDiskItem.strName+=
							FixedSizeStr(m.col2, std::min(mounts.max_col2, (size_t)24), false, true);
				}
				ChDiskItem.strName+= L' ';
				ChDiskItem.strName+= BoxSymbols[BS_V1];
				ChDiskItem.strName+= L' ';
				ChDiskItem.strName+= FixedSizeStr(m.col3, std::min(mounts.max_col3, (size_t)24), false, true);

				PanelMenuItem item;
				wcsncpy(item.location.path, m.path.CPtr(), ARRAYSIZE(item.location.path) - 1);
				item.location.id = m.id;
				if (item.location.path[0] == L'{' && another_curdir == item.location.path
						&& another_panel->GetPluginHandle() != INVALID_HANDLE_VALUE) {
					item.kind = PanelMenuItem::PLUGIN;
					item.pPlugin = nullptr;
				} else
					item.kind = m.unmountable ? PanelMenuItem::MOUNTPOINT : PanelMenuItem::DIRECTORY;

				ChDisk.SetUserData(&item, sizeof(item), ChDisk.AddItem(&ChDiskItem));
			}
		}

		if (Opt.ChangeDriveMode & DRIVE_SHOW_BOOKMARKS) {
			AddBookmarkItems(ChDisk, Pos);
		}

		if (Opt.ChangeDriveMode & DRIVE_SHOW_PLUGINS) {
			AddPluginItems(ChDisk, Pos);
		}

		Pos = 0;

		int X = X1 + 5;

		if ((this == CtrlObject->Cp()->RightPanel) && IsFullScreen() && (X2 - X1 > 40))
			X = (X2 - X1 + 1) / 2 + 5;

		int Y = (ScrY + 1 - (ChDisk.GetItemCount() + 5)) / 2;

		if (Y < 1)
			Y = 1;

		ChDisk.SetPosition(X, Y, 0, 0);

		if (Y < 3)
			ChDisk.SetBoxType(SHORT_DOUBLE_BOX);

		ChDisk.Show();

		while (!ChDisk.Done()) {
			FarKey Key;
			/*if(Events.DeviceArchivalEvent.Signaled() || Events.DeviceRemoveEvent.Signaled() || Events.MediaArchivalEvent.Signaled() || Events.MediaRemoveEvent.Signaled())
			{
				Key=KEY_CTRLR;
			}
			else*/
			{
				{	// очередная фигня
					ChangeMacroMode MacroMode(MACRO_DISKS);
					Key = ChDisk.ReadInput();
				}
			}
			int SelPos = ChDisk.GetSelectPos();
			PanelMenuItem *item = (PanelMenuItem *)ChDisk.GetUserData(nullptr, 0);

			switch (Key) {
				// Shift-Enter в меню выбора дисков вызывает проводник для данного диска
				case KEY_SHIFTNUMENTER:
				case KEY_SHIFTENTER: {
					if (item && !item->pPlugin) {
						Execute(item->location.path, TRUE, TRUE);
					}
				} break;
				case KEY_CTRLPGUP:
				case KEY_CTRLNUMPAD9: {
					if (Opt.PgUpChangeDisk)
						return -1;
				} break;
				case KEY_INS:
				case KEY_NUMPAD0: {
					//					if (item && item->kind == PanelMenuItem::SHORTCUT)
					//					{
					//						SaveShortcutFolder(item->nItem);
					//					}
					//					else
					{
						ShowBookmarksMenu();
					}
					return SelPos;
				} break;
				case KEY_CTRLA:
				case KEY_F4: {
					if (item) {
						if (item->kind == PanelMenuItem::PLUGIN) {
							if (item->pPlugin) {
								FARString strName = ChDisk.GetItemPtr(SelPos)->strName.SubStr(3);
								RemoveExternalSpaces(strName);

								if (CtrlObject->Plugins.SetHotKeyDialog(strName,
											CtrlObject->Plugins.GetHotKeySettingName(item->pPlugin,
													item->nItem, PluginManager::HKK_DRIVEMENU))) {
									return SelPos;
								}
							}
						} else if (item->kind == PanelMenuItem::SHORTCUT) {
							ShowBookmarksMenu(item->nItem);
							return SelPos;
						} else {
							Mounts::EditHotkey(item->location.path, item->location.id);
							return SelPos;
						}
					}
					break;
				}
				case KEY_NUMDEL:
				case KEY_DEL:

				case KEY_SHIFTNUMDEL:
				case KEY_SHIFTDECIMAL:
				case KEY_SHIFTDEL: {
					if (item)
						switch (item->kind) {
							case PanelMenuItem::MOUNTPOINT: {
								sUnmountPath(item->location.path,
									Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL || Key == KEY_SHIFTDEL);
								return SelPos;
							}

							case PanelMenuItem::SHORTCUT: {
								Bookmarks().Clear(item->nItem);
								return SelPos;
							}

							default:;
						}
				} break;
				case KEY_CTRL1:
				case KEY_RCTRL1:
					Opt.ChangeDriveMode^= DRIVE_SHOW_TYPE;
					return SelPos;
				case KEY_CTRL2:
				case KEY_RCTRL2:
					Opt.ChangeDriveMode^= DRIVE_SHOW_NETNAME;
					return SelPos;
				case KEY_CTRL3:
				case KEY_RCTRL3:
					Opt.ChangeDriveMode^= DRIVE_SHOW_LABEL;
					return SelPos;
				case KEY_CTRL4:
				case KEY_RCTRL4:
					Opt.ChangeDriveMode^= DRIVE_SHOW_FILESYSTEM;
					return SelPos;
				case KEY_CTRL5:
				case KEY_RCTRL5: {
					if (Opt.ChangeDriveMode & DRIVE_SHOW_SIZE) {
						Opt.ChangeDriveMode^= DRIVE_SHOW_SIZE;
						Opt.ChangeDriveMode|= DRIVE_SHOW_SIZE_FLOAT;
					} else {
						if (Opt.ChangeDriveMode & DRIVE_SHOW_SIZE_FLOAT)
							Opt.ChangeDriveMode^= DRIVE_SHOW_SIZE_FLOAT;
						else
							Opt.ChangeDriveMode^= DRIVE_SHOW_SIZE;
					}

					return SelPos;
				}
				case KEY_CTRL6:
				case KEY_RCTRL6:
					Opt.ChangeDriveMode^= DRIVE_SHOW_MOUNTS;
					return SelPos;
				case KEY_CTRL7:
				case KEY_RCTRL7:
					Opt.ChangeDriveMode^= DRIVE_SHOW_PLUGINS;
					return SelPos;
				case KEY_CTRL8:
				case KEY_RCTRL8:
					Opt.ChangeDriveMode^= DRIVE_SHOW_BOOKMARKS;
					return SelPos;
				case KEY_CTRL9:
				case KEY_RCTRL9:
					Opt.ChangeDriveMode^= DRIVE_SHOW_REMOTE;
					return SelPos;
				case KEY_F9:
					ConfigureChangeDriveMode();
					return SelPos;
				case KEY_SHIFTF1: {
					if (item && item->pPlugin) {
						// Вызываем нужный топик, который передали в CommandsMenu()
						FarShowHelp(item->pPlugin->GetModuleName(), nullptr,
								FHELP_SELFHELP | FHELP_NOSHOWERROR | FHELP_USECONTENTS);
					}

					break;
				}
				case KEY_ALTSHIFTF9:

					if (Opt.ChangeDriveMode & DRIVE_SHOW_PLUGINS) {
						ChDisk.Hide();
						CtrlObject->Plugins.Configure();
					}

					return SelPos;
				case KEY_SHIFTF9:

					if (item && item->pPlugin && item->pPlugin->HasConfigure())
						CtrlObject->Plugins.ConfigureCurrent(item->pPlugin, item->nItem);

					return SelPos;
				case KEY_CTRLR:
					return SelPos;
				default:
					ChDisk.ProcessInput();
					break;
			}

			if (ChDisk.Done() && (ChDisk.Modal::GetExitCode() < 0) && !strCurDir.IsEmpty()
					&& (StrCmpN(strCurDir, L"//", 2))) {
				const wchar_t RootDir[4] = {strCurDir.At(0), L':', GOOD_SLASH, L'\0'};

				if (FAR_GetDriveType(RootDir) == DRIVE_NO_ROOT_DIR)
					ChDisk.ClearDone();
			}
		}	// while (!Done)

		if (ChDisk.Modal::GetExitCode() < 0)
			return -1;

		mitem = (PanelMenuItem *)ChDisk.GetUserData(nullptr, 0);

		if (mitem) {
			Item = *mitem;
			mitem = &Item;
		}
	}	// эта скобка надо, см. M#605

	if (ProcessPluginEvent(FE_CLOSE, nullptr))
		return -1;
	ScrBuf.Flush();
	if (!WinPortTesting()) {
		INPUT_RECORD rec;
		PeekInputRecord(&rec);
	}

	if (!mitem)
		return -1;	//???

	if (mitem->kind == PanelMenuItem::PLUGIN) {
		//		fprintf(stderr, "pPlugin=%p nItem=0x%lx\n", mitem->pPlugin, (unsigned long)mitem->nItem);
		LONG_PTR nItem = mitem->nItem;
		// if (nItem == (LONG_PTR)-1)
		//	nItem = (LONG_PTR)mitem->root;

		if (mitem->pPlugin == nullptr) {
			// duplicate plugin instance of passive panel
			auto plugin_name = CtrlObject->Plugins.GetPluginModuleName(another_panel->GetPluginHandle());
			if (!plugin_name.IsEmpty()) {
				auto pPlugin = CtrlObject->Plugins.GetPlugin(plugin_name);
				if (pPlugin) {
					OpenPluginInfo opi = {sizeof(OpenPluginInfo), 0};
					CtrlObject->Plugins.GetOpenPluginInfo(another_panel->GetPluginHandle(), &opi);

					std::wstring hostFile, curDir;

					if (opi.CurDir)
						curDir = opi.CurDir;
					if (opi.HostFile)
						hostFile = opi.HostFile;

					SetLocation_Plugin(!hostFile.empty(), pPlugin, curDir.c_str(),
							hostFile.empty() ? nullptr : hostFile.c_str(), 0);
				}
			}

		} else {
			SetLocation_Plugin(false, mitem->pPlugin, nullptr, nullptr, nItem);
		}
	} else if (mitem->kind == PanelMenuItem::SHORTCUT) {
		ExecShortcutFolder(mitem->nItem);
	} else if (mitem->kind == PanelMenuItem::MOUNTPOINT || mitem->kind == PanelMenuItem::DIRECTORY) {
		SetLocation_Directory(mitem->location.path);
	}

	return -1;
}

void Panel::sUnmountPath(FARString path, bool forced)
{
	if (Opt.Confirm.RemoveHotPlug && Message(MSG_WARNING, 2,
			Msg::ChangeHotPlugDisconnectDriveTitle, Msg::ChangeHotPlugDisconnectDriveQuestion,
			path, Msg::Ok, Msg::Cancel) != 0) {
		return;
	}

	FARString dir;
	bool left_notify_closed = false, right_notify_closed = false;

	if (CtrlObject->Cp()->LeftPanel && CtrlObject->Cp()->LeftPanel->PanelMode == NORMAL_PANEL) {
		CtrlObject->Cp()->LeftPanel->GetCurDirPluginAware(dir);
		if (ArePathesAtSameDevice(path, dir)) {
			CtrlObject->Cp()->LeftPanel->CloseChangeNotification();
			left_notify_closed = true;
		}
	}

	if (CtrlObject->Cp()->RightPanel && CtrlObject->Cp()->RightPanel->PanelMode == NORMAL_PANEL) {
		CtrlObject->Cp()->RightPanel->GetCurDirPluginAware(dir);
		if (ArePathesAtSameDevice(path, dir)) {
			CtrlObject->Cp()->RightPanel->CloseChangeNotification();
			right_notify_closed = true;
		}
	}

	apiGetCurrentDirectory(dir);
	if (ArePathesAtSameDevice(path, dir)) {
		FarChDir(EscapeDevicePath(dir));
	}

	if (Mounts::Unmount(path, forced)) {
		return;
	}

	FarChDir(dir);
	if (left_notify_closed) {
		CtrlObject->Cp()->LeftPanel->CreateChangeNotification(FALSE);
	}
	if (right_notify_closed) {
		CtrlObject->Cp()->RightPanel->CreateChangeNotification(FALSE);
	}
}

bool Panel::SetLocation_Plugin(bool file_plugin, Plugin *plugin, const wchar_t *path,
		const wchar_t *host_file, LONG_PTR item)
{
	HANDLE hPlugin;

	if (file_plugin) {
		hPlugin = CtrlObject->Plugins.OpenFilePlugin(host_file, 0, OFP_ALTERNATIVE, plugin);	// OFP_NORMAL
	} else {
		hPlugin = CtrlObject->Plugins.OpenPlugin(plugin, OPEN_DISKMENU, item);					// nItem
	}

	if (hPlugin == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "SetLocation_Plugin(%d, %p, '%ls', '%ls', %lld) FAILED plugin open\n", file_plugin,
				plugin, path, host_file, (long long)item);
		return false;
	}

	int Focus = GetFocus();
	Panel *NewPanel = CtrlObject->Cp()->ChangePanel(this, FILE_PANEL, TRUE, TRUE);
	NewPanel->SetPluginMode(hPlugin, L"", Focus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible());

	if (path) {
		NewPanel->Update(0);
		NewPanel->Show();
		CtrlObject->Plugins.SetDirectory(hPlugin, L"/", 0);
		if (!CtrlObject->Plugins.SetDirectory(hPlugin, path, 0)) {
			fprintf(stderr, "SetLocation_Plugin(%d, %p, '%ls', '%ls', %lld) FAILED set directory\n",
					file_plugin, plugin, path, host_file, (long long)item);
		}
	}

	NewPanel->Update(0);
	NewPanel->Show();

	if (!Focus && CtrlObject->Cp()->GetAnotherPanel(this)->GetType() == INFO_PANEL) {
		CtrlObject->Cp()->GetAnotherPanel(this)->UpdateKeyBar();
	}

	return true;
}

bool Panel::SetLocation_Directory(const wchar_t *path)
{
	if (!FarChDir(path)) {
		if (CtrlObject->Plugins.ProcessCommandLine(path)) {
			fprintf(stderr, "SetLocation_Directory('%ls') PLUGIN\n", path);
			return true;
		}

		fprintf(stderr, "SetLocation_Directory('%ls') FAILED\n", path);
		return false;
	}

	fprintf(stderr, "SetLocation_Directory('%ls') OK\n", path);

	FARString strNewCurDir;
	apiGetCurrentDirectory(strNewCurDir);

	if ((PanelMode == NORMAL_PANEL) && (GetType() == FILE_PANEL) && !StrCmpI(strCurDir, strNewCurDir)
			&& IsVisible()) {
		// А нужно ли делать здесь Update????
		Update(UPDATE_KEEP_SELECTION);
	} else {
		int Focus = GetFocus();
		Panel *NewPanel = CtrlObject->Cp()->ChangePanel(this, FILE_PANEL, TRUE, FALSE);
		NewPanel->SetCurDir(strNewCurDir, TRUE);
		NewPanel->Show();

		if (Focus || !CtrlObject->Cp()->GetAnotherPanel(this)->IsVisible())
			NewPanel->SetFocus();

		if (!Focus && CtrlObject->Cp()->GetAnotherPanel(this)->GetType() == INFO_PANEL)
			CtrlObject->Cp()->GetAnotherPanel(this)->UpdateKeyBar();
	}

	return true;
}

int Panel::OnFCtlSetLocation(const FarPanelLocation *location)
{
	if (!location->PluginName) {
		return SetLocation_Directory(location->Path);
	}

	auto pPlugin = CtrlObject->Plugins.GetPlugin(location->PluginName);
	if (!pPlugin) {
		fprintf(stderr, "OnFCtlSetLocation: wrong plugin='%ls'", location->PluginName);
		return 0;
	}

	if (location->HostFile) {
		return SetLocation_Plugin(true, pPlugin, location->Path, location->HostFile, 0);
	}

	return SetLocation_Plugin(false, pPlugin, location->Path, nullptr, location->Item);
}

void Panel::FastFindProcessName(Edit *FindEdit, const wchar_t *Src, FARString &strLastName,
		FARString &strName)
{
	wchar_t *Ptr =
			(wchar_t *)malloc((StrLength(Src) + StrLength(FindEdit->GetStringAddr()) + 32) * sizeof(wchar_t));

	if (Ptr) {
		wcscpy(Ptr, FindEdit->GetStringAddr());
		wchar_t *EndPtr = Ptr + StrLength(Ptr);
		wcscat(Ptr, Src);
		Unquote(EndPtr);
		EndPtr = Ptr + StrLength(Ptr);
		//		DWORD Key;

		for (;;) {
			if (EndPtr == Ptr) {
				//				Key=KEY_NONE;
				break;
			}

			if (FindPartNameXLat(Ptr, FALSE, 1, 1)) {
				//				Key=*(EndPtr-1);
				*EndPtr = 0;
				FindEdit->SetString(Ptr);
				strLastName = Ptr;
				strName = Ptr;
				FindEdit->Show();
				break;
			}

			*--EndPtr = 0;
		}

		free(Ptr);
	}
}

int64_t Panel::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	return 0;
}

// корректировка букв
static DWORD _CorrectFastFindKbdLayout(INPUT_RECORD *rec, DWORD Key)
{
	if ((Key & KEY_ALT))	// && Key!=(KEY_ALT|0x3C))
	{
		if ((Key & KEY_SHIFT)) {
			switch (Key)
			// исключения (перекодированные в keyboard.cpp)
			case KEY_ALT + KEY_SHIFT + '`':
			case KEY_ALT + KEY_SHIFT + '_':
			case KEY_ALT + KEY_SHIFT + '=':
				return Key;
		}
		// // _SVS(SysLog(L"_CorrectFastFindKbdLayout>>> %ls | %ls",_FARKEY_ToName(Key),_INPUT_RECORD_Dump(rec)));
		if (rec->Event.KeyEvent.uChar.UnicodeChar
				&& WCHAR(Key & KEY_MASKF) != rec->Event.KeyEvent.uChar.UnicodeChar)		//???
			Key = (Key & (~KEY_MASKF)) | (rec->Event.KeyEvent.uChar.UnicodeChar & KEY_MASKF);	//???

																						// // _SVS(SysLog(L"_CorrectFastFindKbdLayout<<< %ls | %ls",_FARKEY_ToName(Key),_INPUT_RECORD_Dump(rec)));
	}

	return Key;
}

void Panel::FastFind(int FirstKey)
{
	// // _SVS(CleverSysLog Clev(L"Panel::FastFind"));
	INPUT_RECORD rec;
	FARString strLastName, strName;
	FarKey Key, KeyToProcess = 0;
	WaitInFastFind++;
	{
		int FindX = Min(X1 + 9, ScrX - 22);
		int FindY = Min(Y2, ScrY - 2);
		ChangeMacroMode MacroMode(MACRO_SEARCH);
		SaveScreen SaveScr(FindX, FindY, FindX + 21, FindY + 2);
		FastFindShow(FindX, FindY);
		Edit FindEdit;
		FindEdit.SetPosition(FindX + 2, FindY + 1, FindX + 19, FindY + 1);
		FindEdit.SetEditBeyondEnd(FALSE);
		FindEdit.SetObjectColor(FarColorToReal(COL_DIALOGEDIT));
		FindEdit.Show();

		while (!KeyToProcess) {
			if (FirstKey) {
				FirstKey = _CorrectFastFindKbdLayout(FrameManager->GetLastInputRecord(), FirstKey);
				// // _SVS(SysLog(L"Panel::FastFind  FirstKey=%ls  %ls",_FARKEY_ToName(FirstKey),_INPUT_RECORD_Dump(FrameManager->GetLastInputRecord())));
				// // _SVS(SysLog(L"if (FirstKey)"));
				Key = FirstKey;
			} else {
				// // _SVS(SysLog(L"else if (FirstKey)"));
				Key = GetInputRecord(&rec);

				if (rec.EventType == MOUSE_EVENT) {
					if (!(rec.Event.MouseEvent.dwButtonState & 3))
						continue;
					else
						Key = KEY_ESC;
				} else if (!rec.EventType || rec.EventType == KEY_EVENT
						|| rec.EventType == FARMACRO_KEY_EVENT) {
					// для вставки воспользуемся макродвижком...
					if (Key == KEY_CTRLV || Key == KEY_SHIFTINS || Key == KEY_SHIFTNUMPAD0) {
						wchar_t *ClipText = PasteFromClipboard();

						if (ClipText) {
							if (*ClipText) {
								FastFindProcessName(&FindEdit, ClipText, strLastName, strName);
								FastFindShow(FindX, FindY);
							}

							free(ClipText);
						}

						continue;
					} else if (Key == KEY_OP_XLAT) {
						FARString strTempName;
						FindEdit.Xlat();
						FindEdit.GetString(strTempName);
						FindEdit.SetString(L"");
						FastFindProcessName(&FindEdit, strTempName, strLastName, strName);
						FastFindShow(FindX, FindY);
						continue;
					} else if (Key == KEY_OP_PLAINTEXT) {
						FARString strTempName;
						FindEdit.ProcessKey(Key);
						FindEdit.GetString(strTempName);
						FindEdit.SetString(L"");
						FastFindProcessName(&FindEdit, strTempName, strLastName, strName);
						FastFindShow(FindX, FindY);
						continue;
					} else
						Key = _CorrectFastFindKbdLayout(&rec, Key);
				}
			}

			if (Key == KEY_ESC || Key == KEY_F10) {
				KeyToProcess = KEY_NONE;
				break;
			}

			// // _SVS(if (!FirstKey) SysLog(L"Panel::FastFind  Key=%ls  %ls",_FARKEY_ToName(Key),_INPUT_RECORD_Dump(&rec)));
			if (Key >= KEY_ALT_BASE + 0x01 && Key <= KEY_ALT_BASE + 65535)
				Key = Lower(static_cast<WCHAR>(Key - KEY_ALT_BASE));

			if (Key >= KEY_ALTSHIFT_BASE + 0x01 && Key <= KEY_ALTSHIFT_BASE + 65535)
				Key = Lower(static_cast<WCHAR>(Key - KEY_ALTSHIFT_BASE));

			if (Key == KEY_MULTIPLY)
				Key = L'*';

			switch (Key) {
				case KEY_F1: {
					FindEdit.Hide();
					SaveScr.RestoreArea();
					{
						Help::Present(L"FastFind");
					}
					FindEdit.Show();
					FastFindShow(FindX, FindY);
					break;
				}
				case KEY_CTRLNUMENTER:
				case KEY_CTRLENTER:
					FindPartNameXLat(strName, TRUE, 1, 1);
					FindEdit.Show();
					FastFindShow(FindX, FindY);
					break;
				case KEY_CTRLSHIFTNUMENTER:
				case KEY_CTRLSHIFTENTER:
					FindPartNameXLat(strName, TRUE, -1, 1);
					FindEdit.Show();
					FastFindShow(FindX, FindY);
					break;
				case KEY_NONE:
				case KEY_IDLE:
					break;
				default:

					if ((Key < 32 || Key >= 65536) && Key != KEY_BS && Key != KEY_CTRLY && Key != KEY_CTRLBS
							&& Key != KEY_ALT && Key != KEY_SHIFT && Key != KEY_CTRL && Key != KEY_RALT
							&& Key != KEY_RCTRL && !(Key == KEY_CTRLINS || Key == KEY_CTRLNUMPAD0)
							&& !(Key == KEY_SHIFTINS || Key == KEY_SHIFTNUMPAD0)) {
						KeyToProcess = Key;
						break;
					}

					if (FindEdit.ProcessKey(Key)) {
						FindEdit.GetString(strName);

						// уберем двойные '**'
						if (strName.GetLength() > 1 && strName.At(strName.GetLength() - 1) == L'*'
								&& strName.At(strName.GetLength() - 2) == L'*') {
							strName.Truncate(strName.GetLength() - 1);
							FindEdit.SetString(strName);
						}

						/*
							$ 09.04.2001 SVS
							проблемы с быстрым поиском.
							Подробнее в 00573.ChangeDirCrash.txt
						*/
						if (strName.At(0) == L'"') {
							strName.LShift(1);
							FindEdit.SetString(strName);
						}

						if (FindPartNameXLat(strName, FALSE, 1, 1)) {
							strLastName = strName;
						} else {
							if (CtrlObject->Macro.IsExecuting())	// && CtrlObject->Macro.GetLevelState() > 0) // если вставка макросом...
							{
								// CtrlObject->Macro.DropProcess(); // ... то дропнем макропроцесс
								// CtrlObject->Macro.PopState();
								;
							}

							FindEdit.SetString(strLastName);
							strName = strLastName;
						}

						FindEdit.Show();
						FastFindShow(FindX, FindY);
					}

					break;
			}

			FirstKey = 0;
		}
	}
	WaitInFastFind--;
	Show();
	CtrlObject->MainKeyBar->Redraw();
	ScrBuf.Flush();
	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;

	if ((KeyToProcess == KEY_ENTER || KeyToProcess == KEY_NUMENTER) && ActivePanel->GetType() == TREE_PANEL)
		((TreeList *)ActivePanel)->ProcessEnter();
	else
		CtrlObject->Cp()->ProcessKey(KeyToProcess);
}

void Panel::FastFindShow(int FindX, int FindY)
{
	SetFarColor(COL_DIALOGTEXT);
	GotoXY(FindX + 1, FindY + 1);
	Text(L" ");
	GotoXY(FindX + 20, FindY + 1);
	Text(L" ");
	Box(FindX, FindY, FindX + 21, FindY + 2, FarColorToReal(COL_DIALOGBOX), DOUBLE_BOX);
	GotoXY(FindX + 7, FindY);
	SetFarColor(COL_DIALOGBOXTITLE);
	Text(Msg::SearchFileTitle);
}

void Panel::SetFocus()
{
	if (CtrlObject->Cp()->ActivePanel != this) {
		CtrlObject->Cp()->ActivePanel->KillFocus();
		CtrlObject->Cp()->ActivePanel = this;
	}

	ProcessPluginEvent(FE_GOTFOCUS, nullptr);

	if (!GetFocus()) {
		CtrlObject->Cp()->RedrawKeyBar();
		Focus = TRUE;
		Redraw();
		FarChDir(strCurDir);
	}
}

void Panel::KillFocus()
{
	Focus = FALSE;
	ProcessPluginEvent(FE_KILLFOCUS, nullptr);
	Redraw();
}

int Panel::PanelProcessMouse(MOUSE_EVENT_RECORD *MouseEvent, int &RetCode)
{
	RetCode = TRUE;

	if (!ModalMode && !MouseEvent->dwMousePosition.Y) {
		if (MouseEvent->dwMousePosition.X == ScrX) {
			if (Opt.ScreenSaver && !(MouseEvent->dwButtonState & 3)) {
				EndDrag();
				ScreenSaver(TRUE);
				return TRUE;
			}
		} else {
			if ((MouseEvent->dwButtonState & 3) && !MouseEvent->dwEventFlags) {
				EndDrag();

				if (!MouseEvent->dwMousePosition.X)
					CtrlObject->Cp()->ProcessKey(KEY_CTRLO);
				else
					ShellOptions(0, MouseEvent);

				return TRUE;
			}
		}
	}

	if (!IsVisible()
			|| (MouseEvent->dwMousePosition.X < X1 || MouseEvent->dwMousePosition.X > X2
					|| MouseEvent->dwMousePosition.Y < Y1 || MouseEvent->dwMousePosition.Y > Y2)) {
		RetCode = FALSE;
		return TRUE;
	}

	if (DragX != -1) {
		if (!(MouseEvent->dwButtonState & 3)) {
			EndDrag();

			if (!MouseEvent->dwEventFlags && SrcDragPanel != this) {
				MoveToMouse(MouseEvent);
				Redraw();
				SrcDragPanel->ProcessKey(DragMove ? KEY_DRAGMOVE : KEY_DRAGCOPY);
			}

			return TRUE;
		}

		if (MouseEvent->dwMousePosition.Y <= Y1 || MouseEvent->dwMousePosition.Y >= Y2
				|| !CtrlObject->Cp()->GetAnotherPanel(SrcDragPanel)->IsVisible()) {
			EndDrag();
			return TRUE;
		}

		if ((MouseEvent->dwButtonState & 2) && !MouseEvent->dwEventFlags)
			DragMove = !DragMove;

		if (MouseEvent->dwButtonState & 1) {
			if ((abs(MouseEvent->dwMousePosition.X - DragX) > 15 || SrcDragPanel != this) && !ModalMode) {
				if (SrcDragPanel->GetSelCount() == 1 && !DragSaveScr) {
					SrcDragPanel->GoToFile(strDragName);
					SrcDragPanel->Show();
				}

				DragMessage(MouseEvent->dwMousePosition.X, MouseEvent->dwMousePosition.Y, DragMove);
				return TRUE;
			} else {
				delete DragSaveScr;
				DragSaveScr = nullptr;
			}
		}
	}

	if (!(MouseEvent->dwButtonState & 3))
		return TRUE;

	if ((MouseEvent->dwButtonState & 1) && !MouseEvent->dwEventFlags && X2 - X1 < ScrX) {
		DWORD FileAttr;
		MoveToMouse(MouseEvent);
		GetSelNameCompat(nullptr, FileAttr);

		if (GetSelNameCompat(&strDragName, FileAttr) && !TestParentFolderName(strDragName)) {
			SrcDragPanel = this;
			DragX = MouseEvent->dwMousePosition.X;
			DragY = MouseEvent->dwMousePosition.Y;
			DragMove = ShiftPressed;
		}
	}

	return FALSE;
}

int Panel::IsDragging()
{
	return DragSaveScr != nullptr;
}

void Panel::EndDrag()
{
	delete DragSaveScr;
	DragSaveScr = nullptr;
	DragX = DragY = -1;
}

void Panel::DragMessage(int X, int Y, int Move)
{
	FARString strDragMsg, strSelName;
	int SelCount, MsgX, Length;

	if (!(SelCount = SrcDragPanel->GetSelCount()))
		return;

	if (SelCount == 1) {
		FARString strCvtName;
		DWORD FileAttr;
		SrcDragPanel->GetSelNameCompat(nullptr, FileAttr);
		SrcDragPanel->GetSelNameCompat(&strSelName, FileAttr);
		strCvtName = PointToName(strSelName);
		EscapeSpace(strCvtName);
		strSelName = strCvtName;
	} else
		strSelName.Format(Msg::DragFiles, SelCount);

	if (Move)
		strDragMsg.Format(Msg::DragMove, strSelName.CPtr());
	else
		strDragMsg.Format(Msg::DragCopy, strSelName.CPtr());

	if ((Length = (int)strDragMsg.GetLength()) + X > ScrX) {
		MsgX = ScrX - Length;

		if (MsgX < 0) {
			MsgX = 0;
			TruncStrFromEnd(strDragMsg, ScrX);
			Length = (int)strDragMsg.GetLength();
		}
	} else
		MsgX = X;

	ChangePriority ChPriority(ChangePriority::NORMAL);
	delete DragSaveScr;
	DragSaveScr = new SaveScreen(MsgX, Y, MsgX + Length - 1, Y);
	GotoXY(MsgX, Y);
	SetFarColor(COL_PANELDRAGTEXT);
	Text(strDragMsg);
}

int Panel::GetCurDir(FARString &strCurDir)
{
	strCurDir = Panel::strCurDir;	// TODO: ОПАСНО!!!
	return (int)strCurDir.GetLength();
}

int Panel::GetCurDirPluginAware(FARString &strCurDir)
{
	if (PanelMode == PLUGIN_PANEL) {
		HANDLE hPlugin = GetPluginHandle();
		//		PluginHandle *ph = (PluginHandle*)hPlugin;
		OpenPluginInfo Info;
		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

		strCurDir.Clear();

		if (Info.HostFile && *Info.HostFile) {
			strCurDir+= Info.HostFile;
			strCurDir+= L"/";
		}

		strCurDir+= Info.CurDir;
	} else {
		strCurDir = Panel::strCurDir;
	}

	return (int)strCurDir.GetLength();
}

BOOL Panel::SetCurDir(const wchar_t *CurDir, int ClosePlugin)
{
	if (StrCmpI(strCurDir, CurDir) || !TestCurrentDirectory(CurDir)) {
		strCurDir = CurDir;

		if (PanelMode != PLUGIN_PANEL)
			PrepareDiskPath(strCurDir);
	}

	return TRUE;
}

void Panel::InitCurDir(const wchar_t *CurDir)
{
	if (StrCmpI(strCurDir, CurDir) || !TestCurrentDirectory(CurDir)) {
		strCurDir = CurDir;

		if (PanelMode != PLUGIN_PANEL)
			PrepareDiskPath(strCurDir);
	}
}

/*
	$ 14.06.2001 KM
	+ Добавлена установка переменных окружения, определяющих
	текущие директории дисков как для активной, так и для
	пассивной панели. Это необходимо программам запускаемым
	из FAR.
*/
/*
	$ 05.10.2001 SVS
	! Давайте для начала выставим нужные значения для пассивной панели,
	а уж потом...
	А то фигня какая-то получается...
*/
/*
	$ 14.01.2002 IS
	! Убрал установку переменных окружения, потому что она производится
	в FarChDir, которая теперь используется у нас для установления
	текущего каталога.
*/
int Panel::SetCurPath()
{
	if (GetMode() == PLUGIN_PANEL)
		return TRUE;

	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->GetType() != PLUGIN_PANEL) {
		if (IsAlpha(AnotherPanel->strCurDir.At(0)) && AnotherPanel->strCurDir.At(1) == L':'
				&& Upper(AnotherPanel->strCurDir.At(0)) != Upper(strCurDir.At(0))) {
			// сначала установим переменные окружения для пассивной панели
			// (без реальной смены пути, чтобы лишний раз пассивный каталог
			// не перечитывать)
			FarChDir(AnotherPanel->strCurDir, FALSE);
		}
	}

	if (!FarChDir(strCurDir)) {
		while (!FarChDir(strCurDir)) {
			int Result = TestFolder(strCurDir);

			if (Result == TSTFLD_NOTFOUND) {
				if (CheckShortcutFolder(&strCurDir, FALSE, TRUE) && FarChDir(strCurDir)) {
					SetCurDir(strCurDir, TRUE);
					return TRUE;
				}
			} else
				break;

			if (FrameManager && FrameManager->ManagerStarted())		// сначала проверим - а запущен ли менеджер
			{
				SetCurDir(DefaultPanelInitialDirectory(), TRUE);	// если запущен - выставим путь который мы точно знаем что существует
				ChangeDisk();										// и вызовем меню выбора дисков
			} else													// оппа...
			{
				FARString strTemp = strCurDir;
				CutToFolderNameIfFolder(strCurDir);						// подымаемся вверх, для очередной порции ChDir

				if (strTemp.GetLength() == strCurDir.GetLength())		// здесь проблема - видимо диск недоступен
				{
					SetCurDir(DefaultPanelInitialDirectory(), TRUE);	// тогда просто сваливаем в каталог, откуда стартанул FAR.
					break;
				} else {
					if (FarChDir(strCurDir)) {
						SetCurDir(strCurDir, TRUE);
						break;
					}
				}
			}
		}

		return FALSE;
	}

	return TRUE;
}

void Panel::Hide()
{
	ScreenObject::Hide();
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->IsVisible()) {
		if (AnotherPanel->GetFocus())
			if ((AnotherPanel->GetType() == FILE_PANEL && AnotherPanel->IsFullScreen())
					|| (GetType() == FILE_PANEL && IsFullScreen()))
				AnotherPanel->Show();
	}
}

void Panel::Show()
{
	if (Locked())
		return;

	/* $ 03.10.2001 IS перерисуем строчку меню */
	if (Opt.ShowMenuBar)
		CtrlObject->TopMenuBar->Show();

	/* $ 09.05.2001 OT */
	//	SavePrevScreen();
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->IsVisible() && !GetModalMode()) {
		if (SaveScr) {
			SaveScr->AppendArea(AnotherPanel->SaveScr);
		}

		if (AnotherPanel->GetFocus()) {
			if (AnotherPanel->IsFullScreen()) {
				SetVisible(TRUE);
				return;
			}

			if (GetType() == FILE_PANEL && IsFullScreen()) {
				ScreenObject::Show();
				AnotherPanel->Show();
				return;
			}
		}
	}

	ScreenObject::Show();
	ShowScreensCount();
}

void Panel::DrawSeparator(int Y)
{
	if (Y < Y2) {
		SetFarColor(COL_PANELBOX);
		GotoXY(X1, Y);
		ShowSeparator(X2 - X1 + 1, 1);
	}
}

void Panel::ShowScreensCount()
{
	if (Opt.ShowScreensNumber && !X1) {
		int Viewers = FrameManager->GetFrameCountByType(MODALTYPE_VIEWER);
		int Editors = FrameManager->GetFrameCountByType(MODALTYPE_EDITOR);
		int Dialogs = FrameManager->GetFrameCountByType(MODALTYPE_DIALOG);
		bool HasPluginsTasks = CtrlObject->Plugins.HasBackgroundTasks();
		unsigned int vts = VTShell_Count();

		if (Viewers > 0 || Editors > 0 || Dialogs > 0 || vts > 0 || HasPluginsTasks) {
			FARString strScreensText;

			char Prefix = '[';
			if (vts > 0) {
				strScreensText.Format(L"%cT%u", Prefix, vts);
				Prefix = ' ';
			}

			if (Viewers > 0) {
				strScreensText.Format(L"%cV%d", Prefix, Viewers);
				Prefix = ' ';
			}

			if (Editors > 0) {
				strScreensText.AppendFormat(L"%cE%d", Prefix, Editors);
				Prefix = ' ';
			}

			if (Dialogs > 0) {
				strScreensText.AppendFormat(L"%cD%d", Prefix, Dialogs);
				Prefix = ' ';
			}

			if (HasPluginsTasks) {
				const auto &Tasks = CtrlObject->Plugins.BackgroundTasks();
				for (const auto &It : Tasks) {
					strScreensText.AppendFormat(L"%c%ls%u", Prefix, It.first.c_str(), It.second);
					Prefix = ' ';
				}
			}

			if (Prefix != '[') {
				strScreensText+= L"]";
				GotoXY(Opt.ShowColumnTitles ? X1 : X1 + 2, Y1);
				SetFarColor(COL_PANELSCREENSNUMBER);
				Text(strScreensText);
			}
		}
	}
}

void Panel::SetTitle()
{
	if (GetFocus()) {
		FARString strTitleDir(L"{");

		if (!strCurDir.IsEmpty()) {
			strTitleDir+= strCurDir;
		} else {
			FARString strCmdText;
			CtrlObject->CmdLine->GetCurDir(strCmdText);
			strTitleDir+= strCmdText;
		}

		strTitleDir+= L"}";

		ConsoleTitle::SetFarTitle(strTitleDir);
	}
}

FARString &Panel::GetTitle(FARString &strTitle, int SubLen, int TruncSize)
{
	FARString strTitleDir;
	bool truncTitle = (SubLen == -1 || TruncSize == 0) ? false : true;

	if (PanelMode == PLUGIN_PANEL) {
		OpenPluginInfo Info;
		GetOpenPluginInfo(&Info);
		strTitleDir = Info.PanelTitle;
		RemoveExternalSpaces(strTitleDir);
		if (truncTitle)
			TruncStr(strTitleDir, SubLen - TruncSize);
	} else {
		strTitleDir = strCurDir;

		if (truncTitle)
			TruncPathStr(strTitleDir, SubLen - TruncSize);
	}

	strTitle = L" " + strTitleDir + L" ";
	return strTitle;
}

int Panel::SetPluginCommand(int Command, int Param1, LONG_PTR Param2)
{
	_ALGO(CleverSysLog clv(L"Panel::SetPluginCommand"));
	_ALGO(SysLog(L"(Command=%ls, Param1=[%d/0x%08X], Param2=[%d/0x%08X])", _FCTL_ToName(Command), (int)Param1,
			Param1, (int)Param2, Param2));
	int Result = FALSE;
	ProcessingPluginCommand++;
	FilePanels *FPanels = CtrlObject->Cp();
	PluginCommand = Command;
	auto DestFilePanel = dynamic_cast<FileList*>(this);

	switch (Command) {
		case FCTL_SETVIEWMODE:
			Result = FPanels->ChangePanelViewMode(this, Param1, FPanels->IsTopFrame());
			break;

		case FCTL_SETSORTMODE: {
			int Mode = Param1;

			if ((Mode > SM_DEFAULT) && (Mode <= SM_CHTIME)) {
				SetSortMode(--Mode);	// Уменьшим на 1 из-за SM_DEFAULT
				Result = TRUE;
			}
			break;
		}

		case FCTL_SETNUMERICSORT: {
			ChangeNumericSort(Param1);
			Result = TRUE;
			break;
		}

		case FCTL_SETCASESENSITIVESORT: {
			ChangeCaseSensitiveSort(Param1);
			Result = TRUE;
			break;
		}

		case FCTL_GETPANELPLUGINHANDLE: {
			*(HANDLE *)Param2 = (GetPluginHandle() != INVALID_HANDLE_VALUE)
					? CtrlObject->Plugins.GetRealPluginHandle(GetPluginHandle())
					: INVALID_HANDLE_VALUE;
			Result = TRUE;
			break;
		}

		case FCTL_SETPANELLOCATION: {
			Result = OnFCtlSetLocation((const FarPanelLocation *)Param2);
			break;
		}

		case FCTL_SETSORTORDER: {
			ChangeSortOrder(Param1 ? -1 : 1);
			Result = TRUE;
			break;
		}

		case FCTL_SETDIRECTORIESFIRST: {
			ChangeDirectoriesFirst(Param1);
			Result = TRUE;
			break;
		}

		case FCTL_CLOSEPLUGIN:
			strPluginParam = (const wchar_t *)Param2;
			Result = TRUE;
			// if(Opt.CPAJHefuayor)
			//	CtrlObject->Plugins.ProcessCommandLine((char *)PluginParam);
			break;

		case FCTL_GETPANELINFO: {
			if (!Param2)
				break;

			PanelInfo *Info = (PanelInfo *)Param2;
			memset(Info, 0, sizeof(*Info));
			UpdateIfRequired();

			switch (GetType()) {
				case FILE_PANEL:
					Info->PanelType = PTYPE_FILEPANEL;
					break;
				case TREE_PANEL:
					Info->PanelType = PTYPE_TREEPANEL;
					break;
				case QVIEW_PANEL:
					Info->PanelType = PTYPE_QVIEWPANEL;
					break;
				case INFO_PANEL:
					Info->PanelType = PTYPE_INFOPANEL;
					break;
			}

			Info->Plugin = (GetMode() == PLUGIN_PANEL);
			int X1, Y1, X2, Y2;
			GetPosition(X1, Y1, X2, Y2);
			Info->PanelRect.left = X1;
			Info->PanelRect.top = Y1;
			Info->PanelRect.right = X2;
			Info->PanelRect.bottom = Y2;
			Info->Visible = IsVisible();
			Info->Focus = GetFocus();
			Info->ViewMode = GetViewMode();
			Info->SortMode = SM_UNSORTED - UNSORTED + GetSortMode();
			{
				static struct
				{
					int *Opt;
					DWORD Flags;
				} PFLAGS[] = {
						{&Opt.ShowHidden, PFLAGS_SHOWHIDDEN},
						{&Opt.Highlight,  PFLAGS_HIGHLIGHT },
				};
				DWORD Flags = 0;

				for (size_t I = 0; I < ARRAYSIZE(PFLAGS); ++I)
					if (*(PFLAGS[I].Opt))
						Flags|= PFLAGS[I].Flags;

				Flags|= GetSortOrder() < 0 ? PFLAGS_REVERSESORTORDER : 0;
				Flags|= GetSortGroups() ? PFLAGS_USESORTGROUPS : 0;
				Flags|= GetSelectedFirstMode() ? PFLAGS_SELECTEDFIRST : 0;
				Flags|= GetDirectoriesFirst() ? PFLAGS_DIRECTORIESFIRST : 0;
				Flags|= GetNumericSort() ? PFLAGS_NUMERICSORT : 0;
				Flags|= GetCaseSensitiveSort() ? PFLAGS_CASESENSITIVESORT : 0;

				if (CtrlObject->Cp()->LeftPanel == this)
					Flags|= PFLAGS_PANELLEFT;

				Info->Flags = Flags;
			}

			if (GetType() == FILE_PANEL) {
				static int Reenter = 0;

				if (!Reenter && Info->Plugin) {
					Reenter++;
					OpenPluginInfo PInfo;
					DestFilePanel->GetOpenPluginInfo(&PInfo);

					if (PInfo.Flags & OPIF_REALNAMES)
						Info->Flags|= PFLAGS_REALNAMES;

					if (!(PInfo.Flags & OPIF_USEHIGHLIGHTING))
						Info->Flags&= ~PFLAGS_HIGHLIGHT;

					if (PInfo.Flags & OPIF_USECRC32)
						Info->Flags|= PFLAGS_USECRC32;

					Reenter--;
				}

				DestFilePanel->PluginGetPanelInfo(*Info);
			}

			if (!Info->Plugin)	// $ 12.12.2001 DJ - на неплагиновой панели - всегда реальные имена
				Info->Flags|= PFLAGS_REALNAMES;

			Result = TRUE;
			break;
		}

		case FCTL_GETPANELHOSTFILE:
		case FCTL_GETPANELFORMAT:
		case FCTL_GETPANELDIR: {
			FARString strTemp;

			if (Command == FCTL_GETPANELDIR)
				GetCurDir(strTemp);

			if (GetType() == FILE_PANEL) {
				static int Reenter = 0;

				if (!Reenter && GetMode() == PLUGIN_PANEL) {
					Reenter++;

					OpenPluginInfo PInfo;
					DestFilePanel->GetOpenPluginInfo(&PInfo);

					switch (Command) {
						case FCTL_GETPANELHOSTFILE:
							strTemp = PInfo.HostFile;
							break;
						case FCTL_GETPANELFORMAT:
							strTemp = PInfo.Format;
							break;
						case FCTL_GETPANELDIR:
							strTemp = PInfo.CurDir;
							break;
					}

					Reenter--;
				}
			}

			if (Param1 && Param2)
				far_wcsncpy((wchar_t *)Param2, strTemp, Param1);

			Result = (int)strTemp.GetLength() + 1;
			break;
		}

		case FCTL_GETCOLUMNTYPES:
		case FCTL_GETCOLUMNWIDTHS:

			if (GetType() == FILE_PANEL) {
				FARString strColumnTypes, strColumnWidths;
				DestFilePanel->PluginGetColumnTypesAndWidths(strColumnTypes, strColumnWidths);

				if (Command == FCTL_GETCOLUMNTYPES) {
					if (Param1 && Param2)
						far_wcsncpy((wchar_t *)Param2, strColumnTypes, Param1);

					Result = (int)strColumnTypes.GetLength() + 1;
				} else {
					if (Param1 && Param2)
						far_wcsncpy((wchar_t *)Param2, strColumnWidths, Param1);

					Result = (int)strColumnWidths.GetLength() + 1;
				}
			}
			break;

		case FCTL_GETPANELITEM: {
			if (DestFilePanel)
				Result = (int)DestFilePanel->PluginGetPanelItem(Param1, (PluginPanelItem *)Param2);
			break;
		}

		case FCTL_GETSELECTEDPANELITEM: {
			if (DestFilePanel)
				Result = (int)DestFilePanel->PluginGetSelectedPanelItem(Param1, (PluginPanelItem *)Param2);
			break;
		}

		case FCTL_GETCURRENTPANELITEM: {
			if (DestFilePanel) {
				PanelInfo Info;
				DestFilePanel->PluginGetPanelInfo(Info);
				Result = (int)DestFilePanel->PluginGetPanelItem(Info.CurrentItem, (PluginPanelItem *)Param2);
			}
			break;
		}

		case FCTL_BEGINSELECTION: {
			if (GetType() == FILE_PANEL) {
				DestFilePanel->PluginBeginSelection();
				Result = TRUE;
			}
			break;
		}

		case FCTL_SETSELECTION: {
			if (GetType() == FILE_PANEL) {
				DestFilePanel->PluginSetSelection(Param1, Param2 ? true : false);
				Result = TRUE;
			}
			break;
		}

		case FCTL_CLEARSELECTION: {
			if (GetType() == FILE_PANEL) {
				DestFilePanel->PluginClearSelection(Param1);
				Result = TRUE;
			}
			break;
		}

		case FCTL_ENDSELECTION: {
			if (GetType() == FILE_PANEL) {
				DestFilePanel->PluginEndSelection();
				Result = TRUE;
			}
			break;
		}

		case FCTL_UPDATEPANEL:
			Update(Param1 ? UPDATE_KEEP_SELECTION : 0);

			if (GetType() == QVIEW_PANEL)
				CtrlObject->Cp()->GetAnotherPanel(this)->UpdateViewPanel();

			Result = TRUE;
			break;

		case FCTL_REDRAWPANEL: {
			PanelRedrawInfo *Info = (PanelRedrawInfo *)Param2;

			if (Info) {
				CurFile = Info->CurrentItem;
				CurTopFile = Info->TopPanelItem;
			}

			// $ 12.05.2001 DJ перерисовываемся только в том случае, если мы - текущий фрейм
			if (FPanels->IsTopFrame())
				Redraw();

			Result = TRUE;
			break;
		}

		case FCTL_SETPANELDIR: {
			if (Param2) {
				Result = SetCurDir((const wchar_t *)Param2, TRUE);
				// restore current directory to active panel path
				Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
				if (Result && this != ActivePanel) {
					ActivePanel->SetCurPath();
				}
			}
			break;
		}
	}

	ProcessingPluginCommand--;
	return Result;
}

int Panel::GetCurName(FARString &strName)
{
	strName.Clear();
	return FALSE;
}

int Panel::GetCurBaseName(FARString &strName)
{
	strName.Clear();
	return FALSE;
}

BOOL Panel::NeedUpdatePanel(Panel *AnotherPanel)
{
	/* Обновить, если обновление разрешено и пути совпадают */
	if ((!Opt.AutoUpdateLimit || static_cast<DWORD>(GetFileCount()) <= Opt.AutoUpdateLimit)
			&& !StrCmpI(AnotherPanel->strCurDir, strCurDir))
		return TRUE;

	return FALSE;
}

bool Panel::SaveShortcutFolder(int Pos)
{
	FARString strShortcutFolder, strPluginModule, strPluginFile, strPluginData;

	if (PanelMode == PLUGIN_PANEL) {
		HANDLE hPlugin = GetPluginHandle();
		PluginHandle *ph = (PluginHandle *)hPlugin;
		strPluginModule = ph->pPlugin->GetModuleName();
		OpenPluginInfo Info;
		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);
		strPluginFile = Info.HostFile;
		strShortcutFolder = Info.CurDir;
		strPluginData = Info.ShortcutData;
	} else {
		strPluginModule.Clear();
		strPluginFile.Clear();
		strPluginData.Clear();
		strShortcutFolder = strCurDir;
	}

	if (Bookmarks().Set(Pos, &strShortcutFolder, &strPluginModule, &strPluginFile, &strPluginData))
		return true;

	return true;
}

/*
int Panel::ProcessShortcutFolder(FarKey Key,BOOL ProcTreePanel)
{
	FARString strShortcutFolder, strPluginModule, strPluginFile, strPluginData;

	if (GetShortcutFolder(Key-KEY_RCTRL0,&strShortcutFolder,&strPluginModule,&strPluginFile,&strPluginData))
	{
		Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);

		if (ProcTreePanel)
		{
			if (AnotherPanel->GetType()==FILE_PANEL)
			{
				AnotherPanel->SetCurDir(strShortcutFolder,TRUE);
				AnotherPanel->Redraw();
			}
			else
			{
				SetCurDir(strShortcutFolder,TRUE);
				ProcessKey(KEY_ENTER);
			}
		}
		else
		{
			if (AnotherPanel->GetType()==FILE_PANEL && !strPluginModule.IsEmpty())
			{
				AnotherPanel->SetCurDir(strShortcutFolder,TRUE);
				AnotherPanel->Redraw();
			}
		}

		return TRUE;
	}

	return FALSE;
}
*/

bool Panel::ExecShortcutFolder(int Pos)
{
	FARString strShortcutFolder, strPluginModule, strPluginFile, strPluginData;

	if (Bookmarks().Get(Pos, &strShortcutFolder, &strPluginModule, &strPluginFile, &strPluginData)) {
		Panel *SrcPanel = this;
		Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

		switch (GetType()) {
			case TREE_PANEL:
				if (AnotherPanel->GetType() == FILE_PANEL)
					SrcPanel = AnotherPanel;
				break;

			case QVIEW_PANEL:
			case INFO_PANEL: {
				if (AnotherPanel->GetType() == FILE_PANEL)
					SrcPanel = AnotherPanel;
				break;
			}
		}

		int CheckFullScreen = SrcPanel->IsFullScreen();

		if (!strPluginModule.IsEmpty()) {
			if (!strPluginFile.IsEmpty()) {
				switch (CheckShortcutFolder(&strPluginFile, TRUE)) {
					case 0:
						// return FALSE;
					case -1:
						return true;
				}

				/* Своеобразное решение BugZ#50 */
				FARString strRealDir;
				strRealDir = strPluginFile;

				if (CutToSlash(strRealDir)) {
					SrcPanel->SetCurDir(strRealDir, TRUE);
					SrcPanel->GoToFile(PointToName(strPluginFile));

					SrcPanel->ClearAllItem();
				}

				if (SrcPanel->GetType() == FILE_PANEL)
					((FileList *)SrcPanel)->OpenFilePlugin(strPluginFile, FALSE, OFP_SHORTCUT);		//???

				if (!strShortcutFolder.IsEmpty())
					SrcPanel->SetCurDir(strShortcutFolder, FALSE);

				SrcPanel->Show();
			} else {
				switch (CheckShortcutFolder(nullptr, TRUE)) {
					case 0:
						// return FALSE;
					case -1:
						return true;
				}

				for (int I = 0; I < CtrlObject->Plugins.GetPluginsCount(); I++) {
					Plugin *pPlugin = CtrlObject->Plugins.GetPlugin(I);

					if (!StrCmpI(pPlugin->GetModuleName(), strPluginModule)) {
						if (pPlugin->HasOpenPlugin()) {
							HANDLE hNewPlugin = CtrlObject->Plugins.OpenPlugin(pPlugin, OPEN_SHORTCUT,
									(INT_PTR)strPluginData.CPtr());

							if (hNewPlugin != INVALID_HANDLE_VALUE) {
								int CurFocus = SrcPanel->GetFocus();

								Panel *NewPanel =
										CtrlObject->Cp()->ChangePanel(SrcPanel, FILE_PANEL, TRUE, TRUE);
								NewPanel->SetPluginMode(hNewPlugin, L"",
										CurFocus
												|| !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible());

								if (!strShortcutFolder.IsEmpty())
									CtrlObject->Plugins.SetDirectory(hNewPlugin, strShortcutFolder, 0);

								NewPanel->Update(0);
								NewPanel->Show();
							}
						}

						break;
					}
				}
			}

			return true;
		}

		switch (CheckShortcutFolder(&strShortcutFolder, FALSE)) {
			case 0:
				// return FALSE;
			case -1:
				return true;
		}

		/*
		if (SrcPanel->GetType()!=FILE_PANEL)
		{
			SrcPanel=CtrlObject->Cp()->ChangePanel(SrcPanel,FILE_PANEL,TRUE,TRUE);
		}
		*/

		SrcPanel->SetCurDir(strShortcutFolder, TRUE);

		if (CheckFullScreen != SrcPanel->IsFullScreen())
			CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->Show();

		SrcPanel->Redraw();
		return true;
	}
	return false;
}

// Just as FindPartName(), but with retry support through keyboard layout translation, specially for FastFind
bool Panel::FindPartNameXLat(const wchar_t *Name, int Next, int Direct, int ExcludeSets)
{
	if (FindPartName(Name, Next, Direct, ExcludeSets)) {
		return true;
	}

	if (!Opt.XLat.EnableForFastFileFind) {
		return false;
	}

	const size_t NameLen = wcslen(Name);
	StackHeapArray<wchar_t, 0x200> NameXlat(NameLen + 1);

	Xlator xlt(0);
	for (size_t i = 0; i < NameLen; ++i) {
		NameXlat[i] = xlt.Transcode(Name[i]);
		NameXlat[i + 1] = 0;
		if (!FindPartName(NameXlat.Get(), Next, Direct, ExcludeSets)) {
			NameXlat[i] = Name[i];
			if (!FindPartName(NameXlat.Get(), Next, Direct, ExcludeSets)) {
				return false;
			}
		}
	}
	return true;
}
