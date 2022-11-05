#pragma once

/*
format.hpp

Форматирование строк
*/
/*
Copyright (c) 2009 Far Group
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

#include <WinCompat.h>
#include "FARString.hpp"

namespace fmt
{
	struct Cells { }; // makes modifiers operate in screen cells but not characters
	struct Chars { }; // makes modifiers operate in characters but not screen cells

	class Skip
	{
			size_t Value;
		public:
			Skip(size_t Value=static_cast<size_t>(-1)) {this->Value=Value;}
			size_t GetValue()const {return Value;}
	};

	class Expand
	{
			size_t Value;
		public:
			Expand(size_t Value=0) {this->Value=Value;}
			size_t GetValue()const {return Value;}
	};

	class Truncate
	{
			size_t Value;
		public:
			Truncate(size_t Value=static_cast<size_t>(-1)) {this->Value=Value;}
			size_t GetValue()const {return Value;}
	};

	class Size // same as Expand + Truncate with same parameter
	{
			size_t Value;
		public:
			Size(size_t Value=static_cast<size_t>(-1)) {this->Value=Value;}
			size_t GetValue()const {return Value;}
	};

	class FillChar
	{
			WCHAR Value;
		public:
			FillChar(WCHAR Value=L' ') {this->Value=Value;}
			WCHAR GetValue()const {return Value;}
	};

	class LeftAlign {};

	class RightAlign {};

	enum AlignType
	{
		A_LEFT,
		A_RIGHT,
	};

};

class BaseFormat
{
		bool _Cells;
		size_t _Skip;
		size_t _Expand;
		size_t _Truncate;
		WCHAR _FillChar;
		fmt::AlignType _Align;

		void Reset();
		void Put(LPCWSTR Data,size_t Length);

	protected:
		virtual void Commit(const FARString& Data)=0;

	public:
		BaseFormat() {Reset();}
		virtual ~BaseFormat() {}

		// attributes
		inline void SetCells(bool Cells = false) { _Cells = Cells; }
		inline void SetSkip(size_t Skip = 0) { _Skip = Skip; }
		inline void SetTruncate(size_t Truncate = static_cast<size_t>(-1)) { _Truncate=Truncate; }
		inline void SetExpand(size_t Expand = 0) { _Expand = Expand; }
		inline void SetAlign(fmt::AlignType Align = fmt::A_RIGHT) { _Align = Align; }
		inline void SetFillChar(WCHAR Char = L' ') { _FillChar = Char; }

		// data
		BaseFormat& operator<<(INT64 Value);
		BaseFormat& operator<<(UINT64 Value);

		BaseFormat& operator<<(short Value) {return operator<<(static_cast<INT64>(Value));}
		BaseFormat& operator<<(USHORT Value) {return operator<<(static_cast<UINT64>(Value));}

		BaseFormat& operator<<(int Value) {return operator<<(static_cast<INT64>(Value));}
		BaseFormat& operator<<(UINT Value) {return operator<<(static_cast<UINT64>(Value));}
#ifdef _WIN32
		BaseFormat& operator<<(long Value) {return operator<<(static_cast<INT64>(Value));}
		BaseFormat& operator<<(ULONG Value) {return operator<<(static_cast<UINT64>(Value));}
#endif
		BaseFormat& operator<<(WCHAR Value);
		BaseFormat& operator<<(LPCWSTR Data);
		BaseFormat& operator<<(FARString& String);

		// manipulators
		BaseFormat& operator<<(const fmt::Cells&);
		BaseFormat& operator<<(const fmt::Chars&);
		BaseFormat& operator<<(const fmt::Skip& Manipulator);
		BaseFormat& operator<<(const fmt::Expand& Manipulator);
		BaseFormat& operator<<(const fmt::Truncate& Manipulator);
		BaseFormat& operator<<(const fmt::Size& Manipulator);
		BaseFormat& operator<<(const fmt::LeftAlign& Manipulator);
		BaseFormat& operator<<(const fmt::RightAlign& Manipulator);
		BaseFormat& operator<<(const fmt::FillChar& Manipulator);
};

class FormatString:public BaseFormat
{
		FARString Value;
		void Commit(const FARString& Data) {Value+=Data;}
	public:
		operator const wchar_t*()const {return Value;}
		FARString& strValue() {return Value;}
		const FARString& strValue() const {return Value;}
		void Clear() {Value.Clear();}
};

class FormatScreen:public BaseFormat
{
		void Commit(const FARString& Data);
};
