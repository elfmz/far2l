/*
ffolders.cpp

Folder shortcuts
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


#include "ffolders.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "vmenu.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "filelist.hpp"
#include "KeyFileHelper.h"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "dialog.hpp"
#include "FarDlgBuilder.hpp"
#include "plugin.hpp"
#include "plugins.hpp"

static const wchar_t FolderShortcuts[] = L"FolderShortcuts";


class Bookmarks
{
	KeyFileHelper _kfh;

public:
	Bookmarks()
		: _kfh(InMyConfig("bookmarks.ini").c_str(), true)
	{
	}

	void Set(int index, const FARString *path,
		const FARString *plugin = nullptr,
		const FARString *plugin_file = nullptr,
		const FARString *plugin_data = nullptr)
	{
		if ( (!path || path->IsEmpty()) && (!plugin || plugin->IsEmpty()))
		{
			Clear(index);
			return;
		}

		char sec[32]; sprintf(sec, "%u", index);
		_kfh.RemoveSection(sec);

		_kfh.PutString(sec, "Path", path ? path->GetMB().c_str() : "");
		_kfh.PutString(sec, "Plugin", plugin ? plugin->GetMB().c_str() : "");
		_kfh.PutString(sec, "PluginFile", plugin_file ? plugin_file->GetMB().c_str() : "");
		_kfh.PutString(sec, "PluginData", plugin_data ? plugin_data->GetMB().c_str() : "");
	}

	bool Get(int index, FARString *path,
		FARString *plugin = nullptr,
		FARString *plugin_file = nullptr,
		FARString *plugin_data = nullptr)
	{
		char sec[32]; sprintf(sec, "%u", index);
		FARString strFolder(_kfh.GetString(sec, "Path"));

		if (!strFolder.IsEmpty())
			apiExpandEnvironmentStrings(strFolder, *path);
		else
			path->Clear();

		if (plugin)
			*plugin = _kfh.GetString(sec, "Plugin");

		if (plugin_file)
			*plugin_file = _kfh.GetString(sec, "PluginFile");

		if (plugin_data)
			*plugin_data = _kfh.GetString(sec, "PluginData");

		return (!path->IsEmpty() || (plugin && !plugin->IsEmpty()));
	}

	void Clear(int index)
	{
		char sec[32]; sprintf(sec, "%u", index);
		_kfh.RemoveSection(sec);
		if (index < 10)
			return;

		for (int dst_index = index, miss_counter = 0;;)
		{
			FARString path, plugin, plugin_file, plugin_data;
			if (Get(index, &path, &plugin, &plugin_file, &plugin_data))
			{
				if (dst_index != index)
				{
					Set(dst_index, &path, &plugin, &plugin_file, &plugin_data);
				}
				++dst_index;
				miss_counter = 0;
			}
			else if (++miss_counter >= 10)
			{
				for (; dst_index <= index; ++dst_index)
				{
					 sprintf(sec, "%u", dst_index);
					_kfh.RemoveSection(sec);
				}
				break;
			}

			++index;
		}
	}
};

bool GetShortcutFolder(int Pos,
		FARString *pDestFolder,
		FARString *pPluginModule,
		FARString *pPluginFile,
		FARString *pPluginData)
{
	return Bookmarks().Get(Pos, pDestFolder, pPluginModule, pPluginFile, pPluginData);
}

bool SaveFolderShortcut(int Pos,
		FARString *pSrcFolder,
		FARString *pPluginModule,
		FARString *pPluginFile,
		FARString *pPluginData)
{
	Bookmarks().Set(Pos, pSrcFolder, pPluginModule, pPluginFile, pPluginData);
	return true;
}

bool ClearFolderShortcut(int Pos)
{
	Bookmarks().Clear(Pos);
	return true;
}


static int ShowFolderShortcutMenu(int Pos)
{
	int ExitCode=-1;
	Bookmarks b;
	{
		int I;
		MenuItemEx ListItem;
		VMenu FolderList(MSG(MFolderShortcutsTitle),nullptr,0,ScrY-4);
		FolderList.SetFlags(VMENU_WRAPMODE); // VMENU_SHOWAMPERSAND|
		FolderList.SetHelp(FolderShortcuts);
		FolderList.SetPosition(-1,-1,0,0);
		FolderList.SetBottomTitle(MSG(MFolderShortcutBottom));

		for (I=0; ; I++)
		{
			FARString strFolderName, strPlugin;
			FARString strValueName;
			ListItem.Clear();
			b.Get(I, &strFolderName, &strPlugin);
			//TruncStr(strFolderName,60);

			if (strFolderName.IsEmpty())
			{
				strFolderName = strPlugin.IsEmpty()
					? MSG(MShortcutNone) : MSG(MShortcutPlugin);
			}

//wxWidgets doesn't distinguish right/left modifiers
//			ListItem.strName.Format(L"%ls+&%d   %ls", MSG(MRightCtrl),I,strFolderName.CPtr());
			if (I < 10)
			{
				ListItem.strName.Format(L"[%ls | Ctrl+Alt] + &%d   %ls", MSG(MRightCtrl), I,strFolderName.CPtr());
			}
			else
			{
				ListItem.strName.Format(L"%ls", strFolderName.CPtr());
			}
			ListItem.SetSelect(I == Pos);
			FolderList.AddItem(&ListItem);

			if (I >= 10 && strFolderName == MSG(MShortcutNone))
			{
				break;
			}
		}

		FolderList.Show();

		while (!FolderList.Done())
		{
			DWORD Key=FolderList.ReadInput();
			int SelPos=FolderList.GetSelectPos();

			switch (Key)
			{
				case KEY_NUMDEL:
				case KEY_DEL:
					b.Clear(SelPos);
					return(SelPos);

				case KEY_NUMPAD0:
				case KEY_INS:
				{
					Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
					FARString strNewDir, strNewPluginModule, strNewPluginFile, strNewPluginData;
					CtrlObject->CmdLine->GetCurDir(strNewDir);

					if (ActivePanel->GetMode() == PLUGIN_PANEL)
					{
						OpenPluginInfo Info;
						ActivePanel->GetOpenPluginInfo(&Info);
						PluginHandle *ph = (PluginHandle*)ActivePanel->GetPluginHandle();
						strNewPluginModule = ph->pPlugin->GetModuleName();
						strNewPluginFile = Info.HostFile;
						strNewPluginData = Info.ShortcutData;
					}

					b.Set(SelPos, &strNewDir, &strNewPluginModule, &strNewPluginFile, &strNewPluginData);
					return(SelPos);
				}
				case KEY_F4:
				{
					FARString strNewDir;
					b.Get(SelPos, &strNewDir);
					FARString strTemp = strNewDir;

					DialogBuilder Builder(MFolderShortcutsTitle, FolderShortcuts);
					Builder.AddText(MFSShortcut);
					Builder.AddEditField(&strNewDir, 50, L"FS_Path", DIF_EDITPATH);
					//...
					Builder.AddOKCancel();

					if (Builder.ShowDialog())
					{
						Unquote(strNewDir);

						if (!IsLocalRootPath(strNewDir))
							DeleteEndSlash(strNewDir);

						BOOL Saved=TRUE;
						apiExpandEnvironmentStrings(strNewDir,strTemp);

						if (apiGetFileAttributes(strTemp) == INVALID_FILE_ATTRIBUTES)
						{
							WINPORT(SetLastError)(ERROR_PATH_NOT_FOUND);
							Saved=!Message(MSG_WARNING | MSG_ERRORTYPE, 2, MSG(MError), strNewDir, MSG(MSaveThisShortcut), MSG(MYes), MSG(MNo));
						}

						if (Saved)
						{
							b.Set(SelPos, &strNewDir);
							return(SelPos);
						}
					}

					break;
				}
				default:
					FolderList.ProcessInput();
					break;
			}
		}

		ExitCode=FolderList.Modal::GetExitCode();
		FolderList.Hide();
	}

	if (ExitCode>=0)
	{
		CtrlObject->Cp()->ActivePanel->ExecShortcutFolder(ExitCode);
	}

	return -1;
}

void ShowFolderShortcut(int Pos)
{
	while (Pos != -1)
	{
		Pos = ShowFolderShortcutMenu(Pos);
	}
}
