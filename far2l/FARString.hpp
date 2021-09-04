#pragma once

/*
FARString.hpp

Unicode строка
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

#include "local.hpp"

/********************************************************************************************
 * This is a FARString - homegrew reference-counted string widely used in this project.
 * Major benefit and drawback of FARString: reference counter is not thread safe!
 * This means if two threads use string copies that share same content then HUGE problems
 * may arise in case of that threads will modify, destroy or even just copy that strings.
 * One day will need to do something with this, either to use std::atomic for ref counter
 * either move to std::wstring trying to mitigate its copying behaviour by std::string_view
 * either do something smart and tricky. For now multithreading support of FARString looks
 * so:
 *   DONT USE FARString IF YOU NEED TO COPY ITS INSTANCES TO BE USED IN DIFFERENT THREADS!
 * But if you really strongly want this and know what you're doing - use FARString::Clone
 * to get really independent FARString copies where you (should) know this is needed.
 * And use FARSTRING_THREAD_SANITIZER macro below to validate such code.
 */

class FARString
{
	class Content // must be trivial
	{
		static Content sEmptyData; //для оптимизации создания пустых FARString

		unsigned int m_nCapacity;	// not including final NULL char
		unsigned int m_nRefCount;	// zero means single owner
		unsigned int m_nLength;		// not including final NULL char
		wchar_t m_Data[1];

		static void Destroy(Content *data);

	public:
		static inline Content *EmptySingleton() { return &sEmptyData; }

		static Content *Create(size_t nCapacity);
		static Content *Create(size_t nCapacity, const wchar_t *SrcData, size_t SrcLen);

		void SetLength(size_t nLength);

		inline wchar_t *GetData() { return m_Data; }
		inline size_t GetCapacity() const { return m_nCapacity; }
		inline size_t GetLength() const { return m_nLength; }

		inline bool SingleOwner() const { return m_nRefCount == 0; }

		inline void AddRef()
		{
			++m_nRefCount;
		}
		inline void DecRef()
		{
			if (0 == m_nRefCount--)
				Destroy(this);
		}
	} *m_pContent;

	inline void Init()
	{
		m_pContent = Content::EmptySingleton();
	}

	inline void Init(const wchar_t *SrcData, size_t SrcLen)
	{
		m_pContent = LIKELY(SrcLen)
			? Content::Create(SrcLen, SrcData, SrcLen)
			: Content::EmptySingleton();
	}

	inline void Init(const wchar_t *SrcData)
	{
		if (LIKELY(SrcData))
			Init(SrcData, wcslen(SrcData));
		else
			Init();
	}

	void PrepareForModify(size_t nCapacity);
	void PrepareForModify();

public:

	inline FARString() { Init(); }

	inline FARString(FARString &&strOriginal) : m_pContent(strOriginal.m_pContent) { strOriginal.Init(); }
	inline FARString(const FARString &strCopy) : m_pContent(strCopy.m_pContent) { m_pContent->AddRef(); }
	inline FARString(const wchar_t *lpwszData) { Init(lpwszData); }
	inline FARString(const wchar_t *lpwszData, size_t nLength) { Init(lpwszData, nLength); }
	inline FARString(const std::wstring &strData) { Init(strData.c_str(), strData.size()); }

	inline FARString(const char *lpszData, UINT CodePage=CP_UTF8) { Init(); Copy(lpszData, CodePage); }
	inline FARString(const std::string &strData, UINT CodePage=CP_UTF8) { Init(); Copy(strData.c_str(), CodePage); }

	inline ~FARString() { /*if (m_pContent) он не должен быть nullptr*/ m_pContent->DecRef(); }

	// Returns copy of *this string but that copy uses OWN, not shared with *this, content.
	inline FARString Clone() const { return FARString(CPtr(), GetLength()); }

	wchar_t *GetBuffer(size_t nLength = (size_t)-1);
	void ReleaseBuffer(size_t nLength = (size_t)-1);

	inline size_t GetLength() const { return m_pContent->GetLength(); }
	size_t Truncate(size_t nLength);

	inline wchar_t At(size_t nIndex) const { return m_pContent->GetData()[nIndex]; }

	inline bool IsEmpty() const { return !(m_pContent->GetLength() && *m_pContent->GetData()); }

	size_t GetCharString(char *lpszStr, size_t nSize, UINT CodePage=CP_UTF8) const;
	std::string GetMB() const;

	int __cdecl Format(const wchar_t * format, ...);

	FARString& Replace(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen);
	FARString& Replace(size_t Pos, size_t Len, const FARString& Str) { return Replace(Pos, Len, Str.CPtr(), Str.GetLength()); }
	FARString& Replace(size_t Pos, size_t Len, const wchar_t* Str) { return Replace(Pos, Len, Str, StrLength(NullToEmpty(Str))); }
	FARString& Replace(size_t Pos, size_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }

	FARString& Append(const wchar_t* Str, size_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
	FARString& Append(const FARString& Str) { return Append(Str.CPtr(), Str.GetLength()); }
	FARString& Append(const wchar_t* Str) { return Append(Str, StrLength(NullToEmpty(Str))); }
	FARString& Append(wchar_t Ch) { return Append(&Ch, 1); }
	FARString& Append(const char *lpszAdd, UINT CodePage=CP_UTF8);

	FARString& Insert(size_t Pos, const wchar_t* Str, size_t StrLen) { return Replace(Pos, 0, Str, StrLen); }
	FARString& Insert(size_t Pos, const FARString& Str) { return Insert(Pos, Str.CPtr(), Str.GetLength()); }
	FARString& Insert(size_t Pos, const wchar_t* Str) { return Insert(Pos, Str, StrLength(NullToEmpty(Str))); }
	FARString& Insert(size_t Pos, wchar_t Ch) { return Insert(Pos, &Ch, 1); }

	FARString& Copy(const wchar_t *Str, size_t StrLen) { return Replace(0, GetLength(), Str, StrLen); }
	FARString& Copy(const wchar_t *Str) { return Copy(Str, StrLength(NullToEmpty(Str))); }
	FARString& Copy(wchar_t Ch) { return Copy(&Ch, 1); }
	FARString& Copy(const FARString &Str);
	FARString& Copy(const char *lpszData, UINT CodePage=CP_UTF8);

	FARString& Remove(size_t Pos, size_t Len = 1) { return Replace(Pos, Len, nullptr, 0); }
	FARString& LShift(size_t nShiftCount, size_t nStartPos=0) { return Remove(nStartPos, nShiftCount); }

	FARString& Clear();

	inline const wchar_t *CPtr() const { return m_pContent->GetData(); }
	inline const wchar_t *CEnd() const { return m_pContent->GetData() + m_pContent->GetLength(); }
	inline operator const wchar_t *() const { return m_pContent->GetData(); }

	FARString SubStr(size_t Pos, size_t Len = -1);

	inline FARString& operator=(FARString &&strOriginal) { m_pContent = strOriginal.m_pContent; strOriginal.Init(); return *this; }
	FARString& operator=(const FARString &strCopy) { return Copy(strCopy); }
	FARString& operator=(const char *lpszData) { return Copy(lpszData); }
	FARString& operator=(const wchar_t *lpwszData) { return Copy(lpwszData); }
	FARString& operator=(wchar_t chData) { return Copy(chData); }
	FARString& operator=(const std::string &strSrc) { return Copy(strSrc.c_str(), CP_UTF8); }
	FARString& operator=(const std::wstring &strSrc) { return Copy(strSrc.c_str()); }

	FARString& operator+=(const FARString &strAdd) { return Append(strAdd); }
	FARString& operator+=(const char *lpszAdd) { return Append(lpszAdd); }
	FARString& operator+=(const wchar_t *lpwszAdd) { return Append(lpwszAdd); }
	FARString& operator+=(wchar_t chAdd) { return Append(chAdd); }

	bool Equal(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen) const;
	bool Equal(size_t Pos, const wchar_t* Str, size_t StrLen) const { return Equal(Pos, StrLen, Str, StrLen); }
	bool Equal(size_t Pos, const wchar_t* Str) const { return Equal(Pos, StrLength(Str), Str, StrLength(Str)); }
	bool Equal(size_t Pos, const FARString& Str) const { return Equal(Pos, Str.GetLength(), Str.CPtr(), Str.GetLength()); }
	bool Equal(size_t Pos, wchar_t Ch) const { return Equal(Pos, 1, &Ch, 1); }
	bool operator==(const FARString& Str) const { return Equal(0, GetLength(), Str.CPtr(), Str.GetLength()); }
	bool operator==(const wchar_t* Str) const { return Equal(0, GetLength(), Str, StrLength(Str)); }
	bool operator==(wchar_t Ch) const { return Equal(0, GetLength(), &Ch, 1); }

	bool operator!=(const FARString& Str) const { return !Equal(0, GetLength(), Str.CPtr(), Str.GetLength()); }
	bool operator!=(const wchar_t* Str) const { return !Equal(0, GetLength(), Str, StrLength(Str)); }
	bool operator!=(wchar_t Ch) const { return !Equal(0, GetLength(), &Ch, 1); }

	bool operator<(const FARString& Str) const;

	FARString& Lower(size_t nStartPos=0, size_t nLength=(size_t)-1);
	FARString& Upper(size_t nStartPos=0, size_t nLength=(size_t)-1);

	bool Pos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;
	bool Pos(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos=0) const;
	bool PosI(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos=0) const;
	bool RPos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;

	bool Contains(wchar_t Ch, size_t nStartPos=0) const { return !wcschr(m_pContent->GetData()+nStartPos,Ch) ? false : true; }
	bool Contains(const wchar_t *lpwszFind, size_t nStartPos=0) const { return !wcsstr(m_pContent->GetData()+nStartPos,lpwszFind) ? false : true; }

	bool Begins(wchar_t Ch) const { return m_pContent->GetLength() > 0 && *m_pContent->GetData() == Ch; }
	bool Begins(const wchar_t *lpwszFind) const { return m_pContent->GetLength() > 0 && wcsncmp(m_pContent->GetData(), lpwszFind, wcslen(lpwszFind)) == 0; }

	template <typename CharT>
		bool ContainsAnyOf(const CharT *needles)
	{
		return FindAnyOfChars(CPtr(), CPtr() + GetLength(), needles) != nullptr;
	}
};

FARString operator+(const FARString &strSrc1, const FARString &strSrc2);
FARString operator+(const FARString &strSrc1, const char *lpszSrc2);
FARString operator+(const FARString &strSrc1, const wchar_t *lpwszSrc2);
