#pragma once
#ifndef __FARKEYS_HPP__
#define __FARKEYS_HPP__
#ifndef FAR_USE_INTERNALS
#define FAR_USE_INTERNALS
#endif	// END FAR_USE_INTERNALS
#ifdef FAR_USE_INTERNALS
/*
keys.hpp

Внутренние фаровские имена клавиш

ВНИМАНИЕ!
	Новые пвсевдоклавиши, типа KEY_FOCUS_CHANGED и KEY_CONSOLE_BUFFER_RESIZE
	добавлять между KEY_END_FKEY и KEY_END_SKEY
*/
#else	// ELSE FAR_USE_INTERNALS
/*
	farkeys.h

	Inside KeyName for FAR Manager <%VERSION%>
*/
#endif	// END FAR_USE_INTERNALS

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

EXCEPTION:
Far Manager plugins that use this header file can be distributed under any
other possible license with no implications from the above license on them.
*/

#include <cstdint>
#include <limits>
#include <WinCompat.h>
#include "far2sdk/farkeys.h"


#define STRIP_KEY_CODE(K)  (((FarKey)(K)) & ~KEY_CTRLMASK)
#define KEY_IN_RANGE(K, BASE) ((K) >= (BASE) && (K) < (BASE) + KEYS_PER_RANGE)
#define IS_KEY_NORMAL(K)      KEY_IN_RANGE((K), 0)
#define IS_KEY_EXTENDED(K)    KEY_IN_RANGE((K), EXTENDED_KEY_BASE)
#define IS_KEY_INTERNAL(K)    KEY_IN_RANGE((K), INTERNAL_KEY_BASE)
#define IS_KEY_INTERNAL_2(K)  KEY_IN_RANGE((K), INTERNAL_KEY_BASE_2)

#ifdef FAR_USE_INTERNALS
# define IS_KEY_MACRO(K)    KEY_IN_RANGE((K), INTERNAL_MACRO_BASE)
# define IS_INTERNAL_KEY_REAL(Key)                                                                              \
	((Key) == KEY_MSWHEEL_UP || (Key) == KEY_MSWHEEL_DOWN || (Key) == KEY_NUMDEL || (Key) == KEY_NUMENTER      \
			|| (Key) == KEY_MSWHEEL_LEFT || (Key) == KEY_MSWHEEL_RIGHT || (Key) == KEY_MSLCLICK                \
			|| (Key) == KEY_MSRCLICK || (Key) == KEY_MSM1CLICK || (Key) == KEY_MSM2CLICK                       \
			|| (Key) == KEY_MSM3CLICK)
#endif	// END FAR_USE_INTERNALS

#endif	// __FARKEYS_HPP__
