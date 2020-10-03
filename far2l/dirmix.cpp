/*
dirmix.cpp

Misc functions for working with directories
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


#include "dirmix.hpp"
#include "cvtname.hpp"
#include "message.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "lasterror.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "treelist.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "scantree.hpp"
#include "delete.hpp"
#include <atomic>


BOOL FarChDir(const wchar_t *NewDir, BOOL ChangeDir)
{
	if (!NewDir || !*NewDir)
		return FALSE;

	BOOL rc=FALSE;
	FARString strCurDir;

	{
		if (ChangeDir)
		{
			if (*NewDir=='/') {
				strCurDir = NewDir;
				rc = apiSetCurrentDirectory(strCurDir); // здесь берем корень
			} else {
				apiGetCurrentDirectory(strCurDir);
				ConvertNameToFull(NewDir,strCurDir);
				PrepareDiskPath(strCurDir,false); // TRUE ???
				rc = apiSetCurrentDirectory(strCurDir);				
			}
		//fprintf(stderr, "FarChDir: %ls - %u\n", NewDir, rc);
		}
	}

	return rc;
}

/*
  Функция TestFolder возвращает одно состояний тестируемого каталога:

    TSTFLD_NOTFOUND   (2) - нет такого
    TSTFLD_NOTEMPTY   (1) - не пусто
    TSTFLD_EMPTY      (0) - пусто
    TSTFLD_NOTACCESS (-1) - нет доступа
    TSTFLD_ERROR     (-2) - ошибка (кривые параметры или нехватило памяти для выделения промежуточных буферов)
*/
int TestFolder(const wchar_t *Path)
{
	if (!(Path && *Path)) // проверка на вшивость
		return TSTFLD_ERROR;

	FARString strFindPath = Path;
	// сообразим маску для поиска.
	AddEndSlash(strFindPath);
	strFindPath += L"*";

	// первая проверка - че-нить считать можем?
	FAR_FIND_DATA_EX fdata;
	FindFile Find(strFindPath);
	bool bFind = false;
	if(Find.Get(fdata))
	{
		return TSTFLD_NOTEMPTY;
	}
	if (!bFind)
	{
		GuardLastError lstError;

		if (lstError.Get() == ERROR_FILE_NOT_FOUND)
			return TSTFLD_EMPTY;

		// собственно... не факт, что диск не читаем, т.к. на чистом диске в корне нету даже "."
		// поэтому посмотрим на Root
		GetPathRoot(Path,strFindPath);

		if (!StrCmp(Path,strFindPath))
		{
			// проверка атрибутов гарантировано скажет - это бага BugZ#743 или пустой корень диска.
			if (apiGetFileAttributes(strFindPath)!=INVALID_FILE_ATTRIBUTES)
			{
				if (lstError.Get() == ERROR_ACCESS_DENIED)
					return TSTFLD_NOTACCESS;
				return TSTFLD_EMPTY;
			}
		}

		strFindPath = Path;

		if (CheckShortcutFolder(&strFindPath,FALSE,TRUE))
		{
			if (StrCmp(Path,strFindPath))
				return TSTFLD_NOTFOUND;
		}

		return TSTFLD_NOTACCESS;
	}


	// однозначно каталог пуст
	return TSTFLD_EMPTY;
}

/*
   Проверка пути или хост-файла на существование
   Если идет проверка пути (IsHostFile=FALSE), то будет
   предпринята попытка найти ближайший путь. Результат попытки
   возвращается в переданном TestPath.

   Return: 0 - бЯда.
           1 - ОБИ!,
          -1 - Почти что ОБИ, но ProcessPluginEvent вернул TRUE
   TestPath может быть пустым, тогда просто исполним ProcessPluginEvent()

*/
int CheckShortcutFolder(FARString *pTestPath,int IsHostFile, BOOL Silent)
{
	if (pTestPath && !pTestPath->IsEmpty() && apiGetFileAttributes(*pTestPath) == INVALID_FILE_ATTRIBUTES)
	{
		int FoundPath=0;
		FARString strTarget = *pTestPath;
		TruncPathStr(strTarget, ScrX-16);

		if (IsHostFile)
		{
			WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);

			if (!Silent)
				Message(MSG_WARNING | MSG_ERRORTYPE, 1, MSG(MError), strTarget, MSG(MOk));
		}
		else // попытка найти!
		{
			WINPORT(SetLastError)(ERROR_PATH_NOT_FOUND);

			if (Silent || !Message(MSG_WARNING | MSG_ERRORTYPE, 2, MSG(MError), strTarget, MSG(MNeedNearPath), MSG(MHYes),MSG(MHNo)))
			{
				FARString strTestPathTemp = *pTestPath;

				for (;;)
				{
					if (!CutToSlash(strTestPathTemp,true))
						break;

					if (apiGetFileAttributes(strTestPathTemp) != INVALID_FILE_ATTRIBUTES)
					{
						int ChkFld=TestFolder(strTestPathTemp);

						if (ChkFld > TSTFLD_ERROR && ChkFld < TSTFLD_NOTFOUND)
						{
							if (!(pTestPath->At(0) == GOOD_SLASH && pTestPath->At(1) == GOOD_SLASH && !strTestPathTemp.At(1)))
							{
								*pTestPath = strTestPathTemp;

								if (pTestPath->GetLength() == 2) // для случая "C:", иначе попадем в текущий каталог диска C:
									AddEndSlash(*pTestPath);

								FoundPath=1;
							}

							break;
						}
					}
				}
			}
		}

		if (!FoundPath)
			return 0;
	}

	if (CtrlObject->Cp()->ActivePanel->ProcessPluginEvent(FE_CLOSE,nullptr))
		return -1;

	return 1;
}

void CreatePath(FARString &strPath)
{
	wchar_t *ChPtr = strPath.GetBuffer();
	wchar_t *DirPart = ChPtr;
	BOOL bEnd = FALSE;

	for (;;)
	{
		if (!*ChPtr || IsSlash(*ChPtr))
		{
			if (!*ChPtr)
				bEnd = TRUE;

			*ChPtr = 0;

			if (apiCreateDirectory(strPath, nullptr))
				TreeList::AddTreeName(strPath);

			if (bEnd)
				break;

			*ChPtr = GOOD_SLASH;
			DirPart = ChPtr+1;
		}

		ChPtr++;
	}

	strPath.ReleaseBuffer();
}

std::string GetHelperPathName(const char *name)
{
 	std::string out = g_strFarPath.GetMB();
	out+= GOOD_SLASH;
	out+= name;

	struct stat s;
	if (stat(out.c_str(), &s) == 0)
		return out;

	if (TranslateInstallPath_Share2Lib(out)) {
		if (stat(out.c_str(), &s) == 0)
			return out;
	}

	fprintf(stderr, "GetHelperPathName('%s') - not found\n", name);
	return out;
}

std::string GetMyScriptQuoted(const char *name)
{
	std::string out = "\"";
	out+= EscapeCmdStr(GetHelperPathName(name));
	out+= "\"";
	return out;
}


void PrepareTemporaryOpenPath(FARString &Path)
{
	Path = InMyTemp("open");

	std::vector<FARString> outdated;

	ScanTree scan_tree(0, 0);
	scan_tree.SetFindPath(Path.CPtr(), L"*", 0);
	FAR_FIND_DATA_EX found_data;
	FARString found_name;
	time_t now = time(nullptr);
	while (scan_tree.GetNextName(&found_data, found_name)) {
		struct timespec ts_mod = {}, ts_change = {};
		WINPORT(FileTimeToLocalFileTime)(&found_data.ftUnixModificationTime, &found_data.ftUnixModificationTime);
		WINPORT(FileTimeToLocalFileTime)(&found_data.ftUnixStatusChangeTime, &found_data.ftUnixStatusChangeTime);
		WINPORT(FileTime_Win32ToUnix)(&found_data.ftUnixModificationTime, &ts_mod);
		WINPORT(FileTime_Win32ToUnix)(&found_data.ftUnixStatusChangeTime, &ts_change);
		time_t delta = std::min(now - ts_mod.tv_sec, now - ts_change.tv_sec);
		if (delta > 60) {//one minute ought be enouht to open anything (c)
			outdated.push_back(found_name);
			fprintf(stderr, "PrepareTemporaryOpenPath: delta=%u for '%ls'\n", 
				(unsigned int)delta, found_name.CPtr());
		}
	};

	for (const auto &p : outdated) {
		DeleteDirTree(p.CPtr());
	}
	apiCreateDirectory(Path, nullptr);
	
	static std::atomic<unsigned short>	s_counter;
	char tmp[64]; sprintf(tmp, "%c%u_%u", GOOD_SLASH, (unsigned int)getpid(), (unsigned int)++s_counter);
	
	Path+= tmp;
	apiCreateDirectory(Path, nullptr);
}


FARString DefaultPanelInitialDirectory()
{
	FARString out;
	const char *home = getenv("HOME");
	if (home && *home) {
		out = home;
	} else
		out = g_strFarPath;
	
	DeleteEndSlash(out);
	return out;
}
