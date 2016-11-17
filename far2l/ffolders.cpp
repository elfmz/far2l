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
#include "registry.hpp"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "dialog.hpp"
#include "FarDlgBuilder.hpp"
#include "plugin.hpp"
#include "plugins.hpp"

static int ShowFolderShortcutMenu(int Pos);
static const wchar_t HelpFolderShortcuts[]=L"FolderShortcuts";

enum PSCR_CMD
{
	PSCR_CMDGET,
	PSCR_CMDSET,
	PSCR_CMDDELALL,
};

enum PSCR_RECTYPE
{
	PSCR_RT_SHORTCUT,
	PSCR_RT_PLUGINMODULE,
	PSCR_RT_PLUGINFILE,
	PSCR_RT_PLUGINDATA,
};

static int ProcessShortcutRecord(int Command,int ValType,int RecNumber, FARString *pValue)
{
	static const wchar_t FolderShortcuts[]=L"FolderShortcuts";
	static const wchar_t *RecTypeName[]=
	{
		L"Shortcut%d",
		L"PluginModule%d",
		L"PluginFile%d",
		L"PluginData%d",
	};
	FARString strValueName;

	if (Command != PSCR_CMDDELALL)
		strValueName.Format(RecTypeName[ValType], RecNumber);

	switch(Command)
	{
		case PSCR_CMDGET:
			return GetRegKey(FolderShortcuts,strValueName,*pValue,L"");
		case PSCR_CMDSET:
			return SetRegKey(FolderShortcuts,strValueName,NullToEmpty(*pValue));
		case PSCR_CMDDELALL:
			for (size_t I=0; I < ARRAYSIZE(RecTypeName); ++I)
			{
				strValueName.Format(RecTypeName[I],RecNumber);
				SetRegKey(FolderShortcuts,strValueName,L"");
			}
			return TRUE;
	}

	return FALSE;
}

int GetShortcutFolder(int Pos,FARString *pDestFolder,
                      FARString *pPluginModule,
                      FARString *pPluginFile,
                      FARString *pPluginData)
{
	FARString strFolder;
	ProcessShortcutRecord(PSCR_CMDGET,PSCR_RT_SHORTCUT,Pos,&strFolder);
	apiExpandEnvironmentStrings(strFolder, *pDestFolder);

	if (pPluginModule)
		ProcessShortcutRecord(PSCR_CMDGET,PSCR_RT_PLUGINMODULE,Pos,pPluginModule);

	if (pPluginFile)
		ProcessShortcutRecord(PSCR_CMDGET,PSCR_RT_PLUGINFILE,Pos,pPluginFile);

	if (pPluginData)
		ProcessShortcutRecord(PSCR_CMDGET,PSCR_RT_PLUGINDATA,Pos,pPluginData);

	return (!pDestFolder->IsEmpty() || (pPluginModule && !pPluginModule->IsEmpty()));
}


int SaveFolderShortcut(int Pos,FARString *pSrcFolder,
                       FARString *pPluginModule,
                       FARString *pPluginFile,
                       FARString *pPluginData)
{
	ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_SHORTCUT,Pos,pSrcFolder);
	ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_PLUGINMODULE,Pos,pPluginModule);
	ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_PLUGINFILE,Pos,pPluginFile);
	ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_PLUGINDATA,Pos,pPluginData);
	return TRUE;
}


void ShowFolderShortcut()
{
	int Pos=0;

	while (Pos!=-1)
		Pos=ShowFolderShortcutMenu(Pos);
}


static int ShowFolderShortcutMenu(int Pos)
{
	int ExitCode=-1;
	{
		int I;
		MenuItemEx ListItem;
		VMenu FolderList(MSG(MFolderShortcutsTitle),nullptr,0,ScrY-4);
		FolderList.SetFlags(VMENU_WRAPMODE); // VMENU_SHOWAMPERSAND|
		FolderList.SetHelp(HelpFolderShortcuts);
		FolderList.SetPosition(-1,-1,0,0);
		FolderList.SetBottomTitle(MSG(MFolderShortcutBottom));

		for (I=0; I<10; I++)
		{
			FARString strFolderName;
			FARString strValueName;
			ListItem.Clear();
			ProcessShortcutRecord(PSCR_CMDGET,PSCR_RT_SHORTCUT,I,&strFolderName);
			//TruncStr(strFolderName,60);

			if (strFolderName.IsEmpty())
			{
				ProcessShortcutRecord(PSCR_CMDGET,PSCR_RT_PLUGINMODULE,I,&strFolderName);

				if (strFolderName.IsEmpty())
					strFolderName = MSG(MShortcutNone);
				else
					strFolderName = MSG(MShortcutPlugin);
			}

//wxWidgets doesn't distinguish right/left modifiers
//			ListItem.strName.Format(L"%ls+&%d   %ls", MSG(MRightCtrl),I,strFolderName.CPtr());
			ListItem.strName.Format(L"[%ls | Ctrl+Alt] + &%d   %ls", MSG(MRightCtrl), I,strFolderName.CPtr());
			ListItem.SetSelect(I == Pos);
			FolderList.AddItem(&ListItem);
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
				case KEY_NUMPAD0:
				case KEY_INS:
				{
					ProcessShortcutRecord(PSCR_CMDDELALL,0,SelPos,nullptr);

					if (Key == KEY_INS || Key == KEY_NUMPAD0)
					{
						Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
						FARString strNewDir;
						CtrlObject->CmdLine->GetCurDir(strNewDir);
						ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_SHORTCUT,SelPos,&strNewDir);

						if (ActivePanel->GetMode() == PLUGIN_PANEL)
						{
							OpenPluginInfo Info;
							ActivePanel->GetOpenPluginInfo(&Info);
							FARString strTemp;
							PluginHandle *ph = (PluginHandle*)ActivePanel->GetPluginHandle();
							strTemp = ph->pPlugin->GetModuleName();
							ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_PLUGINMODULE,SelPos,&strTemp);
							strTemp = Info.HostFile;
							ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_PLUGINFILE,SelPos,&strTemp);
							strTemp = Info.ShortcutData;
							ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_PLUGINDATA,SelPos,&strTemp);
						}
					}

					return(SelPos);
				}
				case KEY_F4:
				{
					FARString strNewDir;
					FARString strTemp;

					ProcessShortcutRecord(PSCR_CMDGET,PSCR_RT_SHORTCUT,SelPos,&strNewDir);
					strTemp = strNewDir;

					DialogBuilder Builder(MFolderShortcutsTitle, HelpFolderShortcuts);
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
							ProcessShortcutRecord(PSCR_CMDDELALL,0,SelPos,nullptr);
							ProcessShortcutRecord(PSCR_CMDSET,PSCR_RT_SHORTCUT,SelPos,&strNewDir);
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
