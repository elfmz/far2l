#include "utils.h"
#include <WinCompat.h>
#include <WinPort.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include "ConvertUTF.h"
#include <errno.h>
#include <os_call.hpp>

#include <algorithm>

#if __FreeBSD__
# include <malloc_np.h>
#elif __APPLE__
# include <malloc/malloc.h>
#else
# include <malloc.h>
#endif


//TODO: Implement convertion according to locale set, but not only UTF8
//NB: Routines here should not change or preserve result of WINPORT(GetLastError)

namespace FailTolerantUTF8
{
	//This functionality intendent to convert between UTF8 and wide chars in fail-toleran manner,
	//that means that incorrectly encoded UTF8 will be translated to corresponding wide representation
	//with escaping of incorrect sequences. That mean if source UTF8 contains escaping char - it also 
	//will be escaped, even if source is completely valid UTF8.
	//Corresponding reverse translation from wide to UTF8 will revert escaping.
	//However fail tolerance works only in one direction - from UTF8 to UTF32. Broken UTF32 processed as 
	//usually - with lost of corrupted sequences.
	
	#define ESCAPE_CHAR 0x1a
	
	template <class WIDE_UTF>
		static void FromWide(const wchar_t *src, size_t src_len, std::string &dst,
			ConversionResult (*pCalcSpace)(int *, const WIDE_UTF**, const WIDE_UTF*, ConversionFlags),
			ConversionResult (*pConvert)(const WIDE_UTF**, const WIDE_UTF*, UTF8**, UTF8*, ConversionFlags)	)
	{
		static_assert(sizeof(WIDE_UTF) == sizeof(wchar_t), "FromWide: bad WIDE_UTF");
		if (!src_len) {
			dst.clear();
			return;
		}
		
		const WIDE_UTF *src_end = (const WIDE_UTF*)(src + src_len);
		
		int len = 0;
		const WIDE_UTF *tmp = (const WIDE_UTF *)src;
		pCalcSpace (&len, &tmp, src_end, lenientConversion);
		dst.resize(len);
		if (!len)
			return;

		tmp = (const WIDE_UTF *)src;
		UTF8 *target = (UTF8 *)&dst[0];
		UTF8 *target_end = target + len;
		pConvert(&tmp, src_end, &target, target_end, lenientConversion);
		assert(target==target_end);
		
		for (size_t p = dst.find(ESCAPE_CHAR); p!=std::string::npos; p = dst.find(ESCAPE_CHAR, p)) {
			if ( p + 2 >= dst.size()) break;
			char hex[] = {dst[p + 1], dst[p + 2], 0};
			unsigned char c = Hex2Byte(hex);
			dst.replace(p, 3, 1, (char)c);
			p++;
		}
	}

	template <class WIDE_UTF, bool HONOR_INCOMPLETE = false>
		static size_t ToWide(const char *src, size_t src_len, std::wstring &dst,
			ConversionResult (*pCalcSpace)(int *, const UTF8**, const UTF8*, ConversionFlags),
			ConversionResult (*pConvert)(const UTF8**, const UTF8*, WIDE_UTF**, WIDE_UTF*, ConversionFlags) )
	{
		static_assert(sizeof(WIDE_UTF) == sizeof(wchar_t), "ToWide: bad WIDE_UTF");
		if (!src_len) {
			dst.clear();
			return 0;
		}
		
		const UTF8 *src_begin = (const UTF8*)src;
		const UTF8 *src_end = (const UTF8*)(src + src_len);
		size_t dst_len = 0;
		wchar_t wz[16];
		do {
			const UTF8* tmp = (const UTF8*)src;
			int len = 0;
			pCalcSpace (&len, &tmp, src_end, strictConversion);
			if (tmp!=(const UTF8*)src) {
				assert (len > 0);
				dst.resize(dst_len + len);
				WIDE_UTF *target = (WIDE_UTF *)&dst[dst_len];
				WIDE_UTF *target_end = target + len;
				ConversionResult cr = pConvert ((const UTF8**)&src, 
									tmp, &target, target_end, strictConversion);
				assert (cr == conversionOK);
				assert (tmp == (const UTF8*)src);
				assert (target == target_end);
				for (size_t pos = dst.find(ESCAPE_CHAR, dst_len); 
						pos != std::wstring::npos; pos = dst.find(ESCAPE_CHAR, pos)) {
					swprintf(wz, ARRAYSIZE(wz), L"%02x", ESCAPE_CHAR);
					dst.insert(pos + 1, wz);
					pos+= 3;
					len++;
				}
				dst_len+= len;
			} else if (!HONOR_INCOMPLETE || ((const char*)src_end - src) >= 6) {
				swprintf(wz, ARRAYSIZE(wz), L"%c%02x", ESCAPE_CHAR, *(unsigned char *)src);
				dst.resize(dst_len);
				dst+= wz;
				dst_len = dst.size();
				src++;
			} else {
				break;
			}
		} while (src!=(const char *)src_end);

		return src - (const char*)src_begin;
	}
}

static void Wide2MB(const wchar_t *src, size_t src_len, std::string &dst)
{
#if (__WCHAR_MAX__ > 0xffff)
	FailTolerantUTF8::FromWide<UTF32>(src, src_len, dst, CalcSpaceUTF32toUTF8, ConvertUTF32toUTF8);
#else
	FailTolerantUTF8::FromWide<UTF16>(src, src_len, dst, CalcSpaceUTF16toUTF8, ConvertUTF16toUTF8);
#endif	
}

void MB2Wide(const char *src, size_t src_len, std::wstring &dst)
{
#if (__WCHAR_MAX__ > 0xffff)
	FailTolerantUTF8::ToWide<UTF32>(src, src_len, dst, CalcSpaceUTF8toUTF32, ConvertUTF8toUTF32);
#else
	FailTolerantUTF8::ToWide<UTF16>(src, src_len, dst, CalcSpaceUTF8toUTF16, ConvertUTF8toUTF16);
#endif	
}


size_t MB2Wide_HonorIncomplete(const char *src, size_t src_len, std::wstring &dst)
{
#if (__WCHAR_MAX__ > 0xffff)
	return FailTolerantUTF8::ToWide<UTF32, true>(src, src_len, dst, CalcSpaceUTF8toUTF32, ConvertUTF8toUTF32);
#else
	return FailTolerantUTF8::ToWide<UTF16, true>(src, src_len, dst, CalcSpaceUTF8toUTF16, ConvertUTF8toUTF16);
#endif	
}

//////////////////

void Wide2MB(const wchar_t *src, std::string &dst)
{
	Wide2MB(src, wcslen(src), dst);
}

void MB2Wide(const char *src, std::wstring &dst)
{
	MB2Wide(src, strlen(src), dst);
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


void StrWide2MB(const std::wstring &src, std::string &dst) 
{
	Wide2MB(src.c_str(), src.size(), dst);
}
std::string StrWide2MB(const std::wstring &src) 
{
	std::string dst;
	Wide2MB(src.c_str(), src.size(), dst);
	return dst;
}

void StrMB2Wide(const std::string &src, std::wstring &dst) 
{
	MB2Wide(src.c_str(), src.size(), dst);
}
std::wstring StrMB2Wide(const std::string &src) 
{
	std::wstring dst;
	MB2Wide(src.c_str(), src.size(), dst);
	return dst;
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

size_t StrStartsFrom(const std::string &haystack, const char *needle)
{
	size_t l = strlen(needle);
	if (!l || haystack.size() < l)
		return 0;
		
	return memcmp(haystack.c_str(), needle, l) ? 0 : l;
}

template <class STRING_T>
	static STRING_T EscapeQuotasT(STRING_T str)
{
	for(size_t p = str.find('\"'); p!=std::string::npos; p = str.find('\"', p)) {
		str.insert(p, 1, '\\');
		p+= 2;
	}
	return str;
}

std::string EscapeQuotas(const std::string &str) {return EscapeQuotasT(str); }
std::wstring EscapeQuotas(const std::wstring &str) {return EscapeQuotasT(str); }


std::string EscapeEscapes(std::string str)
{
	for (size_t p = 0; (p + 1) < str.size(); ) {
		if (str[p] == '\\' && (str[p + 1] == '\"' || str[p + 1] == '\\' || str[ p + 1] == '\t') ) {
			str.insert(p, 2, '\\');
			p+= 4;
		} else
			++p;
	}
	return str;
}

template <class STRING_T>
	static void QuoteCmdArgT(STRING_T &str)
{
	STRING_T tmp(1, '\"');
	tmp+= EscapeQuotas(str);
	tmp+= '\"';
	str.swap(tmp);
}

void QuoteCmdArg(std::string &str) { QuoteCmdArgT(str); }
void QuoteCmdArg(std::wstring &str) { QuoteCmdArgT(str); }

void QuoteCmdArgIfNeed(std::string &str)
{
	if (str.find_first_of(" \"\'\r\n\t&|;,()") != std::string::npos) {
		QuoteCmdArg(str);
	}
}

void QuoteCmdArgIfNeed(std::wstring &str)
{
	if (str.find_first_of(L" \"\'\r\n\t&|;,()") != std::wstring::npos) {
		QuoteCmdArg(str);
	}
}
	
////////////////////////////////////////////////////////////////

void CheckedCloseFD(int &fd)
{
	int tmp = fd;
	if (tmp != -1) {
               fd = -1;
               if (os_call_int(close, tmp) != 0) {
                       perror("CheckedCloseFD");
                       abort();
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

ErrnoSaver::ErrnoSaver() : v(errno) 
{
}

ErrnoSaver::~ErrnoSaver() 
{ 
	errno = v;
}


//////////
int pipe_cloexec(int pipedes[2])
{
#if defined(__APPLE__) || defined(__CYGWIN__)
	int r = os_call_int(pipe, pipedes);
	if (r==0) {
		fcntl(pipedes[0], F_SETFD, FD_CLOEXEC);
		fcntl(pipedes[1], F_SETFD, FD_CLOEXEC);
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

bool isCombinedUTF32(wchar_t c)
{
	return c >= 0x0300 && c <= 0x036F;
}

size_t GetMallocSize(void *p)
{
#ifdef _WIN32
	return _msize(p);
#elif defined(__APPLE__)
	return malloc_size(p);
#else
	return malloc_usable_size(p);
#endif
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



#ifdef __CYGWIN__
extern "C"
{
char * itoa(int i, char *a, int radix)
{
	switch (radix) {
		case 10: sprintf(a, "%d", i); break;
		case 16: sprintf(a, "%x", i); break;
	}
	return a;
}
}
#endif


unsigned long htoul(const char *str)
{
	unsigned long out = 0;
	for (;;++str) {
		if (*str >= '0' && *str <= '9') {
			out<<= 4;
			out+= *str - '0';

		} else if (*str >= 'a' && *str <= 'f') {
			out<<= 4;
			out+= 10 + (*str - 'a');

		} else if (*str >= 'A' && *str <= 'F') {
			out<<= 4;
			out+= 10 + (*str - 'A');

		} else
			return out;
	}
}
