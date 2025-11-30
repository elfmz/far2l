#pragma once

#ifdef __APPLE__
# include <machine/endian.h>  // __BYTE_ORDER
#elif defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/endian.h>  // __BYTE_ORDER
#else
# include <endian.h>  // __BYTE_ORDER
#endif

#include <cstdint>

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
	static_assert(sizeof(V) == 1 || sizeof(V) == 2 || sizeof(V) == 4 || sizeof(V) == 8, "RevBytes works only with sane-sized integers");
	switch (sizeof(V)) {
		case 8: return (V)__builtin_bswap64((uint64_t)value);
		case 4: return (V)__builtin_bswap32((uint32_t)value);
		case 2: return (V)__builtin_bswap16((uint16_t)value);
		default: // case 1:
			return value;
	}
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
# define ENDIAN_IS_BIG
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
# define ENDIAN_IS_BIG
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
		defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || \
		defined(__MIPSEB__)
# define ENDIAN_IS_BIG
#endif

#ifdef ENDIAN_IS_BIG
# define LITEND(V)   (RevBytes(V))
# define LITEND_INPLACE(V)   V = LITEND(V)
# define BIGEND(V)   (V)
# define BIGEND_INPLACE(V)
# define LITEND_INPLACE_FILETIME(FT) { \
	std::swap((FT).dwLowDateTime, (FT).dwHighDateTime); \
	(FT).dwLowDateTime = __builtin_bswap32((FT).dwLowDateTime); \
	(FT).dwHighDateTime = __builtin_bswap32((FT).dwHighDateTime); \
}

#else // #ifdef ENDIAN_IS_BIG
# define LITEND(V)   (V)
# define LITEND_INPLACE(V)
# define BIGEND(V)   (RevBytes(V))
# define BIGEND_INPLACE(V)   V = BIGEND(V)
# define LITEND_INPLACE_FILETIME(FT)

#endif // #ifdef ENDIAN_IS_BIG
