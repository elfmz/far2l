//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//


#include <ttmath/ttmath.h>

#ifndef SARG_H
#define SARG_H

typedef ttmath::Big<1,26> Big;
typedef ttmath::Int<24> BigInt;
typedef ttmath::UInt<24> BigUInt;

enum EArgType
{
	SA_BIG,
	SA_INT64, SA_UINT64, SA_INT, SA_SHORT, SA_CHAR,
	SA_UINT, SA_USHORT, SA_BYTE, SA_DOUBLE, SA_FLOAT,
};

struct SArg
{
private:
	EArgType type;
	bool empty;
	int  size;
	union
	{
		int64_t v_int64;
		int     v_int;
		short   v_short;
		char    v_char;
		uint64_t v_uint64;
		unsigned int   v_uint;
		unsigned short v_ushort;
		unsigned char  v_byte;
		double  v_dbl;
		float   v_flt;

	} arg {};

	// XXX:
	Big v_big;

public:
	SArg();
	~SArg();
	SArg(const SArg &a);

	SArg(int64_t var);
	SArg(bool var);
	SArg(short var);
	SArg(char var);
	SArg(int var);
	SArg(uint64_t var);
	SArg(unsigned int var);
	SArg(unsigned short var);
	SArg(unsigned char var);
	SArg(double var);
	SArg(float var);

	SArg(const Big & var);



	bool isempty() const;
	int  varsize() const;
	EArgType gettype();

	bool IsFixedLength() const;

	SArg &operator=(const SArg & op);
	// XXX:
	//SArg &operator=(double op);

	friend SArg operator+ (const SArg & op1, const SArg & op2);
	friend SArg operator- (const SArg & op1, const SArg & op2);
	friend SArg operator* (const SArg & op1, const SArg & op2);
	friend SArg operator/ (const SArg & op1, const SArg & op2);
	friend SArg operator| (const SArg & op1, const SArg & op2);
	friend SArg operator& (const SArg & op1, const SArg & op2);
	friend SArg operator^  (const SArg & op1, const SArg & op2);
	friend SArg operator>> (const SArg & op1, int n);
	friend SArg operator<< (const SArg & op1, int n);

	friend bool operator== (const SArg & op1, const SArg & op2);
	friend bool operator!= (const SArg & op1, const SArg & op2);
	friend bool operator>  (const SArg & op1, const SArg & op2);
	friend bool operator<  (const SArg & op1, const SArg & op2);

	SArg Ror(const SArg & op) const;
	SArg Rol(const SArg & op) const;

	SArg Pow(const SArg & op) const;

	SArg operator-  ();
	SArg operator%  (const SArg & op) const;


	SArg operator~  () const;


	SArg operator+= (const SArg & op);
	SArg operator-= (const SArg & op);

	operator int64_t() const;
	operator int() const;
	operator short() const;
	operator char() const;
	operator uint64_t() const;
	operator unsigned int() const;
	operator unsigned short() const;
	operator unsigned char() const;
	operator float() const;
	operator double() const;
	operator bool() const;

	Big GetBig() const;
	Big GetInt() const;

	// Print fixed length numbers
	bool Print(std::wstring &str, int radix, int num_delim, wchar_t delim_digit);

	static Big GreatestCommonDiv(Big numer, Big denom);

public:

	static SArg _factor(const SArg & op1);
	static SArg _frac(const SArg & op1);
	static SArg _floor(const SArg & op1);
	static SArg _ceil(const SArg & op1);
	static SArg _sin(const SArg & op1);
	static SArg _cos(const SArg & op1);
	static SArg _tan(const SArg & op1);
	static SArg _arctan(const SArg & op1);
	static SArg _ln(const SArg & op1);
	static SArg _rnd();
	//static SArg _sum", 1 },
	//static SArg _avr", 1 },
	static SArg _if(const SArg & op1, const SArg & op2, const SArg & op3);

	static SArg _f2b(const SArg & op1);
	static SArg _d2b(const SArg & op1);
	static SArg _b2f(const SArg & op1);
	static SArg _b2d(const SArg & op1);
	static SArg _finf();
	static SArg _fnan();

	static SArg _numer(const SArg & op1);
	static SArg _denom(const SArg & op1);
	static SArg _gcd(const SArg & op1, const SArg & op2);

	static SArg to_int64(const SArg & op1);
	static SArg to_uint64(const SArg & op1);
	static SArg to_int(const SArg & op1);
	static SArg to_uint(const SArg & op1);
	static SArg to_short(const SArg & op1);
	static SArg to_ushort(const SArg & op1);
	static SArg to_char(const SArg & op1);
	static SArg to_byte(const SArg & op1);
	static SArg to_double(const SArg & op1);
	static SArg to_float(const SArg & op1);
};

typedef struct SArg *PArg;

#endif // of SARG_H

