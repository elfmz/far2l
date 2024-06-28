#pragma once

/*
message.hpp

Вывод MessageBox
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

#include <WinCompat.h>
#include <vector>
#include "FARString.hpp"
#include "lang.hpp"

#define MAX_WIDTH_MESSAGE static_cast<DWORD>(ScrX - 13)

#define ADDSPACEFORPSTRFORMESSAGE 16

enum
{
	MSG_WARNING        = 0x00000001,
	MSG_ERRORTYPE      = 0x00000002,
	MSG_KEEPBACKGROUND = 0x00000004,
	MSG_LEFTALIGN      = 0x00000010,
	MSG_KILLSAVESCREEN = 0x00000020,
	MSG_DISPLAYNOTIFY  = 0x00000080
};


struct Messager : protected std::vector<const wchar_t *>
{
	Messager(FarLangMsg title);
	Messager(const wchar_t *title);
	Messager();		// title supposed to be set by very first Add()

	~Messager();

	Messager &Add(FarLangMsg v);
	Messager &Add(const wchar_t *v);
	inline Messager &Add() { return *this; }

	template <class FirstItemT, class SecondItemT, class... OtherItemsT>
	Messager &Add(const FirstItemT &FirstItem, const SecondItemT &SecondItem, OtherItemsT... OtherItems)
	{
		return Add(FirstItem).Add(SecondItem, OtherItems...);
	}

	int Show(DWORD Flags, int Buttons, INT_PTR PluginNumber);
	int Show(DWORD Flags, int Buttons);
	int Show(int Buttons = 0);

};

struct ExMessager : Messager
{
	ExMessager(FarLangMsg title);
	ExMessager(const wchar_t *title);
	ExMessager();		// title supposed to be set by very first Add()

	~ExMessager();

	Messager &AddFormatV(const wchar_t *fmt, va_list args);
	Messager &AddFormat(FarLangMsg fmt, ...);
	Messager &AddFormat(const wchar_t *fmt, ...);
	Messager &AddDup(const wchar_t *v);

private:
	std::vector<FARString> _owneds;
};

template <class TitleT, class... ItemsT>
int Message(DWORD Flags, int Buttons, const TitleT &Title, ItemsT... Items)
{
	return Messager(Title).Add(Items...).Show(Flags, Buttons);
}

void SetMessageHelp(const wchar_t *Topic);
void GetMessagePosition(int &X1, int &Y1, int &X2, int &Y2);

/*
	$ 12.03.2002 VVM
	Новая функция - пользователь попытался прервать операцию.
	Зададим вопрос.
	Возвращает:
		FALSE - продолжить операцию
		TRUE  - прервать операцию
*/
bool AbortMessage();

bool GetErrorString(FARString &strErrStr);
void SetErrorString(const FARString &strErrStr);
