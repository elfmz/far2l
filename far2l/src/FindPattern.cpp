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

template <> inline bool ScannedPattern<uint8_t, 6>::Chunk::MatchNonEmptyData(const uint8_t *left, const uint8_t *right, unsigned char len)
{
	return left[0] == right[0]
		&& (len < 2 || left[1] == right[1])
		&& (len < 3 || left[2] == right[2])
		&& (len < 4 || left[3] == right[3])
		&& (len < 5 || left[4] == right[4])
		&& (len < 6 || left[5] == right[5]);
}

template <> inline bool ScannedPattern<uint16_t, 2>::Chunk::MatchNonEmptyData(const uint16_t *left, const uint16_t *right, unsigned char len)
{
	return left[0] == right[0]
		&& (len < 2 || left[1] == right[1]);
}

template <> inline bool ScannedPattern<uint32_t, 1>::Chunk::MatchNonEmptyData(const uint32_t *left, const uint32_t *right, unsigned char len)
{
	return left[0] == right[0];
}

template <> inline bool ScannedPattern<uint16_t, 1>::Chunk::MatchNonEmptyData(const uint16_t *left, const uint16_t *right, unsigned char len)
{
	return left[0] == right[0];
}

template <> inline bool ScannedPattern<uint8_t, 1>::Chunk::MatchNonEmptyData(const uint8_t *left, const uint8_t *right, unsigned char len)
{
	return left[0] == right[0];
}

////////////////

template <class PatternsT>
	static void AddUniqPattern(PatternsT &patterns, const typename PatternsT::value_type &sp)
{
	if (std::find(patterns.begin(), patterns.end(), sp) == patterns.end()) {
		patterns.emplace_back(sp);
		patterns.back().GetReady();
	}
}

void FindPattern::AddBytesPattern(const uint8_t *pattern, size_t len)
{
	ScannedPattern8 sp8;
	sp8.chain.resize(len);
	for (size_t i = 0; i < len; ++i) {
		sp8.chain[i].SetBase(&pattern[i], 1);
	}
	AddUniqPattern(_patterns8, sp8);
}

bool FindPattern::AddTextPattern(const wchar_t *pattern, unsigned int codepage)
{
	CPINFO cpi{};
	if (codepage != CP_UTF8 && codepage != CP_UTF7
	  && (!WINPORT(GetCPInfo)(codepage, &cpi) || (cpi.MaxCharSize != 1 && cpi.MaxCharSize != 2 && cpi.MaxCharSize != 4))
		) {
		fprintf(stderr, "%s: bad codepage=%u MaxCharSize=%u\n", __FUNCTION__, codepage, cpi.MaxCharSize);
		return false;
	}

	ScannedPattern8 sp8;
	ScannedPattern16 sp16;
	ScannedPattern32 sp32;
	ScannedPattern8x sp8x;
	ScannedPattern16x sp16x;

	bool utf_all_short = true;

	BOOL used_def_chr = FALSE;
	PBOOL p_used_def_chr = (!IsUTF7(codepage) && !IsUTF8(codepage) && !IsUTF16(codepage) && !IsUTF32(codepage)) ? &used_def_chr : nullptr;

	for (;*pattern; ++pattern) {
		union U {
			char     c[32];
			uint8_t  u8[32];
			uint16_t u16[16];
			uint32_t u32[8];
		} base, alt;

		int alt_r = 0;
		int r = WINPORT(WideCharToMultiByte)(codepage, 0, pattern, 1, base.c, ARRAYSIZE(base.c), nullptr, p_used_def_chr);
		if (r <= 0 || used_def_chr) {
			fprintf(stderr, "%s: failed to covert to cp=%d wc=0x%x '%lc'\n", __FUNCTION__, codepage, *pattern, *pattern);
			return false;
		}
		if (!_case_sensitive) {
			wchar_t tmp[2] = {*pattern, 0};
			WINPORT(CharLowerBuff)(tmp, 1);
			if (tmp[0] == *pattern) {
				WINPORT(CharUpperBuff)(tmp, 1);
			}
			if (tmp[0] != *pattern) {
				alt_r = WINPORT(WideCharToMultiByte)(codepage, 0, tmp, 1, alt.c, ARRAYSIZE(alt.c), nullptr, p_used_def_chr);
				if (alt_r <= 0 || used_def_chr) {
					fprintf(stderr, "%s: failed to covert to cp=%d alt wc=0x%x '%lc'\n", __FUNCTION__, codepage, *tmp, *tmp);
					used_def_chr = FALSE;
					alt_r = 0;
					// dont fail, cuz still may proceed with base bytes
				}
			}
		}

		if (codepage == CP_UTF8 || codepage == CP_UTF7) {
			// fill sp8x and sp8 and use later if string consists of all single byte chars
			sp8x.chain.emplace_back();
			sp8x.chain.back().SetBase(&base.u8[0], r);
			sp8.chain.emplace_back();
			sp8.chain.back().SetBase(&base.u8[0], 1);
			if (alt_r > 0) {
				sp8x.chain.back().SetAlt(&alt.u8[0], alt_r);
				sp8.chain.back().SetAlt(&alt.u8[0], 1);
			}
			if (r > 1 || alt_r > 1) {
				utf_all_short = false;
			}

		} else if (codepage == CP_UTF16LE || codepage == CP_UTF16BE) {
			// fill sp16x and sp16 and use later if string consists of all single word chars
			sp16x.chain.emplace_back();
			sp16x.chain.back().SetBase(&base.u16[0], r / 2);
			sp16.chain.emplace_back();
			sp16.chain.back().SetBase(&base.u16[0], 1);
			if (alt_r > 1) {
				sp16x.chain.back().SetAlt(&alt.u16[0], alt_r / 2);
				sp16.chain.back().SetAlt(&alt.u16[0], 1);
			}
			if (r > 2 || alt_r > 2) {
				utf_all_short = false;
			}

		} else switch(cpi.MaxCharSize) {
			case 1:
				sp8.chain.emplace_back();
				sp8.chain.back().SetBase(&base.u8[0], r);
				sp8.chain.back().SetAlt(&alt.u8[0], alt_r);
				break;

			case 2:
				sp16.chain.emplace_back();
				sp16.chain.back().SetBase(&base.u16[0], r / 2);
				sp16.chain.back().SetAlt(&alt.u16[0], alt_r / 2);
				break;

			case 4:
				sp32.chain.emplace_back();
				sp32.chain.back().SetBase(&base.u32[0], r / 4);
				sp32.chain.back().SetAlt(&alt.u32[0], alt_r / 4);
				break;

			default:
				return false;
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
		case 1:
			AddUniqPattern(_patterns8, sp8);
			break;

		case 2:
			AddUniqPattern(_patterns16, sp16);
			break;

		case 4:
			AddUniqPattern(_patterns32, sp32);
			break;
	}

	return true;
}


void FindPattern::GetReady()
{
	_min_pattern_size = std::numeric_limits<size_t>::max();
	_look_behind = 0;

	for (const auto &p : _patterns8x) {
		_min_pattern_size = std::min(_min_pattern_size, p.MinSize());
		_look_behind = std::max(_look_behind, p.LookBehind());
	}
	for (const auto &p : _patterns16x) {
		_min_pattern_size = std::min(_min_pattern_size, p.MinSize());
		_look_behind = std::max(_look_behind, p.LookBehind());
	}
	for (const auto &p : _patterns8) {
		_min_pattern_size = std::min(_min_pattern_size, p.MinSize());
		_look_behind = std::max(_look_behind, p.LookBehind());
	}
	for (const auto &p : _patterns16) {
		_min_pattern_size = std::min(_min_pattern_size, p.MinSize());
		_look_behind = std::max(_look_behind, p.LookBehind());
	}
	for (const auto &p : _patterns32) {
		_min_pattern_size = std::min(_min_pattern_size, p.MinSize());
		_look_behind = std::max(_look_behind, p.LookBehind());
	}

	if (_whole_words) {
		size_t whole_char_size = 1;
		if (!_patterns8x.empty()) {
			whole_char_size = std::max(whole_char_size, ScannedPattern8x::Chunk::CapacitySize);
		}
		if (!_patterns16x.empty()) {
			whole_char_size = std::max(whole_char_size, ScannedPattern16x::Chunk::CapacitySize);
		}
		if (!_patterns8.empty()) {
			whole_char_size = std::max(whole_char_size, ScannedPattern8::Chunk::CapacitySize);
		}
		if (!_patterns16.empty()) {
			whole_char_size = std::max(whole_char_size, ScannedPattern16::Chunk::CapacitySize);
		}
		if (!_patterns32.empty()) {
			whole_char_size = std::max(whole_char_size, ScannedPattern32::Chunk::CapacitySize);
		}
		_look_behind+= whole_char_size;
	}
	if (!_patterns32.empty()) {
		_look_behind = AlignUp(_look_behind, 4);
	} else if (!_patterns16.empty() || !_patterns16x.empty()) {
		_look_behind = AlignUp(_look_behind, 2);
	}

	fprintf(stderr, "FindPattern::GetReady: 8x:%lu 16x:%lu 8:%lu 16:%lu 32:%lu MPS=%lu LB=%lu\n",
			_patterns8x.size(), _patterns16x.size(),
			_patterns8.size(), _patterns16.size(), _patterns32.size(),
			_min_pattern_size, _look_behind);
}

/** Helper function used if MatchPattern found match, its goal is to go back and find starting position of
  * matched substring. MatchPattern could do remembering itself, but this would defeat its performance.
  */
template <class PatternT, class ELEMENT_T>
	std::pair<size_t, size_t> LookupMatchedRange(const PatternT &pattern, const ELEMENT_T *data, size_t edge)
{
	size_t len = 0;
	// go back in pattern elements sequence finding matched length to do next back step until reaching pattern start
	for (size_t chain_pos = pattern.chain.size(); chain_pos; ) {
		--chain_pos;
		const auto &chunk = pattern.chain[chain_pos];
		if (len + chunk.MinLen() <= edge && chunk.Match(data + edge - len - chunk.MinLen(), chunk.MinLen())) {
			len+= chunk.MinLen();

		} else if (len + chunk.MaxLen() <= edge && chunk.Match(data + edge - len - chunk.MaxLen(), chunk.MaxLen())) {
			len+= chunk.MaxLen();

		} else {
			fprintf(stderr, "LookupMatchedRange: fixme\n");
			abort();
		}
	}

	return std::make_pair(edge - len, edge); //  [start, edge) so caller will know exact range of matched substring
}

/** Actual finder routine. It sequentially goes through data array comparing current element with currently
  * selected pattern's chunk. Note that chunk may represent more than single element so in case of match
  * data iterated forward by all matched elements and pattern iterated to next chunk. In case of mismatch
  * one of two actions could be taken, depending on chunk position in pattern:
  *  - if chunk is at the head of pattern - then data array iterated forward by single element
  *  - otherwise then pattern is 'rewinded' to pre-defined during pattern creation point
  */
template <bool WITH_ALT, class PatternT, class ELEMENT_T>
	std::pair<size_t, size_t> MatchPattern(const PatternT &pattern, const ELEMENT_T *data, size_t len)
{
	size_t chain_pos = 0;
	for (size_t i = 0; i != len; ) {
		size_t match = WITH_ALT
			? pattern.chain[chain_pos].Match(data + i, len - i)
			: pattern.chain[chain_pos].MatchBase(data + i, len - i);
		if (!match) {
			if (chain_pos != 0) {
				chain_pos = pattern.chain[chain_pos].RewindPos();
			} else {
				++i;
			}
		} else {
			++chain_pos;
			i+= match;
			if (UNLIKELY(chain_pos == pattern.chain.size())) {
				return LookupMatchedRange(pattern, data, i);
			}
		}
	}
	return std::make_pair((size_t)-1, (size_t)-1);
}

// Проверяем символ на принадлежность разделителям слов
static bool IsWordDiv(const wchar_t symbol)
{
	// Так же разделителем является конец строки и пробельные символы
	return !symbol || IsSpace(symbol) || IsEol(symbol) || IsWordDiv(Opt.strWordDiv,symbol);
}

template <class ELEMENT_T, class PatternsT>
	std::pair<size_t, size_t> MatchPatterns(const PatternsT &patterns, const void *data, size_t len, bool whole_words, bool first_fragment, bool last_fragment, bool case_sensitive)
{
	for (const auto &p : patterns) {
		for (size_t ofs = 0;;) {
			const auto &r = case_sensitive
					? MatchPattern<false>(p, (const ELEMENT_T *)data + ofs, len / sizeof(ELEMENT_T) - ofs)
					: MatchPattern<true>(p, (const ELEMENT_T *)data + ofs, len / sizeof(ELEMENT_T) - ofs);
			if (r.first == (size_t)-1) {
				break;
			}
			if (!whole_words)
				return r;

			const bool left_ok = (first_fragment && ofs + r.first == 0)
				|| (ofs + r.first != 0 && IsWordDiv( ((const ELEMENT_T *)data)[ofs + r.first - 1]));

			if (left_ok) {
				const bool right_ok = (last_fragment && ofs + r.second >= len)
					|| (ofs + r.second < len && IsWordDiv( ((const ELEMENT_T *)data)[ofs + r.second]));
				if (right_ok) {
					return r;
				}
			}

			ofs+= r.second;
		}
	}
	return std::make_pair((size_t)-1, (size_t)-1);
}

std::pair<size_t, size_t> FindPattern::Match(const void *data, size_t len, bool first_fragment, bool last_fragment)
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
