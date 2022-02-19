#pragma once

/*
lang.hpp
*/
/*
Copyright (c) 2010 Far Group
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

#include "farplug-wide.h" // for FarLangMsgID

struct FarLangMsg
{
	FarLangMsgID _id;

	inline FarLangMsgID ID() const { return _id; }
	inline const wchar_t *CPtr() const { return GetMsg(_id); }
	inline operator const wchar_t *() const { return GetMsg(_id); }

	inline FarLangMsg operator+ (int delta) { return FarLangMsg{_id + delta}; }
	inline FarLangMsg operator- (int delta) { return FarLangMsg{_id - delta}; }

	inline bool operator == (const FarLangMsg other) const { return _id == other._id; }
	inline bool operator == (const FarLangMsgID other_id) const { return _id == other_id; }

	inline bool operator != (const FarLangMsg other) const { return _id != other._id; }
	inline bool operator != (const FarLangMsgID other_id) const { return _id != other_id; }

private:
	static const wchar_t *GetMsg(FarLangMsgID id); // impl in cfg/language.cpp
};

namespace Msg
{
#define DECLARE_FARLANGMSG(NAME, ID) static constexpr FarLangMsg NAME{ ID };
#include "bootstrap/lang.inc"
#undef DECLARE_FARLANGMSG
};

