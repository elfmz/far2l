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

#include <sys/types.h>
#include <string.h>
#include <string>
#include <cctweaks.h>
#include <WinCompat.h>
#include "locale.hpp"
#include "lang.hpp"

inline const wchar_t *NullToEmpty(const wchar_t *s) { return s ? s : L""; }

/***********************************************************************************************************
	This is a FARString - homegrew reference-counted string widely used in this project.
	This puts some restrictions on how this string may be used:
		- Don't write into const pointer returned by CPtr(), doing that may affect data used by irrelevant code.
		Use GetBuffer/ReleaseBuffer or simple plain assignment instead.
		- Don't modify single FARString instance from different threads without serialization.
		Create per-thread copies of FARString and modifying them from that threads is perfectly fine however.
		- Avoid excessive copying. While its doesn't copy string content, it performs still slow HW-interlocked
		reference counter manipulations, so better use passing-by-reference and std::move where possible.
*/

class FARString
{
	/*
		<Content> represents actual content of string that may be shared across different FARStrings
		and it must be trivial cuz there is sEmptyData singletone shared across all empty strings and
		thus it may be accessed during early static objects initialization; using Meyer's singletone
		implementation for it would cause compiler to include local static strapping code that has some
		runtime overhead. Using trivial class makes possible just to create global static variable that
		will be resided in BSS and thus will have all fields zero-initialized by linker and ready to use
		just upon app loading without using initializing c-tor. This is also main reason why cannot use
		virtual methods, std::atomic<> instead of __atomic_* etc
	*/

	class Content
	{
		static Content sEmptyData; //для оптимизации создания пустых FARString

		volatile unsigned int m_nRefCount;
		unsigned int m_nCapacity;	// not including final NULL char
		unsigned int m_nLength;		// not including final NULL char
		wchar_t m_Data[1];

		static void Destroy(Content *c);

	public:
		static inline Content *EmptySingleton() { return &sEmptyData; }

		static Content *Create(size_t nCapacity);
		static Content *Create(size_t nCapacity, const wchar_t *SrcData, size_t SrcLen);

		void AddRef();
		void DecRef();
		unsigned int GetRefs() const { return __atomic_load_n(&m_nRefCount, __ATOMIC_RELAXED); }

		void SetLength(size_t nLength);
		inline wchar_t *GetData() { return m_Data; }
		inline size_t GetCapacity() const { return m_nCapacity; }
		inline size_t GetLength() const { return m_nLength; }

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
	static void ScanForLeaks();

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

	// Use to avoid reallocations if final length of string is known
	void Reserve(size_t DesiredCapacity);

	wchar_t *GetBuffer(size_t nLength = (size_t)-1);
	void ReleaseBuffer(size_t nLength = (size_t)-1);

	inline size_t GetLength() const { return m_pContent->GetLength(); }
	size_t Truncate(size_t nLength);

	size_t CellsCount() const;
	size_t TruncateByCells(size_t nCount);

	inline wchar_t At(size_t nIndex) const { return m_pContent->GetData()[nIndex]; }

	inline bool IsEmpty() const { return !(m_pContent->GetLength() && *m_pContent->GetData()); }

	size_t GetCharString(char *lpszStr, size_t nSize, UINT CodePage=CP_UTF8) const;
	std::string GetMB() const;
	inline std::wstring GetWide() const { return std::wstring(CPtr(), GetLength()); }

	FARString& Format(const wchar_t *format, ...);
	FARString& AppendFormat(const wchar_t *format, ...);

	FARString& Replace(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen);
	FARString& Replace(size_t Pos, size_t Len, const FARString& Str) { return Replace(Pos, Len, Str.CPtr(), Str.GetLength()); }
	FARString& Replace(size_t Pos, size_t Len, const wchar_t* Str) { return Replace(Pos, Len, Str, StrLength(NullToEmpty(Str))); }
	FARString& Replace(size_t Pos, size_t Len, wchar_t Ch, size_t Count = 1);

	wchar_t ReplaceChar(size_t Pos, wchar_t Ch);

	FARString& Append(const wchar_t* Str, size_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
	FARString& Append(const FARString& Str) { return Append(Str.CPtr(), Str.GetLength()); }
	FARString& Append(const wchar_t* Str) { return Append(Str, StrLength(NullToEmpty(Str))); }
	FARString& Append(wchar_t Ch, size_t Count = 1) { return Replace(GetLength(), 0, Ch, Count); } 
	FARString& Append(const char *lpszAdd, UINT CodePage=CP_UTF8);

	FARString& Insert(size_t Pos, const wchar_t* Str, size_t StrLen) { return Replace(Pos, 0, Str, StrLen); }
	FARString& Insert(size_t Pos, const FARString& Str) { return Insert(Pos, Str.CPtr(), Str.GetLength()); }
	FARString& Insert(size_t Pos, const wchar_t* Str) { return Insert(Pos, Str, StrLength(NullToEmpty(Str))); }
	FARString& Insert(size_t Pos, wchar_t Ch, size_t Count = 1) { return Replace(Pos, 0, Ch, Count); } 

	FARString& Copy(const wchar_t *Str, size_t StrLen) { return Replace(0, GetLength(), Str, StrLen); }
	FARString& Copy(const wchar_t *Str) { return Copy(Str, StrLength(NullToEmpty(Str))); }
	FARString& Copy(wchar_t Ch) { return Copy(&Ch, 1); }
	FARString& Copy(const FARString &Str);
	FARString& Copy(const char *lpszData, UINT CodePage=CP_UTF8);

	template <typename ARRAY_T>
		FARString& CopyArray(const ARRAY_T &a)
	{
		static_assert ( sizeof(a) != sizeof(void *), "CopyArray should be used with arrays but not pointers");
		return Copy(a, tnzlen(a, ARRAYSIZE(a)));
	}


	FARString& Remove(size_t Pos, size_t Len = 1) { return Replace(Pos, Len, nullptr, 0); }
	FARString& LShift(size_t nShiftCount, size_t nStartPos=0) { return Remove(nStartPos, nShiftCount); }

	FARString& Clear();

	inline const wchar_t *CPtr() const { return m_pContent->GetData(); }
	inline const wchar_t *CEnd() const { return m_pContent->GetData() + m_pContent->GetLength(); }
	inline operator const wchar_t *() const { return m_pContent->GetData(); }

	FARString SubStr(size_t Pos, size_t Len = -1);

	inline FARString& operator=(FARString &&strOriginal) { std::swap(m_pContent, strOriginal.m_pContent); return *this; }
	inline FARString& operator=(const FARString &strCopy) { return Copy(strCopy); }
	inline FARString& operator=(const char *lpszData) { return Copy(lpszData); }
	inline FARString& operator=(const wchar_t *lpwszData) { return Copy(lpwszData); }
	inline FARString& operator=(wchar_t chData) { return Copy(chData); }
	inline FARString& operator=(const std::string &strSrc) { return Copy(strSrc.c_str(), CP_UTF8); }
	inline FARString& operator=(const std::wstring &strSrc) { return Copy(strSrc.c_str()); }

	inline FARString& operator+=(const FARString &strAdd) { return Append(strAdd); }
	inline FARString& operator+=(const char *lpszAdd) { return Append(lpszAdd); }
	inline FARString& operator+=(const wchar_t *lpwszAdd) { return Append(lpwszAdd); }
	inline FARString& operator+=(wchar_t chAdd) { return Append(chAdd); }

	bool Equal(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen) const;
	inline bool Equal(size_t Pos, const wchar_t* Str, size_t StrLen) const { return Equal(Pos, StrLen, Str, StrLen); }
	inline bool Equal(size_t Pos, const wchar_t* Str) const { return Equal(Pos, StrLength(Str), Str, StrLength(Str)); }
	inline bool Equal(size_t Pos, const FARString& Str) const { return Equal(Pos, Str.GetLength(), Str.CPtr(), Str.GetLength()); }
	inline bool Equal(size_t Pos, wchar_t Ch) const { return Equal(Pos, 1, &Ch, 1); }
	inline bool operator==(const FARString& Str) const { return Equal(0, GetLength(), Str.CPtr(), Str.GetLength()); }
	inline bool operator==(const wchar_t* Str) const { return Equal(0, GetLength(), Str, StrLength(Str)); }
	inline bool operator==(wchar_t Ch) const { return Equal(0, GetLength(), &Ch, 1); }

	inline bool operator!=(const FARString& Str) const { return !Equal(0, GetLength(), Str.CPtr(), Str.GetLength()); }
	inline bool operator!=(const wchar_t* Str) const { return !Equal(0, GetLength(), Str, StrLength(Str)); }
	inline bool operator!=(wchar_t Ch) const { return !Equal(0, GetLength(), &Ch, 1); }

	bool operator<(const FARString& Str) const;

	FARString& Lower(size_t nStartPos=0, size_t nLength=(size_t)-1);
	FARString& Upper(size_t nStartPos=0, size_t nLength=(size_t)-1);

	bool Pos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;
	bool Pos(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos=0) const;
	bool PosI(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos=0) const;
	bool RPos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;

	bool Contains(wchar_t Ch, size_t nStartPos = 0) const;
	bool Contains(const wchar_t *lpwszFind, size_t nStartPos = 0) const;

	inline bool Begins(wchar_t Ch) const
		{ return m_pContent->GetLength() > 0 && *m_pContent->GetData() == Ch; }

	bool Begins(const FARString &strFind) const;
	bool Begins(const wchar_t *lpwszFind) const;

	inline bool Ends(wchar_t Ch) const
		{ return m_pContent->GetLength() > 0 && m_pContent->GetData()[m_pContent->GetLength() - 1] == Ch; }

	inline bool Ends(const FARString &strFind) const;
	inline bool Ends(const wchar_t *lpwszFind) const;

	template <typename CharT>
		bool ContainsAnyOf(const CharT *needles)
	{
		return FindAnyOfChars(CPtr(), CPtr() + GetLength(), needles) != nullptr;
	}
};

FARString operator+(const FARString &strSrc1, const FARString &strSrc2);
FARString operator+(const FARString &strSrc1, const char *lpszSrc2);
FARString operator+(const FARString &strSrc1, const wchar_t *lpwszSrc2);

void FARStringFmtV(FARString &str, bool append, const wchar_t *format, va_list argptr);
