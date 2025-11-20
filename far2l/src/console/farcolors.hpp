#pragma once

/*
farcolors.hpp
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

//#include <WinCompat.h>
#include "colors.hpp"
#include "palette.hpp"
#include "farcolors.hpp"

#define SIZE_ARRAY_FARCOLORS 160

class FarColors : NonCopyable
{
public:
	static uint64_t setcolors[SIZE_ARRAY_FARCOLORS];
	uint64_t colors[SIZE_ARRAY_FARCOLORS];
	static uint32_t GammaCorrection;
	static bool GammaChanged;
    static FarColors FARColors;

	FarColors() noexcept;
	~FarColors() noexcept;

	static void InitFarColors( ) noexcept;
	static void SaveFarColors( ) noexcept;

	void Set() noexcept {
		memcpy(setcolors, colors, sizeof(setcolors[0]) * SIZE_ARRAY_FARCOLORS);
	}

	static void Set(uint64_t *colors) noexcept {
		memcpy(setcolors, colors, sizeof(setcolors[0]) * SIZE_ARRAY_FARCOLORS);
	}

	static void SetRange(size_t startindex, size_t colorscount, uint64_t *colors ) noexcept {
		if (colors && startindex >= 0 && startindex < SIZE_ARRAY_FARCOLORS && startindex + colorscount <= SIZE_ARRAY_FARCOLORS)
			memcpy(setcolors + startindex, colors, sizeof(setcolors[0]) * colorscount);
	}

	bool Load(KeyFileHelper &kfh) noexcept;
	bool Save(KeyFileHelper &kfh) noexcept;
	void ResetToDefaultIndex(uint8_t *indexes = nullptr) noexcept;
	void ResetToDefaultIndexRGB(uint8_t *indexes = nullptr) noexcept;
	void Reset(bool RGB = false) noexcept;

	const uint64_t &operator[](size_t const Index) const noexcept {
		return colors[Index];
	}

	size_t size() const noexcept {
		return SIZE_ARRAY_FARCOLORS;
	}
};

inline uint64_t FarColorToReal(unsigned int FarColor)
{
	return (FarColor < SIZE_ARRAY_FARCOLORS) ? FarColors::setcolors[FarColor] : 4 * 16 + 15;
}

extern uint8_t BlackColorsIndex16[SIZE_ARRAY_FARCOLORS];
extern uint8_t DefaultColorsIndex16[SIZE_ARRAY_FARCOLORS];

