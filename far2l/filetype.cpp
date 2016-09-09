/*
filetype.cpp

Работа с ассоциациями файлов
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

#include "filetype.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "dialog.hpp"
#include "vmenu.hpp"
#include "plognmn.hpp"
#include "ctrlobj.hpp"
#include "cmdline.hpp"
#include "history.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "CFileMask.hpp"
#include "registry.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "execute.hpp"
#include "fnparce.hpp"
#include "strmix.hpp"

struct FileTypeStrings
{
	const wchar_t *Help,*HelpModify,*State,
	*Associations,*TypeFmt, *Type0,
	*Execute, *Desc, *Mask, *View, *Edit,
	*AltExec, *AltView, *AltEdit;
};


const FileTypeStrings FTS=
{
	L"FileAssoc",L"FileAssocModify",L"State",
	L"Associations",L"Associations/Type%d",L"Associations/Type",
	L"Execute",L"Description",L"Mask",L"View",L"Edit",
	L"AltExec",L"AltView",L"AltEdit"
};



int GetDescriptionWidth(const wchar_t *Name=nullptr)
{
	int Width=0;
	RenumKeyRecord(FTS.Associations,FTS.TypeFmt,FTS.Type0);

	for (int NumLine=0;; NumLine++)
	{
		FARString strRegKey;
		strRegKey.Format(FTS.TypeFmt, NumLine);
		FARString strMask;

		if (!GetRegKey(strRegKey,FTS.Mask, strMask, L""))
			break;

		CFileMask FMask;

		if (!FMask.Set(strMask, FMF_SILENT))
			continue;

		FARString strDescription;
		GetRegKey(strRegKey,FTS.Desc,strDescription,L"");
		int CurWidth;

		if (!Name)
			CurWidth = HiStrlen(strDescription);
		else
		{
			if (!FMask.Compare(Name))
				continue;

			FARString strExpandedDesc = strDescription;
			SubstFileName(strExpandedDesc,Name,nullptr,nullptr,TRUE);
			CurWidth = HiStrlen(strExpandedDesc);
		}

		if (CurWidth>Width)
			Width=CurWidth;
	}

	if (Width>ScrX/2)
		Width=ScrX/2;

	return(Width);
}

/* $ 14.01.2001 SVS
   Добавим интелектуальности.
   Если встречается "IF" и оно выполняется, то команда
   помещается в список

   Вызывается для F3, F4 - ассоциации
   Enter в ком строке - ассоциации.
*/
/* $ 06.07.2001
   + Используем CFileMask вместо GetCommaWord, этим самым добиваемся того, что
     можно использовать маски исключения
   - Убрал непонятный мне запрет на использование маски файлов типа "*.*"
     (был когда-то, вроде, такой баг-репорт)
*/
bool ProcessLocalFileTypes(const wchar_t *Name, int Mode, bool AlwaysWaitFinish)
{
	RenumKeyRecord(FTS.Associations,FTS.TypeFmt,FTS.Type0);
	MenuItemEx TypesMenuItem;
	VMenu TypesMenu(MSG(MSelectAssocTitle),nullptr,0,ScrY-4);
	TypesMenu.SetHelp(FTS.Help);
	TypesMenu.SetFlags(VMENU_WRAPMODE);
	TypesMenu.SetPosition(-1,-1,0,0);
	int DizWidth=GetDescriptionWidth(Name);
	int ActualCmdCount=0; // отображаемых ассоциаций в меню
	CFileMask FMask; // для работы с масками файлов
	FARString strCommand, strDescription;
	int CommandCount=0;

	for (int I=0;; I++)
	{
		strCommand.Clear();
		FARString strRegKey, strMask;
		strRegKey.Format(FTS.TypeFmt,I);

		if (!GetRegKey(strRegKey,FTS.Mask,strMask,L""))
			break;

		if (FMask.Set(strMask,FMF_SILENT))
		{
			if (FMask.Compare(Name))
			{
				LPCWSTR Type=nullptr;

				switch (Mode)
				{
					case FILETYPE_EXEC:
						Type=FTS.Execute;
						break;
					case FILETYPE_VIEW:
						Type=FTS.View;
						break;
					case FILETYPE_EDIT:
						Type=FTS.Edit;
						break;
					case FILETYPE_ALTEXEC:
						Type=FTS.AltExec;
						break;
					case FILETYPE_ALTVIEW:
						Type=FTS.AltView;
						break;
					case FILETYPE_ALTEDIT:
						Type=FTS.AltEdit;
						break;
				}

				DWORD State=GetRegKey(strRegKey,FTS.State,0xffffffff);

				if (State&(1<<Mode))
				{
					FARString strNewCommand;
					GetRegKey(strRegKey,Type,strNewCommand,L"");

					if (!strNewCommand.IsEmpty())
					{
						strCommand = strNewCommand;
						GetRegKey(strRegKey,FTS.Desc,strDescription,L"");
						CommandCount++;
					}
				}
			}

			if (strCommand.IsEmpty())
				continue;
		}

		TypesMenuItem.Clear();
		FARString strCommandText = strCommand;
		SubstFileName(strCommandText,Name,nullptr,nullptr,TRUE);

		ActualCmdCount++;
		FARString strMenuText;

		if (DizWidth)
		{
			FARString strTitle;

			if (!strDescription.IsEmpty())
			{
				strTitle = strDescription;
				SubstFileName(strTitle, Name,nullptr,nullptr,TRUE);
			}

			size_t Pos=0;
			bool Ampersand=strTitle.Pos(Pos,L'&');

			if (DizWidth+Ampersand>ScrX/2 && Ampersand && static_cast<int>(Pos)>DizWidth)
				Ampersand=false;

			strMenuText.Format(L"%-*.*ls %lc ",DizWidth+Ampersand,DizWidth+Ampersand,strTitle.CPtr(),BoxSymbols[BS_V1]);
		}

		TruncStr(strCommandText,ScrX-DizWidth-14);
		strMenuText += strCommandText;
		TypesMenuItem.strName = strMenuText;
		TypesMenuItem.SetSelect(!I);
		TypesMenu.SetUserData(strCommand.CPtr(),0,TypesMenu.AddItem(&TypesMenuItem));
	}

	if (!CommandCount)
		return false;

	if (!ActualCmdCount)
		return true;

	int ExitCode=0;

	if (ActualCmdCount>1)
	{
		TypesMenu.Process();
		ExitCode=TypesMenu.Modal::GetExitCode();

		if (ExitCode<0)
			return true;
	}

	int Size=TypesMenu.GetUserDataSize(ExitCode);
	LPWSTR Command=strCommand.GetBuffer(Size/sizeof(wchar_t));
	TypesMenu.GetUserData(Command,Size,ExitCode);
	strCommand.ReleaseBuffer(Size);
	FARString strListName, strAnotherListName;
	int PreserveLFN=SubstFileName(strCommand,Name,&strListName,&strAnotherListName);
	bool ListFileUsed=!strListName.IsEmpty()||!strAnotherListName.IsEmpty();

	{
		//PreserveLongName PreserveName(PreserveLFN);
		RemoveExternalSpaces(strCommand);

		if (!strCommand.IsEmpty())
		{
			bool isSilent=(strCommand.At(0)==L'@');

			if (isSilent)
			{
				strCommand.LShift(1);
			}

			ProcessOSAliases(strCommand);

			if (!isSilent)
			{
				CtrlObject->CmdLine->ExecString(strCommand,AlwaysWaitFinish, false, false, ListFileUsed);

				if (!(Opt.ExcludeCmdHistory&EXCLUDECMDHISTORY_NOTFARASS) && !AlwaysWaitFinish) //AN
					CtrlObject->CmdHistory->AddToHistory(strCommand);
			}
			else
			{
#if 1
				SaveScreen SaveScr;
				CtrlObject->Cp()->LeftPanel->CloseFile();
				CtrlObject->Cp()->RightPanel->CloseFile();
				Execute(strCommand,AlwaysWaitFinish, 0, 0, 0, ListFileUsed, true);
#else
				// здесь была бага с прорисовкой (и... вывод данных
				// на команду "@type !@!" пропадал с экрана)
				// сделаем по аналогии с CommandLine::CmdExecute()
				{
					RedrawDesktop RdrwDesktop(TRUE);
					Execute(strCommand,AlwaysWaitFinish, 0, 0, 0, ListFileUsed);
					ScrollScreen(1); // обязательно, иначе деструктор RedrawDesktop
					// проредравив экран забьет последнюю строку вывода.
				}
				CtrlObject->Cp()->LeftPanel->UpdateIfChanged(UIC_UPDATE_FORCE);
				CtrlObject->Cp()->RightPanel->UpdateIfChanged(UIC_UPDATE_FORCE);
				CtrlObject->Cp()->Redraw();
#endif
			}
			if (FrameManager->GetCurrentFrame()->GetType()==MODALTYPE_VIEWER)
			{
				TypesMenu.ResetCursor();
			}
		}
	}

	if (!strListName.IsEmpty())
		apiDeleteFile(strListName);

	if (!strAnotherListName.IsEmpty())
		apiDeleteFile(strAnotherListName);

	return true;
}


void ProcessGlobalFileTypes(const wchar_t *Name, bool AlwaysWaitFinish, bool RunAs)
{
	FARString strName(Name);
	EscapeSpace(strName);
	CtrlObject->CmdLine->ExecString(strName, AlwaysWaitFinish, true, true, false, false, RunAs);

	if (!(Opt.ExcludeCmdHistory&EXCLUDECMDHISTORY_NOTWINASS) && !AlwaysWaitFinish)
	{
		CtrlObject->CmdHistory->AddToHistory(strName);
	}
}

/*
  Используется для запуска внешнего редактора и вьювера
*/
void ProcessExternal(const wchar_t *Command, const wchar_t *Name, bool AlwaysWaitFinish)
{
	FARString strListName, strAnotherListName;
	FARString strFullName;
	FARString strExecStr = Command;
	FARString strFullExecStr = Command;
	{
		int PreserveLFN=SubstFileName(strExecStr,Name,&strListName,&strAnotherListName);
		bool ListFileUsed=!strListName.IsEmpty()||!strAnotherListName.IsEmpty();

		//PreserveLongName PreserveName(PreserveLFN);
		ConvertNameToFull(Name,strFullName);
		//BUGBUGBUGBUGBUGBUG !!! Same ListNames!!!
		SubstFileName(strFullExecStr,strFullName,&strListName,&strAnotherListName);

		CtrlObject->ViewHistory->AddToHistory(strFullExecStr,(AlwaysWaitFinish&1)+2);

		if (strExecStr.At(0) != L'@')
			CtrlObject->CmdLine->ExecString(strExecStr,AlwaysWaitFinish, 0, 0, ListFileUsed, true);
		else
		{
			SaveScreen SaveScr;
			CtrlObject->Cp()->LeftPanel->CloseFile();
			CtrlObject->Cp()->RightPanel->CloseFile();
			Execute(strExecStr.CPtr()+1,AlwaysWaitFinish, 0, 0, 0, ListFileUsed);
		}
	}

	if (!strListName.IsEmpty())
		apiDeleteFile(strListName);

	if (!strAnotherListName.IsEmpty())
		apiDeleteFile(strAnotherListName);

}

static int FillFileTypesMenu(VMenu *TypesMenu,int MenuPos)
{
	int DizWidth=GetDescriptionWidth();
	MenuItemEx TypesMenuItem;
	TypesMenu->DeleteItems();
	int NumLine=0;

	for (;; NumLine++)
	{
		FARString strRegKey;
		strRegKey.Format(FTS.TypeFmt,NumLine);
		TypesMenuItem.Clear();
		FARString strMask;

		if (!GetRegKey(strRegKey,FTS.Mask,strMask,L""))
		{
			break;
		}

		FARString strMenuText;

		if (DizWidth)
		{
			FARString strDescription;
			GetRegKey(strRegKey,FTS.Desc,strDescription,L"");
			FARString strTitle=strDescription;
			size_t Pos=0;
			bool Ampersand=strTitle.Pos(Pos,L'&');

			if (DizWidth+Ampersand > ScrX/2 && Ampersand && static_cast<int>(Pos) > DizWidth)
				Ampersand=false;

			strMenuText.Format(L"%-*.*ls %lc ",DizWidth+Ampersand,DizWidth+Ampersand,strTitle.CPtr(),BoxSymbols[BS_V1]);
		}

		//TruncStr(strMask,ScrX-DizWidth-14);
		strMenuText += strMask;
		TypesMenuItem.strName = strMenuText;
		TypesMenuItem.SetSelect(NumLine==MenuPos);
		TypesMenu->AddItem(&TypesMenuItem);
	}

	TypesMenuItem.strName.Clear();
	TypesMenuItem.SetSelect(NumLine==MenuPos);
	TypesMenu->AddItem(&TypesMenuItem);
	return NumLine;
}

void MoveMenuItem(int Pos,int NewPos)
{
	FARString strSrc,strDst,strTmp;
	strSrc.Format(FTS.TypeFmt,Pos);
	strDst.Format(FTS.TypeFmt,NewPos);
	strTmp.Format(L"Associations/Tmp%u",WINPORT(GetTickCount)());
	CopyLocalKeyTree(strDst,strTmp);
	DeleteKeyTree(strDst);
	CopyLocalKeyTree(strSrc,strDst);
	DeleteKeyTree(strSrc);
	CopyLocalKeyTree(strTmp,strSrc);
	DeleteKeyTree(strTmp);
}

enum EDITTYPERECORD
{
	ETR_DOUBLEBOX,
	ETR_TEXT_MASKS,
	ETR_EDIT_MASKS,
	ETR_TEXT_DESCR,
	ETR_EDIT_DESCR,
	ETR_SEPARATOR1,
	ETR_COMBO_EXEC,
	ETR_EDIT_EXEC,
	ETR_COMBO_ALTEXEC,
	ETR_EDIT_ALTEXEC,
	ETR_COMBO_VIEW,
	ETR_EDIT_VIEW,
	ETR_COMBO_ALTVIEW,
	ETR_EDIT_ALTVIEW,
	ETR_COMBO_EDIT,
	ETR_EDIT_EDIT,
	ETR_COMBO_ALTEDIT,
	ETR_EDIT_ALTEDIT,
	ETR_SEPARATOR2,
	ETR_BUTTON_OK,
	ETR_BUTTON_CANCEL,
};

LONG_PTR WINAPI EditTypeRecordDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	switch (Msg)
	{
		case DN_BTNCLICK:

			switch (Param1)
			{
				case ETR_COMBO_EXEC:
				case ETR_COMBO_ALTEXEC:
				case ETR_COMBO_VIEW:
				case ETR_COMBO_ALTVIEW:
				case ETR_COMBO_EDIT:
				case ETR_COMBO_ALTEDIT:
					SendDlgMessage(hDlg,DM_ENABLE,Param1+1,Param2==BSTATE_CHECKED?TRUE:FALSE);
					break;
			}

			break;
		case DN_CLOSE:

			if (Param1==ETR_BUTTON_OK)
			{
				BOOL Result=TRUE;
				LPCWSTR Masks=reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,ETR_EDIT_MASKS,0));
				CFileMask FMask;

				if (!FMask.Set(Masks,0))
				{
					Result=FALSE;
				}

				return Result;
			}

			break;
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

bool EditTypeRecord(int EditPos,int TotalRecords,bool NewRec)
{
	bool Result=false;
	const int DlgX=76,DlgY=23;
	DialogDataEx EditDlgData[]=
	{
		DI_DOUBLEBOX,3, 1,DlgX-4,DlgY-2,0,0,MSG(MFileAssocTitle),
		DI_TEXT,     5, 2, 0, 2,0,0,MSG(MFileAssocMasks),
		DI_EDIT,     5, 3,DlgX-6, 3,(DWORD_PTR)L"Masks",DIF_FOCUS|DIF_HISTORY,L"",
		DI_TEXT,     5, 4, 0, 4,0,0,MSG(MFileAssocDescr),
		DI_EDIT,     5, 5,DlgX-6, 5,0,0,L"",
		DI_TEXT,     3, 6, 0, 6,0,DIF_SEPARATOR,L"",
		DI_CHECKBOX, 5, 7, 0, 7,1,0,MSG(MFileAssocExec),
		DI_EDIT,     9, 8,DlgX-6, 8,0,DIF_EDITPATH,L"",
		DI_CHECKBOX, 5, 9, 0, 9,1,0,MSG(MFileAssocAltExec),
		DI_EDIT,     9,10,DlgX-6,10,0,DIF_EDITPATH,L"",
		DI_CHECKBOX, 5,11, 0,11,1,0,MSG(MFileAssocView),
		DI_EDIT,     9,12,DlgX-6,12,0,DIF_EDITPATH,L"",
		DI_CHECKBOX, 5,13, 0,13,1,0,MSG(MFileAssocAltView),
		DI_EDIT,     9,14,DlgX-6,14,0,DIF_EDITPATH,L"",
		DI_CHECKBOX, 5,15, 0,15,1,0,MSG(MFileAssocEdit),
		DI_EDIT,     9,16,DlgX-6,16,0,DIF_EDITPATH,L"",
		DI_CHECKBOX, 5,17, 0,17,1,0,MSG(MFileAssocAltEdit),
		DI_EDIT,     9,18,DlgX-6,18,0,DIF_EDITPATH,L"",
		DI_TEXT,     3,DlgY-4, 0,DlgY-4,0,DIF_SEPARATOR,L"",
		DI_BUTTON,   0,DlgY-3, 0,DlgY-3,0,DIF_DEFAULT|DIF_CENTERGROUP,MSG(MOk),
		DI_BUTTON,   0,DlgY-3, 0,DlgY-3,0,DIF_CENTERGROUP,MSG(MCancel),
	};
	MakeDialogItemsEx(EditDlgData,EditDlg);
	FARString strRegKey;
	strRegKey.Format(FTS.TypeFmt,EditPos);

	if (!NewRec)
	{
		GetRegKey(strRegKey,FTS.Mask,EditDlg[ETR_EDIT_MASKS].strData,L"");
		GetRegKey(strRegKey,FTS.Desc,EditDlg[ETR_EDIT_DESCR].strData,L"");
		GetRegKey(strRegKey,FTS.Execute,EditDlg[ETR_EDIT_EXEC].strData,L"");
		GetRegKey(strRegKey,FTS.AltExec,EditDlg[ETR_EDIT_ALTEXEC].strData,L"");
		GetRegKey(strRegKey,FTS.View,EditDlg[ETR_EDIT_VIEW].strData,L"");
		GetRegKey(strRegKey,FTS.AltView,EditDlg[ETR_EDIT_ALTVIEW].strData,L"");
		GetRegKey(strRegKey,FTS.Edit,EditDlg[ETR_EDIT_EDIT].strData,L"");
		GetRegKey(strRegKey,FTS.AltEdit,EditDlg[ETR_EDIT_ALTEDIT].strData,L"");
		DWORD State=GetRegKey(strRegKey,FTS.State,0xffffffff);

		for (int i=FILETYPE_EXEC,Item=ETR_COMBO_EXEC; i<=FILETYPE_ALTEDIT; i++,Item+=2)
		{
			if (!(State&(1<<i)))
			{
				EditDlg[Item].Selected=BSTATE_UNCHECKED;
				EditDlg[Item+1].Flags|=DIF_DISABLE;
			}
		}
	}

	Dialog Dlg(EditDlg,ARRAYSIZE(EditDlg),EditTypeRecordDlgProc);
	Dlg.SetHelp(FTS.HelpModify);
	Dlg.SetPosition(-1,-1,DlgX,DlgY);
	Dlg.Process();

	if (Dlg.GetExitCode()==ETR_BUTTON_OK)
	{
		if (NewRec)
		{
			InsertKeyRecord(FTS.TypeFmt,EditPos,TotalRecords);
		}

		SetRegKey(strRegKey,FTS.Mask,EditDlg[ETR_EDIT_MASKS].strData);
		SetRegKey(strRegKey,FTS.Desc,EditDlg[ETR_EDIT_DESCR].strData);
		SetRegKey(strRegKey,FTS.Execute,EditDlg[ETR_EDIT_EXEC].strData);
		SetRegKey(strRegKey,FTS.AltExec,EditDlg[ETR_EDIT_ALTEXEC].strData);
		SetRegKey(strRegKey,FTS.View,EditDlg[ETR_EDIT_VIEW].strData);
		SetRegKey(strRegKey,FTS.AltView,EditDlg[ETR_EDIT_ALTVIEW].strData);
		SetRegKey(strRegKey,FTS.Edit,EditDlg[ETR_EDIT_EDIT].strData);
		SetRegKey(strRegKey,FTS.AltEdit,EditDlg[ETR_EDIT_ALTEDIT].strData);
		DWORD State=0;

		for (int i=FILETYPE_EXEC,Item=ETR_COMBO_EXEC; i<=FILETYPE_ALTEDIT; i++,Item+=2)
		{
			if (EditDlg[Item].Selected==BSTATE_CHECKED)
			{
				State|=(1<<i);
			}
		}

		SetRegKey(strRegKey,FTS.State,State);
		Result=true;
	}

	return Result;
}

bool DeleteTypeRecord(int DeletePos)
{
	bool Result=false;
	FARString strRecText, strRegKey;
	strRegKey.Format(FTS.TypeFmt,DeletePos);
	GetRegKey(strRegKey,FTS.Mask,strRecText,L"");
	FARString strItemName=strRecText;
	InsertQuote(strItemName);

	if (!Message(MSG_WARNING,2,MSG(MAssocTitle),MSG(MAskDelAssoc),strItemName,MSG(MDelete),MSG(MCancel)))
	{
		DeleteKeyRecord(FTS.TypeFmt,DeletePos);
		Result=true;
	}

	return Result;
}

void EditFileTypes()
{
	int NumLine=0;
	int MenuPos=0;
	RenumKeyRecord(FTS.Associations,FTS.TypeFmt,FTS.Type0);
	VMenu TypesMenu(MSG(MAssocTitle),nullptr,0,ScrY-4);
	TypesMenu.SetHelp(FTS.Help);
	TypesMenu.SetFlags(VMENU_WRAPMODE);
	TypesMenu.SetPosition(-1,-1,0,0);
	TypesMenu.SetBottomTitle(MSG(MAssocBottom));
	while (1)
	{
		bool MenuModified=true;

		while (!TypesMenu.Done())
		{
			if (MenuModified)
			{
				TypesMenu.Hide();
				NumLine=FillFileTypesMenu(&TypesMenu,MenuPos);
				TypesMenu.SetPosition(-1,-1,-1,-1);
				TypesMenu.Show();
				MenuModified=false;
			}

			DWORD Key=TypesMenu.ReadInput();
			MenuPos=TypesMenu.GetSelectPos();

			switch (Key)
			{
				case KEY_NUMDEL:
				case KEY_DEL:

					if (MenuPos<NumLine)
						DeleteTypeRecord(MenuPos);

					MenuModified=true;
					break;
				case KEY_NUMPAD0:
				case KEY_INS:
					EditTypeRecord(MenuPos,NumLine,true);
					MenuModified=true;
					break;
				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4:

					if (MenuPos<NumLine)
						EditTypeRecord(MenuPos,NumLine,false);

					MenuModified=true;
					break;
				case KEY_CTRLUP:
				case KEY_CTRLDOWN:
				{
					if (MenuPos!=TypesMenu.GetItemCount()-1)
					{
						if (!(Key==KEY_CTRLUP && !MenuPos) && !(Key==KEY_CTRLDOWN && MenuPos==TypesMenu.GetItemCount()-2))
						{
							int NewMenuPos=MenuPos+(Key==KEY_CTRLUP?-1:+1);
							MoveMenuItem(MenuPos,NewMenuPos);
							MenuPos=NewMenuPos;
							MenuModified=true;
						}
					}
				}
				break;
				default:
					TypesMenu.ProcessInput();
					break;
			}
		}

		int ExitCode=TypesMenu.Modal::GetExitCode();

		if (ExitCode!=-1)
		{
			MenuPos=ExitCode;
			TypesMenu.ClearDone();
			TypesMenu.WriteInput(KEY_F4);
			continue;
		}

		break;
	}
}
