#include <string>
#include <locale> 
#include <set> 
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <utils.h>

#include <wx/wx.h>
#include <wx/display.h>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "sudo.h"




template <class CHAR_T>
static DWORD EvaluateAttributesT(uint32_t unix_mode, const CHAR_T *name)
{
	DWORD rv = 0;
	switch (unix_mode & S_IFMT) {
		case S_IFCHR: rv = FILE_ATTRIBUTE_DEVICE; break;
		case S_IFDIR: rv = FILE_ATTRIBUTE_DIRECTORY; break;
		case S_IFREG: rv = FILE_ATTRIBUTE_ARCHIVE; break;
#ifndef _WIN32
		case S_IFLNK: rv = FILE_ATTRIBUTE_REPARSE_POINT; break;
		case S_IFSOCK: rv = FILE_ATTRIBUTE_DEVICE; break;
#endif
		default: rv = FILE_ATTRIBUTE_DEVICE;
	}

	if (name) {
		bool dotfile = (*name == '.');
		for (; *name; ++name) {
			if (name[0]==GOOD_SLASH) {
				dotfile = (name[1] == '.');
			}
		}
		if (dotfile)
			rv|= FILE_ATTRIBUTE_HIDDEN;
	}

	if ((unix_mode & (S_IXUSR | S_IXGRP | S_IXOTH))!=0)
		rv|= FILE_ATTRIBUTE_EXECUTABLE;

	return rv;
}
	
extern "C"
{
	struct WinPortHandleFile : WinPortHandle
	{
		WinPortHandleFile(int fd_ = -1) : fd(fd_) {}

		virtual ~WinPortHandleFile()
		{
			sdc_close(fd);
		}
		int fd;
	};
	

	
	
	WINPORT_DECL(CreateDirectory, BOOL, (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes ))
	{
		std::string path = ConsumeWinPath(lpPathName);
		int r = sdc_mkdir(path.c_str(), 0775);
		if (r==-1) {
			WINPORT(TranslateErrno)();
			fprintf(stderr, "Failed to create directory: %s errno %u\n", path.c_str(), errno);
			return FALSE;
		}
		
		return TRUE;
	}


	BOOL WINPORT(RemoveDirectory)( LPCWSTR lpDirName)
	{
		std::string path = ConsumeWinPath(lpDirName);
		int r = sdc_rmdir(path.c_str());
		if (r==-1) {
			WINPORT(TranslateErrno)();
			fprintf(stderr, "Failed to remove directory: %s errno %u\n", path.c_str(),errno);
			return FALSE;
		}
		return TRUE;
	}

	BOOL WINPORT(DeleteFile)( LPCWSTR lpFileName)
	{
		std::string path = ConsumeWinPath(lpFileName);
		int r = sdc_remove(path.c_str());
		if (r==-1) {
			WINPORT(TranslateErrno)();
			fprintf(stderr, "Failed to remove file: %s errno %u\n", path.c_str(), errno);
			return FALSE;
		}
		return TRUE;
	}


	HANDLE WINPORT(CreateFile)( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, 
		DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
	{
		//return CreateFile( lpFileName, dwDesiredAccess, dwShareMode,
		//	lpSecurityAttributes, dwCreationDisposition, 
		//	dwFlagsAndAttributes, hTemplateFile);
		int flags = 0;
		if (dwDesiredAccess & (GENERIC_WRITE|GENERIC_ALL|FILE_WRITE_DATA|FILE_WRITE_ATTRIBUTES)) flags = O_RDWR;
		else if (dwDesiredAccess & (GENERIC_READ|GENERIC_ALL|FILE_READ_DATA|FILE_READ_ATTRIBUTES)) flags = O_RDONLY;
#ifdef _WIN32
		flags|= O_BINARY;
#else		
		flags|= O_CLOEXEC;
#ifdef __linux__
		if ((dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH) != 0)
			flags|= O_SYNC;

		if ((dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING) != 0)
			flags|= O_DIRECT;
		
#endif
#endif
		switch (dwCreationDisposition) 
		{
		case CREATE_ALWAYS: flags|= O_CREAT | O_TRUNC; break;
		case CREATE_NEW: flags|= O_CREAT | O_EXCL; break;
		case OPEN_ALWAYS: flags|= O_CREAT; break;
		case OPEN_EXISTING: break;
		case TRUNCATE_EXISTING: flags|= O_TRUNC; break;
		}
		std::string path = ConsumeWinPath(lpFileName);
		int r = sdc_open(path.c_str(), flags, (dwFlagsAndAttributes&FILE_ATTRIBUTE_EXECUTABLE) ? 0755 : 0644);		
		if (r==-1) {
			WINPORT(TranslateErrno)();

			fprintf(stderr, "CreateFile: " WS_FMT " - dwDesiredAccess=0x%x flags=0x%x path=%s errno=%d\n", 
				lpFileName, dwDesiredAccess, flags, path.c_str(), errno);
				
			return INVALID_HANDLE_VALUE;
		}

#ifndef __linux__
		if ((dwFlagsAndAttributes & (FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING)) != 0) {
			fcntl(r, F_NOCACHE, 1);
		}
#endif
		/*nobody cares.. if ((dwFlagsAndAttributes&FILE_FLAG_BACKUP_SEMANTICS)==0) {
			struct stat s = { };
			sdc_fstat(r, &s);
			if ( (s.st_mode & S_IFMT) == FILE_ATTRIBUTE_DIRECTORY) {
				sdc_close(r);
				WINPORT(SetLastError)(ERROR_DIRECTORY);
				return INVALID_HANDLE_VALUE;
			}
		}*/

		return WinPortHandle_Register(new WinPortHandleFile(r));
	}

	BOOL WINPORT(MoveFile)(LPCWSTR ExistingFileName, LPCWSTR NewFileName )
	{
		struct stat s;
		if (sdc_stat(ConsumeWinPath(NewFileName).c_str(), &s)==0) {
			WINPORT(SetLastError)(ERROR_ALREADY_EXISTS);
			return false;			
		}
			
		return (rename(ConsumeWinPath(ExistingFileName).c_str(), ConsumeWinPath(NewFileName).c_str())==0);
	}

	BOOL WINPORT(MoveFileEx)(LPCWSTR ExistingFileName, LPCWSTR NewFileName,DWORD dwFlags)
	{
		if ((dwFlags&MOVEFILE_REPLACE_EXISTING)==0)
			return WINPORT(MoveFile)(ExistingFileName, NewFileName);

		return (rename(ConsumeWinPath(ExistingFileName).c_str(), ConsumeWinPath(NewFileName).c_str())==0);
	}

	DWORD WINPORT(GetCurrentDirectory)( DWORD  nBufferLength, LPWSTR lpBuffer)
	{
		std::vector<char> buf(nBufferLength + 1);
		if (!_getcwd(&buf[0], nBufferLength)) {
			return (nBufferLength < 1024) ? 1024 : 0;
		}
		
		std::wstring u16 = MB2Wide(&buf[0]);
		memcpy(lpBuffer, u16.c_str(), (u16.size() + 1) * sizeof(*lpBuffer));
		return (DWORD)u16.size();
	}

	BOOL WINPORT(SetCurrentDirectory)(LPCWSTR lpPathName)
	{
		return (sdc_chdir(ConsumeWinPath(lpPathName).c_str())==0) ? TRUE : FALSE;
	}

	BOOL WINPORT(GetFileSizeEx)( HANDLE hFile, PLARGE_INTEGER lpFileSize)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FALSE;
		}
#ifdef _WIN32
		__int64 len = _filelength(wph->fd);
		if (len==(__int64)-1)
			return FALSE;
		lpFileSize->QuadPart = len;
#else
		struct stat s = {0};
		if (sdc_fstat(wph->fd,  &s)<0)
			return FALSE;
		lpFileSize->QuadPart = s.st_size;
#endif
		return TRUE;
	}

	DWORD WINPORT(GetFileSize)( HANDLE  hFile, LPDWORD lpFileSizeHigh)
	{
		LARGE_INTEGER sz64 = {0};
		if (!WINPORT(GetFileSizeEx)(hFile, &sz64)) {
			if (lpFileSizeHigh) *lpFileSizeHigh = 0;
			return INVALID_FILE_SIZE;
		}
		if (lpFileSizeHigh) *lpFileSizeHigh = sz64.HighPart;

		return sz64.LowPart;
	}

	BOOL WINPORT(ReadFile)( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FALSE;
		}
		if (lpOverlapped) {
			fprintf(stderr, "WINPORT(ReadFile) with lpOverlapped\n");
		}
		
		ssize_t done = 0, remain = nNumberOfBytesToRead;
		for (;;) {
			if (!remain) break;
			ssize_t r = sdc_read(wph->fd, lpBuffer, remain);
			if (r < 0) {
				if (done==0)
					return FALSE;
				break;
			}
			if (!r) break;
			lpBuffer = (char *)lpBuffer + r;
			remain-= r;
			done+= r;
		}
		
		//if (r!=nNumberOfBytesToRead && IsDebuggerPresent()) DebugBreak();

		if (lpNumberOfBytesRead) 
			*lpNumberOfBytesRead = done;

		return TRUE;

	}

	BOOL WINPORT(WriteFile)( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
		LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FALSE;
		}
		if (lpOverlapped) {
			fprintf(stderr, "WINPORT(WriteFile) with lpOverlapped\n");
		}

		ssize_t r = sdc_write(wph->fd, lpBuffer, nNumberOfBytesToWrite);
		if (r < 0)
			return FALSE;

		if (lpNumberOfBytesWritten) 
			*lpNumberOfBytesWritten = r;
		return TRUE;
	}

	BOOL WINPORT(SetFilePointerEx)( HANDLE hFile, LARGE_INTEGER liDistanceToMove, 
		PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FALSE;
		}
		int whence;
		switch (dwMoveMethod) {
		case FILE_BEGIN: whence = SEEK_SET; break;
		case FILE_CURRENT: whence = SEEK_CUR; break;
		case FILE_END: whence = SEEK_END; break;
		default:
			return INVALID_SET_FILE_POINTER;
		}

		off_t r = sdc_lseek(wph->fd, liDistanceToMove.QuadPart, whence);
		if (r==(off_t)-1)
			return FALSE;
		if (lpNewFilePointer) lpNewFilePointer->QuadPart = r;
		return TRUE;
	}

	DWORD WINPORT(SetFilePointer)( HANDLE hFile, LONG lDistanceToMove, PLONG  lpDistanceToMoveHigh, DWORD  dwMoveMethod)
	{
		LARGE_INTEGER liDistanceToMove, liNewFilePointer = {0};
		if (lpDistanceToMoveHigh) {
			liDistanceToMove.LowPart = lDistanceToMove;
			liDistanceToMove.HighPart = lpDistanceToMoveHigh ? *lpDistanceToMoveHigh : 0;			
		} else {
			liDistanceToMove.LowPart = lDistanceToMove;
			liDistanceToMove.HighPart = (lDistanceToMove < 0) ? -1 : 0;
		}
		if (!WINPORT(SetFilePointerEx)( hFile, liDistanceToMove, &liNewFilePointer, dwMoveMethod))
			return INVALID_SET_FILE_POINTER;

		if (lpDistanceToMoveHigh)
			*lpDistanceToMoveHigh = liNewFilePointer.HighPart;

		return liNewFilePointer.LowPart;
	}

	BOOL WINPORT(GetFileTime)( HANDLE hFile, LPFILETIME lpCreationTime, 
		LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FALSE;
		}
		struct stat s = {0};
		if (sdc_fstat(wph->fd, &s) < 0)
			return FALSE;
			
		WINPORT(FileTime_UnixToWin32)(s.st_mtim, lpLastWriteTime);
		WINPORT(FileTime_UnixToWin32)(s.st_ctim, lpCreationTime);
		WINPORT(FileTime_UnixToWin32)(s.st_atim, lpLastAccessTime);
		return TRUE;
	}
	

	DWORD WINPORT(EvaluateAttributes)(uint32_t unix_mode, const WCHAR *name)
	{
		return EvaluateAttributesT(unix_mode, name);
	}
	
	DWORD WINPORT(EvaluateAttributesA)(uint32_t unix_mode, const char *name)
	{
		return EvaluateAttributesT(unix_mode, name);		
	}
	
	static int stat_symcheck(const char *path, struct stat &s, DWORD &symattr)
	{
		if (sdc_lstat(path, &s) < 0) {
			fprintf(stderr, "stat_symcheck: lstat failed for %s\n", path);
			return -1;
		}
		
		if ((s.st_mode & S_IFMT) == S_IFLNK) {
			struct stat sdst = {0};
			if (sdc_stat(path, &sdst) == 0) {
				s = sdst;
				symattr = FILE_ATTRIBUTE_REPARSE_POINT;
			} else {
				fprintf(stderr, "stat_symcheck: stat failed for %s\n", path);
				symattr = FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_BROKEN;
			}
		} else
			symattr = 0;
		
		return 0;
	}
	
	

	DWORD WINPORT(GetFileAttributes)(LPCWSTR lpFileName)
	{
		struct stat s = { };
		const std::string &path = ConsumeWinPath(lpFileName);
		
		DWORD symattr = 0;
		if (stat_symcheck(path.c_str(), s, symattr) < 0 ) {
			WINPORT(TranslateErrno)();
			return INVALID_FILE_ATTRIBUTES;			
		}
		
		return ( symattr | WINPORT(EvaluateAttributes)(s.st_mode, lpFileName) );
	}

	DWORD WINPORT(SetFileAttributes)(LPCWSTR lpFileName, DWORD dwAttributes)
	{
		fprintf(stderr, "NOT SUPPORTED: SetFileAttributes('%ls', 0x%x)\n", lpFileName, dwAttributes);
		return TRUE;
	}

	BOOL WINPORT(SetEndOfFile)( HANDLE hFile)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FALSE;
		}

		off_t pos = sdc_lseek(wph->fd, 0, SEEK_CUR);
		if (pos==(off_t)-1)
			return FALSE;
		if (sdc_ftruncate(wph->fd, pos) == -1)
			return FALSE;

		return TRUE;
	}

	BOOL WINPORT(FlushFileBuffers)( HANDLE hFile)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FALSE;
		}
		//fsync(wph->fd);
		return TRUE;
	}

	DWORD WINPORT(GetFileType)( HANDLE hFile)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FILE_TYPE_UNKNOWN;
		}

#ifdef _WIN32
		return FILE_TYPE_DISK;//::GetFileType(hFile);
#else
		struct stat s;
		if (sdc_fstat(wph->fd, &s) == 0) {

			switch (s.st_mode & S_IFMT) {
			case S_IFCHR: return FILE_TYPE_CHAR;
			case S_IFDIR: return FILE_TYPE_DISK;
			case S_IFREG: return FILE_TYPE_DISK;
#ifndef _WIN32
			case S_IFLNK: return FILE_TYPE_DISK;
			case S_IFSOCK: return FILE_TYPE_PIPE;
#endif
			}
		}
		return FILE_TYPE_UNKNOWN;
#endif
	}
	
	WINPORT_DECL(GetFileDescriptor, int, (HANDLE hFile))
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return -1;
		}
		
		return wph->fd;
	}

	//////////////////////////////////
	static void FillWFD(const wchar_t *name, const struct stat &s, WIN32_FIND_DATAW *lpFindFileData, DWORD add_attr)
	{
		lpFindFileData->dwFileAttributes = (add_attr | WINPORT(EvaluateAttributes)(s.st_mode, name));
		WINPORT(FileTime_UnixToWin32)(s.st_mtim, &lpFindFileData->ftLastWriteTime);
		WINPORT(FileTime_UnixToWin32)(s.st_ctim, &lpFindFileData->ftCreationTime);
		WINPORT(FileTime_UnixToWin32)(s.st_atim, &lpFindFileData->ftLastAccessTime);
		lpFindFileData->nFileSizeHigh = (DWORD)(((uint64_t)s.st_size >> 32) & 0xffffffff);
		lpFindFileData->nFileSizeLow = (DWORD)(s.st_size & 0xffffffff);
		lpFindFileData->dwReserved0 = 
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) ? IO_REPARSE_TAG_SYMLINK : 0;
		lpFindFileData->dwReserved1 = 0;
		lpFindFileData->dwUnixMode = s.st_mode;
		wcsncpy(lpFindFileData->cFileName, name, MAX_NAME - 1);
	}


	struct UnixFindFile
	{
		UnixFindFile()
		{
#ifdef _WIN32
			_h = INVALID_HANDLE_VALUE;
#else
			_d = NULL;
#endif
		}
		UnixFindFile(const std::string &root, const std::string &mask) 
		{
			_root = root;
			_mask = mask;
#ifdef _WIN32
			_h = INVALID_HANDLE_VALUE;
#else
			if (_root.size() > 1 && _root[_root.size()-1]==GOOD_SLASH)
				_root.resize(_root.size()-1);
			_d = sdc_opendir(_root.c_str());
			if (!_d) {
				fprintf(stderr, "opendir failed on %s\n", _root.c_str());
			} 
				
#endif
		}

		~UnixFindFile()
		{
#ifdef _WIN32
			if (_h!=INVALID_HANDLE_VALUE) ::FindClose(_h);
#else
			if (_d) sdc_closedir(_d);
#endif
		}

		bool Iterate(LPWIN32_FIND_DATAW lpFindFileData)
		{
#ifdef _WIN32
			WIN32_FIND_DATAA wfd;
			for (;;) {
				if (_h==INVALID_HANDLE_VALUE) {
					_h = ::FindFirstFileA((_root + "*").c_str(), &wfd);
					if (_h==INVALID_HANDLE_VALUE) 
						return false;

				} else if (!::FindNextFileA(_h, &wfd))
					return false;

				if (_mask.empty() || MatchWildcard(wfd.cFileName, _mask.c_str())) break;
			}
			std::string path(_root);
			if (path.empty() || path[path.size()-1]!=GOOD_SLASH) path+= GOOD_SLASH;
			path+= wfd.cFileName;
			std::wstring utf16 = MB2Wide(wfd.cFileName);
#else
			struct dirent *de;
			for (;;) {
				if (!_d)
					return false;

				de = sdc_readdir(_d);
				if (!de)
					return false;
				if (_mask.empty() || MatchWildcard(de->d_name, _mask.c_str())) break;
			}
			std::string path(_root);
			if (path.empty() || path[path.size()-1]!=GOOD_SLASH) path+= GOOD_SLASH;
			path+= de->d_name;
			std::wstring utf16 = MB2Wide(de->d_name);
#endif

			struct stat s = { };
			DWORD symattr = 0;
			if (stat_symcheck(path.c_str(), s, symattr) < 0 ) {
				fprintf(stderr, "UnixFindFile: stat failed for %s\n",path.c_str());
			}
						
			FillWFD((const wchar_t *)utf16.c_str(), s, lpFindFileData, symattr);
			//fprintf(stderr, "stat attr %x for %s\n", lpFindFileData->dwFileAttributes , path.c_str());
			return true;
		}

	private:
#ifdef _WIN32
		HANDLE _h;
#else
		DIR *_d;
#endif
		std::string _root;
		std::string _mask;
	};

	struct UnixFindFiles : std::set<UnixFindFile *>, std::mutex
	{
	} g_unix_find_files;

	static UnixFindFile g_unix_found_file_dummy;

	HANDLE WINPORT(FindFirstFile)(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
	{
		//return ::FindFirstFile(lpFileName, lpFindFileData);
		std::string root(ConsumeWinPath(lpFileName)), mask;
		size_t p = root.rfind(GOOD_SLASH);
		if (p!=std::string::npos) {
			mask.assign(root.c_str() + p + 1);
			root.resize(p + 1);
		} else {
			fprintf(stderr, "FindFirstFile: no slash in root='%s' for lpFileName='%ls'\n", root.c_str(), lpFileName);
		}

		if (mask=="*" || mask=="*.*") mask.clear();
		if (!mask.empty() && mask.find('*')==std::string::npos && mask.find('?')==std::string::npos) {

			struct stat s = { };
			DWORD symattr = 0;
			if (stat_symcheck((root + mask).c_str(), s, symattr) < 0 ) {
				WINPORT(TranslateErrno)();
				return INVALID_HANDLE_VALUE;			
			}

			LPCWSTR name = wcsrchr(lpFileName, GOOD_SLASH);
			if (name) ++name; else name = lpFileName;
			FillWFD(name, s, lpFindFileData, symattr);
			return (HANDLE)&g_unix_found_file_dummy;
		}


		UnixFindFile *uff = new UnixFindFile(root, mask);
		if (!uff->Iterate(lpFindFileData)) {
			WINPORT(TranslateErrno)();
			delete uff;
			fprintf(stderr, "find mask: %s (for %ls) FAILED\n", mask.c_str(), lpFileName);
			WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);
			return INVALID_HANDLE_VALUE;
		}
		//fprintf(stderr, "find mask: %s  (for %ls) - %ls", mask.c_str(), lpFileName, lpFindFileData->cFileName);

		std::lock_guard<std::mutex> lock(g_unix_find_files);
		g_unix_find_files.insert(uff);
		return (HANDLE)uff;
	}

	BOOL WINPORT(FindNextFile)(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
	{
		//return ::FindNextFile(hFindFile, lpFindFileData);
		UnixFindFile *uff = (UnixFindFile *)hFindFile;
		if (uff==&g_unix_found_file_dummy)
			return FALSE;
		std::lock_guard<std::mutex> lock(g_unix_find_files);
		if (g_unix_find_files.find(uff)==g_unix_find_files.end())
			return FALSE;

		if (!uff->Iterate(lpFindFileData)) {
			WINPORT(SetLastError)(ERROR_NO_MORE_FILES);
			return FALSE;
		}
		//fprintf(stderr, "FindNextFile - %ls\n", lpFindFileData->cFileName);


		return TRUE;
	}

	BOOL WINPORT(FindClose)(HANDLE hFindFile)
	{
		//return ::FindClose(hFindFile);
		UnixFindFile *uff = (UnixFindFile *)hFindFile;
		if (uff==&g_unix_found_file_dummy)
			return TRUE;
		{
			std::lock_guard<std::mutex> lock(g_unix_find_files);
			if (g_unix_find_files.erase(uff)==0)
				return FALSE;
		}

		delete uff;
		return TRUE;
	}

	WINPORT_DECL(GetDriveType, UINT, (LPCWSTR lpRootPathName))
	{
		return DRIVE_FIXED;
	}

	WINPORT_DECL(GetTempFileName, UINT,( LPCWSTR path, LPCWSTR prefix, UINT unique, LPWSTR buffer ))
	{
		static const WCHAR formatW[] = {'%','x','.','t','m','p',0};
		int i;
		LPWSTR p;
		DWORD attr;

		if ( !path || !buffer )
		{
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return 0;
		}

		/* ensure that the provided directory exists */
		attr = WINPORT(GetFileAttributes)(path);
		if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
		{
			WINPORT(SetLastError)( ERROR_DIRECTORY );
			return 0;
		}

		size_t path_len = wcslen(path);
		if (path_len>=MAX_PATH) {
			WINPORT(SetLastError)( ERROR_BUFFER_OVERFLOW );
			return 0;
		}
		wcscpy( buffer, path );
		p = buffer + path_len;

		/* add a \, if there isn't one  */
		if ((p == buffer) || (p[-1] !=GOOD_SLASH)) *p++ = GOOD_SLASH;

		if (prefix)
			for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;

		unique &= 0xffff;

		if (unique) swprintf( p, MAX_PATH - 1 - (p - buffer), formatW, unique );
		else
		{
			/* get a "random" unique number and try to create the file */
			HANDLE handle;
			UINT num = WINPORT(GetTickCount)() & 0xffff;
			static UINT last;

			/* avoid using the same name twice in a short interval */
			if (last - num < 10) num = last + 1;
			if (!num) num = 1;
			unique = num;
			do
			{
				swprintf( p, MAX_PATH - 1 - (p - buffer), formatW, unique );
				handle = WINPORT(CreateFile)( buffer, GENERIC_WRITE, 0, NULL,
					CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
				if (handle != INVALID_HANDLE_VALUE)
				{  /* We created it */
					WINPORT(CloseHandle)( handle );
					last = unique;
					break;
				}
				if (WINPORT(GetLastError)() != ERROR_FILE_EXISTS &&
					WINPORT(GetLastError)() != ERROR_SHARING_VIOLATION)
					break;  /* No need to go on */
				if (!(++unique & 0xffff)) unique = 1;
			} while (unique != num);
		}

		return unique;
	}

	WINPORT_DECL(GetFullPathName, DWORD, 
		(LPCTSTR lpFileName,  DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart))
	{
		std::wstring full_name;
		if (*lpFileName!=GOOD_SLASH) {
			WCHAR cd[MAX_PATH+1] = {0};
			WINPORT(GetCurrentDirectory)( MAX_PATH, cd);
			full_name = cd;
			if (*lpFileName!='.') {
				full_name+=GOOD_SLASH;
				full_name+= lpFileName;
			} else
				full_name+= lpFileName + 1;
		} else
			full_name = lpFileName;
		if (nBufferLength<=full_name.size())
			return full_name.size() + 1;
		
		memcpy(lpBuffer, full_name.c_str(), (full_name.size() + 1) * sizeof(WCHAR));
		if (lpFilePart) {
			WCHAR *slash = wcsrchr(lpBuffer, GOOD_SLASH);
			*lpFilePart = slash ? slash + 1 : lpBuffer;
		}
		return full_name.size();
	}
}
