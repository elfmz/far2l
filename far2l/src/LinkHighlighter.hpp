#pragma once

#include <vector>
#include <stdint.h>
#include <cstddef>
#include <string>

namespace LinkHighlighter
{
struct LinkRange
{
	int startChar = 0;
	int lengthChars = 0;
};

extern const uint32_t LinkForegroundRGB;
extern const uint32_t LinkHoverBackgroundRGB;

void DetectLinks(const wchar_t *line, std::vector<LinkRange> &ranges);
void DetectLinks(const wchar_t *line, size_t length, std::vector<LinkRange> &ranges);
uint64_t ApplyLinkColor(uint64_t attr, bool highlighted);
bool Launch(const wchar_t *start, size_t length);
size_t SkipVisibleControlSequence(const wchar_t *line, size_t len, size_t pos);
std::wstring StripVisibleControlSequences(const wchar_t *data, size_t length);
}
