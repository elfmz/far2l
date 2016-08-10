/*
codepage.cpp

Работа с кодовыми страницами
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

#include "codepage.hpp"
#include "lang.hpp"
#include "vmenu.hpp"
#include "savefpos.hpp"
#include "keys.hpp"
#include "registry.hpp"
#include "language.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include "config.hpp"

// Ключ где хранятся имена кодовых страниц
const wchar_t *NamesOfCodePagesKey = L"CodePages/Names";

const wchar_t *FavoriteCodePagesKey = L"CodePages/Favorites";

// Стандартные кодовое страницы
enum StandardCodePages
{
	SearchAll = 1,
	Auto = 2,
	OEM = 4,
	ANSI = 8,
	UTF7 = 16,
	UTF8 = 32,
	UTF16LE = 64,
	UTF16BE = 128,
	AllStandard = OEM | ANSI | UTF7 | UTF8 | UTF16BE | UTF16LE
};

// Источник вызова каллбака прохода по кодовым страницам
enum CodePagesCallbackCallSource
{
	CodePageSelect,
	CodePagesFill,
	CodePageCheck
};

// Номера контролов диалога редактирования имени коловой страницы
enum
{
	EDITCP_BORDER,
	EDITCP_EDIT,
	EDITCP_SEPARATOR,
	EDITCP_OK,
	EDITCP_CANCEL,
	EDITCP_RESET,
};

bool IsCodePageSupported(UINT CodePage)
{
//todo
	return true;
}

UINT SelectCodePage(UINT nCurrent, bool bShowUnicode, bool bShowUTF, bool bShowUTF7)
{
//todo
	return 0;
}

UINT FillCodePagesList(HANDLE dialogHandle, UINT controlId, UINT codePage, bool allowAuto, bool allowAll)
{
//todo
	return 0;
}

wchar_t *FormatCodePageName(UINT CodePage, wchar_t *CodePageName, size_t Length)
{
//todo
	return 0;
}

