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
#pragma hdrstop

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
#include "hotplug.hpp"
#include "clipboard.hpp"
#include "config.hpp"
#include "scrsaver.hpp"
#include "execute.hpp"
#include "ffolders.hpp"
#include "options.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "constitle.hpp"
#include "FarDlgBuilder.hpp"
#include "setattr.hpp"
#include "palette.hpp"
#include "panel.hpp"
#include "drivemix.hpp"

static int DragX,DragY,DragMove;
static Panel *SrcDragPanel;
static SaveScreen *DragSaveScr=nullptr;
static FARString strDragName;

/* $ 21.08.2002 IS
   Класс для хранения пункта плагина в меню выбора дисков
*/
class ChDiskPluginItem
{
	public:
		MenuItemEx Item;
		WCHAR HotKey;

		ChDiskPluginItem() { Clear(); }
		~ChDiskPluginItem() {}

		void Clear() { HotKey = 0; Item.Clear(); }
		bool operator==(const ChDiskPluginItem &rhs) const { return HotKey==rhs.HotKey && !StrCmpI(Item.strName,rhs.Item.strName) && Item.UserData==rhs.Item.UserData; }
		int operator<(const ChDiskPluginItem &rhs) const {return StrCmpI(Item.strName,rhs.Item.strName)<0;}
		const ChDiskPluginItem& operator=(const ChDiskPluginItem &rhs);
};

const ChDiskPluginItem& ChDiskPluginItem::operator=(const ChDiskPluginItem &rhs)
{
	if (this != &rhs)
	{
		Item=rhs.Item;
		HotKey=rhs.HotKey;
	}

	return *this;
}


Panel::Panel():
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
	SrcDragPanel=nullptr;
	DragX=DragY=-1;
};


Panel::~Panel()
{
	_OT(SysLog(L"[%p] Panel::~Panel()", this));
	EndDrag();
}


void Panel::SetViewMode(int ViewMode)
{
	PrevViewMode=ViewMode;
	Panel::ViewMode=ViewMode;
};


void Panel::ChangeDirToCurrent()
{
	FARString strNewDir;
	apiGetCurrentDirectory(strNewDir);
	SetCurDir(strNewDir,TRUE);
}


void Panel::ChangeDisk()
{
	int Pos=0,FirstCall=TRUE;

	if (!strCurDir.IsEmpty() && strCurDir.At(1)==L':')
	{
		Pos=Upper(strCurDir.At(0))-L'A';
	}

	while (Pos!=-1)
	{
		Pos=ChangeDiskMenu(Pos,FirstCall);
		FirstCall=FALSE;
	}
}


struct PanelMenuItem
{
	bool bIsPlugin;

	struct
	{
		Plugin *pPlugin;
		int nItem;
	};

	wchar_t root[0x100];
};

struct TypeMessage
{
	int DrvType;
	int FarMsg;
};

const TypeMessage DrTMsg[]=
{
	{DRIVE_REMOVABLE,MChangeDriveRemovable},
	{DRIVE_FIXED,MChangeDriveFixed},
	{DRIVE_REMOTE,MChangeDriveNetwork},
	{DRIVE_REMOTE_NOT_CONNECTED,MChangeDriveDisconnectedNetwork},
	{DRIVE_CDROM,MChangeDriveCDROM},
	{DRIVE_CD_RW,MChangeDriveCD_RW},
	{DRIVE_CD_RWDVD,MChangeDriveCD_RWDVD},
	{DRIVE_DVD_ROM,MChangeDriveDVD_ROM},
	{DRIVE_DVD_RW,MChangeDriveDVD_RW},
	{DRIVE_DVD_RAM,MChangeDriveDVD_RAM},
	{DRIVE_BD_ROM,MChangeDriveBD_ROM},
	{DRIVE_BD_RW,MChangeDriveBD_RW},
	{DRIVE_HDDVD_ROM,MChangeDriveHDDVD_ROM},
	{DRIVE_HDDVD_RW,MChangeDriveHDDVD_RW},
	{DRIVE_RAMDISK,MChangeDriveRAM},
	{DRIVE_SUBSTITUTE,MChangeDriveSUBST},
	{DRIVE_VIRTUAL,MChangeDriveVirtual},
	{DRIVE_USBDRIVE,MChangeDriveRemovable},
};

static size_t AddPluginItems(VMenu &ChDisk, int Pos, int DiskCount, bool SetSelected)
{
	TArray<ChDiskPluginItem> MPItems;
	ChDiskPluginItem OneItem;
	// Список дополнительных хоткеев, для случая, когда плагинов, добавляющих пункт в меню, больше 9.
	int PluginItem, PluginNumber = 0; // IS: счетчики - плагинов и пунктов плагина
	bool ItemPresent,Done=false;
	FARString strMenuText;
	FARString strPluginText;
	size_t PluginMenuItemsCount = 0;

	while (!Done)
	{
		for (PluginItem=0;; ++PluginItem)
		{
			if (PluginNumber >= CtrlObject->Plugins.GetPluginsCount())
			{
				Done=true;
				break;
			}

			Plugin *pPlugin = CtrlObject->Plugins.GetPlugin(PluginNumber);

			WCHAR HotKey = 0;
			if (!CtrlObject->Plugins.GetDiskMenuItem(
			            pPlugin,
			            PluginItem,
			            ItemPresent,
			            HotKey,
			            strPluginText
			        ))
			{
				Done=true;
				break;
			}

			if (!ItemPresent)
				break;

			strMenuText = strPluginText;

			if (!strMenuText.IsEmpty())
			{
				OneItem.Clear();
				PanelMenuItem *item = new PanelMenuItem;
				item->bIsPlugin = true;
				item->pPlugin = pPlugin;
				item->nItem = PluginItem;

				if (pPlugin->IsOemPlugin())
					OneItem.Item.Flags=LIF_CHECKED|L'A';

				OneItem.Item.strName = strMenuText;
				OneItem.Item.UserDataSize=sizeof(PanelMenuItem);
				OneItem.Item.UserData=(char*)item;
				OneItem.HotKey=HotKey;
				ChDiskPluginItem *pResult = MPItems.addItem(OneItem);

				if (pResult)
				{
					pResult->Item.UserData = (char*)item; //BUGBUG, это фантастика просто. Исправить!!!! связано с работой TArray
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
		} // END: for (PluginItem=0;;++PluginItem)

		++PluginNumber;
	}

	MPItems.Sort();
	MPItems.Pack(); // выкинем дубли
	PluginMenuItemsCount=MPItems.getSize();

	if (PluginMenuItemsCount)
	{
		MenuItemEx ChDiskItem;

		ChDiskItem.Clear();
		ChDiskItem.Flags|=LIF_SEPARATOR;
		ChDiskItem.UserDataSize=0;
		ChDisk.AddItem(&ChDiskItem);
		ChDiskItem.Flags&=~LIF_SEPARATOR;

		for (size_t I=0; I < PluginMenuItemsCount; ++I)
		{
			if (Pos > DiskCount && !SetSelected)
			{
				MPItems.getItem(I)->Item.SetSelect(DiskCount+static_cast<int>(I)+1==Pos);

				if (!SetSelected)
					SetSelected=DiskCount+static_cast<int>(I)+1==Pos;
			}
			const wchar_t HotKeyStr[]={MPItems.getItem(I)->HotKey?L'&':L' ',MPItems.getItem(I)->HotKey?MPItems.getItem(I)->HotKey:L' ',L' ',MPItems.getItem(I)->HotKey?L' ':L'\0',L'\0'};
			MPItems.getItem(I)->Item.strName = FARString(HotKeyStr) + MPItems.getItem(I)->Item.strName;
			ChDisk.AddItem(&MPItems.getItem(I)->Item);

			delete(PanelMenuItem*)MPItems.getItem(I)->Item.UserData;  //ммда...
		}
	}
	return PluginMenuItemsCount;
}

static void ConfigureChangeDriveMode()
{
	DialogBuilder Builder(MChangeDriveConfigure, L"");
	Builder.AddCheckbox(MChangeDriveShowDiskType, &Opt.ChangeDriveMode, DRIVE_SHOW_TYPE);
	Builder.AddCheckbox(MChangeDriveShowNetworkName, &Opt.ChangeDriveMode, DRIVE_SHOW_NETNAME);
	Builder.AddCheckbox(MChangeDriveShowLabel, &Opt.ChangeDriveMode, DRIVE_SHOW_LABEL);
	Builder.AddCheckbox(MChangeDriveShowFileSystem, &Opt.ChangeDriveMode, DRIVE_SHOW_FILESYSTEM);

	BOOL ShowSizeAny = Opt.ChangeDriveMode & (DRIVE_SHOW_SIZE | DRIVE_SHOW_SIZE_FLOAT);

	DialogItemEx *ShowSize = Builder.AddCheckbox(MChangeDriveShowSize, &ShowSizeAny);
	DialogItemEx *ShowSizeFloat = Builder.AddCheckbox(MChangeDriveShowSizeFloat, &Opt.ChangeDriveMode, DRIVE_SHOW_SIZE_FLOAT);
	ShowSizeFloat->Indent(3);
	Builder.LinkFlags(ShowSize, ShowSizeFloat, DIF_DISABLE);

	Builder.AddCheckbox(MChangeDriveShowRemovableDrive, &Opt.ChangeDriveMode, DRIVE_SHOW_REMOVABLE);
	Builder.AddCheckbox(MChangeDriveShowPlugins, &Opt.ChangeDriveMode, DRIVE_SHOW_PLUGINS);
	Builder.AddCheckbox(MChangeDriveShowCD, &Opt.ChangeDriveMode, DRIVE_SHOW_CDROM);
	Builder.AddCheckbox(MChangeDriveShowNetworkDrive, &Opt.ChangeDriveMode, DRIVE_SHOW_REMOTE);

	Builder.AddOKCancel();
	if (Builder.ShowDialog())
	{
		if (ShowSizeAny)
		{
			bool ShowSizeFloat = (Opt.ChangeDriveMode & DRIVE_SHOW_SIZE_FLOAT) ? true : false;
			if (ShowSizeFloat)
				Opt.ChangeDriveMode &= ~DRIVE_SHOW_SIZE;
			else
				Opt.ChangeDriveMode |= DRIVE_SHOW_SIZE;
		}
		else
			Opt.ChangeDriveMode &= ~(DRIVE_SHOW_SIZE | DRIVE_SHOW_SIZE_FLOAT);
	}
}


LONG_PTR WINAPI ChDiskDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	switch (Msg)
	{
		case DN_CTLCOLORDLGITEM:
		{
			if (Param1 == 1) // BUGBUG, magic number
			{
				int Color=FarColorToReal(COL_WARNDIALOGTEXT);
				return ((Param2&0xFF00FF00)|(Color<<16)|Color);
			}
		}
		break;
	}
	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

bool IsCharTrimmable(wchar_t c)
{
	return (c==L' ' || c==L'\t' || c==L'\r' || c==L'\n');
}

void Trim(std::wstring &s)
{
		while (!s.empty() && IsCharTrimmable(s[0])) s.erase(0, 1);
		while (!s.empty() && IsCharTrimmable(s[s.size()-1])) s.resize(s.size() - 1);
}

std::wstring ExtractTilSpace(std::wstring &s)
{
	Trim(s);
	size_t p = s.find(' ');
	size_t p2 = s.find('\t');
	if (p==std::wstring::npos || (p > p2 && p2!=std::wstring::npos)) p = p2;

	std::wstring rv;
	if (p!=std::wstring::npos) {
		rv = s.substr(0, p);
		s.erase(0, p + 1);
	} else 
		rv.swap(s);
	return rv;
}

struct MountedFS
{
	std::wstring root;
	std::wstring fs;
};

typedef std::vector<MountedFS>	MountedFilesystems;

void EnumMountedFilesystems(MountedFilesystems &out, const WCHAR *another = NULL)
{
	bool has_root = false;
	MountedFS mfs;

	out.clear();

	
	FILE *f = popen("df -T", "r");
	if (f) {
		char buf[0x400] = { };
		std::wstring s, tmp;
		bool first = true;
		while (fgets(buf, sizeof(buf)-1, f)!=NULL) {
			if (!first) {
				buf[sizeof(buf)-1] = 0;
				s = MB2Wide(buf);
				
				ExtractTilSpace(s);
				mfs.fs = ExtractTilSpace(s);
				mfs.root.clear();
				for (;;) {
					tmp = ExtractTilSpace(s);
					if (tmp.empty()) break;
					if (tmp[0]=='/') {
						mfs.root = tmp;
						if (tmp.size()==1) has_root = true;
					}
				}
				if (mfs.root==L"/")
					out.insert(out.begin(), mfs);
				else
					out.push_back(mfs);
			}
			first = false;
		}
		pclose(f);
	}

	if (!has_root) {
		MountedFS mfs;
		mfs.fs = L"/";
		mfs.root = L"/";
		out.insert(out.begin(), mfs);
	}
	
	mfs.root = MB2Wide(getenv("HOME"));
	mfs.fs = L"~";
	out.insert(out.begin(), mfs);	
	if (another && mfs.root!=another) {
		mfs.root = another;
		mfs.fs = L"";
		out.insert(out.begin(), mfs);			
	}
}


FARString StringPrepend(FARString s, size_t width)
{
	//fprintf(stderr, "%u %u\n", s.GetSize(), width);
	while (s.GetLength() < width) {
		s.Insert(0, ' ');
	}
	return s;
}



int Panel::ChangeDiskMenu(int Pos,int FirstCall)
{
	/*Events.DeviceArivalEvent.Reset();
	Events.DeviceRemoveEvent.Reset();
	Events.MediaArivalEvent.Reset();
	Events.MediaRemoveEvent.Reset();*/

	class Guard_Macro_DskShowPosType  //фигня какая-то
	{
		public:
			Guard_Macro_DskShowPosType(Panel *curPanel) {Macro_DskShowPosType=(curPanel==CtrlObject->Cp()->LeftPanel)?1:2;};
			~Guard_Macro_DskShowPosType() {Macro_DskShowPosType=0;};
	};
	Guard_Macro_DskShowPosType _guard_Macro_DskShowPosType(this);
	MenuItemEx ChDiskItem;
	FARString strDiskType, strRootDir, strDiskLetter;
//	DWORD Mask = 4,DiskMask;
	int Focus;//DiskCount = 1
	WCHAR I;
	bool SetSelected=false;
	DWORD NetworkMask = 0;

	FARString curdir, another_curdir;
	GetCurDir(curdir);
	CtrlObject->Cp()->GetAnotherPanel(this)->GetCurDir(another_curdir);
	MountedFilesystems filesystems;
	EnumMountedFilesystems(filesystems, (another_curdir==curdir) ? NULL : another_curdir.CPtr());

	PanelMenuItem Item, *mitem=0;
	{ // эта скобка надо, см. M#605
		VMenu ChDisk(MSG(MChangeDriveTitle),nullptr,0,ScrY-Y1-3);
		ChDisk.SetBottomTitle(MSG(MChangeDriveMenuFooter));
		ChDisk.SetFlags(VMENU_NOTCENTER);

		if (this == CtrlObject->Cp()->LeftPanel)
			ChDisk.SetFlags(VMENU_LEFTMOST);

		ChDisk.SetHelp(L"DriveDlg");
		ChDisk.SetFlags(VMENU_WRAPMODE);
		FARString strMenuText;
		int MenuLine = 0;

		/* $ 02.04.2001 VVM
		! Попытка не будить спящие диски... */
		size_t root_width = 4,  fs_width = 4;
		for (auto f : filesystems) {
			if (root_width < f.root.size()) root_width = f.root.size();
			if (fs_width < f.fs.size()) fs_width = f.fs.size();
		}
		
		for (auto f : filesystems) {
			strMenuText.Clear();
			strMenuText += StringPrepend(f.root.c_str(), root_width);
			strMenuText += BoxSymbols[BS_V1];
			strMenuText += StringPrepend(f.fs.c_str(), fs_width);


			ChDiskItem.Clear();

			if (FirstCall)
			{
				ChDiskItem.SetSelect(I==Pos);

				if (!SetSelected)
					SetSelected=(I==Pos);
			}
			else
			{
				if (Pos < filesystems.size())
				{
					ChDiskItem.SetSelect(MenuLine==Pos);

					if (!SetSelected)
						SetSelected=(MenuLine==Pos);
				}
			}

			ChDiskItem.strName = strMenuText;

			PanelMenuItem item;
			item.bIsPlugin = false;
			wcsncpy(item.root, f.root.c_str(), sizeof(item.root)/sizeof(item.root[0]));
			ChDisk.SetUserData(&item, sizeof(item), ChDisk.AddItem(&ChDiskItem));
			MenuLine++;
		}

		size_t PluginMenuItemsCount=0;

		if (Opt.ChangeDriveMode & DRIVE_SHOW_PLUGINS)
		{
			PluginMenuItemsCount = AddPluginItems(ChDisk, Pos, filesystems.size(), SetSelected);
		}

		int X=X1+5;

		if ((this == CtrlObject->Cp()->RightPanel) && IsFullScreen() && (X2-X1 > 40))
			X = (X2-X1+1)/2+5;

		int Y = (ScrY+1-(filesystems.size()+static_cast<int>(PluginMenuItemsCount)+5))/2;

		if (Y < 1)
			Y=1;

		ChDisk.SetPosition(X,Y,0,0);

		if (Y < 3)
			ChDisk.SetBoxType(SHORT_DOUBLE_BOX);

		ChDisk.Show();

		while (!ChDisk.Done())
		{
			int Key;
			/*if(Events.DeviceArivalEvent.Signaled() || Events.DeviceRemoveEvent.Signaled() || Events.MediaArivalEvent.Signaled() || Events.MediaRemoveEvent.Signaled())
			{
				Key=KEY_CTRLR;
			}
			else*/
			{
				{ //очередная фигня
					ChangeMacroMode MacroMode(MACRO_DISKS);
					Key=ChDisk.ReadInput();
				}
			}
			int SelPos=ChDisk.GetSelectPos();
			PanelMenuItem *item = (PanelMenuItem*)ChDisk.GetUserData(nullptr,0);

			switch (Key)
			{
				// Shift-Enter в меню выбора дисков вызывает проводник для данного диска
				case KEY_SHIFTNUMENTER:
				case KEY_SHIFTENTER:
				{
					if (item && !item->bIsPlugin)
					{
						Execute(item->root,FALSE,TRUE,TRUE);
					}
				}
				break;
				case KEY_CTRLPGUP:
				case KEY_CTRLNUMPAD9:
				{
					if (Opt.PgUpChangeDisk)
						return -1;
				}
				break;
				// Т.к. нет способа получить состояние "открытости" устройства,
				// то добавим обработку Ins для CD - "закрыть диск"
				case KEY_INS:
				case KEY_NUMPAD0:
				{
					if (item && !item->bIsPlugin)
					{
					}
				}
				break;
				case KEY_NUMDEL:
				case KEY_DEL:
				{
					if (item && !item->bIsPlugin)
					{
						int Code = DisconnectDrive(item, ChDisk);
						if (Code != DRIVE_DEL_FAIL && Code != DRIVE_DEL_NONE)
						{
							ScrBuf.Lock(); // отменяем всякую прорисовку
							FrameManager->ResizeAllFrame();
							FrameManager->PluginCommit(); // коммитим.
							ScrBuf.Unlock(); // разрешаем прорисовку
							return (((filesystems.size()-SelPos)==1) && (SelPos > 0) && (Code != DRIVE_DEL_EJECT))?SelPos-1:SelPos;
						}
					}
				}
				break;
				case KEY_CTRLA:
				case KEY_F4:
				{
					if (item)
					{
						if (!item->bIsPlugin)
						{
							ShellSetFileAttributes(nullptr, item->root);
							ChDisk.Redraw();
						}
						else
						{
							FARString strRegKey;
							CtrlObject->Plugins.GetHotKeyRegKey(item->pPlugin, item->nItem,strRegKey);
							FARString strName = ChDisk.GetItemPtr(SelPos)->strName + 3;
							RemoveExternalSpaces(strName);
							if(CtrlObject->Plugins.SetHotKeyDialog(strName, strRegKey, L"DriveMenuHotkey"))
							{
								return SelPos;
							}
						}
					}
					break;
				}
				case KEY_SHIFTNUMDEL:
				case KEY_SHIFTDECIMAL:
				case KEY_SHIFTDEL:
				{
					if (item && !item->bIsPlugin)
					{
						RemoveHotplugDevice(item, ChDisk);
						return SelPos;
					}
				}
				break;
				case KEY_CTRL1:
				case KEY_RCTRL1:
					Opt.ChangeDriveMode ^= DRIVE_SHOW_TYPE;
					return SelPos ;
				case KEY_CTRL2:
				case KEY_RCTRL2:
					Opt.ChangeDriveMode ^= DRIVE_SHOW_NETNAME;
					return SelPos;
				case KEY_CTRL3:
				case KEY_RCTRL3:
					Opt.ChangeDriveMode ^= DRIVE_SHOW_LABEL;
					return SelPos;
				case KEY_CTRL4:
				case KEY_RCTRL4:
					Opt.ChangeDriveMode ^= DRIVE_SHOW_FILESYSTEM;
					return SelPos;
				case KEY_CTRL5:
				case KEY_RCTRL5:
				{
					if (Opt.ChangeDriveMode & DRIVE_SHOW_SIZE)
					{
						Opt.ChangeDriveMode ^= DRIVE_SHOW_SIZE;
						Opt.ChangeDriveMode |= DRIVE_SHOW_SIZE_FLOAT;
					}
					else
					{
						if (Opt.ChangeDriveMode & DRIVE_SHOW_SIZE_FLOAT)
							Opt.ChangeDriveMode ^= DRIVE_SHOW_SIZE_FLOAT;
						else
							Opt.ChangeDriveMode ^= DRIVE_SHOW_SIZE;
					}

					return SelPos;
				}
				case KEY_CTRL6:
				case KEY_RCTRL6:
					Opt.ChangeDriveMode ^= DRIVE_SHOW_REMOVABLE;
					return SelPos;
				case KEY_CTRL7:
				case KEY_RCTRL7:
					Opt.ChangeDriveMode ^= DRIVE_SHOW_PLUGINS;
					return SelPos;
				case KEY_CTRL8:
				case KEY_RCTRL8:
					Opt.ChangeDriveMode ^= DRIVE_SHOW_CDROM;
					return SelPos;
				case KEY_CTRL9:
				case KEY_RCTRL9:
					Opt.ChangeDriveMode ^= DRIVE_SHOW_REMOTE;
					return SelPos;
				case KEY_F9:
					ConfigureChangeDriveMode();
					return SelPos;
				case KEY_SHIFTF1:
				{
					if (item && item->bIsPlugin)
					{
						// Вызываем нужный топик, который передали в CommandsMenu()
						FarShowHelp(
						    item->pPlugin->GetModuleName(),
						    nullptr,
						    FHELP_SELFHELP|FHELP_NOSHOWERROR|FHELP_USECONTENTS
						);
					}

					break;
				}
				case KEY_ALTSHIFTF9:

					if (Opt.ChangeDriveMode&DRIVE_SHOW_PLUGINS)
					{
						ChDisk.Hide();
						CtrlObject->Plugins.Configure();
					}

					return SelPos;
				case KEY_SHIFTF9:

					if (item && item->bIsPlugin && item->pPlugin->HasConfigure())
						CtrlObject->Plugins.ConfigureCurrent(item->pPlugin, item->nItem);

					return SelPos;
				case KEY_CTRLR:
					return SelPos;
				default:
					ChDisk.ProcessInput();
					break;
			}

			if (ChDisk.Done() &&
			        (ChDisk.Modal::GetExitCode() < 0) &&
			        !strCurDir.IsEmpty() &&
			        (StrCmpN(strCurDir,L"//",2) ))
			{
				const wchar_t RootDir[4] = {strCurDir.At(0),L':',GOOD_SLASH,L'\0'};

				if (FAR_GetDriveType(RootDir) == DRIVE_NO_ROOT_DIR)
					ChDisk.ClearDone();
			}
		} // while (!Done)

		if (ChDisk.Modal::GetExitCode()<0)
			return -1;

		mitem=(PanelMenuItem*)ChDisk.GetUserData(nullptr,0);

		if (mitem)
		{
			Item=*mitem;
			mitem=&Item;
		}
	} // эта скобка надо, см. M#605

	

	if (ProcessPluginEvent(FE_CLOSE,nullptr))
		return -1;

	ScrBuf.Flush();
	INPUT_RECORD rec;
	PeekInputRecord(&rec);

	if (!mitem)
		return -1; //???

	if (!mitem->bIsPlugin)
	{
		for (;;)
		{
			fprintf(stderr, "chdisk: %ls\n", mitem->root);
			if (FarChDir(mitem->root))
			{
				break;
			}
			return  -1;
			/*else
			{
				//NewDir[2]=GOOD_SLASH;

				if (FarChDir(NewDir))
				{
					break;
				}
			}
			
			
			enum
			{
				CHDISKERROR_DOUBLEBOX,
				CHDISKERROR_TEXT0,
				CHDISKERROR_TEXT1,
				CHDISKERROR_FIXEDIT,
				CHDISKERROR_TEXT2,
				CHDISKERROR_SEPARATOR,
				CHDISKERROR_BUTTON_OK,
				CHDISKERROR_BUTTON_CANCEL,
			};
			//const wchar_t Drive[]={mitem->cDrive,L'\0'};
			FARString strError;
			GetErrorString(strError);
			int Len1=static_cast<int>(strError.GetLength());
			int Len2=StrLength(MSG(MChangeDriveCannotReadDisk));
			int MaxMsg=Min(Max(Len1,Len2), static_cast<int>(MAX_WIDTH_MESSAGE));
			const int DX=Max(MaxMsg+13,40),DY=8;
			const DialogDataEx ChDiskData[]=
			{
				DI_DOUBLEBOX,3,1,DX-4,DY-2,0,0,MSG(MError),
				DI_EDIT,5,2,DX-6,2,0,DIF_READONLY,strError.CPtr(),
				DI_TEXT,5,3,DX-9,3,0,0,MSG(MChangeDriveCannotReadDisk),
				DI_FIXEDIT,5+Len2+1,3,5+Len2+1,3,0,DIF_FOCUS,mitem->root.c_str(),
				DI_TEXT,5+Len2+2,3,5+Len2+2,3,0,0,L":",
				DI_TEXT,3,DY-4,0,DY-4,0,DIF_SEPARATOR,L"",
				DI_BUTTON,0,DY-3,0,DY-3,0,DIF_DEFAULT|DIF_CENTERGROUP,MSG(MRetry),
				DI_BUTTON,0,DY-3,0,DY-3,0,DIF_CENTERGROUP,MSG(MCancel),
			};
			MakeDialogItemsEx(ChDiskData,ChDiskDlg);
			Dialog Dlg(ChDiskDlg, ARRAYSIZE(ChDiskData), ChDiskDlgProc, 0);
			Dlg.SetPosition(-1,-1,DX,DY);
			Dlg.SetDialogMode(DMODE_WARNINGSTYLE);
			Dlg.Process();
			if(Dlg.GetExitCode()==CHDISKERROR_BUTTON_OK)
			{
				mitem->cDrive=ChDiskDlg[CHDISKERROR_FIXEDIT].strData.At(0);
			}
			else
			{
				return -1;
			}*/
		}

		FARString strNewCurDir;
		apiGetCurrentDirectory(strNewCurDir);

		if ((PanelMode == NORMAL_PANEL) &&
		        (GetType() == FILE_PANEL) &&
		        !StrCmpI(strCurDir,strNewCurDir) &&
		        IsVisible())
		{
			// А нужно ли делать здесь Update????
			Update(UPDATE_KEEP_SELECTION);
		}
		else
		{
			Focus=GetFocus();
			Panel *NewPanel=CtrlObject->Cp()->ChangePanel(this, FILE_PANEL, TRUE, FALSE);
			NewPanel->SetCurDir(strNewCurDir,TRUE);
			NewPanel->Show();

			if (Focus || !CtrlObject->Cp()->GetAnotherPanel(this)->IsVisible())
				NewPanel->SetFocus();

			if (!Focus && CtrlObject->Cp()->GetAnotherPanel(this)->GetType() == INFO_PANEL)
				CtrlObject->Cp()->GetAnotherPanel(this)->UpdateKeyBar();
		}
	}
	else //эта плагин, да
	{
		HANDLE hPlugin = CtrlObject->Plugins.OpenPlugin(
		                     mitem->pPlugin,
		                     OPEN_DISKMENU,
		                     mitem->nItem
		                 );

		if (hPlugin != INVALID_HANDLE_VALUE)
		{
			Focus=GetFocus();
			Panel *NewPanel = CtrlObject->Cp()->ChangePanel(this,FILE_PANEL,TRUE,TRUE);
			NewPanel->SetPluginMode(hPlugin,L"",Focus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible());
			NewPanel->Update(0);
			NewPanel->Show();

			if (!Focus && CtrlObject->Cp()->GetAnotherPanel(this)->GetType() == INFO_PANEL)
				CtrlObject->Cp()->GetAnotherPanel(this)->UpdateKeyBar();
		}
	}

	return -1;
}

int Panel::DisconnectDrive(PanelMenuItem *item, VMenu &ChDisk)
{
	return -1;
}

void Panel::RemoveHotplugDevice(PanelMenuItem *item, VMenu &ChDisk)
{
}

/* $ 28.12.2001 DJ
   обработка Del в меню дисков
*/

int Panel::ProcessDelDisk(wchar_t Drive, int DriveType,VMenu *ChDiskMenu)
{
	FARString strMsgText;
	wchar_t DiskLetter[]={Drive,L':',0};

	switch(DriveType)
	{
	case DRIVE_SUBSTITUTE:
		{
			if (Opt.Confirm.RemoveSUBST)
			{
				strMsgText.Format(MSG(MChangeSUBSTDisconnectDriveQuestion),Drive);
				if (Message(MSG_WARNING,2,MSG(MChangeSUBSTDisconnectDriveTitle),strMsgText,MSG(MYes),MSG(MNo)))
				{
					return DRIVE_DEL_FAIL;
				}
			}
			/*if (DelSubstDrive(DiskLetter))
			{
				return DRIVE_DEL_SUCCESS;
			}
			else*/
			{
				int LastError=WINPORT(GetLastError)();
				strMsgText.Format(MSG(MChangeDriveCannotDelSubst),DiskLetter);
				if (LastError==ERROR_OPEN_FILES || LastError==ERROR_DEVICE_IN_USE)
				{
					if (!Message(MSG_WARNING|MSG_ERRORTYPE,2,MSG(MError),strMsgText,
								L"\x1",MSG(MChangeDriveOpenFiles),
								MSG(MChangeDriveAskDisconnect),MSG(MOk),MSG(MCancel)))
					{
						/*if (DelSubstDrive(DiskLetter))
						{
							return DRIVE_DEL_SUCCESS;
						}*/
					}
					else
					{
						return DRIVE_DEL_FAIL;
					}
				}
				Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MError),strMsgText,MSG(MOk));
			}
			return DRIVE_DEL_FAIL; // блин. в прошлый раз забыл про это дело...
		}
		break;

	case DRIVE_REMOTE:
		{
		}
		break;

	case DRIVE_VIRTUAL:
		{
			if (Opt.Confirm.DetachVHD)
			{
				strMsgText.Format(MSG(MChangeVHDDisconnectDriveQuestion),Drive);
				if (Message(MSG_WARNING,2,MSG(MChangeVHDDisconnectDriveTitle),strMsgText,MSG(MYes),MSG(MNo)))
				{
					return DRIVE_DEL_FAIL;
				}
			}
		}
		break;

	}
	return DRIVE_DEL_FAIL;
}


void Panel::FastFindProcessName(Edit *FindEdit,const wchar_t *Src,FARString &strLastName,FARString &strName)
{
	wchar_t *Ptr=(wchar_t *)xf_malloc((StrLength(Src)+StrLength(FindEdit->GetStringAddr())+32)*sizeof(wchar_t));

	if (Ptr)
	{
		wcscpy(Ptr,FindEdit->GetStringAddr());
		wchar_t *EndPtr=Ptr+StrLength(Ptr);
		wcscat(Ptr,Src);
		Unquote(EndPtr);
		EndPtr=Ptr+StrLength(Ptr);
		DWORD Key;

		for (;;)
		{
			if (EndPtr == Ptr)
			{
				Key=KEY_NONE;
				break;
			}

			if (FindPartName(Ptr,FALSE,1,1))
			{
				Key=*(EndPtr-1);
				*EndPtr=0;
				FindEdit->SetString(Ptr);
				strLastName = Ptr;
				strName = Ptr;
				FindEdit->Show();
				break;
			}

			*--EndPtr=0;
		}

		xf_free(Ptr);
	}
}

int64_t Panel::VMProcess(int OpCode,void *vParam,int64_t iParam)
{
	return 0;
}

// корректировка букв
static DWORD _CorrectFastFindKbdLayout(INPUT_RECORD *rec,DWORD Key)
{
	if ((Key&KEY_ALT))// && Key!=(KEY_ALT|0x3C))
	{
		// // _SVS(SysLog(L"_CorrectFastFindKbdLayout>>> %ls | %ls",_FARKEY_ToName(Key),_INPUT_RECORD_Dump(rec)));
		if (rec->Event.KeyEvent.uChar.UnicodeChar && (Key&KEY_MASKF) != rec->Event.KeyEvent.uChar.UnicodeChar) //???
			Key=(Key&0xFFF10000)|rec->Event.KeyEvent.uChar.UnicodeChar;   //???

		// // _SVS(SysLog(L"_CorrectFastFindKbdLayout<<< %ls | %ls",_FARKEY_ToName(Key),_INPUT_RECORD_Dump(rec)));
	}

	return Key;
}

void Panel::FastFind(int FirstKey)
{
	// // _SVS(CleverSysLog Clev(L"Panel::FastFind"));
	INPUT_RECORD rec;
	FARString strLastName, strName;
	int Key,KeyToProcess=0;
	WaitInFastFind++;
	{
		int FindX=Min(X1+9,ScrX-22);
		int FindY=Min(Y2,ScrY-2);
		ChangeMacroMode MacroMode(MACRO_SEARCH);
		SaveScreen SaveScr(FindX,FindY,FindX+21,FindY+2);
		FastFindShow(FindX,FindY);
		Edit FindEdit;
		FindEdit.SetPosition(FindX+2,FindY+1,FindX+19,FindY+1);
		FindEdit.SetEditBeyondEnd(FALSE);
		FindEdit.SetObjectColor(COL_DIALOGEDIT);
		FindEdit.Show();

		while (!KeyToProcess)
		{
			if (FirstKey)
			{
				FirstKey=_CorrectFastFindKbdLayout(FrameManager->GetLastInputRecord(),FirstKey);
				// // _SVS(SysLog(L"Panel::FastFind  FirstKey=%ls  %ls",_FARKEY_ToName(FirstKey),_INPUT_RECORD_Dump(FrameManager->GetLastInputRecord())));
				// // _SVS(SysLog(L"if (FirstKey)"));
				Key=FirstKey;
			}
			else
			{
				// // _SVS(SysLog(L"else if (FirstKey)"));
				Key=GetInputRecord(&rec);

				if (rec.EventType==MOUSE_EVENT)
				{
					if (!(rec.Event.MouseEvent.dwButtonState & 3))
						continue;
					else
						Key=KEY_ESC;
				}
				else if (!rec.EventType || rec.EventType==KEY_EVENT || rec.EventType==FARMACRO_KEY_EVENT)
				{
					// для вставки воспользуемся макродвижком...
					if (Key==KEY_CTRLV || Key==KEY_SHIFTINS || Key==KEY_SHIFTNUMPAD0)
					{
						wchar_t *ClipText=PasteFromClipboard();

						if (ClipText)
						{
							if (*ClipText)
							{
								FastFindProcessName(&FindEdit,ClipText,strLastName,strName);
								FastFindShow(FindX,FindY);
							}

							xf_free(ClipText);
						}

						continue;
					}
					else if (Key == KEY_OP_XLAT)
					{
						FARString strTempName;
						FindEdit.Xlat();
						FindEdit.GetString(strTempName);
						FindEdit.SetString(L"");
						FastFindProcessName(&FindEdit,strTempName,strLastName,strName);
						FastFindShow(FindX,FindY);
						continue;
					}
					else if (Key == KEY_OP_PLAINTEXT)
					{
						FARString strTempName;
						FindEdit.ProcessKey(Key);
						FindEdit.GetString(strTempName);
						FindEdit.SetString(L"");
						FastFindProcessName(&FindEdit,strTempName,strLastName,strName);
						FastFindShow(FindX,FindY);
						continue;
					}
					else
						Key=_CorrectFastFindKbdLayout(&rec,Key);
				}
			}

			if (Key==KEY_ESC || Key==KEY_F10)
			{
				KeyToProcess=KEY_NONE;
				break;
			}

			// // _SVS(if (!FirstKey) SysLog(L"Panel::FastFind  Key=%ls  %ls",_FARKEY_ToName(Key),_INPUT_RECORD_Dump(&rec)));
			if (Key>=KEY_ALT_BASE+0x01 && Key<=KEY_ALT_BASE+65535)
				Key=Lower(static_cast<WCHAR>(Key-KEY_ALT_BASE));

			if (Key>=KEY_ALTSHIFT_BASE+0x01 && Key<=KEY_ALTSHIFT_BASE+65535)
				Key=Lower(static_cast<WCHAR>(Key-KEY_ALTSHIFT_BASE));

			if (Key==KEY_MULTIPLY)
				Key=L'*';

			switch (Key)
			{
				case KEY_F1:
				{
					FindEdit.Hide();
					SaveScr.RestoreArea();
					{
						Help Hlp(L"FastFind");
					}
					FindEdit.Show();
					FastFindShow(FindX,FindY);
					break;
				}
				case KEY_CTRLNUMENTER:
				case KEY_CTRLENTER:
					FindPartName(strName,TRUE,1,1);
					FindEdit.Show();
					FastFindShow(FindX,FindY);
					break;
				case KEY_CTRLSHIFTNUMENTER:
				case KEY_CTRLSHIFTENTER:
					FindPartName(strName,TRUE,-1,1);
					FindEdit.Show();
					FastFindShow(FindX,FindY);
					break;
				case KEY_NONE:
				case KEY_IDLE:
					break;
				default:

					if ((Key<32 || Key>=65536) && Key!=KEY_BS && Key!=KEY_CTRLY &&
					        Key!=KEY_CTRLBS && Key!=KEY_ALT && Key!=KEY_SHIFT &&
					        Key!=KEY_CTRL && Key!=KEY_RALT && Key!=KEY_RCTRL &&
					        !(Key==KEY_CTRLINS||Key==KEY_CTRLNUMPAD0) && !(Key==KEY_SHIFTINS||Key==KEY_SHIFTNUMPAD0))
					{
						KeyToProcess=Key;
						break;
					}

					if (FindEdit.ProcessKey(Key))
					{
						FindEdit.GetString(strName);

						// уберем двойные '**'
						if (strName.GetLength() > 1
						        && strName.At(strName.GetLength()-1) == L'*'
						        && strName.At(strName.GetLength()-2) == L'*')
						{
							strName.SetLength(strName.GetLength()-1);
							FindEdit.SetString(strName);
						}

						/* $ 09.04.2001 SVS
						   проблемы с быстрым поиском.
						   Подробнее в 00573.ChangeDirCrash.txt
						*/
						if (strName.At(0) == L'"')
						{
							strName.LShift(1);
							FindEdit.SetString(strName);
						}

						if (FindPartName(strName,FALSE,1,1))
						{
							strLastName = strName;
						}
						else
						{
							if (CtrlObject->Macro.IsExecuting())// && CtrlObject->Macro.GetLevelState() > 0) // если вставка макросом...
							{
								//CtrlObject->Macro.DropProcess(); // ... то дропнем макропроцесс
								//CtrlObject->Macro.PopState();
								;
							}

							FindEdit.SetString(strLastName);
							strName = strLastName;
						}

						FindEdit.Show();
						FastFindShow(FindX,FindY);
					}

					break;
			}

			FirstKey=0;
		}
	}
	WaitInFastFind--;
	Show();
	CtrlObject->MainKeyBar->Redraw();
	ScrBuf.Flush();
	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;

	if ((KeyToProcess==KEY_ENTER||KeyToProcess==KEY_NUMENTER) && ActivePanel->GetType()==TREE_PANEL)
		((TreeList *)ActivePanel)->ProcessEnter();
	else
		CtrlObject->Cp()->ProcessKey(KeyToProcess);
}


void Panel::FastFindShow(int FindX,int FindY)
{
	SetColor(COL_DIALOGTEXT);
	GotoXY(FindX+1,FindY+1);
	Text(L" ");
	GotoXY(FindX+20,FindY+1);
	Text(L" ");
	Box(FindX,FindY,FindX+21,FindY+2,COL_DIALOGBOX,DOUBLE_BOX);
	GotoXY(FindX+7,FindY);
	SetColor(COL_DIALOGBOXTITLE);
	Text(MSearchFileTitle);
}


void Panel::SetFocus()
{
	if (CtrlObject->Cp()->ActivePanel!=this)
	{
		CtrlObject->Cp()->ActivePanel->KillFocus();
		CtrlObject->Cp()->ActivePanel=this;
	}

	ProcessPluginEvent(FE_GOTFOCUS,nullptr);

	if (!GetFocus())
	{
		CtrlObject->Cp()->RedrawKeyBar();
		Focus=TRUE;
		Redraw();
		FarChDir(strCurDir);
	}
}


void Panel::KillFocus()
{
	Focus=FALSE;
	ProcessPluginEvent(FE_KILLFOCUS,nullptr);
	Redraw();
}


int  Panel::PanelProcessMouse(MOUSE_EVENT_RECORD *MouseEvent,int &RetCode)
{
	RetCode=TRUE;

	if (!ModalMode && !MouseEvent->dwMousePosition.Y)
	{
		if (MouseEvent->dwMousePosition.X==ScrX)
		{
			if (Opt.ScreenSaver && !(MouseEvent->dwButtonState & 3))
			{
				EndDrag();
				ScreenSaver(TRUE);
				return TRUE;
			}
		}
		else
		{
			if ((MouseEvent->dwButtonState & 3) && !MouseEvent->dwEventFlags)
			{
				EndDrag();

				if (!MouseEvent->dwMousePosition.X)
					CtrlObject->Cp()->ProcessKey(KEY_CTRLO);
				else
					ShellOptions(0,MouseEvent);

				return TRUE;
			}
		}
	}

	if (!IsVisible() ||
	        (MouseEvent->dwMousePosition.X<X1 || MouseEvent->dwMousePosition.X>X2 ||
	         MouseEvent->dwMousePosition.Y<Y1 || MouseEvent->dwMousePosition.Y>Y2))
	{
		RetCode=FALSE;
		return TRUE;
	}

	if (DragX!=-1)
	{
		if (!(MouseEvent->dwButtonState & 3))
		{
			EndDrag();

			if (!MouseEvent->dwEventFlags && SrcDragPanel!=this)
			{
				MoveToMouse(MouseEvent);
				Redraw();
				SrcDragPanel->ProcessKey(DragMove ? KEY_DRAGMOVE:KEY_DRAGCOPY);
			}

			return TRUE;
		}

		if (MouseEvent->dwMousePosition.Y<=Y1 || MouseEvent->dwMousePosition.Y>=Y2 ||
		        !CtrlObject->Cp()->GetAnotherPanel(SrcDragPanel)->IsVisible())
		{
			EndDrag();
			return TRUE;
		}

		if ((MouseEvent->dwButtonState & 2) && !MouseEvent->dwEventFlags)
			DragMove=!DragMove;

		if (MouseEvent->dwButtonState & 1)
		{
			if ((abs(MouseEvent->dwMousePosition.X-DragX)>15 || SrcDragPanel!=this) &&
			        !ModalMode)
			{
				if (SrcDragPanel->GetSelCount()==1 && !DragSaveScr)
				{
					SrcDragPanel->GoToFile(strDragName);
					SrcDragPanel->Show();
				}

				DragMessage(MouseEvent->dwMousePosition.X,MouseEvent->dwMousePosition.Y,DragMove);
				return TRUE;
			}
			else
			{
				delete DragSaveScr;
				DragSaveScr=nullptr;
			}
		}
	}

	if (!(MouseEvent->dwButtonState & 3))
		return TRUE;

	if ((MouseEvent->dwButtonState & 1) && !MouseEvent->dwEventFlags &&
	        X2-X1<ScrX)
	{
		DWORD FileAttr;
		MoveToMouse(MouseEvent);
		GetSelNameCompat(nullptr,FileAttr);

		if (GetSelNameCompat(&strDragName,FileAttr) && !TestParentFolderName(strDragName))
		{
			SrcDragPanel=this;
			DragX=MouseEvent->dwMousePosition.X;
			DragY=MouseEvent->dwMousePosition.Y;
			DragMove=ShiftPressed;
		}
	}

	return FALSE;
}


int  Panel::IsDragging()
{
	return DragSaveScr!=nullptr;
}


void Panel::EndDrag()
{
	delete DragSaveScr;
	DragSaveScr=nullptr;
	DragX=DragY=-1;
}


void Panel::DragMessage(int X,int Y,int Move)
{
	FARString strDragMsg, strSelName;
	int SelCount,MsgX,Length;

	if (!(SelCount=SrcDragPanel->GetSelCount()))
		return;

	if (SelCount==1)
	{
		FARString strCvtName;
		DWORD FileAttr;
		SrcDragPanel->GetSelNameCompat(nullptr,FileAttr);
		SrcDragPanel->GetSelNameCompat(&strSelName,FileAttr);
		strCvtName = PointToName(strSelName);
		EscapeSpace(strCvtName);
		strSelName = strCvtName;
	}
	else
		strSelName.Format(MSG(MDragFiles), SelCount);

	if (Move)
		strDragMsg.Format(MSG(MDragMove), strSelName.CPtr());
	else
		strDragMsg.Format(MSG(MDragCopy), strSelName.CPtr());

	if ((Length=(int)strDragMsg.GetLength())+X>ScrX)
	{
		MsgX=ScrX-Length;

		if (MsgX<0)
		{
			MsgX=0;
			TruncStrFromEnd(strDragMsg,ScrX);
			Length=(int)strDragMsg.GetLength();
		}
	}
	else
		MsgX=X;

	ChangePriority ChPriority(ChangePriority::NORMAL);
	delete DragSaveScr;
	DragSaveScr=new SaveScreen(MsgX,Y,MsgX+Length-1,Y);
	GotoXY(MsgX,Y);
	SetColor(COL_PANELDRAGTEXT);
	Text(strDragMsg);
}



int Panel::GetCurDir(FARString &strCurDir)
{
	strCurDir = Panel::strCurDir; // TODO: ОПАСНО!!!
	return (int)strCurDir.GetLength();
}



BOOL Panel::SetCurDir(const wchar_t *CurDir,int ClosePlugin)
{
	if (StrCmpI(strCurDir,CurDir) || !TestCurrentDirectory(CurDir))
	{
		strCurDir = CurDir;

		if (PanelMode!=PLUGIN_PANEL)
			PrepareDiskPath(strCurDir);
	}

	return TRUE;
}


void Panel::InitCurDir(const wchar_t *CurDir)
{
	if (StrCmpI(strCurDir,CurDir) || !TestCurrentDirectory(CurDir))
	{
		strCurDir = CurDir;

		if (PanelMode!=PLUGIN_PANEL)
			PrepareDiskPath(strCurDir);
	}
}


/* $ 14.06.2001 KM
   + Добавлена установка переменных окружения, определяющих
     текущие директории дисков как для активной, так и для
     пассивной панели. Это необходимо программам запускаемым
     из FAR.
*/
/* $ 05.10.2001 SVS
   ! Давайте для начала выставим нужные значения для пассивной панели,
     а уж потом...
     А то фигня какая-то получается...
*/
/* $ 14.01.2002 IS
   ! Убрал установку переменных окружения, потому что она производится
     в FarChDir, которая теперь используется у нас для установления
     текущего каталога.
*/
int Panel::SetCurPath()
{
	if (GetMode()==PLUGIN_PANEL)
		return TRUE;

	Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->GetType()!=PLUGIN_PANEL)
	{
		if (IsAlpha(AnotherPanel->strCurDir.At(0)) && AnotherPanel->strCurDir.At(1)==L':' &&
		        Upper(AnotherPanel->strCurDir.At(0))!=Upper(strCurDir.At(0)))
		{
			// сначала установим переменные окружения для пассивной панели
			// (без реальной смены пути, чтобы лишний раз пассивный каталог
			// не перечитывать)
			FarChDir(AnotherPanel->strCurDir,FALSE);
		}
	}

	if (!FarChDir(strCurDir))
	{
		// здесь на выбор :-)
#if 1

		while (!FarChDir(strCurDir))
		{
			FARString strRoot;
			GetPathRoot(strCurDir, strRoot);

			if (FAR_GetDriveType(strRoot) != DRIVE_REMOVABLE || apiIsDiskInDrive(strRoot))
			{
				int Result=TestFolder(strCurDir);

				if (Result == TSTFLD_NOTFOUND)
				{
					if (CheckShortcutFolder(&strCurDir,FALSE,TRUE) && FarChDir(strCurDir))
					{
						SetCurDir(strCurDir,TRUE);
						return TRUE;
					}
				}
				else
					break;
			}

			if (FrameManager && FrameManager->ManagerStarted()) // сначала проверим - а запущен ли менеджер
			{
				SetCurDir(g_strFarPath,TRUE);                    // если запущен - выставим путь который мы точно знаем что существует
				ChangeDisk();                                    // и вызовем меню выбора дисков
			}
			else                                               // оппа...
			{
				FARString strTemp=strCurDir;
				CutToFolderNameIfFolder(strCurDir);             // подымаемся вверх, для очередной порции ChDir

				if (strTemp.GetLength()==strCurDir.GetLength())  // здесь проблема - видимо диск недоступен
				{
					SetCurDir(g_strFarPath,TRUE);                 // тогда просто сваливаем в каталог, откуда стартанул FAR.
					break;
				}
				else
				{
					if (FarChDir(strCurDir))
					{
						SetCurDir(strCurDir,TRUE);
						break;
					}
				}
			}
		}

#else

		do
		{
			BOOL IsChangeDisk=FALSE;
			char Root[1024];
			GetPathRoot(CurDir,Root);

			if (FAR_GetDriveType(Root) == DRIVE_REMOVABLE && !apiIsDiskInDrive(Root))
				IsChangeDisk=TRUE;
			else if (TestFolder(CurDir) == TSTFLD_NOTACCESS)
			{
				if (FarChDir(Root))
					SetCurDir(Root,TRUE);
				else
					IsChangeDisk=TRUE;
			}

			if (IsChangeDisk)
				ChangeDisk();
		}
		while (!FarChDir(CurDir));

#endif
		return FALSE;
	}

	return TRUE;
}


void Panel::Hide()
{
	ScreenObject::Hide();
	Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->IsVisible())
	{
		if (AnotherPanel->GetFocus())
			if ((AnotherPanel->GetType()==FILE_PANEL && AnotherPanel->IsFullScreen()) ||
			        (GetType()==FILE_PANEL && IsFullScreen()))
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
//  SavePrevScreen();
	Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->IsVisible() && !GetModalMode())
	{
		if (SaveScr)
		{
			SaveScr->AppendArea(AnotherPanel->SaveScr);
		}

		if (AnotherPanel->GetFocus())
		{
			if (AnotherPanel->IsFullScreen())
			{
				SetVisible(TRUE);
				return;
			}

			if (GetType()==FILE_PANEL && IsFullScreen())
			{
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
	if (Y<Y2)
	{
		SetColor(COL_PANELBOX);
		GotoXY(X1,Y);
		ShowSeparator(X2-X1+1,1);
	}
}


void Panel::ShowScreensCount()
{
	if (Opt.ShowScreensNumber && !X1)
	{
		int Viewers=FrameManager->GetFrameCountByType(MODALTYPE_VIEWER);
		int Editors=FrameManager->GetFrameCountByType(MODALTYPE_EDITOR);
		int Dialogs=FrameManager->GetFrameCountByType(MODALTYPE_DIALOG);

		if (Viewers>0 || Editors>0 || Dialogs > 0)
		{
			FARString strScreensText;
			FARString strAdd;
			strScreensText.Format(L"[%d", Viewers);

			if (Editors > 0)
			{
				strAdd.Format(L"+%d", Editors);
				strScreensText += strAdd;
			}

			if (Dialogs > 0)
			{
				strAdd.Format(L",%d", Dialogs);
				strScreensText += strAdd;
			}

			strScreensText += L"]";
			GotoXY(Opt.ShowColumnTitles ? X1:X1+2,Y1);
			SetColor(COL_PANELSCREENSNUMBER);
			Text(strScreensText);
		}
	}
}


void Panel::SetTitle()
{
	if (GetFocus())
	{
		FARString strTitleDir(L"{");

		if (!strCurDir.IsEmpty())
		{
			strTitleDir += strCurDir;
		}
		else
		{
			FARString strCmdText;
			CtrlObject->CmdLine->GetCurDir(strCmdText);
			strTitleDir += strCmdText;
		}

		strTitleDir += L"}";

		ConsoleTitle::SetFarTitle(strTitleDir);
	}
}

FARString &Panel::GetTitle(FARString &strTitle,int SubLen,int TruncSize)
{
	FARString strTitleDir;
	bool truncTitle = (SubLen==-1 || TruncSize==0)?false:true;

	if (PanelMode==PLUGIN_PANEL)
	{
		OpenPluginInfo Info;
		GetOpenPluginInfo(&Info);
		strTitleDir = Info.PanelTitle;
		RemoveExternalSpaces(strTitleDir);
		if (truncTitle)
			TruncStr(strTitleDir,SubLen-TruncSize);
	}
	else
	{
		strTitleDir = strCurDir;

		if (truncTitle)
			TruncPathStr(strTitleDir,SubLen-TruncSize);
	}

	strTitle = L" "+strTitleDir+L" ";
	return strTitle;
}

int Panel::SetPluginCommand(int Command,int Param1,LONG_PTR Param2)
{
	_ALGO(CleverSysLog clv(L"Panel::SetPluginCommand"));
	_ALGO(SysLog(L"(Command=%ls, Param1=[%d/0x%08X], Param2=[%d/0x%08X])",_FCTL_ToName(Command),(int)Param1,Param1,(int)Param2,Param2));
	int Result=FALSE;
	ProcessingPluginCommand++;
	FilePanels *FPanels=CtrlObject->Cp();
	PluginCommand=Command;

	switch (Command)
	{
		case FCTL_SETVIEWMODE:
			Result=FPanels->ChangePanelViewMode(this,Param1,FPanels->IsTopFrame());
			break;

		case FCTL_SETSORTMODE:
		{
			int Mode=Param1;

			if ((Mode>SM_DEFAULT) && (Mode<=SM_CHTIME))
			{
				SetSortMode(--Mode); // Уменьшим на 1 из-за SM_DEFAULT
				Result=TRUE;
			}
			break;
		}

		case FCTL_SETNUMERICSORT:
		{
			ChangeNumericSort(Param1);
			Result=TRUE;
			break;
		}

		case FCTL_SETCASESENSITIVESORT:
		{
			ChangeCaseSensitiveSort(Param1);
			Result=TRUE;
			break;
		}

		case FCTL_SETSORTORDER:
		{
			ChangeSortOrder(Param1?-1:1);
			Result=TRUE;
			break;
		}

		case FCTL_SETDIRECTORIESFIRST:
		{
			ChangeDirectoriesFirst(Param1);
			Result=TRUE;
			break;
		}

		case FCTL_CLOSEPLUGIN:
			strPluginParam = (const wchar_t *)Param2;
			Result=TRUE;
			//if(Opt.CPAJHefuayor)
			//  CtrlObject->Plugins.ProcessCommandLine((char *)PluginParam);
			break;

		case FCTL_GETPANELINFO:
		{
			if (!Param2)
				break;

			PanelInfo *Info=(PanelInfo *)Param2;
			memset(Info,0,sizeof(*Info));
			UpdateIfRequired();

			switch (GetType())
			{
				case FILE_PANEL:
					Info->PanelType=PTYPE_FILEPANEL;
					break;
				case TREE_PANEL:
					Info->PanelType=PTYPE_TREEPANEL;
					break;
				case QVIEW_PANEL:
					Info->PanelType=PTYPE_QVIEWPANEL;
					break;
				case INFO_PANEL:
					Info->PanelType=PTYPE_INFOPANEL;
					break;
			}

			Info->Plugin=(GetMode()==PLUGIN_PANEL);
			int X1,Y1,X2,Y2;
			GetPosition(X1,Y1,X2,Y2);
			Info->PanelRect.left=X1;
			Info->PanelRect.top=Y1;
			Info->PanelRect.right=X2;
			Info->PanelRect.bottom=Y2;
			Info->Visible=IsVisible();
			Info->Focus=GetFocus();
			Info->ViewMode=GetViewMode();
			Info->SortMode=SM_UNSORTED-UNSORTED+GetSortMode();
			{
				static struct
				{
					int *Opt;
					DWORD Flags;
				} PFLAGS[]=
				{
					{&Opt.ShowHidden,PFLAGS_SHOWHIDDEN},
					{&Opt.Highlight,PFLAGS_HIGHLIGHT},
				};
				DWORD Flags=0;

				for (size_t I=0; I < ARRAYSIZE(PFLAGS); ++I)
					if (*(PFLAGS[I].Opt) )
						Flags|=PFLAGS[I].Flags;

				Flags|=GetSortOrder()<0?PFLAGS_REVERSESORTORDER:0;
				Flags|=GetSortGroups()?PFLAGS_USESORTGROUPS:0;
				Flags|=GetSelectedFirstMode()?PFLAGS_SELECTEDFIRST:0;
				Flags|=GetDirectoriesFirst()?PFLAGS_DIRECTORIESFIRST:0;
				Flags|=GetNumericSort()?PFLAGS_NUMERICSORT:0;
				Flags|=GetCaseSensitiveSort()?PFLAGS_CASESENSITIVESORT:0;

				if (CtrlObject->Cp()->LeftPanel == this)
					Flags|=PFLAGS_PANELLEFT;

				Info->Flags=Flags;
			}

			if (GetType()==FILE_PANEL)
			{
				FileList *DestFilePanel=(FileList *)this;
				static int Reenter=0;

				if (!Reenter && Info->Plugin)
				{
					Reenter++;
					OpenPluginInfo PInfo;
					DestFilePanel->GetOpenPluginInfo(&PInfo);

					if (PInfo.Flags & OPIF_REALNAMES)
						Info->Flags |= PFLAGS_REALNAMES;

					if (!(PInfo.Flags & OPIF_USEHIGHLIGHTING))
						Info->Flags &= ~PFLAGS_HIGHLIGHT;

					if (PInfo.Flags & OPIF_USECRC32)
						Info->Flags |= PFLAGS_USECRC32;

					Reenter--;
				}

				DestFilePanel->PluginGetPanelInfo(*Info);
			}

			if (!Info->Plugin) // $ 12.12.2001 DJ - на неплагиновой панели - всегда реальные имена
				Info->Flags |= PFLAGS_REALNAMES;

			Result=TRUE;
			break;
		}

		case FCTL_GETPANELHOSTFILE:
		case FCTL_GETPANELFORMAT:
		case FCTL_GETPANELDIR:
		{
			FARString strTemp;

			if (Command == FCTL_GETPANELDIR)
				GetCurDir(strTemp);

			if (GetType()==FILE_PANEL)
			{
				FileList *DestFilePanel=(FileList *)this;
				static int Reenter=0;

				if (!Reenter && GetMode()==PLUGIN_PANEL)
				{
					Reenter++;

					OpenPluginInfo PInfo;
					DestFilePanel->GetOpenPluginInfo(&PInfo);

					switch (Command)
					{
						case FCTL_GETPANELHOSTFILE:
							strTemp=PInfo.HostFile;
							break;
						case FCTL_GETPANELFORMAT:
							strTemp=PInfo.Format;
							break;
						case FCTL_GETPANELDIR:
							strTemp=PInfo.CurDir;
							break;
					}

					Reenter--;
				}
			}

			if (Param1&&Param2)
				xwcsncpy((wchar_t*)Param2,strTemp,Param1);

			Result=(int)strTemp.GetLength()+1;
			break;
		}

		case FCTL_GETCOLUMNTYPES:
		case FCTL_GETCOLUMNWIDTHS:

			if (GetType()==FILE_PANEL)
			{
				FARString strColumnTypes,strColumnWidths;
				((FileList *)this)->PluginGetColumnTypesAndWidths(strColumnTypes,strColumnWidths);

				if (Command==FCTL_GETCOLUMNTYPES)
				{
					if (Param1&&Param2)
						xwcsncpy((wchar_t*)Param2,strColumnTypes,Param1);

					Result=(int)strColumnTypes.GetLength()+1;
				}
				else
				{
					if (Param1&&Param2)
						xwcsncpy((wchar_t*)Param2,strColumnWidths,Param1);

					Result=(int)strColumnWidths.GetLength()+1;
				}
			}
			break;

		case FCTL_GETPANELITEM:
		{
			Result=(int)((FileList*)this)->PluginGetPanelItem(Param1,(PluginPanelItem*)Param2);
			break;
		}

		case FCTL_GETSELECTEDPANELITEM:
		{
			Result=(int)((FileList*)this)->PluginGetSelectedPanelItem(Param1,(PluginPanelItem*)Param2);
			break;
		}

		case FCTL_GETCURRENTPANELITEM:
		{
			PanelInfo Info;
			FileList *DestPanel = ((FileList*)this);
			DestPanel->PluginGetPanelInfo(Info);
			Result = (int)DestPanel->PluginGetPanelItem(Info.CurrentItem,(PluginPanelItem*)Param2);
			break;
		}

		case FCTL_BEGINSELECTION:
		{
			if (GetType()==FILE_PANEL)
			{
				((FileList *)this)->PluginBeginSelection();
				Result=TRUE;
			}
			break;
		}

		case FCTL_SETSELECTION:
		{
			if (GetType()==FILE_PANEL)
			{
				((FileList *)this)->PluginSetSelection(Param1,Param2?true:false);
				Result=TRUE;
			}
			break;
		}

		case FCTL_CLEARSELECTION:
		{
			if (GetType()==FILE_PANEL)
			{
				reinterpret_cast<FileList*>(this)->PluginClearSelection(Param1);
				Result=TRUE;
			}
			break;
		}

		case FCTL_ENDSELECTION:
		{
			if (GetType()==FILE_PANEL)
			{
				((FileList *)this)->PluginEndSelection();
				Result=TRUE;
			}
			break;
		}

		case FCTL_UPDATEPANEL:
			Update(Param1?UPDATE_KEEP_SELECTION:0);

			if (GetType() == QVIEW_PANEL)
				UpdateViewPanel();

			Result=TRUE;
			break;

		case FCTL_REDRAWPANEL:
		{
			PanelRedrawInfo *Info=(PanelRedrawInfo *)Param2;

			if (Info)
			{
				CurFile=Info->CurrentItem;
				CurTopFile=Info->TopPanelItem;
			}

			// $ 12.05.2001 DJ перерисовываемся только в том случае, если мы - текущий фрейм
			if (FPanels->IsTopFrame())
				Redraw();

			Result=TRUE;
			break;
		}

		case FCTL_SETPANELDIR:
		{
			if (Param2)
			{
				Result = SetCurDir((const wchar_t *)Param2,TRUE);
				// restore current directory to active panel path
				Panel* ActivePanel = CtrlObject->Cp()->ActivePanel;
				if (Result && this != ActivePanel)
				{
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
	if ((!Opt.AutoUpdateLimit || static_cast<DWORD>(GetFileCount()) <= Opt.AutoUpdateLimit) &&
	        !StrCmpI(AnotherPanel->strCurDir,strCurDir))
		return TRUE;

	return FALSE;
}


bool Panel::SaveShortcutFolder(int Pos)
{
	FARString strShortcutFolder,strPluginModule,strPluginFile,strPluginData;

	if (PanelMode==PLUGIN_PANEL)
	{
		HANDLE hPlugin=GetPluginHandle();
		PluginHandle *ph = (PluginHandle*)hPlugin;
		strPluginModule = ph->pPlugin->GetModuleName();
		OpenPluginInfo Info;
		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
		strPluginFile = Info.HostFile;
		strShortcutFolder = Info.CurDir;
		strPluginData = Info.ShortcutData;
	}
	else
	{
		strPluginModule.Clear();
		strPluginFile.Clear();
		strPluginData.Clear();
		strShortcutFolder = strCurDir;
	}

	if (SaveFolderShortcut(Pos,&strShortcutFolder,&strPluginModule,&strPluginFile,&strPluginData))
		return true;

	return true;
}

/*
int Panel::ProcessShortcutFolder(int Key,BOOL ProcTreePanel)
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
	FARString strShortcutFolder,strPluginModule,strPluginFile,strPluginData;

	if (GetShortcutFolder(Pos,&strShortcutFolder,&strPluginModule,&strPluginFile,&strPluginData))
	{
		Panel *SrcPanel=this;
		Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);

		switch (GetType())
		{
			case TREE_PANEL:
				if (AnotherPanel->GetType()==FILE_PANEL)
					SrcPanel=AnotherPanel;
				break;

			case QVIEW_PANEL:
			case INFO_PANEL:
			{
				if (AnotherPanel->GetType()==FILE_PANEL)
					SrcPanel=AnotherPanel;
				break;
			}
		}

		int CheckFullScreen=SrcPanel->IsFullScreen();

		if (!strPluginModule.IsEmpty())
		{
			if (!strPluginFile.IsEmpty())
			{
				switch (CheckShortcutFolder(&strPluginFile,TRUE))
				{
					case 0:
						//              return FALSE;
					case -1:
						return true;
				}

				/* Своеобразное решение BugZ#50 */
				FARString strRealDir;
				strRealDir = strPluginFile;

				if (CutToSlash(strRealDir))
				{
					SrcPanel->SetCurDir(strRealDir,TRUE);
					SrcPanel->GoToFile(PointToName(strPluginFile));

					SrcPanel->ClearAllItem();
				}

				if (SrcPanel->GetType() == FILE_PANEL)
					((FileList*)SrcPanel)->OpenFilePlugin(strPluginFile,FALSE, OFP_SHORTCUT); //???

				if (!strShortcutFolder.IsEmpty())
						SrcPanel->SetCurDir(strShortcutFolder,FALSE);

				SrcPanel->Show();
			}
			else
			{
				switch (CheckShortcutFolder(nullptr,TRUE))
				{
					case 0:
						//              return FALSE;
					case -1:
						return true;
				}

				for (int I=0; I<CtrlObject->Plugins.GetPluginsCount(); I++)
				{
					Plugin *pPlugin = CtrlObject->Plugins.GetPlugin(I);

					if (!StrCmpI(pPlugin->GetModuleName(),strPluginModule))
					{
						if (pPlugin->HasOpenPlugin())
						{
							HANDLE hNewPlugin=CtrlObject->Plugins.OpenPlugin(pPlugin,OPEN_SHORTCUT,(INT_PTR)strPluginData.CPtr());

							if (hNewPlugin!=INVALID_HANDLE_VALUE)
							{
								int CurFocus=SrcPanel->GetFocus();

								Panel *NewPanel=CtrlObject->Cp()->ChangePanel(SrcPanel,FILE_PANEL,TRUE,TRUE);
								NewPanel->SetPluginMode(hNewPlugin,L"",CurFocus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible());

								if (!strShortcutFolder.IsEmpty())
									CtrlObject->Plugins.SetDirectory(hNewPlugin,strShortcutFolder,0);

								NewPanel->Update(0);
								NewPanel->Show();
							}
						}

						break;
					}
				}

				/*
				if(I == CtrlObject->Plugins.PluginsCount)
				{
				  char Target[NM*2];
				  xstrncpy(Target, PluginModule, sizeof(Target));
				  TruncPathStr(Target, ScrX-16);
				  Message (MSG_WARNING | MSG_ERRORTYPE, 1, MSG(MError), Target, MSG (MNeedNearPath), MSG(MOk))
				}
				*/
			}

			return true;
		}

		switch (CheckShortcutFolder(&strShortcutFolder,FALSE))
		{
			case 0:
				//          return FALSE;
			case -1:
				return true;
		}

        /*
		if (SrcPanel->GetType()!=FILE_PANEL)
		{
			SrcPanel=CtrlObject->Cp()->ChangePanel(SrcPanel,FILE_PANEL,TRUE,TRUE);
		}
        */

		SrcPanel->SetCurDir(strShortcutFolder,TRUE);

		if (CheckFullScreen!=SrcPanel->IsFullScreen())
			CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->Show();

		SrcPanel->Redraw();
		return true;
	}
	return false;
}

