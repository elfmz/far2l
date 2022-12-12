#pragma once
#include <vector>
#include <stdint.h>
#include <assert.h>

template <class ELEMENT_T, size_t MAX_ELEMENTS>
	struct ScannedPattern
{
	class Chunk
	{
		uint16_t _rewind_pos{0};
		unsigned char _base_len{0};     // never zero
		unsigned char _alt_len{0};      // can be zero meaning no alternative case representation
		ELEMENT_T _base[MAX_ELEMENTS];
		ELEMENT_T _alt[MAX_ELEMENTS];

		static bool MatchNonEmptyData(const ELEMENT_T *left, const ELEMENT_T *right, unsigned char len);

	public:
		static constexpr size_t CapacitySize = MAX_ELEMENTS * sizeof(ELEMENT_T);

		Chunk() = default;

		void SetBase(const ELEMENT_T *elements, size_t len)
		{
			assert(len && len <= MAX_ELEMENTS);
			_base_len = len;
			std::copy(elements, elements + len, &_base[0]);
		}

		void SetAlt(const ELEMENT_T *elements, size_t len)
		{
			assert(len <= MAX_ELEMENTS);
			_alt_len = len;
			std::copy(elements, elements + len, &_alt[0]);
		}

		bool operator ==(const Chunk &other) const
		{
			return _base_len == other._base_len && MatchNonEmptyData(_base, other._base, _base_len)
				&& _alt_len == other._alt_len
				&& (_alt_len == 0 || MatchNonEmptyData(_alt, other._alt, _alt_len));
		}

		inline size_t Match(const ELEMENT_T *data, size_t len) const
		{
			return (len >= _base_len && MatchNonEmptyData(data, _base, _base_len))
				? _base_len
				: ((_alt_len && len >= _alt_len && MatchNonEmptyData(data, _alt, _alt_len)) ? _alt_len : 0);
		}

		inline size_t MatchBase(const ELEMENT_T *data, size_t len) const
		{
			return (len >= _base_len && MatchNonEmptyData(data, _base, _base_len)) ? _base_len : 0;
		}

		void SetRewindPos(size_t rewind_pos)
		{
			_rewind_pos = rewind_pos;
			assert(_rewind_pos == rewind_pos);
		}

		size_t RewindPos() const
		{
			return _rewind_pos;
		}

		inline size_t MinLen() const
		{
			return (_alt_len && _alt_len < _base_len) ? _alt_len : _base_len;
		}

		inline size_t MaxLen() const
		{
			return (_alt_len > _base_len) ? _alt_len : _base_len;
		}

		inline size_t MinSize() const
		{
			return MinLen() * sizeof(ELEMENT_T);
		}

		inline size_t MaxSize() const
		{
			return MaxLen() * sizeof(ELEMENT_T);
		}
	};

	typedef std::vector<Chunk> Chain;

	Chain chain;
	/** This is a most tricky function:
	  * go through chain and for each non-heading chunk assign 'rewind' position that specifies
	  * where MatchPattern() routine will seek back in pattern chain in case of mismatching this
	  * particular chunk while all previous chunks where recently matched.
	  * Using correct rewind value ensures finding matches in cases like:
	  * {'ac' IN 'aac'}  {'abc' IN 'ababc'} {'bdbdba' IN 'bdbdbdba'} etc
	  */
	void GetReady()
	{
		for (size_t i = 1; i < chain.size(); ++i) {
			for (size_t j = i - 1; j >= 1; --j) {
				if (std::equal(chain.begin(), chain.begin() + j, chain.begin() + (i - j))) {
					size_t n = 2;
					for (;j * n < i; ++n) {
						if (!std::equal(chain.begin(), chain.begin() + j, chain.begin() + j * (n - 1))) {
							break;
						}
						if (!std::equal(chain.begin(), chain.begin() + j, chain.begin() + (i - j * n))) {
							break;
						}
					}
					chain[i].SetRewindPos(j * (n - 1));
					break;
				}
			}
		}
	}

	bool operator ==(const ScannedPattern &other) const
	{
		return chain == other.chain;
	}

	/** returns minimal size (in bytes) of string that could match this pattern */
	size_t MinSize() const
	{
		size_t out = 0;
		for (const auto &chunk : chain) {
			out+= chunk.MinSize();
		}
		return out;
	}

	/** returns size (in bytes) of sub string that should be read from tail of previous scan window
	  * to ensure matching substring that happen to be crossed by scan windows boundary
	  */
	size_t LookBehind() const
	{
		size_t out = 0;
		for (size_t i = 1; i < chain.size(); ++i) {
			out+= chain[i].MaxSize();
		}
		return out;
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

	template <class ScannedPatternT>
		struct PatternsVec : std::vector<ScannedPatternT>
	{
	};

	PatternsVec<ScannedPattern8x>    _patterns8x;  // multibyte UTF7, UTF8
	PatternsVec<ScannedPattern16x>   _patterns16x; // multiword UTF16
	PatternsVec<ScannedPattern8>     _patterns8;   // single byte encodings and single byte UTF7, UTF8
	PatternsVec<ScannedPattern16>    _patterns16;  // double-byte encodings and single word UTF16
	PatternsVec<ScannedPattern32>    _patterns32;  // UTF32

public:
	FindPattern(bool case_sensitive, bool whole_words);

	void AddBytesPattern(const uint8_t *pattern, size_t len);
	bool AddTextPattern(const wchar_t *pattern, unsigned int codepage);

	void GetReady();

	inline size_t MinPatternSize() const { return _min_pattern_size; }
	inline size_t LookBehind() const { return _look_behind; }

	std::pair<size_t, size_t> Match(const void *data, size_t len, bool first_fragment, bool last_fragment);
};
