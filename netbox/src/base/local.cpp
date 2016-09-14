/*
local.cpp

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

#include <headers.hpp>


#include "local.hpp"

const wchar_t DOS_EOL_fmt[]  = L"\r\n";
const wchar_t UNIX_EOL_fmt[] = L"\n";
const wchar_t MAC_EOL_fmt[]  = L"\r";
const wchar_t WIN_EOL_fmt[]  = L"\r\r\n";

int Win32Platform = 0;
int Win32MajorVersion = 0;
int Win32MinorVersion = 0;
int Win32BuildNumber = 0;
// int Win32CSDVersion = 0;

const wchar_t * __cdecl FarStrStrI(const wchar_t *str1, const wchar_t *str2)
{
	const wchar_t *cp = str1;
	const wchar_t *s1, *s2;

	if (!*str2)
		return str1;

	while (*cp)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && !(Lower(*s1)-Lower(*s2)))
		{
			s1++;
			s2++;
		}

		if (!*s2)
			return cp;

		cp++;
	}

	return nullptr;
}

const wchar_t * __cdecl FarStrStr(const wchar_t *str1, const wchar_t *str2)
{
	const wchar_t *cp = str1;
	const wchar_t *s1, *s2;

	if (!*str2)
		return str1;

	while (*cp)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && !(*s1 - *s2))
		{
			s1++;
			s2++;
		}

		if (!*s2)
			return cp;

		cp++;
	}

	return nullptr;
}

const wchar_t * __cdecl RevStrStrI(const wchar_t *str1, const wchar_t *str2)
{
	intptr_t len1 = StrLength(str1);
	intptr_t len2 = StrLength(str2);

	if (len2 > len1)
		return nullptr;

	if (!*str2)
		return &str1[len1];

	const wchar_t *cp = &str1[len1-len2];
	const wchar_t *s1, *s2;

	while (cp >= str1)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && !(Lower(*s1)-Lower(*s2)))
		{
			s1++;
			s2++;
		}

		if (!*s2)
			return cp;

		cp--;
	}

	return nullptr;
}

const wchar_t * __cdecl RevStrStr(const wchar_t *str1, const wchar_t *str2)
{
	intptr_t len1 = StrLength(str1);
	intptr_t len2 = StrLength(str2);

	if (len2 > len1)
		return nullptr;

	if (!*str2)
		return &str1[len1];

	const wchar_t *cp = &str1[len1-len2];
	const wchar_t *s1, *s2;

	while (cp >= str1)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && !(*s1 - *s2))
		{
			s1++;
			s2++;
		}

		if (!*s2)
			return cp;

		cp--;
	}

	return nullptr;
}

