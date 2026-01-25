#include "headers.hpp"
#include "LinkHighlighter.hpp"

#include <cwctype>
#include <string>

#include "execute.hpp"

namespace LinkHighlighter
{
namespace
{
constexpr const wchar_t* Prefixes[] = { L"https:", L"http:", L"mailto:" };

static bool StartsWithNoCase(const wchar_t* s, size_t len, const wchar_t* prefix)
{
	const size_t plen = wcslen(prefix);
	if (plen > len)
		return false;

	for (size_t i = 0; i < plen; ++i) {
		if (towlower(s[i]) != towlower(prefix[i]))
			return false;
	}
	return true;
}

static bool IsLinkBodyChar(wchar_t ch)
{
	return ch &&
		!iswspace(ch) &&
		!wcschr(L"<>\"'[]{}()", ch);
}

static bool IsTrailingLinkPunct(wchar_t ch)
{
	return wcschr(L".,;:!?) ]}", ch) != nullptr;
}

static int HasLinkPrefix(const wchar_t* ptr, size_t remaining)
{
	if (!ptr || !remaining)
		return false;

	for (auto p : Prefixes) {
		size_t plen = wcslen(p);
		if (StartsWithNoCase(ptr, remaining, p))
			if (remaining > plen && IsLinkBodyChar(ptr[plen]))
				return true;
	}
	return false;
}
}

size_t SkipVisibleControlSequence(const wchar_t* line, size_t len, size_t pos)
{
	if (!line || pos >= len)
		return pos;

	auto isEsc = [&](size_t i) {
		return line[i] == L'\x1b'
			|| (i + 2 < len && line[i] == L'%' && line[i + 1] == L'1'
			    && (line[i + 2] == L'B' || line[i + 2] == L'b'));
	};

	if (!isEsc(pos))
		return pos;

	size_t i = pos + (line[pos] == L'\x1b' ? 1 : 3);

	if (i < len && line[i] == L'[') {
		++i;
		while (i < len) {
			wchar_t ch = line[i++];
			if (!(iswdigit(ch) || ch == L';' || ch == L':' || ch == L'?'
			      || ch == L'>' || ch == L'(' || ch == L')'))
				break;
		}
	}

	return i;
}

std::wstring StripVisibleControlSequences(const wchar_t* data, size_t length)
{
	std::wstring out;
	if (!data || !length)
		return out;

	out.reserve(length);

	for (size_t i = 0; i < length; ) {
		size_t next = SkipVisibleControlSequence(data, length, i);
		if (next == i)
			out.push_back(data[i++]);
		else
			i = next;
	}
	return out;
}

const uint32_t LinkForegroundRGB = (0x33u) | (0x88u << 8) | (0xffu << 16);
const uint32_t LinkHoverBackgroundRGB = (0x00u) | (0x40u << 8) | (0x80u << 16);

void DetectLinks(const wchar_t* line, std::vector<LinkRange>& ranges)
{
	if (!line) {
		ranges.clear();
		return;
	}

	DetectLinks(line, wcslen(line), ranges);
}

void DetectLinks(const wchar_t* line, size_t length, std::vector<LinkRange>& ranges)
{
	ranges.clear();
	if (!line || !length)
		return;

	for (size_t i = 0; i < length; ) {
		if (!HasLinkPrefix(line + i, length - i)) {
			++i;
			continue;
		}

		if (i && (iswalnum(line[i - 1]) || line[i - 1] == L'_')) {
			++i;
			continue;
		}

		size_t start = i;
		while (i < length && IsLinkBodyChar(line[i]))
			++i;

		while (i > start && IsTrailingLinkPunct(line[i - 1]))
			--i;

		if (i > start)
			ranges.push_back({ (int)start, (int)(i - start) });
	}
}

uint64_t ApplyLinkColor(uint64_t attr, bool highlighted)
{
	uint64_t result = attr;

	if (result & FOREGROUND_TRUECOLOR) {
		SET_RGB_FORE(result, LinkForegroundRGB);
	} else {
		result &= ~(uint64_t)(FOREGROUND_RED | FOREGROUND_GREEN |
		                      FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
	}

	if (highlighted) {
		if (result & BACKGROUND_TRUECOLOR) {
			SET_RGB_BACK(result, LinkHoverBackgroundRGB);
		} else {
			result &= ~(uint64_t)(BACKGROUND_RED | BACKGROUND_GREEN |
			                      BACKGROUND_BLUE | BACKGROUND_INTENSITY);
			result |= BACKGROUND_GREEN | BACKGROUND_BLUE;
		}
	}

	return result;
}

bool Launch(const wchar_t* start, size_t length)
{
	if (!start || !length || !HasLinkPrefix(start, length))
		return false;

	FARString url;
	url.Copy(start, length);

	farExecuteA(
		url.GetMB().c_str(),
		EF_NOWAIT | EF_HIDEOUT | EF_NOCMDPRINT | EF_OPEN
	);
	return true;
}
}
