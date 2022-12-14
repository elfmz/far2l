#pragma once
#include <vector>
#include <stdint.h>

template <class CodeUnitT, size_t MAX_CODEUNITS>
	struct ScannedPattern
{
	class CodePoint
	{ // represents single code point (grapheme), can be fixed (e.g. UTF32) or variable (e.g. UTF8) size
		uint16_t _rewind_pos;
		unsigned char _base_cnt;       // nonzero count of code units used for 'base' representation
		unsigned char _alt_cnt;        // can be zero meaning no alternative case representation
		CodeUnitT _base[MAX_CODEUNITS];// base representation of code point
		CodeUnitT _alt[MAX_CODEUNITS]; // optional alternative case representation of code point

		static bool MatchNonEmptyData(const CodeUnitT *left, const CodeUnitT *right, unsigned char len) noexcept;

	public:
		static constexpr size_t CodeUnitSize = sizeof(CodeUnitT);

		// separate Setup() method instead of c-tor resulted in smaller code
		void Setup(const CodeUnitT *base_data, size_t base_cnt, const CodeUnitT *alt_data, size_t alt_cnt)
		{
			if (base_cnt != 0 && base_cnt <= MAX_CODEUNITS && alt_cnt <= MAX_CODEUNITS) {
				_rewind_pos = 0;
				_base_cnt = base_cnt;
				_alt_cnt = alt_cnt;
				memcpy(_base, base_data, base_cnt * CodeUnitSize);
				memcpy(_alt, alt_data, alt_cnt * CodeUnitSize);

			} else {
				ThrowPrintf("CodePoint<%ld>: bad base_cnt=%ld alt_cnt=%ld", MAX_CODEUNITS, base_cnt, alt_cnt);
			}
		}

		bool operator ==(const CodePoint &other) const noexcept
		{
			// _rewind_pos ignored intentionally to allow
			// AddUniqPattern and Sequence::EqualParts to work correctly
			return _base_cnt == other._base_cnt && MatchNonEmptyData(_base, other._base, _base_cnt)
				&& _alt_cnt == other._alt_cnt
				&& (_alt_cnt == 0 || MatchNonEmptyData(_alt, other._alt, _alt_cnt));
		}

		inline bool operator !=(const CodePoint &other) const noexcept
		{
			return !operator ==(other);
		}

		inline size_t Match(const CodeUnitT *data, size_t len) const noexcept
		{
			return (len >= _base_cnt && MatchNonEmptyData(data, _base, _base_cnt))
				? _base_cnt
				: ((_alt_cnt && len >= _alt_cnt && MatchNonEmptyData(data, _alt, _alt_cnt)) ? _alt_cnt : 0);
		}

		inline size_t MatchOnlyBase(const CodeUnitT *data, size_t len) const noexcept
		{
			return (len >= _base_cnt && MatchNonEmptyData(data, _base, _base_cnt)) ? _base_cnt : 0;
		}

		inline size_t RewindPos() const noexcept
		{
			return _rewind_pos;
		}

		void SetRewindPos(size_t rewind_pos)
		{
			_rewind_pos = rewind_pos;
			if (_rewind_pos != rewind_pos) {
				ThrowPrintf("too distant rewind_pos");
			}
		}

		size_t MinCnt() const noexcept
		{
			return (_alt_cnt && _alt_cnt < _base_cnt) ? _alt_cnt : _base_cnt;
		}

		size_t MaxCnt() const noexcept
		{
			return (_alt_cnt > _base_cnt) ? _alt_cnt : _base_cnt;
		}

		size_t MinSize() const noexcept
		{
			return MinCnt() * CodeUnitSize;
		}

		size_t MaxSize() const noexcept
		{
			return MaxCnt() * CodeUnitSize;
		}
	};

	struct Sequence : std::vector<CodePoint>
	{	// all this methods used only in init so don't inline them to make code smaller
		void FN_NOINLINE EmplaceCodePoint(
			const CodeUnitT *base_data, size_t base_cnt,
			const CodeUnitT *alt_data, size_t alt_cnt)
		{
			std::vector<CodePoint>::emplace_back();
			std::vector<CodePoint>::back().Setup(base_data, base_cnt, alt_data, alt_cnt);
		}

		static bool FN_NOINLINE EqualRanges(
			typename std::vector<CodePoint>::const_iterator start1,
			typename std::vector<CodePoint>::const_iterator start2,
			size_t count) noexcept
		{
			return std::equal(start1, start1 + count, start2);
		}

		bool EqualParts(size_t start1, size_t start2, size_t count) const noexcept
		{
			return EqualRanges(
				std::vector<CodePoint>::begin() + start1,
				std::vector<CodePoint>::begin() + start2,
				count
			);
		}
	} seq;

	bool operator ==(const ScannedPattern &other) const noexcept
	{
		return other.seq.size() == seq.size()
			&& Sequence::EqualRanges(seq.begin(), other.seq.begin(), seq.size());
	}

	bool operator !=(const ScannedPattern &other) const noexcept
	{
		return !operator ==(other);
	}
};

typedef ScannedPattern<uint8_t, 6> ScannedPattern8x;
typedef ScannedPattern<uint16_t, 2> ScannedPattern16x;
typedef ScannedPattern<uint8_t, 1> ScannedPattern8;
typedef ScannedPattern<uint16_t, 1> ScannedPattern16;
typedef ScannedPattern<uint32_t, 1> ScannedPattern32;

class FindPattern
{
	bool _case_sensitive{false};
	bool _whole_words{false};

	size_t _min_pattern_size{0};
	size_t _look_behind{0};

	std::vector<ScannedPattern8x>    _patterns8x;  // multibyte UTF7, UTF8
	std::vector<ScannedPattern16x>   _patterns16x; // multiword UTF16
	std::vector<ScannedPattern8>     _patterns8;   // single byte encodings and single byte UTF7, UTF8
	std::vector<ScannedPattern16>    _patterns16;  // double-byte encodings and single word UTF16
	std::vector<ScannedPattern32>    _patterns32;  // UTF32

	template <class Patterns>
		void AccontMinimalAndLookBehindSizesFor(Patterns &patterns) noexcept
	{
		size_t min_pattern_size = 0, look_behind = 0;
		for (const auto &pattern : patterns) {
			for (const auto &code_point : pattern.seq) {
				min_pattern_size+= code_point.MinSize();
				look_behind+= code_point.MaxSize();
			}
		}
		if (look_behind >= Patterns::value_type::CodePoint::CodeUnitSize) {
			look_behind-= Patterns::value_type::CodePoint::CodeUnitSize;
		}
		_min_pattern_size = std::min(_min_pattern_size, min_pattern_size);
		_look_behind = std::max(_look_behind, look_behind);
	}

public:
	FindPattern(bool case_sensitive, bool whole_words);

	void AddBytesPattern(const uint8_t *pattern, size_t len);
	void AddTextPattern(const wchar_t *pattern, unsigned int codepage);

	/** Call this once after all patterns added but before using any other method below.
	  */
	void GetReady();

	/** Minimal size (in bytes) of content that can match any of added pattern.
	  */
	inline size_t MinPatternSize() const noexcept { return _min_pattern_size; }

	/** Size (in bytes) of substring that should be prepended from tail of previous scan window
	  * to ensure matching substring that happen to be crossed by scan windows boundary.
	  * Returned value guaranteed to be aligned by size of largest searched codepoint.
	  */
	inline size_t LookBehind() const noexcept { return _look_behind; }

	/** Searches given data array for substring matching any of added pattern.
	  * Data array pointer expected to be aligned by size of largest searched codepoint.
	  * Following arguments needed by whole_words to correctly treat edging scan windows:
	  *  first_fragment indicates if this is very first scan window of actually checked content.
	  *  last_fragment indicates if this is very last scan window of actually checked content.
	  * Returns {start, len} of matching region or {-1, 0} if no match found.
	  */
	std::pair<size_t, size_t> Match(const void *data, size_t len, bool first_fragment, bool last_fragment) const noexcept;
};
