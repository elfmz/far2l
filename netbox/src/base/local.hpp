#pragma once

/*
local.hpp

Сравнение без учета регистра, преобразование регистра
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
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

// #include <stdio.h>
// #include <lmcons.h>
// #define SECURITY_WIN32
// #include <sspi.h>
// #include <secext.h>
#ifndef __linux__
#include <WinUser.h>
#endif

extern const wchar_t DOS_EOL_fmt[];
extern const wchar_t UNIX_EOL_fmt[];
extern const wchar_t MAC_EOL_fmt[];
extern const wchar_t WIN_EOL_fmt[];

extern int Win32Platform;
extern int Win32MajorVersion;
extern int Win32MinorVersion;
extern int Win32BuildNumber;
// extern int Win32CSDVersion;

inline intptr_t __cdecl StrLength(const wchar_t * str) { return wcslen(str ? str : L""); }

inline wchar_t __cdecl Upper(wchar_t Ch) { CharUpperBuff(&Ch, 1); return Ch; }

inline wchar_t __cdecl Lower(wchar_t Ch) { CharLowerBuff(&Ch, 1); return Ch; }

inline int __cdecl StrCmpNNI(const wchar_t * s1, int n1, const wchar_t * s2, int n2) { return CompareString(0, NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT, s1, n1, s2, n2) - 2; }
inline int __cdecl StrLIComp(const wchar_t * s1, const wchar_t * s2, int n) { return StrCmpNNI(s1, n, s2, n); }

inline int __cdecl FarStrCmpI(const wchar_t * s1, const wchar_t * s2) { return CompareString(0, NORM_IGNORECASE|SORT_STRINGSORT, s1,-1, s2, -1) - 2; }

inline int __cdecl StrCmpNN(const wchar_t * s1, int n1, const wchar_t * s2, int n2) { return CompareString(0, NORM_STOP_ON_NULL|SORT_STRINGSORT, s1, n1, s2, n2) - 2; }

