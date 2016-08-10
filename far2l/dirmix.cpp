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
#pragma hdrstop

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

BOOL FarChDir(const wchar_t *NewDir, BOOL ChangeDir)
{
	if (!NewDir || !*NewDir)
		return FALSE;

	BOOL rc=FALSE;
	string strCurDir;

	{
		if (ChangeDir)
		{
			if (*NewDir=='/') {
				strCurDir = NewDir;
				rc = apiSetCurrentDirectory(strCurDir); // çäåñü áåðåì êîðåíü
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
  Ôóíêöèÿ TestFolder âîçâðàùàåò îäíî ñîñòîÿíèé òåñòèðóåìîãî êàòàëîãà:

    TSTFLD_NOTFOUND   (2) - íåò òàêîãî
    TSTFLD_NOTEMPTY   (1) - íå ïóñòî
    TSTFLD_EMPTY      (0) - ïóñòî
    TSTFLD_NOTACCESS (-1) - íåò äîñòóïà
    TSTFLD_ERROR     (-2) - îøèáêà (êðèâûå ïàðàìåòðû èëè íåõâàòèëî ïàìÿòè äëÿ âûäåëåíèÿ ïðîìåæóòî÷íûõ áóôåðîâ)
*/
int TestFolder(const wchar_t *Path)
{
	if (!(Path && *Path)) // ïðîâåðêà íà âøèâîñòü
		return TSTFLD_ERROR;

	string strFindPath = Path;
	// ñîîáðàçèì ìàñêó äëÿ ïîèñêà.
	AddEndSlash(strFindPath);
	strFindPath += L"*";

	// ïåðâàÿ ïðîâåðêà - ÷å-íèòü ñ÷èòàòü ìîæåì?
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

		// ñîáñòâåííî... íå ôàêò, ÷òî äèñê íå ÷èòàåì, ò.ê. íà ÷èñòîì äèñêå â êîðíå íåòó äàæå "."
		// ïîýòîìó ïîñìîòðèì íà Root
		GetPathRoot(Path,strFindPath);

		if (!StrCmp(Path,strFindPath))
		{
			// ïðîâåðêà àòðèáóòîâ ãàðàíòèðîâàíî ñêàæåò - ýòî áàãà BugZ#743 èëè ïóñòîé êîðåíü äèñêà.
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


	// îäíîçíà÷íî êàòàëîã ïóñò
	return TSTFLD_EMPTY;
}

/*
   Ïðîâåðêà ïóòè èëè õîñò-ôàéëà íà ñóùåñòâîâàíèå
   Åñëè èäåò ïðîâåðêà ïóòè (IsHostFile=FALSE), òî áóäåò
   ïðåäïðèíÿòà ïîïûòêà íàéòè áëèæàéøèé ïóòü. Ðåçóëüòàò ïîïûòêè
   âîçâðàùàåòñÿ â ïåðåäàííîì TestPath.

   Return: 0 - áßäà.
           1 - ÎÁÈ!,
          -1 - Ïî÷òè ÷òî ÎÁÈ, íî ProcessPluginEvent âåðíóë TRUE
   TestPath ìîæåò áûòü ïóñòûì, òîãäà ïðîñòî èñïîëíèì ProcessPluginEvent()

*/
int CheckShortcutFolder(string *pTestPath,int IsHostFile, BOOL Silent)
{
	if (pTestPath && !pTestPath->IsEmpty() && apiGetFileAttributes(*pTestPath) == INVALID_FILE_ATTRIBUTES)
	{
		int FoundPath=0;
		string strTarget = *pTestPath;
		TruncPathStr(strTarget, ScrX-16);

		if (IsHostFile)
		{
			WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);

			if (!Silent)
				Message(MSG_WARNING | MSG_ERRORTYPE, 1, MSG(MError), strTarget, MSG(MOk));
		}
		else // ïîïûòêà íàéòè!
		{
			WINPORT(SetLastError)(ERROR_PATH_NOT_FOUND);

			if (Silent || !Message(MSG_WARNING | MSG_ERRORTYPE, 2, MSG(MError), strTarget, MSG(MNeedNearPath), MSG(MHYes),MSG(MHNo)))
			{
				string strTestPathTemp = *pTestPath;

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

								if (pTestPath->GetLength() == 2) // äëÿ ñëó÷àÿ "C:", èíà÷å ïîïàäåì â òåêóùèé êàòàëîã äèñêà C:
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

void CreatePath(string &strPath)
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

			if (Opt.CreateUppercaseFolders && !IsCaseMixed(DirPart) && apiGetFileAttributes(strPath) == INVALID_FILE_ATTRIBUTES)  //BUGBUG
				WINPORT(CharUpper)(DirPart);

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
