#pragma once

/*
farwinapi.hpp

Âðàïåðû âîêðóã íåêîòîðûõ WinAPI ôóíêöèé
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
#define NT_MAX_PATH 32768

struct FAR_FIND_DATA_EX
{
	DWORD    dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	FILETIME ftChangeTime;
	uint64_t nFileSize;
	uint64_t nPackSize;
	DWORD dwUnixMode;
	struct
	{
		DWORD dwReserved0;
		DWORD dwReserved1;
	};

	string   strFileName;
	string   strAlternateFileName;

	void Clear()
	{
		dwFileAttributes=0;
		memset(&ftCreationTime,0,sizeof(ftCreationTime));
		memset(&ftLastAccessTime,0,sizeof(ftLastAccessTime));
		memset(&ftLastWriteTime,0,sizeof(ftLastWriteTime));
		memset(&ftChangeTime,0,sizeof(ftChangeTime));
		nFileSize=0;
		nPackSize=0;
		dwReserved0=0;
		dwReserved1=0;
		strFileName.Clear();
		strAlternateFileName.Clear();
	}

	FAR_FIND_DATA_EX& operator=(const FAR_FIND_DATA_EX &ffdexCopy)
	{
		if (this != &ffdexCopy)
		{
			dwFileAttributes=ffdexCopy.dwFileAttributes;
			ftCreationTime=ffdexCopy.ftCreationTime;
			ftLastAccessTime=ffdexCopy.ftLastAccessTime;
			ftLastWriteTime=ffdexCopy.ftLastWriteTime;
			ftChangeTime=ffdexCopy.ftChangeTime;
			nFileSize=ffdexCopy.nFileSize;
			nPackSize=ffdexCopy.nPackSize;
			dwReserved0=ffdexCopy.dwReserved0;
			dwReserved1=ffdexCopy.dwReserved1;
			strFileName=ffdexCopy.strFileName;
			strAlternateFileName=ffdexCopy.strAlternateFileName;
		}

		return *this;
	}
};

class FindFile: private NonCopyable
{
public:
	FindFile(LPCWSTR Object, bool ScanSymLink = true);
	~FindFile();
	bool Get(FAR_FIND_DATA_EX& FindData);

private:
	HANDLE Handle;
	bool empty;
	FAR_FIND_DATA_EX Data;
};

class File: private NonCopyable
{
public:
	File();
	~File();
	bool Open(LPCWSTR Object, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDistribution, DWORD dwFlagsAndAttributes=0, HANDLE hTemplateFile=nullptr, bool ForceElevation=false);
	bool Read(LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED Overlapped = nullptr);
	bool Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LPOVERLAPPED Overlapped = nullptr) const;
	bool SetPointer(INT64 DistanceToMove, PINT64 NewFilePointer, DWORD MoveMethod);
	bool GetPointer(INT64& Pointer){return SetPointer(0, &Pointer, FILE_CURRENT);}
	bool SetEnd();
	bool GetTime(LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime, LPFILETIME ChangeTime);
	bool SetTime(const FILETIME* CreationTime, const FILETIME* LastAccessTime, const FILETIME* LastWriteTime, const FILETIME* ChangeTime);
	bool GetSize(UINT64& Size);
	bool FlushBuffers();
	bool Close();
	bool Eof();
	bool Opened() const {return Handle != INVALID_HANDLE_VALUE;}

private:
	HANDLE Handle;
};

DWORD apiGetEnvironmentVariable(
    const wchar_t *lpwszName,
    string &strBuffer
);

DWORD apiGetCurrentDirectory(
    string &strCurDir
);

DWORD apiGetTempPath(
    string &strBuffer
);

bool apiExpandEnvironmentStrings(
    const wchar_t *src,
    string &strDest
);

DWORD apiWNetGetConnection(
    const wchar_t *lpwszLocalName,
    string &strRemoteName
);

BOOL apiGetVolumeInformation(
    const wchar_t *lpwszRootPathName,
    string *pVolumeName,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    string *pFileSystemName
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
    bool ScanSymLink=true);

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
    string &strName,
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
    string& FinalFilePath
);

// internal, dont' use outside.
bool GetFileTimeEx(HANDLE Object, LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime, LPFILETIME ChangeTime);
bool SetFileTimeEx(HANDLE Object, const FILETIME* CreationTime, const FILETIME* LastAccessTime, const FILETIME* LastWriteTime, const FILETIME* ChangeTime);

void apiEnableLowFragmentationHeap();
