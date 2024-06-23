#pragma once

template <class C>
	size_t tzlen(const C *ptz)
{
	const C *etz;
	for (etz = ptz; *etz; ++etz);
	return (etz - ptz);
}

template <class C>
	size_t tnzlen(const C *ptz, size_t n)
{
	size_t i;
	for (i = 0; i < n && ptz[i]; ++i);
	return i;
}

// like strnlen(a, ARRAYSIZE(a))
template <typename ARRAY_T>
	size_t CharArrayLen(const ARRAY_T &a)
{
	static_assert ( std::is_pod<ARRAY_T>::value, "CharArrayLen should be used with POD array types only");
	static_assert ( sizeof(a) != sizeof(void *), "CharArrayLen should be used with arrays but not pointers");
	return tnzlen(a, ARRAYSIZE(a));
}

// like strchr but takes into account ARRAYSIZE(a)
template <typename ARRAY_T, class CHAR_T>
	CHAR_T *CharArrayChr(ARRAY_T &a, CHAR_T c)
{
	static_assert ( std::is_pod<ARRAY_T>::value, "CharArrayLen should be used with POD array types only");
	static_assert ( sizeof(a) != sizeof(void *), "CharArrayChr should be used with arrays but not pointers");
	for (size_t i = 0; i < ARRAYSIZE(a); ++i) {
		if (a[i] == c) {
			return &a[i];
		}
	}
	return NULL;
}

// like strrchr but takes into account ARRAYSIZE(a)
template <typename ARRAY_T, class CHAR_T>
	CHAR_T *CharArrayRChr(ARRAY_T &a, CHAR_T c)
{
	static_assert ( std::is_pod<ARRAY_T>::value, "CharArrayRChr should be used with POD array types only");
	static_assert ( sizeof(a) != sizeof(void *), "CharArrayRChr should be used with arrays but not pointers");
	for (size_t i = CharArrayLen(a); i--;) {
		if (a[i] == c) {
			return &a[i];
		}
	}
	return NULL;
}

// like strcmp but takes into account ARRAYSIZE(a1) and ARRAYSIZE(a2)
template <typename ARRAY1_T, typename ARRAY2_T>
	int CharArrayCmp(const ARRAY1_T &a1, const ARRAY2_T &a2)
{
	static_assert ( std::is_pod<ARRAY1_T>::value, "CharArrayCmp should be used with POD array types only");
	static_assert ( std::is_pod<ARRAY2_T>::value, "CharArrayCmp should be used with POD array types only");
	static_assert ( sizeof(a1) != sizeof(void *) && sizeof(a2) != sizeof(void *), "CharArrayCmp should be used with arrays but not pointers");
	size_t i;
	for (i = 0; i < ARRAYSIZE(a1) && i < ARRAYSIZE(a2); ++i) {
		if (a1[i] != a2[i]) {
			return (a1[i] < a2[i]) ? -1 : 1;
		}
	}
	if (i < ARRAYSIZE(a1))
		return 1;
	if (i < ARRAYSIZE(a2))
		return -1;
	return 0;
}

// same as s=a but takes into account ARRAYSIZE(a)
template <class STRING_T, typename ARRAY_T>
	void CharArrayAssignToStr(STRING_T &s, const ARRAY_T &a)
{
	static_assert ( std::is_pod<ARRAY_T>::value, "CharArrayAssignToStr should be used with POD array types only");
	static_assert ( sizeof(a) != sizeof(void *), "CharArrayAssignToStr should be used with arrays but not pointers");
	s.assign(a, tnzlen(a, ARRAYSIZE(a)));
}

// same as s+=a but takes into account ARRAYSIZE(a)
template <class STRING_T, typename ARRAY_T>
	void CharArrayAppendToStr(STRING_T &s, const ARRAY_T &a)
{
	static_assert ( std::is_pod<ARRAY_T>::value, "CharArrayAppendToStr should be used with POD array types only");
	static_assert ( sizeof(a) != sizeof(void *), "CharArrayAppendToStr should be used with arrays but not pointers");
	s.append(a, tnzlen(a, ARRAYSIZE(a)));
}


// same as strcmp(s.c_str(), a) but takes into account ARRAYSIZE(a)
template <typename ARRAY_T, class STRING_T>
	bool CharArrayMatchStr(const ARRAY_T &a, STRING_T &s)
{
	static_assert ( std::is_pod<ARRAY_T>::value, "CharArrayMatchStr should be used with POD array types only");
	static_assert ( sizeof(a) != sizeof(void *), "CharArrayMatchStr should be used with arrays but not pointers");
	const size_t l = tnzlen(a, ARRAYSIZE(a));
	return s.size() == l && s.compare(0, std::string::npos, a, l) == 0;
}

// same as strncpy(s.c_str(), ARRAYSIZE(a)) but ensures last character NUL'ed
template <typename ARRAY_T, class CHAR_T>
	void CharArrayCpyZ(ARRAY_T &dst, const CHAR_T *src)
{
	static_assert ( std::is_pod<ARRAY_T>::value, "CharArrayCpyZ should be used with POD array types only");
	static_assert ( sizeof(dst) != sizeof(void *), "CharArrayCpyZ should be used with arrays but not pointers");
	size_t i;
	for (i = 0; src[i] && i + 1 < ARRAYSIZE(dst); ++i) {
		dst[i] = src[i];
	}
	dst[i] = 0;
}

// same as strncpy(s.c_str(), ARRAYSIZE(a))
template <typename ARRAY_T, class CHAR_T>
	void CharArrayCpy(ARRAY_T &dst, const CHAR_T *src)
{
	static_assert ( std::is_pod<ARRAY_T>::value, "CharArrayCpy should be used with POD array types only");
	static_assert ( sizeof(dst) != sizeof(void *), "CharArrayCpy should be used with arrays but not pointers");
	size_t i;
	for (i = 0; src[i] && i < ARRAYSIZE(dst); ++i) {
		dst[i] = src[i];
	}
	if (LIKELY(i < ARRAYSIZE(dst))) {
		dst[i] = 0;
	}
}
