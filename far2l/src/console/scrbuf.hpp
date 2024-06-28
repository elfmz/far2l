#pragma once

/*
scrbuf.hpp

Буферизация вывода на экран, весь вывод идет через этот буфер
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

#include "bitflags.hpp"
#include "CriticalSections.hpp"
#include <WinCompat.h>

class ScreenBuf
{
private:
	BitFlags SBFlags;

	CHAR_INFO *Buf;
	CHAR_INFO *Shadow;
	CHAR_INFO MacroChar;
	bool MacroCharUsed;
	CHAR_INFO ElevationChar;
	bool ElevationCharUsed;

	SHORT BufX, BufY;
	SHORT CurX, CurY;
	bool CurVisible;
	DWORD CurSize;

	int LockCount;

	CriticalSection CS;

public:
	ScreenBuf();
	~ScreenBuf();

public:
	void AllocBuf(int X, int Y);
	void Lock();
	void Unlock();
	int GetLockCount() { return (LockCount); };
	void SetLockCount(int Count) { LockCount = Count; };
	void ResetShadow();
	void MoveCursor(int X, int Y);
	void GetCursorPos(SHORT &X, SHORT &Y);
	void SetCursorType(bool Visible, DWORD Size);
	void GetCursorType(bool &Visible, DWORD &Size);

public:
	void FillBuf();
	void Read(int X1, int Y1, int X2, int Y2, CHAR_INFO *Text, int MaxTextLength);
	void Write(int X, int Y, const CHAR_INFO *Text, int TextLength);
	void RestoreMacroChar();
	void RestoreElevationChar();

	void ApplyShadow(int X1, int Y1, int X2, int Y2, SaveScreen *sbuf);
	void ApplyColorMask(int X1, int Y1, int X2, int Y2, DWORD64 ColorMask);
	void ApplyColor(int X1, int Y1, int X2, int Y2, DWORD64 Color);
	void ApplyColor(int X1, int Y1, int X2, int Y2, DWORD64 Color, DWORD64 ExceptColor);
	void FillRect(int X1, int Y1, int X2, int Y2, WCHAR Ch, DWORD64 Color);

	void Scroll(int);
	void Flush();
};

extern ScreenBuf ScrBuf;
