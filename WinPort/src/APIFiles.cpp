#include <string>
#include <locale> 
#include <set> 
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <utils.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "sudo.h"
#include <os_call.hpp>

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

	if ((unix_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)
		rv|= FILE_ATTRIBUTE_EXECUTABLE;

	if ((unix_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		rv|= FILE_ATTRIBUTE_READONLY;

	return rv;
}
	
extern "C"
{
	struct WinPortHandleFile : WinPortHandle
	{
		WinPortHandleFile(int fd_ = -1) : fd(fd_) {}

		virtual ~WinPortHandleFile()
		{
			if (os_call_int(sdc_close, fd) == -1 ){
				fprintf(stderr, "~WinPortHandleFile: error %u closing fd %d", errno, fd);
			}	
		}
		int fd;
	};
	

	
	
	WINPORT_DECL(CreateDirectory, BOOL, (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes ))
	{
		const std::string &path = ConsumeWinPath(lpPathName);
		int r = os_call_int(sdc_mkdir, path.c_str(), (mode_t)0775);
			
		return (r == -1) ? FALSE : TRUE;
	}


	BOOL WINPORT(RemoveDirectory)( LPCWSTR lpDirName)
	{
		const std::string &path = ConsumeWinPath(lpDirName);
		int r = os_call_int(sdc_rmdir, path.c_str());

		if (r == -1){
			// fprintf(stderr, "Failed to remove directory: '%s' errno %u\n", path.c_str(),errno);
			return FALSE;
		}
		
		return TRUE;
	}

	BOOL WINPORT(DeleteFile)( LPCWSTR lpFileName)
	{
		const std::string &path = ConsumeWinPath(lpFileName);
		int r = os_call_int(sdc_remove, path.c_str());

		if (r == -1) {
			// fprintf(stderr, "Failed to remove file: '%s' errno %u\n", path.c_str(), errno);
			return FALSE;
		}
		
		return TRUE;
	}

	static int open_all_args(const char* pathname, int flags, mode_t mode)
	{
		return sdc_open(pathname, flags, mode);
	}

	HANDLE WINPORT(CreateFile)( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
		const DWORD *UnixMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
	{
		//return CreateFile( lpFileName, dwDesiredAccess, dwShareMode,
		//	lpSecurityAttributes, dwCreationDisposition, 
		//	dwFlagsAndAttributes, hTemplateFile);
		int flags = 0;
		const bool want_read = (dwDesiredAccess & (GENERIC_READ|GENERIC_ALL|FILE_READ_DATA|FILE_READ_ATTRIBUTES)) != 0;
		const bool want_write = (dwDesiredAccess & (GENERIC_WRITE|GENERIC_ALL|FILE_WRITE_DATA|FILE_WRITE_ATTRIBUTES)) != 0;
		if (want_write) {
			if (want_read) {
				flags = O_RDWR;
			} else {
				flags = O_WRONLY;
			}
		} else if (want_read) {
			flags = O_RDONLY;
		}
#ifdef _WIN32
		flags|= O_BINARY;
#else		
		flags|= O_CLOEXEC;
#ifdef __linux__
		if ((dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH) != 0)
			flags|= O_DSYNC;

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
		const std::string &path = ConsumeWinPath(lpFileName);
		mode_t mode = UnixMode ? *UnixMode : 0644;
		if (dwFlagsAndAttributes & FILE_ATTRIBUTE_EXECUTABLE) {
			mode|= UnixMode ? 0700 : 0711;
		}
		mode&= 0777; // may be allow also SUID/SGID?

		int r = os_call_int(open_all_args, path.c_str(), flags, mode);		
		if (r==-1) {
			//fprintf(stderr, "CreateFile: " WS_FMT " - dwDesiredAccess=0x%x flags=0x%x mode=0%o path=%s errno=%d\n", 
			//	lpFileName, dwDesiredAccess, flags, mode, path.c_str(), errno);
				
			return INVALID_HANDLE_VALUE;
		}

#ifndef __linux__
		if ((dwFlagsAndAttributes & (FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING)) != 0) {
#ifdef __FreeBSD__
			fcntl(r, O_DIRECT, 1);
#elif !defined(__CYGWIN__)
			fcntl(r, F_NOCACHE, 1);
#endif // __FreeBSD__
		}
#endif // __linux__

#if defined(__linux__) || defined(__FreeBSD__)
		if ((dwFlagsAndAttributes & (FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN)) == FILE_FLAG_SEQUENTIAL_SCAN && !want_write) {
			posix_fadvise(r, 0, 0, POSIX_FADV_SEQUENTIAL);
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
		if (os_call_int(sdc_stat, ConsumeWinPath(NewFileName).c_str(), &s)==0) {
			WINPORT(SetLastError)(ERROR_ALREADY_EXISTS);
			return false;			
		}
			
		return (os_call_int(sdc_rename, ConsumeWinPath(ExistingFileName).c_str(), ConsumeWinPath(NewFileName).c_str())==0);
	}

	BOOL WINPORT(MoveFileEx)(LPCWSTR ExistingFileName, LPCWSTR NewFileName,DWORD dwFlags)
	{
		if ((dwFlags&MOVEFILE_REPLACE_EXISTING)==0)
			return WINPORT(MoveFile)(ExistingFileName, NewFileName);

		return (os_call_int(sdc_rename, ConsumeWinPath(ExistingFileName).c_str(), ConsumeWinPath(NewFileName).c_str())==0);
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
		const std::string &path = ConsumeWinPath(lpPathName);
		int r = os_call_int(sdc_chdir, path.c_str());
		if (r == 0)
			return TRUE;
		return FALSE;
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
		struct stat s{};
		if (os_call_int(sdc_fstat, wph->fd,  &s) == -1)
			return FALSE;
		lpFileSize->QuadPart = s.st_size;
#endif
		return TRUE;
	}

	DWORD WINPORT(GetFileSize)( HANDLE  hFile, LPDWORD lpFileSizeHigh)
	{
		LARGE_INTEGER sz64{};
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
			ssize_t r = os_call_v<ssize_t, -1>(sdc_read, wph->fd, lpBuffer, (size_t)remain);
			if (r < 0 && errno == EIO) {
				// workaround for SMB's read error when requested fragment overlaps end of file
				off_t pos = lseek(wph->fd, 0, SEEK_CUR);
				struct stat st{};
				if (pos != -1 && sdc_fstat(wph->fd, &st) == 0
						&& pos < st.st_size && pos + remain > st.st_size) {
					r = os_call_v<ssize_t, -1>(sdc_read, wph->fd, lpBuffer, (size_t)(st.st_size - pos));

				} else {
					errno = EIO;
				}
			}
			if (r < 0) {
				if (done == 0) {
					return FALSE;
				}
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
			if (lpNumberOfBytesWritten)
				*lpNumberOfBytesWritten = 0;
			return FALSE;
		}
		if (lpOverlapped) {
			fprintf(stderr, "WINPORT(WriteFile) with lpOverlapped\n");
		}

		ssize_t r = os_call_v<ssize_t, -1>(sdc_write, wph->fd, lpBuffer, (size_t)nNumberOfBytesToWrite);
		if (r < 0) {
			if (lpNumberOfBytesWritten)
				*lpNumberOfBytesWritten = 0;
			return FALSE;
		}

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
			WINPORT(SetLastError)(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		off_t r = os_call_v<off_t, -1>(sdc_lseek, wph->fd, (off_t)liDistanceToMove.QuadPart, whence);
		if (r==(off_t)-1)
			return FALSE;
		if (lpNewFilePointer) lpNewFilePointer->QuadPart = r;
		return TRUE;
	}

	VOID WINPORT(FileAllocationHint) (HANDLE hFile, DWORD64 HintFileSize)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return;
		}

#ifdef __linux__
		fallocate(wph->fd, FALLOC_FL_KEEP_SIZE, 0, (off_t)HintFileSize);

#elif defined(F_PREALLOCATE)
		fstore_t fst {};
		fst.fst_flags = F_ALLOCATECONTIG;
		fst.fst_posmode = F_PEOFPOSMODE;
		fst.fst_offset = 0;
		fst.fst_length = (off_t)HintFileSize;
		fst.fst_bytesalloc = 0;
		fcntl(wph->fd, F_PREALLOCATE, &fst);
#endif
	}

	DWORD WINPORT(SetFilePointer)( HANDLE hFile, LONG lDistanceToMove, PLONG  lpDistanceToMoveHigh, DWORD  dwMoveMethod)
	{
		LARGE_INTEGER liDistanceToMove, liNewFilePointer = {};
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
		struct stat s{};
		if (os_call_int(sdc_fstat, wph->fd, &s) < 0)
			return FALSE;
			
		WINPORT(FileTime_UnixToWin32)(s.st_mtim, lpLastWriteTime);
		WINPORT(FileTime_UnixToWin32)(s.st_ctim, lpCreationTime);
		WINPORT(FileTime_UnixToWin32)(s.st_atim, lpLastAccessTime);
		return TRUE;
	}

	BOOL WINPORT(SetFileTime)( HANDLE hFile, const FILETIME *lpCreationTime, 
		const FILETIME *lpLastAccessTime, const FILETIME *lpLastWriteTime)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (!wph) {
			return FALSE;
		}

		struct timespec ts[2] = {};
		if (lpLastAccessTime) {
			WINPORT(FileTime_Win32ToUnix)(lpLastAccessTime, &ts[0]);
		}
		if (lpLastWriteTime) {
			WINPORT(FileTime_Win32ToUnix)(lpLastWriteTime, &ts[1]);
		}
		const struct timeval tv[2] = { {ts[0].tv_sec, ts[0].tv_nsec / 1000}, {ts[1].tv_sec, ts[1].tv_nsec / 1000}};
		if (os_call_int(sdc_futimes, wph->fd, tv) < 0)
			return FALSE;

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
		if (os_call_int(sdc_lstat, path, &s) < 0) {
			return -1;
		}
		
		if ((s.st_mode & S_IFMT) == S_IFLNK) {
			struct stat sdst{};
			if (os_call_int(sdc_stat, path, &sdst) == 0) {
				s = sdst;
				symattr = FILE_ATTRIBUTE_REPARSE_POINT;
			} else {
				s.st_size = 0;
				//fprintf(stderr, "stat_symcheck: stat failed for %s\n", path);
				symattr = FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_BROKEN;
			}
		} else
			symattr = 0;
		
		return 0;
	}
	
	

	DWORD WINPORT(GetFileAttributes)(LPCWSTR lpFileName)
	{
		struct stat s{};
		const std::string &path = ConsumeWinPath(lpFileName);
		
		DWORD symattr = 0;
		if (stat_symcheck(path.c_str(), s, symattr) < 0 ) {
			// fprintf(stderr, "GetFileAttributes: stat_symcheck failed for '%s'\n", path.c_str());
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

		off_t pos = os_call_v<off_t, -1>(sdc_lseek, wph->fd, (off_t)0, SEEK_CUR);
		if (pos==(off_t)-1)
			return FALSE;
		if (os_call_int(sdc_ftruncate, wph->fd, pos) == -1)
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
		if (os_call_int(sdc_fstat, wph->fd, &s) == 0) {

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
		lpFindFileData->UnixDevice = s.st_dev;
		lpFindFileData->UnixNode = s.st_ino;
		lpFindFileData->UnixOwner = s.st_uid;
		lpFindFileData->UnixGroup = s.st_gid;
		lpFindFileData->dwUnixMode = s.st_mode;
		lpFindFileData->nHardLinks = (DWORD)s.st_nlink;
		wcsncpy(lpFindFileData->cFileName, name, MAX_NAME - 1);
	}


	struct UnixFindFile
	{
		UnixFindFile(const std::string &root, const std::string &mask, DWORD flags) : _flags(flags)
		{
			_root = root;
			_mask = mask;
			if (_root.size() > 1 && _root[_root.size()-1]==GOOD_SLASH)
				_root.resize(_root.size()-1);
			_d = os_call_pv<DIR>(sdc_opendir, _root.c_str());
			if (!_d) {
				fprintf(stderr, "opendir failed on %s\n", _root.c_str());
			}
		}

		~UnixFindFile()
		{
			if (_d) os_call_int(sdc_closedir, _d);
		}

		bool IsOpened() const
		{
			return (_d != NULL);
		}

		bool Iterate(LPWIN32_FIND_DATAW lpFindFileData)
		{
			struct dirent *de;
			for (;;) {
				if (!_d)
					return false;
				
				errno = 0;
				de = os_call_pv<struct dirent>(sdc_readdir, _d);
				if (!de)
					return false;

				if ( PreMatchDType(de->d_type) && MatchName(de->d_name) ) {
					mode_t hint_mode_type;
					switch (de->d_type) {
						case DT_DIR: hint_mode_type = S_IFDIR; break;
						case DT_REG: hint_mode_type = S_IFREG; break;
						case DT_LNK: hint_mode_type = S_IFLNK; break;
						case DT_BLK: hint_mode_type = S_IFBLK; break;
						case DT_FIFO: hint_mode_type = S_IFIFO; break;
						case DT_CHR: hint_mode_type = S_IFCHR; break;
						case DT_SOCK: hint_mode_type = S_IFSOCK; break;
						default: hint_mode_type = 0; 
					}
					if (MatchAttributesAndFillWFD(hint_mode_type, de->d_name, lpFindFileData))
						return true;
				}
			}
		}

	private:
		bool MatchAttributesAndFillWFD(mode_t hint_mode_type, const char *name, LPWIN32_FIND_DATAW lpFindFileData)
		{
			_tmp.path = _root;
			if (_tmp.path.empty() || _tmp.path[_tmp.path.size()-1] != GOOD_SLASH) 
				_tmp.path+= GOOD_SLASH;

			SudoSilentQueryRegion ssqr(hint_mode_type !=0 && (_flags & FIND_FILE_FLAG_NOT_ANNOYING) != 0);

			_tmp.path+= name;
			struct stat s = { };
			DWORD symattr = 0;
			if (stat_symcheck(_tmp.path.c_str(), s, symattr) < 0 ) {
				fprintf(stderr, "UnixFindFile: stat_symcheck failed for '%s'\n", _tmp.path.c_str());
				symattr|= FILE_ATTRIBUTE_BROKEN;
				if (hint_mode_type)
					s.st_mode = hint_mode_type;
			}
			MB2Wide(name, _tmp.wide_name);
			FillWFD((const wchar_t *)_tmp.wide_name.c_str(), s, lpFindFileData, symattr);
			
			const DWORD attrs = lpFindFileData->dwFileAttributes;
			if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				if ((_flags & FIND_FILE_FLAG_NO_DIRS) != 0 )
					return false;
			}
			if ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
				if ((_flags & FIND_FILE_FLAG_NO_LINKS) != 0 )
					return false;
			}
			if ((attrs & FILE_ATTRIBUTE_DEVICE) != 0) {
				if ((_flags & FIND_FILE_FLAG_NO_DEVICES) != 0 )
					return false;
			}
			if ((attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT))==0) {
				if ((_flags & FIND_FILE_FLAG_NO_FILES) != 0 ) {
					return false;
				}
			}
			
			return true;
		}

		bool MatchName(const char *name) 
		{
			if ((_flags & FIND_FILE_FLAG_NO_CUR_UP) != 0) {
				if (name[0] == '.') {
					if (name[1] == 0) {
						return false;
					}
					if (name[1] == '.' && name[2] == 0) {
						return false;					
					}
				}
			}
			if (_mask.empty())
				return true;
			
			if ((_flags & FIND_FILE_FLAG_CASE_INSENSITIVE) != 0)
				return MatchWildcardICE(name, _mask.c_str());

			return MatchWildcard(name, _mask.c_str());
		}
		
		DIR *_d = nullptr;
		
		bool PreMatchDType(unsigned char d_type)
		{
			switch (d_type) {
				case DT_DIR: return (_flags & FIND_FILE_FLAG_NO_DIRS) == 0;
				case DT_REG: return (_flags & FIND_FILE_FLAG_NO_FILES) == 0;
				case DT_LNK: return (_flags & FIND_FILE_FLAG_NO_LINKS) == 0;
				case DT_UNKNOWN: return true;
				
				default: return (_flags&FIND_FILE_FLAG_NO_DEVICES) == 0;
				
			}
		}

		struct {
			std::string path;
			std::wstring wide_name;			
		} _tmp;
		std::string _root;
		std::string _mask;
		DWORD _flags = 0;
	};

	struct UnixFindFiles : std::set<UnixFindFile *>, std::mutex
	{
	} g_unix_find_files;

	static volatile int g_unix_found_file_dummy = 0xfeedd00d;

	HANDLE WINPORT(FindFirstFileWithFlags)(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData, DWORD dwFlags)
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
		if (!mask.empty() && mask.find('*')==std::string::npos && mask.find('?')==std::string::npos &&
			(dwFlags & FIND_FILE_FLAG_CASE_INSENSITIVE) == 0 ) {

			struct stat s = { };
			DWORD symattr = 0;
			if (stat_symcheck((root + mask).c_str(), s, symattr) < 0 ) {
				// fprintf(stderr, "FindFirstFileWithFlags: stat_symcheck failed for '%s' / '%s'\n", root.c_str(), mask.c_str());
				return INVALID_HANDLE_VALUE;			
			}
			LPCWSTR name = wcsrchr(lpFileName, GOOD_SLASH);
			if (name) ++name; else name = lpFileName;
			FillWFD(name, s, lpFindFileData, symattr);
			return (HANDLE)&g_unix_found_file_dummy;
		}


		UnixFindFile *uff = new UnixFindFile(root, mask, dwFlags);
		if (!uff->IsOpened()) {
			ErrnoSaver es;
			delete uff;
			return INVALID_HANDLE_VALUE;
		}

		if (!uff->Iterate(lpFindFileData)) {
			//WINPORT(TranslateErrno)();
			delete uff;
			//fprintf(stderr, "find mask: %s (for %ls) FAILED\n", mask.c_str(), lpFileName);
			WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);
			return INVALID_HANDLE_VALUE;
		}
		//fprintf(stderr, "find mask: %s  (for %ls) - %ls", mask.c_str(), lpFileName, lpFindFileData->cFileName);
		std::lock_guard<std::mutex> lock(g_unix_find_files);
		g_unix_find_files.insert(uff);
		return (HANDLE)uff;
	}

	WINPORT_DECL(FindFirstFile, HANDLE, (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData))
	{
		return WINPORT(FindFirstFileWithFlags)(lpFileName, lpFindFileData, 0);
	}

	BOOL WINPORT(FindNextFile)(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
	{
		//return ::FindNextFile(hFindFile, lpFindFileData);
		if (hFindFile == (HANDLE)&g_unix_found_file_dummy)
			return FALSE;
		UnixFindFile *uff = (UnixFindFile *)hFindFile;
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
		if (hFindFile == (HANDLE)&g_unix_found_file_dummy)
			return TRUE;

		UnixFindFile *uff = (UnixFindFile *)hFindFile;
		{
			std::lock_guard<std::mutex> lock(g_unix_find_files);
			if (g_unix_find_files.erase(uff)==0)
				return FALSE;
		}

		delete uff;
		return TRUE;
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
			WINPORT(SetLastError)( ERROR_INVALID_NAME);
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
			UINT num = WINPORT(GetTickCount)() & 0xffff;
			static UINT last;

			/* avoid using the same name twice in a short interval */
			if (last - num < 10) num = last + 1;
			if (!num) num = 1;
			unique = num;
			std::string path_mb;
			do {
				swprintf( p, MAX_PATH - 1 - (p - buffer), formatW, unique );
				Wide2MB(buffer, path_mb);
				int fd = sdc_open(path_mb.c_str(), O_RDWR | O_CREAT | O_EXCL, 0640);
				if (fd != -1) {
					 /* We created it */
					sdc_close(fd);
					last = unique;
					break;
				}
				int err = errno;
				if (err != EEXIST && err != EBUSY && err != ETXTBSY)
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
