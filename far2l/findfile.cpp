/*
findfile.cpp

Поиск (Alt-F7)
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

#include "findfile.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "fileview.hpp"
#include "fileedit.hpp"
#include "filelist.hpp"
#include "cmdline.hpp"
#include "chgprior.hpp"
#include "namelist.hpp"
#include "scantree.hpp"
#include "manager.hpp"
#include "scrbuf.hpp"
#include "CFileMask.hpp"
#include "filefilter.hpp"
#include "syslog.hpp"
//#include "localOEM.hpp"
#include "codepage.hpp"
#include "registry.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "delete.hpp"
#include "datetime.hpp"
#include "drivemix.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "mix.hpp"
#include "constitle.hpp"
#include "DlgGuid.hpp"
#include "console.hpp"
#include "wakeful.hpp"
#include "panelmix.hpp"
#include "setattr.hpp"
#include "udlist.hpp"

const int CHAR_TABLE_SIZE=5;
const int LIST_DELTA=64;
const DWORD LIST_INDEX_NONE = static_cast<DWORD>(-1);

const size_t readBufferSizeA=32768;
const size_t readBufferSize=(readBufferSizeA*sizeof(wchar_t));

bool AnySetFindList=false;

// Список найденных файлов. Индекс из списка хранится в меню.
struct FINDLIST
{
	FAR_FIND_DATA_EX FindData;
	size_t ArcIndex;
	DWORD Used;
};

// Список архивов. Если файл найден в архиве, то FindList->ArcIndex указывает сюда.
struct ARCLIST
{
	FARString strArcName;
	HANDLE hPlugin;    // Plugin handle
	DWORD Flags;       // OpenPluginInfo.Flags
	FARString strRootPath; // Root path in plugin after opening.
};

struct InterThreadData
{
private:
	CriticalSection DataCS;
	size_t FindFileArcIndex;
	int Percent;
	int LastFoundNumber;
	int FileCount;
	int DirCount;

	FINDLIST **FindList;
	size_t FindListCapacity;
	size_t FindListCount;
	ARCLIST **ArcList;
	size_t ArcListCapacity;
	size_t ArcListCount;
	FARString strFindMessage;

public:
	void Init()
	{
		CriticalSectionLock Lock(DataCS);
		FindFileArcIndex=LIST_INDEX_NONE;
		Percent=0;
		LastFoundNumber=0;
		FileCount=0;
		DirCount=0;
		FindList=nullptr;
		FindListCapacity=0;
		FindListCount=0;
		ArcList=nullptr;
		ArcListCapacity=0;
		ArcListCount=0;
		strFindMessage.Clear();
	}

	size_t GetFindFileArcIndex(){CriticalSectionLock Lock(DataCS); return FindFileArcIndex;}
	void SetFindFileArcIndex(size_t Value){CriticalSectionLock Lock(DataCS); FindFileArcIndex=Value;}

	int GetPercent(){CriticalSectionLock Lock(DataCS); return Percent;}
	void SetPercent(int Value){CriticalSectionLock Lock(DataCS); Percent=Value;}

	int GetLastFoundNumber(){CriticalSectionLock Lock(DataCS); return LastFoundNumber;}
	void SetLastFoundNumber(int Value){CriticalSectionLock Lock(DataCS); LastFoundNumber=Value;}

	int GetFileCount(){CriticalSectionLock Lock(DataCS); return FileCount;}
	void SetFileCount(int Value){CriticalSectionLock Lock(DataCS); FileCount=Value;}

	int GetDirCount(){CriticalSectionLock Lock(DataCS); return DirCount;}
	void SetDirCount(int Value){CriticalSectionLock Lock(DataCS); DirCount=Value;}

	size_t GetFindListCount(){CriticalSectionLock Lock(DataCS); return FindListCount;}

	void GetFindMessage(FARString& To)
	{
		CriticalSectionLock Lock(DataCS);
		To=strFindMessage;
	}

	void SetFindMessage(const FARString& From)
	{
		CriticalSectionLock Lock(DataCS);
		strFindMessage=From;
	}

	void GetFindListItem(size_t index, FINDLIST& Item)
	{
		CriticalSectionLock Lock(DataCS);
		Item.FindData=FindList[index]->FindData;
		Item.ArcIndex=FindList[index]->ArcIndex;
		Item.Used=FindList[index]->Used;
	}

	void SetFindListItem(size_t index, const FINDLIST& Item)
	{
		CriticalSectionLock Lock(DataCS);
		FindList[index]->FindData=Item.FindData;
		FindList[index]->ArcIndex=Item.ArcIndex;
		FindList[index]->Used=Item.Used;
	}

	void GetArcListItem(size_t index, ARCLIST& Item)
	{
		CriticalSectionLock Lock(DataCS);
		Item.strArcName=ArcList[index]->strArcName;
		Item.hPlugin=ArcList[index]->hPlugin;
		Item.Flags=ArcList[index]->Flags;
		Item.strRootPath=ArcList[index]->strRootPath;
	}

	void SetArcListItem(size_t index, const ARCLIST& Item)
	{
		CriticalSectionLock Lock(DataCS);
		ArcList[index]->strArcName=Item.strArcName;
		ArcList[index]->hPlugin=Item.hPlugin;
		ArcList[index]->Flags=Item.Flags;
		ArcList[index]->strRootPath=Item.strRootPath;
	}

	void ClearAllLists()
	{
		CriticalSectionLock Lock(DataCS);
		FindFileArcIndex=LIST_INDEX_NONE;

		if (FindList)
		{
			for (size_t i = 0; i < FindListCount; i++)
			{
				delete FindList[i];
			}
			xf_free(FindList);
		}
		FindList = nullptr;
		FindListCapacity = FindListCount = 0;

		if (ArcList)
		{
			for (size_t i = 0; i < ArcListCount; i++)
			{
				delete ArcList[i];
			}
			xf_free(ArcList);
		}
		ArcList = nullptr;
		ArcListCapacity = ArcListCount = 0;
	}

	bool FindListGrow()
	{
		CriticalSectionLock Lock(DataCS);
		bool Result=false;
		size_t Delta=(FindListCapacity<256)?LIST_DELTA:FindListCapacity/2;
		FINDLIST** NewList = reinterpret_cast<FINDLIST**>(xf_realloc(FindList,(FindListCapacity+Delta)*sizeof(*FindList)));
		if (NewList)
		{
			FindList=NewList;
			FindListCapacity+=Delta;
			Result=true;
		}
		return Result;
	}

	bool ArcListGrow()
	{
		CriticalSectionLock Lock(DataCS);
		bool Result=false;
		size_t Delta=(ArcListCapacity<256)?LIST_DELTA:ArcListCapacity/2;
		ARCLIST** NewList=reinterpret_cast<ARCLIST**>(xf_realloc(ArcList,(ArcListCapacity+Delta)*sizeof(*ArcList)));

		if (NewList)
		{
			ArcList = NewList;
			ArcListCapacity+= Delta;
			Result=true;
		}
		return Result;
	}

	size_t AddArcListItem(const wchar_t *ArcName,HANDLE hPlugin,DWORD dwFlags,const wchar_t *RootPath)
	{
		CriticalSectionLock Lock(DataCS);
		if ((ArcListCount == ArcListCapacity) && (!ArcListGrow()))
			return LIST_INDEX_NONE;

		ArcList[ArcListCount] = new ARCLIST;
		ArcList[ArcListCount]->strArcName = ArcName;
		ArcList[ArcListCount]->hPlugin = hPlugin;
		ArcList[ArcListCount]->Flags = dwFlags;
		ArcList[ArcListCount]->strRootPath = RootPath;
		AddEndSlash(ArcList[ArcListCount]->strRootPath);
		return ArcListCount++;
	}

	size_t AddFindListItem(const FAR_FIND_DATA_EX& FindData)
	{
		CriticalSectionLock Lock(DataCS);
		if ((FindListCount == FindListCapacity)&&(!FindListGrow()))
			return LIST_INDEX_NONE;

		FindList[FindListCount] = new FINDLIST;
		FindList[FindListCount]->FindData = FindData;
		FindList[FindListCount]->ArcIndex = LIST_INDEX_NONE;
		return FindListCount++;
	}

}
itd;


enum
{
	FIND_EXIT_NONE,
	FIND_EXIT_SEARCHAGAIN,
	FIND_EXIT_GOTO,
	FIND_EXIT_PANEL
};

struct Vars
{
	Vars()
	{
	}

	~Vars()
	{
	}

	// Используются для отправки файлов на временную панель.
	// индекс текущего элемента в списке и флаг для отправки.
	DWORD FindExitIndex;
	bool FindFoldersChanged;
	bool SearchFromChanged;
	bool FindPositionChanged;
	bool Finalized;
	bool PluginMode;

	void Clear()
	{
		FindExitIndex=LIST_INDEX_NONE;
		FindFoldersChanged=false;
		SearchFromChanged=false;
		FindPositionChanged=false;
		Finalized=false;
		PluginMode=false;
	}

};

FARString strFindMask, strFindStr;
int SearchMode,CmpCase,WholeWords,SearchInArchives,SearchHex;

FARString strLastDirName;
FARString strPluginSearchPath;

CriticalSection PluginCS;

//Event PauseEvent(true, true);
//Event StopEvent(true, false);
volatile LONG PauseFlag = 0, StopFlag = 0; 

bool UseFilter=false;
UINT CodePage=CP_AUTODETECT;
UINT64 SearchInFirst=0;

char *readBufferA;
wchar_t *readBuffer;
int codePagesCount;

struct CodePageInfo
{
	UINT CodePage;
	UINT MaxCharSize;
	wchar_t LastSymbol;
	bool WordFound;
} *codePages;

unsigned char *hexFindString;
size_t hexFindStringSize;
wchar_t *findString,*findStringBuffer;

size_t *skipCharsTable;
int favoriteCodePages = 0;

bool InFileSearchInited=false;

CFileMask FileMaskForFindFile;

FileFilter *Filter;

enum ADVANCEDDLG
{
	AD_DOUBLEBOX,
	AD_TEXT_SEARCHFIRST,
	AD_EDIT_SEARCHFIRST,
	AD_CHECKBOX_FINDALTERNATESTREAMS,
	AD_SEPARATOR1,
	AD_TEXT_COLUMNSFORMAT,
	AD_EDIT_COLUMNSFORMAT,
	AD_TEXT_COLUMNSWIDTH,
	AD_EDIT_COLUMNSWIDTH,
	AD_SEPARATOR2,
	AD_BUTTON_OK,
	AD_BUTTON_CANCEL,
};

enum FINDASKDLG
{
	FAD_DOUBLEBOX,
	FAD_TEXT_MASK,
	FAD_EDIT_MASK,
	FAD_SEPARATOR0,
	FAD_TEXT_TEXTHEX,
	FAD_EDIT_TEXT,
	FAD_EDIT_HEX,
	FAD_TEXT_CP,
	FAD_COMBOBOX_CP,
	FAD_SEPARATOR1,
	FAD_CHECKBOX_CASE,
	FAD_CHECKBOX_WHOLEWORDS,
	FAD_CHECKBOX_HEX,
	FAD_CHECKBOX_ARC,
	FAD_CHECKBOX_DIRS,
	FAD_CHECKBOX_LINKS,
	FAD_SEPARATOR_2,
	FAD_SEPARATOR_3,
	FAD_TEXT_WHERE,
	FAD_COMBOBOX_WHERE,
	FAD_CHECKBOX_FILTER,
	FAD_SEPARATOR_4,
	FAD_BUTTON_FIND,
	FAD_BUTTON_DRIVE,
	FAD_BUTTON_FILTER,
	FAD_BUTTON_ADVANCED,
	FAD_BUTTON_CANCEL,
};

enum FINDASKDLGCOMBO
{
	FADC_ALLDISKS,
	FADC_ALLBUTNET,
	FADC_PATH,
	FADC_ROOT,
	FADC_FROMCURRENT,
	FADC_INCURRENT,
	FADC_SELECTED,
};

enum FINDDLG
{
	FD_DOUBLEBOX,
	FD_LISTBOX,
	FD_SEPARATOR1,
	FD_TEXT_STATUS,
	FD_TEXT_STATUS_PERCENTS,
	FD_SEPARATOR2,
	FD_BUTTON_NEW,
	FD_BUTTON_GOTO,
	FD_BUTTON_VIEW,
	FD_BUTTON_PANEL,
	FD_BUTTON_STOP,
};

void InitInFileSearch()
{
	if (!InFileSearchInited && !strFindStr.IsEmpty())
	{
		size_t findStringCount = strFindStr.GetLength();
		// Инициализируем буферы чтения из файла
		readBufferA = (char *)xf_malloc(readBufferSizeA);
		readBuffer = (wchar_t *)xf_malloc(readBufferSize);

		if (!SearchHex)
		{
			// Формируем строку поиска
			if (!CmpCase)
			{
				findStringBuffer = (wchar_t *)xf_malloc(2*findStringCount*sizeof(wchar_t));
				findString=findStringBuffer;

				for (size_t index = 0; index<strFindStr.GetLength(); index++)
				{
					wchar_t ch = strFindStr[index];

					if (WINPORT(IsCharLower)(ch))
					{
						findString[index]=Upper(ch);
						findString[index+findStringCount]=ch;
					}
					else
					{
						findString[index]=ch;
						findString[index+findStringCount]=Lower(ch);
					}
				}
			}
			else
				findString = strFindStr.GetBuffer();

			// Инизиализируем данные для алгоритма поиска
			skipCharsTable = (size_t *)xf_malloc((MAX_VKEY_CODE+1)*sizeof(size_t));

			for (size_t index = 0; index < MAX_VKEY_CODE+1; index++)
				skipCharsTable[index] = findStringCount;

			for (size_t index = 0; index < findStringCount-1; index++)
				skipCharsTable[findString[index]] = findStringCount-1-index;

			if (!CmpCase)
				for (size_t index = 0; index < findStringCount-1; index++)
					skipCharsTable[findString[index+findStringCount]] = findStringCount-1-index;

			// Формируем список кодовых страниц
			if (CodePage == CP_AUTODETECT)
			{
				DWORD data;
				FARString codePageName;
				bool hasSelected = false;

				// Проверяем наличие выбранных страниц символов
				for (int i=0; EnumRegValue(FavoriteCodePagesKey, i, codePageName, (BYTE *)&data, sizeof(data)); i++)
				{
					if (data & CPST_FIND)
					{
						hasSelected = true;
						break;
					}
				}

				// Добавляем стандартные таблицы символов
				if (!hasSelected)
				{
					codePagesCount = StandardCPCount;
					codePages = (CodePageInfo *)xf_malloc(codePagesCount*sizeof(CodePageInfo));
					codePages[0].CodePage = WINPORT(GetOEMCP)();
					codePages[1].CodePage = WINPORT(GetACP)();
					codePages[2].CodePage = CP_UTF7;
					codePages[3].CodePage = CP_UTF8;
					codePages[4].CodePage = CP_UNICODE;
					codePages[5].CodePage = CP_REVERSEBOM;
				}
				else
				{
					codePagesCount = 0;
					codePages = nullptr;
				}

				// Добавляем стандартные таблицы символов
				for (int i=0; EnumRegValue(FavoriteCodePagesKey, i, codePageName, (BYTE *)&data, sizeof(data)); i++)
				{
					if (data & (hasSelected?CPST_FIND:CPST_FAVORITE))
					{
						UINT codePage = _wtoi(codePageName);

						// Проверяем дубли
						if (!hasSelected)
						{
							bool isDouble = false;

							for (int j = 0; j<StandardCPCount; j++)
								if (codePage == codePages[j].CodePage)
								{
									isDouble =true;
									break;
								}

							if (isDouble)
								continue;
						}

						codePages = (CodePageInfo *)xf_realloc((void *)codePages, ++codePagesCount*sizeof(CodePageInfo));
						codePages[codePagesCount-1].CodePage = codePage;
					}
				}
			}
			else
			{
				codePagesCount = 1;
				codePages = (CodePageInfo *)xf_malloc(codePagesCount*sizeof(CodePageInfo));
				codePages[0].CodePage = CodePage;
			}

			for (int index = 0; index<codePagesCount; index++)
			{
				CodePageInfo *cp = codePages+index;

				if (IsUnicodeCodePage(cp->CodePage))
					cp->MaxCharSize = 2;
				else
				{
					CPINFO cpi;

					if (!WINPORT(GetCPInfo)(cp->CodePage, &cpi))
						cpi.MaxCharSize = 0; //Считаем, что ошибка и потом такие таблицы в поиске пропускаем

					cp->MaxCharSize = cpi.MaxCharSize;
				}

				cp->LastSymbol = 0;
				cp->WordFound = false;
			}
		}
		else
		{
			// Формируем hex-строку для поиска
			hexFindStringSize = 0;

			if (SearchHex)
			{
				bool flag = false;
				hexFindString = (unsigned char *)xf_malloc((findStringCount-findStringCount/3+1)/2);

				for (size_t index = 0; index < strFindStr.GetLength(); index++)
				{
					wchar_t symbol = strFindStr.At(index);
					BYTE offset = 0;

					if (symbol >= L'a' && symbol <= L'f')
						offset = 87;
					else if (symbol >= L'A' && symbol <= L'F')
						offset = 55;
					else if (symbol >= L'0' && symbol <= L'9')
						offset = 48;
					else
						continue;

					if (!flag)
						hexFindString[hexFindStringSize++] = ((BYTE)symbol-offset)<<4;
					else
						hexFindString[hexFindStringSize-1] |= ((BYTE)symbol-offset);

					flag = !flag;
				}
			}

			// Инизиализируем данные для аглоритма поиска
			skipCharsTable = (size_t *)xf_malloc((255+1)*sizeof(size_t));

			for (size_t index = 0; index < 255+1; index++)
				skipCharsTable[index] = hexFindStringSize;

			for (size_t index = 0; index < (size_t)hexFindStringSize-1; index++)
				skipCharsTable[hexFindString[index]] = hexFindStringSize-1-index;
		}

		InFileSearchInited=true;
	}
}

void ReleaseInFileSearch()
{
	if (InFileSearchInited && !strFindStr.IsEmpty())
	{
		if (readBufferA)
		{
			xf_free(readBufferA);
			readBufferA=nullptr;
		}

		if (readBuffer)
		{
			xf_free(readBuffer);
			readBuffer=nullptr;
		}

		if (skipCharsTable)
		{
			xf_free(skipCharsTable);
			skipCharsTable=nullptr;
		}

		if (codePages)
		{
			xf_free(codePages);
			codePages=nullptr;
		}

		if (findStringBuffer)
		{
			xf_free(findStringBuffer);
			findStringBuffer=nullptr;
		}

		if (hexFindString)
		{
			xf_free(hexFindString);
			hexFindString=nullptr;
		}

		InFileSearchInited=false;
	}
}

FARString &PrepareDriveNameStr(FARString &strSearchFromRoot)
{
	FARString strCurDir;
	CtrlObject->CmdLine->GetCurDir(strCurDir);
	GetPathRoot(strCurDir,strCurDir);
	DeleteEndSlash(strCurDir);

	if (
	    strCurDir.IsEmpty()||
	    (CtrlObject->Cp()->ActivePanel->GetMode()==PLUGIN_PANEL && CtrlObject->Cp()->ActivePanel->IsVisible())
	)
	{
		strSearchFromRoot = MSG(MSearchFromRootFolder);
	}
	else
	{
		strSearchFromRoot= MSG(MSearchFromRootOfDrive);
		strSearchFromRoot+=L" ";
		strSearchFromRoot+=strCurDir;
	}

	return strSearchFromRoot;
}

// Проверяем символ на принадлежность разделителям слов
bool IsWordDiv(const wchar_t symbol)
{
	// Так же разделителем является конец строки и пробельные символы
	return !symbol||IsSpace(symbol)||IsEol(symbol)||IsWordDiv(Opt.strWordDiv,symbol);
}

void SetPluginDirectory(const wchar_t *DirName,HANDLE hPlugin,bool UpdatePanel=false)
{
	if (DirName && *DirName)
	{
		FARString strName(DirName);
		wchar_t* DirPtr = strName.GetBuffer();
		wchar_t* NamePtr = (wchar_t*) PointToName(DirPtr);

		if (NamePtr != DirPtr)
		{
			*(NamePtr-1) = 0;
			// force plugin to update its file list (that can be empty at this time)
			// if not done SetDirectory may fail
			{
				int FileCount=0;
				PluginPanelItem *PanelData=nullptr;

				if (CtrlObject->Plugins.GetFindData(hPlugin,&PanelData,&FileCount,OPM_SILENT))
				{
					CtrlObject->Plugins.FreeFindData(hPlugin,PanelData,FileCount);
				}
			}

			if (*DirPtr)
			{
				CtrlObject->Plugins.SetDirectory(hPlugin,DirPtr,OPM_SILENT);
			}
			else
			{
				CtrlObject->Plugins.SetDirectory(hPlugin,L"/",OPM_SILENT);
			}
		}

		// Отрисуем панель при необходимости.
		if (UpdatePanel)
		{
			CtrlObject->Cp()->ActivePanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->ActivePanel->GoToFile(NamePtr);
			CtrlObject->Cp()->ActivePanel->Show();
		}

		//strName.ReleaseBuffer(); Не надо. Строка все ровно удаляется, лишний вызов StrLength.
	}
}

LONG_PTR WINAPI AdvancedDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg)
	{
		case DN_CLOSE:

			if (Param1==AD_BUTTON_OK)
			{
				LPCWSTR Data=reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,AD_EDIT_SEARCHFIRST,0));

				if (Data && *Data && !CheckFileSizeStringFormat(Data))
				{
					Message(MSG_WARNING,1,MSG(MFindFileAdvancedTitle),MSG(MBadFileSizeFormat),MSG(MOk));
					return FALSE;
				}
			}

			break;
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

void AdvancedDialog()
{
	DialogDataEx AdvancedDlgData[]=
	{
		DI_DOUBLEBOX,3,1,52,12,0,0,MSG(MFindFileAdvancedTitle),
		DI_TEXT,5,2,0,2,0,0,MSG(MFindFileSearchFirst),
		DI_EDIT,5,3,50,3,0,0,Opt.FindOpt.strSearchInFirstSize,
		DI_CHECKBOX,5,4,0,4,Opt.FindOpt.FindAlternateStreams,0,MSG(MFindAlternateStreams),
		DI_TEXT,3,5,0,5,0,DIF_SEPARATOR,L"",
		DI_TEXT,5,6, 0, 6,0,0,MSG(MFindAlternateModeTypes),
		DI_EDIT,5,7,35, 7,0,0,Opt.FindOpt.strSearchOutFormat,
		DI_TEXT,5,8, 0, 8,0,0,MSG(MFindAlternateModeWidths),
		DI_EDIT,5,9,35, 9,0,0,Opt.FindOpt.strSearchOutFormatWidth,
		DI_TEXT,3,10,0,10,0,DIF_SEPARATOR,L"",
		DI_BUTTON,0,11,0,11,0,DIF_DEFAULT|DIF_CENTERGROUP,MSG(MOk),
		DI_BUTTON,0,11,0,11,0,DIF_CENTERGROUP,MSG(MCancel),
	};
	MakeDialogItemsEx(AdvancedDlgData,AdvancedDlg);
	Dialog Dlg(AdvancedDlg,ARRAYSIZE(AdvancedDlg),AdvancedDlgProc);
	Dlg.SetHelp(L"FindFileAdvanced");
	Dlg.SetPosition(-1,-1,52+4,7+7);
	Dlg.Process();
	int ExitCode=Dlg.GetExitCode();

	if (ExitCode==AD_BUTTON_OK)
	{
		Opt.FindOpt.strSearchInFirstSize = AdvancedDlg[AD_EDIT_SEARCHFIRST].strData;
		SearchInFirst=ConvertFileSizeString(Opt.FindOpt.strSearchInFirstSize);

		Opt.FindOpt.strSearchOutFormat = AdvancedDlg[AD_EDIT_COLUMNSFORMAT].strData;
		Opt.FindOpt.strSearchOutFormatWidth = AdvancedDlg[AD_EDIT_COLUMNSWIDTH].strData;

		memset(Opt.FindOpt.OutColumnTypes,0,sizeof(Opt.FindOpt.OutColumnTypes));
		memset(Opt.FindOpt.OutColumnWidths,0,sizeof(Opt.FindOpt.OutColumnWidths));
		memset(Opt.FindOpt.OutColumnWidthType,0,sizeof(Opt.FindOpt.OutColumnWidthType));
		Opt.FindOpt.OutColumnCount=0;

		if (!Opt.FindOpt.strSearchOutFormat.IsEmpty())
		{
			if (Opt.FindOpt.strSearchOutFormatWidth.IsEmpty())
				Opt.FindOpt.strSearchOutFormatWidth=L"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";

			TextToViewSettings(Opt.FindOpt.strSearchOutFormat.CPtr(),Opt.FindOpt.strSearchOutFormatWidth.CPtr(),
                                  Opt.FindOpt.OutColumnTypes,Opt.FindOpt.OutColumnWidths,Opt.FindOpt.OutColumnWidthType,
                                  Opt.FindOpt.OutColumnCount);
        }

		Opt.FindOpt.FindAlternateStreams=(AdvancedDlg[AD_CHECKBOX_FINDALTERNATESTREAMS].Selected==BSTATE_CHECKED);
	}
}

LONG_PTR WINAPI MainDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	Vars* v = reinterpret_cast<Vars*>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
	switch (Msg)
	{
		case DN_INITDIALOG:
		{
			bool Hex=(SendDlgMessage(hDlg,DM_GETCHECK,FAD_CHECKBOX_HEX,0)==BSTATE_CHECKED);
			SendDlgMessage(hDlg,DM_SHOWITEM,FAD_EDIT_TEXT,!Hex);
			SendDlgMessage(hDlg,DM_SHOWITEM,FAD_EDIT_HEX,Hex);
			SendDlgMessage(hDlg,DM_ENABLE,FAD_TEXT_CP,!Hex);
			SendDlgMessage(hDlg,DM_ENABLE,FAD_COMBOBOX_CP,!Hex);
			SendDlgMessage(hDlg,DM_ENABLE,FAD_CHECKBOX_CASE,!Hex);
			SendDlgMessage(hDlg,DM_ENABLE,FAD_CHECKBOX_WHOLEWORDS,!Hex);
			SendDlgMessage(hDlg,DM_ENABLE,FAD_CHECKBOX_DIRS,!Hex);
			SendDlgMessage(hDlg,DM_EDITUNCHANGEDFLAG,FAD_EDIT_TEXT,1);
			SendDlgMessage(hDlg,DM_EDITUNCHANGEDFLAG,FAD_EDIT_HEX,1);
			SendDlgMessage(hDlg,DM_SETTEXTPTR,FAD_TEXT_TEXTHEX,(LONG_PTR)(Hex?MSG(MFindFileHex):MSG(MFindFileText)));
			SendDlgMessage(hDlg,DM_SETTEXTPTR,FAD_TEXT_CP,(LONG_PTR)MSG(MFindFileCodePage));
			SendDlgMessage(hDlg,DM_SETCOMBOBOXEVENT,FAD_COMBOBOX_CP,CBET_KEY);
			FarListTitles Titles={0,nullptr,0,MSG(MFindFileCodePageBottom)};
			SendDlgMessage(hDlg,DM_LISTSETTITLES,FAD_COMBOBOX_CP,reinterpret_cast<LONG_PTR>(&Titles));
			// Установка запомненных ранее параметров
			CodePage = Opt.FindCodePage;
			favoriteCodePages = FillCodePagesList(hDlg, FAD_COMBOBOX_CP, CodePage, false, true);
			// Текущее значение в в списке выбора кодовых страниц в общем случае модет не совпадать с CodePage,
			// так что получаем CodePage из списка выбора
			FarListPos Position;
			SendDlgMessage(hDlg, DM_LISTGETCURPOS, FAD_COMBOBOX_CP, (LONG_PTR)&Position);
			FarListGetItem Item = { Position.SelectPos };
			SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_CP, (LONG_PTR)&Item);
			CodePage = (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, FAD_COMBOBOX_CP, Position.SelectPos);
			return TRUE;
		}
		case DN_CLOSE:
		{
			switch (Param1)
			{
				case FAD_BUTTON_FIND:
				{
					LPCWSTR Mask=(LPCWSTR)SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,FAD_EDIT_MASK,0);

					if (!Mask||!*Mask)
						Mask=L"*";

					return FileMaskForFindFile.Set(Mask,0);
				}
				case FAD_BUTTON_DRIVE:
				{
					IsRedrawFramesInProcess++;
					CtrlObject->Cp()->ActivePanel->ChangeDisk();
					// Ну что ж, раз пошла такая пьянка рефрешить фреймы
					// будем таким способом.
					//FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
					FrameManager->ResizeAllFrame();
					IsRedrawFramesInProcess--;
					FARString strSearchFromRoot;
					PrepareDriveNameStr(strSearchFromRoot);
					FarListGetItem item={FADC_ROOT};
					SendDlgMessage(hDlg,DM_LISTGETITEM,FAD_COMBOBOX_WHERE,(LONG_PTR)&item);
					item.Item.Text=strSearchFromRoot;
					SendDlgMessage(hDlg,DM_LISTUPDATE,FAD_COMBOBOX_WHERE,(LONG_PTR)&item);
					v->PluginMode=CtrlObject->Cp()->ActivePanel->GetMode()==PLUGIN_PANEL;
					SendDlgMessage(hDlg,DM_ENABLE,FAD_CHECKBOX_DIRS,v->PluginMode?FALSE:TRUE);
					item.ItemIndex=FADC_ALLDISKS;
					SendDlgMessage(hDlg,DM_LISTGETITEM,FAD_COMBOBOX_WHERE,(LONG_PTR)&item);

					if (v->PluginMode)
						item.Item.Flags|=LIF_GRAYED;
					else
						item.Item.Flags&=~LIF_GRAYED;

					SendDlgMessage(hDlg,DM_LISTUPDATE,FAD_COMBOBOX_WHERE,(LONG_PTR)&item);
					item.ItemIndex=FADC_ALLBUTNET;
					SendDlgMessage(hDlg,DM_LISTGETITEM,FAD_COMBOBOX_WHERE,(LONG_PTR)&item);

					if (v->PluginMode)
						item.Item.Flags|=LIF_GRAYED;
					else
						item.Item.Flags&=~LIF_GRAYED;

					SendDlgMessage(hDlg,DM_LISTUPDATE,FAD_COMBOBOX_WHERE,(LONG_PTR)&item);
				}
				break;
				case FAD_BUTTON_FILTER:
					Filter->FilterEdit();
					break;
				case FAD_BUTTON_ADVANCED:
					AdvancedDialog();
					break;
				case -2:
				case -1:
				case FAD_BUTTON_CANCEL:
					return TRUE;
			}

			return FALSE;
		}
		case DN_BTNCLICK:
		{
			switch (Param1)
			{
				case FAD_CHECKBOX_DIRS:
					{
						v->FindFoldersChanged = true;
					}
					break;

				case FAD_CHECKBOX_HEX:
				{
					SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
					FARString strDataStr;
					Transform(strDataStr,(LPCWSTR)SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,Param2?FAD_EDIT_TEXT:FAD_EDIT_HEX,0),Param2?L'X':L'S');
					SendDlgMessage(hDlg,DM_SETTEXTPTR,Param2?FAD_EDIT_HEX:FAD_EDIT_TEXT,(LONG_PTR)strDataStr.CPtr());
					SendDlgMessage(hDlg,DM_SHOWITEM,FAD_EDIT_TEXT,!Param2);
					SendDlgMessage(hDlg,DM_SHOWITEM,FAD_EDIT_HEX,Param2);
					SendDlgMessage(hDlg,DM_ENABLE,FAD_TEXT_CP,!Param2);
					SendDlgMessage(hDlg,DM_ENABLE,FAD_COMBOBOX_CP,!Param2);
					SendDlgMessage(hDlg,DM_ENABLE,FAD_CHECKBOX_CASE,!Param2);
					SendDlgMessage(hDlg,DM_ENABLE,FAD_CHECKBOX_WHOLEWORDS,!Param2);
					SendDlgMessage(hDlg,DM_ENABLE,FAD_CHECKBOX_DIRS,!Param2);
					SendDlgMessage(hDlg,DM_SETTEXTPTR,FAD_TEXT_TEXTHEX,(LONG_PTR)(Param2?MSG(MFindFileHex):MSG(MFindFileText)));

					if (strDataStr.GetLength()>0)
					{
						int UnchangeFlag=(int)SendDlgMessage(hDlg,DM_EDITUNCHANGEDFLAG,FAD_EDIT_TEXT,-1);
						SendDlgMessage(hDlg,DM_EDITUNCHANGEDFLAG,FAD_EDIT_HEX,UnchangeFlag);
					}

					SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
				}
				break;
			}

			break;
		}
		case DM_KEY:
		{
			switch (Param1)
			{
				case FAD_COMBOBOX_CP:
				{
					switch (Param2)
					{
						case KEY_INS:
						case KEY_NUMPAD0:
						case KEY_SPACE:
						{
							// Обработка установки/снятия флажков для стандартных и любимых таблиц символов
							// Получаем текущую позицию в выпадающем списке таблиц символов
							FarListPos Position;
							SendDlgMessage(hDlg, DM_LISTGETCURPOS, FAD_COMBOBOX_CP, (LONG_PTR)&Position);
							// Получаем номер выбранной таблицы симолов
							FarListGetItem Item = { Position.SelectPos };
							SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_CP, (LONG_PTR)&Item);
							UINT SelectedCodePage = (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, FAD_COMBOBOX_CP, Position.SelectPos);
							// Разрешаем отмечать только стандартные и любимые таблицы символов
							int FavoritesIndex = 2 + StandardCPCount + 2;

							if (Position.SelectPos > 1 && Position.SelectPos < FavoritesIndex + (favoriteCodePages ? favoriteCodePages + 1 : 0))
							{
								// Преобразуем номер таблицы сиволов к строке
								FARString strCodePageName;
								strCodePageName.Format(L"%u", SelectedCodePage);
								// Получаем текущее состояние флага в реестре
								int SelectType = 0;
								GetRegKey(FavoriteCodePagesKey, strCodePageName, SelectType, 0);

								// Отмечаем/разотмечаем таблицу символов
								if (Item.Item.Flags & LIF_CHECKED)
								{
									// Для стандартных таблиц символов просто удаляем значение из рееста, для
									// любимых же оставляем в реестре флаг, что таблица символов любимая
									if (SelectType & CPST_FAVORITE)
										SetRegKey(FavoriteCodePagesKey, strCodePageName, CPST_FAVORITE);
									else
										DeleteRegValue(FavoriteCodePagesKey, strCodePageName);

									Item.Item.Flags &= ~LIF_CHECKED;
								}
								else
								{
									SetRegKey(FavoriteCodePagesKey, strCodePageName, CPST_FIND | (SelectType & CPST_FAVORITE ?  CPST_FAVORITE : 0));
									Item.Item.Flags |= LIF_CHECKED;
								}

								// Обновляем текущий элемент в выпадающем списке
								SendDlgMessage(hDlg, DM_LISTUPDATE, FAD_COMBOBOX_CP, (LONG_PTR)&Item);

								if (Position.SelectPos<FavoritesIndex + (favoriteCodePages ? favoriteCodePages + 1 : 0)-2)
								{
									FarListPos Pos={Position.SelectPos+1,Position.TopPos};
									SendDlgMessage(hDlg, DM_LISTSETCURPOS, FAD_COMBOBOX_CP,reinterpret_cast<LONG_PTR>(&Pos));
								}

								// Обрабатываем случай, когда таблица символов может присутствовать, как в стандартных, так и в любимых,
								// т.е. выбор/снятие флага автоматичекски происходуит у обоих элементов
								bool bStandardCodePage = Position.SelectPos < FavoritesIndex;

								for (int Index = bStandardCodePage ? FavoritesIndex : 0; Index < (bStandardCodePage ? FavoritesIndex + favoriteCodePages : FavoritesIndex); Index++)
								{
									// Получаем элемент таблицы симолов
									FarListGetItem CheckItem = { Index };
									SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_CP, (LONG_PTR)&CheckItem);

									// Обрабатываем только таблицы симовлов
									if (!(CheckItem.Item.Flags&LIF_SEPARATOR))
									{
										if (SelectedCodePage == (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, FAD_COMBOBOX_CP, Index))
										{
											if (Item.Item.Flags & LIF_CHECKED)
												CheckItem.Item.Flags |= LIF_CHECKED;
											else
												CheckItem.Item.Flags &= ~LIF_CHECKED;

											SendDlgMessage(hDlg, DM_LISTUPDATE, FAD_COMBOBOX_CP, (LONG_PTR)&CheckItem);
											break;
										}
									}
								}
							}
						}
						break;
					}
				}
				break;
			}

			break;
		}
		case DN_EDITCHANGE:
		{
			FarDialogItem &Item=*reinterpret_cast<FarDialogItem*>(Param2);

			switch (Param1)
			{
				case FAD_EDIT_TEXT:
					{
						// Строка "Содержащих текст"
						if (!v->FindFoldersChanged)
						{
							BOOL Checked = (Item.PtrData && *Item.PtrData)?FALSE:Opt.FindOpt.FindFolders;
							SendDlgMessage(hDlg, DM_SETCHECK, FAD_CHECKBOX_DIRS, Checked?BSTATE_CHECKED:BSTATE_UNCHECKED);
						}

						return TRUE;
					}
					break;

				case FAD_COMBOBOX_CP:
				{
					// Получаем выбранную в выпадающем списке таблицу символов
					CodePage = (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, FAD_COMBOBOX_CP, SendDlgMessage(hDlg, DM_LISTGETCURPOS, FAD_COMBOBOX_CP, 0));
				}
				return TRUE;
				case FAD_COMBOBOX_WHERE:
					{
						v->SearchFromChanged=true;
					}
					return TRUE;
			}
		}
		case DN_HOTKEY:
		{
			if (Param1==FAD_TEXT_TEXTHEX)
			{
				bool Hex=(SendDlgMessage(hDlg,DM_GETCHECK,FAD_CHECKBOX_HEX,0)==BSTATE_CHECKED);
				SendDlgMessage(hDlg,DM_SETFOCUS,Hex?FAD_EDIT_HEX:FAD_EDIT_TEXT,0);
				return FALSE;
			}
		}
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

bool GetPluginFile(size_t ArcIndex, const FAR_FIND_DATA_EX& FindData, const wchar_t *DestPath, FARString &strResultName)
{
	_ALGO(CleverSysLog clv(L"FindFiles::GetPluginFile()"));
	ARCLIST ArcItem;
	itd.GetArcListItem(ArcIndex, ArcItem);
	OpenPluginInfo Info;
	CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin,&Info);
	FARString strSaveDir = NullToEmpty(Info.CurDir);
	AddEndSlash(strSaveDir);
	CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin,L"/",OPM_SILENT);
	//SetPluginDirectory(ArcList[ArcIndex]->strRootPath,hPlugin);
	SetPluginDirectory(FindData.strFileName,ArcItem.hPlugin);
	const wchar_t *lpFileNameToFind = PointToName(FindData.strFileName);
	PluginPanelItem *pItems;
	int nItemsNumber;
	bool nResult=false;

	if (CtrlObject->Plugins.GetFindData(ArcItem.hPlugin,&pItems,&nItemsNumber,OPM_SILENT))
	{
		for (int i=0; i<nItemsNumber; i++)
		{
			PluginPanelItem Item = pItems[i];
			Item.FindData.lpwszFileName=const_cast<LPWSTR>(PointToName(NullToEmpty(pItems[i].FindData.lpwszFileName)));

			if (!StrCmp(lpFileNameToFind,Item.FindData.lpwszFileName))
			{
				nResult=CtrlObject->Plugins.GetFile(ArcItem.hPlugin,&Item,DestPath,strResultName,OPM_SILENT)!=0;
				break;
			}
		}

		CtrlObject->Plugins.FreeFindData(ArcItem.hPlugin,pItems,nItemsNumber);
	}

	CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin,L"/",OPM_SILENT);
	SetPluginDirectory(strSaveDir,ArcItem.hPlugin);
	return nResult;
}

// Алгоритма Бойера-Мура-Хорспула поиска подстроки (Unicode версия)
const int FindStringBMH(const wchar_t* searchBuffer, size_t searchBufferCount)
{
	size_t findStringCount = strFindStr.GetLength();
	const wchar_t *buffer = searchBuffer;
	const wchar_t *findStringLower = CmpCase ? nullptr : findString+findStringCount;
	size_t lastBufferChar = findStringCount-1;

	while (searchBufferCount>=findStringCount)
	{
		for (size_t index = lastBufferChar; buffer[index]==findString[index] || (CmpCase ? 0 : buffer[index]==findStringLower[index]); index--)
			if (!index)
				return static_cast<int>(buffer-searchBuffer);

		size_t offset = skipCharsTable[buffer[lastBufferChar]];
		searchBufferCount -= offset;
		buffer += offset;
	}

	return -1;
}

// Алгоритма Бойера-Мура-Хорспула поиска подстроки (Char версия)
const int FindStringBMH(const unsigned char* searchBuffer, size_t searchBufferCount)
{
	const unsigned char *buffer = searchBuffer;
	size_t lastBufferChar = hexFindStringSize-1;

	while (searchBufferCount>=hexFindStringSize)
	{
		for (size_t index = lastBufferChar; buffer[index]==hexFindString[index]; index--)
			if (!index)
				return static_cast<int>(buffer-searchBuffer);

		size_t offset = skipCharsTable[buffer[lastBufferChar]];
		searchBufferCount -= offset;
		buffer += offset;
	}

	return -1;
}


int LookForString(const wchar_t *Name)
{
#define RETURN(r) { result = (r); goto exit; }
#define CONTINUE(r) { if ((r) || cpIndex==codePagesCount-1) RETURN(r) else continue; }
	// Длина строки поиска
	size_t findStringCount;

	// Если строки поиска пустая, то считаем, что мы всегда что-нибудь найдём
	if (!(findStringCount = strFindStr.GetLength()))
		return (TRUE);

	// Результат поиска
	BOOL result = FALSE;

	File file;
	// Открываем файл
	if(!file.Open(Name, FILE_READ_DATA, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN))
	{
		return FALSE;
	}
	// Количество считанных из файла байт
	DWORD readBlockSize = 0;
	// Количество прочитанных из файла байт
	uint64_t alreadyRead = 0;
	// Смещение на которое мы отступили при переходе между блоками
	int offset=0;

	if (SearchHex)
		offset = (int)hexFindStringSize-1;

	UINT64 FileSize=0;
	file.GetSize(FileSize);

	if (SearchInFirst)
	{
		FileSize=Min(SearchInFirst,FileSize);
	}

	UINT LastPercents=0;

	// Основной цикл чтения из файла

	while (!WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0) && 
		file.Read(readBufferA, (!SearchInFirst || 
			alreadyRead+readBufferSizeA <= SearchInFirst)?readBufferSizeA:static_cast<DWORD>(SearchInFirst-alreadyRead), &readBlockSize))
	{
		UINT Percents=static_cast<UINT>(FileSize?alreadyRead*100/FileSize:0);
		if (Percents!=LastPercents)
		{
			itd.SetPercent(Percents);
			LastPercents=Percents;
		}

		// Увеличиваем счётчик прочитыннх байт
		alreadyRead += readBlockSize;

		// Для hex и обыкновенного поиска разные ветки
		if (SearchHex)
		{
			// Выходим, если ничего не прочитали или прочитали мало
			if (!readBlockSize || readBlockSize<hexFindStringSize)
				RETURN(FALSE)

				// Ищем
				if (FindStringBMH((unsigned char *)readBufferA, readBlockSize)!=-1)
					RETURN(TRUE)
				}
		else
		{
			for (int cpIndex = 0; cpIndex<codePagesCount; cpIndex++)
			{
				// Информация о кодовой странице
				CodePageInfo *cpi = codePages+cpIndex;

				// Пропускаем ошибочные кодовые страницы
				if (!cpi->MaxCharSize)
					CONTINUE(FALSE)

					// Если начало файла очищаем информацию о поиске по словам
					if (WholeWords && alreadyRead==readBlockSize)
					{
						cpi->WordFound = false;
						cpi->LastSymbol = 0;
					}

				// Если ничего не прочитали
				if (!readBlockSize)
					// Если поиск по словам и в конце предыдущего блока было что-то найдено,
					// то считаем, что нашли то, что нужно
					CONTINUE(WholeWords && cpi->WordFound)

					// Выходим, если прочитали меньше размера строки поиска и нет поиска по словам
					if (readBlockSize < findStringCount && !(WholeWords && cpi->WordFound))
						CONTINUE(FALSE)
						// Количество символов в выходном буфере
						unsigned int bufferCount;

				// Буфер для поиска
				wchar_t *buffer;

				// Перегоняем буфер в UTF-16
				if (IsUnicodeCodePage(cpi->CodePage))
				{
					// Вычисляем размер буфера в UTF-16
					bufferCount = readBlockSize / 2;

					// Выходим, если размер буфера меньше длины строки посика
					if (bufferCount < findStringCount)
						CONTINUE(FALSE)
						

					// Копируем буфер чтения в буфер сравнения
					//todo
					if (cpi->CodePage==CP_REVERSEBOM)
					{
						for (size_t i = 0; i < bufferCount; ++i) {
							const uint16_t v = *(uint16_t *)&readBufferA[i * 2];
							readBuffer[i] = (wchar_t)(((v & 0xff) << 8) | ((v & 0xff00) >> 8));
						}
					}
					else
					{
						for (size_t i = 0; i < bufferCount; ++i) {
							const uint16_t v = *(uint16_t *)&readBufferA[i * 2];
							readBuffer[i] = (wchar_t)v;
						}
					}
					buffer = readBuffer;
				}
				else
				{
					// Конвертируем буфер чтения из кодировки поиска в UTF-16
					bufferCount = WINPORT(MultiByteToWideChar)(
					                  cpi->CodePage,
					                  0,
					                  (char *)readBufferA,
					                  readBlockSize,
					                  readBuffer,
					                  readBufferSize
					              );

					// Выходим, если нам не удалось сконвертировать строку
					if (!bufferCount)
						CONTINUE(FALSE)

						// Если прочитали меньше размера строки поиска и поиска по словам, то проверяем
						// первый символ блока на разделитель и выходим
						// Если у нас поиск по словам и в конце предыдущего блока было вхождение
						if (WholeWords && cpi->WordFound)
						{
							// Если конец файла, то считаем, что есть разделитель в конце
							if (findStringCount-1>=bufferCount)
								RETURN(TRUE)
								// Проверяем первый символ текущего блока с учётом обратного смещения, которое делается
								// при переходе между блоками
								cpi->LastSymbol = readBuffer[findStringCount-1];

							if (IsWordDiv(cpi->LastSymbol))
								RETURN(TRUE)

								// Если размер буфера меньше размера слова, то выходим
								if (readBlockSize < findStringCount)
									CONTINUE(FALSE)
								}

					// Устанавливаем буфер стравнения
					buffer = readBuffer;
				}

				unsigned int index = 0;

				do
				{
					// Ищем подстроку в буфере и возвращаем индекс её начала в случае успеха
					int foundIndex = FindStringBMH(buffer+index, bufferCount-index);

					// Если подстрока не найдена идём на следующий шаг
					if (foundIndex == -1)
						break;

					// Если посдстрока найдена и отключен поиск по словам, то считаем что всё хорошо
					if (!WholeWords)
						RETURN(TRUE)
						// Устанавливаем позицию в исходном буфере
						index += foundIndex;

					// Если идёт поиск по словам, то делаем соответвующие проверки
					bool firstWordDiv = false;

					// Если мы находимся вначале блока
					if (!index)
					{
						// Если мы находимся вначале файла, то считаем, что разделитель есть
						// Если мы находимся вначале блока, то проверяем является
						// или нет последний символ предыдущего блока разделителем
						if (alreadyRead==readBlockSize || IsWordDiv(cpi->LastSymbol))
							firstWordDiv = true;
					}
					else
					{
						// Проверяем является или нет предыдущий найденому символ блока разделителем
						cpi->LastSymbol = buffer[index-1];

						if (IsWordDiv(cpi->LastSymbol))
							firstWordDiv = true;
					}

					// Проверяем разделитель в конце, только если найден разделитель вначале
					if (firstWordDiv)
					{
						// Если блок выбран не до конца
						if (index+findStringCount!=bufferCount)
						{
							// Проверяем является или нет последующий за найденым символ блока разделителем
							cpi->LastSymbol = buffer[index+findStringCount];

							if (IsWordDiv(cpi->LastSymbol))
								RETURN(TRUE)
							}
						else
							cpi->WordFound = true;
					}
				}
				while (++index<=bufferCount-findStringCount);

				// Выходим, если мы вышли за пределы количества байт разрешённых для поиска
				if (SearchInFirst && SearchInFirst>=alreadyRead)
					CONTINUE(FALSE)
					// Запоминаем последний символ блока
					cpi->LastSymbol = buffer[bufferCount-1];
			}

			// Получаем смещение на которое мы отступили при переходе между блоками
			offset = (int)((CodePage==CP_AUTODETECT?sizeof(wchar_t):codePages->MaxCharSize)*(findStringCount-1));
		}

		// Если мы потенциально прочитали не весь файл
		if (readBlockSize==readBufferSizeA)
		{
			// Отступаем назад на длину слова поиска минус 1
			if (!file.SetPointer(-1*offset, nullptr, FILE_CURRENT))
				RETURN(FALSE)
				alreadyRead -= offset;
		}
	}

exit:
	// Закрываем хэндл файла
	file.Close();
	// Возвращаем результат
	return (result);
#undef CONTINUE
#undef RETURN
}

bool IsFileIncluded(PluginPanelItem* FileItem, const wchar_t *FullName, DWORD FileAttr)
{
	bool FileFound=FileMaskForFindFile.Compare(FullName);
	size_t ArcIndex=itd.GetFindFileArcIndex();
	HANDLE hPlugin=INVALID_HANDLE_VALUE;
	if(ArcIndex!=LIST_INDEX_NONE)
	{
		ARCLIST ArcItem;
		itd.GetArcListItem(ArcIndex, ArcItem);
		hPlugin=ArcItem.hPlugin;
	}

	while (FileFound)
	{
		// Если включен режим поиска hex-кодов, тогда папки в поиск не включаем
		if ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) && (!Opt.FindOpt.FindFolders || SearchHex))
			return FALSE;

		if (!strFindStr.IsEmpty() && FileFound)
		{
			FileFound=false;

			if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
				break;

			FARString strSearchFileName;
			bool RemoveTemp=false;

			if (hPlugin != INVALID_HANDLE_VALUE)
			{
				if (!CtrlObject->Plugins.UseFarCommand(hPlugin, PLUGIN_FARGETFILES))
				{
					FARString strTempDir;
					FarMkTempEx(strTempDir); // А проверка на nullptr???
					apiCreateDirectory(strTempDir,nullptr);

					bool GetFileResult=false;
					{
						CriticalSectionLock Lock(PluginCS);
						GetFileResult=CtrlObject->Plugins.GetFile(hPlugin,FileItem,strTempDir,strSearchFileName,OPM_SILENT|OPM_FIND)!=FALSE;
					}
					if (!GetFileResult)
					{
						apiRemoveDirectory(strTempDir);
						break;
					}
					RemoveTemp=true;
				}
				else
				{
					strSearchFileName = strPluginSearchPath + FullName;
				}
			}
			else
			{
				strSearchFileName = FullName;
			}

			if (LookForString(strSearchFileName))
				FileFound=true;

			if (RemoveTemp)
			{
				DeleteFileWithFolder(strSearchFileName);
			}
		}

		break;
	}

	return FileFound;
}

LONG_PTR WINAPI FindDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	Vars* v = reinterpret_cast<Vars*>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
	Dialog* Dlg=reinterpret_cast<Dialog*>(hDlg);
	VMenu *ListBox=Dlg->GetAllItem()[FD_LISTBOX]->ListPtr;

	static bool Recurse=false;
	static DWORD ShowTime=0;

	if(!v->Finalized && !Recurse)
	{
		Recurse=true;
		DWORD Time=WINPORT(GetTickCount)();
		if(Time-ShowTime>RedrawTimeout)
		{
			ShowTime=Time;
			if (!WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0))
			{
				FARString strDataStr;
				strDataStr.Format(MSG(MFindFound), itd.GetFileCount(), itd.GetDirCount());
				SendDlgMessage(hDlg,DM_SETTEXTPTR,2,(LONG_PTR)strDataStr.CPtr());

				FARString strSearchStr;

				if (!strFindStr.IsEmpty())
				{
					FARString strFStr(strFindStr);
					TruncStrFromEnd(strFStr,10);
					FARString strTemp(L" \"");
					strTemp+=strFStr+="\"";
					strSearchStr.Format(MSG(MFindSearchingIn), strTemp.CPtr());
				}
				else
					strSearchStr.Format(MSG(MFindSearchingIn), L"");

				FARString strFM;
				itd.GetFindMessage(strFM);
				SMALL_RECT Rect;
				SendDlgMessage(hDlg, DM_GETITEMPOSITION, FD_TEXT_STATUS, reinterpret_cast<LONG_PTR>(&Rect));
				TruncStrFromCenter(strFM, Rect.Right-Rect.Left+1 - static_cast<int>(strSearchStr.GetLength()) - 1);
				strDataStr=strSearchStr+L" "+strFM;
				SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_TEXT_STATUS, reinterpret_cast<LONG_PTR>(strDataStr.CPtr()));

				strDataStr.Format(L"%3d%%",itd.GetPercent());
				SendDlgMessage(hDlg, DM_SETTEXTPTR,FD_TEXT_STATUS_PERCENTS,reinterpret_cast<LONG_PTR>(strDataStr.CPtr()));

				if (itd.GetLastFoundNumber())
				{
					itd.SetLastFoundNumber(0);

					if (ListBox->UpdateRequired())
						SendDlgMessage(hDlg,DM_SHOWITEM,1,1);
				}
			}
		}
		Recurse=false;
	}

	if(!v->Finalized && WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0))
	{
		FARString strMessage;
		strMessage.Format(MSG(MFindDone),itd.GetFileCount(), itd.GetDirCount());
		SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
		SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_SEPARATOR1, reinterpret_cast<LONG_PTR>(L""));
		SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_TEXT_STATUS, reinterpret_cast<LONG_PTR>(strMessage.CPtr()));
		SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_TEXT_STATUS_PERCENTS, reinterpret_cast<LONG_PTR>(L""));
		SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_BUTTON_STOP, reinterpret_cast<LONG_PTR>(MSG(MFindCancel)));
		SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
		ConsoleTitle::SetFarTitle(strMessage);
		v->Finalized=true;
	}

	switch (Msg)
	{
	case DN_DRAWDIALOGDONE:
		{
			DefDlgProc(hDlg,Msg,Param1,Param2);

			// Переместим фокус на кнопку [Go To]
			if ((itd.GetDirCount() || itd.GetFileCount()) && !v->FindPositionChanged)
			{
				v->FindPositionChanged=true;
				SendDlgMessage(hDlg,DM_SETFOCUS,FD_BUTTON_GOTO,0);
			}
			return TRUE;
		}
		break;

	case DN_KEY:
		{
			switch (Param2)
			{
			case KEY_ESC:
			case KEY_F10:
				{
					if (!WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0))
					{
						WINPORT(InterlockedExchange)(&PauseFlag, 1);
						bool LocalRes=true;
						if (Opt.Confirm.Esc)
							LocalRes=AbortMessage()!=0;
						WINPORT(InterlockedExchange)(&PauseFlag, 0);
						if(LocalRes)
						{
							WINPORT(InterlockedExchange)(&StopFlag, 1);
						}
						return TRUE;
					}
				}
				break;

			case KEY_CTRLALTSHIFTPRESS:
			case KEY_ALTF9:
			case KEY_F11:
			case KEY_CTRLW:
				{
					FrameManager->ProcessKey((DWORD)Param2);
					return TRUE;
				}
				break;

			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_TAB:
				{
					if (Param1==FD_BUTTON_STOP)
					{
						v->FindPositionChanged=true;
						SendDlgMessage(hDlg,DM_SETFOCUS,FD_BUTTON_NEW,0);
						return TRUE;
					}
				}
				break;

			case KEY_LEFT:
			case KEY_NUMPAD4:
			case KEY_SHIFTTAB:
				{
					if (Param1==FD_BUTTON_NEW)
					{
						v->FindPositionChanged=true;
						SendDlgMessage(hDlg,DM_SETFOCUS,FD_BUTTON_STOP,0);
						return TRUE;
					}
				}
				break;

			case KEY_UP:
			case KEY_DOWN:
			case KEY_NUMPAD8:
			case KEY_NUMPAD2:
			case KEY_PGUP:
			case KEY_PGDN:
			case KEY_NUMPAD9:
			case KEY_NUMPAD3:
			case KEY_HOME:
			case KEY_END:
			case KEY_NUMPAD7:
			case KEY_NUMPAD1:
			case KEY_MSWHEEL_UP:
			case KEY_MSWHEEL_DOWN:
			case KEY_ALTLEFT:
			case KEY_ALT|KEY_NUMPAD4:
			case KEY_MSWHEEL_LEFT:
			case KEY_ALTRIGHT:
			case KEY_ALT|KEY_NUMPAD6:
			case KEY_MSWHEEL_RIGHT:
			case KEY_ALTSHIFTLEFT:
			case KEY_ALT|KEY_SHIFT|KEY_NUMPAD4:
			case KEY_ALTSHIFTRIGHT:
			case KEY_ALT|KEY_SHIFT|KEY_NUMPAD6:
			case KEY_ALTHOME:
			case KEY_ALT|KEY_NUMPAD7:
			case KEY_ALTEND:
			case KEY_ALT|KEY_NUMPAD1:
				{
					ListBox->ProcessKey((int)Param2);
					return TRUE;
				}
				break;

			/*
			case KEY_CTRLA:
			{
				if (!ListBox->GetItemCount())
				{
					return TRUE;
				}

				size_t ItemIndex = reinterpret_cast<size_t>(ListBox->GetUserData(nullptr,0));

				FINDLIST FindItem;
				itd.GetFindListItem(ItemIndex, FindItem);

				if (ShellSetFileAttributes(NULL,FindItem.FindData.strFileName))
				{
					itd.SetFindListItem(ItemIndex, FindItem);
					SendDlgMessage(hDlg,DM_REDRAW,0,0);
				}
				return TRUE;
			}
			*/

			case KEY_F3:
			case KEY_NUMPAD5:
			case KEY_SHIFTNUMPAD5:
			case KEY_F4:
				{
					if (!ListBox->GetItemCount())
					{
						return TRUE;
					}

					size_t ItemIndex = reinterpret_cast<size_t>(ListBox->GetUserData(nullptr,0));
					bool RemoveTemp=false;
					// Плагины надо закрывать, если открыли.
					bool ClosePlugin=false;
					FARString strSearchFileName;
					FARString strTempDir;

					FINDLIST FindItem;
					itd.GetFindListItem(ItemIndex, FindItem);
					if (FindItem.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						return TRUE;
					}

					// FindFileArcIndex нельзя здесь использовать
					// Он может быть уже другой.
					if(FindItem.ArcIndex != LIST_INDEX_NONE)
					{
						ARCLIST ArcItem;
						itd.GetArcListItem(FindItem.ArcIndex, ArcItem);

						if(!(ArcItem.Flags & OPIF_REALNAMES))
						{
							FARString strFindArcName = ArcItem.strArcName;
							if(ArcItem.hPlugin == INVALID_HANDLE_VALUE)
							{
								int SavePluginsOutput=DisablePluginsOutput;
								DisablePluginsOutput=TRUE;
								{
									CriticalSectionLock Lock(PluginCS);
									ArcItem.hPlugin = CtrlObject->Plugins.OpenFilePlugin(strFindArcName, 0, OFP_SEARCH);
								}
								itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
								DisablePluginsOutput=SavePluginsOutput;

								if (ArcItem.hPlugin == (HANDLE)-2 ||
										ArcItem.hPlugin == INVALID_HANDLE_VALUE)
								{
									ArcItem.hPlugin = INVALID_HANDLE_VALUE;
									itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
									return TRUE;
								}

								ClosePlugin = true;
							}
							FarMkTempEx(strTempDir);
							apiCreateDirectory(strTempDir, nullptr);
							CriticalSectionLock Lock(PluginCS);
							bool bGet=GetPluginFile(FindItem.ArcIndex,FindItem.FindData,strTempDir,strSearchFileName);
							itd.SetFindListItem(ItemIndex, FindItem);
							if (!bGet)
							{
								apiRemoveDirectory(strTempDir);

								if (ClosePlugin)
								{
									CtrlObject->Plugins.ClosePlugin(ArcItem.hPlugin);
									ArcItem.hPlugin = INVALID_HANDLE_VALUE;
									itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
								}
								return FALSE;
							}
							else
							{
								if (ClosePlugin)
								{
									CtrlObject->Plugins.ClosePlugin(ArcItem.hPlugin);
									ArcItem.hPlugin = INVALID_HANDLE_VALUE;
									itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
								}
							}
							RemoveTemp=true;
						}
					}
					else
					{
						strSearchFileName = FindItem.FindData.strFileName;
					}

					DWORD FileAttr=apiGetFileAttributes(strSearchFileName);

					if (FileAttr!=INVALID_FILE_ATTRIBUTES)
					{
						FARString strOldTitle;
						Console.GetTitle(strOldTitle);

						if (Param2==KEY_F3 || Param2==KEY_NUMPAD5 || Param2==KEY_SHIFTNUMPAD5)
						{
							int ListSize=ListBox->GetItemCount();
							NamesList ViewList;

							// Возьмем все файлы, которые имеют реальные имена...
							if (Opt.FindOpt.CollectFiles)
							{
								for (int I=0; I<ListSize; I++)
								{
									FINDLIST FindItem;
									itd.GetFindListItem(reinterpret_cast<size_t>(ListBox->GetUserData(nullptr,0,I)), FindItem);

									bool RealNames=true;
									if(FindItem.ArcIndex != LIST_INDEX_NONE)
									{
										ARCLIST ArcItem;
										itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
										if(!(ArcItem.Flags & OPIF_REALNAMES))
										{
											RealNames=false;
										}
									}

									if (RealNames)
									{
										if (!FindItem.FindData.strFileName.IsEmpty() && !(FindItem.FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
											ViewList.AddName(FindItem.FindData.strFileName);
									}
								}

								FARString strCurDir = FindItem.FindData.strFileName;
								ViewList.SetCurName(strCurDir);
							}

							SendDlgMessage(hDlg,DM_SHOWDIALOG,FALSE,0);
							SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
							{
								FileViewer ShellViewer(strSearchFileName,FALSE,FALSE,FALSE,-1,nullptr,(FindItem.ArcIndex != LIST_INDEX_NONE)?nullptr:(Opt.FindOpt.CollectFiles?&ViewList:nullptr));
								ShellViewer.SetDynamicallyBorn(FALSE);
								ShellViewer.SetEnableF6(TRUE);

								// FindFileArcIndex нельзя здесь использовать
								// Он может быть уже другой.
								if(FindItem.ArcIndex != LIST_INDEX_NONE)
								{
									ARCLIST ArcItem;
									itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
									if(!(ArcItem.Flags & OPIF_REALNAMES))
									{
										ShellViewer.SetSaveToSaveAs(true);
									}
								}
								FrameManager->EnterModalEV();
								FrameManager->ExecuteModal();
								FrameManager->ExitModalEV();
								// заставляем рефрешится экран
								FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
							}
							SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
							SendDlgMessage(hDlg,DM_SHOWDIALOG,TRUE,0);
						}
						else
						{
							SendDlgMessage(hDlg,DM_SHOWDIALOG,FALSE,0);
							SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
							{
								/* $ 14.08.2002 VVM
								  ! Пока-что запретим из поиска переключаться в активный редактор.
								    К сожалению, манагер на это не способен сейчас
															int FramePos=FrameManager->FindFrameByFile(MODALTYPE_EDITOR,SearchFileName);
															int SwitchTo=FALSE;
															if (FramePos!=-1)
															{
																if (!(*FrameManager)[FramePos]->GetCanLoseFocus(TRUE) ||
																	Opt.Confirm.AllowReedit)
																{
																	char MsgFullFileName[NM];
																	xstrncpy(MsgFullFileName,SearchFileName,sizeof(MsgFullFileName));
																	int MsgCode=Message(0,2,MSG(MFindFileTitle),
																				TruncPathStr(MsgFullFileName,ScrX-16),
																				MSG(MAskReload),
																				MSG(MCurrent),MSG(MNewOpen));
																	if (!MsgCode)
																	{
																		SwitchTo=TRUE;
																	}
																	else if (MsgCode==1)
																	{
																		SwitchTo=FALSE;
																	}
																	else
																	{
																		SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
																		SendDlgMessage(hDlg,DM_SHOWDIALOG,TRUE,0);
																		return TRUE;
																	}
																}
																else
																{
																	SwitchTo=TRUE;
																}
															}
															if (SwitchTo)
															{
																(*FrameManager)[FramePos]->SetCanLoseFocus(FALSE);
																(*FrameManager)[FramePos]->SetDynamicallyBorn(FALSE);
																FrameManager->ActivateFrame(FramePos);
																FrameManager->EnterModalEV();
																FrameManager->ExecuteModal ();
																FrameManager->ExitModalEV();
																// FrameManager->ExecuteNonModal();
																// заставляем рефрешится экран
																FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
															}
															else
								*/
								{
									FileEditor ShellEditor(strSearchFileName,CP_AUTODETECT,0);
									ShellEditor.SetDynamicallyBorn(FALSE);
									ShellEditor.SetEnableF6(TRUE);

									// FindFileArcIndex нельзя здесь использовать
									// Он может быть уже другой.
									if(FindItem.ArcIndex != LIST_INDEX_NONE)
									{
										ARCLIST ArcItem;
										itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
										if(!(ArcItem.Flags & OPIF_REALNAMES))
										{
											ShellEditor.SetSaveToSaveAs(TRUE);
										}
									}
									FrameManager->EnterModalEV();
									FrameManager->ExecuteModal();
									FrameManager->ExitModalEV();
									// заставляем рефрешится экран
									FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
								}
							}
							SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
							SendDlgMessage(hDlg,DM_SHOWDIALOG,TRUE,0);
						}
						Console.SetTitle(strOldTitle);
					}
					if (RemoveTemp)
					{
						DeleteFileWithFolder(strSearchFileName);
					}
					return TRUE;
				}
				break;
			}
		}
		break;

	case DN_BTNCLICK:
		{
			v->FindPositionChanged = true;
			switch (Param1)
			{
			case FD_BUTTON_NEW:
				{
					WINPORT(InterlockedExchange)(&StopFlag, 1);
					return FALSE;
				}
				break;

			case FD_BUTTON_STOP:
				{
					if (!WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0))
					{
						WINPORT(InterlockedExchange)(&StopFlag, 1);
						return TRUE;
					}
					else
					{
						return FALSE;
					}
				}
				break;

			case FD_BUTTON_VIEW:
				{
					FindDlgProc(hDlg,DN_KEY,FD_LISTBOX,KEY_F3);
					return TRUE;
				}
				break;

			case FD_BUTTON_GOTO:
			case FD_BUTTON_PANEL:
				{
					// Переход и посыл на панель будем делать не в диалоге, а после окончания поиска.
					// Иначе возможна ситуация, когда мы ищем на панели, потом ее грохаем и создаем новую
					// (а поиск-то идет!) и в результате ФАР трапается.
					if(!ListBox->GetItemCount())
					{
						return TRUE;
					}
					v->FindExitIndex = static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(ListBox->GetUserData(nullptr, 0)));
					return FALSE;
				}
				break;
			}
		}
		break;

	case DN_CLOSE:
		{
			BOOL Result = TRUE;
			if (Param1==FD_LISTBOX)
			{
				if(ListBox->GetItemCount())
				{
					FindDlgProc(hDlg,DN_BTNCLICK,FD_BUTTON_GOTO,0); // emulates a [ Go to ] button pressing;
				}
				else
				{
					Result = FALSE;
				}
			}
			if(Result)
			{
				WINPORT(InterlockedExchange)(&StopFlag, 1);
			}
			return Result;
		}
		break;

	case DN_RESIZECONSOLE:
		{
			PCOORD pCoord = reinterpret_cast<PCOORD>(Param2);
			SMALL_RECT DlgRect;
			SendDlgMessage(hDlg, DM_GETDLGRECT, 0, reinterpret_cast<LONG_PTR>(&DlgRect));
			int DlgWidth=DlgRect.Right-DlgRect.Left+1;
			int DlgHeight=DlgRect.Bottom-DlgRect.Top+1;
			int IncX = pCoord->X - DlgWidth - 2;
			int IncY = pCoord->Y - DlgHeight - 2;
			SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);

			for (int i = 0; i <= FD_BUTTON_STOP; i++)
			{
				SendDlgMessage(hDlg, DM_SHOWITEM, i, FALSE);
			}

			if ((IncX > 0) || (IncY > 0))
			{
				pCoord->X = DlgWidth + (IncX > 0 ? IncX : 0);
				pCoord->Y = DlgHeight + (IncY > 0 ? IncY : 0);
				SendDlgMessage(hDlg, DM_RESIZEDIALOG, 0, reinterpret_cast<LONG_PTR>(pCoord));
			}

			DlgWidth += IncX;
			DlgHeight += IncY;

			for (int i = 0; i < FD_SEPARATOR1; i++)
			{
				SMALL_RECT rect;
				SendDlgMessage(hDlg, DM_GETITEMPOSITION, i, reinterpret_cast<LONG_PTR>(&rect));
				rect.Right += IncX;
				rect.Bottom += IncY;
				SendDlgMessage(hDlg, DM_SETITEMPOSITION, i, reinterpret_cast<LONG_PTR>(&rect));
			}

			for (int i = FD_SEPARATOR1; i <= FD_BUTTON_STOP; i++)
			{
				SMALL_RECT rect;
				SendDlgMessage(hDlg, DM_GETITEMPOSITION, i, reinterpret_cast<LONG_PTR>(&rect));

				if (i == FD_TEXT_STATUS)
				{
					rect.Right += IncX;
				}
				else if (i==FD_TEXT_STATUS_PERCENTS)
				{
					rect.Right+=IncX;
					rect.Left+=IncX;
				}

				rect.Top += IncY;
				SendDlgMessage(hDlg, DM_SETITEMPOSITION, i, reinterpret_cast<LONG_PTR>(&rect));
			}

			if ((IncX <= 0) || (IncY <= 0))
			{
				pCoord->X = DlgWidth;
				pCoord->Y = DlgHeight;
				SendDlgMessage(hDlg, DM_RESIZEDIALOG, 0, reinterpret_cast<LONG_PTR>(pCoord));
			}

			for (int i = 0; i <= FD_BUTTON_STOP; i++)
			{
				SendDlgMessage(hDlg, DM_SHOWITEM, i, TRUE);
			}

			SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
			return TRUE;
		}
		break;

	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

void AddMenuRecord(HANDLE hDlg,const wchar_t *FullName, const FAR_FIND_DATA_EX& FindData)
{
	if (!hDlg)
		return;

	VMenu *ListBox=reinterpret_cast<Dialog*>(hDlg)->GetAllItem()[FD_LISTBOX]->ListPtr;

	if(!ListBox->GetItemCount())
	{
		SendDlgMessage(hDlg, DM_ENABLE, FD_BUTTON_GOTO, TRUE);
		SendDlgMessage(hDlg, DM_ENABLE, FD_BUTTON_VIEW, TRUE);
		if(AnySetFindList)
		{
			SendDlgMessage(hDlg, DM_ENABLE, FD_BUTTON_PANEL, TRUE);
		}
		SendDlgMessage(hDlg, DM_ENABLE, FD_LISTBOX, TRUE);
	}

	MenuItemEx ListItem = {};

	FormatString MenuText;

	FARString strDateStr, strTimeStr;
	const wchar_t *DisplayName=FindData.strFileName;

	unsigned int *ColumnType=Opt.FindOpt.OutColumnTypes;
	int *ColumnWidth=Opt.FindOpt.OutColumnWidths;
	int ColumnCount=Opt.FindOpt.OutColumnCount;
	//int *ColumnWidthType=Opt.FindOpt.OutColumnWidthType;

	MenuText << L' ';

	for (int Count=0; Count < ColumnCount; ++Count)
	{
		unsigned int CurColumnType = ColumnType[Count] & 0xFF;

		switch (CurColumnType)
		{
			case DIZ_COLUMN:
			case OWNER_COLUMN:
			{
				// пропускаем, не реализовано
				break;
			}
			case NAME_COLUMN:
			{
				// даже если указали, пропускаем, т.к. поле имени обязательное и идет в конце.
				break;
			}

			case ATTR_COLUMN:
			{
				MenuText << FormatStr_Attribute(FindData.dwFileAttributes, FindData.dwUnixMode) << BoxSymbols[BS_V1];
				break;
			}
			case NUMSTREAMS_COLUMN:
			case STREAMSSIZE_COLUMN:
			case SIZE_COLUMN:
			case PACKED_COLUMN:
			case NUMLINK_COLUMN:
			{
				UINT64 StreamsSize=0;
				DWORD StreamsCount=0;

				MenuText << FormatStr_Size(
								FindData.nFileSize,
								FindData.nPackSize,
								(CurColumnType == NUMSTREAMS_COLUMN || CurColumnType == NUMLINK_COLUMN)?StreamsCount:StreamsSize,
								DisplayName,
								FindData.dwFileAttributes,
								0,
								FindData.dwReserved0,
								(CurColumnType == NUMSTREAMS_COLUMN || CurColumnType == NUMLINK_COLUMN)?STREAMSSIZE_COLUMN:CurColumnType,
								ColumnType[Count],
								ColumnWidth[Count]);

				MenuText << BoxSymbols[BS_V1];
				break;
			}

			case DATE_COLUMN:
			case TIME_COLUMN:
			case WDATE_COLUMN:
			case ADATE_COLUMN:
			case CDATE_COLUMN:
			case CHDATE_COLUMN:
			{
				const FILETIME *FileTime;
				switch (CurColumnType)
				{
					case CDATE_COLUMN:
						FileTime=&FindData.ftCreationTime;
						break;
					case ADATE_COLUMN:
						FileTime=&FindData.ftLastAccessTime;
						break;
					case CHDATE_COLUMN:
						FileTime=&FindData.ftChangeTime;
						break;
					case DATE_COLUMN:
					case TIME_COLUMN:
					case WDATE_COLUMN:
					default:
						FileTime=&FindData.ftLastWriteTime;
						break;
				}

				MenuText << FormatStr_DateTime(FileTime,CurColumnType,ColumnType[Count],ColumnWidth[Count]) << BoxSymbols[BS_V1];
				break;
			}
		}
	}


	// В плагинах принудительно поставим указатель в имени на имя
	// для корректного его отображения в списке, отбросив путь,
	// т.к. некоторые плагины возвращают имя вместе с полным путём,
	// к примеру временная панель.

	const wchar_t *DisplayName0=DisplayName;
	if (itd.GetFindFileArcIndex() != LIST_INDEX_NONE)
		DisplayName0 = PointToName(DisplayName0);
	MenuText << DisplayName0;

	FARString strPathName=FullName;
	{
		size_t pos;

		if (FindLastSlash(pos,strPathName))
			strPathName.SetLength(pos);
		else
			strPathName.Clear();
	}
	AddEndSlash(strPathName);

	if (StrCmpI(strPathName,strLastDirName))
	{
		if (ListBox->GetItemCount())
		{
			ListItem.Flags|=LIF_SEPARATOR;
			ListBox->AddItem(&ListItem);
			ListItem.Flags&=~LIF_SEPARATOR;
		}

		strLastDirName = strPathName;

		if (itd.GetFindFileArcIndex() != LIST_INDEX_NONE)
		{
			ARCLIST ArcItem;
			itd.GetArcListItem(itd.GetFindFileArcIndex(), ArcItem);
			if(!(ArcItem.Flags & OPIF_REALNAMES) && !ArcItem.strArcName.IsEmpty())
			{
				FARString strArcPathName=ArcItem.strArcName;
				strArcPathName+=L":";

				if (!IsSlash(strPathName.At(0)))
					AddEndSlash(strArcPathName);

				strArcPathName+=(!StrCmp(strPathName,L"./")?L"/":strPathName.CPtr());
				strPathName = strArcPathName;
			}
		}
		ListItem.strName = strPathName;
		size_t ItemIndex = itd.AddFindListItem(FindData);

		if (ItemIndex != LIST_INDEX_NONE)
		{
			// Сбросим данные в FindData. Они там от файла
			FINDLIST FindItem;
			itd.GetFindListItem(ItemIndex, FindItem);
			FindItem.FindData.Clear();
			// Используем LastDirName, т.к. PathName уже может быть искажена
			FindItem.FindData.strFileName = strLastDirName;
			// Used=0 - Имя не попададёт во временную панель.
			FindItem.Used=0;
			// Поставим атрибут у каталога, что-бы он не был файлом :)
			FindItem.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

			size_t ArcIndex=itd.GetFindFileArcIndex();
			if (ArcIndex != LIST_INDEX_NONE)
			{
				FindItem.ArcIndex = ArcIndex;
			}
			itd.SetFindListItem(ItemIndex, FindItem);
			ListBox->SetUserData((void*)(DWORD_PTR)ItemIndex,sizeof(ItemIndex),ListBox->AddItem(&ListItem));
		}
	}

	size_t ItemIndex = itd.AddFindListItem(FindData);

	if (ItemIndex != LIST_INDEX_NONE)
	{
		FINDLIST FindItem;
		itd.GetFindListItem(ItemIndex, FindItem);
		FindItem.FindData.strFileName = FullName;
		FindItem.Used=1;
		size_t ArcIndex=itd.GetFindFileArcIndex();
		if (ArcIndex != LIST_INDEX_NONE)
			FindItem.ArcIndex = ArcIndex;
		itd.SetFindListItem(ItemIndex, FindItem);
	}

	ListItem.strName = MenuText.strValue();
	int ListPos = ListBox->AddItem(&ListItem);
	ListBox->SetUserData((void*)(DWORD_PTR)ItemIndex,sizeof(ItemIndex), ListPos);

	// Выделим как положено - в списке.
	int FC=itd.GetFileCount(), DC=itd.GetDirCount(), LF=itd.GetLastFoundNumber();
	if (!FC && !DC)
	{
		ListBox->SetSelectPos(ListPos, -1);
	}

	if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		DC++;
	}
	else
	{
		FC++;
	}

	LF++;

	itd.SetFileCount(FC);
	itd.SetDirCount(DC);
	itd.SetLastFoundNumber(LF);
}

void AddMenuRecord(HANDLE hDlg,const wchar_t *FullName, const FAR_FIND_DATA& FindData)
{
	FAR_FIND_DATA_EX fdata;
	apiFindDataToDataEx(&FindData, &fdata);
	AddMenuRecord(hDlg,FullName, fdata);
}

void DoPreparePluginList(HANDLE hDlg, bool Internal);

void ArchiveSearch(HANDLE hDlg, const wchar_t *ArcName)
{
	_ALGO(CleverSysLog clv(L"FindFiles::ArchiveSearch()"));
	_ALGO(SysLog(L"ArcName='%ls'",(ArcName?ArcName:L"nullptr")));

	int SavePluginsOutput=DisablePluginsOutput;
	DisablePluginsOutput=TRUE;
	FARString strArcName = ArcName;
	HANDLE hArc=CtrlObject->Plugins.OpenFilePlugin(strArcName, OPM_FIND, OFP_SEARCH);
	DisablePluginsOutput=SavePluginsOutput;

	if (hArc==(HANDLE)-2)
	{
		WINPORT(InterlockedExchange)(&StopFlag, 1);
		_ALGO(SysLog(L"return: hArc==(HANDLE)-2"));
		return;
	}

	if (hArc==INVALID_HANDLE_VALUE)
	{
		_ALGO(SysLog(L"return: hArc==INVALID_HANDLE_VALUE"));
		return;
	}

	int SaveSearchMode=SearchMode;
	size_t SaveArcIndex = itd.GetFindFileArcIndex();
	{
		int SavePluginsOutput=DisablePluginsOutput;
		DisablePluginsOutput=TRUE;

		SearchMode=FINDAREA_FROM_CURRENT;
		OpenPluginInfo Info;
		CtrlObject->Plugins.GetOpenPluginInfo(hArc,&Info);
		itd.SetFindFileArcIndex(itd.AddArcListItem(ArcName, hArc, Info.Flags, Info.CurDir));
		// Запомним каталог перед поиском в архиве. И если ничего не нашли - не рисуем его снова.
		{
			FARString strSaveDirName, strSaveSearchPath;
			size_t SaveListCount = itd.GetFindListCount();
			// Запомним пути поиска в плагине, они могут измениться.
			strSaveSearchPath = strPluginSearchPath;
			strSaveDirName = strLastDirName;
			strLastDirName.Clear();
			DoPreparePluginList(hDlg,true);
			strPluginSearchPath = strSaveSearchPath;
			ARCLIST ArcItem;
			itd.GetArcListItem(itd.GetFindFileArcIndex(), ArcItem);
			{
				CriticalSectionLock Lock(PluginCS);
				CtrlObject->Plugins.ClosePlugin(ArcItem.hPlugin);
			}
			ArcItem.hPlugin = INVALID_HANDLE_VALUE;
			itd.SetArcListItem(itd.GetFindFileArcIndex(), ArcItem);

			if (SaveListCount == itd.GetFindListCount())
				strLastDirName = strSaveDirName;
		}

		DisablePluginsOutput=SavePluginsOutput;
	}
	itd.SetFindFileArcIndex(SaveArcIndex);
	SearchMode=SaveSearchMode;
}

void DoScanTree(HANDLE hDlg, FARString& strRoot)
{
	ScanTree ScTree(FALSE,!(SearchMode==FINDAREA_CURRENT_ONLY||SearchMode==FINDAREA_INPATH),Opt.FindOpt.FindSymLinks);
	FARString strSelName;
	DWORD FileAttr;

	if (SearchMode==FINDAREA_SELECTED)
		CtrlObject->Cp()->ActivePanel->GetSelNameCompat(nullptr,FileAttr);

	while (!WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0))
	{
		FARString strCurRoot;

		if (SearchMode==FINDAREA_SELECTED)
		{
			if (!CtrlObject->Cp()->ActivePanel->GetSelNameCompat(&strSelName,FileAttr))
				break;

			if (!(FileAttr & FILE_ATTRIBUTE_DIRECTORY) || TestParentFolderName(strSelName) || !StrCmp(strSelName,L"."))
				continue;

			strCurRoot = strRoot;
			AddEndSlash(strCurRoot);
			strCurRoot += strSelName;
		}
		else
		{
			strCurRoot = strRoot;
		}

		ScTree.SetFindPath(strCurRoot,L"*");
		itd.SetFindMessage(strCurRoot);
		FAR_FIND_DATA_EX FindData;
		FARString strFullName;

		while (!WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0) && ScTree.GetNextName(&FindData,strFullName))
		{
			//WINPORT(Sleep)(0);
			while (WINPORT(InterlockedCompareExchange)(&PauseFlag, 0, 0)) WINPORT(Sleep)(10);

			bool bContinue=false;
			HANDLE hFindStream=INVALID_HANDLE_VALUE;
			bool FirstCall=true;
			FARString strFindDataFileName=FindData.strFileName;

			while (!WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0))
			{
				FARString strFullStreamName=strFullName;
				if (UseFilter)
				{
					enumFileInFilterType foundType;

					if (!Filter->FileInFilter(FindData,&foundType))
					{
						// сюда заходим, если не попали в фильтр или попали в Exclude-фильтр
						if ((FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && foundType==FIFT_EXCLUDE)
							ScTree.SkipDir(); // скипаем только по Exclude-фильтру, т.к. глубже тоже нужно просмотреть

						{
							bContinue=true;
							break;
						}
					}
				}

				if (((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strFindStr.IsEmpty()) ||
				        (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !strFindStr.IsEmpty()))
				{
					itd.SetFindMessage(strFullName);
				}

				if (IsFileIncluded(nullptr,strFullStreamName,FindData.dwFileAttributes))
				{
					if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
/*						if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) || (FindData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE))
						{
							apiGetCompressedFileSize(strFullStreamName,FindData.nPackSize);
						}
						else */FindData.nPackSize=FindData.nFileSize;
					}
					AddMenuRecord(hDlg,strFullStreamName, FindData);
				}

				break;
			}

			if (bContinue)
			{
				continue;
			}

			if (SearchInArchives)
				ArchiveSearch(hDlg,strFullName);
		}

		if (SearchMode!=FINDAREA_SELECTED)
			break;
	}
}

void ScanPluginTree(HANDLE hDlg, HANDLE hPlugin, DWORD Flags, int& RecurseLevel)
{
	PluginPanelItem *PanelData=nullptr;
	int ItemCount=0;
	bool GetFindDataResult=false;
	{
		CriticalSectionLock Lock(PluginCS);
		{
			if(!WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0))
			{
				GetFindDataResult=CtrlObject->Plugins.GetFindData(hPlugin,&PanelData,&ItemCount,OPM_FIND)!=FALSE;
			}
		}
	}
	if (!GetFindDataResult)
	{
		return;
	}

	RecurseLevel++;

	if (SearchMode!=FINDAREA_SELECTED || RecurseLevel!=1)
	{
		for (int I=0; I<ItemCount && !WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0); I++)
		{
			//WINPORT(Sleep)(0);
			while (WINPORT(InterlockedCompareExchange)(&PauseFlag, 0, 0))WINPORT(Sleep)(10);

			PluginPanelItem *CurPanelItem=PanelData+I;
			FARString strCurName=CurPanelItem->FindData.lpwszFileName;
			FARString strFullName;

			if (!StrCmp(strCurName,L".") || TestParentFolderName(strCurName))
				continue;

			strFullName = strPluginSearchPath;
			strFullName += strCurName;

			if (!UseFilter || Filter->FileInFilter(CurPanelItem->FindData))
			{
				if (((CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strFindStr.IsEmpty()) ||
				        (!(CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !strFindStr.IsEmpty()))
				{
					itd.SetFindMessage(strFullName);
				}

				if (IsFileIncluded(CurPanelItem,strCurName,CurPanelItem->FindData.dwFileAttributes))
					AddMenuRecord(hDlg,strFullName, CurPanelItem->FindData);

				if (SearchInArchives && (hPlugin != INVALID_HANDLE_VALUE) && (Flags & OPIF_REALNAMES))
					ArchiveSearch(hDlg,strFullName);
			}
		}
	}

	if (SearchMode!=FINDAREA_CURRENT_ONLY)
	{
		for (int I=0; I<ItemCount && !WINPORT(InterlockedCompareExchange)(&StopFlag, 0, 0); I++)
		{
			PluginPanelItem *CurPanelItem=PanelData+I;
			FARString strCurName=CurPanelItem->FindData.lpwszFileName;

			if ((CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			        StrCmp(strCurName,L".") && !TestParentFolderName(strCurName) &&
			        (!UseFilter || Filter->FileInFilter(CurPanelItem->FindData)) &&
			        (SearchMode!=FINDAREA_SELECTED || RecurseLevel!=1 ||
			         CtrlObject->Cp()->ActivePanel->IsSelected(strCurName)))
			{
				bool SetDirectoryResult=false;
				{
					CriticalSectionLock Lock(PluginCS);
					SetDirectoryResult=CtrlObject->Plugins.SetDirectory(hPlugin,strCurName,OPM_FIND)!=FALSE;
				}
				if (SetDirectoryResult)
				{
					strPluginSearchPath += strCurName;
					strPluginSearchPath += L"/";
					ScanPluginTree(hDlg, hPlugin, Flags, RecurseLevel);

					size_t pos=0;
					if (strPluginSearchPath.RPos(pos, GOOD_SLASH))
						strPluginSearchPath.SetLength(pos);

					if (strPluginSearchPath.RPos(pos, GOOD_SLASH))
						strPluginSearchPath.SetLength(pos+1);
					else
						strPluginSearchPath.Clear();

					bool SetDirectoryResult=false;
					{
						CriticalSectionLock Lock(PluginCS);
						SetDirectoryResult=CtrlObject->Plugins.SetDirectory(hPlugin,L"..",OPM_FIND)!=FALSE;
					}
					if (!SetDirectoryResult)
					{
						WINPORT(InterlockedExchange)(&StopFlag, 1);
					}
				}
			}
		}
	}

	CtrlObject->Plugins.FreeFindData(hPlugin,PanelData,ItemCount);
	RecurseLevel--;
}

void DoPrepareFileList(HANDLE hDlg)
{
	FARString strRoot;
	CtrlObject->CmdLine->GetCurDir(strRoot);

	UserDefinedList List(L';',L';',ULF_UNIQUE);

	if (SearchMode==FINDAREA_INPATH)
	{
		FARString strPathEnv;
		apiGetEnvironmentVariable(L"PATH",strPathEnv);
		List.Set(strPathEnv);
	}
	else if (SearchMode==FINDAREA_ROOT)
	{
		GetPathRoot(strRoot,strRoot);
		List.Set(strRoot);
	}
	else if (SearchMode==FINDAREA_ALL || SearchMode==FINDAREA_ALL_BUTNETWORK)
	{
		DList<FARString> Volumes; List.AddItem(L"/");
	}
	else
	{
		List.Set(strRoot);
	}

	while(!List.IsEmpty())
	{
		strRoot = List.GetNext();
		DoScanTree(hDlg, strRoot);
	}

	itd.SetPercent(0);
	WINPORT(InterlockedExchange)(&StopFlag, 1);
}

void DoPreparePluginList(HANDLE hDlg, bool Internal)
{
	ARCLIST ArcItem;
	itd.GetArcListItem(itd.GetFindFileArcIndex(), ArcItem);
	OpenPluginInfo Info;
	FARString strSaveDir;
	{
		CriticalSectionLock Lock(PluginCS);
		CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin,&Info);
		strSaveDir = Info.CurDir;
		if (SearchMode==FINDAREA_ROOT || SearchMode==FINDAREA_ALL || SearchMode==FINDAREA_ALL_BUTNETWORK || SearchMode==FINDAREA_INPATH)
		{
			CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin,L"/",OPM_FIND);
			CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin,&Info);
		}
	}

	strPluginSearchPath=Info.CurDir;

	if (!strPluginSearchPath.IsEmpty())
		AddEndSlash(strPluginSearchPath);

	int RecurseLevel=0;
	ScanPluginTree(hDlg,ArcItem.hPlugin,ArcItem.Flags, RecurseLevel);

	if (SearchMode==FINDAREA_ROOT || SearchMode==FINDAREA_ALL || SearchMode==FINDAREA_ALL_BUTNETWORK || SearchMode==FINDAREA_INPATH)
	{
		CriticalSectionLock Lock(PluginCS);
		CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin,strSaveDir,OPM_FIND);
	}

	if (!Internal)
	{
		itd.SetPercent(0);
		WINPORT(InterlockedExchange)(&StopFlag, 1);
	}
}

struct THREADPARAM
{
	bool PluginMode;
	HANDLE hDlg;
};

DWORD ThreadRoutine(LPVOID Param)
{
	InitInFileSearch();
	THREADPARAM* tParam=reinterpret_cast<THREADPARAM*>(Param);
	tParam->PluginMode?DoPreparePluginList(tParam->hDlg, false):DoPrepareFileList(tParam->hDlg);
	ReleaseInFileSearch();
	return 0;
}

bool FindFilesProcess(Vars& v)
{
	_ALGO(CleverSysLog clv(L"FindFiles::FindFilesProcess()"));
	// Если используется фильтр операций, то во время поиска сообщаем об этом
	FARString strTitle=MSG(MFindFileTitle);
	FARString strSearchStr;

	itd.Init();

	if (!strFindMask.IsEmpty())
	{
		strTitle+=L": ";
		strTitle+=strFindMask;

		if (UseFilter)
		{
			strTitle+=L" (";
			strTitle+=MSG(MFindUsingFilter);
			strTitle+=L")";
		}
	}
	else
	{
		if (UseFilter)
		{
			strTitle+=L" (";
			strTitle+=MSG(MFindUsingFilter);
			strTitle+=L")";
		}
	}

	if (!strFindStr.IsEmpty())
	{
		FARString strFStr=strFindStr;
		TruncStrFromEnd(strFStr,10);
		InsertQuote(strFStr);
		FARString strTemp=L" ";
		strTemp+=strFStr;
		strSearchStr.Format(MSG(MFindSearchingIn),strTemp.CPtr());
	}
	else
	{
		strSearchStr.Format(MSG(MFindSearchingIn), L"");
	}

	int DlgWidth = ScrX + 1 - 2;
	int DlgHeight = ScrY + 1 - 2;
	DialogDataEx FindDlgData[]=
	{
		DI_DOUBLEBOX,3,1,(short)(DlgWidth-4),(short)(DlgHeight-2),0,DIF_SHOWAMPERSAND,strTitle,
		DI_LISTBOX,4,2,(short)(DlgWidth-5),(short)(DlgHeight-7),0,DIF_LISTNOBOX|DIF_DISABLE,0,
		DI_TEXT,0,(short)(DlgHeight-6),0,(short)(DlgHeight-6),0,DIF_SEPARATOR2,L"",
		DI_TEXT,5,(short)(DlgHeight-5),(short)(DlgWidth-(strFindStr.IsEmpty()?6:12)),(short)(DlgHeight-5),0,DIF_SHOWAMPERSAND,strSearchStr,
		DI_TEXT,(short)(DlgWidth-9),(short)(DlgHeight-5),(short)(DlgWidth-6),(short)(DlgHeight-5),0,(strFindStr.IsEmpty()?DIF_HIDDEN:0),L"",
		DI_TEXT,0,(short)(DlgHeight-4),0,(short)(DlgHeight-4),0,DIF_SEPARATOR,L"",
		DI_BUTTON,0,(short)(DlgHeight-3),0,(short)(DlgHeight-3),0,DIF_FOCUS|DIF_DEFAULT|DIF_CENTERGROUP,MSG(MFindNewSearch),
		DI_BUTTON,0,(short)(DlgHeight-3),0,(short)(DlgHeight-3),0,DIF_CENTERGROUP|DIF_DISABLE,MSG(MFindGoTo),
		DI_BUTTON,0,(short)(DlgHeight-3),0,(short)(DlgHeight-3),0,DIF_CENTERGROUP|DIF_DISABLE,MSG(MFindView),
		DI_BUTTON,0,(short)(DlgHeight-3),0,(short)(DlgHeight-3),0,DIF_CENTERGROUP|DIF_DISABLE,MSG(MFindPanel),
		DI_BUTTON,0,(short)(DlgHeight-3),0,(short)(DlgHeight-3),0,DIF_CENTERGROUP,MSG(MFindStop),
	};
	MakeDialogItemsEx(FindDlgData,FindDlg);
	ChangePriority ChPriority(ChangePriority::NORMAL);

	if (v.PluginMode)
	{
		Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
		HANDLE hPlugin=ActivePanel->GetPluginHandle();
		OpenPluginInfo Info;
		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
		itd.SetFindFileArcIndex(itd.AddArcListItem(Info.HostFile, hPlugin, Info.Flags, Info.CurDir));

		if (itd.GetFindFileArcIndex() == LIST_INDEX_NONE)
			return false;

		if (!(Info.Flags & OPIF_REALNAMES))
		{
			FindDlg[FD_BUTTON_PANEL].Type=DI_TEXT;
			FindDlg[FD_BUTTON_PANEL].strData.Clear();
		}
	}

	AnySetFindList = false;
	for (int i=0; i<CtrlObject->Plugins.GetPluginsCount(); i++)
	{
		if (CtrlObject->Plugins.GetPlugin(i)->HasSetFindList())
		{
			AnySetFindList=true;
			break;
		}
	}

	if (!AnySetFindList)
	{
		FindDlg[FD_BUTTON_PANEL].Flags|=DIF_DISABLE;
	}

	Dialog Dlg=Dialog(FindDlg,ARRAYSIZE(FindDlg),FindDlgProc, reinterpret_cast<LONG_PTR>(&v));
//  pDlg->SetDynamicallyBorn();
	Dlg.SetHelp(L"FindFileResult");
	Dlg.SetPosition(-1, -1, DlgWidth, DlgHeight);
	// Надо бы показать диалог, а то инициализация элементов запаздывает
	// иногда при поиске и первые элементы не добавляются
	Dlg.InitDialog();
	Dlg.Show();

	strLastDirName.Clear();

	THREADPARAM Param={v.PluginMode,reinterpret_cast<HANDLE>(&Dlg)};
	HANDLE Thread = WINPORT(CreateThread)(nullptr, 0, ThreadRoutine, &Param, 0, nullptr);
	if (Thread)
	{
		wakeful W;
		Dlg.Process();
		WINPORT(WaitForSingleObject)(Thread,INFINITE);
		WINPORT(CloseHandle)(Thread);

		WINPORT(InterlockedExchange)(&PauseFlag, 0);
		WINPORT(InterlockedExchange)(&StopFlag, 0);

		switch (Dlg.GetExitCode())
		{
			case FD_BUTTON_NEW:
			{
				return true;
			}

			case FD_BUTTON_PANEL:
			// Отработаем переброску на временную панель
			{
				size_t ListSize = itd.GetFindListCount();
				PluginPanelItem *PanelItems=new PluginPanelItem[ListSize];

				if (!PanelItems)
					ListSize=0;

				int ItemsNumber=0;

				for (size_t i=0; i<ListSize; i++)
				{
					FINDLIST FindItem;
					itd.GetFindListItem(i, FindItem);
					if (!FindItem.FindData.strFileName.IsEmpty() && FindItem.Used)
					// Добавляем всегда, если имя задано
					{
						// Для плагинов с виртуальными именами заменим имя файла на имя архива.
						// панель сама уберет лишние дубли.
						bool IsArchive=false;
						if(FindItem.ArcIndex != LIST_INDEX_NONE)
						{
							ARCLIST ArcItem;
							itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
							if(!(ArcItem.Flags&OPIF_REALNAMES))
							{
								IsArchive=true;
							}
						}
						// Добавляем только файлы или имена архивов или папки когда просили
						if (IsArchive || (Opt.FindOpt.FindFolders && !SearchHex) ||
							    !(FindItem.FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
						{
							if (IsArchive)
							{
								ARCLIST ArcItem;
								itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
								FindItem.FindData.strFileName = ArcItem.strArcName;
								itd.SetFindListItem(i, FindItem);
							}
							PluginPanelItem *pi=&PanelItems[ItemsNumber++];
							memset(pi,0,sizeof(*pi));
							apiFindDataExToData(&FindItem.FindData, &pi->FindData);

							if (IsArchive)
								pi->FindData.dwFileAttributes = 0;

							if (pi->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							{
								DeleteEndSlash(pi->FindData.lpwszFileName);
							}
						}
					}
				}

				HANDLE hNewPlugin=CtrlObject->Plugins.OpenFindListPlugin(PanelItems,ItemsNumber);

				if (hNewPlugin!=INVALID_HANDLE_VALUE)
				{
					Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
					Panel *NewPanel=CtrlObject->Cp()->ChangePanel(ActivePanel,FILE_PANEL,TRUE,TRUE);
					NewPanel->SetPluginMode(hNewPlugin,L"",true);
					NewPanel->SetVisible(TRUE);
					NewPanel->Update(0);
					//if (FindExitIndex != LIST_INDEX_NONE)
					//NewPanel->GoToFile(FindList[FindExitIndex].FindData.cFileName);
					NewPanel->Show();
				}

				for (int i = 0; i < ItemsNumber; i++)
					apiFreeFindData(&PanelItems[i].FindData);

				delete[] PanelItems;
				break;
			}
			case FD_BUTTON_GOTO:
			case FD_LISTBOX:
			{
				FINDLIST FindItem;
				itd.GetFindListItem(v.FindExitIndex, FindItem);
				FARString strFileName=FindItem.FindData.strFileName;
				Panel *FindPanel=CtrlObject->Cp()->ActivePanel;

				if (FindItem.ArcIndex != LIST_INDEX_NONE)
				{
					ARCLIST ArcItem;
					itd.GetArcListItem(FindItem.ArcIndex, ArcItem);

					if (ArcItem.hPlugin == INVALID_HANDLE_VALUE)
					{
						FARString strArcName = ArcItem.strArcName;

						if (FindPanel->GetType()!=FILE_PANEL)
						{
							FindPanel=CtrlObject->Cp()->ChangePanel(FindPanel,FILE_PANEL,TRUE,TRUE);
						}

						FARString strArcPath=strArcName;
						CutToSlash(strArcPath);
						FindPanel->SetCurDir(strArcPath,TRUE);
						ArcItem.hPlugin=((FileList *)FindPanel)->OpenFilePlugin(strArcName,FALSE, OFP_SEARCH);
						if (ArcItem.hPlugin==(HANDLE)-2)
							ArcItem.hPlugin = INVALID_HANDLE_VALUE;
						itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
					}

					if (ArcItem.hPlugin != INVALID_HANDLE_VALUE)
					{
						OpenPluginInfo Info;
						CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin,&Info);

						if (SearchMode==FINDAREA_ROOT ||
							    SearchMode==FINDAREA_ALL ||
							    SearchMode==FINDAREA_ALL_BUTNETWORK ||
							    SearchMode==FINDAREA_INPATH)
							CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin,L"/",0);

						SetPluginDirectory(strFileName,ArcItem.hPlugin,TRUE);
					}
				}
				else
				{
					FARString strSetName;
					size_t Length=strFileName.GetLength();

					if (!Length)
						break;

					if (Length>1 && IsSlash(strFileName.At(Length-1)) && strFileName.At(Length-2)!=L':')
						strFileName.SetLength(Length-1);

					if ((apiGetFileAttributes(strFileName)==INVALID_FILE_ATTRIBUTES) && (WINPORT(GetLastError)() != ERROR_ACCESS_DENIED))
						break;

					const wchar_t *NamePtr = PointToName(strFileName);
					strSetName = NamePtr;

					strFileName.SetLength(NamePtr-strFileName.CPtr());
					Length=strFileName.GetLength();

					if (Length>1 && IsSlash(strFileName.At(Length-1)) && strFileName.At(Length-2)!=L':')
						strFileName.SetLength(Length-1);

					if (strFileName.IsEmpty())
						break;

					if (FindPanel->GetType()!=FILE_PANEL &&
						    CtrlObject->Cp()->GetAnotherPanel(FindPanel)->GetType()==FILE_PANEL)
						FindPanel=CtrlObject->Cp()->GetAnotherPanel(FindPanel);

					if ((FindPanel->GetType()!=FILE_PANEL) || (FindPanel->GetMode()!=NORMAL_PANEL))
					// Сменим панель на обычную файловую...
					{
						FindPanel=CtrlObject->Cp()->ChangePanel(FindPanel,FILE_PANEL,TRUE,TRUE);
						FindPanel->SetVisible(TRUE);
						FindPanel->Update(0);
					}

					// ! Не меняем каталог, если мы уже в нем находимся.
					// Тем самым добиваемся того, что выделение с элементов панели не сбрасывается.
					FARString strDirTmp;
					FindPanel->GetCurDir(strDirTmp);
					Length=strDirTmp.GetLength();

					if (Length>1 && IsSlash(strDirTmp.At(Length-1)) && strDirTmp.At(Length-2)!=L':')
						strDirTmp.SetLength(Length-1);

					if (StrCmpI(strFileName, strDirTmp))
						FindPanel->SetCurDir(strFileName,TRUE);

					if (!strSetName.IsEmpty())
						FindPanel->GoToFile(strSetName);

					FindPanel->Show();
					FindPanel->SetFocus();
				}
				break;
			}
		}
	}
	return false;
}

FindFiles::FindFiles()
{
	_ALGO(CleverSysLog clv(L"FindFiles::FindFiles()"));
	static FARString strLastFindMask=L"*.*", strLastFindStr;
	// Статической структуре и статические переменные
	static FARString strSearchFromRoot;
	static int LastCmpCase=0,LastWholeWords=0,LastSearchInArchives=0,LastSearchHex=0;
	// Создадим объект фильтра
	Filter=new FileFilter(CtrlObject->Cp()->ActivePanel,FFT_FINDFILE);
	CmpCase=LastCmpCase;
	WholeWords=LastWholeWords;
	SearchInArchives=LastSearchInArchives;
	SearchHex=LastSearchHex;
	SearchMode=Opt.FindOpt.FileSearchMode;
	UseFilter=Opt.FindOpt.UseFilter;
	strFindMask = strLastFindMask;
	strFindStr = strLastFindStr;
	strSearchFromRoot = MSG(MSearchFromRootFolder);

	Vars v;

	do
	{
		v.Clear();
		itd.ClearAllLists();
		Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
		v.PluginMode=ActivePanel->GetMode()==PLUGIN_PANEL && ActivePanel->IsVisible();
		PrepareDriveNameStr(strSearchFromRoot);
		const wchar_t *MasksHistoryName=L"Masks",*TextHistoryName=L"SearchText";
		const wchar_t *HexMask=L"HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH";
		const wchar_t VSeparator[]={BoxSymbols[BS_T_H1V1],BoxSymbols[BS_V1],BoxSymbols[BS_V1],BoxSymbols[BS_V1],BoxSymbols[BS_B_H1V1],0};
		struct DialogDataEx FindAskDlgData[]=
		{
			DI_DOUBLEBOX,3,1,74,18,0,0,MSG(MFindFileTitle),
			DI_TEXT,5,2,0,2,0,0,MSG(MFindFileMasks),
			DI_EDIT,5,3,72,3,(DWORD_PTR)MasksHistoryName,DIF_FOCUS|DIF_HISTORY|DIF_USELASTHISTORY,L"",
			DI_TEXT,3,4,0,4,0,DIF_SEPARATOR,L"",
			DI_TEXT,5,5,0,5,0,0,L"",
			DI_EDIT,5,6,72,6,(DWORD_PTR)TextHistoryName,DIF_HISTORY,L"",
			DI_FIXEDIT,5,6,72,6,(DWORD_PTR)HexMask,DIF_MASKEDIT,L"",
			DI_TEXT,5,7,0,7,0,0,L"",
			DI_COMBOBOX,5,8,72,8,0,DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND,L"",
			DI_TEXT,3,9,0,9,0,DIF_SEPARATOR,L"",
			DI_CHECKBOX,5,10,0,10,0,0,MSG(MFindFileCase),
			DI_CHECKBOX,5,11,0,11,0,0,MSG(MFindFileWholeWords),
			DI_CHECKBOX,5,12,0,12,0,0,MSG(MSearchForHex),
			DI_CHECKBOX,40,10,0,10,0,0,MSG(MFindArchives),
			DI_CHECKBOX,40,11,0,11,0,0,MSG(MFindFolders),
			DI_CHECKBOX,40,12,0,12,0,0,MSG(MFindSymLinks),
			DI_TEXT,3,13,0,13,0,DIF_SEPARATOR,L"",
			DI_VTEXT,38,9,0,9,0,DIF_BOXCOLOR,VSeparator,
			DI_TEXT,5,14,0,14,0,0,MSG(MSearchWhere),
			DI_COMBOBOX,5,15,36,15,0,DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND,L"",
			DI_CHECKBOX,40,15,0,15,UseFilter?BSTATE_CHECKED:BSTATE_UNCHECKED,DIF_AUTOMATION,MSG(MFindUseFilter),
			DI_TEXT,3,16,0,16,0,DIF_SEPARATOR,L"",
			DI_BUTTON,0,17,0,17,0,DIF_DEFAULT|DIF_CENTERGROUP,MSG(MFindFileFind),
			DI_BUTTON,0,17,0,17,0,DIF_CENTERGROUP,MSG(MFindFileDrive),
			DI_BUTTON,0,17,0,17,0,DIF_CENTERGROUP|DIF_AUTOMATION|(UseFilter?0:DIF_DISABLE),MSG(MFindFileSetFilter),
			DI_BUTTON,0,17,0,17,0,DIF_CENTERGROUP,MSG(MFindFileAdvanced),
			DI_BUTTON,0,17,0,17,0,DIF_CENTERGROUP,MSG(MCancel),
		};
		MakeDialogItemsEx(FindAskDlgData,FindAskDlg);

		if (strFindStr.IsEmpty())
			FindAskDlg[FAD_CHECKBOX_DIRS].Selected=Opt.FindOpt.FindFolders;

		FarListItem li[]=
		{
			{0,MSG(MSearchAllDisks)},
			{0,MSG(MSearchAllButNetwork)},
			{0,MSG(MSearchInPATH)},
			{0,strSearchFromRoot},
			{0,MSG(MSearchFromCurrent)},
			{0,MSG(MSearchInCurrent)},
			{0,MSG(MSearchInSelected)},
		};
		li[FADC_ALLDISKS+SearchMode].Flags|=LIF_SELECTED;
		FarList l={ARRAYSIZE(li),li};
		FindAskDlg[FAD_COMBOBOX_WHERE].ListItems=&l;

		if (v.PluginMode)
		{
			OpenPluginInfo Info;
			CtrlObject->Plugins.GetOpenPluginInfo(ActivePanel->GetPluginHandle(),&Info);

			if (!(Info.Flags & OPIF_REALNAMES))
				FindAskDlg[FAD_CHECKBOX_ARC].Flags |= DIF_DISABLE;

			if (FADC_ALLDISKS+SearchMode==FADC_ALLDISKS || FADC_ALLDISKS+SearchMode==FADC_ALLBUTNET)
			{
				li[FADC_ALLDISKS].Flags=0;
				li[FADC_ALLBUTNET].Flags=0;
				li[FADC_ROOT].Flags|=LIF_SELECTED;
			}

			li[FADC_ALLDISKS].Flags|=LIF_GRAYED;
			li[FADC_ALLBUTNET].Flags|=LIF_GRAYED;
			FindAskDlg[FAD_CHECKBOX_LINKS].Selected=0;
			FindAskDlg[FAD_CHECKBOX_LINKS].Flags|=DIF_DISABLE;
		}
		else
			FindAskDlg[FAD_CHECKBOX_LINKS].Selected=Opt.FindOpt.FindSymLinks;

		if (!(FindAskDlg[FAD_CHECKBOX_ARC].Flags & DIF_DISABLE))
			FindAskDlg[FAD_CHECKBOX_ARC].Selected=SearchInArchives;

		FindAskDlg[FAD_EDIT_MASK].strData = strFindMask;

		if (SearchHex)
			FindAskDlg[FAD_EDIT_HEX].strData = strFindStr;
		else
			FindAskDlg[FAD_EDIT_TEXT].strData = strFindStr;

		FindAskDlg[FAD_CHECKBOX_CASE].Selected=CmpCase;
		FindAskDlg[FAD_CHECKBOX_WHOLEWORDS].Selected=WholeWords;
		FindAskDlg[FAD_CHECKBOX_HEX].Selected=SearchHex;
		int ExitCode;
		Dialog Dlg(FindAskDlg,ARRAYSIZE(FindAskDlg),MainDlgProc, reinterpret_cast<LONG_PTR>(&v));
		Dlg.SetAutomation(FAD_CHECKBOX_FILTER,FAD_BUTTON_FILTER,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.SetHelp(L"FindFile");
		Dlg.SetId(FindFileId);
		Dlg.SetPosition(-1,-1,78,20);
		Dlg.Process();
		ExitCode=Dlg.GetExitCode();
		//Рефреш текущему времени для фильтра сразу после выхода из диалога
		Filter->UpdateCurrentTime();

		if (ExitCode!=FAD_BUTTON_FIND)
		{
			return;
		}

		Opt.FindCodePage = CodePage;
		CmpCase=FindAskDlg[FAD_CHECKBOX_CASE].Selected;
		WholeWords=FindAskDlg[FAD_CHECKBOX_WHOLEWORDS].Selected;
		SearchHex=FindAskDlg[FAD_CHECKBOX_HEX].Selected;
		SearchInArchives=FindAskDlg[FAD_CHECKBOX_ARC].Selected;

		if (v.FindFoldersChanged)
		{
			Opt.FindOpt.FindFolders=(FindAskDlg[FAD_CHECKBOX_DIRS].Selected==BSTATE_CHECKED);
		}

		if (!v.PluginMode)
		{
			Opt.FindOpt.FindSymLinks=(FindAskDlg[FAD_CHECKBOX_LINKS].Selected==BSTATE_CHECKED);
		}

		UseFilter=(FindAskDlg[FAD_CHECKBOX_FILTER].Selected==BSTATE_CHECKED);
		Opt.FindOpt.UseFilter=UseFilter;
		strFindMask = !FindAskDlg[FAD_EDIT_MASK].strData.IsEmpty() ? FindAskDlg[FAD_EDIT_MASK].strData:L"*";

		if (SearchHex)
		{
			strFindStr = FindAskDlg[FAD_EDIT_HEX].strData;
			RemoveTrailingSpaces(strFindStr);
		}
		else
			strFindStr = FindAskDlg[FAD_EDIT_TEXT].strData;

		if (!strFindStr.IsEmpty())
		{
			strGlobalSearchString = strFindStr;
			GlobalSearchCase=CmpCase;
			GlobalSearchWholeWords=WholeWords;
			GlobalSearchHex=SearchHex;
		}

		switch (FindAskDlg[FAD_COMBOBOX_WHERE].ListPos)
		{
			case FADC_ALLDISKS:
				SearchMode=FINDAREA_ALL;
				break;
			case FADC_ALLBUTNET:
				SearchMode=FINDAREA_ALL_BUTNETWORK;
				break;
			case FADC_PATH:
				SearchMode=FINDAREA_INPATH;
				break;
			case FADC_ROOT:
				SearchMode=FINDAREA_ROOT;
				break;
			case FADC_FROMCURRENT:
				SearchMode=FINDAREA_FROM_CURRENT;
				break;
			case FADC_INCURRENT:
				SearchMode=FINDAREA_CURRENT_ONLY;
				break;
			case FADC_SELECTED:
				SearchMode=FINDAREA_SELECTED;
				break;
		}

		if (v.SearchFromChanged)
		{
			Opt.FindOpt.FileSearchMode=SearchMode;
		}

		LastCmpCase=CmpCase;
		LastWholeWords=WholeWords;
		LastSearchHex=SearchHex;
		LastSearchInArchives=SearchInArchives;
		strLastFindMask = strFindMask;
		strLastFindStr = strFindStr;

		if (!strFindStr.IsEmpty())
			Editor::SetReplaceMode(FALSE);
	}
	while (FindFilesProcess(v));

	CtrlObject->Cp()->ActivePanel->SetTitle();
}

FindFiles::~FindFiles()
{
	FileMaskForFindFile.Free();
	itd.ClearAllLists();
	ScrBuf.ResetShadow();

	if (Filter)
	{
		delete Filter;
	}
}
