#ifndef REGEXP_HPP_18B41BD7_69F8_461A_8A81_069B447D5554
#define REGEXP_HPP_18B41BD7_69F8_461A_8A81_069B447D5554
#pragma once

/*
RegExp.hpp

Regular expressions
Syntax and semantics are very close to perl
*/
/*
Copyright © 2000 Konstantin Stupnik
Copyright © 2008 Far Group
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

#include <farplug-wide.h>
#include <unordered_map>
#include <string>
#include <string.h>
#include "FARString.hpp"

//----------------------------------------------------------------------------

//#define RE_DEBUG

struct ReStringView // TODO: replace with std::wstring_view once will adopt C++17
{
	typedef wchar_t value_type;

	ReStringView(const ReStringView&) = default;
	ReStringView(ReStringView&&) = default;

	ReStringView& operator=(const ReStringView&) = default;

	inline ReStringView(const FARString &str)
		: _pw(str.CPtr()), _sz(str.GetLength())
	{
	}

	inline ReStringView()
		: _pw(nullptr), _sz(0)
	{
	}

	inline ReStringView(const wchar_t *pw, size_t sz = std::string::npos)
		: _pw(pw), _sz((sz == std::string::npos) ? wcslen(pw) : sz)
	{
	}

	inline ReStringView substr(size_t pos, size_t sz = std::string::npos) const
	{
		return ReStringView(_pw + pos, std::min(_sz - pos, sz));
	}

	inline bool operator ==(const ReStringView &other)
	{
		return _sz == other._sz && wmemcmp(_pw, other._pw, _sz) == 0;
	}

	inline bool operator !=(const ReStringView &other)
	{
		return !operator ==(other);
	}

	inline wchar_t front() const { return _sz ? _pw[0] : 0; }
	inline wchar_t back() const { return _sz ? _pw[_sz - 1] : 0; }
	inline const wchar_t *data() const { return _pw; }
	inline size_t size() const { return _sz; }
	inline bool empty() const { return _sz == 0; }

	inline const wchar_t operator[](size_t i) const { return _pw[i]; }

	size_t rfind(wchar_t c) const
	{
		for (size_t i = _sz; i;) {
			--i;
			if (_pw[i] == c) {
				return i;
			}
		}

		return std::string::npos;
	}

	inline const wchar_t *cbegin() const { return _pw; }

	inline const wchar_t *cend() const { return _pw + _sz; }

private:
	const wchar_t *_pw;
	size_t _sz;
};


//! Possible compile and runtime errors returned by LastError.
enum REError
{
	//! No errors
	errNone=0,
	//! RegExp wasn't even tried to compile
	errNotCompiled,
	//! expression contain syntax error
	errSyntax,
	//! Unbalanced brackets
	errBrackets,
	//! Max recursive brackets level reached. Controlled in compile time
	errMaxDepth,
	//! Invalid options combination
	errOptions,
	//! Reference to nonexistent bracket
	errInvalidBackRef,
	//! Invalid escape char
	errInvalidEscape,
	//! Invalid range value
	errInvalidRange,
	//! Quantifier applied to invalid object. f.e. lookahead assertion
	errInvalidQuantifiersCombination,
	//! Size of match array isn't large enough.
	errNotEnoughMatches,
	//! Attempt to match RegExp with Named Brackets, and no storage class provided.
	errNoStorageForNB,
	//! Reference to undefined named bracket
	errReferenceToUndefinedNamedBracket,
	//! Only fixed length look behind assertions are supported
	errVariableLengthLookBehind,

	errCancelled
};

enum
{
	//! Match in a case insensitive manner
	OP_IGNORECASE   =0x0001,
	//! Single line mode, dot meta-character will match newline symbol
	OP_SINGLELINE   =0x0002,
	//! MultiLine mode, ^ and $ can match line start and line end
	OP_MULTILINE    =0x0004,
	//! Extended syntax, spaces symbols are ignored unless escaped
	OP_XTENDEDSYNTAX=0x0008,
	//! Perl style RegExp provided. i.e. /expression/imsx
	OP_PERLSTYLE    =0x0010,
	//! Optimize after compile
	OP_OPTIMIZE     =0x0020,
	//! Strict escapes - only unrecognized escape will produce errInvalidEscape error
	OP_STRICT       =0x0040,
	//! Replace backslash with slash, used
	//! when RegExp source embedded in c++ sources
	OP_CPPMODE      =0x0080,
};

//! Hash table with match info
struct MatchHash
{
	std::unordered_map<std::wstring, RegExpMatch> Matches;
};

/*! Regular expressions support class.

Expressions must be Compile'ed first,
and than Match string or Search for matching fragment.
*/
class RegExp
{
public:
	struct REOpCode;
	class UniSet;
	struct StateStackItem;

private:
		// code
		std::vector<REOpCode> code;
		char slashChar;
		char backslashChar;

		std::unique_ptr<UniSet> firstptr;
		UniSet& first;

		int havefirst{};
		int havelookahead{};

		int minlength{};

		// error info
		mutable int errorcode;
		mutable int errorpos{};
		int srcstart{};

		// options
		int bracketscount{};
		int maxbackref{};
		int havenamedbrackets{};

		bool ignorecase{};

#ifdef RE_DEBUG
		std::wstring resrc;
#endif

		int CalcLength(ReStringView src);
		bool InnerCompile(const wchar_t* start, const wchar_t* src, int srclength, int options);

		bool InnerMatch(const wchar_t* start, const wchar_t* str, const wchar_t* strend, RegExpMatch* match, int& matchcount, MatchHash* hmatch, std::vector<StateStackItem>& stack) const;

		void TrimTail(const wchar_t* start, const wchar_t*& strend) const;

		// BUGBUG not thread safe!
		// TODO: split to compile errors (stateful) and match errors (stateless)
		bool SetError(int _code, int pos) const { errorcode = _code; errorpos = pos; return false; }

		int StrCmp(const wchar_t*& str,const wchar_t* start,const wchar_t* end) const;

	public:
		//! Default constructor.
		RegExp();
		~RegExp();

		RegExp(RegExp&&) noexcept;
		RegExp& operator=(RegExp&&) = delete;

		/*! Compile regular expression
		    Generate internal op-codes of expression.

		    \param src - source of expression
		    \param options - compile options

		    If compilation fails error code can be obtained with LastError function,
		    position of error in a expression can be obtained with ErrorPosition function.
		    See error codes in REError enumeration.
		    \sa LastError
		    \sa REError
		    \sa ErrorPosition
		    \sa options
		*/
		bool Compile(ReStringView src, int options=OP_PERLSTYLE|OP_OPTIMIZE);

		/*! Try to optimize regular expression
		    Significally speedup Search mode in some cases.
		*/
		bool Optimize();

		/*! Try to match string with regular expression
		    \param text - string to match
		    \param match - array of SMatch structures that receive brackets positions.
		    \param matchcount - in/out parameter that indicate number of items in
		    match array on input, and number of brackets on output.
		    \param hmatch - storage of named brackets.
		    \sa SMatch
		*/
		bool Match(ReStringView text, RegExpMatch* match, int& matchcount, MatchHash* hmatch = nullptr) const;
		/*! Advanced version of match. Can be used for multiple matches
		    on one string (to imitate /g modifier of perl regexp
		*/
		bool MatchEx(ReStringView text, size_t From, RegExpMatch* match, int& matchcount, MatchHash* hmatch = nullptr) const;
		/*! Try to find substring that will match regexp.
		    Parameters and return value are the same as for Match.
		    It is highly recommended to call Optimize before Search.
		*/
		bool Search(ReStringView text, RegExpMatch* match, int& matchcount, MatchHash* hmatch = nullptr) const;
		/*! Advanced version of search. Can be used for multiple searches
		    on one string (to imitate /g modifier of perl regexp
		*/
		bool SearchEx(ReStringView text, size_t From, RegExpMatch* match, int& matchcount, MatchHash* hmatch = nullptr) const;

		bool Search(ReStringView Str) const;

		/*! Get last error
		    \return code of the last error
		    Check REError for explanation
		    \sa REError
		    \sa ErrorPosition
		*/
		int LastError() const {return errorcode;}
		/*! Get last error position.
		    \return position of the last error in the regexp source.
		    \sa LastError
		*/
		int ErrorPosition() const { return srcstart + errorpos; }
		/*! Get number of brackets in expression
		    \return number of brackets, excluding brackets of type (:expr)
		    and named brackets.
		*/
		int GetBracketsCount() const {return bracketscount;}
};

#endif // REGEXP_HPP_18B41BD7_69F8_461A_8A81_069B447D5554
