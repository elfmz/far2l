#pragma once
#include <string>
#include "UtfDefines.h"

/************************************************************************************************
 This functionality intendent to convert between UTF8 and wide chars in fault-tolerant manner,
 that means that incorrectly encoded UTF8 will be translated to corresponding wide representation
 with escaping of incorrect sequences. That mean if source UTF8 contains escaping char - it also
 will be escaped, even if source is completely valid UTF8.
 Corresponding reverse translation from wide to UTF8 will revert escaping.
 However fail tolerance works only in one direction - from UTF8 to UTF32. Broken UTF32 processed as
 usually - with lost of corrupted sequences, but escaped sequences are recovered to initial values.
*/

void Wide2MB(const wchar_t *src_begin, size_t src_len, std::string &dst, bool append = false);
void Wide2MB(const wchar_t *src, std::string &dst, bool append = false);
void MB2Wide(const char *src, size_t src_len, std::wstring &dst, bool append = false);
void MB2Wide(const char *src, std::wstring &dst, bool append = false);

std::string Wide2MB(const wchar_t *src);
std::wstring MB2Wide(const char *src);

void StrWide2MB(const std::wstring &src, std::string &dst, bool append = false);
std::string StrWide2MB(const std::wstring &src);

void StrMB2Wide(const std::string &src, std::wstring &dst, bool append = false);
std::wstring StrMB2Wide(const std::string &src);

/************************************************************************************************
 Some special routines
*/

// converts wchar_t(s) to multibyte _appending_ result to dst, doesnt care about escaping
// replaces untranslatable wchar_t-s by CHAR_REPLACEMENT
void Wide2MB_UnescapedAppend(const wchar_t wc, std::string &dst);
void Wide2MB_UnescapedAppend(const wchar_t *src_begin, size_t src_len, std::string &dst);

// same as MB2Wide but skips processing of up to last 6 invalid source chars and returns count of processed chars
size_t MB2Wide_HonorIncomplete(const char *src, size_t src_len, std::wstring &dst, bool append = false);

// those returned unsigneds are combinations of CONV_* flags or zero on success
unsigned MB2Wide_Unescaped(const char *src, size_t &src_len, wchar_t &dst, bool fail_on_illformed);
unsigned MB2Wide_Unescaped(const char *src, size_t &src_len, wchar_t *dst, size_t &dst_len, bool fail_on_illformed);
unsigned Wide2MB_Unescaped(const wchar_t *src, size_t &src_len, char *dst, size_t &dst_len, bool fail_on_illformed);
