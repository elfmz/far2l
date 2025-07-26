#pragma once
#include <stdint.h>
#include <wchar.h>
#include <array>

class CharClasses
{
	const wchar_t _c;
#ifdef RUNTIME_ICU
	int32_t _prop_EAST_ASIAN_WIDTH{-1};
	int32_t _prop_JOINING_TYPE{-1};
	int32_t _prop_GENERAL_CATEGORY{-1};
	int32_t _prop_BLOCK{-1};
#endif

	static constexpr size_t UNICODE_SIZE = 0x110000;
	static std::array<uint8_t, UNICODE_SIZE> charFlags;
	enum CharFlags : uint8_t {
		IS_PREFIX    = 1 << 0,
		IS_SUFFIX    = 1 << 1,
		IS_FULLWIDTH = 1 << 2,
	};
	static bool initialized;

	static void InitCharFlags() {
		if (initialized)
			return;
		initialized = true;

		for (wchar_t ch = 0; ch < UNICODE_SIZE; ++ch) {
			CharClasses cc(ch);
			if (cc.Prefix())
				charFlags[ch] |= IS_PREFIX;
			if (cc.Suffix())
				charFlags[ch] |= IS_SUFFIX;
			if (cc.FullWidth())
				charFlags[ch] |= IS_FULLWIDTH;
		}
	}
public:
	inline CharClasses(wchar_t c) : _c(c) {}

	bool FullWidth();
	bool Prefix();
	bool Suffix();
	inline bool Xxxfix()
	{
		return Prefix() || Suffix();
	}

	static inline bool IsFullWidth(wchar_t c) {
		InitCharFlags();
		return c < UNICODE_SIZE && (charFlags[c] & IS_FULLWIDTH);
	}
	static inline bool IsPrefix(wchar_t c) {
		InitCharFlags();
		return c < UNICODE_SIZE && (charFlags[c] & IS_PREFIX);
	}
	static inline bool IsSuffix(wchar_t c) {
		InitCharFlags();
		return c < UNICODE_SIZE && (charFlags[c] & IS_SUFFIX);
	}
	static inline bool IsXxxfix(wchar_t c) {
		InitCharFlags();
		return IsPrefix(c) || IsSuffix(c);
	}

};
