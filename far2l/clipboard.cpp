/*
clipboard.cpp

Работа с буфером обмена.
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

#include "clipboard.hpp"
#include "console.hpp"

static const wchar_t FAR_VerticalBlock_Unicode[] = L"FAR_VerticalBlock_Unicode";

/* ------------------------------------------------------------ */
// CF_OEMTEXT CF_TEXT CF_UNICODETEXT CF_HDROP
HGLOBAL Clipboard::hInternalClipboard[5] = {0};
UINT    Clipboard::uInternalClipboardFormat[5] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF};

bool Clipboard::UseInternalClipboard = false;
bool Clipboard::InternalClipboardOpen = false;

//Sets UseInternalClipboard to State, and returns previous state
bool Clipboard::SetUseInternalClipboardState(bool State)
{
	bool OldState = UseInternalClipboard;
	UseInternalClipboard = State;
	return OldState;
}

bool Clipboard::GetUseInternalClipboardState()
{
	return UseInternalClipboard;
}

UINT Clipboard::RegisterFormat(LPCWSTR lpszFormat)
{
	if (UseInternalClipboard)
	{
		if (!StrCmp(lpszFormat,FAR_VerticalBlock_Unicode))
		{
			return 0xFEB1;
		}

		return 0;
	}

	return WINPORT(RegisterClipboardFormat)(lpszFormat);
}


BOOL Clipboard::Open()
{
	if (UseInternalClipboard)
	{
		if (!InternalClipboardOpen)
		{
			InternalClipboardOpen=true;
			return TRUE;
		}

		return FALSE;
	}

	return WINPORT(OpenClipboard)(NULL);//Console.GetWindow());
}

BOOL Clipboard::Close()
{
	if (UseInternalClipboard)
	{
		if (InternalClipboardOpen)
		{
			InternalClipboardOpen=false;
			return TRUE;
		}

		return FALSE;
	}

	return WINPORT(CloseClipboard)();
}

BOOL Clipboard::Empty()
{
	if (UseInternalClipboard)
	{
		if (InternalClipboardOpen)
		{
			for (size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
			{
				if (hInternalClipboard[I])
				{
					WINPORT(ClipboardFree)(hInternalClipboard[I]);
					hInternalClipboard[I]=0;
					uInternalClipboardFormat[I]=0xFFFF;
				}
			}

			return TRUE;
		}

		return FALSE;
	}

	return WINPORT(EmptyClipboard)();
}

HANDLE Clipboard::GetData(UINT uFormat)
{
	if (UseInternalClipboard)
	{
		if (InternalClipboardOpen && uFormat != 0xFFFF)
		{
			for (size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
			{
				if (uInternalClipboardFormat[I] == uFormat)
				{
					return hInternalClipboard[I];
				}
			}
		}

		return (HANDLE)nullptr;
	}

	return WINPORT(GetClipboardData)(uFormat);
}

HANDLE Clipboard::SetData(UINT uFormat,HANDLE hMem)
{
	if (UseInternalClipboard)
	{
		if (InternalClipboardOpen)
		{
			for (size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
			{
				if (!hInternalClipboard[I])
				{
					hInternalClipboard[I]=hMem;
					uInternalClipboardFormat[I]=uFormat;
					return hMem;
				}
			}
		}

		return (HANDLE)nullptr;
	}

	return WINPORT(SetClipboardData)(uFormat,hMem);
}

bool Clipboard::IsFormatAvailable(UINT Format)
{
	if (UseInternalClipboard)
	{
		for (size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
		{
			if (uInternalClipboardFormat[I] != 0xFFFF && uInternalClipboardFormat[I]==Format)
			{
				return true;
			}
		}

		return false;
	}

	return WINPORT(IsClipboardFormatAvailable)(Format) != FALSE;
}

// Перед вставкой производится очистка буфера
bool Clipboard::Copy(const wchar_t *Data, bool IsVertical)
{
	Empty();

	if (!AddData(CF_UNICODETEXT, Data, (wcslen(Data) + 1) * sizeof(wchar_t))) {
		return false;
	}

	if (IsVertical) {
		UINT FormatType = RegisterFormat(FAR_VerticalBlock_Unicode);
		if (FormatType) {
			AddData(FormatType, "\0\0\0\0", 4);
		}
	}

	return true;
}

bool Clipboard::AddData(UINT FormatType, const void *Data, size_t Size)
{
	if (!Data || !Size)
		return true;

	void *CData = WINPORT(ClipboardAlloc)(Size);

	if (!CData)
		return false;

	memcpy(CData, Data, Size);
	if (!SetData(FormatType, (HANDLE)CData))
	{
		WINPORT(ClipboardFree)(CData);
		return false;
	}

	return true;
}

// max - без учета символа конца строки!
// max = -1 - there is no limit!
wchar_t *Clipboard::Paste(bool &IsVertical, int MaxChars)
{
	PVOID ClipData = GetData(CF_UNICODETEXT);

	if (!ClipData)
		return nullptr;

	size_t CharsCount = wcsnlen((const wchar_t *)ClipData,
		WINPORT(ClipboardSize)(ClipData) / sizeof(wchar_t));

	if (MaxChars >= 0 && CharsCount < (size_t)MaxChars)
		CharsCount = (size_t)MaxChars;

	wchar_t *ClipText = (wchar_t *)malloc((CharsCount + 1) * sizeof(wchar_t));
	if (ClipText)
	{
		wmemcpy(ClipText, (const wchar_t *)ClipData, CharsCount);
		ClipText[CharsCount] = 0;
	}

	IsVertical = false;

	if (wcsstr(ClipText, NATIVE_EOLW)) {
		UINT FormatType = RegisterFormat(FAR_VerticalBlock_Unicode);
		if (FormatType && IsFormatAvailable(FormatType)) {
			IsVertical = true;
		}
	}

	return ClipText;
}

wchar_t *Clipboard::Paste()
{
	bool IsVertical;
	return Paste(IsVertical);
}

/* ------------------------------------------------------------ */
int WINAPI CopyToClipboard(const wchar_t *Data)
{
	Clipboard clip;

	if (!clip.Open())
		return FALSE;

	BOOL ret = clip.Copy(Data);

	clip.Close();

	return ret;
}

wchar_t *PasteFromClipboardEx(int MaxChars)
{
	Clipboard clip;

	if (!clip.Open())
		return nullptr;

	bool IsVertical;
	wchar_t *ClipText = clip.Paste(IsVertical, MaxChars);

	clip.Close();

	return ClipText;
}

wchar_t * WINAPI PasteFromClipboard()
{
	return PasteFromClipboardEx();
}

BOOL EmptyInternalClipboard()
{
	bool OldState = Clipboard::SetUseInternalClipboardState(true);

	Clipboard clip;

	if (!clip.Open())
		return FALSE;

	BOOL ret = clip.Empty();

	clip.Close();

	Clipboard::SetUseInternalClipboardState(OldState);

	return ret;
}
