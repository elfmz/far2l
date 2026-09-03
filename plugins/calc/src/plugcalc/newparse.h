//
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#ifndef CALC_NEWPARSE_H
#define CALC_NEWPARSE_H

#include <sgml/sgml.h>
#include <unordered_map>
#undef min
#undef max
#include <mathexpression/MathExpressionBase.h>

#include "sarg.h"
#include "syntax.h"


enum CALC_ERROR
{
	ERR_OK = 0,
	ERR_ALL, ERR_EXPR, ERR_TYPE,
	ERR_PARAM, ERR_BRACK, ERR_ZERO, ERR_FLOW, ERR_TOOLONG
};

enum CALC_CONV_TYPE
{
	CALC_CONV_DECPT = -1,
	CALC_CONV_DECEXP = -2,
	CALC_CONV_ENTER = -3,
	CALC_CONV_UNITS = -4,
};

class CalcParser;

class CalcAddonPart
{
public:
	CalcAddonPart()
	{
		parser = NULL;
	}
	CalcAddonPart(const CalcAddonPart & p);

	~CalcAddonPart();

	void Parse(bool no_args = false);

public:
	friend class CalcParser;
	std::wstring expr;
	CalcParser *parser;	/// pre-parsed pattern expression
	int str_pos;		/// abs. string position for replacement
};

class CalcAddon
{
public:
	CalcAddon()
	{
		radix = 10;
		flags = CALC_ADDON_FLAG_NONE;
	}

public:
	std::wstring name;
	std::wstring expr;

	int radix, flags;

	std::vector<CalcAddonPart> parts;
};

struct SDialogElem
{
	wchar_t Name[64];
	int Type;
	Big scale;
	int addon_idx;
	int column_idx;

	CalcAddonPart *input, *scale_expr;

	// PEditObject Edit;
	SDialogElem *Next;
	SDialogElem();
	~SDialogElem();
};

typedef SDialogElem *PDialogElem;

struct SDialogData
{
	wchar_t Name[64];
	int num;
	SDialogElem *Elem;
	SDialogData *Next;
	SDialogData();
	~SDialogData();
};

typedef SDialogData *PDialogData;

wchar_t *convertToString(SArg val, int type_idx, int num_lim, bool append_suffix, bool pad_zeroes, bool group_delim);
void print_repeating_decimal(std::wstring & s, SArg val, int num_lim, bool group_delim);
void print_continued_decimal(std::wstring & s, SArg val, int num_lim, bool group_delim);
int CalcMenu(int c);
void SetUnitsDialogDims();
void ShowUnitsDialog(int no);
struct SArg;

class CalcParser : public MathExpressionBase<SArg>
{
	typedef MathExpressionBase<SArg> mybase;

	friend wchar_t *convertToString(SArg val, int type_idx, int num_lim, bool append_suffix, bool pad_zeroes, bool group_delim);
	friend void print_repeating_decimal(std::wstring & s, SArg val, int num_lim, bool group_delim);
	friend void print_continued_decimal(std::wstring & s, SArg val, int num_lim, bool group_delim);
	friend int CalcMenu(int c);
	friend void SetUnitsDialogDims();
	friend void ShowUnitsDialog(int no);
	friend struct SArg;
	friend class CalcAddonPart;

public:
	typedef SArg value_type;

	CalcParser();
	CalcParser(const CalcParser & p);

	~CalcParser();

	static bool InitTables(int rep_fraction_max_start, int rep_fraction_max_period, int cont_fraction_max);
	static bool ProcessData(PSgmlEl BaseRc, bool case_sensitive);
	static void FillDialogData(PSgmlEl Base, bool case_sensitive, const wchar_t *lang_name);
	static bool SetDelims(wchar_t decimal, wchar_t args, wchar_t digit);
	static bool ProcessAddons();
	static bool ProcessDialogData();
	static int  GetNumDialogs();

	/// Add all functs, ops, consts
	static bool AddAll(bool add_user_ops_and_funcs = true);

	static void GetFraction(Big b, BigInt *numer, BigInt *denom);
	static void RoundUp(Big &b);

	SArg Parse(const wchar_t* str, bool case_sensitive);

	CALC_ERROR GetError();

public:
	/// parsed addons
	static std::vector<CalcAddon> addons;
	static unsigned main_addons_num;
	static wchar_t delim_decimal, delim_args, delim_digit;

	static ttmath::Conv from_convs[17], to_convs[17];

protected:

	bool parse_number(SArg *value, const wchar_t *curpos, wchar_t **endptr);

	static bool FillSet(PSgmlEl set, bool case_sensitive);
	static bool AddLexem(PSyntax &syntax, PSgmlEl Ch, PSgmlEl set, bool case_sensitive);
	static int  DelSpaces(wchar_t *str);

	static std::wstring ReplaceDelims(const wchar_t *str);

	bool SetVar(wchar_t *name, SArg value);

	static PSyntax Consts;
	static PSyntax Ops;
	static PSyntax Functs;
	static PSyntax Addons;
	static PSyntax Numerals;
	static PVars   Vars;

protected:

	static SArg builtin_binary_op(const SArg & op0, const SArg & op1);
	static SArg builtin_unary_not(const SArg & op);

	static SArg binary_op(const SArg & op0, const SArg & op1);
	static SArg unary_op(const SArg & op0);

	class UserFunctionList : public std::vector<CalcParser *>
	{
	public:
		UserFunctionList()	{}
		~UserFunctionList();
	};

protected:
	CALC_ERROR math_error;
	string func_name;

	static std::unordered_map<std::wstring, CalcParser *> user_bin_ops, user_un_ops;

protected:
	static UserFunctionList *user_funcs;

	static FunctionList allFunctions;
	static NamedConstantList allNamedConstants;
	static UnaryOperationTable allUnaryOpTable;
	static BinaryOperationTable allBinaryOpTable;

	static std::vector<BigInt> rep_fraction_coefs;
	static std::vector<Big> rep_mul1, rep_mul2;
	static Big rep_fraction_thr, rep_fraction_thr2;
	static int rep_fraction_max_start, rep_fraction_max_period;
	static int cont_fraction_max;

	static PDialogData DialogData;
	static int DialogsNum;

};

/// if type_idx >= 0 then it's index to addons, or type enum if < 0
wchar_t *convertToString(const SArg & val, int type_idx, int num_lim = 0, bool append_suffix = false, bool pad_zeroes = true, bool group_delim = true, CALC_ERROR *error_code = NULL);

#endif // of CALC_NEWPARSE_H
