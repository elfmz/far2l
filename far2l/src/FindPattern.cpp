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

template <class CodeUnitT, size_t MAX_CODEUNITS>
	class CodePoint
{ // represents single code point (grapheme), can be fixed (e.g. UTF32) or variable (e.g. UTF8) size
	uint16_t _rew;                 // where matcher routine will seek back if this codepoint not matched
	unsigned char _base_cnt;       // nonzero count of code units used for 'base' representation
	unsigned char _alt_cnt;        // can be zero meaning no alternative case representation
	CodeUnitT _base[MAX_CODEUNITS];// base representation of code point
	CodeUnitT _alt[MAX_CODEUNITS]; // optional alternative case representation of code point

	static bool MatchNonEmptyData(const CodeUnitT *left, const CodeUnitT *right, unsigned char len) noexcept;

public:
	typedef CodeUnitT CodeUnit;
	static constexpr size_t MaxCodeUnits = MAX_CODEUNITS;
	static constexpr size_t CodeUnitSize = sizeof(CodeUnitT);

	// separate Setup() method instead of c-tor results in smaller code
	void Setup(size_t rew, const CodeUnitT *base_data, size_t base_cnt, const CodeUnitT *alt_data, size_t alt_cnt)
	{
		if (base_cnt > 0 && base_cnt <= MAX_CODEUNITS && alt_cnt <= MAX_CODEUNITS) {
			_rew = rew;
			_base_cnt = base_cnt;
			_alt_cnt = alt_cnt;
			if (MAX_CODEUNITS == 1) {
				_base[0] = *base_data;
				if (alt_cnt) {
					_alt[0] = *alt_data;
				}
			} else {
				memcpy(_base, base_data, base_cnt * sizeof(CodeUnitT));
				memcpy(_alt, alt_data, alt_cnt * sizeof(CodeUnitT));
			}

		} else {
			ThrowPrintf("CodePoint<%ld>: bad base_cnt=%ld alt_cnt=%ld", MAX_CODEUNITS, base_cnt, alt_cnt);
		}
	}

	bool FN_NOINLINE SameAs(const CodePoint<CodeUnitT, MAX_CODEUNITS> &other) const noexcept
	{
		if (MAX_CODEUNITS == 1) {
			return _base_cnt == other._base_cnt && _alt_cnt == other._alt_cnt
				&& _base[0] == other._base[0]
				&& (_alt_cnt == 0 || _alt[0] == other._alt[0]);

		}
		return _base_cnt == other._base_cnt && _alt_cnt == other._alt_cnt
			&& memcmp(_base, other._base, _base_cnt * sizeof(CodeUnitT)) == 0
			&& (_alt_cnt == 0 || memcmp(_alt, other._alt, _alt_cnt * sizeof(CodeUnitT)) == 0);
	}

	inline size_t Rewind() const noexcept
	{
		return _rew;
	}

	void SetRewind(size_t rew) noexcept
	{
		_rew = rew;
	}

	size_t MinCnt() const noexcept
	{
		if (MAX_CODEUNITS == 1) {
			return 1;
		}
		return _alt_cnt ? std::min(_base_cnt, _alt_cnt) : _base_cnt;
	}

	size_t MaxCnt() const noexcept
	{
		if (MAX_CODEUNITS == 1) {
			return 1;
		}
		return std::max(_base_cnt, _alt_cnt);
	}

	inline size_t MatchOnlyBase(const CodeUnitT *data, size_t len) const noexcept
	{
		if (MAX_CODEUNITS == 1) {
			return (LIKELY(len) && data[0] == _base[0]) ? 1 : 0;
		}
		return (LIKELY(len >= _base_cnt) && MatchNonEmptyData(data, _base, _base_cnt)) ? _base_cnt : 0;
	}

	inline size_t Match(const CodeUnitT *data, size_t len) const noexcept
	{
		if (MAX_CODEUNITS == 1) {
			return (LIKELY(len) && (data[0] == _base[0] || (_alt_cnt && data[0] == _alt[0]))) ? 1 : 0;
		}
		return (LIKELY(len >= _base_cnt) && MatchNonEmptyData(data, _base, _base_cnt))
			? _base_cnt
			: (_alt_cnt && LIKELY(len >= _alt_cnt) && MatchNonEmptyData(data, _alt, _alt_cnt) ? _alt_cnt : 0);
	}
};

template <> inline bool CodePoint<uint8_t, 6>::MatchNonEmptyData(const uint8_t *left, const uint8_t *right, unsigned char len) noexcept
{
	return left[0] == right[0]
		&& (len < 2 || left[1] == right[1])
		&& (len < 3 || left[2] == right[2])
		&& (len < 4 || left[3] == right[3])
		&& (len < 5 || left[4] == right[4])
		&& (len < 6 || left[5] == right[5]);
}

template <> inline bool CodePoint<uint16_t, 2>::MatchNonEmptyData(const uint16_t *left, const uint16_t *right, unsigned char len) noexcept
{
	return left[0] == right[0]
		&& (len < 2 || left[1] == right[1]);
}

template <> inline bool CodePoint<uint32_t, 1>::MatchNonEmptyData(const uint32_t *left, const uint32_t *right, unsigned char len) noexcept
{
	return left[0] == right[0];
}

template <> inline bool CodePoint<uint16_t, 1>::MatchNonEmptyData(const uint16_t *left, const uint16_t *right, unsigned char len) noexcept
{
	return left[0] == right[0];
}

template <> inline bool CodePoint<uint8_t, 1>::MatchNonEmptyData(const uint8_t *left, const uint8_t *right, unsigned char len) noexcept
{
	return left[0] == right[0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct IScannedPattern
{
	struct Metrics
	{
		size_t code_unit{0};
		size_t min_pattern{0};
		size_t max_pattern{0};
	};
	virtual ~IScannedPattern() = default;
	virtual void GetReady() = 0;
	virtual const Metrics &GetMetrics() const noexcept = 0;
	virtual size_t GetCapacity() const noexcept = 0;
	virtual std::pair<size_t, size_t> FindMatch(const void *begin, size_t len, bool first_fragment, bool last_fragment) const noexcept = 0;
	virtual void AppendCodePoint(const void *base, size_t base_size, const void *alt, size_t alt_size) = 0;

	// used to check for duplicated patterns
	virtual bool SameAs(const IScannedPattern *other) const noexcept = 0;
	virtual uint64_t Summary() const noexcept = 0;
};

template <class CodePointT>
	class ScannedPattern : public IScannedPattern
{
	typedef typename CodePointT::CodeUnit CodeUnit;
	std::vector<CodePointT> _seq;
	Metrics _metrics;
	bool _case_sensitive;
	bool _whole_words;
	bool _foreign_endian;

	virtual bool SameAs(const IScannedPattern *other) const noexcept
	{
		if (Summary() != other->Summary()) {
			return false;
		}

		const ScannedPattern<CodePointT> *sibling = (const ScannedPattern<CodePointT> *)other;
		for (size_t i = 0; i != _seq.size(); ++i) {
			if (!_seq[i].SameAs(sibling->_seq[i])) {
				return false;
			}
		}

		return true;
	}

	virtual uint64_t Summary() const noexcept
	{
		return uint64_t(sizeof(CodeUnit) | (CodePointT::MaxCodeUnits << 8) | (_seq.size() << 16));
	}

	virtual const Metrics &GetMetrics() const noexcept
	{
		return _metrics;
	}

	virtual size_t GetCapacity() const noexcept
	{
		return CodePointT::MaxCodeUnits * CodePointT::CodeUnitSize;
	}

	virtual void AppendCodePoint(const void *base, size_t base_size, const void *alt, size_t alt_size)
	{
		_seq.emplace_back();
		_seq.back().Setup(_seq.size() - 1,
			(const CodeUnit *)base, base_size / sizeof(CodeUnit),
			(const CodeUnit *)alt, alt_size / sizeof(CodeUnit));
	}

	virtual void GetReady()
	{
		if (_seq.size() > std::numeric_limits<uint16_t>::max()) {
			ThrowPrintf("too many codepoints");
		}
		/**
			This is a most tricky code here:
			go through just added seq and for each non-heading codepoint assign 'rewind' position
			that specifies where MatchPattern() routine will seek back in pattern seq in case of
			mismatching this particular codepoint while all previous chunks where recently matched.
			Using correct rewind value ensures finding matches in cases like:
			{'ac' IN 'aac'}  {'abc' IN 'ababc'} {'bdbdba' IN 'bdbdbdba'} etc
		*/
		for (size_t i = 2; i < _seq.size(); ++i) {
			for (size_t j = i - 1; j >= 1; --j) {
				if (SameParts(0, i - j, j)) {
					size_t n = 2;
					for (;j * n < i; ++n) {
						if (!SameParts(0, j * (n - 1), j)
						 || !SameParts(0, (i - j * n), j)) {
							break;
						}
					}
					_seq[i].SetRewind(i - j * (n - 1));
					break;
				}
			}
		}

		_metrics.code_unit = sizeof(CodeUnit);
		for (const auto &code_point : _seq) {
			_metrics.min_pattern+= code_point.MinCnt();
			_metrics.max_pattern+= code_point.MaxCnt();
		}
	}

	bool FN_NOINLINE SameParts(size_t start1, size_t start2, size_t count) const noexcept
	{
		//ASSERT(std::max(start1, start2) + count <= _seq.size());
		for (size_t i = 0; i != count; ++i) {
			if (!_seq[start1 + i].SameAs(_seq[start2 + i])) {
				return false;
			}
		}
		return true;
	}

	// Проверяем символ на принадлежность разделителям слов
	bool IsCodeUnitDiv(CodeUnit cu) const noexcept
	{
		wchar_t symbol = _foreign_endian ? RevBytes(cu) : cu;
		// Так же разделителем является конец строки и пробельные символы
		return !symbol || IsSpace(symbol) || IsEol(symbol) || IsWordDiv(Opt.strWordDiv, symbol);
	}

	/**
		Helper function used if FindMatchCaseSpecific found match, its goal is to go back and find starting position
		of matched substring. FindMatchCaseSpecific could do remembering itself, but this would defeat its performance.
	*/
	size_t FN_NOINLINE LookbackMatchedRange(const CodeUnit *data) const noexcept
	{// go back in pattern elements sequence finding matched length to do next back step until reaching pattern start
		size_t len = 0;
		for (auto seq_it = _seq.rbegin(); seq_it != _seq.rend(); ++seq_it) {
			if (seq_it->Match(data - len - seq_it->MinCnt(), seq_it->MinCnt())) {
				len+= seq_it->MinCnt();

			} else if (LIKELY(seq_it->Match(data - len - seq_it->MaxCnt(), seq_it->MaxCnt()))) {
				len+= seq_it->MaxCnt();

			} else {
				ABORT();
			}
		}
		return len;
	}

	/**
		Actual finder routine. It sequentially goes through data array comparing current element with currently
		selected pattern's codepoint. Note that codepoint may represent more than single codeunit so if such
		codepoint matched then all matched codeunits skipped on data array and pattern iterated to next codepoint.
		In case of mismatch one of two actions could be taken, depending on codepoint position in pattern:
		- if codepoint is at the head of pattern - then data array iterated forward by single codeunit.
		- otherwise pattern is 'rewinded' to position pre-calculated during pattern initialization.
		Returns zero if match was not found or count of matched codeunits if match was found and also
		data then adjusted to past-matched substring code unit.
	*/
	template <bool CASE_SENSITIVE>
		inline size_t FindMatchCaseSpecific(const CodeUnit *&data, const CodeUnit *end) const noexcept
	{
		const auto seq_end = _seq.end();
		auto seq_it = _seq.begin();
		for (const CodeUnit *cur = data; LIKELY(cur != end); ) {
			const size_t match = CASE_SENSITIVE
				? seq_it->MatchOnlyBase(cur, end - cur)
				: seq_it->Match(cur, end - cur);
			if (match) {
				cur+= match;
				++seq_it;
				if (UNLIKELY(seq_it == seq_end)) {
					data = cur;
					return LookbackMatchedRange(cur);
				}
			} else if (UNLIKELY(seq_it->Rewind())) {
				seq_it-= seq_it->Rewind();
			} else {
				++cur;
			}
		}
		return 0;
	}

	virtual std::pair<size_t, size_t> FindMatch(const void *begin, size_t len, bool first_fragment, bool last_fragment) const noexcept
	{
		const CodeUnit *cu_data = (const CodeUnit *)begin; // already aligned
		const CodeUnit *cu_end = cu_data + len / sizeof(CodeUnit);
		size_t r;
		for (;;) {
			r = _case_sensitive
				? FindMatchCaseSpecific<true>(cu_data, cu_end)
				: FindMatchCaseSpecific<false>(cu_data, cu_end);
			if (!r) {
				return std::make_pair((size_t)-1, 0);
			}
			if (!_whole_words) {
				break;
			}
			// cu_data now points to the end of matched sequence
			// r represents length of matched sequence that precedes cu_data
			// Is matched sequence surrounded by 'div' code units or content's edges?
			const bool left_at_begin = (cu_data - r == (const CodeUnit *)begin);
			const bool left_div = (left_at_begin && first_fragment)
				|| (!left_at_begin && IsCodeUnitDiv(*(cu_data - r - 1)));
			if (left_div) {
				const bool right_at_end = (cu_data == cu_end);
				const bool right_div = (right_at_end && last_fragment)
					|| (!right_at_end && IsCodeUnitDiv(*cu_data));
				if (right_div) {
					break;
				}
			}
		}

		return std::make_pair(
			size_t((const char *)(cu_data - r) - (const char *)begin),
			r * sizeof(CodeUnit));
	}

public:
	ScannedPattern(bool case_sensitive, bool whole_words, bool foreign_endian)
		: _case_sensitive(case_sensitive), _whole_words(whole_words), _foreign_endian(foreign_endian)
	{
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////

FindPattern::FindPattern(bool case_sensitive, bool whole_words)
	: _case_sensitive(case_sensitive), _whole_words(whole_words)
{
}

FindPattern::~FindPattern()
{
}

struct CodePointConvertor
{
	unsigned int base_size{0}, alt_size{0};
	char base[32], alt[32];

	CodePointConvertor(wchar_t wc, unsigned int codepage, bool case_sensitive)
	{
		BOOL used_def_chr{FALSE};
		PBOOL p_used_def_chr = (!IsUTF7(codepage) && !IsUTF8(codepage) && !IsUTF16(codepage) && !IsUTF32(codepage)) ? &used_def_chr : nullptr;

		if (!case_sensitive) {
			wchar_t alt_wc = wc;
			WINPORT(CharUpperBuff)(&alt_wc, 1);
			if (alt_wc == wc) {
				WINPORT(CharLowerBuff)(&alt_wc, 1);
				if (alt_wc != wc) { // force base data be lowercase to keep base/alt consistency
					std::swap(wc, alt_wc);
				}
			}
			if (alt_wc != wc) {
				int r = WINPORT(WideCharToMultiByte)(codepage, 0, &alt_wc, 1, alt, sizeof(alt), nullptr, p_used_def_chr);
				if (r <= 0 || used_def_chr) { // dont fail yet, cuz still may proceed with lowercase base
					fprintf(stderr, "%s: failed to covert to cp=%d alt wc=0x%x '%lc'\n", __FUNCTION__, codepage, alt_wc, alt_wc);
					used_def_chr = FALSE;

				} else {
					alt_size = r;
				}
			}
		}

		int r = WINPORT(WideCharToMultiByte)(codepage, 0, &wc, 1, base, sizeof(base), nullptr, p_used_def_chr);
		if (r <= 0 || used_def_chr) {
			if (alt_size == 0) { // no codepoint representation at all -> bail out
				ThrowPrintf("failed to covert to cp=%d wc=0x%x '%lc'", codepage, wc, wc);
			}
			// if uppercase was succeeded then use it as base
			memcpy(base, alt, alt_size);
			base_size = alt_size;
			alt_size = 0;

		} else {
			base_size = r;
		}
	}
};

class PatternGenerator
{
	// in case of UTF7/8/16 encodings two options are possible:
	// if only short codepoints specified (i.e. latin-only UTF8) then use single-byte sequence
	// but if there're 'complex' codepoints - then use UTF-specific 'long' sequence
	ScannedPatternPtr _new_p, _long_new_p;
	unsigned int _codepage;
	int _max_char_size;
	bool _case_sensitive;

public:
	PatternGenerator(unsigned int codepage, int max_char_size, bool case_sensitive, bool whole_words)
		: _codepage(codepage), _max_char_size(max_char_size), _case_sensitive(case_sensitive)
	{
		switch (_codepage) {
			case CP_UTF7: case CP_UTF8:
				_new_p.reset(new ScannedPattern<CodePoint<uint8_t, 1>>(case_sensitive, whole_words, false));
				_long_new_p.reset(new ScannedPattern<CodePoint<uint8_t, 6>>(case_sensitive, whole_words, false));
				break;

			case CP_UTF16BE: case CP_UTF16LE:
				_new_p.reset(new ScannedPattern<CodePoint<uint16_t, 1>>(case_sensitive, whole_words, _codepage == CP_UTF16BE));
				_long_new_p.reset(new ScannedPattern<CodePoint<uint16_t, 2>>(case_sensitive, whole_words, _codepage == CP_UTF16BE));
				break;

			default: switch(_max_char_size) {
				case 1: _new_p.reset(new ScannedPattern<CodePoint<uint8_t, 1>>(case_sensitive, whole_words, false)); break;
				case 2: _new_p.reset(new ScannedPattern<CodePoint<uint16_t, 1>>(case_sensitive, whole_words, false)); break;
				case 4: _new_p.reset(new ScannedPattern<CodePoint<uint32_t, 1>>(case_sensitive, whole_words, _codepage == CP_UTF32BE)); break;
				default:
					ThrowPrintf("%s: bad max_char_size=%d for codepage=%d\n", __FUNCTION__, max_char_size, codepage);
			}
		}
	}

	void AppendCodePoint(wchar_t wc)
	{
		CodePointConvertor cpc(wc, _codepage, _case_sensitive);
		if (_long_new_p) {
			_long_new_p->AppendCodePoint(cpc.base, cpc.base_size, cpc.alt, cpc.alt_size);
			if (_new_p && (_new_p->GetCapacity() < cpc.base_size || _new_p->GetCapacity() < cpc.alt_size)) {
				_new_p.reset();
			}
		}
		if (_new_p) {
			_new_p->AppendCodePoint(cpc.base, cpc.base_size, cpc.alt, cpc.alt_size);
		}
	}

	ScannedPatternPtr Finalize()
	{
		return _new_p ? std::move(_new_p) : std::move(_long_new_p);
	}
};

void FindPattern::AddTextPattern(const wchar_t *pattern, unsigned int codepage)
{
	if (!*pattern) {
		ThrowPrintf("empty text pattern");
	}

	CPINFO cpi{};
	if (!WINPORT(GetCPInfo)(codepage, &cpi)) {
		ThrowPrintf("bad codepage=%u", codepage);
	}

	PatternGenerator pg(codepage, cpi.MaxCharSize, _case_sensitive, _whole_words);
	for (;*pattern; ++pattern) {
		pg.AppendCodePoint(*pattern);
	}
	AddPattern(pg.Finalize());
}

void FindPattern::AddBytesPattern(const uint8_t *pattern, size_t len)
{
	if (len == 0) {
		ThrowPrintf("empty bytes pattern");
	}

	ScannedPatternPtr new_p(new ScannedPattern<CodePoint<uint8_t, 1>>(false, false, false));
	for (size_t i = 0; i < len; ++i) {
		new_p->AppendCodePoint(&pattern[i], 1, nullptr, 0);
	}

	AddPattern(std::move(new_p));
}

void FindPattern::AddPattern(ScannedPatternPtr &&new_p)
{
	for (const auto &p : _patterns) {
		if (p->SameAs(new_p.get())) {
			return;
		}
	}
	new_p->GetReady();
	_patterns.emplace_back(std::move(new_p));
}

void FindPattern::GetReady()
{
	_min_pattern_size = std::numeric_limits<size_t>::max();
	_look_behind = 0;
	size_t max_code_unit = 1;
	for (const auto &pp : _patterns) {
		const auto &m = pp->GetMetrics();
		max_code_unit = std::max(max_code_unit, m.code_unit);
		_min_pattern_size = std::min(_min_pattern_size, m.min_pattern * m.code_unit);
		_look_behind = std::max(_look_behind,
			(m.max_pattern > 1) ? (m.max_pattern - 1) * m.code_unit : 0);
	}
	_look_behind = AlignUp(_look_behind, max_code_unit);

	fprintf(stderr, "FindPattern::GetReady: count:%lu MPS=%lu LB=%lu\n",
		_patterns.size(), _min_pattern_size, _look_behind);

	if (_patterns.empty()) {
		ThrowPrintf("no patterns defined");
	}
}

std::pair<size_t, size_t> FindPattern::FindMatch(const void *data, size_t len, bool first_fragment, bool last_fragment) const noexcept
{
	for (const auto &pattern : _patterns) {
		const auto &r = pattern->FindMatch(data, len, first_fragment, last_fragment);
		if (r.second) {
			return r;
		}
	}
	return std::make_pair((size_t)-1, 0);
}
