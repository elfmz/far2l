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

struct utf32 final
{
    static size_t const max_unicode_symbol_size = 1;
    static size_t const max_supported_symbol_size = 1;

    static uint32_t const max_supported_code_point = 0x7FFFFFFF;

    using char_type = uint32_t;

    template<typename ReadFn>
    static uint32_t read(ReadFn && read_fn)
    {
        char_type const ch = std::forward<ReadFn>(read_fn)();
        if (ch < 0x80000000)
            return ch;
        return -1; //throw std::runtime_error("Too large utf32 char");
    }

    template<typename WriteFn>
    static void write(uint32_t const cp, WriteFn && write_fn)
    {
        if (cp < 0x80000000)
            std::forward<WriteFn>(write_fn)(static_cast<char_type>(cp));
        else
            return throw std::runtime_error("Too large utf32 code point");
    }
};

}}
