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

#include <ww898/utf_selector.hpp>
#include <ww898/utf_config.hpp>

#include <cstdint>
#include <iterator>

namespace ww898 {
namespace utf {

namespace detail {

enum struct conv_impl { normal, random_interator, binary_copy };

template<
    typename Utf,
    typename Outf,
    typename It,
    typename Oit,
    conv_impl>
struct conv_strategy final
{
    bool operator()(It &it, It const eit, Oit &oit) const
    {
        auto tmp = it;
        bool read_ended = false;
        auto const read_fn = [&tmp, &eit, &read_ended]
            {
                if (tmp == eit) {
                    read_ended = true;
                    return -1;
                }

                return *tmp++;
            };
        auto const write_fn = [&oit] (typename Outf::char_type const ch) { oit.push_back(ch); };

        while (tmp != eit) {
            if (oit.fully_filled())
                return false;
            auto const cp = Utf::read(read_fn);
            if (read_ended || cp == (uint32_t)-1)
                return false;
            Outf::write(cp, write_fn);
            it = tmp;
        }

        return true;
    }
};

template<
    typename Utf,
    typename Outf,
    typename It,
    typename Oit>
struct conv_strategy<Utf, Outf, It, Oit, conv_impl::random_interator> final
{
    bool operator()(It &it, It const eit, Oit &oit) const
    {
        auto tmp = it;
        auto const write_fn = [&oit] (typename Outf::char_type const ch) { oit.push_back(ch); };
        if (eit - tmp >= static_cast<typename std::iterator_traits<It>::difference_type>(Utf::max_supported_symbol_size))
        {
            auto const fast_read_fn = [&tmp] { return *tmp++; };
            auto const fast_eit = eit - Utf::max_supported_symbol_size;
               while (tmp < fast_eit) {
                if (oit.fully_filled())
                       return false;
                   auto const cp = Utf::read(fast_read_fn);
                      if (cp == (uint32_t)-1)
                       return false;
                   Outf::write(cp, write_fn);
                it = tmp;
            }
        }
        bool read_ended = false;
        auto const read_fn = [&tmp, &eit, &read_ended]
            {
                if (tmp == eit) {
                    read_ended = true;
                }
                return (tmp != eit) ? *tmp++ : -1;
            };

        while (tmp != eit) {
            if (oit.fully_filled())
                return false;
            auto const cp = Utf::read(read_fn);
            if (read_ended || cp == (uint32_t)-1)
                return false;
            Outf::write(cp, write_fn);
            it = tmp;
        }
        return true;
    }
};

template<
    typename Utf,
    typename Outf,
    typename It,
    typename Oit>
struct conv_strategy<Utf, Outf, It, Oit, conv_impl::binary_copy> final
{
    bool operator()(It &it, It const eit, Oit &oit) const
    {
        auto tmp = it;

        while (tmp != eit) {
            if (oit.fully_filled())
                return false;
            oit.push_back(*tmp++);
            it = tmp;
        }

        return true;
    }
};

}

template<
    typename Utf,
    typename Outf,
    typename It,
    typename Eit,
    typename Oit>
bool conv(It & it, Eit && eit, Oit &oit)
{
    return detail::conv_strategy<Utf, Outf, //It, Oit,
            typename std::decay<It>::type,
            typename std::decay<Oit>::type,
            std::is_same<Utf, Outf>::value
                ? detail::conv_impl::binary_copy
                : std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<typename std::decay<It>::type>::iterator_category>::value
                    ? detail::conv_impl::random_interator
                    : detail::conv_impl::normal>()(
        it,
        std::forward<Eit>(eit),
        oit);
}

}}
