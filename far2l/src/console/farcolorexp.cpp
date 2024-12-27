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
//#include "color.hpp"

//#define __USE_MATHEXPRESSION_H 1

static const std::pair<const uint64_t, const char *> ColorStyleFlags[]{
	{COMMON_LVB_STRIKEOUT,	   "STYLE_STRIKEOUT" },
	{COMMON_LVB_UNDERSCORE,	   "STYLE_UNDERSCORE"},
	{COMMON_LVB_REVERSE_VIDEO, "STYLE_INVERSE"	  }
};

static const char *ForegroundColorNames[] {"F_BLACK", "F_BLUE", "F_GREEN", "F_CYAN", "F_RED", "F_MAGENTA",
	"F_BROWN", "F_LIGHTGRAY", "F_DARKGRAY", "F_LIGHTBLUE", "F_LIGHTGREEN", "F_LIGHTCYAN", "F_LIGHTRED",
	"F_LIGHTMAGENTA", "F_YELLOW", "F_WHITE"
};

static const char *BackgroundColorNames[]{"B_BLACK", "B_BLUE", "B_GREEN", "B_CYAN", "B_RED", "B_MAGENTA",
	"B_BROWN", "B_LIGHTGRAY", "B_DARKGRAY", "B_LIGHTBLUE", "B_LIGHTGREEN", "B_LIGHTCYAN", "B_LIGHTRED",
	"B_LIGHTMAGENTA", "B_YELLOW", "B_WHITE"
};

static uint64_t f_rand(void)
{
	return 999;
}

static uint64_t f_FOREGROUND(uint64_t rgb)
{
	return (rgb << 16) | FOREGROUND_TRUECOLOR;
}

static uint64_t f_FORE_RGB(uint64_t r, uint64_t g, uint64_t b)
{
	return ((r | (g << 8) | (b << 16)) << 16) | FOREGROUND_TRUECOLOR;
}

static uint64_t f_BACK_RGB(uint64_t r, uint64_t g, uint64_t b)
{
	return ((r | (g << 8) | (b << 16)) << 40) | BACKGROUND_TRUECOLOR;
}

static uint64_t f_BACKGROUND(uint64_t rgb)
{
	return (rgb << 40) | BACKGROUND_TRUECOLOR;
}

static uint64_t f_RGB(uint64_t r, uint64_t g, uint64_t b)
{
	return r | (g << 8) | (b << 16);
}

#ifdef __USE_MATHEXPRESSION_H
#include "MathExpression/MathExpressionBase.h"

class FarColorExpression : public MathExpressionBase<uint64_t>
{
	typedef MathExpressionBase<uint64_t> base;
	typedef FarColorExpression this_class;

public:
	FarColorExpression()
	{
		BinaryOperations.set_num_levels(4);

		BinaryOperations[1]->add(&operator_plus, "+");
		BinaryOperations[1]->add(&operator_minus, "-");
		BinaryOperations[2]->add(&operator_multiply, "*");
		BinaryOperations[2]->add(&operator_divide, "/");
		BinaryOperations[0]->add(&operator_or, "|");
		BinaryOperations[0]->add(&operator_and, "&");
		BinaryOperations[0]->add(&operator_mod, "%");
		BinaryOperations[0]->add(&operator_xor, "^");
		BinaryOperations[3]->add(&operator_shr, ">>");
		BinaryOperations[3]->add(&operator_shl, "<<");

		UnaryOperations.add(&operator_unary_not, "~");
		UnaryOperations.add(&operator_unary_logical_not, "!");
		UnaryOperations.add(&operator_unary_minus, "-");
		UnaryOperations.add(&operator_unary_plus, "+");

		add_function<f_rand>("RAND");
		add_function<f_rand>("rand");
		add_function<f_RGB>("RGB");
		add_function<f_RGB>("rgb");
		add_function<f_FORE_RGB>("FORE_RGB");
		add_function<f_FORE_RGB>("fore_rgb");
		add_function<f_FOREGROUND>("FOREGROUND");
		add_function<f_FOREGROUND>("foreground");
		add_function<f_BACK_RGB>("BACK_RGB");
		add_function<f_BACK_RGB>("back_rgb");
		add_function<f_BACKGROUND>("BACKGROUND");
		add_function<f_BACKGROUND>("background");
/*
		add_named_constant(0x0001, "FOREGROUND_BLUE");
		add_named_constant(0x0002, "FOREGROUND_GREEN");
		add_named_constant(0x0004, "FOREGROUND_RED");
		add_named_constant(0x0008, "FOREGROUND_INTENSITY");
		add_named_constant(0x0010, "BACKGROUND_BLUE");
		add_named_constant(0x0020, "BACKGROUND_GREEN");
		add_named_constant(0x0040, "BACKGROUND_RED");
		add_named_constant(0x0080, "BACKGROUND_INTENSITY");
		add_named_constant(0x0100, "FOREGROUND_TRUECOLOR");
		add_named_constant(0x0200, "BACKGROUND_TRUECOLOR");
*/
		for (auto i : ColorStyleFlags) {
			add_named_constant(i.first, i.second);
		}

		for (size_t i = 0; i < 16; i++) {
			add_named_constant(i, ForegroundColorNames[i]);
		}

		for (size_t i = 0; i < 16; i++) {
			add_named_constant(i << 4, BackgroundColorNames[i]);
		}
	}

protected:
	bool parse_number(uint64_t *value, const char *curpos, char **endptr);

	static uint64_t operator_unary_not(uint64_t a) { return ~a; }
	static uint64_t operator_unary_logical_not(uint64_t a) { return !a; }
	static uint64_t operator_or(uint64_t a, uint64_t b) { return a | b; }
	static uint64_t operator_mod(uint64_t a, uint64_t b) { return a % b; }
	static uint64_t operator_and(uint64_t a, uint64_t b) { return a & b; }
	static uint64_t operator_xor(uint64_t a, uint64_t b) { return a ^ b; }
	static uint64_t operator_shr(uint64_t a, uint64_t b) { return a >> b; }
	static uint64_t operator_shl(uint64_t a, uint64_t b) { return a << b; }
};

bool FarColorExpression::parse_number(uint64_t *value, const char *curpos, char **endptr)
{
	if (can_be_part_of_identifier(*curpos) && !isdigit(*curpos))
		return false;

	if (curpos[0] == '#' && isxdigit(curpos[1])) {
		uint32_t bgr = strtoul(curpos + 1, endptr, 16);
		*value = RGB_2_BGR(bgr);
		return *endptr > curpos;
	}

	if (curpos[0] == '0' && (curpos[1] == 'x' || curpos[1] == 'X')) {
		*value = strtoull(curpos + 2, endptr, 16);
		return *endptr > curpos;
	}

	*value = strtoull(curpos, endptr, 10);
	return *endptr > curpos;
}

#else
#include "colorexpdef.h"
#include <unordered_map>

#if 0
#include <functional>
#include <tuple>
#include <type_traits>

template<typename F>
struct function_traits : public function_traits<typename std::decay<F>::type> {};

template<typename R, typename... Args>
struct function_traits<R(Args...)> {
    static constexpr size_t arity = sizeof...(Args);
};

template<typename T>
constexpr bool is_function_v = std::is_function<T>::value;

template<typename T>
size_t get_num_args() {
    if (!is_function_v<T>) throw std::runtime_error("Not a function");
    
    using FT = function_traits<T>;
    return FT::arity;
}
#endif

typedef struct mtoken_s
{
	void *ref;
	uint8_t *name;
	uint32_t len;
	uint32_t id;
	uint64_t ui64Value;
	uint32_t index;
} mtoken_t;

struct FarColorExpression
{
	struct ffunction
	{
		void *fptr;
		size_t argnum;
	};

	mtoken_t *tc;
	mtoken_t tokens[128];
	uint32_t totalTokens;
	std::unordered_map<std::string, uint64_t> constnamesmap;
	std::unordered_map<std::string, ffunction> ffuncmap;

	uint32_t Tokenize(uint8_t *cmd, uint32_t len);
	uint64_t Parse(void);

	void eval_operator(const int op_id, uint64_t *lop, uint64_t *rop);
	mtoken_t *ParseOperand();
	mtoken_t *ParseLevel1();
	mtoken_t *ParseLevel2();
	mtoken_t *ParseLevel3();
	mtoken_t *ParseLevel4();
	mtoken_t *ParseLevel5();
	mtoken_t *ParseLevel6();
	mtoken_t *ParseLevel7();

	void add_function(uint64_t (*f)(uint64_t, uint64_t, uint64_t), const char *name)
	{
		ffuncmap[name] = {(void *)f, 3};
	}
	void add_function(uint64_t (*f)(uint64_t), const char *name) { ffuncmap[name] = {(void *)f, 1}; }
	void add_function(uint64_t (*f)(), const char *name) { ffuncmap[name] = {(void *)f, 0}; }

	FarColorExpression()
	{
		add_function(f_rand, "RAND");
		add_function(f_rand, "rand");
		add_function(f_RGB, "RGB");
		add_function(f_RGB, "rgb");

		add_function(f_FORE_RGB, "FORE_RGB");
		add_function(f_FORE_RGB, "fore_rgb");
		add_function(f_FOREGROUND, "FOREGROUND");
		add_function(f_FOREGROUND, "foreground");

		add_function(f_BACK_RGB, "BACK_RGB");
		add_function(f_BACK_RGB, "back_rgb");
		add_function(f_BACKGROUND, "BACKGROUND");
		add_function(f_BACKGROUND, "background");

		for (auto i : ColorStyleFlags) {
			constnamesmap[i.second] = i.first;
		}

		for (size_t i = 0; i < 16; i++) {
			constnamesmap[ForegroundColorNames[i]] = i;
		}

		for (size_t i = 0; i < 16; i++) {
			constnamesmap[BackgroundColorNames[i]] = i << 4;
		}

		constnamesmap["FOREGROUND_RED"] = 4;
	}
};

void FarColorExpression::eval_operator(const int op_id, uint64_t *lop, uint64_t *rop)
{
	switch (op_id) {
		case _TOKEN_ID_MUL:
			*lop *= *rop;
			break;
		case _TOKEN_ID_DIV:
			*lop /= *rop;
			break;
		case _TOKEN_ID_MOD:
			*lop %= *rop;
			break;
		case _TOKEN_ID_ADD:
			*lop += *rop;
			break;
		case _TOKEN_ID_SUB:
			*lop -= *rop;
			break;
		case _TOKEN_ID_SHR:
			*lop >>= *rop;
			break;
		case _TOKEN_ID_SHL:
			*lop <<= *rop;
			break;
		case _TOKEN_ID_OR:
			*lop |= *rop;
			break;
		case _TOKEN_ID_AND:
			*lop &= *rop;
			break;
		case _TOKEN_ID_XOR:
			*lop ^= *rop;
			break;
	}
}

mtoken_t *FarColorExpression::ParseLevel1()
{
	mtoken_t *val = ParseLevel2();
	return val;
}

mtoken_t *FarColorExpression::ParseLevel2()
{
	mtoken_t *val = ParseLevel3();
	uint32_t opId = _TOKEN_OP(tc->id);

	// Bit operators & | ^
	while (_TOKEN_TYPE(tc->id) == _TOKEN_TYPE_BINARY_OPERATOR && opId >= _TOKEN_ID_OR) {
		++tc;
		eval_operator(opId, &val->ui64Value, &ParseLevel3()->ui64Value);
		opId = _TOKEN_OP(tc->id);
	}

	return val;
}

mtoken_t *FarColorExpression::ParseLevel3()
{
	mtoken_t *val = ParseLevel4();
	uint32_t opId = _TOKEN_OP(tc->id);

	// Bit operators + - << >>
	while (_TOKEN_TYPE(tc->id) == _TOKEN_TYPE_BINARY_OPERATOR && opId >= _TOKEN_ID_ADD
			&& opId < _TOKEN_ID_OR) {
		++tc;
		eval_operator(opId, &val->ui64Value, &ParseLevel4()->ui64Value);
		opId = _TOKEN_OP(tc->id);
	}

	return val;
}

mtoken_t *FarColorExpression::ParseLevel4()
{
	mtoken_t *val = ParseLevel5();
	uint32_t opId = _TOKEN_OP(tc->id);

	// * / %
	while (_TOKEN_TYPE(tc->id) == _TOKEN_TYPE_BINARY_OPERATOR && opId < _TOKEN_ID_ADD) {
		++tc;
		eval_operator(opId, &val->ui64Value, &ParseLevel5()->ui64Value);
		opId = _TOKEN_OP(tc->id);
	}

	return val;
}

mtoken_t *FarColorExpression::ParseLevel5()
{
	mtoken_t *val;

	if (_TOKEN_TYPE(tc->id) == _TOKEN_TYPE_UNARY_OP_PREFIX) {
		mtoken_t *preftc = tc++;
		uint32_t num, opcode;

		while (_TOKEN_TYPE(tc->id) == _TOKEN_TYPE_UNARY_OP_PREFIX)
			++tc;

		num = (uint32_t)(tc - preftc);
		preftc = tc - 1;
		val = ParseLevel6();

		do {
			opcode = _TOKEN_OP(preftc->id);
			if (opcode == _TOKEN_ID_UNARY_PREFIX_MINUS) {
				val->ui64Value = -val->ui64Value;
			} else if (opcode == _TOKEN_ID_UNARY_NOT) {
				val->ui64Value = ~val->ui64Value;
			}
			--preftc;
		} while (--num);

		return val;
	}

	val = ParseLevel6();

	return val;
}

mtoken_t *FarColorExpression::ParseLevel6()
{
	mtoken_t *val;

	if (tc->id == _TOKEN_TYPE_FUNCTION_OPEN) {
		mtoken_t *fnt = tc;
		uint64_t params[8];

		if (tc->index) {
			for (uint32_t i = 0; i < fnt->index; ++i) {
				++tc;
				val = ParseLevel2();
				params[i] = val->ui64Value;
			}
		} else
			++tc;

		val = tc;
		if (tc->id != _TOKEN_TYPE_FUNCTION_CLOSE) {
		}
		++tc;
		val->id = _TOKEN_TYPE_IMM;

		if (!fnt->index) {
			uint64_t (*fn)() = (uint64_t (*)())fnt->ref;
			val->ui64Value = fn();
		} else if (fnt->index == 1) {
			uint64_t (*fn)(uint64_t) = (uint64_t (*)(uint64_t))fnt->ref;
			val->ui64Value = fn(params[0]);
		} else if (fnt->index == 3) {
			uint64_t (*fn)(uint64_t, uint64_t, uint64_t) = (uint64_t (*)(uint64_t, uint64_t, uint64_t))fnt->ref;
			val->ui64Value = fn(params[0], params[1], params[2]);
		}
		return val;
	}

	val = ParseLevel7();

	return val;
}

mtoken_t *FarColorExpression::ParseLevel7()
{
	mtoken_t *val;

	if (tc->id == _TOKEN_TYPE_BRACE_OPEN) {
		++tc;
		val = ParseLevel2();

		if (tc->id != _TOKEN_TYPE_BRACE_CLOSE) {
		}
		++tc;
		return val;
	}

	if (_TOKEN_TYPE(tc->id) == _TOKEN_TYPE_IMM) {
		val = tc++;
		return val;
	}

	val = tc++;
	return val;
}

uint64_t FarColorExpression::Parse(void)
{
	tc = &tokens[0];
	mtoken_t *val = tc;

	if (!totalTokens)
		return 0;

	while (tc < &tokens[totalTokens]) {
		// z_cmdtoken_t *ftc = tc;
		val = ParseLevel1();
	}

	return val->ui64Value;
}

#define T_SkipWhiteSpace()                                                                                   \
	while ((tchtab[*cmd] & TCH_BLANK) && len) {                                                              \
		++cmd;                                                                                               \
		--len;                                                                                               \
	}

#define T_GetID()                                                                                            \
	while ((tchtab[*cmd] & TCH_ID) && len) {                                                                 \
		++cmd;                                                                                               \
		--len;                                                                                               \
	}

#define T_SkipZeros()                                                                                        \
	while (*cmd == '0' && len) {                                                                             \
		++cmd;                                                                                               \
		--len;                                                                                               \
	}

uint32_t FarColorExpression::Tokenize(uint8_t *cmd, uint32_t len)
{
	uint32_t tokpermission;
	uint32_t nesting[32];
	int32_t feedparms[32];
	uint32_t level;

	mtoken_t *t = &tokens[0];

	if (len == 0xFFFFFFFF)
		len = strlen((const char *)cmd);

//	fprintf(stderr, "len = %u cmd = [%s]\n", len, cmd );

	this->totalTokens = 0;
	if (!len)
		return 0;

	tokpermission = TOKEN_PERM_UNARY_OP_PREFIX | TOKEN_PERM_FUNCTION_OPEN | TOKEN_PERM_VAR | TOKEN_PERM_IMM
			| TOKEN_PERM_BRACE_OPEN;

	T_SkipWhiteSpace();
	if (!len)
		return 0;

	level = 0;
	nesting[level] = 0;
	feedparms[level] = 0;

	while (1) {
		T_SkipWhiteSpace();
		if (!len)
			break;

		if (*cmd == ',') {	 // ,

			if (!(tokpermission & TOKEN_PERM_VIRGULE))
				return TOK_ERR_UNEXPECTED_VIRGULE;
			if (!level)
				return TOK_ERR_NOT_INSIDE_COMM_OR_FUNC;
			if (nesting[level])
				return TOK_ERR_UNCLOSED_BRACE;
			if (--feedparms[level] < 1)
				return TOK_ERR_TOO_MANY_ARGS;

			++totalTokens;
			t->name = cmd;
			t->id = _TOKEN_TYPE_VIRGULE;
			t->len = 1;

			++cmd;
			if (!--len) {
				return TOK_ERR_UNEXPECTED_END_OF_EXPR;
			}

			++t;
			tokpermission = TOKEN_PERM_UNARY_OP_PREFIX | TOKEN_PERM_FUNCTION_OPEN | TOKEN_PERM_VAR
					| TOKEN_PERM_IMM | TOKEN_PERM_BRACE_OPEN;
			continue;
		}

		if (*cmd == '(') {	 // (

			if (!(tokpermission & TOKEN_PERM_BRACE_OPEN))
				return TOK_ERR_NOT_ALLOWED_BRACE_OPEN;

			++nesting[level];

			++totalTokens;
			t->name = cmd;
			t->id = _TOKEN_TYPE_BRACE_OPEN;
			t->len = 1;

			++cmd;
			if (!--len)
				return TOK_ERR_END_WITH_OPEN_BRACE;

			++t;
			tokpermission = TOKEN_PERM_UNARY_OP_PREFIX | TOKEN_PERM_FUNCTION_OPEN | TOKEN_PERM_VAR
					| TOKEN_PERM_IMM | TOKEN_PERM_BRACE_OPEN;
			continue;
		}

		if (*cmd == ')') {	 // )
			if (!nesting[level]) {
				if (!(tokpermission & TOKEN_PERM_FUNCTION_CLOSE))
					return TOK_ERR_CANNOT_CLOSE_BRACE_HERE;
				if (!level)
					return TOK_ERR_UNEXPECTED_CLOSE_BRACE;
				if (feedparms[level] > 1)
					return TOK_ERR_NEED_MORE_ARGS;

				++totalTokens;
				t->name = cmd;
				t->id = _TOKEN_TYPE_FUNCTION_CLOSE;
				t->len = 1;

				tokpermission = TOKEN_PERM_BINARY_OPERATOR | TOKEN_PERM_FUNCTION_CLOSE | TOKEN_PERM_VIRGULE
						| TOKEN_PERM_BRACE_CLOSE | TOKEN_PERM_SEPARATOR;
				--level;
				++cmd;

				if (!--len)
					break;
				++t;
				continue;
			}

			if (!(tokpermission & TOKEN_PERM_BRACE_CLOSE))
				return TOK_ERR_CANNOT_CLOSE_BRACE_HERE;
			if (!(nesting[level] & 65535))
				return TOK_ERR_UNEXPECTED_CLOSE_BRACE;

			--nesting[level];

			++totalTokens;
			t->name = cmd;
			t->id = _TOKEN_TYPE_BRACE_CLOSE;
			t->len = 1;

			++cmd;
			if (!--len)
				break;

			++t;
			tokpermission = TOKEN_PERM_BINARY_OPERATOR | TOKEN_PERM_FUNCTION_CLOSE | TOKEN_PERM_VIRGULE
					| TOKEN_PERM_BRACE_CLOSE | TOKEN_PERM_SEPARATOR;
			continue;
		}

		if (tchtab[*cmd] & TCH_LETTER) {
			if (!(tokpermission & (TOKEN_PERM_VAR | TOKEN_PERM_FUNCTION_OPEN))) {

				return TOK_ERR_INVALID_TOKEN;
			}

			++this->totalTokens;
			t->name = cmd;
			T_GetID();
			t->len = (uint32_t)(cmd - t->name);

			if (t->len > _TOKEN_MAX_ID_LEN) {
				return TOK_ERR_INVALID_TOKEN;
			}

			T_SkipWhiteSpace();

			if (len && *cmd == '(') {	// (

				t->id = _TOKEN_TYPE_FUNCTION_OPEN;
				if (!(tokpermission & TOKEN_PERM_FUNCTION_OPEN))
					return TOK_ERR_CANNOT_OPEN_FUNC_HERE;
				if (++level >= 32)
					return TOK_ERR_NESTING_OVERFLOW;

				nesting[level] = 0;
				ffunction *ref = NULL;

				auto it = ffuncmap.find(std::string((const char *)t->name, t->len));
				if (it == ffuncmap.end()) {
					// not found
					return 9;
				}

				ref = &it->second;
				t->ref = ref->fptr;
				t->index = ref->argnum;

				++cmd;	  // skip (
				if (!--len)
					return TOK_ERR_END_WITH_OPEN_BRACE;
				++t;

				if (!ref->argnum) {
					tokpermission = TOKEN_PERM_FUNCTION_CLOSE;
				} else {
					tokpermission = TOKEN_PERM_UNARY_OP_PREFIX | TOKEN_PERM_FUNCTION_OPEN | TOKEN_PERM_VAR
							| TOKEN_PERM_IMM | TOKEN_PERM_BRACE_OPEN;
				}
				feedparms[level] = ref->argnum;
				continue;
			}

			t->id = _TOKEN_TYPE_IMM;
			if (!(tokpermission & TOKEN_PERM_VAR))
				return TOK_ERR_NOT_ALLOWED_VAR_HERE;

			auto it = constnamesmap.find(std::string((const char *)t->name, t->len));
			if (it == constnamesmap.end()) {
				// not found
				return 999;
			}

			t->ui64Value = it->second;
			++t;
			tokpermission = TOKEN_PERM_BINARY_OPERATOR | TOKEN_PERM_UNARY_OP_POSTFIX
					| TOKEN_PERM_FUNCTION_CLOSE | TOKEN_PERM_VIRGULE | TOKEN_PERM_BRACE_CLOSE;

			continue;
		}

		if ((tchtab[*cmd] & TCH_DIGIT) || *cmd == '#') {	 // IMM

			uint64_t value = 0;

			++totalTokens;
			t->id = _TOKEN_TYPE_IMM;
			t->name = cmd;

			if (!(tokpermission & TOKEN_PERM_IMM))
				return TOK_ERR_IMM_NOT_ALLOWED_HERE;

			tokpermission = TOKEN_PERM_BINARY_OPERATOR | TOKEN_PERM_FUNCTION_CLOSE | TOKEN_PERM_VIRGULE
					| TOKEN_PERM_BRACE_CLOSE | TOKEN_PERM_SEPARATOR;

			if (cmd[1] == 'X' || cmd[1] == 'x') {
				cmd += 2;
				value = strtoull((const char *)cmd, (char **)&cmd, 16);
			}
			else if (cmd[0] == '#') {
				cmd ++;
				uint32_t bgr = strtoul((const char *)cmd, (char **)&cmd, 16);
				value = RGB_2_BGR(bgr);
//				value = strtoull((const char *)cmd, (char **)&cmd, 16);
			}
			else {
				value = strtoull((const char *)cmd, (char **)&cmd, 10);
			}

			t->ui64Value = value;
			t->len = (uint32_t)(cmd - t->name);
			len -= t->len;

			if (!len)
				break;

			++t;
			continue;
		}

		if (tchtab[*cmd] & TCH_OPERATOR) {	  // op

			if (tokpermission & TOKEN_PERM_UNARY_OP_PREFIX) {
				++totalTokens;
				t->name = cmd;

				if (*cmd == 126) {	  // ~
					if (!(tokpermission & TOKEN_PERM_UNARY_OP_ARITH_PREFIX))
						return TOK_ERR_UNEXPECTED_ARITH_OP;

					++cmd;
					if (!--len)
						return TOK_END_OF_EXPR_WITH_BINOP;

					t->id = _TOKEN_TYPE_UNARY_OP_PREFIX | _TOKEN_ID_UNARY_NOT;
					t->len = 1;
					++t;
					tokpermission = TOKEN_PERM_UNARY_OP_PREFIX | TOKEN_PERM_VAR | TOKEN_PERM_IMM
							| TOKEN_PERM_BRACE_OPEN | TOKEN_PERM_FUNCTION_OPEN;
					continue;
				}

				if (*cmd == 43) {	 // +

					++cmd;
					if (!--len) {
						return TOK_END_OF_EXPR_WITH_BINOP;
					}

					if (!(tokpermission & TOKEN_PERM_UNARY_OP_ARITH_PREFIX)) {
						return TOK_ERR_UNEXPECTED_ARITH_OP;
					}

					t->id = _TOKEN_TYPE_UNARY_OP_PREFIX | _TOKEN_ID_UNARY_PREFIX_PLUS;
					t->len = 1;

					++t;

					tokpermission = TOKEN_PERM_UNARY_OP_PREFIX | TOKEN_PERM_VAR | TOKEN_PERM_IMM
							| TOKEN_PERM_BRACE_OPEN | TOKEN_PERM_FUNCTION_OPEN;

					continue;
				}

				if (*cmd == 45) {	 // -

					++cmd;
					if (!--len)
						return TOK_END_OF_EXPR_WITH_BINOP;

					if (!(tokpermission & TOKEN_PERM_UNARY_OP_ARITH_PREFIX))
						return TOK_ERR_UNEXPECTED_ARITH_OP;

					t->id = _TOKEN_TYPE_UNARY_OP_PREFIX | _TOKEN_ID_UNARY_PREFIX_MINUS;
					t->len = 1;

					++t;
					tokpermission = TOKEN_PERM_UNARY_OP_PREFIX | TOKEN_PERM_VAR | TOKEN_PERM_IMM
							| TOKEN_PERM_BRACE_OPEN | TOKEN_PERM_FUNCTION_OPEN;

					continue;
				}

				return TOK_ERR_UNKNOWN_PREFIX_OP;
			}

			if (tokpermission & TOKEN_PERM_BINARY_OPERATOR) {
				uint8_t fch = *cmd;
				uint8_t dop = OpLut[fch];

				++this->totalTokens;
				t->name = cmd;
				t->len = 1;

				t->id = _TOKEN_TYPE_BINARY_OPERATOR;
				++cmd;
				if (!--len)
					return TOK_END_OF_EXPR_WITH_BINOP;

				if (dop) {
					if (dop & Z_OPEARTOR_DOUBLE_CH) {
						if (*cmd != fch)
							return TOK_ERR_SYNTAX_ERROR;
						++cmd;
						if (!--len)
							return TOK_END_OF_EXPR_WITH_BINOP;
					}

					t->id |= dop & 15;
					++t;
					tokpermission = TOKEN_PERM_UNARY_OP_PREFIX | TOKEN_PERM_FUNCTION_OPEN | TOKEN_PERM_VAR
							| TOKEN_PERM_IMM | TOKEN_PERM_BRACE_OPEN;
					continue;
				}

				return TOK_ERR_UNKNOWN_BINOP;
			}
			return TOK_ERR_UNKNOWN_OP;
		}

		return TOK_ERR_INVALID_TOKEN;
		++cmd;

		if (!--len)
			return 0;
	}

	if (level > 1) {
		return TOK_ERR_UNEXPECTED_END_OF_EXPR_WITHIN_FUNC;
	}

	if (level)
		return TOK_ERR_UNEXPECTED_END_OF_EXPR_WITHIN_FUNC_OR_CMD;

	if (nesting[level])
		return TOK_ERR_END_OF_EXPR_UNCLOSED_BRACE;

	if (feedparms[level] > 1)
		return TOK_ERR_PARMS_NOT_FEED;

	return 0;
}
#endif

FarColorExpression fcolorparser;

bool ExprToFarColor(const char *exsp, uint64_t &c)
{
	if (!exsp) return false;
#ifdef __USE_MATHEXPRESSION_H
	if (!fcolorparser.parse(exsp))
		return false;

	c = fcolorparser.eval();
#else

//	uint32_t rez = 	fcolorparser.Tokenize((uint8_t *)exsp, -1);

//	fprintf(stderr, "*** tkoenizing... %s TOKEN REZULT = %u \n", exsp, rez );

	if (fcolorparser.Tokenize((uint8_t *)exsp, -1) != 0)
		return false;

	c = fcolorparser.Parse();
#endif

	return true;
}

size_t FarColorToExpr(uint64_t c, char *exsp, size_t s)
{
	bool addop = false;
	size_t n = 0;

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

	n += snprintf(exsp + n, s - n, "(%s | %s)", BackgroundColorNames[(c >> 4) & 0xF],
			ForegroundColorNames[c & 0xF]);

	for (auto i : ColorStyleFlags) {
		if (c & i.first) {
			exsp[n++] = ' ';
			exsp[n++] = '+';
			exsp[n++] = ' ';
			n += snprintf(exsp + n, s - n, "%s", i.second);
		}
	}

	return n;
}
