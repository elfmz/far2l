#pragma once
#include <cstdint>

static inline void MulU64U64To128(uint64_t a, uint64_t b, uint64_t &hi, uint64_t &lo)
{
	const uint64_t a0 = (uint32_t)a;
	const uint64_t a1 = a >> 32;
	const uint64_t b0 = (uint32_t)b;
	const uint64_t b1 = b >> 32;

	const uint64_t p0 = a0 * b0;
	const uint64_t p1 = a0 * b1;
	const uint64_t p2 = a1 * b0;
	const uint64_t p3 = a1 * b1;

	uint64_t mid = p1 + p2;
	uint64_t mid_carry = (mid < p1) ? 1ULL : 0ULL;

	uint64_t lo_tmp = p0 + (mid << 32);
	uint64_t lo_carry = (lo_tmp < p0) ? 1ULL : 0ULL;

	hi = p3 + (mid >> 32) + (mid_carry << 32) + lo_carry;
	lo = lo_tmp;
}

static inline uint64_t DivU128U64Knuth(uint64_t hi, uint64_t lo, uint64_t d)
{
	if (d == 0) return 0;
	if (hi == 0) return lo / d;

	const uint32_t d_hi = (uint32_t)(d >> 32);

	int shift = 0;
	if (d_hi == 0) {
		const uint32_t d_lo = (uint32_t)d;
		if (d_lo == 0) return 0;
		shift = __builtin_clz(d_lo);
	} else {
		shift = __builtin_clz(d_hi);
	}

	const uint64_t dn = d << shift;
	const uint64_t un1 = (hi << shift) | (lo >> (64 - shift));
	const uint64_t un0 = lo << shift;

	const uint32_t dn1 = (uint32_t)(dn >> 32);
	const uint32_t dn0 = (uint32_t)dn;

	uint32_t un32 = (uint32_t)(un1 >> 32);
	uint32_t un10 = (uint32_t)un1;
	uint32_t un00 = (uint32_t)(un0 >> 32);
	uint32_t un0l = (uint32_t)un0;

	uint64_t q1 = ((uint64_t)un32 << 32 | un10) / dn1;
	uint64_t r1 = ((uint64_t)un32 << 32 | un10) - q1 * dn1;

	while (q1 >= (1ULL << 32) || q1 * dn0 > ((r1 << 32) | un00)) {
		q1--;
		r1 += dn1;
		if (r1 >= (1ULL << 32)) break;
	}

	uint64_t un10_ = ((uint64_t)un32 << 32 | un10) - q1 * dn1;
	uint64_t un00_ = ((uint64_t)un00 << 32 | un0l) - q1 * dn0;
	if ((int64_t)un00_ < 0) {
		un00_ += (1ULL << 32);
		un10_ -= 1;
	}

	uint64_t q0 = un00_ / dn1;
	uint64_t r0 = un00_ - q0 * dn1;

	while (q0 >= (1ULL << 32) || q0 * dn0 > ((r0 << 32) | un0l)) {
		q0--;
		r0 += dn1;
		if (r0 >= (1ULL << 32)) break;
	}

	return (q1 << 32) | q0;
}

static inline uint64_t MulDivU64(uint64_t a, uint64_t b, uint64_t d)
{
	if (d == 0) return 0;
#if defined(__SIZEOF_INT128__)
	const unsigned __int128 prod = (unsigned __int128)a * (unsigned __int128)b;
	return (uint64_t)(prod / d);
#else
	uint64_t hi = 0, lo = 0;
	MulU64U64To128(a, b, hi, lo);
	return DivU128U64Knuth(hi, lo, d);
#endif
}
