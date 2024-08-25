#pragma once
#include <string>
#include <vector>

std::string EscapeLikeInC(std::string str);

std::string EscapeEscapes(std::string str);
std::string EscapeQuotes(const std::string &str);
std::wstring EscapeQuotes(const std::wstring &str);

/* by default escapes those chars as if whole string is a double-quoted command argument */
std::string EscapeCmdStr(const std::string &str, const char *escaped_chars = "\\\"$`");
std::wstring EscapeCmdStr(const std::wstring &str, const wchar_t *escaped_chars = L"\\\"$`");

void QuoteCmdArg(std::string &str);
void QuoteCmdArg(std::wstring &str);
void QuoteCmdArgIfNeed(std::string &str);
void QuoteCmdArgIfNeed(std::wstring &str);

