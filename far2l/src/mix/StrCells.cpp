#include "headers.hpp"

#include <utils.h>
#include "StrCells.h"
#include "config.hpp"

size_t StrCellsCount(const wchar_t *pwz, size_t nw)
{
	size_t out = 0;
	for (size_t i = 0; i < nw; ++i) {
		CharClasses cc(pwz[i]);
		if (cc.FullWidth()) {
			out+= 2;
		} else if ((i == nw - 1 || !cc.Prefix()) && !cc.Suffix() ) {
			++out;
		}
	}
	return out;
}

size_t StrZCellsCount(const wchar_t *pwz)
{
	size_t out = 0;
	for (size_t i = 0; pwz[i] != 0; ++i) {
		CharClasses cc(pwz[i]);
		if (cc.FullWidth()) {
			out+= 2;
		} else if ((pwz[i + 1] == 0 || !cc.Prefix()) && !cc.Suffix() ) {
			++out;
		}
	}
	return out;
}

size_t StrSizeOfCells(const wchar_t *pwz, size_t n, size_t &ng, bool round_up)
{
	size_t i = 0, g = 0;
	for (; g < ng && i < n; ++g) {
		for (; i < n; ++i) {
			if (!CharClasses(pwz[i]).Xxxfix()) {
				break;
			}
		}
		if (i < n) {
			if (CharClasses(pwz[i]).FullWidth()) {
//				++g;
//				if (!round_up && g == ng) {
				if (!round_up && (g + 1) == ng) {
					break;
				}
				++g;
			}
			++i;
		}
		for (; i < n; ++i) {
			if (!CharClasses(pwz[i]).Suffix()) {
				break;
			}
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
		if (!CharClasses(pwz[ofs]).Xxxfix() && StrCellsCount(pwz + ofs, n - ofs) + rpl.len <= ng) {
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
		while (n > 0 && CharClasses(pwz[n - 1]).Xxxfix()) {
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
	while (cut_start > 0 && CharClasses(pwz[cut_start]).Xxxfix()) {
		--cut_start;
	}
	auto cut_end = cut_start + rpl.len;
	while (cut_end < n && CharClasses(pwz[cut_end]).Xxxfix()) {
		++cut_end;
	}

	while (StrCellsCount(pwz, cut_start) + StrCellsCount(pwz + cut_end, n - cut_end) + rpl.len > ng) {
		if (cut_start > 0) {
			--cut_start;
			while (cut_start > 0 && CharClasses(pwz[cut_start]).Xxxfix()) {
				--cut_start;
			}
			if (StrCellsCount(pwz, cut_start) + StrCellsCount(pwz + cut_end, n - cut_end) + rpl.len <= ng) {
				break;
			}
		}
		if (cut_end < n) {
			++cut_end;
			while (cut_end < n && CharClasses(pwz[cut_end]).Xxxfix()) {
				++cut_end;
			}
		}
	}

	wmemmove(&pwz[cut_start + rpl.len], &pwz[cut_end], n - cut_end);
	wmemcpy(&pwz[cut_start], rpl.wz, rpl.len);
	n-= (cut_end - cut_start);
	n+= rpl.len;
}
