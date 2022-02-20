/*
delete.cpp

Удаление файлов
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


#include "lang.hpp"
#include "panel.hpp"
#include "chgprior.hpp"
#include "filepanels.hpp"
#include "scantree.hpp"
#include "treelist.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "filelist.hpp"
#include "manager.hpp"
#include "constitle.hpp"
#include "TPreRedrawFunc.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "mix.hpp"
#include "dirinfo.hpp"
#include "wakeful.hpp"
#include "execute.hpp"

#if defined(__APPLE__) || defined(__FreeBSD__)
  #include <errno.h>
#endif

static void ShellDeleteMsg(const wchar_t *Name,int Wipe,int Percent);
static int ShellRemoveFile(const wchar_t *Name,int Wipe);
static int ERemoveDirectory(const wchar_t *Name,int Wipe);
static int RemoveToRecycleBin(const wchar_t *Name);
static int WipeFile(const wchar_t *Name);
static int WipeDirectory(const wchar_t *Name);
static void PR_ShellDeleteMsg();

static int ReadOnlyDeleteMode,SkipMode,SkipWipeMode,SkipFoldersMode,DeleteAllFolders;
ULONG ProcessedItems;

ConsoleTitle *DeleteTitle=nullptr;

enum {DELETE_SUCCESS,DELETE_YES,DELETE_SKIP,DELETE_CANCEL};

struct AskDeleteReadOnly
{
	AskDeleteReadOnly(const wchar_t *Name,int Wipe);
	
	~AskDeleteReadOnly()
	{
		if (_ump)
			_ump->Unmake();
	}
	
	inline int Choice() const
	{
		return _r;
	}

	private:
	IUnmakeWritablePtr _ump;
	int _r;
};

void ShellDelete(Panel *SrcPanel,int Wipe)
{
	SudoClientRegion scr;
	//todo ChangePriority ChPriority(Opt.DelThreadPriority);
	TPreRedrawFuncGuard preRedrawFuncGuard(PR_ShellDeleteMsg);
	FAR_FIND_DATA_EX FindData;
	FARString strDeleteFilesMsg;
	FARString strSelName;
	FARString strDizName;
	FARString strFullName;
	DWORD FileAttr;
	int SelCount,UpdateDiz;
	int DizPresent;
	int Ret;
	BOOL NeedUpdate=TRUE, NeedSetUpADir=FALSE;
	int Opt_DeleteToRecycleBin=Opt.DeleteToRecycleBin;
	/*& 31.05.2001 OT Запретить перерисовку текущего фрейма*/
	Frame *FrameFromLaunched=FrameManager->GetCurrentFrame();
	FrameFromLaunched->Lock();
	DeleteAllFolders=!Opt.Confirm.DeleteFolder;
	UpdateDiz=(Opt.Diz.UpdateMode==DIZ_UPDATE_ALWAYS ||
	           (SrcPanel->IsDizDisplayed() &&
	            Opt.Diz.UpdateMode==DIZ_UPDATE_IF_DISPLAYED));

	if (!(SelCount=SrcPanel->GetSelCount()))
		goto done;

	// TODO: Удаление в корзину только для  FIXED-дисков
	{
//    char FSysNameSrc[NM];
		SrcPanel->GetSelNameCompat(nullptr,FileAttr);
		SrcPanel->GetSelNameCompat(&strSelName,FileAttr);
	}

	if (SelCount==1)
	{
		SrcPanel->GetSelNameCompat(nullptr,FileAttr);
		SrcPanel->GetSelNameCompat(&strSelName,FileAttr);

		if (TestParentFolderName(strSelName) || strSelName.IsEmpty())
		{
			NeedUpdate=FALSE;
			goto done;
		}

		strDeleteFilesMsg = strSelName;
	}
	else
	{
		// в зависимости от числа ставим нужное окончание
		const wchar_t *Ends;
		wchar_t StrItems[16];
		_itow(SelCount,StrItems,10);
		Ends=Msg::AskDeleteItemsA;
		int LenItems=StrLength(StrItems);

		if (LenItems > 0)
		{
			if ((LenItems >= 2 && StrItems[LenItems-2] == L'1') ||
			        StrItems[LenItems-1] >= L'5' ||
			        StrItems[LenItems-1] == L'0')
				Ends=Msg::AskDeleteItemsS;
			else if (StrItems[LenItems-1] == L'1')
				Ends=Msg::AskDeleteItems0;
		}

		strDeleteFilesMsg.Format(Msg::AskDeleteItems,SelCount,Ends);
	}

	Ret=1;
	
	if (Ret && (Opt.Confirm.Delete || SelCount>1))// || (FileAttr & FILE_ATTRIBUTE_DIRECTORY)))
	{
		const wchar_t *DelMsg;
		const wchar_t *TitleMsg = Wipe ? Msg::DeleteWipeTitle : Msg::DeleteTitle;
		/* $ 05.01.2001 IS
		   ! Косметика в сообщениях - разные сообщения в зависимости от того,
		     какие и сколько элементов выделено.
		*/
		BOOL folder=(FileAttr & FILE_ATTRIBUTE_DIRECTORY);

		if (SelCount==1)
		{
			if (Wipe && !(FileAttr & FILE_ATTRIBUTE_REPARSE_POINT))
				DelMsg = folder ? Msg::AskWipeFolder : Msg::AskWipeFile;
			else
			{
				if (Opt.DeleteToRecycleBin && !(FileAttr & FILE_ATTRIBUTE_REPARSE_POINT))
					DelMsg = folder ? Msg::AskDeleteRecycleFolder : Msg::AskDeleteRecycleFile;
				else
					DelMsg = folder ? Msg::AskDeleteFolder : Msg::AskDeleteFile;
			}
		}
		else
		{
			if (Wipe && !(FileAttr & FILE_ATTRIBUTE_REPARSE_POINT))
			{
				DelMsg=Msg::AskWipe;
				TitleMsg=Msg::DeleteWipeTitle;
			}
			else if (Opt.DeleteToRecycleBin && !(FileAttr & FILE_ATTRIBUTE_REPARSE_POINT))
				DelMsg=Msg::AskDeleteRecycle;
			else
				DelMsg=Msg::AskDelete;
		}

		SetMessageHelp(L"DeleteFile");

		if (Message(0,2,TitleMsg,DelMsg,strDeleteFilesMsg,(Wipe?Msg::DeleteWipe:Opt.DeleteToRecycleBin?Msg::DeleteRecycle:Msg::Delete),Msg::Cancel))
		{
			NeedUpdate=FALSE;
			goto done;
		}
	}

	if (Opt.Confirm.Delete && SelCount>1)
	{
		//SaveScreen SaveScr;
		SetCursorType(FALSE,0);
		SetMessageHelp(L"DeleteFile");

		if (Message(MSG_WARNING,2,(Wipe?Msg::WipeFilesTitle:Msg::DeleteFilesTitle),(Wipe?Msg::AskWipe:Msg::AskDelete),
		            strDeleteFilesMsg,Msg::DeleteFileAll,Msg::DeleteFileCancel))
		{
			NeedUpdate=FALSE;
			goto done;
		}
	}

	if (UpdateDiz)
		SrcPanel->ReadDiz();

	SrcPanel->GetDizName(strDizName);
	DizPresent=(!strDizName.IsEmpty() && apiGetFileAttributes(strDizName)!=INVALID_FILE_ATTRIBUTES);
	DeleteTitle = new ConsoleTitle(Msg::DeletingTitle);

	if ((NeedSetUpADir=CheckUpdateAnotherPanel(SrcPanel,strSelName)) == -1)
		goto done;

	if (SrcPanel->GetType()==TREE_PANEL)
		FarChDir(L"/");

	{
		wakeful W;
		bool Cancel=false;
		//SaveScreen SaveScr;
		SetCursorType(FALSE,0);
		ReadOnlyDeleteMode=-1;
		SkipMode=-1;
		SkipWipeMode=-1;
		SkipFoldersMode=-1;
		ULONG ItemsCount=0;
		ProcessedItems=0;

		if (Opt.DelOpt.DelShowTotal)
		{
			SrcPanel->GetSelNameCompat(nullptr,FileAttr);
			DWORD StartTime=WINPORT(GetTickCount)();
			bool FirstTime=true;

			while (SrcPanel->GetSelNameCompat(&strSelName,FileAttr) && !Cancel)
			{
				if (!(FileAttr&FILE_ATTRIBUTE_REPARSE_POINT))
				{
					if (FileAttr&FILE_ATTRIBUTE_DIRECTORY)
					{
						DWORD CurTime=WINPORT(GetTickCount)();

						if (CurTime-StartTime>RedrawTimeout || FirstTime)
						{
							StartTime=CurTime;
							FirstTime=false;

							if (CheckForEscSilent() && ConfirmAbortOp())
							{
								Cancel=true;
								break;
							}

							ShellDeleteMsg(strSelName,Wipe,-1);
						}

						uint32_t CurrentFileCount,CurrentDirCount,ClusterSize;
						UINT64 FileSize,PhysicalSize;

						if (GetDirInfo(nullptr,strSelName,CurrentDirCount,CurrentFileCount,FileSize,PhysicalSize,ClusterSize,-1,nullptr,0)>0)
						{
							ItemsCount+=CurrentFileCount+CurrentDirCount+1;
						}
						else
						{
							Cancel=true;
						}
					}
					else
					{
						ItemsCount++;
					}
				}
			}
		}

		SrcPanel->GetSelNameCompat(nullptr,FileAttr);
		DWORD StartTime=WINPORT(GetTickCount)();
		bool FirstTime=true;

		while (SrcPanel->GetSelNameCompat(&strSelName,FileAttr) && !Cancel)
		{
			int Length=(int)strSelName.GetLength();

			if (!Length) continue;

			DWORD CurTime=WINPORT(GetTickCount)();

			if (CurTime-StartTime>RedrawTimeout || FirstTime)
			{
				StartTime=CurTime;
				FirstTime=false;

				if (CheckForEscSilent() && ConfirmAbortOp())
				{
					Cancel=true;
					break;
				}

				ShellDeleteMsg(strSelName,Wipe,Opt.DelOpt.DelShowTotal?(ItemsCount?(ProcessedItems*100/ItemsCount):0):-1);
			}

			if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!DeleteAllFolders)
				{
					ConvertNameToFull(strSelName, strFullName);

					if (TestFolder(strFullName) == TSTFLD_NOTEMPTY)
					{
						int MsgCode=0;

						// для symlink`а не нужно подтверждение
						if (!(FileAttr & FILE_ATTRIBUTE_REPARSE_POINT))
							MsgCode=Message(MSG_WARNING,4,(Wipe?Msg::WipeFolderTitle:Msg::DeleteFolderTitle),
							                (Wipe?Msg::WipeFolderConfirm:Msg::DeleteFolderConfirm),strFullName,
							                (Wipe?Msg::DeleteFileWipe:Msg::DeleteFileDelete),Msg::DeleteFileAll,
							                Msg::DeleteFileSkip,Msg::DeleteFileCancel);

						if (MsgCode<0 || MsgCode==3)
						{
							NeedSetUpADir=FALSE;
							break;
						}

						if (MsgCode==1)
							DeleteAllFolders=1;

						if (MsgCode==2)
							continue;
					}
				}

				bool DirSymLink=(FileAttr&FILE_ATTRIBUTE_DIRECTORY && FileAttr&FILE_ATTRIBUTE_REPARSE_POINT);

				if (!DirSymLink && (!Opt.DeleteToRecycleBin || Wipe))
				{
					FARString strFullName;
					ScanTree ScTree(TRUE,TRUE,FALSE);
					FARString strSelFullName;

					if (IsAbsolutePath(strSelName))
					{
						strSelFullName=strSelName;
					}
					else
					{
						SrcPanel->GetCurDir(strSelFullName);
						AddEndSlash(strSelFullName);
						strSelFullName+=strSelName;
					}

					ScTree.SetFindPath(strSelFullName,L"*", 0);
					DWORD StartTime=WINPORT(GetTickCount)();

					while (ScTree.GetNextName(&FindData,strFullName))
					{
						DWORD CurTime=WINPORT(GetTickCount)();

						if (CurTime-StartTime>RedrawTimeout)
						{
							StartTime=CurTime;

							if (CheckForEscSilent())
							{
								int AbortOp = ConfirmAbortOp();

								if (AbortOp)
								{
									Cancel=true;
									break;
								}
							}

							ShellDeleteMsg(strFullName,Wipe,Opt.DelOpt.DelShowTotal?(ItemsCount?(ProcessedItems*100/ItemsCount):0):-1);
						}

						if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							if (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
							{
								if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
									apiMakeWritable(strFullName); //apiSetFileAttributes(strFullName,FILE_ATTRIBUTE_NORMAL);

								int MsgCode=ERemoveDirectory(strFullName,Wipe);

								if (MsgCode==DELETE_CANCEL)
								{
									Cancel=true;
									break;
								}
								else if (MsgCode==DELETE_SKIP)
								{
									ScTree.SkipDir();
									continue;
								}

								TreeList::DelTreeName(strFullName);

								if (UpdateDiz)
									SrcPanel->DeleteDiz(strFullName);

								continue;
							}

							if (!DeleteAllFolders && !ScTree.IsDirSearchDone() && TestFolder(strFullName) == TSTFLD_NOTEMPTY)
							{
								int MsgCode=Message(MSG_WARNING,4,(Wipe?Msg::WipeFolderTitle:Msg::DeleteFolderTitle),
								                    (Wipe?Msg::WipeFolderConfirm:Msg::DeleteFolderConfirm),strFullName,
								                    (Wipe?Msg::DeleteFileWipe:Msg::DeleteFileDelete),Msg::DeleteFileAll,
								                    Msg::DeleteFileSkip,Msg::DeleteFileCancel);

								if (MsgCode<0 || MsgCode==3)
								{
									Cancel=true;
									break;
								}

								if (MsgCode==1)
									DeleteAllFolders=1;

								if (MsgCode==2)
								{
									ScTree.SkipDir();
									continue;
								}
							}

							if (ScTree.IsDirSearchDone())
							{
								if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
									apiMakeWritable(strFullName); //apiSetFileAttributes(strFullName,FILE_ATTRIBUTE_NORMAL);

								int MsgCode=ERemoveDirectory(strFullName,Wipe);

								if (MsgCode==DELETE_CANCEL)
								{
									Cancel=true;;
									break;
								}
								else if (MsgCode==DELETE_SKIP)
								{
									//ScTree.SkipDir();
									continue;
								}

								TreeList::DelTreeName(strFullName);
							}
						}
						else
						{
							if (ShellRemoveFile(strFullName,Wipe)==DELETE_CANCEL)
							{
								Cancel=true;
								break;
							}
						}
					}
				}

				if (!Cancel)
				{
					if (FileAttr & FILE_ATTRIBUTE_READONLY)
						apiMakeWritable(strSelName); //apiSetFileAttributes(strSelName,FILE_ATTRIBUTE_NORMAL);

					int DeleteCode;

					// нефига здесь выделываться, а надо учесть, что удаление
					// симлинка в корзину чревато потерей оригинала.
					if (DirSymLink || !Opt.DeleteToRecycleBin || Wipe)
					{
						DeleteCode=ERemoveDirectory(strSelName,Wipe);

						if (DeleteCode==DELETE_CANCEL)
							break;
						else if (DeleteCode==DELETE_SUCCESS)
						{
							TreeList::DelTreeName(strSelName);

							if (UpdateDiz)
								SrcPanel->DeleteDiz(strSelName);
						}
					}
					else
					{
						DeleteCode=RemoveToRecycleBin(strSelName);

						if (!DeleteCode)
							Message(MSG_WARNING|MSG_ERRORTYPE,1,Msg::Error,Msg::CannotDeleteFolder,strSelName,Msg::Ok);
						else
						{
							TreeList::DelTreeName(strSelName);

							if (UpdateDiz)
								SrcPanel->DeleteDiz(strSelName);
						}
					}
				}
			}
			else
			{
				int DeleteCode=ShellRemoveFile(strSelName,Wipe);

				if (DeleteCode==DELETE_SUCCESS && UpdateDiz)
				{
					SrcPanel->DeleteDiz(strSelName);
				}

				if (DeleteCode==DELETE_CANCEL)
					break;
			}
		}
	}

	if (UpdateDiz)
		if (DizPresent==(!strDizName.IsEmpty() && apiGetFileAttributes(strDizName)!=INVALID_FILE_ATTRIBUTES))
			SrcPanel->FlushDiz();

	delete DeleteTitle;
done:
	Opt.DeleteToRecycleBin=Opt_DeleteToRecycleBin;
	// Разрешить перерисовку фрейма
	FrameFromLaunched->Unlock();

	if (NeedUpdate)
	{
		ShellUpdatePanels(SrcPanel,NeedSetUpADir);
	}
}

static void PR_ShellDeleteMsg()
{
	PreRedrawItem preRedrawItem=PreRedraw.Peek();
	ShellDeleteMsg(static_cast<const wchar_t*>(preRedrawItem.Param.Param1),static_cast<int>(reinterpret_cast<INT_PTR>(preRedrawItem.Param.Param4)),static_cast<int>(preRedrawItem.Param.Param5));
}

void ShellDeleteMsg(const wchar_t *Name,int Wipe,int Percent)
{
	FARString strProgress;
	size_t Width=52;

	if (Percent!=-1)
	{
		size_t Length=Width-5; // -5 под проценты
		wchar_t *Progress=strProgress.GetBuffer(Length);

		if (Progress)
		{
			size_t CurPos=Min(Percent,100)*Length/100;
			wmemset(Progress,BoxSymbols[BS_X_DB],CurPos);
			wmemset(Progress+(CurPos),BoxSymbols[BS_X_B0],Length-CurPos);
			strProgress.ReleaseBuffer(Length);
			FormatString strTmp;
			strTmp<<L" "<<fmt::Width(3)<<Percent<<L"%";
			strProgress+=strTmp;
			DeleteTitle->Set(L"{%d%%} %ls", Percent, (Wipe ? Msg::DeleteWipeTitle : Msg::DeleteTitle).CPtr());
		}

	}

	FARString strOutFileName(Name);
	TruncPathStr(strOutFileName,static_cast<int>(Width));
	CenterStr(strOutFileName,strOutFileName,static_cast<int>(Width));
	Message(0,0,(Wipe?Msg::DeleteWipeTitle:Msg::DeleteTitle),(Percent>=0||!Opt.DelOpt.DelShowTotal)?(Wipe?Msg::DeletingWiping:Msg::Deleting):Msg::ScanningFolder,strOutFileName,strProgress.IsEmpty()?nullptr:strProgress.CPtr());
	PreRedrawItem preRedrawItem=PreRedraw.Peek();
	preRedrawItem.Param.Param1=static_cast<void*>(const_cast<wchar_t*>(Name));
	preRedrawItem.Param.Param4=(void *)(INT_PTR)Wipe;
	preRedrawItem.Param.Param5=(int64_t)Percent;
	PreRedraw.SetParam(preRedrawItem.Param);
}

AskDeleteReadOnly::AskDeleteReadOnly(const wchar_t *Name,int Wipe) 
	: _r(DELETE_YES)
{
	int MsgCode;
	_ump = apiMakeWritable(Name);

	if (!_ump)//(Attr & FILE_ATTRIBUTE_READONLY))
		return;//(DELETE_YES);

	if (!Opt.Confirm.RO)
		ReadOnlyDeleteMode=1;

	if (ReadOnlyDeleteMode!=-1)
		MsgCode=ReadOnlyDeleteMode;
	else
	{
		MsgCode=Message(MSG_WARNING,5,Msg::Warning,Msg::DeleteRO,Name,
		                (Wipe?Msg::AskWipeRO:Msg::AskDeleteRO),(Wipe?Msg::DeleteFileWipe:Msg::DeleteFileDelete),
						Msg::DeleteFileAll,Msg::DeleteFileSkip,Msg::DeleteFileSkipAll,Msg::DeleteFileCancel);
	}

	switch (MsgCode)
	{
		case 1:
			ReadOnlyDeleteMode=1;
			break;
		case 2:
			_r = DELETE_SKIP;
			return;
		case 3:
			ReadOnlyDeleteMode=3;
			_r = DELETE_SKIP;
			return;
		case -1:
		case -2:
		case 4:
			_r = DELETE_CANCEL;
			return;
	}

	//apiSetFileAttributes(Name,FILE_ATTRIBUTE_NORMAL);
	//return(DELETE_YES);
}



int ShellRemoveFile(const wchar_t *Name, int Wipe)
{
	ProcessedItems++;
	int MsgCode=0;
	std::unique_ptr<AskDeleteReadOnly> AskDeleteRO;
	/* have to pretranslate to full path otherwise plain remove()
	 * below will malfunction if current directory requires sudo
	 */
	FARString strFullName;
	ConvertNameToFull(Name, strFullName);

	if (Wipe || Opt.DeleteToRecycleBin)
	{  /* in case its a not a simple deletion - check/sanitize RO files prior any actions,
		* cuz code that doing such things is not aware about such complications
		*/
		AskDeleteRO.reset(new AskDeleteReadOnly(strFullName, Wipe));
		if (AskDeleteRO->Choice() != DELETE_YES)
			return AskDeleteRO->Choice();
	}

	for (;;)
	{
		if (Wipe)
		{
			if (SkipWipeMode!=-1)
			{
				MsgCode=SkipWipeMode;
			}
			else if (0) //todo if (GetNumberOfLinks(strFullName)>1)
			{
				/*
				                            Файл
				                         "имя файла"
				                Файл имеет несколько жестких ссылок.
				  Уничтожение файла приведет к обнулению всех ссылающихся на него файлов.
				                        Уничтожать файл?
				*/
				MsgCode=Message(MSG_WARNING,5,Msg::Error,strFullName,
				                Msg::DeleteHardLink1,Msg::DeleteHardLink2,Msg::DeleteHardLink3,
				                Msg::DeleteFileWipe,Msg::DeleteFileAll,Msg::DeleteFileSkip,Msg::DeleteFileSkipAll,Msg::DeleteCancel);
			}

			switch (MsgCode)
			{
				case -1:
				case -2:
				case 4:
					return DELETE_CANCEL;
				case 3:
					SkipWipeMode=2;
				case 2:
					return DELETE_SKIP;
				case 1:
					SkipWipeMode=0;
				case 0:

					if (WipeFile(strFullName))
						return DELETE_SUCCESS;
			}
		}
		else if (!Opt.DeleteToRecycleBin)
		{
			// first try simple removal, only if it will fail
			// then fallback to AskDeleteRO and sdc_remove
			const std::string &mbFullName = strFullName.GetMB();
			if (!AskDeleteRO)
			{
				if (remove(mbFullName.c_str()) == 0 || errno == ENOENT) {
					break;
				}
				AskDeleteRO.reset(new AskDeleteReadOnly(strFullName, Wipe));
				if (AskDeleteRO->Choice() != DELETE_YES)
					return AskDeleteRO->Choice();
			}
			if (sdc_remove(mbFullName.c_str()) == 0 || errno == ENOENT) {
				break;
			}
		}
		else if (RemoveToRecycleBin(strFullName))
			break;

		if (SkipMode!=-1)
			MsgCode=SkipMode;
		else
		{
			MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,4,Msg::Error,
			                Msg::CannotDeleteFile,strFullName,Msg::DeleteRetry,
			                Msg::DeleteSkip,Msg::DeleteFileSkipAll,Msg::DeleteCancel);
		}

		switch (MsgCode)
		{
			case -1:
			case -2:
			case 3:
				return DELETE_CANCEL;
			case 2:
				SkipMode=1;
			case 1:
				return DELETE_SKIP;
		}
	}

	return DELETE_SUCCESS;
}


int ERemoveDirectory(const wchar_t *Name,int Wipe)
{
	ProcessedItems++;

	for (;;)
	{
		if (Wipe)
		{
			if (WipeDirectory(Name))
				break;
		} else {
			struct stat s = {};
			if (sdc_lstat(Wide2MB(Name).c_str(), &s)==0 && (s.st_mode & S_IFMT)==S_IFLNK )
			{
				if (sdc_unlink(Wide2MB(Name).c_str()) == 0) //if (apiDeleteFile(Name))
					break;
			}
			if (apiRemoveDirectory(Name))
				break;
		}

		int MsgCode;

		if (SkipFoldersMode!=-1)
			MsgCode=SkipFoldersMode;
		else
		{
			FARString strFullName;
			ConvertNameToFull(Name,strFullName);

			MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,4,Msg::Error,
			                Msg::CannotDeleteFolder,Name,Msg::DeleteRetry,
			                Msg::DeleteSkip,Msg::DeleteFileSkipAll,Msg::DeleteCancel);
		}

		switch (MsgCode)
		{
			case -1:
			case -2:
			case 3:
				return DELETE_CANCEL;
			case 1:
				return DELETE_SKIP;
			case 2:
				SkipFoldersMode=2;
				return DELETE_SKIP;
		}
	}

	return DELETE_SUCCESS;
}

int RemoveToRecycleBin(const wchar_t *Name)
{
	std::string name_mb = Wide2MB(Name);

	unsigned int flags = EF_HIDEOUT;
	if (sudo_client_is_required_for(name_mb.c_str(), true))
		flags|= EF_SUDO;

	QuoteCmdArgIfNeed(name_mb);

	std::string cmd = GetMyScriptQuoted("trash.sh");
	cmd+= ' ';
	cmd+= name_mb;

	int r = farExecuteA(cmd.c_str(), flags);
	if (r==0)
		return TRUE;

	errno = r;
	return FALSE;
}

static FARString WipingRename(const wchar_t *Name)
{
	FARString strTempName = Name;
	CutToSlash(strTempName, false);
	for (size_t i = 0, ii = 3 + (rand() % 4);
			(i < ii || apiGetFileAttributes(strTempName) != INVALID_FILE_ATTRIBUTES); ++i)
	{
		strTempName+= (wchar_t)'a' + (rand() % 26);
	}

	if (!apiMoveFile(Name, strTempName))
	{
		fprintf(stderr, "%s: error %u renaming '%ls' to '%ls'\n",
			__FUNCTION__, errno, Name, strTempName.CPtr());
		return Name;
	}

	fprintf(stderr, "%s: renamed '%ls' to '%ls'\n",
		__FUNCTION__, Name, strTempName.CPtr());

	return strTempName;
}

int WipeFile(const wchar_t *Name)
{
	uint64_t FileSize;
	apiMakeWritable(Name); //apiSetFileAttributes(Name,FILE_ATTRIBUTE_NORMAL);
	File WipeFile;
	if(!WipeFile.Open(Name, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_SEQUENTIAL_SCAN))
	{
		return FALSE;
	}

	if (!WipeFile.GetSize(FileSize))
	{
		WipeFile.Close();
		return FALSE;
	}

	if(FileSize)
	{
		const int BufSize=65536;
		LPBYTE Buf=new BYTE[BufSize];
		memset(Buf, Opt.WipeSymbol, BufSize); // используем символ заполнитель
		DWORD Written;
		while (FileSize>0)
		{
			DWORD WriteSize=(DWORD)Min((uint64_t)BufSize,FileSize);
			WipeFile.Write(Buf,WriteSize,&Written);
			FileSize-=WriteSize;
		}
		WipeFile.Write(Buf,BufSize,&Written);
		delete[] Buf;
		WipeFile.SetPointer(0,nullptr,FILE_BEGIN);
		WipeFile.SetEnd();
	}

	WipeFile.Close();

	FARString strRemoveName = WipingRename(Name);
	return apiDeleteFile(strRemoveName);
}


int WipeDirectory(const wchar_t *Name)
{
	FARString strTempName, strPath;

	if (FirstSlash(Name))
	{
		strPath = Name;
		DeleteEndSlash(strPath);
		CutToSlash(strPath);
	}

	FARString strRemoveName = WipingRename(Name);
	return apiRemoveDirectory(strRemoveName);
}

int DeleteFileWithFolder(const wchar_t *FileName)
{
	SudoClientRegion scr;
	FARString strFileOrFolderName, strParentFolder;
	strFileOrFolderName = FileName;
	Unquote(strFileOrFolderName);

	strParentFolder = strFileOrFolderName;
	CutToSlash(strParentFolder, true);
	if (!strParentFolder.IsEmpty())
		apiMakeWritable(strParentFolder);
	apiMakeWritable(strFileOrFolderName);
	/*BOOL Ret=apiSetFileAttributes(strFileOrFolderName,FILE_ATTRIBUTE_NORMAL);

	if (Ret)*/
	{
		if (apiDeleteFile(strFileOrFolderName)) //BUGBUG
		{
			return apiRemoveDirectory(strParentFolder);
		}
	}

	WINPORT(SetLastError)((_localLastError = WINPORT(GetLastError)()));
	return FALSE;
}


void DeleteDirTree(const wchar_t *Dir)
{
	if (!*Dir || (IsSlash(Dir[0]) && !Dir[1]))
		return;

	SudoClientRegion scr;
	FARString strFullName;
	FAR_FIND_DATA_EX FindData;
	ScanTree ScTree(TRUE,TRUE,FALSE);
	ScTree.SetFindPath(Dir,L"*",0);

	while (ScTree.GetNextName(&FindData, strFullName))
	{
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			apiMakeWritable(strFullName);
//		apiSetFileAttributes(strFullName,FILE_ATTRIBUTE_NORMAL);

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (ScTree.IsDirSearchDone())
				apiRemoveDirectory(strFullName);
		}
		else
			apiDeleteFile(strFullName);
	}
	apiMakeWritable(Dir);
//	apiSetFileAttributes(Dir,FILE_ATTRIBUTE_NORMAL);
	apiRemoveDirectory(Dir);
}
