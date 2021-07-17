#include "utils.h"
#include <WinCompat.h>
#include <WinPort.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "ConvertUTF.h"

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
			unsigned char c = ParseHexByte(hex);
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
