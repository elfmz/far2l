﻿/*
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

#include <ww898/cp_utf8.hpp>
#include <ww898/cp_utf16.hpp>
#include <ww898/cp_utf32.hpp>
#include <ww898/cp_utfw.hpp>

namespace ww898 {
namespace utf {
namespace detail {

template<typename Ch>
struct utf_selector final {};

template<> struct utf_selector<         char> final { using type = utf8 ; };
template<> struct utf_selector<unsigned char> final { using type = utf8 ; };
template<> struct utf_selector<signed   char> final { using type = utf8 ; };
template<> struct utf_selector<char16_t     > final { using type = utf16; };
template<> struct utf_selector<char32_t     > final { using type = utf32; };
template<> struct utf_selector<wchar_t      > final { using type = utfw ; };

}

template<typename Ch>
using utf_selector = detail::utf_selector<typename std::decay<Ch>::type>;

template<typename Ch>
using utf_selector_t = typename utf_selector<Ch>::type;

}}
