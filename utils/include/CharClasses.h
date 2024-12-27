#pragma once
#include <stdint.h>
#include <wchar.h>

class CharClasses
{
	const wchar_t _c;
#ifdef RUNTIME_ICU
	int32_t _prop_EAST_ASIAN_WIDTH{-1};
	int32_t _prop_JOINING_TYPE{-1};
	int32_t _prop_GENERAL_CATEGORY{-1};
	int32_t _prop_BLOCK{-1};
#endif

public:
	inline CharClasses(wchar_t c) : _c(c) {}

	bool FullWidth();
	bool Prefix();
	bool Suffix();
	inline bool Xxxfix()
	{
		return Prefix() || Suffix();
	}
};
