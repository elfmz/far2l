/*
mix.cpp

Êó÷à ðàçíûõ âñïîìîãàòåëüíûõ ôóíêöèé
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

#include "mix.hpp"
#include "CFileMask.hpp"
#include "scantree.hpp"
#include "config.hpp"
#include "pathmix.hpp"

int ToPercent(uint32_t N1,uint32_t N2)
{
	if (N1 > 10000)
	{
		N1/=100;
		N2/=100;
	}

	if (!N2)
		return 0;

	if (N2<N1)
		return(100);

	return((int)(N1*100/N2));
}

int ToPercent64(uint64_t N1, uint64_t N2)
{
	if (N1 > 10000)
	{
		N1/=100;
		N2/=100;
	}

	if (!N2)
		return 0;

	if (N2<N1)
		return 100;

	return static_cast<int>(N1*100/N2);
}

/* $ 30.07.2001 IS
     1. Ïðîâåðÿåì ïðàâèëüíîñòü ïàðàìåòðîâ.
     2. Òåïåðü îáðàáîòêà êàòàëîãîâ íå çàâèñèò îò ìàñêè ôàéëîâ
     3. Ìàñêà ìîæåò áûòü ñòàíäàðòíîãî ôàðîâñêîãî âèäà (ñî ñêîáêàìè,
        ïåðå÷èñëåíèåì è ïð.). Ìîæåò áûòü íåñêîëüêî ìàñîê ôàéëîâ, ðàçäåëåííûõ
        çàïÿòûìè èëè òî÷êîé ñ çàïÿòîé, ìîæíî óêàçûâàòü ìàñêè èñêëþ÷åíèÿ,
        ìîæíî çàêëþ÷àòü ìàñêè â êàâû÷êè. Êîðî÷å, âñå êàê è äîëæíî áûòü :-)
*/
void WINAPI FarRecursiveSearch(const wchar_t *InitDir,const wchar_t *Mask,FRSUSERFUNC Func,DWORD Flags,void *Param)
{
	if (Func && InitDir && *InitDir && Mask && *Mask)
	{
		CFileMask FMask;

		if (!FMask.Set(Mask, FMF_SILENT)) return;

		Flags=Flags&0x000000FF; // òîëüêî ìëàäøèé áàéò!
		ScanTree ScTree(Flags & FRS_RETUPDIR,Flags & FRS_RECUR, Flags & FRS_SCANSYMLINK);
		FAR_FIND_DATA_EX FindData;
		string strFullName;
		ScTree.SetFindPath(InitDir,L"*");

		while (ScTree.GetNextName(&FindData,strFullName))
		{
			if (FMask.Compare(FindData.strFileName))
			{
				FAR_FIND_DATA fdata;
				apiFindDataExToData(&FindData, &fdata);

				if (!Func(&fdata,strFullName,Param))
				{
					apiFreeFindData(&fdata);
					break;
				}

				apiFreeFindData(&fdata);
			}
		}
	}
}

/* $ 14.09.2000 SVS
 + Ôóíêöèÿ FarMkTemp - ïîëó÷åíèå èìåíè âðåìåííîãî ôàéëà ñ ïîëíûì ïóòåì.
    Dest - ïðèåìíèê ðåçóëüòàòà
    Template - øàáëîí ïî ïðàâèëàì ôóíêöèè mktemp, íàïðèìåð "FarTmpXXXXXX"
    Âåðíåò òðåáóåìûé ðàçìåð ïðèåìíèêà.
*/
int WINAPI FarMkTemp(wchar_t *Dest, DWORD size, const wchar_t *Prefix)
{
	string strDest;
	if (FarMkTempEx(strDest, Prefix, TRUE) && Dest && size)
	{
		xwcsncpy(Dest, strDest, size);
	}
	return static_cast<int>(strDest.GetLength()+1);
}

/*
             v - òî÷êà
   prefXXX X X XXX
       \ / ^   ^^^\ PID + TID
        |  \------/
        |
        +---------- [0A-Z]
*/
string& FarMkTempEx(string &strDest, const wchar_t *Prefix, BOOL WithTempPath, const wchar_t *UserTempPath)
{
	if (!(Prefix && *Prefix))
		Prefix=L"FTMP";
	string strPath = L".";

	if (WithTempPath)
	{
		apiGetTempPath(strPath);
	}
	else if(UserTempPath)
	{
		strPath=UserTempPath;
	}

	AddEndSlash(strPath);

	wchar_t *lpwszDest = strDest.GetBuffer(StrLength(Prefix)+strPath.GetLength()+13);
	UINT uniq = WINPORT(GetCurrentProcessId)(), savePid = uniq;

	for (;;)
	{
		if (!uniq) ++uniq;

		if (WINPORT(GetTempFileName)(strPath, Prefix, uniq, lpwszDest)
		        && apiGetFileAttributes(lpwszDest) == INVALID_FILE_ATTRIBUTES) break;

		if (++uniq == savePid)
		{
			*lpwszDest = 0;
			break;
		}
	}

	strDest.ReleaseBuffer();
	return strDest;
}
