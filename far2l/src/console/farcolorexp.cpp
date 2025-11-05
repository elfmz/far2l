/*
farcolorexp.cpp
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "colors.hpp"
#include "headers.hpp"
#include "farcolors.hpp"

#include "colorexpdef.h"
//#include <cctweaks.h>

#ifdef _MSC_VER
	#define	FAR_ALIGNED(x) __declspec( align(x) )
	#define	FN_STATIC_INLINE static __forceinline
	#define	FN_INLINE __forceinline
#else
	#define	FAR_ALIGNED(x) __attribute__ ((aligned (x)))
	#define	FN_STATIC_INLINE static __attribute__((always_inline)) inline
	#define	FN_INLINE __attribute__((always_inline)) inline
#endif

static const std::pair<const uint64_t, const char *> ColorStyleFlags[]{
		{COMMON_LVB_STRIKEOUT,	   "STYLE_STRIKEOUT" },
		{COMMON_LVB_UNDERSCORE,	   "STYLE_UNDERSCORE"},
		{COMMON_LVB_REVERSE_VIDEO, "STYLE_INVERSE"}
};

static const char *ForegroundColorNames[]{"F_BLACK", "F_BLUE", "F_GREEN", "F_CYAN", "F_RED", "F_MAGENTA",
		"F_BROWN", "F_LIGHTGRAY", "F_DARKGRAY", "F_LIGHTBLUE", "F_LIGHTGREEN", "F_LIGHTCYAN", "F_LIGHTRED",
		"F_LIGHTMAGENTA", "F_YELLOW", "F_WHITE"};

static const char *BackgroundColorNames[]{"B_BLACK", "B_BLUE", "B_GREEN", "B_CYAN", "B_RED", "B_MAGENTA",
		"B_BROWN", "B_LIGHTGRAY", "B_DARKGRAY", "B_LIGHTBLUE", "B_LIGHTGREEN", "B_LIGHTCYAN", "B_LIGHTRED",
		"B_LIGHTMAGENTA", "B_YELLOW", "B_WHITE"};

typedef struct tident_s {
    const char *key;
    intptr_t value;
	int keylen;
	int n;
} tident_t;

static tident_t const_idents[] = {
{"BLACK", 0, 5, 0 },
{"BLUE", 1, 4, 0 },
{"BROWN", 6, 5, 0 },
{"B_BLACK", 0, 7, 0 },
{"B_BLUE", 16, 6, 0 },
{"B_BROWN", 96, 7, 0 },
{"B_CYAN", 48, 6, 0 },
{"B_DARKGRAY", 128, 10, 0 },
{"B_GREEN", 32, 7, 0 },
{"B_LIGHTBLUE", 144, 11, 0 },
{"B_LIGHTCYAN", 176, 11, 0 },
{"B_LIGHTGRAY", 112, 11, 0 },
{"B_LIGHTGREEN", 160, 12, 0 },
{"B_LIGHTMAGENTA", 208, 14, 0 },
{"B_LIGHTRED", 192, 10, 0 },
{"B_MAGENTA", 80, 9, 0 },
{"B_RED", 64, 5, 0 },
{"B_WHITE", 240, 7, 0 },
{"B_YELLOW", 224, 8, 0 },
{"CYAN", 3, 4, 0 },
{"DARKGRAY", 8, 8, 0 },
{"F_BLACK", 0, 7, 0 },
{"F_BLUE", 1, 6, 0 },
{"F_BROWN", 6, 7, 0 },
{"F_CYAN", 3, 6, 0 },
{"F_DARKGRAY", 8, 10, 0 },
{"F_GREEN", 2, 7, 0 },
{"F_LIGHTBLUE", 9, 11, 0 },
{"F_LIGHTCYAN", 11, 11, 0 },
{"F_LIGHTGRAY", 7, 11, 0 },
{"F_LIGHTGREEN", 10, 12, 0 },
{"F_LIGHTMAGENTA", 13, 14, 0 },
{"F_LIGHTRED", 12, 10, 0 },
{"F_MAGENTA", 5, 9, 0 },
{"F_RED", 4, 5, 0 },
{"F_WHITE", 15, 7, 0 },
{"F_YELLOW", 14, 8, 0 },
{"GREEN", 2, 5, 0 },
{"LIGHTBLUE", 9, 9, 0 },
{"LIGHTCYAN", 11, 9, 0 },
{"LIGHTGRAY", 7, 9, 0 },
{"LIGHTGREEN", 10, 10, 0 },
{"LIGHTMAGENTA", 13, 12, 0 },
{"LIGHTRED", 12, 8, 0 },
{"MAGENTA", 5, 7, 0 },
{"RED", 4, 3, 0 },
{"STYLE_INVERSE", 16384, 13, 0 },
{"STYLE_STRIKEOUT", 8192, 15, 0 },
{"STYLE_UNDERSCORE", 32768, 16, 0 },
{"WHITE", 15, 5, 0 },
{"YELLOW", 14, 6, 0 }
};

constexpr int const_idents_total = sizeof(const_idents) / sizeof(const_idents[0]);

static uint32_t xs30_seed[4] = {0x3D696D09, 0xCD6BEB33, 0x9D1A0022, 0x9D1B0022};
static uint64_t f_rand(void)
{
	uint32_t *zseed = &xs30_seed[0];
	zseed[0] ^= zseed[0] << 16;
	zseed[0] ^= zseed[0] >> 5;
	zseed[0] ^= zseed[0] << 1;
	uint32_t t = zseed[0];
	zseed[0] = zseed[1];
	zseed[1] = zseed[2];
	zseed[2] = t ^ zseed[0] ^ zseed[1];
	return (uint64_t)zseed[0];
}

//static uint64_t f_rand(void) { return 999; }
static uint64_t f_FOREGROUND(uint64_t rgb) { return (rgb << 16) | FOREGROUND_TRUECOLOR; }
static uint64_t f_FORE_RGB(uint64_t r, uint64_t g, uint64_t b) { return ((r | (g << 8) | (b << 16)) << 16) | FOREGROUND_TRUECOLOR; }
static uint64_t f_BACK_RGB(uint64_t r, uint64_t g, uint64_t b) { return ((r | (g << 8) | (b << 16)) << 40) | BACKGROUND_TRUECOLOR; }
static uint64_t f_BACKGROUND(uint64_t rgb) { return (rgb << 40) | BACKGROUND_TRUECOLOR; }
static uint64_t f_RGB(uint64_t r, uint64_t g, uint64_t b) { return r | (g << 8) | (b << 16); }

static tident_t t_functions[] = {
	{"back_rgb", (intptr_t)(void *)f_BACK_RGB, 8, 3 },
	{"background", (intptr_t)(void *)f_BACKGROUND, 10, 1 },
	{"fore_rgb", (intptr_t)(void *)f_FORE_RGB, 8, 3 },
	{"foreground", (intptr_t)(void *)f_FOREGROUND, 10, 1 },
	{"rand", (intptr_t)(void *)f_rand, 4, 0 },
	{"rgb", (intptr_t)(void *)f_RGB, 3, 3 },
};

constexpr int t_functions_total = sizeof(t_functions) / sizeof(t_functions[0]);

static int tident_compare(const void *a, const void *b) { return strcmp(((tident_t *)a)->key, ((tident_t *)b)->key); }

static int bin_search_id(tident_t *ids, int size, const char *key, int keylen) {
	int left = 0, right = size - 1;
	while (left <= right) {
		int cmp, mid = left + ((right - left) >> 1);
		if (ids[mid].keylen > keylen) {
			cmp = memcmp(ids[mid].key, key, keylen);
			if (!cmp) { right = mid - 1; continue; }
		}
		else if (ids[mid].keylen < keylen) {
			cmp = memcmp(ids[mid].key, key, ids[mid].keylen);
			if (!cmp) { left = mid + 1; continue; }
		}
		else {
			cmp = memcmp(ids[mid].key, key, keylen);
	        if (!cmp) return mid;
		}
        if (cmp < 0)
			left = mid + 1;
		else
			right = mid - 1;
	}
	return -1;
}

FN_STATIC_INLINE void eval_operator(const uint8_t op_id, uint64_t &lop, uint64_t &rop)
{
	switch (op_id) {
		case _OP_ID_MUL: lop *= rop; break;
		case _OP_ID_DIV: lop /= rop; break;
		case _OP_ID_MOD: lop %= rop; break;
		case _OP_ID_ADD: lop += rop; break;
		case _OP_ID_SUB: lop -= rop; break;
		case _OP_ID_SHR: lop >>= rop; break;
		case _OP_ID_SHL: lop <<= rop; break;
		case _OP_ID_OR:  lop |= rop; break;
		case _OP_ID_AND: lop &= rop; break;
		case _OP_ID_XOR: lop ^= rop; break;
	}
}

#define T_MAX_VALS 128
#define T_MAX_OPS 64
#define T_MAX_F_IN 32

#define T_SkipWhiteSpace() while (*expr == ' ' && len) { ++expr, --len; }
#define T_SkipAllIn() while (*expr != ')' && len) { ++expr, --len; }
#define T_GetID() while (tchtab[*expr] & TCH_ID && len) { ++expr, --len; }
#define T_SkipZeros() while (*expr == '0' && len) { ++expr, --len; }

#define T_PeekOP() (opp != -1 ? ops[opp] : 0)
#define T_PopOP() (opp != -1 ? ops[opp--] : 0)
#define T_PushOP(_val) if (opp < T_MAX_OPS - 1) ops[++opp] = _val
#define T_PeekVAL() (valp != -1 ? vals[valp] : 0)
#define T_PopVAL() (valp != -1 ? vals[valp--] : 0)
#define T_PushVAL(_val) if (valp < T_MAX_VALS - 1) vals[++valp] = _val
#define T_CheckOP()\
	if (flags) {\
		while (opp != -1 && (T_PeekOP() & 0xFFFFFFE0) >= (1 << 5)) {\
			T_evOP();\
		}\
		T_PushOP(_OP_ID_ADD + (1 << 5));\
	}

#define T_GetValue()\
	if (expr[0] == '0' && (expr[1] == 'X' || expr[1] == 'x') ) {\
		expr += 2;\
		if ( len < 3 )\
			return 0;\
		len -= 2;\
		if (atoh_lut256[*expr] == 0xFF)\
			return 0;\
		T_atoh();\
	}\
	else if (expr[0] == '#') {\
		++expr;\
		if (!--len)\
			return 0;\
		T_atoh();\
		value = RGB_2_BGR(value);\
	}\
	else {\
		T_atoui();\
	}

#define T_atoh() \
	do {\
		value <<= 4;\
		value += atoh_lut256[ *expr ];\
		-- len;\
	} while ( atoh_lut256[*++expr] != 0xFF && len );

#define T_atoui() \
	do {\
		value *= 10;\
		value += *expr - '0';\
		--len;\
	} while ( tchtab[ *++expr ] & TCH_DIGIT && len );

#define T_evOP() \
	uint64_t op = T_PopOP() & 15;\
	if (op == _OP_ID_FUNC) {\
		uint64_t rv = 0;\
		if (infp != -1) {\
			int ind = inf[infp--].ind;\
			switch(t_functions[ind].n) {\
				case 1: rv = ((uint64_t (*)(uint64_t))(void *)t_functions[ind].value)( T_PopVAL() ); break;\
				case 2: {\
					uint64_t arg2 = T_PopVAL();\
					uint64_t arg1 = T_PopVAL();\
					rv = ((uint64_t (*)(uint64_t, uint64_t))(void *)t_functions[ind].value)( arg1, arg2 );\
					break;\
				}\
				case 3: {\
					uint64_t arg3 = T_PopVAL();\
					uint64_t arg2 = T_PopVAL();\
					uint64_t arg1 = T_PopVAL();\
					rv = ((uint64_t (*)(uint64_t, uint64_t, uint64_t))(void *)t_functions[ind].value)( arg1, arg2, arg3 );\
					break;\
				}\
			}\
		}\
		T_PushVAL(rv);\
		continue;\
	}\
	uint64_t rv = T_PopVAL();\
	uint64_t lv = T_PopVAL();\
	eval_operator(op, lv, rv);\
	T_PushVAL(lv);

uint64_t ExprToFarColor(const char *_expr, size_t len)
{
	const uint8_t *expr = (const uint8_t*)_expr;
	uint64_t vals[T_MAX_VALS];
	uint8_t ops[T_MAX_OPS];
	struct t_infn_s {
		uint16_t ind;
		uint8_t ffd;
		uint8_t nst;
	} inf[T_MAX_F_IN];
	intptr_t valp = -1, opp = -1, infp = -1;
	uint32_t flags = 0;

	if (!expr || !len) return 0;

	T_SkipWhiteSpace();
	if (!len) return 0;

	while (len > 0) {

		T_SkipWhiteSpace();
		if (!len)
			break;

		if (*expr == '(') {
			T_CheckOP();
			T_PushOP(_OP_ID_BRACKET_OPEN);
			if (infp != -1) {
				++inf[infp].nst;
			}
			++expr, --len, flags = 0;
			continue;
		}

		if (*expr == ',') {
			if (infp == -1 || !inf[infp].ffd) {
				T_CheckOP();
				++expr, --len, flags = 0;
				continue;
			}
		}

		if (*expr == ')' || *expr == ',') {
			while (opp != -1 && T_PeekOP() != _OP_ID_BRACKET_OPEN) {
				T_evOP();
			}
			if (infp != -1) {
				if (!inf[infp].nst) {
					if (--inf[infp].ffd) {
						++expr, --len, flags = 0; continue;
					}
				}
				else {
					--inf[infp].nst;
				}
			}
			T_PopOP();
			++expr, --len, flags = 1;
			continue;
		}

		if ( (tchtab[*expr] & TCH_LETTER)) {
			const uint8_t *id = expr;
			T_GetID();
			int id_len = (int)(expr - id);
			T_SkipWhiteSpace();
			T_CheckOP();
			flags = 1;

			if (len && (*expr == '(' || *expr == ':')) {
				int ind = bin_search_id(t_functions, t_functions_total, (const char *)id, id_len);
				if (ind == -1 || infp >= T_MAX_F_IN - 1) { // not found
					T_PushVAL(0);
					T_SkipAllIn(); ++expr, --len;
					continue;
				}
				if (t_functions[ind].n == 0) {
					T_PushVAL( ((uint64_t (*)())(void *)t_functions[ind].value)( ) );
					T_SkipAllIn(); ++expr, --len;
					continue;
				}
				if (*expr == ':' && t_functions[ind].n == 1) {
					++expr, --len;
					T_SkipWhiteSpace();
					if (!len) break;
					if ((tchtab[*expr] & TCH_DIGIT) || *expr == '#') {
						uint64_t value = 0;
						T_GetValue();
						T_PushVAL( ((uint64_t (*)(uint64_t))(void *)t_functions[ind].value)( value ) );
						continue;
					}
				}
				T_PushOP(_OP_ID_FUNC | (2 << 5));
				inf[++infp].ind = ind; inf[infp].ffd = t_functions[ind].n, inf[infp].nst = 0;
				T_PushOP(_OP_ID_BRACKET_OPEN);
				++expr, --len, flags = 0; // skip (
				continue;
			}

			int ind = bin_search_id(const_idents, const_idents_total, (const char *)id, id_len);
			if (ind == -1) { // not found
				T_PushVAL(0);
				continue;
			}

			T_PushVAL(const_idents[ind].value);
			continue;
		}

		if ((tchtab[*expr] & TCH_DIGIT) || *expr == '#') {
			uint64_t value = 0;
			T_CheckOP();
			flags = 1;
			T_GetValue();
			T_PushVAL(value);
			continue;
		}

		if (tchtab[*expr] & TCH_OPERATOR) {	  // op
			uint64_t op_id = OpLut[*expr];
			if (op_id & _OP_DOUBLE_CH) {
				if (tchtab[expr[1]] & TCH_OPERATOR) {
					++expr, --len;
					if (!len) break;
				}
			}
			while (opp != -1 && (T_PeekOP() & 0xFFFFFFE0) >= (op_id & 0xFFFFFFE0)) {
				T_evOP();
			}
			T_PushOP(op_id);
			++expr, --len, flags = 0;
			continue;
		}

		if (*expr == ';') break;
		++expr, --len;
	}

	while (opp != -1) {
		T_evOP();
    }

    return T_PopVAL();
}

size_t FarColorToExpr(uint64_t c, char *exsp, size_t s)
{
	bool addop = false;
	size_t n = 0;

	if (c & BACKGROUND_TRUECOLOR) {
		uint32_t bc = (uint32_t)(c >> 40);
		n += snprintf(exsp, s - n, "background:#%02x%02x%02x ", GetRValue(bc), GetGValue(bc), GetBValue(bc));
		addop = true;
	}
	if (c & FOREGROUND_TRUECOLOR) {
		uint32_t bc = (uint32_t)((c >> 16) & 0xFFFFFF);
		n += snprintf(exsp + n, s - n, "foreground:#%02x%02x%02x", GetRValue(bc), GetGValue(bc), GetBValue(bc));
		addop = true;
	}


#if 0
	if (c & BACKGROUND_TRUECOLOR) {
		uint32_t bc = (uint32_t)(c >> 40);
		n += snprintf(exsp, s - n, "BACK_RGB(%u, %u, %u)", GetRValue(bc), GetGValue(bc), GetBValue(bc));
		addop = true;
	}
	if (c & FOREGROUND_TRUECOLOR) {
		if (addop) {
			exsp[n++] = ' ';
			exsp[n++] = '|';
			exsp[n++] = ' ';
		}
		uint32_t bc = (uint32_t)((c >> 16) & 0xFFFFFF);
		n += snprintf(exsp + n, s - n, "FORE_RGB(%u, %u, %u)", GetRValue(bc), GetGValue(bc), GetBValue(bc));
		addop = true;
	}

	if (addop) {
		exsp[n++] = ' ';
		exsp[n++] = '+';
		exsp[n++] = ' ';
	}
#endif

	if (addop) {
//		exsp[n++] = ' ';
		exsp[n++] = ',';
		exsp[n++] = ' ';
	}

	n += snprintf(exsp + n, s - n, "%s | %s", BackgroundColorNames[(c >> 4) & 0xF],
			ForegroundColorNames[c & 0xF]);

	for (auto i : ColorStyleFlags) {
		if (c & i.first) {
			exsp[n++] = ' ';
			exsp[n++] = '+';
			exsp[n++] = ' ';
			n += snprintf(exsp + n, s - n, "%s", i.second);
		}
	}

#if 1 // rgb comments
	if (c & BACKGROUND_TRUECOLOR || c & FOREGROUND_TRUECOLOR) {
		exsp[n++] = ';';
		exsp[n++] = ' ';
	}
	if (c & BACKGROUND_TRUECOLOR) {
		uint32_t bc = (uint32_t)(c >> 40);
		n += snprintf(exsp + n, s - n, "back_rgb(%u, %u, %u)", GetRValue(bc), GetGValue(bc), GetBValue(bc));
		addop = true;
	}
	if (c & FOREGROUND_TRUECOLOR) {
		if (addop) {
			exsp[n++] = ' ';
			exsp[n++] = '|';
			exsp[n++] = ' ';
		}
		uint32_t bc = (uint32_t)((c >> 16) & 0xFFFFFF);
		n += snprintf(exsp + n, s - n, "fore_rgb(%u, %u, %u)", GetRValue(bc), GetGValue(bc), GetBValue(bc));
		addop = true;
	}
#endif

	return n;
}
