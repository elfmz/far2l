#include "utils.h"

size_t StrVisualLength(const wchar_t *pwz, size_t n)
{
	size_t out = 0;
	for (size_t i = 0; i < n; ++i) {
		if (IsCharFullWidth(*pwz)) {
			out+= 2;
		} else if ((i == n - 1 || !IsCharPrefix(pwz[i])) && (i == 0 || !IsCharSuffix(pwz[i]))) {
			++out;
		}
	}
	return out;
}

size_t StrVisualLookupGrapheme(const wchar_t *pwz, size_t n)
{
	size_t i = 0;
	for (; i < n; ++i) {
		if (!IsCharPrefix(pwz[i])) {
			break;
		}
	}
	if (i < n) {
		++i;
	}
	for (; i < n; ++i) {
		if (!IsCharSuffix(pwz[i])) {
			break;
		}
	}
	return i;
}

void StrVisualTruncateLeft(wchar_t *pwz, size_t &n, size_t vl_max)
{
	size_t vl = StrVisualLength(pwz, n);
	if (vl <= vl_max || n < 3) {
		return;
	}

	for (size_t ofs = 3; ofs < n; ++ofs) {
		if (!IsCharXxxfix(pwz[ofs]) && StrVisualLength(pwz + ofs, n - ofs) + 3 <= vl_max) {
			n-= ofs;
			wmemmove(pwz + 3, pwz + ofs, n);
			n+= 3;
			wmemcpy(pwz, L"...", 3);
			return;
		}
	}
	wcsncpy(pwz, L"...", vl_max);
	n = vl_max;
}

void StrVisualTruncateRight(wchar_t *pwz, size_t &n, size_t vl_max)
{
	size_t vl = StrVisualLength(pwz, n);
	if (vl <= vl_max || n < 3) {
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
	} while (StrVisualLength(pwz, n) + 3 > vl_max);

	wmemcpy(&pwz[n], L"...", 3);
	n+= 3;
}

void StrVisualTruncateCenter(wchar_t *pwz, size_t &n, size_t vl_max)
{
	size_t vl = StrVisualLength(pwz, n);
	if (vl <= vl_max || n < 3) {
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

	while (StrVisualLength(pwz, cut_start) + StrVisualLength(pwz + cut_end, n - cut_end) + 3 > vl_max) {
		if (cut_start > 0) {
			--cut_start;
			while (cut_start > 0 && IsCharXxxfix(pwz[cut_start])) {
				--cut_start;
			}
			if (StrVisualLength(pwz, cut_start) + StrVisualLength(pwz + cut_end, n - cut_end) + 3 <= vl_max) {
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



