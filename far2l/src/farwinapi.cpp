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
#include <sys/statvfs.h>
#include <fcntl.h>
#include <errno.h>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__CYGWIN__)
#include <sys/mount.h>
#elif !defined(__HAIKU__)
#include <sys/statfs.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif
#include "pathmix.hpp"
#include "mix.hpp"
#include "ctrlobj.hpp"
#include "config.hpp"
#include "MountInfo.h"
#include "FSFileFlags.h"

static void TranslateFindFile(const WIN32_FIND_DATA &wfd, FAR_FIND_DATA_EX &FindData)
{
	FindData.ftCreationTime = wfd.ftCreationTime;
	FindData.ftLastAccessTime = wfd.ftLastAccessTime;
	FindData.ftLastWriteTime = wfd.ftLastWriteTime;
	FindData.ftChangeTime = wfd.ftLastWriteTime;
	FindData.UnixOwner = wfd.UnixOwner;
	FindData.UnixGroup = wfd.UnixGroup;
	FindData.UnixDevice = wfd.UnixDevice;
	FindData.UnixNode = wfd.UnixNode;
	FindData.nPhysicalSize = wfd.nPhysicalSize;
	FindData.nFileSize = wfd.nFileSize;
	FindData.dwFileAttributes = wfd.dwFileAttributes;
	FindData.dwUnixMode = wfd.dwUnixMode;
	FindData.nHardLinks = wfd.nHardLinks;
	FindData.nBlockSize = wfd.nBlockSize;

	if (FindData.nHardLinks > 1)
		FindData.dwFileAttributes |= FILE_ATTRIBUTE_HARDLINKS;

	FindData.strFileName.CopyArray(wfd.cFileName);
}

FindFile::FindFile(LPCWSTR Object, bool ScanSymLink, DWORD WinPortFindFlags)
	:
	Handle(INVALID_HANDLE_VALUE)
{
	// Strange things happen with ScanSymLink in original code:
	// looks like tricky attempt to resolve symlinks in path without elevation
	// while elevation required to perform actual FindFile operation.
	// It seems this is not necessary for Linux,
	// if confirmed: ScanSymLink should be removed from here and apiGetFindDataEx
	WinPortFindFlags|= FIND_FILE_FLAG_NO_CUR_UP;

	Handle = WINPORT(FindFirstFileWithFlags)(Object, &wfd, WinPortFindFlags);
	if (Handle == INVALID_HANDLE_VALUE) {
		err = errno;
	}
}

FindFile::~FindFile()
{
	if (Handle != INVALID_HANDLE_VALUE) {
		WINPORT(FindClose)(Handle);
	}
}

bool FindFile::Get(FAR_FIND_DATA_EX &FindData)
{
	if (Handle == INVALID_HANDLE_VALUE) {
		errno = err;
		return false;
	}
	TranslateFindFile(wfd, FindData);

	if (!WINPORT(FindNextFile)(Handle, &wfd)) {
		err = errno;
		WINPORT(FindClose)(Handle);
		Handle = INVALID_HANDLE_VALUE;
	}

	if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && FindData.strFileName.At(0) == L'.'
			&& ((FindData.strFileName.At(1) == L'.' && !FindData.strFileName.At(2))
					|| !FindData.strFileName.At(1))) {		// FIND_FILE_FLAG_NO_CUR_UP should handle this
		ABORT();
	}

	return true;
}

File::File()
	:
	Handle(INVALID_HANDLE_VALUE)
{}

File::~File()
{
	Close();
}

bool File::Open(LPCWSTR Object, DWORD DesiredAccess, DWORD ShareMode, const DWORD *UnixMode,
		DWORD CreationDistribution, DWORD FlagsAndAttributes, HANDLE TemplateFile, bool ForceElevation)
{
	Handle = apiCreateFile(Object, DesiredAccess, ShareMode, UnixMode, CreationDistribution,
			FlagsAndAttributes, TemplateFile, ForceElevation);
	return Handle != INVALID_HANDLE_VALUE;
}

bool File::Read(LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED Overlapped)
{
	return WINPORT(ReadFile)(Handle, Buffer, NumberOfBytesToRead, NumberOfBytesRead, Overlapped) != FALSE;
}

bool File::Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten,
		LPOVERLAPPED Overlapped)
{
	return WINPORT(WriteFile)(Handle, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten, Overlapped)
			!= FALSE;
}

bool File::SetPointer(INT64 DistanceToMove, PINT64 NewFilePointer, DWORD MoveMethod)
{
	BOOL r;
	LARGE_INTEGER li;
	li.QuadPart = DistanceToMove;
	if (NewFilePointer) {
		LARGE_INTEGER li_new;
		li_new.QuadPart = *NewFilePointer;
		r = WINPORT(SetFilePointerEx)(Handle, li, &li_new, MoveMethod);
		*NewFilePointer = li_new.QuadPart;
	} else {
		r = WINPORT(SetFilePointerEx)(Handle, li, NULL, MoveMethod);
	}

	return r != FALSE;
}

void File::AllocationHint(UINT64 Size)
{
	WINPORT(FileAllocationHint)(Handle, Size);
}

bool File::AllocationRequire(UINT64 Size)
{
	return WINPORT(FileAllocationRequire)(Handle, Size) != FALSE;
}

bool File::SetEnd()
{
	return WINPORT(SetEndOfFile)(Handle) != FALSE;
}

bool File::GetTime(LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime,
		LPFILETIME ChangeTime)
{
	return GetFileTimeEx(Handle, CreationTime, LastAccessTime, LastWriteTime, ChangeTime);
}

bool File::SetTime(const FILETIME *CreationTime, const FILETIME *LastAccessTime,
		const FILETIME *LastWriteTime, const FILETIME *ChangeTime)
{
	return SetFileTimeEx(Handle, CreationTime, LastAccessTime, LastWriteTime, ChangeTime);
}

bool File::GetSize(UINT64 &Size)
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
	if (r != 0) {
		WINPORT(SetLastError)(r);
		return false;
	}

	return true;
}

int File::Descriptor()
{
	return WINPORT(GetFileDescriptor)(Handle);
}

FemaleBool File::QueryFileExtendedAttributes(FileExtendedAttributes &xattr)
{
	xattr.clear();

	int fd = WINPORT(GetFileDescriptor)(Handle);
	if (fd == -1)
		return FB_NO;

	std::vector<char> buf(0x100);
	for (;;) {
		ssize_t r = sdc_flistxattr(fd, &buf[0], buf.size() - 1);
		if (r == -1) {
			if (errno != ERANGE)
				return FB_NO;
			buf.resize(buf.size() * 2);
		} else if (r == 0) {
			return FB_YES;
		} else {
			buf.resize(r);
			break;
		}
	}

	const char *p = &buf[0];
	const char *e = p + buf.size();
	while (p < e) {
		const std::string name(p);
		xattr[name];
		p+= (name.size() + 1);
	}

	bool any_ok = false, any_failed = false;
	for (FileExtendedAttributes::iterator i = xattr.begin(); i != xattr.end();) {
		for (;;) {
			ssize_t r = sdc_fgetxattr(fd, i->first.c_str(), &buf[0], buf.size() - 1);
			if (r >= 0) {
				buf.resize(r);
				i->second.swap(buf);
				buf.resize(0x100);
				++i;
				any_ok = true;
				break;
			} else if (errno != ERANGE) {
				fprintf(stderr, "File::QueryFileExtendedAttributes: err=%u for '%s'\n", errno,
						i->first.c_str());
				i = xattr.erase(i);
				any_failed = true;
				break;
			} else {
				buf.resize(buf.size() * 2);
			}
		}
	}
	return any_failed ? (any_ok ? FB_MAYBE : FB_NO) : FB_YES;
}

FemaleBool File::SetFileExtendedAttributes(const FileExtendedAttributes &xattr)
{
	int fd = WINPORT(GetFileDescriptor)(Handle);
	if (fd == -1)
		return FB_NO;

	bool any_ok = false, any_failed = false;
	for (FileExtendedAttributes::const_iterator i = xattr.begin(); i != xattr.end(); ++i) {
		int r;
		if (i->second.empty()) {
			r = sdc_fsetxattr(fd, i->first.c_str(), "", 0, 0);
		} else
			r = sdc_fsetxattr(fd, i->first.c_str(), &i->second[0], i->second.size(), 0);
		if (r == -1) {
			fprintf(stderr, "File::SetFileExtendedAttributes: err=%u for '%s'\n", errno, i->first.c_str());
			any_failed = true;
		} else
			any_ok = true;
	}
	return any_failed ? (any_ok ? FB_MAYBE : FB_NO) : FB_YES;
}

bool File::Close()
{
	bool Result = true;
	if (Handle != INVALID_HANDLE_VALUE) {
		Result = WINPORT(CloseHandle)(Handle) != FALSE;
		Handle = INVALID_HANDLE_VALUE;
	}
	return Result;
}

bool File::Eof()
{
	INT64 Ptr = 0;
	GetPointer(Ptr);
	UINT64 Size = 0;
	GetSize(Size);
	return static_cast<UINT64>(Ptr) == Size;
}

//////////////////////////////////////////////////////////////

bool apiIsDevNull(const wchar_t *Src)
{
	return IsPathIn(Src, DEVNULLW);
}

BOOL apiDeleteFile(const wchar_t *lpwszFileName)
{
	BOOL Result = WINPORT(DeleteFile)(lpwszFileName);
	return Result;
}

BOOL apiRemoveDirectory(const wchar_t *DirName)
{
	BOOL Result = WINPORT(RemoveDirectory)(DirName);
	return Result;
}

HANDLE apiCreateFile(const wchar_t *Object, DWORD DesiredAccess, DWORD ShareMode, const DWORD *UnixMode,
		DWORD CreationDistribution, DWORD FlagsAndAttributes, HANDLE TemplateFile, bool ForceElevation)
{
	HANDLE Handle = WINPORT(CreateFile)(Object, DesiredAccess, ShareMode, UnixMode, CreationDistribution,
			FlagsAndAttributes, TemplateFile);
	if (Handle == INVALID_HANDLE_VALUE && apiIsDevNull(Object)) {
		Handle = WINPORT(CreateFile)(DEVNULLW, DesiredAccess, ShareMode, UnixMode, CreationDistribution,
				FlagsAndAttributes, TemplateFile);
	}
	return Handle;
}

BOOL apiMoveFile(const wchar_t *lpwszExistingFileName,		// address of name of the existing file
		const wchar_t *lpwszNewFileName						// address of new name for the file
)
{
	BOOL Result = WINPORT(MoveFile)(lpwszExistingFileName, lpwszNewFileName);
	return Result;
}

BOOL apiMoveFileEx(const wchar_t *lpwszExistingFileName,	// address of name of the existing file
		const wchar_t *lpwszNewFileName,					// address of new name for the file
		DWORD dwFlags										// flag to determine how to move file
)
{
	BOOL Result = WINPORT(MoveFileEx)(lpwszExistingFileName, lpwszNewFileName, dwFlags);
	return Result;
}

bool apiGetEnvironmentVariable(const char *lpszName, FARString &strBuffer)
{
	const char *env = Environment::GetVariable(lpszName);
	if (!env)
		return false;

	strBuffer = env;
	return true;
}

bool apiGetEnvironmentVariable(const wchar_t *lpwszName, FARString &strBuffer)
{
	return apiGetEnvironmentVariable(Wide2MB(lpwszName).c_str(), strBuffer);
}

FARString &strCurrentDirectory()
{
	static FARString strCurrentDirectory;
	return strCurrentDirectory;
}

void InitCurrentDirectory()
{
	// get real curdir:
	WCHAR Buffer[MAX_PATH];
	DWORD Size = WINPORT(GetCurrentDirectory)(ARRAYSIZE(Buffer), Buffer);
	if (Size) {
		FARString strInitCurDir;
		if (Size > ARRAYSIZE(Buffer)) {
			LPWSTR InitCurDir = strInitCurDir.GetBuffer(Size);
			WINPORT(GetCurrentDirectory)(Size, InitCurDir);
			strInitCurDir.ReleaseBuffer(Size - 1);
		} else {
			strInitCurDir.Copy(Buffer, Size);
		}
		// set virtual curdir:
		apiSetCurrentDirectory(strInitCurDir);
	}
}

DWORD apiGetCurrentDirectory(FARString &strCurDir)
{
	// never give outside world a direct pointer to our internal string
	// who knows what they gonna do
	strCurDir.Copy(strCurrentDirectory().CPtr(), strCurrentDirectory().GetLength());
	return static_cast<DWORD>(strCurDir.GetLength());
}

BOOL apiSetCurrentDirectory(LPCWSTR lpPathName, bool Validate)
{
	// correct path to our standard
	FARString strDir = lpPathName;
	if (lpPathName[0] != GOOD_SLASH || lpPathName[1] != 0)
		DeleteEndSlash(strDir);
	// LPCWSTR CD=strDir;
	//	int Offset=HasPathPrefix(CD)?4:0;
	///	if ((CD[Offset] && CD[Offset+1]==L':' && !CD[Offset+2]) || IsLocalVolumeRootPath(CD))
	// AddEndSlash(strDir);

	if (strDir == strCurrentDirectory())
		return TRUE;

	if (Validate) {
		DWORD attr = WINPORT(GetFileAttributes)(lpPathName);
		if (attr == 0xffffffff) {
			fprintf(stderr, "apiSetCurrentDirectory: get attr error %u for %ls\n", WINPORT(GetLastError()),
					lpPathName);
			return FALSE;
		} else if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			fprintf(stderr, "apiSetCurrentDirectory: not dir attr 0x%x for %ls\n", attr, lpPathName);
			return FALSE;
		}
	}

	strCurrentDirectory() = strDir;

	// try to synchronize far cur dir with process cur dir
	// WTF??? if(CtrlObject && CtrlObject->Plugins.GetOemPluginsCount())
	{
		if (!WINPORT(SetCurrentDirectory)(strCurrentDirectory())) {
			fprintf(stderr, "apiSetCurrentDirectory: set curdir error %u for %ls\n", WINPORT(GetLastError()),
					lpPathName);
			if (Validate) {
				return FALSE;
			}
		}
	}

	return TRUE;
}

void apiGetTempPath(FARString &strBuffer)
{
	strBuffer = InMyTemp();
};

bool apiExpandEnvironmentStrings(const wchar_t *src, FARString &strDest)
{
	std::string s;
	Wide2MB(src, s);
	bool out = Environment::ExpandString(s, false);
	strDest = s;
	return out;
}

BOOL apiGetVolumeInformation(const wchar_t *lpwszRootPathName, FARString *pVolumeName,
		DWORD64 *lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
		FARString *pFileSystemName)
{
	struct statvfs svfs {};
	const std::string &path = Wide2MB(lpwszRootPathName);
	if (sdc_statvfs(path.c_str(), &svfs) != 0) {
		return FALSE;
	}

	if (lpMaximumComponentLength)
		*lpMaximumComponentLength = svfs.f_namemax;
	if (lpVolumeSerialNumber)
		*lpVolumeSerialNumber = (DWORD)svfs.f_fsid;
	if (lpFileSystemFlags)
		*lpFileSystemFlags = 0;		// TODO: svfs.f_flags;

	if (pVolumeName) {
		pVolumeName->Clear();
#if 0
#if defined(FS_IOC_GETFSLABEL) && defined(FSLABEL_MAX)
		int fd = open(path.c_str(), O_RDONLY);
		if (fd != -1) {
			char label[FSLABEL_MAX + 1]{};
			if (ioctl(fd, FS_IOC_GETFSLABEL, label) == 0) {
				*pVolumeName = label;
			}
			close(fd);
		}
#endif
#endif
	}

	if (pFileSystemName) {
		*pFileSystemName = MountInfo().GetFileSystem(path);
	}

	return TRUE;
}

void apiFindDataToDataEx(const FAR_FIND_DATA *pSrc, FAR_FIND_DATA_EX *pDest)
{
	pDest->ftCreationTime = pSrc->ftCreationTime;
	pDest->ftLastAccessTime = pSrc->ftLastAccessTime;
	pDest->ftLastWriteTime = pSrc->ftLastWriteTime;
	pDest->ftChangeTime.dwHighDateTime = 0;
	pDest->ftChangeTime.dwLowDateTime = 0;
	pDest->UnixOwner = 0;
	pDest->UnixGroup = 0;
	pDest->UnixDevice = 0;
	pDest->UnixNode = 0;
	pDest->nPhysicalSize = pSrc->nPhysicalSize;
	pDest->nFileSize = pSrc->nFileSize;
	pDest->dwFileAttributes = pSrc->dwFileAttributes;
	pDest->dwUnixMode = pSrc->dwUnixMode;
	pDest->nHardLinks = 1;
	pDest->nBlockSize = 0;
	pDest->strFileName = pSrc->lpwszFileName;
}

void apiFindDataExToData(const FAR_FIND_DATA_EX *pSrc, FAR_FIND_DATA *pDest)
{
	pDest->ftCreationTime = pSrc->ftCreationTime;
	pDest->ftLastAccessTime = pSrc->ftLastAccessTime;
	pDest->ftLastWriteTime = pSrc->ftLastWriteTime;
	pDest->nPhysicalSize = pSrc->nPhysicalSize;
	pDest->nFileSize = pSrc->nFileSize;
	pDest->dwFileAttributes = pSrc->dwFileAttributes;
	pDest->dwUnixMode = pSrc->dwUnixMode;
	pDest->lpwszFileName = wcsdup(pSrc->strFileName);
}

void apiFreeFindData(FAR_FIND_DATA *pData)
{
	free(pData->lpwszFileName);
}

BOOL apiGetFindDataForExactPathName(const wchar_t *lpwszFileName, FAR_FIND_DATA_EX &FindData)
{
	struct stat s{};
	int r = sdc_lstat(Wide2MB(lpwszFileName).c_str(), &s);
	if (r == -1) {
		return FALSE;
	}

	FindData.Clear();

	FindData.nPhysicalSize = ((DWORD64)s.st_blocks) * 512;

	DWORD symattr = 0;
	if ((s.st_mode & S_IFMT) == S_IFLNK) {
		struct stat s2
		{};
		if (sdc_stat(Wide2MB(lpwszFileName).c_str(), &s2) == 0) {
			s = s2;
			symattr = FILE_ATTRIBUTE_REPARSE_POINT;
		} else {
			symattr = FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_BROKEN;
		}
	}

	WINPORT(FileTime_UnixToWin32)(s.st_ctim, &FindData.ftCreationTime);
	WINPORT(FileTime_UnixToWin32)(s.st_atim, &FindData.ftLastAccessTime);
	WINPORT(FileTime_UnixToWin32)(s.st_mtim, &FindData.ftLastWriteTime);
	FindData.ftChangeTime = FindData.ftLastWriteTime;
	FindData.UnixOwner = s.st_uid;
	FindData.UnixGroup = s.st_gid;
	FindData.UnixDevice = s.st_dev;
	FindData.UnixNode = s.st_ino;
	FindData.dwFileAttributes = WINPORT(EvaluateAttributes)(s.st_mode, FindData.strFileName) | symattr;
	FindData.nFileSize = s.st_size;
	FindData.dwUnixMode = s.st_mode;
	FindData.nHardLinks = (DWORD)s.st_nlink;
	FindData.nBlockSize = (DWORD)s.st_blksize;
	FindData.strFileName = PointToName(lpwszFileName);

	if (FindData.nHardLinks > 1)
		FindData.dwFileAttributes |= FILE_ATTRIBUTE_HARDLINKS;

	return TRUE;
}

BOOL apiGetFindDataEx(const wchar_t *lpwszFileName, FAR_FIND_DATA_EX &FindData, DWORD WinPortFindFlags)
{
	FindFile Find(lpwszFileName, true, WinPortFindFlags);
	if (Find.Get(FindData)) {
		return TRUE;
	}

	if (!FindAnyOfChars(lpwszFileName, "*?")) {
		if (apiGetFindDataForExactPathName(lpwszFileName, FindData)) {
			return TRUE;
		}
	}

	fprintf(stderr, "apiGetFindDataEx: FAILED - %ls\n", lpwszFileName);

	FindData.Clear();
	FindData.dwFileAttributes = INVALID_FILE_ATTRIBUTES;	// BUGBUG

	return FALSE;
}

bool apiGetFileSizeEx(HANDLE hFile, UINT64 &Size)
{
	bool Result = false;

	if (WINPORT(GetFileSizeEx)(hFile, reinterpret_cast<PLARGE_INTEGER>(&Size))) {
		Result = true;
	}
	return Result;
}

int apiGetFileTypeByName(const wchar_t *Name)
{
	HANDLE hFile = apiCreateFile(Name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0);

	if (hFile == INVALID_HANDLE_VALUE)
		return FILE_TYPE_UNKNOWN;

	int Type = WINPORT(GetFileType)(hFile);
	WINPORT(CloseHandle)(hFile);
	return Type;
}

BOOL apiGetDiskSize(const wchar_t *Path, uint64_t *TotalSize, uint64_t *TotalFree, uint64_t *UserFree)
{
	struct statfs s = {};
	if (statfs(Wide2MB(Path).c_str(), &s) != 0) {
		return FALSE;
	}
	*TotalSize = *TotalFree = *UserFree = s.f_bsize;	// f_frsize
	*TotalSize*= s.f_blocks;
	*TotalFree*= s.f_bfree;
	*UserFree*= s.f_bavail;
	return TRUE;
}

BOOL apiCreateDirectory(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	if (WINPORT(CreateDirectory)(lpPathName, lpSecurityAttributes))
		return TRUE;

	if (apiIsDevNull(lpPathName))
		return TRUE;

	return FALSE;
}

DWORD apiGetFileAttributes(LPCWSTR lpFileName)
{
	DWORD Result = WINPORT(GetFileAttributes)(lpFileName);
	return Result;
}

BOOL apiSetFileAttributes(LPCWSTR lpFileName, DWORD dwFileAttributes)
{
	if (WINPORT(SetFileAttributes)(lpFileName, dwFileAttributes))
		return TRUE;

	if (apiIsDevNull(lpFileName))
		return TRUE;

	return FALSE;
}

bool apiPathExists(LPCWSTR lpPathName)
{
	struct stat s {};
	return sdc_lstat(Wide2MB(lpPathName).c_str(), &s) == 0;
}

bool apiPathIsDir(LPCWSTR lpPathName)
{
	struct stat s {};
	return sdc_stat(Wide2MB(lpPathName).c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}

bool apiPathIsFile(LPCWSTR lpPathName)
{
	struct stat s {};
	return sdc_stat(Wide2MB(lpPathName).c_str(), &s) == 0 && S_ISREG(s.st_mode);
}

IUnmakeWritablePtr apiMakeWritable(LPCWSTR lpFileName)
{
	FARString strFullName;
	ConvertNameToFull(lpFileName, strFullName);
	if (apiIsDevNull(strFullName))
		return IUnmakeWritablePtr();

	struct UnmakeWritable : IUnmakeWritable
	{
		std::string target, dir;
		mode_t target_mode, dir_mode;
		unsigned long target_flags, dir_flags;
		bool target_flags_modified, dir_flags_modified;
		UnmakeWritable()
			:
			target_mode(0), dir_mode(0), target_flags_modified(false), dir_flags_modified(false)
		{}

		virtual void Unmake()
		{
			if (target_mode) {
				chmod(target.c_str(), target_mode);
			}

			if (dir_mode) {
				chmod(dir.c_str(), dir_mode);
			}

			if (target_flags_modified) {
				sdc_fs_flags_set(target.c_str(), target_flags);
			}

			if (dir_flags_modified) {
				sdc_fs_flags_set(dir.c_str(), dir_flags);
			}
		}
	} *um = new UnmakeWritable;

	// dont want to trigger sudo due to missing +w so use sdc_* for chmod
	um->target = strFullName.GetMB();
	struct stat s = {};

	if (um->target.size() > 1) {
		um->dir = um->target;
		size_t p = um->dir.rfind(GOOD_SLASH, um->dir.size() - 2);
		if (p != std::string::npos) {
			um->dir.resize(p);
			if (stat(um->dir.c_str(), &s) == 0 && (s.st_mode & S_IWUSR) != S_IWUSR) {
				if (chmod(um->dir.c_str(), s.st_mode | S_IWUSR) == 0) {
					um->dir_mode = s.st_mode;
				}
			}
		}
	}

	if (stat(um->target.c_str(), &s) == 0 && (s.st_mode & S_IWUSR) != S_IWUSR) {
		if (chmod(um->target.c_str(), s.st_mode | S_IWUSR) == 0) {
			um->target_mode = s.st_mode;
		}
	}

#if defined(__CYGWIN__)
// TODO: handle chattr +i
#else
	if (!um->dir.empty() && sdc_fs_flags_get(um->dir.c_str(), &um->dir_flags) != -1
			&& FS_FLAGS_CONTAIN_IMMUTABLE(um->dir_flags)) {
		if (sdc_fs_flags_set(um->dir.c_str(), FS_FLAGS_WITHOUT_IMMUTABLE(um->dir_flags)) != -1) {
			um->dir_flags_modified = true;
		}
	}

	if ((s.st_mode & S_IFMT) == S_IFREG		// calling sdc_fs_flags_get on special files useless and may stuck
			&& sdc_fs_flags_get(um->target.c_str(), &um->target_flags) != -1
			&& FS_FLAGS_CONTAIN_IMMUTABLE(um->target_flags)) {
		if (sdc_fs_flags_set(um->target.c_str(), FS_FLAGS_WITHOUT_IMMUTABLE(um->target_flags)) != -1) {
			um->target_flags_modified = true;
		}
	}
#endif

	if (um->target_mode == 0 && um->dir_mode == 0 && !um->target_flags_modified && !um->dir_flags_modified) {
		delete um;
		um = nullptr;
	}

	return IUnmakeWritablePtr(um);
}

bool apiCreateSymbolicLink(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags)
{
	return false;	// todo
}

BOOL apiCreateHardLink(LPCWSTR lpFileName, LPCWSTR lpExistingFileName,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	return FALSE;	// todo
}

bool apiGetFinalPathNameByHandle(HANDLE hFile, FARString &FinalFilePath)
{
	FinalFilePath = L"";
	return false;
}

bool GetFileTimeEx(HANDLE Object, LPFILETIME CreationTime, LPFILETIME LastAccessTime,
		LPFILETIME LastWriteTime, LPFILETIME ChangeTime)
{
	memset(ChangeTime, 0, sizeof(*ChangeTime));
	return WINPORT(GetFileTime)(Object, CreationTime, LastAccessTime, LastWriteTime) != FALSE;
}

bool SetFileTimeEx(HANDLE Object, const FILETIME *CreationTime, const FILETIME *LastAccessTime,
		const FILETIME *LastWriteTime, const FILETIME *ChangeTime)
{
	return WINPORT(SetFileTime)(Object, CreationTime, LastAccessTime, LastWriteTime) != FALSE;
}
