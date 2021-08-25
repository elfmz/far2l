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

#include <cstdint>
#include <stdexcept>

namespace ww898 {
namespace utf {

//                1         0
//       98765432109876543210
//       ||||||||||||||||||||
// 110110xxxxxxxxxx|||||||||| high surrogate
//           110111xxxxxxxxxx low  surrogate
struct utf16 final
{
    static size_t const max_unicode_symbol_size = 2;
    static size_t const max_supported_symbol_size = max_unicode_symbol_size;

    static uint32_t const max_supported_code_point = 0x10FFFF;

    using char_type = uint16_t;

    static char_type const min_surrogate = 0xD800;
    static char_type const max_surrogate = 0xDFFF;

    static char_type const min_surrogate_high = 0xD800;
    static char_type const max_surrogate_high = 0xDBFF;

    static char_type const min_surrogate_low = 0xDC00;
    static char_type const max_surrogate_low = 0xDFFF;

    template<typename ReadFn>
    static uint32_t read(ReadFn && read_fn)
    {
        char_type const ch0 = read_fn();
        if (ch0 < 0xD800) // [0x0000‥0xD7FF]
            return ch0;
        if (ch0 < 0xDC00) // [0xD800‥0xDBFF] [0xDC00‥0xDFFF]
        {
            char_type const ch1 = read_fn(); if (ch1 >> 10 != 0x37) return -1; //throw std::runtime_error("The low utf16 surrogate char is expected");
            return (ch0 << 10) + ch1 - 0x35FDC00;
        }
        if (ch0 < 0xE000)
            return -1; //throw std::runtime_error("The high utf16 surrogate char is expected");
        // [0xE000‥0xFFFF]
        return ch0;
    }

    template<typename WriteFn>
    static void write(uint32_t const cp, WriteFn && write_fn)
    {
        if (cp < 0xD800) // [0x0000‥0xD7FF]
            write_fn(static_cast<char_type>(cp));
        else if (cp < 0x10000)
        {
            if (cp < 0xE000)
                throw std::runtime_error("The utf16 code point can not be in surrogate range");
            // [0xE000‥0xFFFF]
            write_fn(static_cast<char_type>(cp));
        }
        else if (cp < 0x110000) // [0xD800‥0xDBFF] [0xDC00‥0xDFFF]
        {
            write_fn(static_cast<char_type>(0xD7C0 + (cp >> 10        )));
            write_fn(static_cast<char_type>(0xDC00 + (cp       & 0x3FF)));
        }
        else
            throw std::runtime_error("Too large the utf16 code point");
    }
};

}}
