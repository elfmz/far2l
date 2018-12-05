/*
constitle.cpp

Заголовок консоли
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


#include "constitle.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "ctrlobj.hpp"
#include "CriticalSections.hpp"
#include "console.hpp"
#include <stdarg.h>

static const FARString& GetFarTitleAddons()
{
	// " - Far%Ver%Admin"
	/*
		%Ver      - 2.0
		%Build    - 1259
		%Platform - x86
		%Admin    - MFarTitleAddonsAdmin
    */
	static FormatString strVer, strBuild;
	static bool bFirstRun = true;
	static FARString strTitleAddons;

	strTitleAddons.Copy(L" - Far ",7);
	strTitleAddons += Opt.strTitleAddons;

	if (bFirstRun)
	{
		bFirstRun = false;
		strVer << HIWORD(FAR_VERSION) << L"." << LOWORD(FAR_VERSION);
		strBuild << MB2Wide(FAR_BUILD).c_str();
	}

	ReplaceStrings(strTitleAddons,L"%Ver",strVer,-1,true);
	ReplaceStrings(strTitleAddons,L"%Build",strBuild,-1,true);
	ReplaceStrings(strTitleAddons,L"%Platform",
#if defined(__x86_64__)
	L"x64",
#elif defined(__ppc64__)
	L"ppc64",
#elif defined(__arm64__) || defined(__aarch64__)
	L"arm64",
#elif defined(__arm__)
	L"arm",
#else
	L"x86",
#endif
	-1,true);
	ReplaceStrings(strTitleAddons,L"%Admin",Opt.IsUserAdmin?MSG(MFarTitleAddonsAdmin):L"",-1,true);

	char hn[0x100] = {};
	if (gethostname(hn, sizeof(hn) - 1) >= 0) {
		ReplaceStrings(strTitleAddons,L"%Host", StrMB2Wide(hn).c_str(),-1,true);
	}
	const char *user = getenv("USER");
	ReplaceStrings(strTitleAddons,L"%User", StrMB2Wide(user ? user : "").c_str(),-1,true);

	RemoveTrailingSpaces(strTitleAddons);

	return strTitleAddons;
}

bool ConsoleTitle::TitleModified = false;
DWORD ConsoleTitle::ShowTime = 0;

CriticalSection TitleCS;

ConsoleTitle::ConsoleTitle(const wchar_t *title)
{
	CriticalSectionLock Lock(TitleCS);
	Console.GetTitle(strOldTitle);

	if (title)
		ConsoleTitle::SetFarTitle(title, true);
}

ConsoleTitle::~ConsoleTitle()
{
	CriticalSectionLock Lock(TitleCS);
	const FARString &strTitleAddons = GetFarTitleAddons();
	size_t OldLen = strOldTitle.GetLength();
	size_t AddonsLen = strTitleAddons.GetLength();

	if (AddonsLen <= OldLen)
	{
		if (!StrCmpI(strOldTitle.CPtr()+OldLen-AddonsLen, strTitleAddons))
			strOldTitle.SetLength(OldLen-AddonsLen);
	}

	ConsoleTitle::SetFarTitle(strOldTitle, true);
}

void ConsoleTitle::Set(const wchar_t *fmt, ...)
{
	CriticalSectionLock Lock(TitleCS);
	wchar_t msg[2048];
	va_list argptr;
	va_start(argptr, fmt);
	vswprintf(msg, ARRAYSIZE(msg)-1, fmt, argptr);
	va_end(argptr);
	SetFarTitle(msg);
}

void ConsoleTitle::SetFarTitle(const wchar_t *Title, bool Force)
{
	CriticalSectionLock Lock(TitleCS);
	static FARString strFarTitle;
	FARString strOldFarTitle;

	if (Title)
	{
		Console.GetTitle(strOldFarTitle);
		strFarTitle=Title;
		strFarTitle.SetLength(0x100);
		strFarTitle+=GetFarTitleAddons();
		TitleModified=true;

		if (StrCmp(strOldFarTitle, strFarTitle) &&
		        ((CtrlObject->Macro.IsExecuting() && !CtrlObject->Macro.IsDsableOutput()) ||
		         !CtrlObject->Macro.IsExecuting() || CtrlObject->Macro.IsExecutingLastKey()))
		{
			DWORD CurTime=WINPORT(GetTickCount)();
			if(CurTime-ShowTime>RedrawTimeout || Force)
			{
				ShowTime=CurTime;
				Console.SetTitle(strFarTitle);
				TitleModified=true;
			}
		}
	}
	else
	{
		/*
			Title=nullptr для случая, когда нужно выставить пред.заголовок
			SetFarTitle(nullptr) - это не для всех!
			Этот вызов имеет право делать только макро-движок!
		*/
		Console.SetTitle(strFarTitle);
		TitleModified=false;
		//_SVS(SysLog(L"  (nullptr)FarTitle='%s'",FarTitle));
	}
}
