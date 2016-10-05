/*
farwinapi.cpp

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

#include "headers.hpp"

#include <sys/stat.h>
#include "pathmix.hpp"
#include "mix.hpp"
#include "ctrlobj.hpp"
#include "config.hpp"

struct PSEUDO_HANDLE
{
	HANDLE ObjectHandle;
	PVOID BufferBase;
	ULONG NextOffset;
	ULONG BufferSize;
};

void TranslateFindFile(const WIN32_FIND_DATA &wfd, FAR_FIND_DATA_EX& FindData)
{
	FindData.dwFileAttributes = wfd.dwFileAttributes;
	FindData.ftCreationTime = wfd.ftCreationTime;
	FindData.ftLastAccessTime = wfd.ftLastAccessTime;
	FindData.ftLastWriteTime = wfd.ftLastWriteTime;
	FindData.ftChangeTime = wfd.ftLastWriteTime;
	FindData.nFileSize = wfd.nFileSizeHigh;
	FindData.nFileSize<<= 32;
	FindData.nFileSize|= wfd.nFileSizeLow;
	FindData.nPackSize = 0;//WTF?
	FindData.dwReserved0 = wfd.dwReserved0;
	FindData.dwReserved1 = wfd.dwReserved1;
	FindData.dwUnixMode = wfd.dwUnixMode;
	FindData.strFileName = wfd.cFileName;
}

HANDLE FindFirstFileInternal(LPCWSTR Name, FAR_FIND_DATA_EX& FindData)
{
	WIN32_FIND_DATA wfd = {0};
	HANDLE Result = WINPORT(FindFirstFile)(Name, &wfd);
	if (Result!=INVALID_HANDLE_VALUE)
		TranslateFindFile(wfd, FindData);

	return Result;
}

bool FindNextFileInternal(HANDLE Find, FAR_FIND_DATA_EX& FindData)
{
	WIN32_FIND_DATA wfd = {0};
	if (!WINPORT(FindNextFile)(Find, &wfd))
		return FALSE;

	TranslateFindFile(wfd, FindData);
	return TRUE;
}

bool FindCloseInternal(HANDLE Find)
{
	return (WINPORT(FindClose)(Find)!=FALSE);
}

FindFile::FindFile(LPCWSTR Object, bool ScanSymLink):
	Handle(INVALID_HANDLE_VALUE),
	empty(false)
{
	FARString strName(NTPath(Object).Get());

	// temporary disable elevation to try "real" name first
	DWORD OldElevationMode = Opt.ElevationMode;
	Opt.ElevationMode = 0;
	Handle = FindFirstFileInternal(strName, Data);
	Opt.ElevationMode = OldElevationMode;

	if (Handle == INVALID_HANDLE_VALUE && WINPORT(GetLastError)() == ERROR_ACCESS_DENIED)
	{
		if(ScanSymLink)
		{
			FARString strReal(strName);
			// only links in path should be processed, not the object name itself
			CutToSlash(strReal);
			ConvertNameToReal(strReal, strReal);
			AddEndSlash(strReal);
			strReal+=PointToName(Object);
			strReal = NTPath(strReal);
			Handle = FindFirstFileInternal(strReal, Data);
		}

	}
	empty = Handle == INVALID_HANDLE_VALUE;
}

FindFile::~FindFile()
{
	if(Handle != INVALID_HANDLE_VALUE)
	{
		FindCloseInternal(Handle);
	}
}

bool FindFile::Get(FAR_FIND_DATA_EX& FindData)
{
	bool Result = false;
	if (!empty)
	{
		FindData = Data;
		Result = true;
	}
	if(Result)
	{
		empty = !FindNextFileInternal(Handle, Data);
	}

	// skip ".." & "."
	if(Result && FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY && FindData.strFileName.At(0) == L'.' &&
		// хитрый способ - у виртуальных папок не бывает SFN, в отличие от.
		((FindData.strFileName.At(1) == L'.' && !FindData.strFileName.At(2)) || !FindData.strFileName.At(1)))
	{
		Result = Get(FindData);
	}
	return Result;
}




File::File():
	Handle(INVALID_HANDLE_VALUE)
{
}

File::~File()
{
	Close();
}

bool File::Open(LPCWSTR Object, DWORD DesiredAccess, DWORD ShareMode, LPSECURITY_ATTRIBUTES SecurityAttributes, DWORD CreationDistribution, DWORD FlagsAndAttributes, HANDLE TemplateFile, bool ForceElevation)
{
	Handle = apiCreateFile(Object, DesiredAccess, ShareMode, SecurityAttributes, CreationDistribution, FlagsAndAttributes, TemplateFile, ForceElevation);
	return Handle != INVALID_HANDLE_VALUE;
}

bool File::Read(LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED Overlapped)
{
	return WINPORT(ReadFile)(Handle, Buffer, NumberOfBytesToRead, NumberOfBytesRead, Overlapped) != FALSE;
}

bool File::Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LPOVERLAPPED Overlapped) const
{
	return WINPORT(WriteFile)(Handle, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten, Overlapped) != FALSE;
}

bool File::SetPointer(INT64 DistanceToMove, PINT64 NewFilePointer, DWORD MoveMethod)
{
	return WINPORT(SetFilePointerEx)(Handle, *reinterpret_cast<PLARGE_INTEGER>(&DistanceToMove), reinterpret_cast<PLARGE_INTEGER>(NewFilePointer), MoveMethod) != FALSE;
}

bool File::SetEnd()
{
	return WINPORT(SetEndOfFile)(Handle) != FALSE;
}

bool File::GetTime(LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime, LPFILETIME ChangeTime)
{
	return GetFileTimeEx(Handle, CreationTime, LastAccessTime, LastWriteTime, ChangeTime);
}

bool File::SetTime(const FILETIME* CreationTime, const FILETIME* LastAccessTime, const FILETIME* LastWriteTime, const FILETIME* ChangeTime)
{
	return SetFileTimeEx(Handle, CreationTime, LastAccessTime, LastWriteTime, ChangeTime);
}

bool File::GetSize(UINT64& Size)
{
	return apiGetFileSizeEx(Handle, Size);
}

bool File::FlushBuffers()
{
	return WINPORT(FlushFileBuffers)(Handle) != FALSE;
}

bool File::Chmod(DWORD dwUnixMode)
{
	int r = sdc_fchmod(WINPORT(GetFileDescriptor)(Handle), dwUnixMode);
	if (r!=0) {
		WINPORT(SetLastError)(r);
		return false;
	}
	
	return true;
}

bool File::Close()
{
	bool Result=true;
	if(Handle!=INVALID_HANDLE_VALUE)
	{
		Result = WINPORT(CloseHandle)(Handle) != FALSE;
		Handle = INVALID_HANDLE_VALUE;
	}
	return Result;
}

bool File::Eof()
{
	INT64 Ptr=0;
	GetPointer(Ptr);
	UINT64 Size=0;
	GetSize(Size);
	return static_cast<UINT64>(Ptr)==Size;
}

BOOL apiDeleteFile(const wchar_t *lpwszFileName)
{
	FARString strNtName(NTPath(lpwszFileName).Get());
	BOOL Result = WINPORT(DeleteFile)(strNtName);
	return Result;
}

BOOL apiRemoveDirectory(const wchar_t *DirName)
{
	FARString strNtName(NTPath(DirName).Get());
	BOOL Result = WINPORT(RemoveDirectory)(strNtName);
	return Result;
}

HANDLE apiCreateFile(const wchar_t* Object, DWORD DesiredAccess, DWORD ShareMode, LPSECURITY_ATTRIBUTES SecurityAttributes, DWORD CreationDistribution, DWORD FlagsAndAttributes, HANDLE TemplateFile, bool ForceElevation)
{
	FARString strObject(NTPath(Object).Get());
	FlagsAndAttributes|=FILE_FLAG_BACKUP_SEMANTICS|(CreationDistribution==OPEN_EXISTING?FILE_FLAG_POSIX_SEMANTICS:0);

	HANDLE Handle=WINPORT(CreateFile)(strObject, DesiredAccess, ShareMode, SecurityAttributes, CreationDistribution, FlagsAndAttributes, TemplateFile);
	if(Handle == INVALID_HANDLE_VALUE)
	{
		DWORD Error=WINPORT(GetLastError)();
		if(Error==ERROR_FILE_NOT_FOUND||Error==ERROR_PATH_NOT_FOUND)
		{
			FlagsAndAttributes&=~FILE_FLAG_POSIX_SEMANTICS;
			Handle = WINPORT(CreateFile)(strObject, DesiredAccess, ShareMode, SecurityAttributes, CreationDistribution, FlagsAndAttributes, TemplateFile);
		}
	}

	return Handle;
}

BOOL apiMoveFile(
    const wchar_t *lpwszExistingFileName, // address of name of the existing file
    const wchar_t *lpwszNewFileName   // address of new name for the file
)
{
	FARString strFrom(NTPath(lpwszExistingFileName).Get()), strTo(NTPath(lpwszNewFileName).Get());
	BOOL Result = WINPORT(MoveFile)(strFrom, strTo);
	return Result;
}

BOOL apiMoveFileEx(
    const wchar_t *lpwszExistingFileName, // address of name of the existing file
    const wchar_t *lpwszNewFileName,   // address of new name for the file
    DWORD dwFlags   // flag to determine how to move file
)
{
	FARString strFrom(NTPath(lpwszExistingFileName).Get()), strTo(NTPath(lpwszNewFileName).Get());
	BOOL Result = WINPORT(MoveFileEx)(strFrom, strTo, dwFlags);
	return Result;
}

DWORD apiGetEnvironmentVariable(const wchar_t *lpwszName, FARString &strBuffer)
{
	WCHAR Buffer[MAX_PATH];
	DWORD Size = WINPORT(GetEnvironmentVariable)(lpwszName, Buffer, ARRAYSIZE(Buffer));

	if (Size)
	{
		if(Size>ARRAYSIZE(Buffer))
		{
			wchar_t *lpwszBuffer = strBuffer.GetBuffer(Size);
			Size = WINPORT(GetEnvironmentVariable)(lpwszName, lpwszBuffer, Size);
			strBuffer.ReleaseBuffer();
		}
		else
		{
			strBuffer.Copy(Buffer, Size);
		}
	}

	return Size;
}

FARString& strCurrentDirectory()
{
	static FARString strCurrentDirectory;
	return strCurrentDirectory;
}

void InitCurrentDirectory()
{
	//get real curdir:
	WCHAR Buffer[MAX_PATH];
	DWORD Size=WINPORT(GetCurrentDirectory)(ARRAYSIZE(Buffer),Buffer);
	if(Size)
	{
		FARString strInitCurDir;
		if(Size>ARRAYSIZE(Buffer))
		{
			LPWSTR InitCurDir=strInitCurDir.GetBuffer(Size);
			WINPORT(GetCurrentDirectory)(Size,InitCurDir);
			strInitCurDir.ReleaseBuffer(Size-1);
		}
		else
		{
			strInitCurDir.Copy(Buffer, Size);
		}
		//set virtual curdir:
		apiSetCurrentDirectory(strInitCurDir);
	}
}

DWORD apiGetCurrentDirectory(FARString &strCurDir)
{
	//never give outside world a direct pointer to our internal string
	//who knows what they gonna do
	strCurDir.Copy(strCurrentDirectory().CPtr(),strCurrentDirectory().GetLength());
	return static_cast<DWORD>(strCurDir.GetLength());
}

BOOL apiSetCurrentDirectory(LPCWSTR lpPathName, bool Validate)
{
	// correct path to our standard
	FARString strDir=lpPathName;
	if (lpPathName[0]!='/' || lpPathName[1]!=0) 
		DeleteEndSlash(strDir);
	LPCWSTR CD=strDir;
//	int Offset=HasPathPrefix(CD)?4:0;
///	if ((CD[Offset] && CD[Offset+1]==L':' && !CD[Offset+2]) || IsLocalVolumeRootPath(CD))
		//AddEndSlash(strDir);
	

	if (strDir == strCurrentDirectory())
		return TRUE;

	if (Validate)
	{
		FARString strLookup=lpPathName;
		AddEndSlash(strLookup);
		strLookup+=L"*";
		FAR_FIND_DATA_EX fd;
		if (!apiGetFindDataEx(strLookup, fd))
		{
			DWORD LastError = WINPORT(GetLastError)();
			if(!(LastError == ERROR_FILE_NOT_FOUND || LastError == ERROR_NO_MORE_FILES)) {
				fprintf(stderr, "apiSetCurrentDirectory: validate failed for %ls\n", lpPathName);
				return FALSE;
			}
		}
	}

	strCurrentDirectory()=strDir;

	// try to synchronize far cur dir with process cur dir
	if(CtrlObject && CtrlObject->Plugins.GetOemPluginsCount())
	{
		WINPORT(SetCurrentDirectory)(strCurrentDirectory());
	}

	return TRUE;
}

DWORD apiGetTempPath(FARString &strBuffer)
{
	::apiGetEnvironmentVariable(L"TEMP", strBuffer);
	if (strBuffer.IsEmpty())
		strBuffer = L"/var/tmp";
	return strBuffer.GetSize();
};


bool apiExpandEnvironmentStrings(const wchar_t *src, FARString &strDest)
{
	bool Result = false;
	WCHAR Buffer[MAX_PATH];
	DWORD Size = WINPORT(ExpandEnvironmentStrings)(src, Buffer, ARRAYSIZE(Buffer));
	if (Size)
	{
		if (Size > ARRAYSIZE(Buffer))
		{
			FARString strSrc(src); //src can point to strDest data
			wchar_t *lpwszDest = strDest.GetBuffer(Size);
			strDest.ReleaseBuffer(WINPORT(ExpandEnvironmentStrings)(strSrc, lpwszDest, Size)-1);
		}
		else
		{
			strDest.Copy(Buffer, Size-1);
		}
		Result = true;
	}

	return Result;
}

DWORD apiWNetGetConnection(const wchar_t *lpwszLocalName, FARString &strRemoteName)
{
	return ERROR_SUCCESS-1;
}

BOOL apiGetVolumeInformation(
    const wchar_t *lpwszRootPathName,
    FARString *pVolumeName,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    FARString *pFileSystemName
)
{
	//todo
	return FALSE;
	/*
	wchar_t *lpwszVolumeName = pVolumeName?pVolumeName->GetBuffer(MAX_PATH+1):nullptr;  //MSDN!
	wchar_t *lpwszFileSystemName = pFileSystemName?pFileSystemName->GetBuffer(MAX_PATH+1):nullptr;
	BOOL bResult = GetVolumeInformation(
	                   lpwszRootPathName,
	                   lpwszVolumeName,
	                   lpwszVolumeName?MAX_PATH:0,
	                   lpVolumeSerialNumber,
	                   lpMaximumComponentLength,
	                   lpFileSystemFlags,
	                   lpwszFileSystemName,
	                   lpwszFileSystemName?MAX_PATH:0
	               );

	if (lpwszVolumeName)
		pVolumeName->ReleaseBuffer();

	if (lpwszFileSystemName)
		pFileSystemName->ReleaseBuffer();

	return bResult;*/
}

void apiFindDataToDataEx(const FAR_FIND_DATA *pSrc, FAR_FIND_DATA_EX *pDest)
{
	pDest->dwFileAttributes = pSrc->dwFileAttributes;
	pDest->ftCreationTime = pSrc->ftCreationTime;
	pDest->ftLastAccessTime = pSrc->ftLastAccessTime;
	pDest->ftLastWriteTime = pSrc->ftLastWriteTime;
	pDest->ftChangeTime.dwHighDateTime=0;
	pDest->ftChangeTime.dwLowDateTime=0;
	pDest->nFileSize = pSrc->nFileSize;
	pDest->nPackSize = pSrc->nPackSize;
	pDest->dwUnixMode = pSrc->dwUnixMode;
	pDest->strFileName = pSrc->lpwszFileName;
}

void apiFindDataExToData(const FAR_FIND_DATA_EX *pSrc, FAR_FIND_DATA *pDest)
{
	pDest->dwFileAttributes = pSrc->dwFileAttributes;
	pDest->ftCreationTime = pSrc->ftCreationTime;
	pDest->ftLastAccessTime = pSrc->ftLastAccessTime;
	pDest->ftLastWriteTime = pSrc->ftLastWriteTime;
	pDest->nFileSize = pSrc->nFileSize;
	pDest->nPackSize = pSrc->nPackSize;
	pDest->dwUnixMode = pSrc->dwUnixMode;
	pDest->lpwszFileName = xf_wcsdup(pSrc->strFileName);
}

void apiFreeFindData(FAR_FIND_DATA *pData)
{
	xf_free(pData->lpwszFileName);
}

BOOL apiGetFindDataEx(const wchar_t *lpwszFileName, FAR_FIND_DATA_EX& FindData,bool ScanSymLink)
{
	FindFile Find(lpwszFileName, ScanSymLink);
	if(Find.Get(FindData))
	{
		return TRUE;
	}
	else if (!wcspbrk(lpwszFileName,L"*?"))
	{
		struct stat s = {0};
		if (sdc_stat(Wide2MB(lpwszFileName).c_str(), &s)==0) {
			FindData.Clear();
			WINPORT(FileTime_UnixToWin32)(s.st_mtim, &FindData.ftLastWriteTime);
			WINPORT(FileTime_UnixToWin32)(s.st_ctim, &FindData.ftCreationTime);
			WINPORT(FileTime_UnixToWin32)(s.st_atim, &FindData.ftLastAccessTime);
			FindData.dwFileAttributes = WINPORT(EvaluateAttributes)(s.st_mode, lpwszFileName);
			FindData.nFileSize = s.st_size;
			FindData.dwUnixMode = s.st_mode;
			FindData.dwReserved0 = FindData.dwReserved1 = 0;
			FindData.strFileName = PointToName(lpwszFileName);
			return TRUE;
		}
	}

	fprintf(stderr, "apiGetFindDataEx: FAILED - %ls", lpwszFileName);		

	FindData.Clear();
	FindData.dwFileAttributes = INVALID_FILE_ATTRIBUTES; //BUGBUG

	return FALSE;
}

bool apiGetFileSizeEx(HANDLE hFile, UINT64 &Size)
{
	bool Result=false;

	if (WINPORT(GetFileSizeEx)(hFile,reinterpret_cast<PLARGE_INTEGER>(&Size)))
	{
		Result=true;
	}
	return Result;
}

int apiRegEnumKeyEx(HKEY hKey,DWORD dwIndex,FARString &strName,PFILETIME lpftLastWriteTime)
{
	int ExitCode=ERROR_MORE_DATA;

	for (DWORD Size=512; ExitCode==ERROR_MORE_DATA; Size<<=1)
	{
		wchar_t *Name=strName.GetBuffer(Size);
		DWORD Size0=Size;
		ExitCode=WINPORT(RegEnumKeyEx)(hKey,dwIndex,Name,&Size0,nullptr,nullptr,nullptr,lpftLastWriteTime);
		strName.ReleaseBuffer();
	}

	return ExitCode;
}

BOOL apiIsDiskInDrive(const wchar_t *Root)
{
	FARString strVolName;
	FARString strDrive;
	DWORD  MaxComSize;
	DWORD  Flags;
	FARString strFS;
	strDrive = Root;
	AddEndSlash(strDrive);
	BOOL Res = apiGetVolumeInformation(strDrive, &strVolName, nullptr, &MaxComSize, &Flags, &strFS);
	return Res;
}

int apiGetFileTypeByName(const wchar_t *Name)
{
	HANDLE hFile=apiCreateFile(Name,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0);

	if (hFile==INVALID_HANDLE_VALUE)
		return FILE_TYPE_UNKNOWN;

	int Type=WINPORT(GetFileType)(hFile);
	WINPORT(CloseHandle)(hFile);
	return Type;
}

BOOL apiGetDiskSize(const wchar_t *Path,uint64_t *TotalSize, uint64_t *TotalFree, uint64_t *UserFree)
{
	//todo
	return FALSE;
	/*
	int ExitCode=0;
	uint64_t uiTotalSize,uiTotalFree,uiUserFree;
	uiUserFree=0;
	uiTotalSize=0;
	uiTotalFree=0;
	FARString strPath(NTPath(Path).Get());
	AddEndSlash(strPath);
	ExitCode=GetDiskFreeSpaceEx(strPath,(PULARGE_INTEGER)&uiUserFree,(PULARGE_INTEGER)&uiTotalSize,(PULARGE_INTEGER)&uiTotalFree);

	if (TotalSize)
		*TotalSize = uiTotalSize;

	if (TotalFree)
		*TotalFree = uiTotalFree;

	if (UserFree)
		*UserFree = uiUserFree;

	return ExitCode;*/
}

BOOL apiCreateDirectory(LPCWSTR lpPathName,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	return apiCreateDirectoryEx(nullptr, lpPathName, lpSecurityAttributes);
}

BOOL apiCreateDirectoryEx(LPCWSTR TemplateDirectory, LPCWSTR NewDirectory, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
	BOOL Result = WINPORT(CreateDirectory)(NewDirectory, SecurityAttributes);
	return Result;
}

DWORD apiGetFileAttributes(LPCWSTR lpFileName)
{
	FARString strNtName(NTPath(lpFileName).Get());
	DWORD Result = WINPORT(GetFileAttributes)(strNtName);
	return Result;
}

BOOL apiSetFileAttributes(LPCWSTR lpFileName,DWORD dwFileAttributes)
{
	FARString strNtName(NTPath(lpFileName).Get());
	BOOL Result = WINPORT(SetFileAttributes)(strNtName, dwFileAttributes);
	return Result;

}

bool apiCreateSymbolicLink(LPCWSTR lpSymlinkFileName,LPCWSTR lpTargetFileName,DWORD dwFlags)
{
	return false;//todo
}

BOOL apiCreateHardLink(LPCWSTR lpFileName,LPCWSTR lpExistingFileName,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	return FALSE;//todo
}

bool apiGetFinalPathNameByHandle(HANDLE hFile, FARString& FinalFilePath)
{
	FinalFilePath = L"";
	return false;
}


void apiEnableLowFragmentationHeap()
{
}

bool GetFileTimeEx(HANDLE Object, LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime, LPFILETIME ChangeTime)
{
	memset(ChangeTime, 0, sizeof(*ChangeTime));
	return WINPORT(GetFileTime)(Object, CreationTime, LastAccessTime, LastWriteTime)!=FALSE;
}

bool SetFileTimeEx(HANDLE Object, const FILETIME* CreationTime, const FILETIME* LastAccessTime, const FILETIME* LastWriteTime, const FILETIME* ChangeTime)
{
	bool Result = false;
//todo
	return Result;
}
