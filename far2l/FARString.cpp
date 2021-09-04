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


FARString::Content FARString::Content::sEmptyData{}; // all fields zero inited by loader

FARString::Content *FARString::Content::Create(size_t nCapacity)
{
	Content *out = (Content *)malloc(sizeof(Content) + sizeof(wchar_t) * nCapacity);
	//Так как ни где выше в коде мы не готовы на случай что памяти не хватит
	//то уж лучше и здесь не проверять, а сразу падать
	out->m_nRefCount = 0; // zero means single owner
	out->m_nCapacity = (unsigned int)std::min(nCapacity, (size_t)std::numeric_limits<unsigned int>::max());
	out->m_nLength = 0;
	out->m_Data[0] = 0;
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

void FARString::Content::Destroy(Content *data)
{
	// empty string may be implicitely shared across different threads
	// that will cause race condition on modifying its m_nRefCount
	// THIS CONSIDERED NORMAL :) and just skip any destroy that could
	// come for it - it should never ever been destroyed.
	if (LIKELY(data != EmptySingleton()))
	{
		free(data);
	}
	else
		data->m_nRefCount = 0x1000; // prevent such occasions to happen frequently
}

void FARString::Content::SetLength(size_t nLength)
{
	m_nLength = std::min(nLength, (size_t)m_nCapacity);
	m_Data[m_nLength] = 0;
}

//////////////////////////

void FARString::PrepareForModify(size_t nCapacity)
{
	if (!m_pContent->SingleOwner() || nCapacity > m_pContent->GetCapacity())
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
	// invoke PrepareForModify with current length but not capacity intentially
	// so in case of single owner this will not alter things but in case of
	// not single owner then new copy will be alloced with capacity == length
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
	assert(Pos <= m_pContent->GetLength());
	assert(Len <= m_pContent->GetLength());
	assert(Pos + Len <= m_pContent->GetLength());

	// Data and *this must not intersect (but Data can be located entirely within *this)
	const bool DataStartsInside = (Data >= CPtr() && Data < CEnd());
	const bool DataEndsInside = (Data + DataLen > CPtr() && Data + DataLen <= CEnd());
	assert(DataStartsInside == DataEndsInside);

	if (!Len && !DataLen)
		return *this;

	const size_t NewLength = m_pContent->GetLength() + DataLen - Len;
	if (!DataStartsInside && m_pContent->SingleOwner() && NewLength <= m_pContent->GetCapacity())
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
		wmemcpy(NewData->GetData() + Pos + DataLen, m_pContent->GetData() + Pos + Len, m_pContent->GetLength() - Pos - Len);
		m_pContent->DecRef();
		m_pContent = NewData;
	}

	m_pContent->SetLength(NewLength);

	return *this;
}

FARString& FARString::Append(const char *lpszAdd, UINT CodePage)
{
	if (lpszAdd && *lpszAdd)
	{
		int nAddSize = WINPORT(MultiByteToWideChar)(CodePage,0,lpszAdd,-1,nullptr,0);
		if (nAddSize > 0) {
			size_t nNewLength = m_pContent->GetLength() + nAddSize - 1; // minus NUL char that implicitely there
			PrepareForModify(nNewLength);
			WINPORT(MultiByteToWideChar)(CodePage, 0, lpszAdd, -1, m_pContent->GetData() + m_pContent->GetLength(), nAddSize);
			m_pContent->SetLength(nNewLength);
		}
	}

	return *this;
}

FARString& FARString::Copy(const FARString &Str)
{
	if (Str.m_pContent != m_pContent)
	{
		m_pContent->DecRef();
		m_pContent = Str.m_pContent;
		m_pContent->AddRef();
	}

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

FARString FARString::SubStr(size_t Pos, size_t Len) {
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
	return FARString(strSrc1).Append(strSrc2);
}

FARString operator+(const FARString &strSrc1, const char *lpszSrc2)
{
	return FARString(strSrc1).Append(lpszSrc2);
}

FARString operator+(const FARString &strSrc1, const wchar_t *lpwszSrc2)
{
	return FARString(strSrc1).Append(lpwszSrc2);
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

int __cdecl FARString::Format(const wchar_t * format, ...)
{
	int retValue;

	va_list argptr;
	va_start(argptr, format);

	for (size_t Size = 0x100; Size; Size<<= 1)
	{
		StackHeapArray<wchar_t, 0x200> buf(Size);
		if (!buf.Get())
		{
			retValue = -1;
			break;
		}

		va_list argptr_copy;
		va_copy(argptr_copy, argptr);
		retValue = vswprintf(buf.Get(), Size, format, argptr_copy);
		va_end(argptr_copy);

		if (retValue >= 0 && size_t(retValue) < Size)
		{
			Copy(buf.Get(), retValue);
			break;
		}
	}
	va_end(argptr);

	return retValue;
}

FARString& FARString::Lower(size_t nStartPos, size_t nLength)
{
	PrepareForModify();
	WINPORT(CharLowerBuff)(m_pContent->GetData() + nStartPos,
		DWORD((nLength == (size_t)-1) ? m_pContent->GetLength() - nStartPos : nLength));
	return *this;
}

FARString&  FARString::Upper(size_t nStartPos, size_t nLength)
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

	for (;;lpwszStrScan--)
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
