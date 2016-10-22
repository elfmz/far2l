/*
scantree.cpp

Сканирование текущего каталога и, опционально, подкаталогов на
предмет имен файлов
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


#include "scantree.hpp"
#include "syslog.hpp"
#include "config.hpp"
#include "pathmix.hpp"

ScanTree::ScanTree(int RetUpDir,int Recurse, int ScanJunction)
{
	Flags.Change(FSCANTREE_RETUPDIR,RetUpDir);
	Flags.Change(FSCANTREE_RECUR,Recurse);
	Flags.Change(FSCANTREE_SCANSYMLINK,(ScanJunction==-1?Opt.ScanJunction:ScanJunction));
	ScanItems.setDelta(10);
}

void ScanTree::SetFindPath(const wchar_t *Path,const wchar_t *Mask, const DWORD NewScanFlags)
{
	ScanItems.Free();
	ScanItems.addItem();
	Flags.Clear(FSCANTREE_FILESFIRST);
	strFindMask = Mask;
	strFindPath = Path;
	ConvertNameToReal(strFindPath, ScanItems.lastItem()->RealPath);
	AddEndSlash(strFindPath);
	strFindPath += strFindMask;
	Flags.Flags=(Flags.Flags&0x0000FFFF)|(NewScanFlags&0xFFFF0000);
}

bool ScanTree::GetNextName(FAR_FIND_DATA_EX *fdata,FARString &strFullName)
{
	if (!ScanItems.getCount())
		return false;

	bool Done=false;
	Flags.Clear(FSCANTREE_SECONDDIRNAME);

	for (;;)
	{
		if (!ScanItems.lastItem()->Find)
		{
			DWORD WinPortFindFlags = 0;
			if (Flags.Check(FSCANTREE_NOLINKS)) WinPortFindFlags|= FIND_FILE_FLAG_NO_LINKS;
			if (Flags.Check(FSCANTREE_NOFILES)) WinPortFindFlags|= FIND_FILE_FLAG_NO_FILES;
			if (Flags.Check(FSCANTREE_NODEVICES)) WinPortFindFlags|= FIND_FILE_FLAG_NO_DEVICES;
			if (Flags.Check(FSCANTREE_CASE_INSENSITIVE)) WinPortFindFlags|= FIND_FILE_FLAG_CASE_INSENSITIVE;
			ScanItems.lastItem()->Find = new FindFile(strFindPath, Flags.Check(FSCANTREE_SCANSYMLINK), WinPortFindFlags);
				
		}
		Done=!ScanItems.lastItem()->Find->Get(*fdata);
		
		if (Flags.Check(FSCANTREE_FILESFIRST))
		{
			if (ScanItems.lastItem()->Flags.Check(FSCANTREE_SECONDPASS))
			{
				if (!Done && !(fdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					continue;
			}
			else
			{
				if (!Done && (fdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					continue;

				if (Done)
				{
					if(ScanItems.lastItem()->Find)
					{
						delete ScanItems.lastItem()->Find;
						ScanItems.lastItem()->Find = nullptr;
					}
					ScanItems.lastItem()->Flags.Set(FSCANTREE_SECONDPASS);
					continue;
				}
			}
		}
		break;
	}

	if (Done)
	{
		ScanItems.deleteItem(ScanItems.getCount()-1);

		if (!ScanItems.getCount())
			return false;
		else
		{
			if (!ScanItems.lastItem()->Flags.Check(FSCANTREE_INSIDEJUNCTION))
				Flags.Clear(FSCANTREE_INSIDEJUNCTION);

			CutToSlash(strFindPath,true);

			if (Flags.Check(FSCANTREE_RETUPDIR))
			{
				strFullName = strFindPath;
				apiGetFindDataEx(strFullName, *fdata);
			}

			CutToSlash(strFindPath);
			strFindPath += strFindMask;
			_SVS(SysLog(L"1. FullName='%ls'",strFullName.CPtr()));

			if (Flags.Check(FSCANTREE_RETUPDIR))
			{
				Flags.Set(FSCANTREE_SECONDDIRNAME);
				return true;
			}

			return GetNextName(fdata,strFullName);
		}
	}
	else
	{
		if ((fdata->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && Flags.Check(FSCANTREE_RECUR) &&
		        (!(fdata->dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) || Flags.Check(FSCANTREE_SCANSYMLINK)))
		{
			FARString RealPath(ScanItems.lastItem()->RealPath);
			AddEndSlash(RealPath);
			RealPath += fdata->strFileName;


			//recursive symlinks guard
			bool Recursion = false;

			if (fdata->dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) {
				ConvertNameToReal(RealPath, RealPath);
				//check if converted path points to same location is already scanned or to parent path of already scanned location
				//NB: in original FAR here was exact-match check all pathes (not only symlinks)
				//that caused excessive scan from FS root cuz in Linux links pointing to / are usual situation unlike Windows

				const size_t RealPathLen = RealPath.GetLength();
				for (size_t i = 0; i < ScanItems.getCount(); i++) {
					FARString &IthPath = ScanItems.getItem(i)->RealPath;
					const size_t IthPathLen = IthPath.GetLength();
					if ( (IthPathLen >= RealPathLen && memcmp(IthPath.CPtr(), 
							RealPath.CPtr(), RealPathLen * sizeof(wchar_t)) == 0) &&
							(IthPathLen == RealPathLen || 
							IthPath.At(RealPathLen) == GOOD_SLASH || 
							RealPathLen == 1) ) {
						Recursion = true;
						break;
					}
				}
			}

			if (!Recursion)
			{
				CutToSlash(strFindPath);
				strFindPath += fdata->strFileName;
				strFullName = strFindPath;
				strFindPath += L"/";
				strFindPath += strFindMask;
				ScanItems.addItem();
				ScanItems.lastItem()->Flags = ScanItems.getItem(ScanItems.getCount()-2)->Flags; // наследуем флаг
				ScanItems.lastItem()->Flags.Clear(FSCANTREE_SECONDPASS);
				ScanItems.lastItem()->RealPath = RealPath;

				if (fdata->dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)
				{
					ScanItems.lastItem()->Flags.Set(FSCANTREE_INSIDEJUNCTION);
					Flags.Set(FSCANTREE_INSIDEJUNCTION);
				}

				return true;
			}
		}
	}

	strFullName = strFindPath;
	CutToSlash(strFullName);
	strFullName += fdata->strFileName;
	return true;
}

void ScanTree::SkipDir()
{
	if (!ScanItems.getCount())
		return;

	ScanItems.deleteItem(ScanItems.getCount()-1);

	if (!ScanItems.getCount())
		return;

	if (!ScanItems.lastItem()->Flags.Check(FSCANTREE_INSIDEJUNCTION))
		Flags.Clear(FSCANTREE_INSIDEJUNCTION);

	CutToSlash(strFindPath,true);
	CutToSlash(strFindPath);
	strFindPath += strFindMask;
}
