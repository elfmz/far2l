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

#include "plugin.hpp"
#include "UnicodeString.hpp"
#include "noncopyable.hpp"
#include <string.h>
#include <map>
#include <vector>
#include <memory>

#define NT_MAX_PATH 32768

struct FAR_FIND_DATA_EX
{
	union {
		FILETIME ftLastAccessTime;
		FILETIME ftUnixAccessTime;
	};
	union {
		FILETIME ftLastWriteTime;
		FILETIME ftUnixModificationTime;
	};
	union {
		FILETIME ftCreationTime;
		FILETIME ftUnixStatusChangeTime;
	};
	FILETIME ftChangeTime;
	uint64_t nFileSize;
	uint64_t nPackSize;
	uint64_t UnixDevice;
	uint64_t UnixNode;
	uid_t UnixOwner;
	gid_t UnixGroup;

	DWORD dwFileAttributes;
	DWORD dwUnixMode;
	DWORD nHardLinks;
	struct
	{
		DWORD dwReserved0;
		DWORD dwReserved1;
	};

	FARString   strFileName;

	void Clear()
	{
		memset(&ftLastAccessTime,0,sizeof(ftLastAccessTime));
		memset(&ftLastWriteTime,0,sizeof(ftLastWriteTime));
		memset(&ftCreationTime,0,sizeof(ftCreationTime));
		memset(&ftChangeTime,0,sizeof(ftChangeTime));
		nFileSize = 0;
		nPackSize = 0;
		UnixDevice = 0;
		UnixNode = 0;
		UnixOwner = 0;
		UnixGroup = 0;
		dwFileAttributes = 0;
		dwUnixMode = 0;
		nHardLinks = 0;
		dwReserved0 = 0;
		dwReserved1 = 0;
		strFileName.Clear();
	}

	FAR_FIND_DATA_EX& operator=(const FAR_FIND_DATA_EX &ffdexCopy)
	{
		if (this != &ffdexCopy)
		{
			ftLastAccessTime=ffdexCopy.ftLastAccessTime;
			ftLastWriteTime=ffdexCopy.ftLastWriteTime;
			ftCreationTime=ffdexCopy.ftCreationTime;
			ftChangeTime=ffdexCopy.ftChangeTime;
			nFileSize=ffdexCopy.nFileSize;
			nPackSize=ffdexCopy.nPackSize;
			UnixDevice=ffdexCopy.UnixDevice;
			UnixNode=ffdexCopy.UnixNode;
			UnixOwner=ffdexCopy.UnixOwner;
			UnixGroup=ffdexCopy.UnixGroup;
			dwFileAttributes=ffdexCopy.dwFileAttributes;
			dwUnixMode=ffdexCopy.dwUnixMode;
			nHardLinks = ffdexCopy.nHardLinks;;
			dwReserved0=ffdexCopy.dwReserved0;
			dwReserved1=ffdexCopy.dwReserved1;
			strFileName=ffdexCopy.strFileName;
		}

		return *this;
	}
};

class FindFile: private NonCopyable
{
public:
	FindFile(LPCWSTR Object, bool ScanSymLink = true, DWORD WinPortFindFlags = FIND_FILE_FLAG_NO_CUR_UP); //FIND_FILE_FLAG_NO_CUR_UP is enforced
	~FindFile();
	bool Get(FAR_FIND_DATA_EX& FindData);

private:
	HANDLE Handle;
	bool empty;
	FAR_FIND_DATA_EX Data;
};

typedef std::map<std::string, std::vector<char> > FileExtendedAttributes;

class File: private NonCopyable
{
public:
	File();
	virtual ~File();
	virtual bool Open(LPCWSTR Object, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDistribution, DWORD dwFlagsAndAttributes=0, HANDLE hTemplateFile=nullptr, bool ForceElevation=false);
	virtual bool Read(LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED Overlapped = nullptr);
	virtual bool Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LPOVERLAPPED Overlapped = nullptr);
	virtual bool SetPointer(INT64 DistanceToMove, PINT64 NewFilePointer, DWORD MoveMethod);
	virtual bool GetPointer(INT64& Pointer){return SetPointer(0, &Pointer, FILE_CURRENT);}
	virtual bool SetEnd();
	virtual bool GetTime(LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime, LPFILETIME ChangeTime);
	virtual bool SetTime(const FILETIME* CreationTime, const FILETIME* LastAccessTime, const FILETIME* LastWriteTime, const FILETIME* ChangeTime);
	virtual bool GetSize(UINT64& Size);
	virtual bool FlushBuffers();
	virtual bool Chmod(DWORD dwUnixMode);
	virtual FemaleBool QueryFileExtendedAttributes(FileExtendedAttributes &xattr);
	virtual FemaleBool SetFileExtendedAttributes(const FileExtendedAttributes &xattr);
	virtual bool Close();
	virtual bool Eof();
	bool Opened() const {return Handle != INVALID_HANDLE_VALUE;}

private:
	HANDLE Handle;
};

class FileSeekDefer: public File
{
public:
	FileSeekDefer();
	virtual ~FileSeekDefer();

	virtual bool Open(LPCWSTR Object, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDistribution, DWORD dwFlagsAndAttributes=0, HANDLE hTemplateFile=nullptr, bool ForceElevation=false);
	virtual bool Read(LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED Overlapped = nullptr);
	virtual bool Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LPOVERLAPPED Overlapped = nullptr);
	virtual bool SetPointer(INT64 DistanceToMove, PINT64 NewFilePointer, DWORD MoveMethod);
	virtual bool GetPointer(INT64& Pointer);
	virtual bool SetEnd();
	virtual bool Eof();

private:
	bool FlushPendingSeek();

	UINT64 CurrentPointer, Size;
	bool SeekPending;
};


DWORD apiGetEnvironmentVariable(
    const wchar_t *lpwszName,
    FARString &strBuffer
);

DWORD apiGetCurrentDirectory(
    FARString &strCurDir
);

DWORD apiGetTempPath(
    FARString &strBuffer
);

bool apiExpandEnvironmentStrings(
    const wchar_t *src,
    FARString &strDest
);

DWORD apiWNetGetConnection(
    const wchar_t *lpwszLocalName,
    FARString &strRemoteName
);

BOOL apiGetVolumeInformation(
    const wchar_t *lpwszRootPathName,
    FARString *pVolumeName,
    LPDWORD lpVolumeSerialNumber,
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

BOOL apiGetFindDataEx(
    const wchar_t *lpwszFileName,
    FAR_FIND_DATA_EX& FindData,
    bool ScanSymLink = true,
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
    LPSECURITY_ATTRIBUTES SecurityAttributes,
    DWORD CreationDistribution,
    DWORD FlagsAndAttributes=0,
    HANDLE TemplateFile=nullptr,
    bool ForceElevation = false
);

BOOL apiMoveFile(
    const wchar_t *lpwszExistingFileName, // address of name of the existing file
    const wchar_t *lpwszNewFileName   // address of new name for the file
);

BOOL apiMoveFileEx(
    const wchar_t *lpwszExistingFileName, // address of name of the existing file
    const wchar_t *lpwszNewFileName,   // address of new name for the file
    DWORD dwFlags   // flag to determine how to move file
);

int apiRegEnumKeyEx(
    HKEY hKey,
    DWORD dwIndex,
    FARString &strName,
    PFILETIME lpftLastWriteTime=nullptr
);

BOOL apiIsDiskInDrive(
    const wchar_t *Root
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

BOOL apiCreateDirectoryEx(
    LPCWSTR TemplateDirectory,
    LPCWSTR NewDirectory,
    LPSECURITY_ATTRIBUTES SecurityAttributes
);

DWORD apiGetFileAttributes(
    LPCWSTR lpFileName
);

BOOL apiSetFileAttributes(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
);

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

void apiEnableLowFragmentationHeap();
