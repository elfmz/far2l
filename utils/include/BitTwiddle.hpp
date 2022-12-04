#pragma once

#ifdef __APPLE__
# include <machine/endian.h>  // __BYTE_ORDER
#elif defined(__FreeBSD__)
# include <sys/endian.h>  // __BYTE_ORDER
#else
# include <endian.h>  // __BYTE_ORDER
#endif

template <class POD_T>
	inline void ZeroFill(POD_T &dst)
{
	static_assert ( std::is_pod<POD_T>::value, "ZeroFill should be used with POD types only");
	static_assert ( sizeof(dst) != sizeof(void *), "ZeroFill should not be used with pointers");
	memset(&dst, 0, sizeof(dst));
}

template <class V, class A>
	inline V AlignUp(V v, A a)
{
	const auto r = v % a;
	return r ? v + (a - (A)r) : v;
}

template <class V, class A>
	inline V AlignDown(V v, A a)
{
	const auto r = v % a;
	return v - r;
}

template <class V>
	inline V RevBytes(const V &value)
{
	switch (sizeof(V)) {
		case 1: return value;
		case 2: return (V)__builtin_bswap16((uint16_t)value);
		case 4: return (V)__builtin_bswap32((uint32_t)value);
		case 8: return (V)__builtin_bswap64((uint64_t)value);
	}
	abort();
}

template <class V>
	inline void RevBytes(V *values, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		values[i] = RevBytes(values[i]);
	}
}

template <class V>
	inline void RevBytes(V *dst_values, const V *src_values, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		dst_values[i] = RevBytes(src_values[i]);
	}
}

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define LITEND(V)   (RevBytes(V))
#else
# define LITEND(V)   (V)
#endif

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
# define LITEND_FILETIME_CONVERT(FT) { \
	std::swap(FT.dwLowDateTime, dwHighDateTime); \
	FT.dwLowDateTime = __builtin_bswap32(FT.dwLowDateTime); \
	FT.dwHighDateTime = __builtin_bswap32(FT.dwHighDateTime); \
}
#else
# define LITEND_FILETIME_CONVERT(FT)
#endif

