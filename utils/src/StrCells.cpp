#include "utils.h"

size_t StrCellsCount(const wchar_t *pwz, size_t nw)
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

size_t StrCellsCount(const wchar_t *pwz)
{
	return StrCellsCount(pwz, wcslen(pwz));
}

size_t StrSizeOfCells(const wchar_t *pwz, size_t n, size_t &ng, bool round_up)
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


size_t StrSizeOfCell(const wchar_t *pwz, size_t n)
{
	size_t ng = 1;
	return StrSizeOfCells(pwz, n, ng, true);
}

void StrCellsTruncateLeft(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = StrCellsCount(pwz, n);
	if (vl <= ng || n < 3) {
		return;
	}

	for (size_t ofs = 3; ofs < n; ++ofs) {
		if (!IsCharXxxfix(pwz[ofs]) && StrCellsCount(pwz + ofs, n - ofs) + 3 <= ng) {
			n-= ofs;
			wmemmove(pwz + 3, pwz + ofs, n);
			n+= 3;
			wmemcpy(pwz, L"...", 3);
			return;
		}
	}
	wcsncpy(pwz, L"...", ng);
	n = ng;
}

void StrCellsTruncateRight(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = StrCellsCount(pwz, n);
	if (vl <= ng || n < 3) {
		return;
	}

	n-= 3; // pre-reserve space for ...
	do {
		while (n > 0 && IsCharXxxfix(pwz[n - 1])) {
			--n;
		}
		if (n == 0) {
			break;
		}
		--n;
	} while (StrCellsCount(pwz, n) + 3 > ng);

	wmemcpy(&pwz[n], L"...", 3);
	n+= 3;
}

void StrCellsTruncateCenter(wchar_t *pwz, size_t &n, size_t ng)
{
	size_t vl = StrCellsCount(pwz, n);
	if (vl <= ng || n < 3) {
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
	auto cut_end = cut_start + 3;
	while (cut_end < n && IsCharXxxfix(pwz[cut_end])) {
		++cut_end;
	}

	while (StrCellsCount(pwz, cut_start) + StrCellsCount(pwz + cut_end, n - cut_end) + 3 > ng) {
		if (cut_start > 0) {
			--cut_start;
			while (cut_start > 0 && IsCharXxxfix(pwz[cut_start])) {
				--cut_start;
			}
			if (StrCellsCount(pwz, cut_start) + StrCellsCount(pwz + cut_end, n - cut_end) + 3 <= ng) {
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

	wmemmove(&pwz[cut_start + 3], &pwz[cut_end], n - cut_end);
	wmemcpy(&pwz[cut_start], L"...", 3);
	n-= (cut_end - cut_start);
	n+= 3;
}



