#pragma once

/*
color.hpp

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

#ifndef RGB
#define RGB(r, g, b)   ((uint32_t)(((uint32_t)(r) | ((uint32_t)(g) << 8)) | (((uint32_t)(uint8_t)(b)) << 16)))
#define GetRValue(rgb) ((uint8_t)(rgb))
#define GetGValue(rgb) ((uint8_t)((rgb) >> 8))
#define GetBValue(rgb) ((uint8_t)((rgb) >> 16))
#define HSV RGB
#define GetHValue GetRValue
#define GetSValue GetGValue
#define GetVValue GetBValue
#endif

#ifndef RGBA
#define RGBA(r, g, b, a) ((uint32_t)((uint32_t)RGB(r, g, b) | (uint32_t)(a) << 24))
#define GetAValue(rgba)	 ((uint8_t)((rgba) >> 24))
#endif

#define RGB_2_BGR(c) (c & 0x00ff00) | (((c >> 16) & 0xff) | ((c & 0xff) << 16))
#define ATTR_RGBBACK_NEGF(rgb) (((((rgb & 0x0000FF00) >> 7) + (rgb & 0x000000FF) + ((rgb & 0x00FF0000) >> 16)) < 475 ? 0x000000FFFFFF000F : 0) + (((uint64_t)rgb << 40) | (BACKGROUND_TRUECOLOR + FOREGROUND_TRUECOLOR)))
#define ATTR_RGBBACK_NEGF2(rgb) (((((rgb & 0x0000FF00) >> 7) + (rgb & 0x000000FF) + ((rgb & 0x00FF0000) >> 16)) < 475 ? 0x000000FFFFFF000F : 0) + (((uint64_t)rgb << 40) | (BACKGROUND_TRUECOLOR + FOREGROUND_TRUECOLOR)))

#ifndef IS_BIG_ENDIAN
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	#define IS_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
	#define IS_BIG_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
		defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || \
		defined(__MIPSEB__)
	#define IS_BIG_ENDIAN 1
#else
	#define IS_BIG_ENDIAN 0
#endif
#endif

uint32_t HSV_2_RGB(uint32_t hsv);
uint32_t RGB_2_HSV(uint32_t rgb);
