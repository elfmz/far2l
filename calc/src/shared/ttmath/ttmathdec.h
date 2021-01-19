/*
 * This file is a part of TTMath Bignum Library
 * and is distributed under the (new) BSD licence.
 * Author: Tomasz Sowa <t.sowa@ttmath.org>
 */

/* 
 * Copyright (c) 2012, Tomasz Sowa
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *    
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    
 *  * Neither the name Tomasz Sowa nor the names of contributors to this
 *    project may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef headerfilettmathdec
#define headerfilettmathdec

#include "ttmathtypes.h"
#include "ttmaththreads.h"
#include "ttmathuint.h"



namespace ttmath
{

template<uint value_size, uint dec_digits>
class Dec
{
public:

	UInt<value_size> value;
	unsigned char info;


	/*!
		Sign
		the mask of a bit from 'info' which means that there is a sign
		(when the bit is set)
	*/
	#define TTMATH_DEC_SIGN 128


	/*!
		Not a number
		if this bit is set that there is not a valid number
	*/
	#define TTMATH_DEC_NAN  64




	Dec()
	{
		info = TTMATH_DEC_NAN;
	}


	Dec(const char * s)
	{
		info = TTMATH_DEC_NAN;
		FromString(s);
	}


	Dec<value_size, dec_digits> & operator=(const char * s)
	{
		FromString(s);

	return *this;
	}


	uint FromString(const char * s, const char ** after_source = 0, bool * value_read = 0)
	{
		return FromStringBase(s, after_source, value_read);
	}


	void ToString(std::string & result) const
	{
		ToStringBase(result);
	}


	/*!
		this method clears a specific bit in the 'info' variable

		bit is one of: 
	*/
	void ClearInfoBit(unsigned char bit)
	{
		info = info & (~bit);
	}


	/*!
		this method sets a specific bit in the 'info' variable

		bit is one of: 

	*/
	void SetInfoBit(unsigned char bit)
	{
		info = info | bit;
	}


	/*!
		this method returns true if a specific bit in the 'info' variable is set

		bit is one of: 
	*/
	bool IsInfoBit(unsigned char bit) const
	{
		return (info & bit) != 0;
	}


	bool IsNan() const 
	{
		return IsInfoBit(TTMATH_DEC_NAN);
	}


	bool IsSign() const 
	{
		return IsInfoBit(TTMATH_DEC_SIGN);
	}


	/*!
		this method sets the sign

			e.g.
			-1 -> -1
			2  -> -2

		we do not check whether there is a zero or not, if you're using this method
		you must be sure that the value is (or will be afterwards) different from zero
	*/
	void SetSign()
	{
		SetInfoBit(TTMATH_DEC_SIGN);
	}


	void SetNaN()
	{
		SetInfoBit(TTMATH_DEC_NAN);
	}


	void Abs()
	{
		ClearInfoBit(TTMATH_DEC_SIGN);
	}



	uint Add(const Dec<value_size, dec_digits> & arg)
	{
	uint c = 0;

		if( IsSign() == arg.IsSign() )
		{
			c += value.Add(arg.value);		
		}
		else
		{
			bool is_sign;

			if( value > arg.value )
			{
				is_sign = IsSign();
				value.Sub(arg.value);
			}
			else
			{
				is_sign = arg.IsSign();
				UInt<value_size> temp(this->value);
				value = arg.value;
				value.Sub(temp);
			}

			is_sign ? SetSign() : Abs();
		}

		if( c )
			SetNaN();

	return (c==0)? 0 : 1;
	}

/*
	uint Sub(const Dec<value_size, dec_digits> & arg)
	{
	}
*/

private:






#ifndef TTMATH_MULTITHREADS

	/*!
	*/
	void SetMultipler(UInt<value_size> & result)
	{
		// this guardian is initialized before the program runs (static POD type)
		static int guardian = 0;
		static UInt<value_size> multipler;
	
		if( guardian == 0 )
		{
			multipler = 10;
			multipler.Pow(dec_digits);
			guardian = 1;
		}

		result = multipler;
	}

#else

	/*!
	*/
	void SetMultipler(UInt<value_size> & result)
	{
		// this guardian is initialized before the program runs (static POD type)
		volatile static sig_atomic_t guardian = 0;
		static UInt<value_size> * pmultipler;
	
		// double-checked locking
		if( guardian == 0 )
		{
			ThreadLock thread_lock;

			// locking
			if( thread_lock.Lock() )
			{
				static UInt<value_size> multipler;

				if( guardian == 0 )
				{
					pmultipler = &multipler;
					multipler = 10;
					multipler.Pow(dec_digits);
					guardian = 1;
				}
			}
			else
			{
				// there was a problem with locking, we store the result directly in 'result' object
				result = 10;
				result.Pow(dec_digits);
				
			return;
			}

			// automatically unlocking
		}

		result = *pmultipler;
	}

#endif



	/*!
		an auxiliary method for converting from a string
	*/
	template<class char_type>
	uint FromStringBase(const char_type * s, const char_type ** after_source = 0, bool * value_read = 0)
	{
		UInt<value_size> multipler;
		const char_type * after;
		uint c = 0;
		info = 0;

		Misc::SkipWhiteCharacters(s);

		if( *s == '-' )
		{
			s += 1;
			SetSign();
		}
		else
		if( *s == '+' )
		{
			s += 1;
		}

		c += value.FromString(s, 10, &after, value_read);

		if( after_source )
			*after_source = after;

		SetMultipler(multipler);
		c += value.Mul(multipler);

		if( *after == '.' )
			c += FromStringBaseAfterComma(after+1, after_source);

		if( c )
			SetInfoBit(TTMATH_DEC_NAN);

	return (c==0)? 0 : 1;
	}


	template<class char_type>
	uint FromStringBaseAfterComma(const char_type * s, const char_type ** after_source = 0, bool * value_read = 0)
	{
		UInt<value_size> temp;
		UInt<value_size> multipler;
		sint z;
		uint c = 0;
		size_t i = dec_digits;

		SetMultipler(multipler);

		for( ; i>0 && (z=Misc::CharToDigit(*s, 10)) != -1 ; --i, ++s )
		{
			multipler.DivInt(10);
			temp.SetZero();

			if( value_read )
				*value_read = true;

			if( c == 0 )
			{
				temp.table[0] = z;
				c += temp.Mul(multipler);
				c += value.Add(temp);
			}
		}

		if( i == 0 && (z=Misc::CharToDigit(*s, 10)) != -1 && z >= 5 )
			c += value.AddOne();

		if( after_source )
		{
			while( (z=Misc::CharToDigit(*s, 10)) != -1 )
				s += 1;

			*after_source = s;
		}

	return c;
	}



	template<class string_type>
	void ToStringBase(string_type & result) const
	{
		if( IsNan() )
		{
			result = "NaN";
			return;
		}

		value.ToStringBase(result, 10, IsSign());

		if( dec_digits > 0 )
		{
			size_t size = result.size();

			if( IsSign() && size > 0 )
				size -= 1;

			if( dec_digits >= size )
			{
				size_t zeroes = dec_digits - size + 1;
				size_t start  = IsSign() ? 1 : 0;
				result.insert(start, zeroes, '0');
			}

			result.insert(result.end() - dec_digits, '.');
		}
	}



};


} // namespace

#endif
