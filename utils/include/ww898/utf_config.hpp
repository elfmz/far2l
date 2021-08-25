/*
 * MIT License
 * 
 * Copyright (c) 2017-2019 Mikhail Pilin
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

// Normally `__cpp_lib_string_view` should be defined in string header
#include <string>

#if !defined(__cpp_lib_string_view)
#if defined(_MSVC_LANG)
#define __cpp_lib_string_view _MSVC_LANG
#else
#define __cpp_lib_string_view __cplusplus
#endif
#endif

namespace ww898 {
namespace utf {
static uint32_t const max_unicode_code_point = 0x10FFFF;
}}
