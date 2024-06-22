#include "utils.h"
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <os_call.hpp>

#include <algorithm>

#if defined(__FreeBSD__) || defined(__DragonFly__)
# include <malloc_np.h>
#elif __APPLE__
# include <malloc/malloc.h>
#else
# include <malloc.h>
#endif


////////////////////////////
void CheckedCloseFD(int &fd)
{
	int tmp = fd;
	if (tmp != -1) {
		fd = -1;
		if (os_call_int(close, tmp) != 0) {
			const int err = errno;
			fprintf(stderr, "%s: %d\n", __FUNCTION__, err);
			ASSERT(err != EBADF);
		}
	}
}

void CheckedCloseFDPair(int *fd)
{
	CheckedCloseFD(fd[0]);
	CheckedCloseFD(fd[1]);
}

size_t WriteAll(int fd, const void *data, size_t len, size_t chunk)
{
	for (size_t ofs = 0; ofs < len; ) {
		if (chunk == (size_t)-1 || chunk >= len) {
			chunk = len;
		}
		ssize_t written = write(fd, (const char *)data + ofs, chunk);
		if (written <= 0) {
			if (errno != EAGAIN && errno != EINTR) {
				return ofs;
			}
		} else {
			ofs+= std::min((size_t)written, chunk);
		}
	}
	return len;
}

size_t ReadAll(int fd, void *data, size_t len)
{
	for (size_t ofs = 0; ofs < len; ) {
		ssize_t readed = read(fd, (char *)data + ofs, len - ofs);
		if (readed <= 0) {
			if (readed == 0 || (errno != EAGAIN && errno != EINTR)) {
				return ofs;
			}

		} else {
			ofs+= (size_t)readed;
		}
	}
	return len;
}

ssize_t ReadWritePiece(int fd_src, int fd_dst)
{
	char buf[32768];
	for (;;) {
		ssize_t r = read(fd_src, buf, sizeof(buf));
		if (r < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				continue;
			}

			return -1;
		}

		if (r > 0) {
			if (WriteAll(fd_dst, buf, (size_t)r) != (size_t)r) {
				return -1;
			}
		}

		return r;
	}
}

//////////////

int pipe_cloexec(int pipedes[2])
{
#if defined(__APPLE__) || defined(__CYGWIN__) || defined(__HAIKU__)
	int r = os_call_int(pipe, pipedes);
	if (r==0) {
		MakeFDCloexec(pipedes[0]);
		MakeFDCloexec(pipedes[1]);
	}
	return r;
#else
	return os_call_int(pipe2, pipedes, O_CLOEXEC);
#endif	
}

bool IsPathIn(const wchar_t *path, const wchar_t *root)
{	
	const size_t path_len = wcslen(path);
	size_t root_len = wcslen(root);

	while (root_len > 1 && root[root_len - 1] == GOOD_SLASH)
		--root_len;

	if (path_len < root_len)
		return false;

	if (memcmp(path, root, root_len * sizeof(wchar_t)) != 0)
		return false;
	
	if (root_len > 1 && path[root_len] && path[root_len] != GOOD_SLASH)
		return false;

	return true;
}

void AbbreviateString(std::string &path, size_t needed_length)
{
	size_t len = path.size();
	if (needed_length < 1) {
		needed_length = 1;
	}
	if (len > needed_length) {
		size_t delta = len - (needed_length - 1);
		path.replace((path.size() - delta) / 2, delta, "…");//"...");
	}
}

const wchar_t *FileSizeToFractionAndUnits(unsigned long long &value)
{
	if (value > 100ll * 1024ll * 1024ll * 1024ll * 1024ll) {
		value = (1024ll * 1024ll * 1024ll * 1024ll);
		return L"TB";
	}

	if (value > 100ll * 1024ll * 1024ll * 1024ll) {
		value = (1024ll * 1024ll * 1024ll);
		return L"GB";
	}

	if (value > 100ll * 1024ll * 1024ll ) {
		value = (1024ll * 1024ll);
		return L"MB";

	}

	if (value > 100ll * 1024ll ) {
		value = (1024ll);
		return L"KB";
	}

	value = 1;
	return L"B";
}

std::wstring ThousandSeparatedString(unsigned long long value)
{
	std::wstring str;
	for (size_t th_sep = 0; value != 0;) {
		wchar_t digit = L'0' + (value % 10);
		value/= 10;
		if (th_sep == 3) {
			str+= L'`';
			th_sep = 0;
		} else {
			++th_sep;
		}
		str+= digit;
	}

	if (str.empty()) {
		str = L"0";
	} else {
		std::reverse(str.begin(), str.end());
	}
	return str;
}

std::wstring FileSizeString(unsigned long long value)
{
	unsigned long long fraction = value;
	const wchar_t *units = FileSizeToFractionAndUnits(fraction);
	value/= fraction;

	std::wstring str = ThousandSeparatedString(value);
	str+= L' ';
	str+= units;
	return str;
}

static inline bool CaseIgnoreEngChrMatch(const char c1, const char c2)
{
	if (c1 != c2) {
		if (c1 >= 'A' && c1 <= 'Z') { 
			if (c1 + ('a' - 'A') != c2) {
				return false;
			}

		} else if (c1 >= 'a' && c1 <= 'z') {
			if (c1 - ('a' - 'A') != c2) {
				return false;
			}

		} else {
			return false;
		}
	}

	return true;
}

bool CaseIgnoreEngStrMatch(const std::string &str1, const std::string &str2)
{
	return str1.size() == str2.size() && CaseIgnoreEngStrMatch(str1.c_str(), str2.c_str(), str1.size());
}

bool CaseIgnoreEngStrMatch(const char *str1, const char *str2, size_t len)
{
	for (size_t i = 0; i != len; ++i) {
		if (!CaseIgnoreEngChrMatch(str1[i], str2[i])) {
			return false;
		}
	}

	return true;
}

const char *CaseIgnoreEngStrChr(const char c, const char *str, size_t len)
{
	for (size_t i = 0; i != len; ++i) {
		if (CaseIgnoreEngChrMatch(c, str[i])) {
			return &str[i];
		}
	}

	return nullptr;
}
