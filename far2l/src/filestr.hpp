#pragma once

/*
filestr.hpp

Класс GetFileString
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

#include <vector>
#include <WinCompat.h>
#include "FARString.hpp"

// BUGBUG, delete!
class OldGetFileString
{
	private:
		FILE *SrcFile;
		int ReadPos, ReadSize;

		char ReadBuf[8192];
		wchar_t wReadBuf[8192];

		int m_nStrLength;
		char *Str;

		int m_nwStrLength;
		wchar_t *wStr;

		bool SomeDataLost;
		bool bCrCr;

	private:
		int GetAnsiString(char **DestStr, int &Length);
		int GetUnicodeString(wchar_t **DestStr, int &Length, bool bBigEndian);

	public:
		OldGetFileString(FILE *SrcFile);
		~OldGetFileString();

	public:
		int GetString(wchar_t **DestStr, int nCodePage, int &Length);
		bool IsConversionValid() { return !SomeDataLost; };
};

bool DetectFileMagic(FILE *file, UINT &nCodePage);

class StringReader
{
	std::vector<char> _tmp;

public:
	wchar_t *Read(FILE *file, wchar_t *lpwszDest, size_t nDestLength, int nCodePage);
};

//-----------------------------------------------------------------------------
struct GetFileStringContext
{
	GetFileStringContext(File &SrcFile_) : 
		SrcFile(SrcFile_),
		bCrCr(false),
		SomeDataLost(false),
		ReadPos(0),
		ReadSize(0)
	{
	}

	File &SrcFile;
	bool bCrCr;
	bool SomeDataLost;
	DWORD ReadPos;	
	DWORD ReadSize;
};


template <class CHAR_T> class TypedStringReader;
typedef TypedStringReader<char> Char_StringReader;
typedef TypedStringReader<uint16_t> UTF16_StringReader;
typedef TypedStringReader<uint32_t> UTF32_StringReader;


class GetFileString
{
public:
		GetFileString(File& SrcFile);
		~GetFileString();
		int PeekString(LPWSTR* DestStr, UINT nCodePage, int& Length);
		int GetString(LPWSTR* DestStr, UINT nCodePage, int& Length);
		bool IsConversionValid() { return !context.SomeDataLost; }

	private:
		GetFileString(const GetFileString&) = delete;

		GetFileStringContext context;
		UTF32_StringReader *UTF32Reader;
		UTF16_StringReader *UTF16Reader;
		Char_StringReader *CharReader;

		bool Peek;
		int LastLength;
		LPWSTR* LastString;
		int LastResult;
		
		std::wstring Buffer;
};

bool GetFileFormat(File& file, UINT& nCodePage, bool* pSignatureFound = nullptr, bool bUseHeuristics = true);
bool GetFileFormat2(FARString strFileName, UINT& nCodePage, bool* pSignatureFound, bool bUseHeuristics, bool bCheckIfSupported);
