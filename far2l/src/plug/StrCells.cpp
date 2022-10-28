#include "headers.hpp"

#include <utils.h>
#include "StrCells.h"
#include "config.hpp"

extern "C"
{

__attribute__ ((visibility("default"))) size_t FarStrCellsCount(const wchar_t *pwz, size_t nw)
{
	size_t out = 0;
	for (size_t i = 0; i < nw; ++i) {
		if (IsCharFullWidth(pwz[i])) {
			out+= 2;
		} else if ((i == nw - 1 || !IsCharPrefix(pwz[i])) && (i == 0 || !IsCharSuffix(pwz[i]))) {
			++out;
		}
	}
	return out;
}

__attribute__ ((visibility("default"))) size_t FarStrZCellsCount(const wchar_t *pwz)
{
	return FarStrCellsCount(pwz, wcslen(pwz));
}

__attribute__ ((visibility("default"))) size_t FarStrSizeOfCells(const wchar_t *pwz, size_t n, size_t &ng, bool round_up)
{
	size_t i = 0, g = 0;
	for (; g < ng && i < n; ++g) {
		for (; i < n; ++i) {
			if (!IsCharPrefix(pwz[i])) {
				break;
			}
		}
		if (i < n) {
			if (IsCharFullWidth(pwz[i])) {
				++g;
				if (!round_up && g == ng) {
					break;
				}
			}
			++i;
		}
		for (; i < n; ++i) {
			if (!IsCharSuffix(pwz[i])) {
				break;
			}
		}
	}
	ng = g;
	return i;
}

__attribute__ ((visibility("default"))) size_t FarStrSizeOfCell(const wchar_t *pwz, size_t n)
{
	size_t ng = 1;
	return FarStrSizeOfCells(pwz, n, ng, true);
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

__attribute__ ((visibility("default"))) void FarStrCellsTruncateLeft(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = FarStrCellsCount(pwz, n);
	const auto &rpl = ChooseTruncReplacement();
	if (vl <= ng || n < rpl.len) {
		return;
	}

	for (size_t ofs = rpl.len; ofs < n; ++ofs) {
		if (!IsCharXxxfix(pwz[ofs]) && FarStrCellsCount(pwz + ofs, n - ofs) + rpl.len <= ng) {
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

__attribute__ ((visibility("default"))) void FarStrCellsTruncateRight(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = FarStrCellsCount(pwz, n);
	const auto &rpl = ChooseTruncReplacement();
	if (vl <= ng || n < rpl.len) {
		return;
	}

	n-= rpl.len; // pre-reserve space for ...
	do {
		while (n > 0 && IsCharXxxfix(pwz[n - 1])) {
			--n;
		}
		if (n == 0) {
			break;
		}
		--n;
	} while (FarStrCellsCount(pwz, n) + rpl.len > ng);

	wmemcpy(&pwz[n], rpl.wz, rpl.len);
	n+= rpl.len;
}

__attribute__ ((visibility("default"))) void FarStrCellsTruncateCenter(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = FarStrCellsCount(pwz, n);
	const auto &rpl = ChooseTruncReplacement();
	if (vl <= ng || n < rpl.len) {
		return;
	}

	auto cut_start = n / 2;
	if (cut_start > 0) {
		--cut_start;
	}
	if (cut_start > 0) {
		--cut_start;
	}
	while (cut_start > 0 && IsCharXxxfix(pwz[cut_start])) {
		--cut_start;
	}
	auto cut_end = cut_start + rpl.len;
	while (cut_end < n && IsCharXxxfix(pwz[cut_end])) {
		++cut_end;
	}

	while (FarStrCellsCount(pwz, cut_start) + FarStrCellsCount(pwz + cut_end, n - cut_end) + rpl.len > ng) {
		if (cut_start > 0) {
			--cut_start;
			while (cut_start > 0 && IsCharXxxfix(pwz[cut_start])) {
				--cut_start;
			}
			if (FarStrCellsCount(pwz, cut_start) + FarStrCellsCount(pwz + cut_end, n - cut_end) + rpl.len <= ng) {
				break;
			}
		}
		if (cut_end < n) {
			++cut_end;
			while (cut_end < n && IsCharXxxfix(pwz[cut_end])) {
				++cut_end;
			}
		}
	}

	wmemmove(&pwz[cut_start + rpl.len], &pwz[cut_end], n - cut_end);
	wmemcpy(&pwz[cut_start], rpl.wz, rpl.len);
	n-= (cut_end - cut_start);
	n+= rpl.len;
}

}
