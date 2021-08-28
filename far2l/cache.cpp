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
#include <CheckedCast.hpp>

#include <fcntl.h>


BufferedFileView::BufferedFileView()
{
}

BufferedFileView::~BufferedFileView()
{
	Close();
	free(Buffer);
}

bool BufferedFileView::Open(const std::string &PathName)
{
	Close();

	FD = sdc_open(PathName.c_str(), O_RDONLY);
	if (FD == -1) {
		return false;
	}

#ifndef __APPLE__
	posix_fadvise(FD, 0, 0, POSIX_FADV_SEQUENTIAL); // todo: sdc_posix_fadvise
#endif

	FileSize = 0;
	ActualizeFileSize();
	return true;
}

void BufferedFileView::SetPointer(INT64 Ptr, int Whence)
{
	switch (Whence) {
		case SEEK_SET:
			CurPtr = (Ptr > 0) ? Ptr : 0;
			break;

		case SEEK_CUR:
			Ptr+= CurPtr;
			CurPtr = (Ptr > 0) ? Ptr : 0;
		break;

		case SEEK_END:
			Ptr+= FileSize;
			CurPtr = (Ptr > 0) ? Ptr : 0;
		break;
	}
}

void BufferedFileView::Close()
{
	Clear();
	if (FD != -1) {
		sdc_close(FD);
		FD = -1;
	}
}

void BufferedFileView::ActualizeFileSize()
{
	struct stat s = {};
	if (FD != -1 && sdc_fstat(FD, &s) == 0 && FileSize != (UINT64)s.st_size) {
		Clear();
		FileSize = s.st_size;
	}
}

void BufferedFileView::Clear()
{
	BufferBounds.Ptr = 0;
	BufferBounds.End = 0;
}

DWORD BufferedFileView::Read(void *Buf, DWORD Size)
{
	LPBYTE View = ViewBytesSlide(Size);
	if (Size)
		memcpy(Buf, View, Size);
	return Size;
}

LPBYTE BufferedFileView::ViewBytesSlide(DWORD &Size)
{
	LPBYTE View = ViewBytesAt(CurPtr, Size);
	CurPtr+= Size;
	return View;
}

LPBYTE BufferedFileView::ViewBytesAt(UINT64 Ptr, DWORD &Size)
{
	if (Ptr >= BufferBounds.Ptr && Ptr + Size <= BufferBounds.End) {
		return &Buffer[CheckedCast<size_t>(Ptr - BufferBounds.Ptr)];
	}

	Bounds NewBufferBounds;
	if (Ptr < LastPtr) { // backward buffering
		CalcBufferBounds(NewBufferBounds, Ptr, Size, AheadCount, BehindCount);
	} else { // forward buffering
		CalcBufferBounds(NewBufferBounds, Ptr, Size, BehindCount, AheadCount);
	}

	LastPtr = Ptr;

	DWORD NewBufferSize = CheckedCast<DWORD>(NewBufferBounds.End - NewBufferBounds.Ptr);

	LPBYTE NewBuffer;
	if (NewBufferSize > BufferSize) {
		NewBuffer = AllocBuffer(NewBufferSize + CapacityStock * AlignSize);
		if (NewBuffer == nullptr) {
			NewBuffer = AllocBuffer(NewBufferSize);
			if (NewBuffer == nullptr) {
				Size = 0;
				return nullptr;
			}
		} else {
			NewBufferSize+= CapacityStock * AlignSize;
		}
	} else {
		NewBuffer = Buffer;
	}

	UINT64 IntersectPtr = std::max(NewBufferBounds.Ptr, BufferBounds.Ptr);
	UINT64 IntersectEnd = std::min(NewBufferBounds.End, BufferBounds.End);

	if (IntersectPtr < IntersectEnd) {
		memmove(&NewBuffer[IntersectPtr - NewBufferBounds.Ptr],
			&Buffer[IntersectPtr - BufferBounds.Ptr],
			CheckedCast<size_t>(IntersectEnd - IntersectPtr));
		if (IntersectPtr > NewBufferBounds.Ptr) {
			DWORD NeedDirectReadSize = CheckedCast<DWORD>(IntersectPtr - NewBufferBounds.Ptr);
			DWORD DirectReadSize = DirectReadAt(NewBufferBounds.Ptr, NewBuffer, NeedDirectReadSize);
			if (NeedDirectReadSize != DirectReadSize) {
				NewBufferBounds.End = NewBufferBounds.Ptr + DirectReadSize;
			}
		}

		if (NewBufferBounds.End > IntersectEnd) {
			DWORD NeedDirectReadSize = CheckedCast<DWORD>(NewBufferBounds.End - IntersectEnd);
			DWORD DirectReadSize = DirectReadAt(IntersectEnd,
				&NewBuffer[IntersectEnd - NewBufferBounds.Ptr], NeedDirectReadSize);
			NewBufferBounds.End = IntersectEnd + DirectReadSize;
		}

	} else {
		DWORD NeedDirectReadSize = CheckedCast<DWORD>(NewBufferBounds.End - NewBufferBounds.Ptr);
		DWORD DirectReadSize = DirectReadAt(NewBufferBounds.Ptr, &NewBuffer[0], NeedDirectReadSize);
		NewBufferBounds.End = NewBufferBounds.Ptr + DirectReadSize;
	}

	if (NewBuffer != Buffer) {
		free(Buffer);
		Buffer = NewBuffer;
		BufferSize = NewBufferSize;
	}

	BufferBounds = NewBufferBounds;

	if (Ptr < BufferBounds.Ptr || Ptr >= BufferBounds.End) {
		Size = 0;
		return nullptr;
	}

	DWORD AvailableSize = CheckedCast<DWORD>(BufferBounds.End - Ptr);
	if (Size > AvailableSize) {
		Size = AvailableSize;
	}

	return &Buffer[CheckedCast<size_t>(Ptr - BufferBounds.Ptr)];
}

LPBYTE BufferedFileView::AllocBuffer(size_t Size)
{
	void *ptr;
	if (posix_memalign(&ptr, AlignSize, Size) != 0) {
		ptr = (LPBYTE)malloc(Size);
	}

	return (LPBYTE)ptr;
}

void BufferedFileView::CalcBufferBounds(Bounds &bi, UINT64 Ptr, DWORD DataSize, DWORD CountLefter, DWORD CountRighter)
{
	bi.Ptr = AlignDown(Ptr);
	if (bi.Ptr > AheadCount * AlignSize) {
		bi.Ptr-= CountLefter * AlignSize;
	} else {
		bi.Ptr= 0;
	}

	bi.End = AlignUp(Ptr + DataSize + CountRighter * AlignSize);
}


DWORD BufferedFileView::DirectReadAt(UINT64 Ptr, LPVOID Data, DWORD DataSize)
{
	if (FD == -1)
		return 0;

	ssize_t r = sdc_pread(FD, Data, DataSize, CheckedCast<off_t>(Ptr));
	if (r <= 0)
		return 0;

//	fprintf(stderr, "BufferedFileView::DirectReadAt: [%lx ... %lx)\n", (unsigned long)Ptr, (unsigned long)(Ptr + DataSize));

	return CheckedCast<DWORD>(r);
}

/////////////

CachedWrite::CachedWrite(File& file):
	Buffer(reinterpret_cast<LPBYTE>(malloc(BufferSize))),
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
		free(Buffer);
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
