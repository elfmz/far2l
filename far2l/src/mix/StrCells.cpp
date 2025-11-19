#include "headers.hpp"

#include <utils.h>
#include "StrCells.h"
#include "config.hpp"

size_t StrCellsCount(const wchar_t *pwz, size_t nw)
{
	size_t out = 0;
	bool joining = false;
	for (size_t i = 0; i < nw; ++i) {
		if (pwz[i] == CharClasses::ZERO_WIDTH_JOINER) {
			joining = true;
			continue;
		} else if (CharClasses::IsXxxfix(pwz[i])) {
			continue;
		} else if (!joining) {
			out += CharClasses::IsFullWidth(&pwz[i]) ? 2 : 1;
		}
		joining = false;
	}
	return out;
}

size_t StrZCellsCount(const wchar_t *pwz)
{
	size_t out = 0;
	bool joining = false;
	for (size_t i = 0; pwz[i] != 0; ++i) {
		if (pwz[i] == CharClasses::ZERO_WIDTH_JOINER) {
			joining = true;
			continue;
		} else if (CharClasses::IsXxxfix(pwz[i])) {
			continue;
		} else if (!joining) {
			out += CharClasses::IsFullWidth(&pwz[i]) ? 2 : 1;
		}
		joining = false;
	}
	return out;
}

size_t StrSizeOfCells(const wchar_t *pwz, size_t n, size_t &ng, bool round_up)
{
	size_t i = 0, g = 0;
	bool joining = false;

	size_t char_width = 1;
	while (g < ng && i < n) {
		char_width = 1;
		for (; i < n; ++i) {
			if (pwz[i] == CharClasses::ZERO_WIDTH_JOINER) {
				joining = true;
				++i;
				break;
			}
			if (!CharClasses::IsXxxfix(pwz[i]))
				break;
		}
		if (i < n) {
			if (CharClasses::IsFullWidth(&pwz[i])) {
//				++g;
//				if (!round_up && g == ng) {
				if (!round_up && (g + char_width) >= ng) {
					break;
				}
				char_width=2;
			}
			++i;
			joining = false;
		}
		for (; i < n; ++i) {
			if (pwz[i] == CharClasses::ZERO_WIDTH_JOINER) {
				joining = true;
				++i;
				break;
			}
			if (!CharClasses::IsSuffix(pwz[i]))
				break;
		}

		if (joining) {
			continue;
		} else {
			g+= char_width;
		}
	}
	ng = g;
	return i;
}

size_t StrSizeOfCell(const wchar_t *pwz, size_t n)
{
	size_t ng = 1;
	return StrSizeOfCells(pwz, n, ng, true);
}

static struct TruncReplacement
{
	const wchar_t *wz;
	size_t len;
} s_trunc_replacement[2] = { {L"...", 3}, {L"…", 1} };

static const struct TruncReplacement &ChooseTruncReplacement()
{
	return s_trunc_replacement[Opt.NoGraphics ? 0 : 1];
}

void StrCellsTruncateLeft(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = StrCellsCount(pwz, n);
	const auto &rpl = ChooseTruncReplacement();
	if (vl <= ng || n < rpl.len) {
		return;
	}

	for (size_t ofs = rpl.len; ofs < n; ++ofs) {
		if (!CharClasses::IsXxxfix(pwz[ofs]) && StrCellsCount(pwz + ofs, n - ofs) + rpl.len <= ng) {
			n-= ofs;
			wmemmove(pwz + rpl.len, pwz + ofs, n);
			n+= rpl.len;
			wmemcpy(pwz, rpl.wz, rpl.len); //…
			return;
		}
	}
	wcsncpy(pwz, rpl.wz, ng);
	n = std::min(ng, rpl.len);
}

void StrCellsTruncateRight(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = StrCellsCount(pwz, n);
	const auto &rpl = ChooseTruncReplacement();
	if (vl <= ng || n < rpl.len) {
		return;
	}

	n-= rpl.len; // pre-reserve space for ...
	do {
		while (n > 0 && CharClasses::IsXxxfix(pwz[n - 1])) {
			--n;
		}
		if (n == 0) {
			break;
		}
		--n;
	} while (StrCellsCount(pwz, n) + rpl.len > ng);

	wmemcpy(&pwz[n], rpl.wz, rpl.len);
	n+= rpl.len;
}

void StrCellsTruncateCenter(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = StrCellsCount(pwz, n);
	const auto &rpl = ChooseTruncReplacement();
	if (vl <= ng || n < rpl.len) {
		return;
	}

	auto cut_start = n / 2;
	if (cut_start > 0) {
		--cut_start;
	}
	if (cut_start > 0 && rpl.len > 1) {
		--cut_start;
	}
	while (cut_start > 0 && CharClasses::IsXxxfix(pwz[cut_start])) {
		--cut_start;
	}
	auto cut_end = cut_start + rpl.len;
	while (cut_end < n && CharClasses::IsXxxfix(pwz[cut_end])) {
		++cut_end;
	}

	while (StrCellsCount(pwz, cut_start) + StrCellsCount(pwz + cut_end, n - cut_end) + rpl.len > ng) {
		if (cut_start > 0) {
			--cut_start;
			while (cut_start > 0 && CharClasses::IsXxxfix(pwz[cut_start])) {
				--cut_start;
			}
			if (StrCellsCount(pwz, cut_start) + StrCellsCount(pwz + cut_end, n - cut_end) + rpl.len <= ng) {
				break;
			}
		}
		if (cut_end < n) {
			++cut_end;
			while (cut_end < n && CharClasses::IsXxxfix(pwz[cut_end])) {
				++cut_end;
			}
		}
	}

	wmemmove(&pwz[cut_start + rpl.len], &pwz[cut_end], n - cut_end);
	wmemcpy(&pwz[cut_start], rpl.wz, rpl.len);
	n-= (cut_end - cut_start);
	n+= rpl.len;
}
