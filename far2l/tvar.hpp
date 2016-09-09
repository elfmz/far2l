#pragma once

/*
tvar.hpp

Реализация класса TVar ("кастрированый" вариант - только целое и строковое значение)
(для макросов)

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

//---------------------------------------------------------------
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.
//---------------------------------------------------------------

enum TVarType
{
	vtUnknown = -1,
	vtInteger = 0,
	vtString  = 1,
	vtDouble  = 2,
};

typedef int (*TVarFuncCmp)(TVarType vt,const void *, const void *);

class TVarSet;
class TAbstractSet;
const int V_TABLE_SIZE = 23;
typedef TVarSet *(TVarTable)[V_TABLE_SIZE];

class TVar
{
	private:
		TVarType vType;
		int64_t inum;
		double  dnum;
		wchar_t *str;

	private:
		static int CompAB(const TVar& a, const TVar& b, TVarFuncCmp fcmp);

	public:
		TVar(int64_t = 0);
		TVar(const wchar_t*);
		TVar(int);
		TVar(double);
		TVar(const TVar&);
		~TVar();

	public:
		friend TVar operator+(const TVar&, const TVar&);
		friend TVar operator-(const TVar&, const TVar&);
		friend TVar operator*(const TVar&, const TVar&);
		friend TVar operator/(const TVar&, const TVar&);
		friend TVar operator%(const TVar&, const TVar&);

		friend TVar operator&(const TVar&, const TVar&);
		friend TVar operator|(const TVar&, const TVar&);
		friend TVar operator^(const TVar&, const TVar&);
		friend TVar operator>>(const TVar&, const TVar&);
		friend TVar operator<<(const TVar&, const TVar&);

		friend TVar operator&&(const TVar&, const TVar&);
		friend TVar operator||(const TVar&, const TVar&);
		friend TVar xor_op(const TVar&, const TVar&);

		TVar& operator=(const TVar&);

		TVar& operator+=(const TVar& b)  { return *this = *this+b;  };
		TVar& operator-=(const TVar& b)  { return *this = *this-b;  };
		TVar& operator*=(const TVar& b)  { return *this = *this*b;  };
		TVar& operator/=(const TVar& b)  { return *this = *this/b;  };
		TVar& operator%=(const TVar& b)  { return *this = *this%b;  };
		TVar& operator&=(const TVar& b)  { return *this = *this&b;  };
		TVar& operator|=(const TVar& b)  { return *this = *this|b;  };
		TVar& operator^=(const TVar& b)  { return *this = *this^b;  };
		TVar& operator>>=(const TVar& b) { return *this = *this>>b; };
		TVar& operator<<=(const TVar& b) { return *this = *this<<b; };

		TVar operator+();
		TVar operator-();
		TVar operator!();
		TVar operator~();

		friend int operator==(const TVar&, const TVar&);
		friend int operator!=(const TVar&, const TVar&);
		friend int operator<(const TVar&, const TVar&);
		friend int operator<=(const TVar&, const TVar&);
		friend int operator>(const TVar&, const TVar&);
		friend int operator>=(const TVar&, const TVar&);

		TVar& AppendStr(wchar_t);
		TVar& AppendStr(const TVar&);

		TVarType type() { return vType; };
		void SetType(TVarType newType) {vType=newType;};

		int isString()   const { return vType == vtString;  }
		int isInteger()  const { return vType == vtInteger; }
		int isDouble()   const { return vType == vtDouble;  }
		int isUnknown()  const { return vType == vtUnknown;  }

		double d()         const;
		int64_t i()        const;
		const wchar_t *s() const;

		const wchar_t *toString();
		double toDouble();
		int64_t toInteger();

		int64_t getInteger() const;
		double getDouble() const;
};

//---------------------------------------------------------------
// Работа с таблицами имен переменных
//---------------------------------------------------------------

class TAbstractSet
{
	public:
		wchar_t *str;
		TAbstractSet *next;

	public:
		TAbstractSet(const wchar_t *s)
		{
			str = nullptr;
			next = nullptr;

			if (s)
			{
				str = new wchar_t[StrLength(s)+1];
				wcscpy(str, s);
			}
		}

		~TAbstractSet()
		{
			if (str)
				delete [] str;
		}
};

class TVarSet : public TAbstractSet
{
	public:
		TVar value;

	public:
		TVarSet(const wchar_t *s) : TAbstractSet(s), value() {}
};

extern int isVar(TVarTable, const wchar_t*);
extern TVarSet *varEnum(TVarTable, int, int);
extern TVarSet *varLook(TVarTable worktable, const wchar_t* name, bool ins=false);
extern void varKill(TVarTable, const wchar_t*);
extern void initVTable(TVarTable);
extern void deleteVTable(TVarTable);

inline TVarSet *varInsert(TVarTable t, const wchar_t *s)
{
	return varLook(t, s, true);
}
