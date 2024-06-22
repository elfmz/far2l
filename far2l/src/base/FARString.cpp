/*
FARString.hpp

Unicode строки
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

#include <StackHeapArray.hpp>
#include <stdarg.h>
#include <limits>
#include "lang.hpp"

/// change to 1 to enable stats & leaks detection (slow!)
#if 0
#include <mutex>
#include <set>

struct DbgStr
{
	std::mutex Mtx;
	unsigned long long Addrefs = 0;
	std::set<void *> Instances;
};

static DbgStr &DBGSTR()
{
	static DbgStr s_out;
	return s_out;
}

void FN_NOINLINE dbgStrCreated(void *c, unsigned int Capacity)
{
	std::lock_guard<std::mutex> lock(DBGSTR().Mtx);
	if (!DBGSTR().Instances.insert(c).second) { ABORT(); }
//	fprintf(stderr, "dbgStrCreated: Instances=%lu Addrefs=%llu [%u]\n",
//		(unsigned long)DBGSTR().Instances.size(), DBGSTR().Addrefs, Capacity);
}

void FN_NOINLINE dbgStrDestroyed(void *c, unsigned int Capacity)
{
	std::lock_guard<std::mutex> lock(DBGSTR().Mtx);
	if (!DBGSTR().Instances.erase(c)) { ABORT(); }
//	fprintf(stderr, "dbgStrDestroyed: Instances=%lu Addrefs=%llu [%u]\n",
//		(unsigned long)DBGSTR().Instances.size(), DBGSTR().Addrefs, Capacity);
}

void FN_NOINLINE dbgStrAddref(const wchar_t *Data)
{
	std::lock_guard<std::mutex> lock(DBGSTR().Mtx);
	++DBGSTR().Addrefs;
//	fprintf(stderr, "dbgStrAddref: Instances=%lu Addrefs=%llu '%ls'\n",
//		(unsigned long)DBGSTR().Instances.size(), DBGSTR().Addrefs, Data);
}

static std::set<void *> s_PrevStrings;

void FARString::ScanForLeaks()
{
	std::lock_guard<std::mutex> lock(DBGSTR().Mtx);
	const auto &strs = DBGSTR().Instances;
	fprintf(stderr, "========= %s: Count=%lu Addrefs=%llu\n",
		__FUNCTION__, (unsigned long)strs.size(), DBGSTR().Addrefs);

//	fprintf(stderr, "dbgStrAddref: Instances=%lu Addrefs=%llu '%ls'\n",
//		(unsigned long)DBGSTR().Instances.size(), DBGSTR().Addrefs, Data);

	if (!s_PrevStrings.empty())
	{
		for (const auto &p : strs) if (s_PrevStrings.find(p) == s_PrevStrings.end())
		{
			FARString::Content *c = (FARString::Content *)p;
			fprintf(stderr, " + refs=%u cap=%u len=%u data='%ls'\n",
				c->GetRefs(), (unsigned int)c->GetCapacity(), (unsigned int)c->GetLength(), c->GetData());
		}
	}

	s_PrevStrings = strs;
}

#else
# define dbgStrCreated(c, Capacity)
# define dbgStrDestroyed(c, Capacity)
# define dbgStrAddref(Data)
void FARString::ScanForLeaks() { }
#endif

FARString::Content FARString::Content::sEmptyData{}; // all fields zero inited by loader

FARString::Content *FARString::Content::Create(size_t nCapacity)
{
	if (UNLIKELY(!nCapacity))
		return EmptySingleton();

	Content *out = (Content *)malloc(sizeof(Content) + sizeof(wchar_t) * nCapacity);
	//Так как ни где выше в коде мы не готовы на случай что памяти не хватит
	//то уж лучше и здесь не проверять, а сразу падать
	out->m_nRefCount = 1;
	out->m_nCapacity = (unsigned int)std::min(nCapacity, (size_t)std::numeric_limits<unsigned int>::max());
	out->m_nLength = 0;
	out->m_Data[0] = 0;

	dbgStrCreated(out, out->m_nCapacity);
	return out;
}

FARString::Content *FARString::Content::Create(size_t nCapacity, const wchar_t *SrcData, size_t SrcLen)
{
	Content *out = Content::Create(nCapacity);
	out->m_nLength = (unsigned int)std::min(std::min(nCapacity, SrcLen), (size_t)std::numeric_limits<unsigned int>::max());
	wmemcpy(out->m_Data, SrcData, out->m_nLength);
	out->m_Data[out->m_nLength] = 0;
	return out;
}

void FN_NOINLINE FARString::Content::Destroy(Content *c)
{
	dbgStrDestroyed(c, c->m_nCapacity);
	ASSERT(c != EmptySingleton());
	free(c);
}

void FARString::Content::AddRef()
{
	if (LIKELY(m_nCapacity)) // only empty singletone has zero capacity
	{
		dbgStrAddref(m_Data);
		__atomic_add_fetch(&m_nRefCount, 1, __ATOMIC_RELAXED);
	}
}

void FARString::Content::DecRef()
{
	// __atomic_load_n(relaxed) usually doesn't use any HW interlocking
	// thus its a fast path for empty or single-owner strings
	unsigned int n = __atomic_load_n(&m_nRefCount, __ATOMIC_RELAXED);
	if (LIKELY(n == 0))
	{  // (only) empty singletone has always-zero m_nRefCount
		return;
	}

	if (LIKELY(n != 1))
	{
		if (LIKELY(__atomic_sub_fetch(&m_nRefCount, 1, __ATOMIC_RELEASE) != 0))
			return;
	}

	Destroy(this);
}

void FARString::Content::SetLength(size_t nLength)
{
	m_nLength = std::min(nLength, (size_t)m_nCapacity);
	m_Data[m_nLength] = 0;
}

//////////////////////////

void FARString::PrepareForModify(size_t nCapacity)
{
	if (m_pContent->GetRefs() != 1 || nCapacity > m_pContent->GetCapacity())
	{
		size_t NewCapacity = nCapacity;
		if (m_pContent->GetCapacity() > 0 && NewCapacity > m_pContent->GetCapacity())
			NewCapacity+= std::max(NewCapacity - m_pContent->GetCapacity(), (size_t)16);

		Content *pNewData =
			Content::Create(NewCapacity, m_pContent->GetData(), m_pContent->GetLength());
		pNewData->SetLength(m_pContent->GetLength());
		m_pContent->DecRef();
		m_pContent = pNewData;
	}
}

void FARString::PrepareForModify()
{
	// invoke PrepareForModify with current length but not capacity intentionally
	// so in case of single owner this will not alter things but in case of
	// not single owner then new copy will be allocated with capacity == length
	PrepareForModify(m_pContent->GetLength());
}

size_t FARString::GetCharString(char *lpszStr, size_t nSize, UINT CodePage) const
{
	if (!lpszStr || !nSize)
		return 0;

	int SrcLen;
	for (SrcLen = (int)m_pContent->GetLength(); SrcLen; --SrcLen)
	{
		int DstLen = WINPORT(WideCharToMultiByte)(CodePage, 0,
			m_pContent->GetData(), SrcLen, nullptr, 0, nullptr, nullptr);
		if (DstLen < (int)nSize)
		{
			DstLen = WINPORT(WideCharToMultiByte)(CodePage, 0,
				m_pContent->GetData(), SrcLen, lpszStr, (int)nSize, nullptr, nullptr);
			if (DstLen >= 0 && DstLen < (int)nSize)
			{
				lpszStr[DstLen] = 0;
				return DstLen + 1;
			}
		}
	}

	*lpszStr = 0;
	return 1;
}

FARString& FARString::Replace(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen)
{
	// Pos & Len must be valid
	ASSERT(Pos <= m_pContent->GetLength());
	ASSERT(Len <= m_pContent->GetLength());
	ASSERT(Pos + Len <= m_pContent->GetLength());

	// Data and *this must not intersect (but Data can be located entirely within *this)
	const bool DataStartsInside = (Data >= CPtr() && Data < CEnd());
	const bool DataEndsInside = (Data + DataLen > CPtr() && Data + DataLen <= CEnd());
	ASSERT(DataStartsInside == DataEndsInside);

	if (UNLIKELY(!Len && !DataLen))
		return *this;

	const size_t NewLength = m_pContent->GetLength() + DataLen - Len;
	if (!DataStartsInside && m_pContent->GetRefs() == 1 && NewLength <= m_pContent->GetCapacity())
	{
		wmemmove(m_pContent->GetData() + Pos + DataLen,
			m_pContent->GetData() + Pos + Len, m_pContent->GetLength() - Pos - Len);
		wmemcpy(m_pContent->GetData() + Pos, Data, DataLen);
	}
	else
	{
		Content *NewData = Content::Create(NewLength);
		wmemcpy(NewData->GetData(), m_pContent->GetData(), Pos);
		wmemcpy(NewData->GetData() + Pos, Data, DataLen);
		wmemcpy(NewData->GetData() + Pos + DataLen,
			m_pContent->GetData() + Pos + Len, m_pContent->GetLength() - Pos - Len);
		m_pContent->DecRef();
		m_pContent = NewData;
	}

	m_pContent->SetLength(NewLength);

	return *this;
}

FARString& FARString::Replace(size_t Pos, size_t Len, const wchar_t Ch, size_t Count)
{
	// Pos & Len must be valid
	ASSERT(Pos <= m_pContent->GetLength());
	ASSERT(Len <= m_pContent->GetLength());
	ASSERT(Pos + Len <= m_pContent->GetLength());

	if (UNLIKELY(!Len && !Count))
		return *this;

	const size_t NewLength = m_pContent->GetLength() + Count - Len;
	if (m_pContent->GetRefs() == 1 && NewLength <= m_pContent->GetCapacity())
	{
		wmemmove(m_pContent->GetData() + Pos + Count,
			m_pContent->GetData() + Pos + Len, m_pContent->GetLength() - Pos - Len);
		wmemset(m_pContent->GetData() + Pos, Ch, Count);
	}
	else
	{
		Content *NewData = Content::Create(NewLength);
		wmemcpy(NewData->GetData(), m_pContent->GetData(), Pos);
		wmemset(NewData->GetData() + Pos, Ch, Count);
		wmemcpy(NewData->GetData() + Pos + Count,
			m_pContent->GetData() + Pos + Len, m_pContent->GetLength() - Pos - Len);
		m_pContent->DecRef();
		m_pContent = NewData;
	}

	m_pContent->SetLength(NewLength);

	return *this;
}

wchar_t FARString::ReplaceChar(size_t Pos, wchar_t Ch)
{
	ASSERT(Pos < m_pContent->GetLength());
	PrepareForModify();
	std::swap(m_pContent->GetData()[Pos], Ch);
	return Ch;
}

FARString& FARString::Append(const char *lpszAdd, UINT CodePage)
{
	if (lpszAdd && *lpszAdd)
	{
		int nAddSize = WINPORT(MultiByteToWideChar)(CodePage,0,lpszAdd,-1,nullptr,0);
		if (nAddSize > 0) {
			size_t nNewLength = m_pContent->GetLength() + nAddSize - 1; // minus NUL char that implicitly there
			PrepareForModify(nNewLength);
			WINPORT(MultiByteToWideChar)(CodePage, 0, lpszAdd, -1, m_pContent->GetData() + m_pContent->GetLength(), nAddSize);
			m_pContent->SetLength(nNewLength);
		}
	}

	return *this;
}

FARString& FARString::Copy(const FARString &Str)
{
	auto prev_pContent = m_pContent;
	m_pContent = Str.m_pContent;
	m_pContent->AddRef();
	prev_pContent->DecRef();

	return *this;
}

FARString& FARString::Copy(const char *lpszData, UINT CodePage)
{
	m_pContent->DecRef();

	if (!lpszData || !*lpszData)
	{
		Init();
	}
	else
	{
		int nSize = WINPORT(MultiByteToWideChar)(CodePage,0,lpszData,-1,nullptr,0);
		if (nSize > 0) {
			m_pContent = Content::Create(nSize - 1);
			WINPORT(MultiByteToWideChar)(CodePage, 0, lpszData, -1, m_pContent->GetData(), nSize);
			m_pContent->SetLength(nSize - 1);

		} else 
			Init();
	}

	return *this;
}

FARString FARString::SubStr(size_t Pos, size_t Len)
{
	if (Pos >= GetLength())
		return FARString();
	if (Len == static_cast<size_t>(-1) || Len > GetLength() || Pos + Len > GetLength())
		Len = GetLength() - Pos;
	return FARString(m_pContent->GetData() + Pos, Len);
}

bool FARString::Equal(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen) const
{
	if (Pos >= m_pContent->GetLength())
		Len = 0;
	else if (Len >= m_pContent->GetLength() || Pos + Len >= m_pContent->GetLength())
		Len = m_pContent->GetLength() - Pos;

	if (Len != DataLen)
		return false;

	return !wmemcmp(m_pContent->GetData() + Pos, Data, Len);
}

bool FARString::operator<(const FARString& Str) const
{
	return wmemcmp(CPtr(), Str.CPtr(), std::min(GetLength(), Str.GetLength()) + 1 ) < 0;
}

FARString operator+(const FARString &strSrc1, const FARString &strSrc2)
{
	FARString out = strSrc1.Clone();
	out.Append(strSrc2);
	return out;
}

FARString operator+(const FARString &strSrc1, const char *lpszSrc2)
{
	FARString out = strSrc1.Clone();
	out.Append(lpszSrc2);
	return out;
}

FARString operator+(const FARString &strSrc1, const wchar_t *lpwszSrc2)
{
	FARString out = strSrc1.Clone();
	out.Append(lpwszSrc2);
	return out;
}

wchar_t *FARString::GetBuffer(size_t nLength)
{
	if (nLength == (size_t)-1)
		nLength = m_pContent->GetLength();

	PrepareForModify(nLength);
	return m_pContent->GetData();
}

void FARString::ReleaseBuffer(size_t nLength)
{
	if (nLength == (size_t)-1)
		nLength = wcsnlen(m_pContent->GetData(), m_pContent->GetCapacity());
	else if (nLength > m_pContent->GetCapacity())
		nLength = m_pContent->GetCapacity();

	PrepareForModify(nLength);
	m_pContent->SetLength(nLength);
}

size_t FARString::Truncate(size_t nLength)
{
	if (nLength < m_pContent->GetLength())
	{
		if (!nLength)
		{
			m_pContent->DecRef();
			Init();
		}
		else
		{
			PrepareForModify(nLength);
			m_pContent->SetLength(nLength);
		}
	}

	return m_pContent->GetLength();
}

FARString& FARString::Clear()
{
	Truncate(0);

	return *this;
}

void FARString::Reserve(size_t DesiredCapacity)
{
	if (DesiredCapacity > m_pContent->GetCapacity())
	{
		PrepareForModify(DesiredCapacity);
	}
}

void FARStringFmtV(FARString &str, bool append, const wchar_t *format, va_list argptr)
{
	const size_t StartSize = 0x200;		// 512 chars ought to be enough for mosts
	const size_t MaxSize = 0x1000000;	// 16 millions ought to be enough for anybody
	size_t Size;

	for (Size = StartSize; Size <= MaxSize; Size<<= 1)
	{
		StackHeapArray<wchar_t, StartSize> buf(Size);
		if (UNLIKELY(!buf.Get()))
			break;

		va_list argptr_copy;
		va_copy(argptr_copy, argptr);
		int retValue = vswprintf(buf.Get(), Size, format, argptr_copy);
		va_end(argptr_copy);

		if (retValue >= 0 && size_t(retValue) < Size)
		{
			if (append)
				str.Append(buf.Get(), retValue);
			else
				str.Copy(buf.Get(), retValue);
			return;
		}
	}

	fprintf(stderr, "%s: failed; Size=%lu format='%ls'\n", __FUNCTION__, (unsigned long)Size, format);
}

FARString& FARString::Format(const wchar_t * format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	FARStringFmtV(*this, false, format, argptr);
	va_end(argptr);
	return *this;
}

FARString& FARString::AppendFormat(const wchar_t * format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	FARStringFmtV(*this, true, format, argptr);
	va_end(argptr);
	return *this;
}

FARString& FARString::Lower(size_t nStartPos, size_t nLength)
{
	PrepareForModify();
	WINPORT(CharLowerBuff)(m_pContent->GetData() + nStartPos,
		DWORD((nLength == (size_t)-1) ? m_pContent->GetLength() - nStartPos : nLength));
	return *this;
}

FARString& FARString::Upper(size_t nStartPos, size_t nLength)
{
	PrepareForModify();
	WINPORT(CharUpperBuff)(m_pContent->GetData() + nStartPos,
		DWORD((nLength == (size_t)-1) ? m_pContent->GetLength() - nStartPos : nLength));
	return *this;
}

bool FARString::Pos(size_t &nPos, wchar_t Ch, size_t nStartPos) const
{
	const wchar_t *lpwszData = m_pContent->GetData();
	const wchar_t *lpFound = wcschr(lpwszData + nStartPos, Ch);

	if (lpFound)
	{
		nPos = lpFound - lpwszData;
		return true;
	}

	return false;
}

bool FARString::Pos(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos) const
{
	const wchar_t *lpwszData = m_pContent->GetData();
	const wchar_t *lpFound = wcsstr(lpwszData + nStartPos, lpwszFind);

	if (lpFound)
	{
		nPos = lpFound - lpwszData;
		return true;
	}

	return false;
}

bool FARString::PosI(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos) const
{
	const wchar_t *lpwszData = m_pContent->GetData();
	const wchar_t *lpFound = StrStrI(lpwszData + nStartPos, lpwszFind);

	if (lpFound)
	{
		nPos = lpFound - lpwszData;
		return true;
	}

	return false;
}

bool FARString::RPos(size_t &nPos, wchar_t Ch, size_t nStartPos) const
{
	const wchar_t *lpwszData = m_pContent->GetData();
	const wchar_t *lpwszStrStart = lpwszData + nStartPos;
	const wchar_t *lpwszStrScan = lpwszData + m_pContent->GetLength();

	for (;;--lpwszStrScan)
	{
		if (*lpwszStrScan == Ch)
		{
			nPos = lpwszStrScan - lpwszData;
			return true;
		}

		if (lpwszStrScan == lpwszStrStart)
			return false;
	}
}

std::string FARString::GetMB() const
{
	std::string out;
	Wide2MB(CPtr(), GetLength(), out);
	return out;
}

bool FARString::Contains(wchar_t Ch, size_t nStartPos) const
{
	return wcschr(m_pContent->GetData() + nStartPos, Ch) != nullptr;
}

bool FARString::Contains(const wchar_t *lpwszFind, size_t nStartPos) const
{
	return wcsstr(m_pContent->GetData() + nStartPos, lpwszFind) != nullptr;
}

bool FARString::Begins(const FARString &strFind) const
{
	return m_pContent->GetLength() >= strFind.m_pContent->GetLength()
		&& wmemcmp(m_pContent->GetData(),
			strFind.m_pContent->GetData(), strFind.m_pContent->GetLength()) == 0;
}

bool FARString::Begins(const wchar_t *lpwszFind) const
{
	const size_t nFind = wcslen(lpwszFind);

	return m_pContent->GetLength() >= nFind
		&& wcsncmp(m_pContent->GetData(), lpwszFind, nFind) == 0;
}

bool FARString::Ends(const FARString &strFind) const
{
	return m_pContent->GetLength() >= strFind.m_pContent->GetLength()
		&& wmemcmp(m_pContent->GetData() + m_pContent->GetLength() - strFind.m_pContent->GetLength(),
			strFind.m_pContent->GetData(), strFind.m_pContent->GetLength()) == 0;
}

bool FARString::Ends(const wchar_t *lpwszFind) const
{
	const size_t nFind = wcslen(lpwszFind);

	return m_pContent->GetLength() >= nFind
		&& wmemcmp(m_pContent->GetData() + m_pContent->GetLength() - nFind, lpwszFind, nFind) == 0;
}


size_t FARString::CellsCount() const
{
	return StrCellsCount(CPtr(), GetLength());
}

size_t FARString::TruncateByCells(size_t nCount)
{
	size_t ng = nCount;
	size_t sz = StrSizeOfCells(CPtr(), GetLength(), ng, false);
	Truncate(sz);
	return ng;
}
