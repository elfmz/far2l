#pragma once
#include <vector>
#include <stdint.h>
#include <assert.h>

template <class ELEMENT_T, size_t MAX_ELEMENTS>
	struct ScannedPattern
{
	// inspired by Aho-Corasick
	class Chunk
	{
		uint16_t _rewind_pos;
		unsigned char _len;     	// never zero
		unsigned char _alt_len;     // can be zero
		struct Elements
		{
			ELEMENT_T contained[MAX_ELEMENTS];

			bool NonEmptyMatchData(const ELEMENT_T *data, unsigned char len) const;

		} _elements, _alt_elements;

	public:
		Chunk() = default;

		Chunk(size_t len, const ELEMENT_T *elements,
			size_t alt_len, const ELEMENT_T *alt_elements)
		{
			assert(len <= MAX_ELEMENTS);
			assert(alt_len <= MAX_ELEMENTS);
			_rewind_pos = 0;
			_len = len;
			_alt_len = alt_len;
			std::copy(elements, elements + len, &_elements.contained[0]);
			std::copy(alt_elements, alt_elements + alt_len, &_alt_elements.contained[0]);
		}

		bool operator ==(const Chunk &other) const
		{
			return _len == other._len && _elements.NonEmptyMatchData(other._elements.contained, _len)
				&& _alt_len == other._alt_len
				&& (_alt_len == 0 || _alt_elements.NonEmptyMatchData(other._alt_elements.contained, _alt_len));
		}

		inline size_t Match(const ELEMENT_T *data, size_t len) const
		{
			if (len >= _len && _elements.NonEmptyMatchData(data, _len)) {
				return _len;
			}

			if (_alt_len && len >= _alt_len && _alt_elements.NonEmptyMatchData(data, _alt_len)) {
				return _alt_len;
			}

			return 0;
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
			return (_alt_len && _alt_len < _len) ? _alt_len : _len;
		}

		inline size_t MaxLen() const
		{
			return (_alt_len && _alt_len > _len) ? _alt_len : _len;
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

	ScannedPattern(const std::vector<std::pair<std::vector<ELEMENT_T>, std::vector<ELEMENT_T>>> &chain_src)
	{
		chain.clear();
		for (const auto &chunk_src : chain_src) {
			chain.emplace_back(
				chunk_src.first.size(), chunk_src.first.data(),
				chunk_src.second.size(), chunk_src.second.data());
		}
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

	size_t MinSize() const
	{
		size_t out = 0;
		for (const auto &chunk : chain) {
			out+= chunk.MinSize();
		}
		return out;
	}

	size_t LookBehind() const
	{
		size_t out = 0;
		for (size_t i = 1; i < chain.size(); ++i) {
			out+= chain[i].MaxSize();
		}
		return out;
	}
};

typedef ScannedPattern<uint8_t, 6> ScannedPatternUTF8;
typedef ScannedPattern<uint16_t, 2> ScannedPatternUTF16;
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
		template <class CHAIN_SRC_T>
			void EmplaceUniq(const CHAIN_SRC_T &new_chain_src)
		{
			std::vector<ScannedPatternT>::emplace_back(new_chain_src);
			for (size_t i = 0; i + 1 < std::vector<ScannedPatternT>::size(); ++i) {
				if (std::vector<ScannedPatternT>::operator[](i) == std::vector<ScannedPatternT>::back()) {
					std::vector<ScannedPatternT>::pop_back();
					return;
				}
			}
		}
	};

	PatternsVec<ScannedPatternUTF8>  _patternsUTF8;
	PatternsVec<ScannedPatternUTF16> _patternsUTF16;
	PatternsVec<ScannedPattern8>     _patterns8;
	PatternsVec<ScannedPattern16>    _patterns16;
	PatternsVec<ScannedPattern32>    _patterns32;

public:
	FindPattern(bool case_sensitive, bool whole_words);

	void AddBytesPattern(const uint8_t *pattern, size_t len);
	bool AddTextPattern(const wchar_t *pattern, unsigned int codepage);

	void GetReady();

	inline size_t MinPatternSize() const { return _min_pattern_size; }
	inline size_t LookBehind() const { return _look_behind; }

	std::pair<size_t, size_t> Match(const void *data, size_t len, bool first_fragment, bool last_fragment);
};
