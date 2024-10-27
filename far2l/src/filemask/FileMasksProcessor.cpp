/*
FileMasksProcessor.cpp

Класс для работы с простыми масками файлов (не учитывается наличие масок
исключения).
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

#include "FileMasksProcessor.hpp"
#include "processname.hpp"
#include "udlist.hpp"
#include "MaskGroups.hpp"

// Protect against cases like a=<a> or a=<b>, b=<a>, etc.
// It also restricts valid nesting depth but the limit is high enough to cover all practical cases.
// This method was chosen due to its simplicity.
#define MAXCALLDEPTH 64

static wchar_t *FindExcludeChar(wchar_t *masks)
{
	for (bool regexp=false; *masks; masks++)
	{
		if (!regexp)
		{
			if (*masks == EXCLUDEMASKSEPARATOR)
				return masks;
			if (*masks == L'/')
				regexp = true;
		}
		else
		{
			if (*masks == L'\\')
			{
				if (*(++masks) == 0) // skip the next char
					break;
			}
			else if (*masks == L'/')
				regexp = false;
		}
	}
	return nullptr;
}

bool SingleFileMask::Set(const wchar_t *Masks, DWORD Flags)
{
	Mask = Masks;
	return !Mask.IsEmpty();
}

bool SingleFileMask::Compare(const wchar_t *Name, bool ignorecase) const
{
	return CmpName(Mask.CPtr(), Name, false, ignorecase);
}

void SingleFileMask::Reset()
{
	Mask.Clear();
}

RegexMask::RegexMask() : re(nullptr)
{
}

void RegexMask::Reset()
{
	re.reset();
}

bool RegexMask::Set(const wchar_t *masks, DWORD Flags)
{
	Reset();

	if (*masks == L'/')
	{
		re.reset(new(std::nothrow) RegExp);
		if (re && re->Compile(masks, OP_PERLSTYLE|OP_OPTIMIZE))
		{
			return true;
		}
		Reset();
	}

	return false;
}

bool RegexMask::Compare(const wchar_t *FileName, bool ignorecase) const
{
	if (re)
	{
		RegExpMatch MatchData[1];
		int BrCount = ARRAYSIZE(MatchData);
		return re->Search(ReStringView(FileName), MatchData, BrCount);
	}

	return false;
}

FileMasksProcessor::FileMasksProcessor() : CallDepth(0)
{
}

FileMasksProcessor::FileMasksProcessor(int aCallDepth) : CallDepth(aCallDepth)
{
}

void FileMasksProcessor::Reset()
{
	for (auto I: IncludeMasks) { I->Reset(); delete I; }
	for (auto I: ExcludeMasks) { I->Reset(); delete I; }

	IncludeMasks.clear();
	ExcludeMasks.clear();
}

/*
 Инициализирует список масок. Принимает список, разделенных запятой.
 Возвращает FALSE при неудаче (например, одна из
 длина одной из масок равна 0)
*/
bool FileMasksProcessor::Set(const wchar_t *masks, DWORD Flags)
{
	Reset();

	if (CallDepth >= MAXCALLDEPTH) return false;

	if (!*masks) return false;

	bool rc = false;
	wchar_t *MasksStr = wcsdup(masks);

	if (MasksStr) {
		rc = true;

		wchar_t *pInclude = MasksStr;
		while (iswspace(*pInclude))
			pInclude++;

		wchar_t *pExclude = FindExcludeChar(pInclude);

		if (pExclude) {
			*pExclude++ = 0;
			while (iswspace(*pExclude))
				pExclude++;

			if (*pInclude == 0 && *pExclude == 0)
				rc = false;
			else if (*pExclude == 0) // treat empty exclude mask as no exclude mask
				pExclude = nullptr;
			else {
				wchar_t *ptr = FindExcludeChar(pExclude);
				if (ptr) *ptr = 0; // ignore all starting from the 2-nd exclude char
			}
		}

		if (rc)
			rc = SetPart(*pInclude ? pInclude : L"*", Flags & FMF_ADDASTERISK, IncludeMasks);

		if (rc && pExclude)
			rc = SetPart(pExclude, 0, ExcludeMasks);

		free(MasksStr);
	}

	if (!rc)
		Reset();

	return rc;
}

/*
 Компилирует список масок.
 Принимает список масок, разделенных запятой или точкой с запятой.
 Возвращает FALSE при неудаче (например, длина одной из масок равна 0).
*/
bool FileMasksProcessor::SetPart(const wchar_t *masks, DWORD Flags, std::vector<BaseFileMask*> &Target)
{
	DWORD flags = ULF_PACKASTERISKS | ULF_PROCESSBRACKETS | ULF_PROCESSREGEXP;

	if (Flags & FMF_ADDASTERISK)
		flags |= ULF_ADDASTERISK;

	UserDefinedList UdList(L';', L',', flags);

//	UserDefinedList(DWORD Flags=0, wchar_t separator1=L';', wchar_t separator2=L',');
//	UserDefinedList(WORD separator1, WORD separator2, DWORD Flags);

	if (UdList.Set(masks))
	{
		FARString strMask;
		const wchar_t *onemask;

		for (int I=0; (onemask=UdList.Get(I)); I++)
		{
			BaseFileMask *baseMask = nullptr;

			if (*onemask == L'<')
			{
				auto pStart = onemask;
				auto pEnd = wcschr(++pStart, L'>');
				if (pEnd && pEnd!=pStart && pEnd[1]==0)
				{
					FARString strKey(pStart, pEnd-pStart);
					if (GetMaskGroup(strKey, strMask))
					{
						baseMask = new(std::nothrow) FileMasksProcessor(CallDepth + 1);
						onemask = strMask.CPtr();
					}
				}
			}

			if (!baseMask)
			{
				if (*onemask == L'/')
					baseMask = new(std::nothrow) RegexMask;
				else
					baseMask = new(std::nothrow) SingleFileMask();
			}

			if (baseMask && baseMask->Set(onemask,0))
			{
				Target.push_back(baseMask);
			}
			else
			{
				Reset();
				delete baseMask;
				return false;
			}
		}
		return true;
	}

	return false;
}

/* сравнить имя файла со списком масок
   Возвращает TRUE в случае успеха.
   Путь к файлу в FileName НЕ игнорируется */
bool FileMasksProcessor::Compare(const wchar_t *FileName, bool ignorecase) const
{
	for (auto I: IncludeMasks)
	{
		if (I->Compare(FileName, ignorecase))
		{
			for (auto J: ExcludeMasks)
			{
				if (J->Compare(FileName, ignorecase))	return false;
			}
			return true;
		}
	}
	return false;
}
