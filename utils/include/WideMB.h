#pragma once
#include <string>

void Wide2MB(const wchar_t *src, std::string &dst);
void MB2Wide(const char *src, size_t src_len, std::wstring &dst);
void MB2Wide(const char *src, std::wstring &dst);
size_t MB2Wide_HonorIncomplete(const char *src, size_t src_len, std::wstring &dst);
std::string Wide2MB(const wchar_t *src);
std::wstring MB2Wide(const char *src);

void StrWide2MB(const std::wstring &src, std::string &dst);
std::string StrWide2MB(const std::wstring &src);

void StrMB2Wide(const std::string &src, std::wstring &dst);
std::wstring StrMB2Wide(const std::string &src);
