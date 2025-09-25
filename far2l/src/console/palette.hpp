#pragma once

/*
palette.hpp

Таблица цветов
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

#include <WinCompat.h>
#include <utils.h>
#include <KeyFileHelper.h>
#include "color.hpp"

class Palette : NonCopyable
{

public:

	union {
		uint32_t palbuff[32];
		struct {
			uint32_t background[16];
			uint32_t foreground[16];
		};
	};

//    static rgbcolor_t cur_palette[32];
    static Palette FARPalette;

	Palette() noexcept;
	~Palette() noexcept;

	static void InitFarPalette( ) noexcept;

	void Set();
	bool Load(KeyFileHelper &kfh) noexcept;
	bool Save(KeyFileHelper &kfh) noexcept;
	void ResetToDefault() noexcept;
//	bool GammaChanged;

	const uint32_t &operator[](size_t const Index) const
	{
		return palbuff[Index];
	}

	size_t size() const
	{
		return 32;
	}

};
