#include "headers.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <limits>
#include "FindPattern.hpp"
#include "strmix.hpp"
#include "config.hpp"
#include "codepage.hpp"

FindPattern::FindPattern(bool case_sensitive, bool whole_words)
	: _case_sensitive(case_sensitive), _whole_words(whole_words)
{
}

template <> bool ScannedPattern<uint8_t, 6>::Chunk::Elements::NonEmptyMatchData(const uint8_t *data, unsigned char len) const
{
	switch (len) {
		case 6: if (data[5] != contained[5]) return false;
		case 5: if (data[4] != contained[4]) return false;
		case 4: if (data[3] != contained[3]) return false;
		case 3: if (data[2] != contained[2]) return false;
		case 2: if (data[1] != contained[1]) return false;
//		case 1: if (data[0] != (*this)[0]) return false;
	}

	return (data[0] == contained[0]);
}

template <> bool ScannedPattern<uint16_t, 2>::Chunk::Elements::NonEmptyMatchData(const uint16_t *data, unsigned char len) const
{
	switch (len) {
		case 2: if (data[1] != contained[1]) return false;
//		case 1: if (data[0] != (*this)[0]) return false;
	}
	return (data[0] == contained[0]);
}

template <> bool ScannedPattern<uint32_t, 1>::Chunk::Elements::NonEmptyMatchData(const uint32_t *data, unsigned char len) const
{
	return data[0] == contained[0];
}

template <> bool ScannedPattern<uint16_t, 1>::Chunk::Elements::NonEmptyMatchData(const uint16_t *data, unsigned char len) const
{
	return data[0] == contained[0];
}

template <> bool ScannedPattern<uint8_t, 1>::Chunk::Elements::NonEmptyMatchData(const uint8_t *data, unsigned char len) const
{
	return data[0] == contained[0];
}

////////////////

void FindPattern::AddBytesPattern(const uint8_t *pattern, size_t len)
{
	std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> chain_src;
	while (--len) {
		chain_src.emplace_back();
		chain_src.back().first.emplace_back(*(pattern++));
	}
	_patterns8.emplace_back(chain_src);
}

bool FindPattern::AddTextPattern(const wchar_t *pattern, unsigned int codepage)
{
	std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> chain_src8;
	std::vector<std::pair<std::vector<uint16_t>, std::vector<uint16_t>>> chain_src16;
	std::vector<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>> chain_src32;

	CPINFO cpi{};
	if (codepage != CP_UTF8 && codepage != CP_UTF7
	  && (!WINPORT(GetCPInfo)(codepage, &cpi) || (cpi.MaxCharSize != 1 && cpi.MaxCharSize != 2 && cpi.MaxCharSize != 4))
		) {
		fprintf(stderr, "%s: bad codepage=%u MaxCharSize=%u\n", __FUNCTION__, codepage, cpi.MaxCharSize);
		return false;
	}

	size_t utf8_sizes[6] = {0};
	BOOL used_def_chr = FALSE;
	PBOOL p_used_def_chr = (!IsUTF7(codepage) && !IsUTF8(codepage) && !IsUTF16(codepage) && !IsUTF32(codepage)) ? &used_def_chr : nullptr;

	for (;*pattern; ++pattern) {
		union U {
			char u8[32];
			uint16_t u16[16];
			uint32_t u32[8];
		} base, alt;

		int alt_r = 0;
		int r = WINPORT(WideCharToMultiByte)(codepage, 0, pattern, 1, base.u8, ARRAYSIZE(base.u8), nullptr, p_used_def_chr);
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
				alt_r = WINPORT(WideCharToMultiByte)(codepage, 0, tmp, 1, alt.u8, ARRAYSIZE(alt.u8), nullptr, p_used_def_chr);
				if (alt_r <= 0 || used_def_chr) {
					fprintf(stderr, "%s: failed to covert to cp=%d alt wc=0x%x '%lc'\n", __FUNCTION__, codepage, *tmp, *tmp);
					// dont fail, cuz still may proceed with base bytes
				}
			}
		}

		if (codepage == CP_UTF8 || codepage == CP_UTF7 || cpi.MaxCharSize == 1) {
			chain_src8.emplace_back();
			chain_src8.back().first.insert( chain_src8.back().first.end(),
				&base.u8[0], (&base.u8[0] + r) );
			if (codepage == CP_UTF8 || codepage == CP_UTF7) {
				utf8_sizes[std::min((int)ARRAYSIZE(utf8_sizes) - 1, r - 1)]++;
			}
			if (alt_r > 0) {
				chain_src8.back().second.insert( chain_src8.back().second.end(),
					&alt.u8[0], (&alt.u8[0] + alt_r) );
				if (codepage == CP_UTF8 || codepage == CP_UTF7) {
					utf8_sizes[std::min((int)ARRAYSIZE(utf8_sizes) - 1, alt_r - 1)]++;
				}
			}
		} else {
			if ((r % cpi.MaxCharSize) != 0 || (alt_r % cpi.MaxCharSize) != 0) {
				fprintf(stderr, "%s: misaligned codepage=%u r=%d alt_r=%d MaxCharSize=%u\n",
					__FUNCTION__, codepage, r, alt_r, cpi.MaxCharSize);
				return false;
			}

			if (cpi.MaxCharSize == 2) {
				chain_src16.emplace_back();
				chain_src16.back().first.insert( chain_src16.back().first.end(),
					(&base.u16[0]), (&base.u16[0] + r / 2) );
				if (alt_r > 0) {
						chain_src16.back().second.insert( chain_src16.back().second.end(),
						(&alt.u16[0]), (&alt.u16[0] + alt_r / 2) );
				}
			} else {
				chain_src32.emplace_back();
				chain_src32.back().first.insert( chain_src32.back().first.end(),
					(&base.u32[0]), (&base.u32[0] + r / 4) );
				if (alt_r > 0) {
						chain_src32.back().second.insert( chain_src32.back().second.end(),
						(&alt.u32[0]), (&alt.u32[0] + alt_r / 4) );
				}
			}
		}
	}

	if (codepage == CP_UTF8 || codepage == CP_UTF7) {
		// optimization: if all chars of this UTF8 are single bytes - then add it to single-byte patterns
		if (!utf8_sizes[1] && !utf8_sizes[2] && !utf8_sizes[3] && !utf8_sizes[4] && !utf8_sizes[5]) {
			_patterns8.EmplaceUniq(chain_src8);
		} else {
			_patternsUTF8.EmplaceUniq(chain_src8);
		}

	} else if (codepage == CP_UTF16LE || codepage == CP_UTF16BE) {
		_patternsUTF16.EmplaceUniq(chain_src16);

	} else switch(cpi.MaxCharSize) {
		case 1:
			_patterns8.EmplaceUniq(chain_src8);
			break;

		case 2:
			_patterns16.EmplaceUniq(chain_src16);
			break;

		case 4:
			_patterns32.EmplaceUniq(chain_src32);
			break;
	}

	return true;
}


void FindPattern::GetReady()
{
	_min_pattern_size = std::numeric_limits<size_t>::max();
	_look_behind = 0;

	for (const auto &p : _patternsUTF8) {
		_min_pattern_size = std::min(_min_pattern_size, p.MinSize());
		_look_behind = std::max(_look_behind, p.LookBehind());
	}
	for (const auto &p : _patternsUTF16) {
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
		if (!_patternsUTF8.empty()) _look_behind+= 6;
		else if (!_patterns32.empty()) _look_behind+= 4;
		else if (!_patternsUTF16.empty()) _look_behind+= 4; // fetch two behind chars to ensure valid surrogate pair
		else if (!_patterns16.empty()) _look_behind+= 2;
		else if (!_patterns8.empty()) _look_behind+= 1;
	}
	if (!_patterns32.empty()) {
		_look_behind = AlignUp(_look_behind, 4);
	} else if (!_patterns16.empty() || !_patternsUTF16.empty()) {
		_look_behind = AlignUp(_look_behind, 2);
	}

	fprintf(stderr, "FindPattern::GetReady: UTF8:%lu UTF16:%lu B8:%lu B16:%lu B32:%lu _min_pattern_size=%lu _look_behind=%lu\n",
			_patternsUTF8.size(), _patternsUTF16.size(),
			_patterns8.size(), _patterns16.size(), _patterns32.size(),
			_min_pattern_size, _look_behind);
}

template <class PatternT, class ELEMENT_T>
	std::pair<size_t, size_t> LookupMatchedRange(const PatternT &pattern, const ELEMENT_T *data, size_t edge)
{
	size_t len = 0;
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

	return std::make_pair(edge - len, edge);
}

template <class PatternT, class ELEMENT_T>
	std::pair<size_t, size_t> MatchPattern(const PatternT &pattern, const ELEMENT_T *data, size_t len)
{
	size_t chain_pos = 0;
	for (size_t i = 0; i != len; ) {
		const size_t match = pattern.chain[chain_pos].Match(data + i, len - i);
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
	std::pair<size_t, size_t> MatchPatterns(const PatternsT &patterns, const void *data, size_t len, bool whole_words, bool first_fragment, bool last_fragment)
{
	for (const auto &p : patterns) {
		for (size_t ofs = 0;;) {
			const auto &r = MatchPattern(p, (const ELEMENT_T *)data + ofs, len / sizeof(ELEMENT_T) - ofs);
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
	auto r = MatchPatterns<uint8_t>(_patternsUTF8, data, len, _whole_words, first_fragment, last_fragment);
	if (r.first == (size_t)-1) {
		r = MatchPatterns<uint16_t>(_patternsUTF16, data, len, _whole_words, first_fragment, last_fragment);
	}
	if (r.first == (size_t)-1) {
		r = MatchPatterns<uint8_t>(_patterns8, data, len, _whole_words, first_fragment, last_fragment);
	}
	if (r.first == (size_t)-1) {
		r = MatchPatterns<uint16_t>(_patterns16, data, len, _whole_words, first_fragment, last_fragment);
	}
	if (r.first == (size_t)-1) {
		r = MatchPatterns<uint32_t>(_patterns32, data, len, _whole_words, first_fragment, last_fragment);
	}
	return r;
}
