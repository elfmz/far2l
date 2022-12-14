#include "headers.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <limits>
#include <algorithm>
#include "FindPattern.hpp"
#include "strmix.hpp"
#include "config.hpp"
#include "codepage.hpp"

FindPattern::FindPattern(bool case_sensitive, bool whole_words)
	: _case_sensitive(case_sensitive), _whole_words(whole_words)
{
}

template <> inline bool ScannedPattern<uint8_t, 6>::CodePoint::MatchNonEmptyData(const uint8_t *left, const uint8_t *right, unsigned char len) noexcept
{
	return left[0] == right[0]
		&& (len < 2 || left[1] == right[1])
		&& (len < 3 || left[2] == right[2])
		&& (len < 4 || left[3] == right[3])
		&& (len < 5 || left[4] == right[4])
		&& (len < 6 || left[5] == right[5]);
}

template <> inline bool ScannedPattern<uint16_t, 2>::CodePoint::MatchNonEmptyData(const uint16_t *left, const uint16_t *right, unsigned char len) noexcept
{
	return left[0] == right[0]
		&& (len < 2 || left[1] == right[1]);
}

template <> inline bool ScannedPattern<uint32_t, 1>::CodePoint::MatchNonEmptyData(const uint32_t *left, const uint32_t *right, unsigned char len) noexcept
{
	return left[0] == right[0];
}

template <> inline bool ScannedPattern<uint16_t, 1>::CodePoint::MatchNonEmptyData(const uint16_t *left, const uint16_t *right, unsigned char len) noexcept
{
	return left[0] == right[0];
}

template <> inline bool ScannedPattern<uint8_t, 1>::CodePoint::MatchNonEmptyData(const uint8_t *left, const uint8_t *right, unsigned char len) noexcept
{
	return left[0] == right[0];
}

////////////////

template <class PatternsT>
	static void FN_NOINLINE AddUniqPattern(PatternsT &patterns, const typename PatternsT::value_type &sp)
{
	if (std::find(patterns.begin(), patterns.end(), sp) != patterns.end()) {
		return; // dont add duplicated patterns
	}

	patterns.emplace_back(sp);
	auto &seq = patterns.back().seq;

	/** This is a most tricky code here:
	  * go through just added seq and for each non-heading codepoint assign 'rewind' position
	  * that specifies where MatchPattern() routine will seek back in pattern seq in case of
	  * mismatching this particular codepoint while all previous chunks where recently matched.
	  * Using correct rewind value ensures finding matches in cases like:
	  * {'ac' IN 'aac'}  {'abc' IN 'ababc'} {'bdbdba' IN 'bdbdbdba'} etc
	  */
	for (size_t i = 1; i < seq.size(); ++i) {
		for (size_t j = i - 1; j >= 1; --j) {
			if (seq.EqualParts(0, i - j, j)) {
				size_t n = 2;
				for (;j * n < i; ++n) {
					if (!seq.EqualParts(0, j * (n - 1), j)) {
						break;
					}
					if (!seq.EqualParts(0, (i - j * n), j)) {
						break;
					}
				}
				seq[i].SetRewindPos(j * (n - 1));
				break;
			}
		}
	}
}

void FindPattern::AddBytesPattern(const uint8_t *pattern, size_t len)
{
	if (len == 0) {
		ThrowPrintf("empty bytes pattern");
	}

	ScannedPattern8 sp8;
	for (size_t i = 0; i < len; ++i) {
		sp8.seq.EmplaceCodePoint(&pattern[i], 1, nullptr, 0);
	}
	AddUniqPattern(_patterns8, sp8);
}

struct CodePointConvertor
{
	union U {
		char     c[32];
		uint8_t  u8[32];
		uint16_t u16[16];
		uint32_t u32[8];
	} base, alt;

	unsigned int base_size{0}, alt_size{0};

	CodePointConvertor(wchar_t wc, unsigned int codepage, bool case_sensitive)
	{
		BOOL used_def_chr{FALSE};
		PBOOL p_used_def_chr = (!IsUTF7(codepage) && !IsUTF8(codepage) && !IsUTF16(codepage) && !IsUTF32(codepage)) ? &used_def_chr : nullptr;

		if (!case_sensitive) {
			wchar_t alt_wc = wc;
			WINPORT(CharUpperBuff)(&alt_wc, 1);
			if (alt_wc == wc) {
				WINPORT(CharLowerBuff)(&alt_wc, 1);
				if (alt_wc != wc) { // base data is lowercase to keep base/alt consistency
					std::swap(wc, alt_wc);
				}
			}
			if (alt_wc != wc) {
				int r = WINPORT(WideCharToMultiByte)(codepage, 0, &alt_wc, 1, alt.c, ARRAYSIZE(alt.c), nullptr, p_used_def_chr);
				if (r <= 0 || used_def_chr) {
					// dont fail, cuz still may proceed with base bytes
					fprintf(stderr, "%s: failed to covert to cp=%d alt wc=0x%x '%lc'\n", __FUNCTION__, codepage, alt_wc, alt_wc);
					used_def_chr = FALSE;

				} else {
					alt_size = r;
				}
			}
		}

		base_size = WINPORT(WideCharToMultiByte)(codepage, 0, &wc, 1, base.c, ARRAYSIZE(base.c), nullptr, p_used_def_chr);
		if (base_size <= 0 || used_def_chr) {
			ThrowPrintf("failed to covert to cp=%d wc=0x%x '%lc'", codepage, wc, wc);
		}
	}
};

void FindPattern::AddTextPattern(const wchar_t *pattern, unsigned int codepage)
{
	if (!*pattern) {
		ThrowPrintf("empty text pattern");
	}

	CPINFO cpi{};
	if (codepage != CP_UTF8 && codepage != CP_UTF7 && codepage != CP_UTF16BE && codepage != CP_UTF16LE
	  && (!WINPORT(GetCPInfo)(codepage, &cpi) || (cpi.MaxCharSize != 1 && cpi.MaxCharSize != 2 && cpi.MaxCharSize != 4))
		) {
		ThrowPrintf("bad codepage=%u MaxCharSize=%u", codepage, cpi.MaxCharSize);
	}

	ScannedPattern8 sp8;
	ScannedPattern16 sp16;
	ScannedPattern32 sp32;
	ScannedPattern8x sp8x;
	ScannedPattern16x sp16x;

	bool utf_all_short = true;

	for (;*pattern; ++pattern) {
		CodePointConvertor cpc(*pattern, codepage, _case_sensitive);

		if (codepage == CP_UTF8 || codepage == CP_UTF7) {
			// fill sp8x and sp8 and use later if string consists of all single byte chars
			sp8x.seq.EmplaceCodePoint(cpc.base.u8, cpc.base_size, cpc.alt.u8, cpc.alt_size);
			sp8.seq.EmplaceCodePoint(cpc.base.u8, 1, cpc.alt.u8, cpc.alt_size ? 1 : 0);
			if (cpc.base_size > sizeof(uint8_t) || cpc.alt_size > sizeof(uint8_t)) {
				utf_all_short = false;
			}

		} else if (codepage == CP_UTF16LE || codepage == CP_UTF16BE) {
			// fill sp16x and sp16 and use later if string consists of all single word chars
			sp16x.seq.EmplaceCodePoint(cpc.base.u16, cpc.base_size / sizeof(uint16_t),
				cpc.alt.u16, cpc.alt_size / sizeof(uint16_t));
			sp16.seq.EmplaceCodePoint(cpc.base.u16, 1,
				cpc.alt.u16, (cpc.alt_size >= sizeof(uint16_t)) ? 1 : 0);
			if (cpc.base_size > sizeof(uint16_t) || cpc.alt_size > sizeof(uint16_t)) {
				utf_all_short = false;
			}

		} else switch(cpi.MaxCharSize) {
			case sizeof(uint8_t):
				sp8.seq.EmplaceCodePoint(cpc.base.u8, cpc.base_size, cpc.alt.u8, cpc.alt_size);
				break;

			case sizeof(uint16_t):
				sp16.seq.EmplaceCodePoint(cpc.base.u16, cpc.base_size / sizeof(uint16_t),
					cpc.alt.u16, cpc.alt_size / sizeof(uint16_t));
				break;

			case sizeof(uint32_t):
				sp32.seq.EmplaceCodePoint(cpc.base.u32, cpc.base_size / sizeof(uint32_t),
					cpc.alt.u32, cpc.alt_size / sizeof(uint32_t));
				break;
		}
	}

	if (codepage == CP_UTF8 || codepage == CP_UTF7) {
		if (utf_all_short) {
			AddUniqPattern(_patterns8, sp8);
		} else {
			AddUniqPattern(_patterns8x, sp8x);
		}

	} else if (codepage == CP_UTF16BE || codepage == CP_UTF16LE) {
		if (utf_all_short) {
			AddUniqPattern(_patterns16, sp16);
		} else {
			AddUniqPattern(_patterns16x, sp16x);
		}

	} else switch(cpi.MaxCharSize) {
		case sizeof(uint8_t):
			AddUniqPattern(_patterns8, sp8);
			break;

		case sizeof(uint16_t):
			AddUniqPattern(_patterns16, sp16);
			break;

		case sizeof(uint32_t):
			AddUniqPattern(_patterns32, sp32);
			break;
	}
}

void FindPattern::GetReady()
{
	_min_pattern_size = std::numeric_limits<size_t>::max();
	_look_behind = 0;

	AccontMinimalAndLookBehindSizesFor(_patterns8x);
	AccontMinimalAndLookBehindSizesFor(_patterns16x);
	AccontMinimalAndLookBehindSizesFor(_patterns8);
	AccontMinimalAndLookBehindSizesFor(_patterns16);
	AccontMinimalAndLookBehindSizesFor(_patterns32);

	if (!_patterns32.empty()) {
		_look_behind = AlignUp(_look_behind, 4);
	} else if (!_patterns16.empty() || !_patterns16x.empty()) {
		_look_behind = AlignUp(_look_behind, 2);
	}

	fprintf(stderr, "FindPattern::GetReady: 8x:%lu 16x:%lu 8:%lu 16:%lu 32:%lu MPS=%lu LB=%lu\n",
		_patterns8x.size(), _patterns16x.size(), _patterns8.size(), _patterns16.size(),
		_patterns32.size(), _min_pattern_size, _look_behind);

	if (_patterns8x.empty() && _patterns16x.empty()
	  && _patterns8.empty() && _patterns16.empty() && _patterns32.empty()) {
		ThrowPrintf("no patterns defined");
	}
}

/** Helper function used if MatchPattern found match, its goal is to go back and find starting position of
  * matched substring. MatchPattern could do remembering itself, but this would defeat its performance.
  */
template <class SeqT, class CodeUnitT>
	static std::pair<size_t, size_t> LookbackMatchedRange(const SeqT &seq, const CodeUnitT *data, size_t edge) noexcept
{
	size_t len = 0;
	// go back in pattern elements sequence finding matched length to do next back step until reaching pattern start
	for (auto seq_it = seq.rbegin(); seq_it != seq.rend(); ++seq_it) {
		if (len + seq_it->MinCnt() <= edge && seq_it->Match(data + edge - len - seq_it->MinCnt(), seq_it->MinCnt())) {
			len+= seq_it->MinCnt();

		} else if (len + seq_it->MaxCnt() <= edge && seq_it->Match(data + edge - len - seq_it->MaxCnt(), seq_it->MaxCnt())) {
			len+= seq_it->MaxCnt();

		} else {
			fprintf(stderr, "LookupMatchedRange: fixme\n");
			abort();
		}
	}

	return std::make_pair(edge - len, len); //  {start, len} so caller will know exact range of matched substring
}

/** Actual finder routine. It sequentially goes through data array comparing current element with currently
  * selected pattern's codepoint. Note that codepoint may represent more than single codeunit so if such
  * codepoint matched then all matched codeunits skipped on data array and pattern iterated to next codepoint.
  * In case of mismatch one of two actions could be taken, depending on codepoint position in pattern:
  *  - if codepoint is at the head of pattern - then data array iterated forward by single codeunit.
  *  - otherwise pattern is 'rewinded' to position pre-calculated during pattern initialization.
  */
template <bool CASE_SENSITIVE, class SeqT, class CodeUnitT>
	static std::pair<size_t, size_t> MatchSeq(const SeqT &seq, const CodeUnitT *data, size_t len) noexcept
{
	auto seq_it = seq.begin();
	for (size_t i = 0; i != len; ) {
		const size_t match = CASE_SENSITIVE
			? seq_it->MatchOnlyBase(data + i, len - i)
			: seq_it->Match(data + i, len - i);

		if (match) {
			i+= match;
			++seq_it;
			if (UNLIKELY(seq_it == seq.end())) {
				return LookbackMatchedRange(seq, data, i);
			}

		} else if (seq_it != seq.begin()) {
			seq_it = seq.begin() + seq_it->RewindPos();

		} else {
			++i;
		}
	}
	return std::make_pair((size_t)-1, (size_t)0);
}

// Проверяем символ на принадлежность разделителям слов
static bool IsWordDiv(const wchar_t symbol) noexcept
{
	// Так же разделителем является конец строки и пробельные символы
	return !symbol || IsSpace(symbol) || IsEol(symbol) || IsWordDiv(Opt.strWordDiv,symbol);
}

template <class CodeUnitT, class PatternsT>
	static std::pair<size_t, size_t> MatchPatterns(const PatternsT &patterns, const void *data, size_t len, bool whole_words, bool first_fragment, bool last_fragment, bool case_sensitive) noexcept
{
	// translate pointer and len to CodeUnitT-based metrics
	const CodeUnitT *cud = (const CodeUnitT *)data;
	len/= sizeof(CodeUnitT);

	for (const auto &p : patterns) {
		for (size_t ofs = 0;;) {
			const auto &r = case_sensitive
					? MatchSeq<true>(p.seq, cud + ofs, len - ofs)
					: MatchSeq<false>(p.seq, cud + ofs, len - ofs);
			if (r.first == (size_t)-1) {
				break;
			}
			ofs+= r.first;
			if (whole_words) {
				const size_t edge = ofs + r.second;
				const bool left_ok = (ofs == 0 && first_fragment) || (ofs != 0 && IsWordDiv(cud[ofs - 1]));
				const bool right_ok = (edge >= len && last_fragment) || (edge < len && IsWordDiv(cud[edge]));
				if (!left_ok || !right_ok) {
					ofs = edge;
					continue;
				}
			}
			// all checks passed, return matched range translated back to bytes
			return std::make_pair(ofs * sizeof(CodeUnitT), r.second * sizeof(CodeUnitT));
		}
	}

	return std::make_pair((size_t)-1, (size_t)0);
}

std::pair<size_t, size_t> FindPattern::Match(const void *data, size_t len, bool first_fragment, bool last_fragment) const noexcept
{
	auto r = MatchPatterns<uint8_t>(_patterns8x, data, len, _whole_words, first_fragment, last_fragment, _case_sensitive);
	if (r.first == (size_t)-1) {
		r = MatchPatterns<uint16_t>(_patterns16x, data, len, _whole_words, first_fragment, last_fragment, _case_sensitive);
	}
	if (r.first == (size_t)-1) {
		r = MatchPatterns<uint8_t>(_patterns8, data, len, _whole_words, first_fragment, last_fragment, _case_sensitive);
	}
	if (r.first == (size_t)-1) {
		r = MatchPatterns<uint16_t>(_patterns16, data, len, _whole_words, first_fragment, last_fragment, _case_sensitive);
	}
	if (r.first == (size_t)-1) {
		r = MatchPatterns<uint32_t>(_patterns32, data, len, _whole_words, first_fragment, last_fragment, _case_sensitive);
	}
	return r;
}
