#pragma once

/*
farwinapi.hpp

Враперы вокруг некоторых WinAPI функций
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

#include <farplug-wide.h>
#include "FARString.hpp"
#include "noncopyable.hpp"
#include <string.h>
#include <map>
#include <vector>
#include <memory>

#define NT_MAX_PATH 32768

struct FAR_FIND_DATA_EX
{
	union {
		FILETIME ftCreationTime;
		FILETIME ftUnixStatusChangeTime;
	};
	union {
		FILETIME ftLastAccessTime;
		FILETIME ftUnixAccessTime;
	};
	union {
		FILETIME ftLastWriteTime;
		FILETIME ftUnixModificationTime;
	};
	FILETIME ftChangeTime;
	uid_t UnixOwner;
	gid_t UnixGroup;
	uint64_t UnixDevice;
	uint64_t UnixNode;
	uint64_t nPhysicalSize;
	uint64_t nFileSize;

	DWORD dwFileAttributes;
	DWORD dwUnixMode;
	DWORD nHardLinks;
	DWORD nBlockSize;

	FARString strFileName;

	void Clear()
	{
		memset(&ftCreationTime,0,sizeof(ftCreationTime));
		memset(&ftLastAccessTime,0,sizeof(ftLastAccessTime));
		memset(&ftLastWriteTime,0,sizeof(ftLastWriteTime));
		memset(&ftChangeTime,0,sizeof(ftChangeTime));
		UnixOwner = 0;
		UnixGroup = 0;
		UnixDevice = 0;
		UnixNode = 0;
		nPhysicalSize = 0;
		nFileSize = 0;
		dwFileAttributes = 0;
		dwUnixMode = 0;
		nHardLinks = 0;
		nBlockSize = 0;
		strFileName.Clear();
	}

/*	FAR_FIND_DATA_EX& operator=(const FAR_FIND_DATA_EX &ffdexCopy)
	{
		if (this != &ffdexCopy)
		{
			ftCreationTime = ffdexCopy.ftCreationTime;
			ftLastAccessTime = ffdexCopy.ftLastAccessTime;
			ftLastWriteTime = ffdexCopy.ftLastWriteTime;
			ftChangeTime = ffdexCopy.ftChangeTime;
			UnixDevice = ffdexCopy.UnixDevice;
			UnixOwner = ffdexCopy.UnixOwner;
			UnixGroup = ffdexCopy.UnixGroup;
			UnixNode = ffdexCopy.UnixNode;
			nPhysicalSize = ffdexCopy.nPhysicalSize;
			nFileSize = ffdexCopy.nFileSize;
			dwFileAttributes = ffdexCopy.dwFileAttributes;
			dwUnixMode = ffdexCopy.dwUnixMode;
			nHardLinks = ffdexCopy.nHardLinks;;
			strFileName = ffdexCopy.strFileName;
		}

		return *this;
	}*/
};

class FindFile: private NonCopyable
{
public:
	FindFile(LPCWSTR Object, bool ScanSymLink = true, DWORD WinPortFindFlags = FIND_FILE_FLAG_NO_CUR_UP); //FIND_FILE_FLAG_NO_CUR_UP is enforced
	~FindFile();
	bool Get(FAR_FIND_DATA_EX& FindData);

private:
	HANDLE Handle;
	WIN32_FIND_DATA wfd;
	int err;
};

typedef std::map<std::string, std::vector<char> > FileExtendedAttributes;

class File: private NonCopyable
{
public:
	File();
	~File();
	bool Open(LPCWSTR Object, DWORD dwDesiredAccess, DWORD dwShareMode, const DWORD *UnixMode, DWORD dwCreationDistribution, DWORD dwFlagsAndAttributes=0, HANDLE hTemplateFile=nullptr, bool ForceElevation=false);
	bool Read(LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED Overlapped = nullptr);
	bool Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LPOVERLAPPED Overlapped = nullptr);
	bool SetPointer(INT64 DistanceToMove, PINT64 NewFilePointer, DWORD MoveMethod);
	bool GetPointer(INT64& Pointer){return SetPointer(0, &Pointer, FILE_CURRENT);}
	bool SetEnd();
	bool GetTime(LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime, LPFILETIME ChangeTime);
	bool SetTime(const FILETIME* CreationTime, const FILETIME* LastAccessTime, const FILETIME* LastWriteTime, const FILETIME* ChangeTime);
	bool GetSize(UINT64& Size);
	void AllocationHint(UINT64 Size);
	bool AllocationRequire(UINT64 Size);
	bool FlushBuffers();
	bool Chmod(DWORD dwUnixMode);
	FemaleBool QueryFileExtendedAttributes(FileExtendedAttributes &xattr);
	FemaleBool SetFileExtendedAttributes(const FileExtendedAttributes &xattr);
	bool Close();
	bool Eof();
	bool Opened() const {return Handle != INVALID_HANDLE_VALUE;}
	int Descriptor();

private:
	HANDLE Handle;
};

bool apiIsDevNull(const wchar_t *Src);

bool apiGetEnvironmentVariable(
	const char *lpszName,
	FARString &strBuffer
);
bool apiGetEnvironmentVariable(
	const wchar_t *lpwszName,
	FARString &strBuffer
);

DWORD apiGetCurrentDirectory(
	FARString &strCurDir
);

void apiGetTempPath(
	FARString &strBuffer
);

bool apiExpandEnvironmentStrings(
	const wchar_t *src,
	FARString &strDest
);

BOOL apiGetVolumeInformation(
	const wchar_t *lpwszRootPathName,
	FARString *pVolumeName,
	DWORD64 *lpVolumeSerialNumber,
	LPDWORD lpMaximumComponentLength,
	LPDWORD lpFileSystemFlags,
	FARString *pFileSystemName
);

void apiFindDataToDataEx(
	const FAR_FIND_DATA *pSrc,
	FAR_FIND_DATA_EX *pDest);

void apiFindDataExToData(
	const FAR_FIND_DATA_EX *pSrc,
	FAR_FIND_DATA *pDest
);

void apiFreeFindData(
	FAR_FIND_DATA *pData
);

BOOL apiGetFindDataForExactPathName(const wchar_t *lpwszFileName, FAR_FIND_DATA_EX& FindData);

BOOL apiGetFindDataEx(
	const wchar_t *lpwszFileName,
	FAR_FIND_DATA_EX& FindData,
	DWORD WinPortFindFlags = 0);

bool apiGetFileSizeEx(
	HANDLE hFile,
	UINT64 &Size);

//junk

BOOL apiDeleteFile(
	const wchar_t *lpwszFileName
);

BOOL apiRemoveDirectory(
	const wchar_t *DirName
);

HANDLE apiCreateFile(
	const wchar_t* Object,
	DWORD DesiredAccess,
	DWORD ShareMode,
	const DWORD *UnixMode,
	DWORD CreationDistribution,
	DWORD FlagsAndAttributes=0,
	HANDLE TemplateFile=nullptr,
	bool ForceElevation = false
);

BOOL apiMoveFile(
	const wchar_t *lpwszExistingFileName, // address of name of the existing file
	const wchar_t *lpwszNewFileName       // address of new name for the file
);

BOOL apiMoveFileEx(
	const wchar_t *lpwszExistingFileName, // address of name of the existing file
	const wchar_t *lpwszNewFileName,      // address of new name for the file
	DWORD dwFlags                         // flag to determine how to move file
);

int apiGetFileTypeByName(
	const wchar_t *Name
);

BOOL apiGetDiskSize(
	const wchar_t *Path,
	uint64_t*TotalSize,
	uint64_t *TotalFree,
	uint64_t *UserFree
);

BOOL apiCreateDirectory(
	LPCWSTR lpPathName,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

DWORD apiGetFileAttributes(
	LPCWSTR lpFileName
);

BOOL apiSetFileAttributes(
	LPCWSTR lpFileName,
	DWORD dwFileAttributes
);


bool apiPathExists(LPCWSTR lpPathName);
bool apiPathIsDir(LPCWSTR lpPathName);
bool apiPathIsFile(LPCWSTR lpPathName);

struct IUnmakeWritable
{
	virtual ~IUnmakeWritable() {} 
	virtual void Unmake() = 0;
};

typedef std::unique_ptr<IUnmakeWritable>	IUnmakeWritablePtr;

IUnmakeWritablePtr apiMakeWritable(LPCWSTR lpFileName);

class TemporaryMakeWritable
{
	IUnmakeWritablePtr _unmake;

public:
	inline TemporaryMakeWritable(LPCWSTR lpFileName) 
		: _unmake(apiMakeWritable(lpFileName))
	{
	}
	
	inline ~TemporaryMakeWritable()
	{
		if (_unmake)
			_unmake->Unmake();
	}
};

void InitCurrentDirectory();

BOOL apiSetCurrentDirectory(
	LPCWSTR lpPathName,
	bool Validate = true
);

// for elevation only, dont' use outside.
bool CreateSymbolicLinkInternal(LPCWSTR Object,LPCWSTR Target, DWORD dwFlags);

bool apiCreateSymbolicLink(
	LPCWSTR lpSymlinkFileName,
	LPCWSTR lpTargetFileName,
	DWORD dwFlags
);

BOOL apiCreateHardLink(
	LPCWSTR lpFileName,
	LPCWSTR lpExistingFileName,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

bool apiGetFinalPathNameByHandle(
	HANDLE hFile,
	FARString& FinalFilePath
);

// internal, dont' use outside.
bool GetFileTimeEx(HANDLE Object, LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime, LPFILETIME ChangeTime);
bool SetFileTimeEx(HANDLE Object, const FILETIME* CreationTime, const FILETIME* LastAccessTime, const FILETIME* LastWriteTime, const FILETIME* ChangeTime);

