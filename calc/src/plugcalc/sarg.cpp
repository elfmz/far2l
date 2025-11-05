//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits>

#include "newparse.h"
#include "sarg.h"
#include "syntax.h"


SArg::SArg()
{
	type = SA_BIG;
	v_big = 0;
	arg.v_int64 = 0;
	empty = true;
}

SArg::~SArg()
{
}

SArg::SArg(const SArg &a)
{
	type  = a.type;
	arg   = a.arg;
	v_big = a.v_big;
	empty = a.empty;
}

SArg::SArg(int64_t var)
{
	type = SA_INT64;
	arg.v_int64 = var;
	empty = false;
}

SArg::SArg(bool var)
{
	type = SA_INT;
	arg.v_int = var;
	empty = false;
}

SArg::SArg(int var)
{
	type = SA_INT;
	arg.v_int = var;
	empty = false;
}

SArg::SArg(short var)
{
	type = SA_SHORT;
	arg.v_short = var;
	empty = false;
}

SArg::SArg(char var)
{
	type = SA_CHAR;
	arg.v_char = var;
	empty = false;
}

SArg::SArg(uint64_t var)
{
	type = SA_UINT64;
	arg.v_uint64 = var;
	empty = false;
}

SArg::SArg(unsigned int var)
{
	type = SA_UINT;
	arg.v_uint = var;
	empty = false;
}

SArg::SArg(unsigned short var)
{
	type = SA_USHORT;
	arg.v_ushort = var;
	empty = false;
}

SArg::SArg(unsigned char var)
{
	type = SA_BYTE;
	arg.v_byte = var;
	empty = false;
}

SArg::SArg(const Big & var)
{
	type = SA_BIG;
	v_big = var;
	empty = false;
}

SArg::SArg(float var)
{
	type = SA_FLOAT;
	arg.v_flt = var;
	empty = false;
}

SArg::SArg(double var)
{
	type = SA_DOUBLE;
	arg.v_dbl = var;
	empty = false;
}

//////////////////////////// operators /////////////////////////////////

SArg &SArg::operator=(const SArg & op)
{
	type = op.type;
	empty = op.empty;
	arg = op.arg;
	v_big = op.v_big;
	return *this;
}

SArg operator+(const SArg & op1, const SArg & op2)
{
	if ((op1.type == SA_DOUBLE || op1.type == SA_FLOAT) && (op2.type == SA_FLOAT || op2.type == SA_DOUBLE))
	{
		if (op1.type == SA_FLOAT && op2.type == SA_FLOAT)
			return (float)op1 + (float)op2;
		return (double)op1 + (double)op2;
	}
	if (op2.type == SA_BIG || op2.type == SA_DOUBLE || op2.type == SA_FLOAT)
		return op1.GetBig() + op2.GetBig();
	switch(op1.type)
	{
	case SA_BIG: return op1.v_big + op2.GetBig();

	case SA_INT64:  return op1.arg.v_int64 + (int64_t)op2;
	case SA_INT:    return op1.arg.v_int + (int)op2;
	case SA_SHORT:  return short(op1.arg.v_short + (short)op2);
	case SA_CHAR:   return char(op1.arg.v_char + (char)op2);
	case SA_UINT64: return op1.arg.v_uint64 + (uint64_t)op2;
	case SA_UINT:   return op1.arg.v_uint + (unsigned int)op2;
	case SA_USHORT: return (unsigned short)(op1.arg.v_ushort + (unsigned short)op2);
	case SA_BYTE:   return (unsigned char)(op1.arg.v_byte + (unsigned char)op2);
	case SA_DOUBLE: return op1.arg.v_dbl + (double)op2;
	case SA_FLOAT:  return op1.arg.v_flt + (float)op2;
	}
	throw ERR_TYPE;
}

SArg SArg::operator-()
{
	SArg arg;
	arg.type = this->type;
	return arg - *this;
}

SArg operator- (const SArg & op1, const SArg & op2)
{
	if ((op1.type == SA_DOUBLE || op1.type == SA_FLOAT) && (op2.type == SA_FLOAT || op2.type == SA_DOUBLE))
	{
		if (op1.type == SA_FLOAT && op2.type == SA_FLOAT)
			return (float)op1 - (float)op2;
		return (double)op1 - (double)op2;
	}
	if (op2.type == SA_BIG || op2.type == SA_DOUBLE || op2.type == SA_FLOAT)
		return op1.GetBig() - op2.GetBig();
	switch(op1.type)
	{
	case SA_BIG: return op1.v_big - op2.GetBig();

	case SA_INT64:  return op1.arg.v_int64 - (int64_t)op2;
	case SA_INT:    return op1.arg.v_int - (int)op2;
	case SA_SHORT:  return short(op1.arg.v_short - (short)op2);
	case SA_CHAR:   return char(op1.arg.v_char - (char)op2);
	case SA_UINT64: return op1.arg.v_uint64 - (uint64_t)op2;
	case SA_UINT:   return op1.arg.v_uint - (unsigned int)op2;
	case SA_USHORT: return (unsigned short)(op1.arg.v_ushort - (unsigned short)op2);
	case SA_BYTE:   return (unsigned char)(op1.arg.v_byte - (unsigned char)op2);
	case SA_DOUBLE: return op1.arg.v_dbl - (double)op2;
	case SA_FLOAT:  return op1.arg.v_flt - (float)op2;
	}
	throw ERR_TYPE;
}

SArg SArg::operator% (const SArg & op) const
{
	if (op.GetBig() == 0) throw ERR_ZERO;
	Big bi = this->GetBig();
	return ttmath::Mod(bi, op.GetBig());
}

SArg operator/ (const SArg & op1, const SArg & op2)
{
	if ((op1.type == SA_DOUBLE || op1.type == SA_FLOAT) && (op2.type == SA_FLOAT || op2.type == SA_DOUBLE))
	{
		if (op1.type == SA_FLOAT && op2.type == SA_FLOAT)
			return (float)op1 / (float)op2;
		return (double)op1 / (double)op2;
	}
	if (op2.GetBig() == 0)
		throw ERR_ZERO;
	return op1.GetBig() / op2.GetBig();
}

SArg operator* (const SArg & op1, const SArg & op2)
{
	if ((op1.type == SA_DOUBLE || op1.type == SA_FLOAT) && (op2.type == SA_FLOAT || op2.type == SA_DOUBLE))
	{
		if (op1.type == SA_FLOAT && op2.type == SA_FLOAT)
			return (float)op1 * (float)op2;
		return (double)op1 * (double)op2;
	}
	if (op2.type == SA_BIG || op2.type == SA_DOUBLE || op2.type == SA_FLOAT)
		return op1.GetBig() * op2.GetBig();
	switch(op1.type)
	{
	case SA_BIG: return op1.v_big * op2.GetBig();

	case SA_INT64:  return op1.arg.v_int64 * (int64_t)op2;
	case SA_INT:    return op1.arg.v_int * (int)op2;
	case SA_SHORT:  return short(op1.arg.v_short * (short)op2);
	case SA_CHAR:   return char(op1.arg.v_char * (char)op2);
	case SA_UINT64: return op1.arg.v_uint64 * (uint64_t)op2;
	case SA_UINT:   return op1.arg.v_uint * (unsigned int)op2;
	case SA_USHORT: return (unsigned short)(op1.arg.v_ushort * (unsigned short)op2);
	case SA_BYTE:   return (unsigned char)(op1.arg.v_byte * (unsigned char)op2);
	case SA_DOUBLE: return op1.arg.v_dbl * (double)op2;
	case SA_FLOAT:  return op1.arg.v_flt * (float)op2;
	}
	throw ERR_TYPE;
}

SArg SArg::operator~() const
{
	switch(type)
	{
	case SA_INT64:  return ~arg.v_int64;
	case SA_INT:    return ~arg.v_int;
	case SA_SHORT:  return (short)~arg.v_short;
	case SA_CHAR:   return (char)~arg.v_char;
	case SA_UINT64: return ~arg.v_uint64;
	case SA_UINT:   return ~arg.v_uint;
	case SA_USHORT: return (unsigned short)~arg.v_ushort;
	case SA_BYTE:   return (unsigned char)~arg.v_byte;

	case SA_BIG:
	case SA_DOUBLE:
	case SA_FLOAT:
		 {
			BigInt bint;
			this->GetBig().ToInt(bint);
			bint.BitNot2();
			Big bi;
			bi.FromInt(bint);
			return bi;
		 }
	}
	throw ERR_TYPE;
}

SArg operator| (const SArg & op1, const SArg & op2)
{
	switch(op1.type)
	{
	case SA_INT64:  return op1.arg.v_int64 | (int64_t)op2;
	case SA_INT:    return op1.arg.v_int | (int)op2;
	case SA_SHORT:  return short(op1.arg.v_short | (short)op2);
	case SA_CHAR:   return char(op1.arg.v_char | (char)op2);
	case SA_UINT64: return op1.arg.v_uint64 | (uint64_t)op2;
	case SA_UINT:   return op1.arg.v_uint | (unsigned int)op2;
	case SA_USHORT: return (unsigned short)(op1.arg.v_ushort | (unsigned short)op2);
	case SA_BYTE:   return (unsigned char)(op1.arg.v_byte | (unsigned char)op2);

	case SA_BIG:
	case SA_DOUBLE:
	case SA_FLOAT:
		{ Big bi = op1.GetInt(); if (bi.BitOr(op2.GetInt()) != 0) throw ERR_PARAM; return bi; }
	}
	throw ERR_TYPE;
}

SArg operator^ (const SArg & op1, const SArg & op2)
{
	switch(op1.type)
	{
	case SA_INT64:  return op1.arg.v_int64 ^ (int64_t)op2;
	case SA_INT:    return op1.arg.v_int ^ (int)op2;
	case SA_SHORT:  return short(op1.arg.v_short ^ (short)op2);
	case SA_CHAR:   return char(op1.arg.v_char ^ (char)op2);
	case SA_UINT64: return op1.arg.v_uint64 ^ (uint64_t)op2;
	case SA_UINT:   return op1.arg.v_uint ^ (unsigned int)op2;
	case SA_USHORT: return (unsigned short)(op1.arg.v_ushort ^ (unsigned short)op2);
	case SA_BYTE:   return (unsigned char)(op1.arg.v_byte ^ (unsigned char)op2);

	case SA_BIG:
	case SA_DOUBLE:
	case SA_FLOAT:
		{ Big bi = op1.GetInt(); if (bi.BitXor(op2.GetInt()) != 0) throw ERR_PARAM; return bi; }
	}
	throw ERR_TYPE;
}

SArg operator& (const SArg & op1, const SArg & op2)
{
	switch(op1.type)
	{
	case SA_INT64:  return op1.arg.v_int64 & (int64_t)op2;
	case SA_INT:    return op1.arg.v_int & (int)op2;
	case SA_SHORT:  return short(op1.arg.v_short & (short)op2);
	case SA_CHAR:   return char(op1.arg.v_char & (char)op2);
	case SA_UINT64: return op1.arg.v_uint64 & (uint64_t)op2;
	case SA_UINT:   return op1.arg.v_uint & (unsigned int)op2;
	case SA_USHORT: return (unsigned short)(op1.arg.v_ushort & (unsigned short)op2);
	case SA_BYTE:   return (unsigned char)(op1.arg.v_byte & (unsigned char)op2);

	case SA_BIG:
	case SA_DOUBLE:
	case SA_FLOAT:
		{	Big bi = op1.GetInt(); if (bi.BitAnd(op2.GetInt()) != 0) throw ERR_PARAM; return bi; }
	}
	throw ERR_TYPE;
}

SArg operator>> (const SArg & op1, int n)
{
	switch(op1.type)
	{
	case SA_INT64:  return op1.arg.v_int64 >> n;
	case SA_INT:    return op1.arg.v_int >> n;
	case SA_SHORT:  return short(op1.arg.v_short >> n);
	case SA_CHAR:   return char(op1.arg.v_char >> n);
	case SA_UINT64: return op1.arg.v_uint64 >> n;
	case SA_UINT:   return op1.arg.v_uint >> n;
	case SA_USHORT: return (unsigned short)(op1.arg.v_ushort >> n);
	case SA_BYTE:   return (unsigned char)(op1.arg.v_byte >> n);

	case SA_BIG:
	case SA_DOUBLE:
	case SA_FLOAT:
		{
			BigInt bint; op1.GetBig().ToInt(bint); bint.Rcr(n);
			Big bi; bi.FromInt(bint); return bi;
		}
	}
	throw ERR_TYPE;
}

SArg operator<< (const SArg & op1, int n)
{
	switch(op1.type)
	{
	case SA_INT64:  return op1.arg.v_int64 << n;
	case SA_INT:    return op1.arg.v_int << n;
	case SA_SHORT:  return short(op1.arg.v_short << n);
	case SA_CHAR:   return char(op1.arg.v_char << n);
	case SA_UINT64: return op1.arg.v_uint64 << n;
	case SA_UINT:   return op1.arg.v_uint << n;
	case SA_USHORT: return (unsigned short)(op1.arg.v_ushort << n);
	case SA_BYTE:   return (unsigned char)(op1.arg.v_byte << n);

	case SA_BIG:
	case SA_DOUBLE:
	case SA_FLOAT:
		{
			BigInt bint; op1.GetBig().ToInt(bint); bint.Rcl(n);
			Big bi; bi.FromUInt(bint); return bi;
		}
	}
	throw ERR_TYPE;
}

SArg SArg::Ror(const SArg & op) const
{
	int sh = (int)op % (varsize()*8);
	int sh2 = varsize()*8 - sh;
	switch(type)
	{
	case SA_INT64:
		return (int64_t)((int64_t)(arg.v_int64>>sh) | (int64_t)(arg.v_int64<<sh2));
	case SA_UINT64:
		return (uint64_t)((uint64_t)(arg.v_uint64>>sh) | (uint64_t)(arg.v_uint64<<sh2));
	case SA_INT:
		return (int)((int)(arg.v_int>>sh) | (int)(arg.v_int<<sh2));
	case SA_UINT:
		return (unsigned int)((unsigned int)(arg.v_uint>>sh) | (unsigned int)(arg.v_uint<<sh2));
	case SA_SHORT:
		return (short)((short)(arg.v_short>>sh) | (short)(arg.v_short<<sh2));
	case SA_USHORT:
		return (unsigned short)((unsigned short)(arg.v_ushort>>sh) | (unsigned short)(arg.v_ushort<<sh2));
	case SA_CHAR:
		return (char)((char)(arg.v_char>>sh) | (char)(arg.v_char<<sh2));
	case SA_BYTE:
		return (unsigned char)((unsigned char)(arg.v_byte>>sh) | (unsigned char)(arg.v_byte<<sh2));
	default:
		throw ERR_TYPE;
	}

}

SArg SArg::Rol(const SArg & op) const
{
	int sh = (int)op % (varsize()*8);
	int sh2 = varsize()*8 - sh;
	switch(type)
	{
	case SA_INT64:
		return (int64_t)((int64_t)(arg.v_int64<<sh) | (int64_t)(sh2 ? arg.v_int64>>sh2 : 0));
	case SA_UINT64:
		return (uint64_t)((uint64_t)(arg.v_uint64<<sh) | (uint64_t)(sh2 ? arg.v_uint64>>sh2 : 0));
	case SA_INT:
		return (int)((int)(arg.v_int<<sh) | (int)(sh2 ? arg.v_int>>sh2 : 0));
	case SA_UINT:
		return (unsigned int)((unsigned int)(arg.v_uint<<sh) | (unsigned int)(sh2 ? arg.v_uint>>sh2 : 0));
	case SA_SHORT:
		return (short)((short)(arg.v_short<<sh) | (short)(sh2 ? arg.v_short>>sh2 : 0));
	case SA_USHORT:
		return (unsigned short)((unsigned short)(arg.v_ushort<<sh) | (unsigned short)(sh2 ? arg.v_ushort>>sh2 : 0));
	case SA_CHAR:
		return (char)((char)(arg.v_char<<sh) | (char)(sh2 ? arg.v_char>>sh2 : 0));
	case SA_BYTE:
		return (unsigned char)((unsigned char)(arg.v_byte<<sh) | (unsigned char)(sh2 ? arg.v_byte>>sh2 : 0));
	default:
		throw ERR_TYPE;
	}
}

SArg SArg::Pow(const SArg & op) const
{
	if ((type == SA_DOUBLE || type == SA_FLOAT) && (op.type == SA_FLOAT || op.type == SA_DOUBLE))
	{
		if (type == SA_FLOAT && op.type == SA_FLOAT)
			return powf(arg.v_flt, (float)op);
		return pow(arg.v_dbl, (double)op);
	}
	Big num = GetBig();
	ttmath::uint err = num.Pow(op.GetBig());
	if (err == 1)
		throw ERR_FLOW;
	if (err == 2)
		throw ERR_PARAM;
	return num;
}

SArg SArg::operator+= (const SArg & op)
{
	*this = *this + op;
	return *this;
}

bool operator== (const SArg & op1, const SArg & op2)
{
	return op1.GetBig() == op2.GetBig();
}

bool operator!= (const SArg & op1, const SArg & op2)
{
	return op1.GetBig() != op2.GetBig();
}

bool operator> (const SArg & op1, const SArg & op2)
{
	switch(op1.type)
	{
	case SA_INT64:  return op1.arg.v_int64 > (int64_t)op2;
	case SA_UINT64: return op1.arg.v_uint64 > (uint64_t)op2;
	default:
		return op1.GetBig() > op2.GetBig();
	}
	throw ERR_TYPE;
}

bool operator< (const SArg & op1, const SArg & op2)
{
	switch(op1.type)
	{
	case SA_INT64:  return op1.arg.v_int64 < (int64_t)op2;
	case SA_UINT64: return op1.arg.v_uint64 < (uint64_t)op2;
	default:
		return op1.GetBig() < op2.GetBig();
	}
	throw ERR_TYPE;
}

/////////////////////////// type cast //////////////////////////////

SArg::operator int64_t() const
{
	switch(type)
	{
	case SA_BIG:
	{
		BigInt d;
		//Big b = v_big;
		//b += CalcParser::roundup_value;
		//b.ToInt(d);
		v_big.ToInt(d);
		ttmath::sint res = 0;
		d.ToInt(res);
		return res;
	}

	case SA_INT64:  return arg.v_int64;
	case SA_INT:    return arg.v_int;
	case SA_SHORT:  return arg.v_short;
	case SA_CHAR:   return arg.v_char;
	case SA_UINT64: return arg.v_uint64;
	case SA_UINT:   return arg.v_uint;
	case SA_USHORT: return arg.v_ushort;
	case SA_BYTE:   return arg.v_byte;
	case SA_DOUBLE:  return (signed long long int)arg.v_dbl;
	case SA_FLOAT:  return (signed long long int)arg.v_flt;
	}
	throw ERR_ALL;
}

SArg::operator int() const
{
	return (int)operator int64_t();
};
SArg::operator short() const
{
	return (short)operator int64_t();
};
SArg::operator char() const
{
	return (char)operator int64_t();
};

SArg::operator uint64_t() const
{
	switch(type)
	{
	case SA_BIG: {
		BigInt d;
		v_big.ToInt(d);
		ttmath::uint res = 0;
		d.ToUInt(res);
		return res;
	}

	case SA_INT64:  return (unsigned long long int)arg.v_int64;
	case SA_INT:    return (unsigned int)arg.v_int;
	case SA_SHORT:  return (unsigned int)arg.v_short;
	case SA_CHAR:   return (unsigned int)arg.v_char;
	case SA_UINT64: return arg.v_uint64;
	case SA_UINT:   return arg.v_uint;
	case SA_USHORT: return arg.v_ushort;
	case SA_BYTE:   return arg.v_byte;
	case SA_DOUBLE: return (unsigned long long int)arg.v_dbl;
	case SA_FLOAT:  return (unsigned long long int)arg.v_flt;
	}
	throw ERR_ALL;
}

SArg::operator unsigned int() const
{
	return (unsigned int)operator uint64_t();
};
SArg::operator unsigned short() const
{
	return (unsigned short)operator uint64_t();
};
SArg::operator unsigned char() const
{
	return (unsigned char)operator uint64_t();
};

Big SArg::GetBig() const
{
	Big big;
	switch(type)
	{
	case SA_BIG: return v_big;

	case SA_INT64:  big.FromInt(arg.v_int64);  return big;
	case SA_UINT64: big.FromInt(arg.v_uint64);  return big;
	case SA_INT:    big.FromInt(arg.v_int);  return big;
	case SA_SHORT:  big.FromInt(arg.v_short);  return big;
	case SA_CHAR:   big.FromInt(arg.v_char);  return big;
	case SA_UINT:   big.FromInt(arg.v_uint);  return big;
	case SA_USHORT: big.FromInt((unsigned int)arg.v_ushort);  return big;
	case SA_BYTE:   big.FromInt((unsigned int)arg.v_byte);  return big;
	case SA_DOUBLE: big.FromDouble(arg.v_dbl); return big;
	case SA_FLOAT:  big.FromDouble(arg.v_flt); return big;
	}
	throw ERR_ALL;
}

Big SArg::GetInt() const
{
	Big bi = v_big;
	ttmath::SkipFraction(bi);
	return bi;
}

SArg::operator float() const
{
	float f;
	if (type == SA_FLOAT)
		return arg.v_flt;
	if (type == SA_DOUBLE)
		return (float)arg.v_dbl;
	if (this->GetBig().ToFloat(f) == 1)
		throw ERR_FLOW;
	return f;
}

SArg::operator double() const
{
	double d;
	if (type == SA_FLOAT)
		return (double)arg.v_flt;
	if (type == SA_DOUBLE)
		return arg.v_dbl;
	if (this->GetBig().ToDouble(d) == 1)
		throw ERR_FLOW;
	return d;
}

SArg::operator bool() const
{
	return (int)*this != 0;
}

/////////////////////////// services //////////////////////////////

bool SArg::Print(std::wstring &str, int radix, int num_delim, wchar_t delim_digit)
{
	uint64_t data, data_mask;
	int numdigits, data_shift;
	bool sign = false;
	if (!IsFixedLength())
		return false;
	switch (radix)
	{
	case 2:
		numdigits = 1;
		data_mask = 1;
		data_shift = 1;
		break;
	case 8:
		numdigits = 3;
		data_mask = 7;
		data_shift = 3;
		break;
	case 16:
		numdigits = 4;
		data_mask = 0xf;
		data_shift = 4;
		break;
	default:
		return false;
	}

	switch(type)
	{
	case SA_DOUBLE:
	case SA_FLOAT:
		return false;

	case SA_UINT64:
		data = arg.v_uint64;
		break;
	case SA_UINT:
		data = arg.v_uint;
		break;
	case SA_USHORT:
		data = arg.v_ushort;
		break;
	case SA_BYTE:
		data = arg.v_byte;
		break;

	case SA_INT64:
		if (arg.v_int64 < 0)
		{
			sign = true;
			data = -arg.v_int64;
		} else
			data = arg.v_int64;
		break;
	case SA_INT:
		if (arg.v_int < 0)
		{
			sign = true;
			data = -arg.v_int;
		} else
			data = arg.v_int;
		break;
	case SA_SHORT:
		if (arg.v_short < 0)
		{
			sign = true;
			data = -arg.v_short;
		} else
			data = arg.v_short;
		break;
	case SA_CHAR:
		if (arg.v_char < 0)
		{
			sign = true;
			data = -arg.v_char;
		} else
			data = arg.v_char;
		break;

	default: ;
	}

	if (sign)
		str += L"-";
	const wchar_t *digits = L"0123456789ABCDEF";
	for (int i = (varsize()*8 + numdigits-1)/numdigits - 1; i >= 0; i--)
	{
		str += digits[(data >> (i * data_shift)) & data_mask];

		if (i > 0 && (i % num_delim) == 0 && delim_digit)
			str += delim_digit;
	}

	return true;
}

bool SArg::isempty() const
{
	return empty;
}

int SArg::varsize() const
{
	switch(type)
	{
	case SA_UINT64:
	case SA_INT64:
		return sizeof(int64_t);
	case SA_INT:
	case SA_UINT:
		return sizeof(int);
	case SA_SHORT:
	case SA_USHORT:
		return sizeof(short);
	case SA_CHAR:
	case SA_BYTE:
		return sizeof(char);
	case SA_DOUBLE:
		return sizeof(double);
	case SA_FLOAT:
		return sizeof(float);

	default:
		return 4;
	}
}

bool SArg::IsFixedLength() const
{
	switch(type)
	{
	case SA_UINT64:
	case SA_INT64:
	case SA_INT:
	case SA_UINT:
	case SA_SHORT:
	case SA_USHORT:
	case SA_CHAR:
	case SA_BYTE:
	case SA_DOUBLE:
	case SA_FLOAT:
		return true;
	default:
		return false;
	}
}

EArgType SArg::gettype()
{
	return type;
}

//////////////////////////////////////////////////////////

SArg SArg::_factor(const SArg & op1)
{
	if ((int)op1 < 0)
		throw ERR_PARAM;
	// XXX: only CPU speed limits this...
	if (op1.GetBig() > Big(10000))
		throw ERR_FLOW;
	return ttmath::Factorial(op1.GetBig());
}

SArg SArg::_frac(const SArg & op1)
{
	if (op1.type == SA_FLOAT)
		return (float)op1 - floorf(float(op1));
	else if (op1.type == SA_DOUBLE)
		return (double)op1 - floor(double(op1));
	Big res = op1.GetBig();
	res.RemainFraction();
	return res;
}

SArg SArg::_floor(const SArg & op1)
{
	if (op1.type == SA_FLOAT)
		return floorf(float(op1));
	else if (op1.type == SA_DOUBLE)
		return floor(double(op1));
	return ttmath::Floor(op1.GetBig());
}

SArg SArg::_ceil(const SArg & op1)
{
	if (op1.type == SA_FLOAT)
		return ceilf(float(op1));
	else if (op1.type == SA_DOUBLE)
		return ceil(double(op1));
	return ttmath::Ceil(op1.GetBig());
}

SArg SArg::_sin(const SArg & op1)
{
	if (op1.type == SA_FLOAT)
		return sinf(float(op1));
	else if (op1.type == SA_DOUBLE)
		return sin(double(op1));
	return ttmath::Sin(op1.GetBig());
}

SArg SArg::_cos(const SArg & op1)
{
	if (op1.type == SA_FLOAT)
		return cosf(float(op1));
	else if (op1.type == SA_DOUBLE)
		return cos(double(op1));
	return ttmath::Cos(op1.GetBig());
}

SArg SArg::_tan(const SArg & op1)
{
	if (op1.type == SA_FLOAT)
		return tanf(float(op1));
	else if (op1.type == SA_DOUBLE)
		return tan(double(op1));
	ttmath::ErrorCode err;
	SArg ret = ttmath::Tan(op1.GetBig(), &err);
	if (err != 0) throw ERR_PARAM;
		return ret;
}

SArg SArg::_arctan(const SArg & op1)
{
	if (op1.type == SA_FLOAT)
		return atanf(float(op1));
	else if (op1.type == SA_DOUBLE)
		return atan(double(op1));
	return ttmath::ATan(op1.GetBig());
}

SArg SArg::_ln(const SArg & op1)
{
	if (op1.type == SA_FLOAT)
		return logf(float(op1));
	else if (op1.type == SA_DOUBLE)
		return log(double(op1));
	ttmath::ErrorCode err;
	SArg ret = ttmath::Ln(op1.GetBig(), &err);
	if (err != 0) throw ERR_PARAM;
		return ret;
}

SArg SArg::_rnd()
{
	//return Big(rand())*Big(rand())/Big(RAND_MAX*RAND_MAX);

	Big b;
	int man = sizeof(b.mantissa.table) / sizeof(ttmath::uint);
	for (int i = man - 1; i >= 0; i--)
	{
		// TODO: perhaps, find more easy method?
		b.mantissa.table[i] = 0;
		for (int j = TTMATH_BITS_PER_UINT - 8; j >= 0; j -= 8)
			b.mantissa.table[i] |= ((ttmath::uint(rand()) & 0xff) << j);
	}

	b.exponent = -ttmath::sint(man * TTMATH_BITS_PER_UINT);
	b.info = 0;
	return b;
}

SArg SArg::_if(const SArg & op1, const SArg & op2, const SArg & op3)
{
	if (op1)
		return op2;
	return op3;
}

SArg SArg::_f2b(const SArg & op1)
{
	/// \WARNING! hardware-dependent!
	float sf = (float)op1;
	unsigned int *i = (unsigned int *)&sf;
	return SArg(*i);
}

SArg SArg::_d2b(const SArg & op1)
{
	/// \WARNING! hardware-dependent!
	double df = (double)op1;
	uint64_t *i = (uint64_t *)&df;
	return SArg(*i);
}

SArg SArg::_b2f(const SArg & op1)
{
	/// \WARNING! hardware-dependent!
	unsigned int i = (unsigned int)op1;
	float *sf = (float *)&i;
	return SArg(*sf);
}

SArg SArg::_b2d(const SArg & op1)
{
	/// \WARNING! hardware-dependent!
	uint64_t i = (uint64_t)op1;
	double *df = (double *)&i;
	return SArg(*df);
}

SArg SArg::_finf()
{
	return SArg(std::numeric_limits<float>::infinity());
}

SArg SArg::_fnan()
{
	return SArg(std::numeric_limits<float>::quiet_NaN());
}

SArg SArg::_numer(const SArg & op1)
{
	BigInt numer, denom;
	Big b = op1.GetBig();
	if (b.IsNan())
		return _fnan();
	CalcParser::GetFraction(b, &numer, &denom);
	if (b.IsSign())
		numer.SetSign();
	return SArg(numer);
}

SArg SArg::_denom(const SArg & op1)
{
	BigInt numer, denom;
	Big b = op1.GetBig();
	if (b.IsNan())
		return _fnan();
	CalcParser::GetFraction(b, &numer, &denom);
	return SArg(denom);
}

SArg SArg::_gcd(const SArg & op1, const SArg & op2)
{
	Big n = op1.GetBig();
	Big d = op2.GetBig();
	n.Abs();
	n.SkipFraction();
	d.Abs();
	d.SkipFraction();
	return GreatestCommonDiv(n, d);
}

//SArg _sum();
//SArg _avr();

/////////////////////////////////////////////////////////////////////////////////

SArg SArg::to_int64(const SArg & op1)
{
	return (int64_t)op1;
}

SArg SArg::to_uint64(const SArg & op1)
{
	return (uint64_t)op1;
}

SArg SArg::to_int(const SArg & op1)
{
	return (int)op1;
}

SArg SArg::to_uint(const SArg & op1)
{
	return (unsigned int)op1;
}

SArg SArg::to_short(const SArg & op1)
{
	return (short)op1;
}

SArg SArg::to_ushort(const SArg & op1)
{
	return (unsigned short)op1;
}

SArg SArg::to_char(const SArg & op1)
{
	return (char)op1;
}

SArg SArg::to_byte(const SArg & op1)
{
	return (unsigned char)op1;
}

SArg SArg::to_double(const SArg & op1)
{
	return (double)op1;
}

SArg SArg::to_float(const SArg & op1)
{
	return (float)op1;
}

Big SArg::GreatestCommonDiv(Big numer, Big denom)
{
	if (numer < denom)
	{
		Big temp = numer;
		numer = denom;
		denom = temp;
	}
	if (denom == 0)
		return numer;
	else
	{
		if (numer.Mod(denom) != 0)
			return numer;
		return GreatestCommonDiv(denom, numer);
	}
}
