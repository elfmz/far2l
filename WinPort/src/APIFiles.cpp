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
		case 0: case S_IFREG: rv = FILE_ATTRIBUTE_ARCHIVE; break;
		case S_IFDIR: rv = FILE_ATTRIBUTE_DIRECTORY; break;
#ifndef _WIN32
		case S_IFLNK: rv = FILE_ATTRIBUTE_REPARSE_POINT; break;
		case S_IFSOCK: rv = FILE_ATTRIBUTE_DEVICE_SOCK; break;
#endif
		case S_IFCHR: rv = FILE_ATTRIBUTE_DEVICE_CHAR; break;
		case S_IFBLK: rv = FILE_ATTRIBUTE_DEVICE_BLOCK; break;
		case S_IFIFO: rv = FILE_ATTRIBUTE_DEVICE_FIFO; break;
		default: rv = FILE_ATTRIBUTE_DEVICE_CHAR | FILE_ATTRIBUTE_BROKEN;
	}

	if (*name == '.')
		rv|= FILE_ATTRIBUTE_HIDDEN;

	if ((unix_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)
		rv|= FILE_ATTRIBUTE_EXECUTABLE;

	if ((unix_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		rv|= FILE_ATTRIBUTE_READONLY;

	return rv;
}

template <class CHAR_T>
	static const CHAR_T *PointToNamePart(const CHAR_T *pathname)
{
	const CHAR_T *out = pathname;
	for (;*pathname; ++pathname) {
		if (*pathname == GOOD_SLASH) {
			out = pathname + 1;
		}
	}

	return out;
}

extern "C"
{
	struct WinPortHandleFile : MagicWinPortHandle<0> // <0> - for file handles
	{
		int fd;

		WinPortHandleFile(int fd_ = -1) : fd(fd_) {}

		virtual ~WinPortHandleFile()
		{
			if (fd != -1) {
				fprintf(stderr, "~WinPortHandleFile: unclosed fd %d\n", fd);
			}
		}

	protected:

		virtual bool Cleanup() noexcept
		{
			bool out = (fd == -1 || os_call_int(sdc_close, fd) == 0);
			if (!out) {
				ErrnoSaver es;
				fprintf(stderr, "WinPortHandleFile: error %u closing fd %d\n", es.Get(), fd);
			}

			fd = -1;
			return out;
		}

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
			//fprintf(stderr, "CreateFile: %ls - dwDesiredAccess=0x%x flags=0x%x mode=0%o path=%s errno=%d\n",
			//	lpFileName, dwDesiredAccess, flags, mode, path.c_str(), errno);

			return INVALID_HANDLE_VALUE;
		}

#ifndef __linux__
		if ((dwFlagsAndAttributes & (FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING)) != 0) {
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__HAIKU__)
			fcntl(r, O_DIRECT, 1);
#elif !defined(__CYGWIN__)
			fcntl(r, F_NOCACHE, 1);
#endif // __FreeBSD__
		}
#endif // __linux__

		if ((dwFlagsAndAttributes & (FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN)) == FILE_FLAG_SEQUENTIAL_SCAN && !want_write) {
			HintFDSequentialAccess(r);
		}

		/*nobody cares.. if ((dwFlagsAndAttributes&FILE_FLAG_BACKUP_SEMANTICS)==0) {
			struct stat s = { };
			sdc_fstat(r, &s);
			if ( (s.st_mode & S_IFMT) == FILE_ATTRIBUTE_DIRECTORY) {
				sdc_close(r);
				WINPORT(SetLastError)(ERROR_DIRECTORY);
				return INVALID_HANDLE_VALUE;
			}
		}*/

		auto *wph = new(std::nothrow) WinPortHandleFile(r);
		if (!wph) {
			sdc_close(r);
			return INVALID_HANDLE_VALUE;
		}

		return wph->Register();
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

	DWORD WINPORT(GetCurrentDirectory)( DWORD nBufferLength, LPWSTR lpBuffer)
	{
		std::vector<char> buf(nBufferLength + 1);
		if (UNLIKELY(!_getcwd(buf.data(), nBufferLength))) {
			return (nBufferLength < PATH_MAX) ? PATH_MAX : 0;
		}

		const std::wstring &wstr = MB2Wide(buf.data());
		if (UNLIKELY(wstr.size() + 1 > nBufferLength)) {
			return DWORD(wstr.size() + 1);
		}

		wmemcpy(lpBuffer, wstr.c_str(), wstr.size() + 1);
		return (DWORD)wstr.size();
	}

	BOOL WINPORT(SetCurrentDirectory)(LPCWSTR lpPathName)
	{
		const std::string &path = ConsumeWinPath(lpPathName);
		auto r = os_call_int(sdc_chdir, path.c_str());
		return UNLIKELY(r == -1) ? FALSE : TRUE;
	}

	BOOL WINPORT(GetFileSizeEx)( HANDLE hFile, PLARGE_INTEGER lpFileSize)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (UNLIKELY(!wph)) {
			return FALSE;
		}
#ifdef _WIN32
		__int64 len = _filelength(wph->fd);
		if (len==(__int64)-1)
			return FALSE;
		lpFileSize->QuadPart = len;
#else
		struct stat s{};
		if (os_call_int(sdc_fstat, wph->fd, &s) == -1)
			return FALSE;
		lpFileSize->QuadPart = s.st_size;
#endif
		return TRUE;
	}

	DWORD64 WINPORT(GetFileSize64)( HANDLE hFile)
	{
		LARGE_INTEGER sz64{};
		if (!WINPORT(GetFileSizeEx)(hFile, &sz64)) {
			return INVALID_FILE_SIZE64;
		}

		return sz64.QuadPart;
	}

	DWORD WINPORT(GetFileSize)( HANDLE hFile, LPDWORD lpFileSizeHigh)
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
		if (UNLIKELY(!wph)) {
			return FALSE;
		}
		if (UNLIKELY(lpOverlapped)) {
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
		if (UNLIKELY(!wph)) {
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
		if (UNLIKELY(!wph)) {
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
		if (UNLIKELY(r==(off_t)-1))
			return FALSE;
		if (lpNewFilePointer) lpNewFilePointer->QuadPart = r;
		return TRUE;
	}

	VOID WINPORT(FileAllocationHint) (HANDLE hFile, DWORD64 HintFileSize)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (UNLIKELY(!wph)) {
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

	BOOL WINPORT(FileAllocationRequire) (HANDLE hFile, DWORD64 RequireFileSize)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (UNLIKELY(!wph)) {
			return FALSE;
		}

		struct stat s{};
		if (fstat(wph->fd, &s) == -1)
			return FALSE;

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__HAIKU__)
		int ret = posix_fallocate(wph->fd, 0, (off_t)RequireFileSize);
		if (ret == 0)
			return TRUE;

#elif defined(F_PREALLOCATE)
		if ((off_t)RequireFileSize <= s.st_size)
			return TRUE;

		fstore_t fst {};
		fst.fst_flags = F_ALLOCATECONTIG;
		fst.fst_posmode = F_PEOFPOSMODE;
		fst.fst_offset = 0;
		fst.fst_length = (off_t)((off_t)RequireFileSize - s.st_size);
		fst.fst_bytesalloc = 0;

		int ret = fcntl(wph->fd, F_PREALLOCATE, &fst);
		if (ret == -1) {
			fst.fst_flags = F_ALLOCATEALL;
			fst.fst_bytesalloc = 0;
			ret = fcntl(wph->fd, F_PREALLOCATE, &fst);
		}

		if (ret != -1 && ftruncate(wph->fd, (off_t)RequireFileSize) == 0) {
			return TRUE;
		}
#endif

		// posix_fallocate/F_PREALLOCATE unsupported or failed, try to fallback
		// to file expansion by writing zero bytes to ensure space allocation
		char dummy[0x10000]{};
		for (off_t ofs = s.st_size; ofs < (off_t)RequireFileSize; ) {
			const size_t piece = (size_t)std::min((off_t)sizeof(dummy), (off_t)RequireFileSize - ofs);
			const ssize_t r = pwrite(wph->fd, dummy, piece, ofs);
			if (r == 0 || (r < 0 && errno != EAGAIN && errno != EINTR)) {
				int err = errno;
				if (ftruncate(wph->fd, s.st_size) == -1) { // revert original size
					perror("FileAllocationRequire: ftruncate to original length");
				}
				errno = err;
				return FALSE;
			}
			if (r > 0) {
				ofs+= (size_t)r;
			}
		}

		return TRUE;
	}

	DWORD WINPORT(SetFilePointer)( HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
	{
		LARGE_INTEGER liDistanceToMove, liNewFilePointer = {};
		liDistanceToMove.LowPart = lDistanceToMove;
		if (lpDistanceToMoveHigh) {
			liDistanceToMove.HighPart = *lpDistanceToMoveHigh;
		} else {
			liDistanceToMove.HighPart = (lDistanceToMove < 0) ? -1 : 0;
		}
		auto r = WINPORT(SetFilePointerEx)( hFile, liDistanceToMove, &liNewFilePointer, dwMoveMethod);
		if (UNLIKELY(!r))
			return INVALID_SET_FILE_POINTER;

		if (lpDistanceToMoveHigh)
			*lpDistanceToMoveHigh = liNewFilePointer.HighPart;

		return liNewFilePointer.LowPart;
	}

	BOOL WINPORT(GetFileTime)( HANDLE hFile, LPFILETIME lpCreationTime,
		LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (UNLIKELY(!wph)) {
			return FALSE;
		}
		struct stat s{};
		auto r = os_call_int(sdc_fstat, wph->fd, &s);
		if (UNLIKELY(r < 0))
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
		if (UNLIKELY(!wph)) {
			return FALSE;
		}

		struct timespec ts[2] = {};
		if (lpLastAccessTime) {
			WINPORT(FileTime_Win32ToUnix)(lpLastAccessTime, &ts[0]);
		}
		if (lpLastWriteTime) {
			WINPORT(FileTime_Win32ToUnix)(lpLastWriteTime, &ts[1]);
		}

		auto r = os_call_int(sdc_futimens, wph->fd, (const struct timespec *)ts);
		if (UNLIKELY(r < 0))
			return FALSE;

		return TRUE;
	}


	DWORD WINPORT(EvaluateAttributes)(uint32_t unix_mode, const WCHAR *pathname)
	{
		return EvaluateAttributesT(unix_mode, PointToNamePart(pathname));
	}

	DWORD WINPORT(EvaluateAttributesA)(uint32_t unix_mode, const char *pathname)
	{
		return EvaluateAttributesT(unix_mode, PointToNamePart(pathname));
	}

	class Statocaster
	{
		struct stat _st_dst;
		struct stat _st_lnk;
		DWORD _attr;

		inline const struct stat &DereferencedStat() const
		{
			return ((_attr & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_BROKEN)) == FILE_ATTRIBUTE_REPARSE_POINT)
				? _st_dst : _st_lnk;
		}

	public:
		Statocaster(const char *pathname, const char *name = nullptr)
		{
			if (os_call_int(sdc_lstat, pathname, &_st_lnk) < 0) {
				_attr = INVALID_FILE_ATTRIBUTES;
				return;
			}

			if (!name) {
				name = PointToNamePart(pathname);
			}

			if ((_st_lnk.st_mode & S_IFMT) != S_IFLNK) {
				_attr = 0;

			} else if (os_call_int(sdc_stat, pathname, &_st_dst) < 0) {
				_attr = FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_BROKEN;
				_st_lnk.st_size = 0;

			} else {
				_attr = FILE_ATTRIBUTE_REPARSE_POINT;
			}

#if defined(__APPLE__) || defined(__FreeBSD__)  || defined(__DragonFly__)
			if (DereferencedStat().st_flags & UF_HIDDEN) { // chflags hidden FILENAME
				_attr|= FILE_ATTRIBUTE_HIDDEN;
			}
#endif
			_attr|= EvaluateAttributesT(DereferencedStat().st_mode, name);
		}

		inline DWORD Attributes() const
		{
			return _attr;
		}

		bool FillWFD(WIN32_FIND_DATAW *wfd) const
		{
			if (_attr == INVALID_FILE_ATTRIBUTES) {
				return false;
			}

			const auto &s = DereferencedStat();

			WINPORT(FileTime_UnixToWin32)(s.st_ctim, &wfd->ftCreationTime);
			WINPORT(FileTime_UnixToWin32)(s.st_atim, &wfd->ftLastAccessTime);
			WINPORT(FileTime_UnixToWin32)(s.st_mtim, &wfd->ftLastWriteTime);
			wfd->UnixOwner = s.st_uid;
			wfd->UnixGroup = s.st_gid;
			wfd->UnixDevice = s.st_dev;
			wfd->UnixNode = s.st_ino;
			wfd->nPhysicalSize = ((DWORD64)_st_lnk.st_blocks) * 512;
			wfd->nFileSize = (DWORD64)s.st_size;
			wfd->dwFileAttributes = _attr;
			wfd->dwUnixMode = s.st_mode;
			wfd->nHardLinks = (DWORD)s.st_nlink;
			wfd->nBlockSize = (DWORD)s.st_blksize;
			return true;
		}
	};

	DWORD WINPORT(GetFileAttributes)(LPCWSTR lpFileName)
	{
		return Statocaster(ConsumeWinPath(lpFileName).c_str()).Attributes();
	}

	DWORD WINPORT(SetFileAttributes)(LPCWSTR lpFileName, DWORD dwAttributes)
	{
		fprintf(stderr, "NOT SUPPORTED: SetFileAttributes('%ls', 0x%x)\n", lpFileName, dwAttributes);
		return TRUE;
	}

	BOOL WINPORT(SetEndOfFile)( HANDLE hFile)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (UNLIKELY(!wph)) {
			return FALSE;
		}

		off_t pos = os_call_v<off_t, -1>(sdc_lseek, wph->fd, (off_t)0, SEEK_CUR);
		if (UNLIKELY(pos==(off_t)-1))
			return FALSE;
		auto r = os_call_int(sdc_ftruncate, wph->fd, pos);
		if (UNLIKELY(r == -1))
			return FALSE;

		return TRUE;
	}

	BOOL WINPORT(FlushFileBuffers)( HANDLE hFile)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (UNLIKELY(!wph)) {
			return FALSE;
		}
		//fsync(wph->fd);
		return TRUE;
	}

	DWORD WINPORT(GetFileType)( HANDLE hFile)
	{
		AutoWinPortHandle<WinPortHandleFile> wph(hFile);
		if (UNLIKELY(!wph)) {
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
		if (UNLIKELY(!wph)) {
			return -1;
		}

		return wph->fd;
	}

	struct UnixFindFile
	{
		UnixFindFile(const std::string &root, const std::string &mask, DWORD flags) : _flags(flags)
		{
			_root = root;
			_mask = mask;
			if (_root.size() > 1 && _root.back() == GOOD_SLASH)
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

#ifndef __HAIKU__
				if (PreMatchDType(de->d_type) && MatchName(de->d_name) ) {
					mode_t hint_mode_type = 0;
					switch (de-> d_type) {
						case DT_DIR: hint_mode_type = S_IFDIR; break;
						case DT_REG: hint_mode_type = S_IFREG; break;
						case DT_LNK: hint_mode_type = S_IFLNK; break;
						case DT_BLK: hint_mode_type = S_IFBLK; break;
						case DT_FIFO: hint_mode_type = S_IFIFO; break;
						case DT_CHR: hint_mode_type = S_IFCHR; break;
						case DT_SOCK: hint_mode_type = S_IFSOCK; break;
						default: hint_mode_type = 0;
					}
					if (MatchAttributesAndFillWFD(de->d_name, lpFindFileData, hint_mode_type))
#else
				if (MatchName(de->d_name) ) {
					if (MatchAttributesAndFillWFD(de->d_name, lpFindFileData))
#endif
						return true;
				}
			}
		}

	private:
		void ZeroFillWFD(LPWIN32_FIND_DATAW wfd)
		{
			memset(&wfd->ftLastWriteTime, 0, sizeof(wfd->ftLastWriteTime));
			memset(&wfd->ftCreationTime, 0, sizeof(wfd->ftCreationTime));
			memset(&wfd->ftLastAccessTime, 0, sizeof(wfd->ftLastAccessTime));
			wfd->UnixOwner = 0;
			wfd->UnixGroup = 0;
			wfd->UnixDevice = 0;
			wfd->UnixNode = 0;
			wfd->nPhysicalSize = 0;
			wfd->nFileSize = 0;
			wfd->dwFileAttributes = 0;
			wfd->dwUnixMode = 0;
			wfd->nHardLinks = 0;
			wfd->nBlockSize = 0;
			wfd->cFileName[0] = 0;
		}

		bool MatchAttributesAndFillWFD(const char *name, LPWIN32_FIND_DATAW wfd, mode_t hint_mode_type = 0)
		{
			_tmp.path = _root;
			if (_tmp.path.empty() || _tmp.path.back() != GOOD_SLASH)
				_tmp.path+= GOOD_SLASH;
			_tmp.path+= name;

			SudoSilentQueryRegion ssqr(hint_mode_type !=0 && (_flags & FIND_FILE_FLAG_NOT_ANNOYING) != 0);
			if (!Statocaster(_tmp.path.c_str(), name).FillWFD(wfd)) {
				fprintf(stderr, "UnixFindFile: errno=%u hmt=0%o on '%s'\n",
					errno, hint_mode_type, _tmp.path.c_str());
				ZeroFillWFD(wfd);
				wfd->dwFileAttributes = FILE_ATTRIBUTE_BROKEN | EvaluateAttributesT(hint_mode_type, name);
			}

			MB2Wide(name, _tmp.wide_name);
			wcsncpy(wfd->cFileName, _tmp.wide_name.c_str(), MAX_NAME);

			const DWORD attrs = wfd->dwFileAttributes;
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
			if ((attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT)) == 0) {
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

#ifndef __HAIKU__
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
#endif

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
		if (!lpFileName || !*lpFileName)
			return INVALID_HANDLE_VALUE;

		//return ::FindFirstFile(lpFileName, lpFindFileData);
		std::string root(ConsumeWinPath(lpFileName)), mask;
		size_t p = root.rfind(GOOD_SLASH);
		if (p != std::string::npos) {
			mask.assign(root.c_str() + p + 1);
			root.resize(p + 1);
		} else {
			fprintf(stderr, "FindFirstFile: no slash in root='%s' for lpFileName='%ls'\n", root.c_str(), lpFileName);
			mask.swap(root);
			root = "./";
		}

		if (mask == "*" || mask == "*.*") {
			mask.clear();
		}

		if (!mask.empty() && mask.find_first_of("*?") == std::string::npos
			&& (dwFlags & FIND_FILE_FLAG_CASE_INSENSITIVE) == 0 )
		{

			if (!Statocaster((root + mask).c_str(), mask.c_str()).FillWFD(lpFindFileData)) {
				return INVALID_HANDLE_VALUE;
			}

			LPCWSTR last_slash = wcsrchr(lpFileName, GOOD_SLASH);
			wcsncpy(lpFindFileData->cFileName, last_slash ? last_slash + 1 : lpFileName, MAX_NAME);
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
		//fprintf(stderr, "find mask: %s (for %ls) - %ls", mask.c_str(), lpFileName, lpFindFileData->cFileName);
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

		/* add a \, if there isn't one */
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
					break; /* No need to go on */
				if (!(++unique & 0xffff)) unique = 1;
			} while (unique != num);
		}

		return unique;
	}

	WINPORT_DECL(GetFullPathName, DWORD,
		(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart))
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
