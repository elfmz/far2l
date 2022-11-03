#pragma once

/*
cache.hpp

Кеширование записи в файл/чтения из файла
*/
/*
Copyright (c) 2009 Far Group
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

#include <string>
#include <WinCompat.h>
#include "farwinapi.hpp"

class BufferedFileView
{
	BufferedFileView(const BufferedFileView&) = delete;
public:
	BufferedFileView();
	~BufferedFileView();

	bool Open(const std::string &PathName);
	void Close();

	bool Opened() const { return FD != -1; }

	void ActualizeFileSize();

	void SetPointer(INT64 Ptr, int Whence = SEEK_SET);
	inline void GetPointer(INT64 &Ptr) const { Ptr = CurPtr; }
	inline bool GetSize(UINT64& Size) const { Size = FileSize; return Opened(); };
	inline bool Eof() const { return CurPtr >= FileSize; }

	void Clear();

	DWORD Read(void *Buf, DWORD Size);

	LPBYTE ViewBytesSlide(DWORD &Size);
	LPBYTE ViewBytesAt(UINT64 Ptr, DWORD &Size);


private:
	enum
	{
		AlignSize = 0x1000, // must be power of 2
		AheadCount = 0x10,
		BehindCount = 0x1,
		CapacityStock = 0x4
	};

	struct Bounds
	{
		UINT64 Ptr = 0;  // must be multiple of AlignSize
		UINT64 End = 0;
	} BufferBounds;

	int FD = -1;

	LPBYTE Buffer = nullptr;
	DWORD BufferSize = 0;
	UINT64 CurPtr = 0, LastPtr = 0;
	UINT64 FileSize = 0;
	// something from /proc/ that has zero stat size but still can read some content from it
	bool PseudoFile = false;


	template <class T>
		inline T AlignUp(T v)
	{
		T a = v & (T)(AlignSize - 1);
		return a ? v + (AlignSize - a) : v;
	}

	template <class T>
		inline T AlignDown(T v)
	{
		return v & (~(T)(AlignSize - 1));
	}

	DWORD DirectReadAt(UINT64 Ptr, LPVOID Data, DWORD DataSize);
	LPBYTE AllocBuffer(size_t Size);

	void CalcBufferBounds(Bounds &bi, UINT64 Ptr, DWORD DataSize, DWORD CountLefter, DWORD CountRighter);


};


class CachedWrite
{
public:
	CachedWrite(File& file);
	~CachedWrite();
	bool Write(LPCVOID Data, DWORD DataSize);
	bool Flush();

private:
	LPBYTE Buffer;
	File& file;
	enum {BufferSize=0x10000};
	DWORD FreeSize;
	bool Flushed;

};
