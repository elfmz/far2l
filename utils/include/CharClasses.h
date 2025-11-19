#pragma once
#include <stdint.h>
#include <wchar.h>
#include <array>
#include <memory>
#include <unordered_map>

class CharClasses
{
	const wchar_t _c;
#ifdef RUNTIME_ICU
	int32_t _prop_EAST_ASIAN_WIDTH{-1};
	int32_t _prop_JOINING_TYPE{-1};
	int32_t _prop_GENERAL_CATEGORY{-1};
	int32_t _prop_BLOCK{-1};
#endif

	static constexpr wchar_t ASCII_MAX = 0x7F;
	static constexpr size_t UNICODE_SIZE = 0x110000;
	static constexpr size_t SHIFT = 8;
	static constexpr size_t CHAR_BLOCK_SIZE = 1 << SHIFT;
	static constexpr size_t BLOCK_COUNT = (UNICODE_SIZE + CHAR_BLOCK_SIZE - 1) / CHAR_BLOCK_SIZE;

	using Block = std::array<uint8_t, CHAR_BLOCK_SIZE>;
	static std::array<std::shared_ptr<Block>, BLOCK_COUNT> blocks;

	enum CharFlags : uint8_t {
		IS_PREFIX    = 1 << 0,
		IS_SUFFIX    = 1 << 1,
		IS_FULLWIDTH = 1 << 2,
	};
	static bool initialized;

	struct BlockHasher { //FNV-1a hash
		static constexpr std::size_t fnv_offset_basis =
			(SIZE_MAX == UINT64_MAX) ? 0xcbf29ce484222325ULL : 0x811c9dc5UL;

		static constexpr std::size_t fnv_prime =
			(SIZE_MAX == UINT64_MAX) ? 0x100000001b3ULL : 0x01000193UL;

		std::size_t operator()(const std::shared_ptr<Block>& b) const {
			std::size_t hash = fnv_offset_basis;
			for (uint8_t v : *b)
				hash = (hash ^ v) * fnv_prime;
			return hash;
		}
	};

	struct BlockEqual {
		bool operator()(const std::shared_ptr<Block>& a, const std::shared_ptr<Block>& b) const {
			return *a == *b;
		}
	};

	static inline uint8_t Get(wchar_t c) {
		if (!initialized) InitCharFlags();
		if (static_cast<uint32_t>(c) >= UNICODE_SIZE) return 0;

		size_t high = c >> SHIFT;
		size_t low = c & (CHAR_BLOCK_SIZE - 1);
		auto& block = blocks[high];
		return block ? (*block)[low] : 0;
	}

public:
	static constexpr wchar_t VARIATION_SELECTOR_16 = 0xFE0F;
	static constexpr wchar_t ZERO_WIDTH_JOINER = 0x200D;
	inline CharClasses(wchar_t c) : _c(c) {}

	static void InitCharFlags() {
		if (initialized)
			return;
		initialized = true;

		for (uint32_t ch = 0; ch < UNICODE_SIZE; ++ch) {
			uint8_t flags = 0;
			CharClasses cc(ch);
			if (cc.Prefix())     flags |= IS_PREFIX;
			if (cc.Suffix())     flags |= IS_SUFFIX;
			if (cc.FullWidth())  flags |= IS_FULLWIDTH;
			if (flags) {
				size_t high = ch >> SHIFT;
				size_t low = ch & (CHAR_BLOCK_SIZE - 1);
				if (!blocks[high])
					blocks[high] = std::make_shared<Block>();
				(*blocks[high])[low] = flags;
			}
		}
		// deduplication
		std::unordered_map<std::shared_ptr<Block>, std::shared_ptr<Block>, BlockHasher, BlockEqual> dedupMap;
		for (auto& blk : blocks) {
			if (!blk) continue;
			auto it = dedupMap.find(blk);
			if (it != dedupMap.end()) {
				blk = it->second;
			} else {
				dedupMap[blk] = blk;
			}
		}
/*
		size_t block_count = dedupMap.size();;
		size_t total_bytes = block_count * CHAR_BLOCK_SIZE * sizeof(uint8_t);
		fprintf(stderr, "[CharClasses] Allocated blocks: %zu"
						", total bytes: %zu\n", block_count, total_bytes );
*/
	}

	bool FullWidth();
	bool Prefix();
	bool Suffix();
	inline bool Xxxfix()
	{
		return Prefix() || Suffix();
	}

	static inline bool IsFullWidth(const wchar_t* p) {
		if (!p || *p <= ASCII_MAX) return false;
		//Variation Selector-16 indicates that the previous character should be rendered as an image
		if (*(p + 1) == VARIATION_SELECTOR_16) return true;
		return Get(*p) & IS_FULLWIDTH;
	}
	static inline bool IsFullWidth(const wchar_t* p, size_t n) {
		if (!p || *p <= ASCII_MAX) return false;
		//Variation Selector-16 indicates that the previous character should be rendered as an image
		if (n > 1 && *(p + 1) == VARIATION_SELECTOR_16) return true;
		return Get(*p) & IS_FULLWIDTH;
	}
	static inline bool IsFullWidth(wchar_t c) {
		if (c <= ASCII_MAX) return false;
		return Get(c) & IS_FULLWIDTH;
	}
	static inline bool IsPrefix(wchar_t c) {
		if (c <= ASCII_MAX) return false;
		return Get(c) & IS_PREFIX;
	}
	static inline bool IsSuffix(wchar_t c) {
		if (c <= ASCII_MAX) return false;
		return Get(c) & IS_SUFFIX;
	}
	static inline bool IsXxxfix(wchar_t c) {
		if (c <= ASCII_MAX) return false;
		return Get(c) & (IS_PREFIX | IS_SUFFIX);
	}

};
