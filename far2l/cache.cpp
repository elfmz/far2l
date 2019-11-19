/*
cache.cpp

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

#include "headers.hpp"

#include "cache.hpp"

CachedRead::CachedRead(FileSeekDefer& file):
	file(file),
	Buffer(nullptr),
	BufferPtr(-1),
	BufferEnd(-1),
	LastReadPtr(-1)
{
	if (BufferSize) {
		if (posix_memalign((void **)&Buffer, AlignSize, BufferSize) != 0) {
			Buffer = (LPBYTE)malloc(BufferSize);
		}
	}
}

CachedRead::~CachedRead()
{
	free(Buffer);
}

void CachedRead::Clear()
{
	BufferPtr = -1;
	BufferEnd = -1;
}

DWORD CachedRead::Read(LPVOID Data, DWORD DataSize)
{
	INT64 Ptr = 0;
	if (!DirectTell(Ptr)) {
		return 0;
	}

	DWORD BytesRead = ReadAt(Ptr, Data, DataSize);

	if (!DirectSeek(Ptr + BytesRead)) {
		fprintf(stderr,
			"CachedRead::Read: failed to seek at %ld + %ld\n",
			(unsigned long)Ptr, (unsigned long)BytesRead);
	}

	return BytesRead;
}

bool CachedRead::ReadByte(LPBYTE Data)
{
	INT64 Ptr = 0;
	if (!DirectTell(Ptr)) {
		return false;
	}

	if (Ptr >= BufferPtr && Ptr < BufferEnd) {
		LastReadPtr = Ptr;
		*Data = Buffer[(size_t)(Ptr - BufferPtr)];
		DirectSeek(Ptr + 1);
		return true;
	}

	BufferDirection Direction = (Ptr >= LastReadPtr) ? BufferForward : BufferBackward;
	LastReadPtr = Ptr;

	DWORD ReadSize = RefreshReadAt(Ptr, Data, 1, Direction);

	DirectSeek(Ptr + ReadSize);

	return ReadSize > 0;
}

DWORD CachedRead::ReadAt(INT64 Ptr, LPVOID Data, DWORD DataSize)
{
//	return DirectReadAt(Ptr, Data, DataSize);//debug

	BufferDirection Direction = (Ptr >= LastReadPtr) ? BufferForward : BufferBackward;

	LastReadPtr = Ptr;

	INT64 End = Ptr + DataSize;
	DWORD BytesRead = 0;
#if 1
	if (BufferPtr != BufferEnd) {
		INT64 IntersectPtr = std::max(Ptr, BufferPtr);
		INT64 IntersectEnd = std::min(End, BufferEnd);
		if (IntersectPtr < IntersectEnd) {
			DWORD CpyLen = (DWORD)(IntersectEnd - IntersectPtr);
			memcpy((char *)Data + (size_t)(IntersectPtr - Ptr), &Buffer[(size_t)(IntersectPtr - BufferPtr)], CpyLen);
			BytesRead+= CpyLen;

			if (Direction == BufferForward) { // forward buffering:
				if (Ptr < IntersectPtr) { // directly read prior-buffered fragment
					DWORD BytesReadDirectly = DirectReadAt(Ptr, Data, (DWORD)(IntersectPtr - Ptr));
					if (BytesReadDirectly != (DWORD)(IntersectPtr - Ptr)) {
						// discard data fetched from buffer since prior fragment is truncated
						return BytesReadDirectly;
					}
					BytesRead+= BytesReadDirectly;
				}
				Data = (char *)Data + BytesRead;
				Ptr = IntersectEnd;

			} else { // backward buffering: 
				if (End > IntersectEnd) { // try to directly read post-buffered fragment
					DWORD BytesReadDirectly = DirectReadAt(IntersectEnd,
						(char *)Data + (size_t)(IntersectEnd - Ptr), (DWORD)(End - IntersectEnd));
					// dont care if this operation returned less or even 0
					BytesRead+= BytesReadDirectly;
				}
				End = IntersectPtr;
			}

			DataSize-= BytesRead;
		}
	}
#endif
	if (DataSize == 0) {
		return BytesRead;
	}

	DWORD RefreshRead = RefreshReadAt(Ptr, Data, DataSize, Direction);

	if (RefreshRead != DataSize && Direction == BufferBackward) {
		// if failed to directly read full fragment prior buffered fetch
		// then results of buffered fetch must be discared
		return RefreshRead;
	}

	return BytesRead + RefreshRead;
}

DWORD CachedRead::RefreshReadAt(INT64 Ptr, LPVOID Data, DWORD DataSize, BufferDirection Direction)
{
	if (DataSize >= BufferSize - AlignSize) {
		return DirectReadAt(Ptr, Data, DataSize);
	}

	INT64 End = Ptr + DataSize;

	if (Direction == BufferForward) { // forward buffering:
		// Buffer layout: [1..AlignSize|DataSize|BufferSize - AlignSize - DataSize]

		INT64 NewBufferPtr = Ptr & ~(INT64)(AlignSize - 1);
		if (NewBufferPtr == Ptr && NewBufferPtr != 0) {
			NewBufferPtr-= AlignSize;
		}

		DWORD BytesReadDirectly;

		if (NewBufferPtr >= BufferPtr && NewBufferPtr + AlignSize <= BufferEnd) {
			memmove(Buffer, &Buffer[NewBufferPtr - BufferPtr], AlignSize);
			BytesReadDirectly = AlignSize;
			BytesReadDirectly+= DirectReadAt(NewBufferPtr + AlignSize, &Buffer[AlignSize], BufferSize - AlignSize);
		} else {
			BytesReadDirectly = DirectReadAt(NewBufferPtr, Buffer, BufferSize);
		}

		BufferPtr = NewBufferPtr;
		BufferEnd = BufferPtr + BytesReadDirectly;
		if (BufferEnd <= Ptr) {
			return 0;
		}

		DWORD BytesReadBuffered = (DWORD)(std::min(End, BufferEnd) - Ptr);
		memcpy(Data, &Buffer[(DWORD)(Ptr - BufferPtr)], BytesReadBuffered);
		return BytesReadBuffered;
	}

	// backward buffering:
	INT64 NewBufferPtr;
	if (Ptr + DataSize >= BufferSize) {
		// Buffer layout: [BufferSize - AlignSize - DataSize|DataSize|1..AlignSize]
		NewBufferPtr = Ptr + DataSize - BufferSize + AlignSize;
		NewBufferPtr&= ~(INT64)(AlignSize - 1);
	} else {
		NewBufferPtr = 0;
	}

	DWORD BytesReadDirectly;

	if (NewBufferPtr + BufferSize - AlignSize >= BufferPtr && NewBufferPtr + BufferSize <= BufferEnd) {
		memmove(&Buffer[BufferSize - AlignSize], &Buffer[NewBufferPtr + BufferSize - AlignSize - BufferPtr], AlignSize);

		BytesReadDirectly = DirectReadAt(NewBufferPtr, Buffer, BufferSize - AlignSize);
		if (BytesReadDirectly == BufferSize - AlignSize) {
			BytesReadDirectly+= AlignSize;
		}

	} else {
		BytesReadDirectly = DirectReadAt(NewBufferPtr, Buffer, BufferSize);
	}
	BufferPtr = NewBufferPtr;
	BufferEnd = BufferPtr + BytesReadDirectly;

	if (Ptr >= BufferEnd) {
		return 0;
	}

	DWORD BytesReadBuffered = std::min(End, BufferEnd) - Ptr;
	memcpy(Data, &Buffer[Ptr - BufferPtr], BytesReadBuffered);

	return BytesReadBuffered;
}

bool CachedRead::DirectTell(INT64 &Ptr)
{
	return file.GetPointer(Ptr);
}

bool CachedRead::DirectSeek(INT64 Ptr)
{
	return file.SetPointer(Ptr, NULL, FILE_BEGIN);
}

DWORD CachedRead::DirectReadAt(INT64 Ptr, LPVOID Data, DWORD DataSize)
{
	if (!DirectSeek(Ptr)) {
		return 0;
	}

	DWORD BytesRead = 0;
	if (!file.Read(Data, DataSize, &BytesRead)) {
		return 0;
	}

	//	fprintf(stderr, "DirectReadAt: [0x%lx 0x%lx)\n", (unsigned long)Ptr, (unsigned long)Ptr + BytesRead);
	return BytesRead;
}

/////////////

CachedWrite::CachedWrite(File& file):
	Buffer(reinterpret_cast<LPBYTE>(xf_malloc(BufferSize))),
	file(file),
	FreeSize(BufferSize),
	Flushed(false)
{
}

CachedWrite::~CachedWrite()
{
	Flush();

	if (Buffer)
	{
		xf_free(Buffer);
	}
}

bool CachedWrite::Write(LPCVOID Data, DWORD DataSize)
{
	bool Result=false;
	bool SuccessFlush=true;

	if (Buffer)
	{
		if (DataSize>FreeSize)
		{
			SuccessFlush=Flush();
		}

		if(SuccessFlush)
		{
			if (DataSize>FreeSize)
			{
				DWORD WrittenSize=0;

				if (file.Write(Data, DataSize,&WrittenSize) && DataSize==WrittenSize)
				{
					Result=true;
				}
			}
			else
			{
				memcpy(&Buffer[BufferSize-FreeSize],Data,DataSize);
				FreeSize-=DataSize;
				Flushed=false;
				Result=true;
			}
		}
	}
	return Result;
}

bool CachedWrite::Flush()
{
	if (Buffer)
	{
		if (!Flushed)
		{
			DWORD WrittenSize=0;

			if (file.Write(Buffer, BufferSize-FreeSize, &WrittenSize, nullptr) && BufferSize-FreeSize==WrittenSize)
			{
				Flushed=true;
				FreeSize=BufferSize;
			}
		}
	}

	return Flushed;
}
