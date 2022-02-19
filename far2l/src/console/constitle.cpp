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
#include "interf.hpp"
#include "config.hpp"
#include "ctrlobj.hpp"
#include "CriticalSections.hpp"
#include "console.hpp"
#include <stdarg.h>

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
	ConsoleTitle::SetFarTitle(strOldTitle, true, true);
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

void ConsoleTitle::SetFarTitle(const wchar_t *Title, bool Force, bool Restoring)
{
	CriticalSectionLock Lock(TitleCS);
	static FARString strFarTitle;
	static FARString strVer(FAR_BUILD);
	static FARString strPlatform(FAR_PLATFORM);
	FARString strOldFarTitle, strFarState;

	if (Title)
	{
		Console.GetTitle(strOldFarTitle);

		strFarState=Title;
		strFarState.Truncate(0x100);
		if (Restoring) {
			strFarTitle = strFarState;
		} else {
			/*
				%State    - default window title
				%Ver      - 2.3.102-beta
				%Platform - x86
				%Backend  - gui
				%Admin    - Msg::FarTitleAddonsAdmin
			*/
			strFarTitle=Opt.strWindowTitle;
			ReplaceStrings(strFarTitle,L"%Ver",strVer,-1);
			ReplaceStrings(strFarTitle,L"%Platform", strPlatform, -1);
			ReplaceStrings(strFarTitle,L"%Backend", WinPortBackend(), -1);
			ReplaceStrings(strFarTitle,L"%Admin",Opt.IsUserAdmin?Msg::FarTitleAddonsAdmin:L"",-1);

			FARString hn, un;
			apiGetEnvironmentVariable("HOSTNAME", hn);
			apiGetEnvironmentVariable("USER", un);
			ReplaceStrings(strFarTitle,L"%Host",hn,-1);
			ReplaceStrings(strFarTitle,L"%User",un,-1);

			// сделаем эту замену последней во избежание случайных совпадений
			// подстрок из strFarState с другими переменными
			ReplaceStrings(strFarTitle,L"%State",strFarState,-1);

			RemoveExternalSpaces(strFarTitle);
		}
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
