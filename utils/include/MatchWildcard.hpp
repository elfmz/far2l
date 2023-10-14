#pragma once
#include <assert.h>

template <class CHAR_T, class CHAR_COMPARE>
	bool MatchWildcardT(const CHAR_T *str, const CHAR_T *wild)
{
	// originally written by Jack Handy
	const CHAR_T *cp = nullptr, *mp = nullptr;
	while ((*str) && (*wild != (CHAR_T)'*')) {
		if (!CHAR_COMPARE()(*wild, *str) && (*wild != (CHAR_T)'?'))
			return false;
		
		wild++;
		str++;
	}

	while (*str) {
		if (*wild == (CHAR_T)'*') {
			if (!*++wild)
				return true;
			
			mp = wild;
			cp = str+1;

		} else if (CHAR_COMPARE()(*wild, *str) || (*wild == (CHAR_T)'?')) {
			wild++;
			str++;

		} else {
			assert (mp && cp);
			wild = mp;
			str = cp++;
		}
	}

	while (*wild == (CHAR_T)'*') wild++;
	return !*wild;
}

template <class CHAR_T>
	struct CharComparePlain
{
	bool operator()(const CHAR_T &left, const CHAR_T &right) const
	{
		return (left == right);
	}
};

template <class CHAR_T>
	CHAR_T EngLower(CHAR_T c)
{
	return (c >= (CHAR_T)'A' && c <= (CHAR_T)'Z') ? c + (CHAR_T)'a' - (CHAR_T)'A' : c;
}

template <class CHAR_T>
	struct CharCompareICE
{
	bool operator()(const CHAR_T &left, const CHAR_T &right) const
	{
		return (EngLower(left) == EngLower(right));
	}
};

/////////////////////////

template <class CHAR_T>
	bool MatchWildcard(const CHAR_T *str, const CHAR_T *wild) // case sensitive
{
	return MatchWildcardT<CHAR_T, CharComparePlain<CHAR_T> >(str, wild);
}

template <class CHAR_T>
	bool MatchWildcardICE(const CHAR_T *str, const CHAR_T *wild)  // case-insensitive for english chars
{
	return MatchWildcardT<CHAR_T, CharCompareICE<CHAR_T> >(str, wild);
}
