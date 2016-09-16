#include "utils.h"
#include <WinCompat.h>
#include <WinPort.h>
#include <sys/stat.h>

void Wide2MB(const wchar_t *src, std::string &dst)
{
	size_t src_len = wcslen(src);
	if (!src_len) {
		dst.clear();
		return;
	}

	WINPORT(LastErrorGuard) leg;
	int r = WINPORT(WideCharToMultiByte)(CP_UTF8, 0, src, src_len, NULL, 0, NULL, NULL);
	dst.resize( ( (r > 0) ? r : src_len) + 1);
	
	for (;; ) {
		r = WINPORT(WideCharToMultiByte)(CP_UTF8, 0, src, src_len, &dst[0], dst.size(), NULL, NULL);
		if (r < 0) {
			dst.clear();
			break;
		}
		if (r==0) {
			if (WINPORT(GetLastError)()==ERROR_INSUFFICIENT_BUFFER ) {
				dst.resize(dst.size() + 8 + dst.size()/2);
			} else {
				fprintf(stderr, "Wide2MB('" WS_FMT "') - failed\n", src);
				dst.clear();
				break;
			}
		}
		else if ((size_t)r<=dst.size()) {
			dst.resize(r);
			break;
		}
	}
}

void MB2Wide(const char *src, std::wstring &dst)
{
	size_t src_len = strlen(src);
	if (!src_len) {
		dst.clear();
		return;
	}

	WINPORT(LastErrorGuard) leg;
	int r = WINPORT(MultiByteToWideChar)(CP_UTF8, 0, src, src_len, NULL, 0);	
	dst.resize( ( (r > 0) ? r : src_len) + 1);
	
	for (;; ) {
		r = WINPORT(MultiByteToWideChar)(CP_UTF8, 0, src, src_len, &dst[0], dst.size());
		if (r < 0) {
			dst.clear();			
			break;
		}
		if (r==0) {
			if (WINPORT(GetLastError)()==ERROR_INSUFFICIENT_BUFFER) {
				dst.resize(dst.size() + 8 + dst.size()/2);
			} else {
				fprintf(stderr, "MB2Wide('%s') - failed\n", src);
				dst.clear();
				break;
			}
		} else if ((size_t)r<=dst.size()) {
			dst.resize(r);
			break;
		}
	}
}

/////////////////////

std::string Wide2MB(const wchar_t *src)
{
	std::string dst;
	Wide2MB(src, dst);
	return dst;
}

std::wstring MB2Wide(const char *src)
{
	std::wstring dst;
	MB2Wide(src, dst);
	return dst;
}


std::string StrWide2MB(const std::wstring &src) 
{
	return Wide2MB(src.c_str());
}
std::wstring StrMB2Wide(const std::string &src) 
{
	return MB2Wide(src.c_str());
}

////////////////////////////

unsigned char Hex2Digit(const char hex)
{
	if (hex>='0' && hex<='9')
		return hex - '0';
	if (hex>='a' && hex<='f')
		return 10 + hex - 'a';
	if (hex>='A' && hex<='F')
		return 10 + hex - 'A';

	return 0;
}

unsigned char Hex2Byte(const char *hex)
{
	unsigned char r = Hex2Digit(hex[0]);
	r<<=4;
	r+= Hex2Digit(hex[1]);
	return r;
}

std::string InMyProfile(const char *subpath)
{
#ifdef _WIN32
	std::string path = "D:\\.far2l";
#else	
	const char *home = getenv("HOME");
	std::string path = home ? home : "/tmp";
	path+= "/.far2l";
#endif
	
	mkdir(path.c_str(), 0777);
	if (subpath) {
		if (*subpath != GOOD_SLASH) 
			path+= GOOD_SLASH;
		for (const char *p = subpath; *p; ++p) {
			if (*p == GOOD_SLASH)
				mkdir(path.c_str(), 0777);
			path+= *p;
		}
	}
	
	return path;
	
}
