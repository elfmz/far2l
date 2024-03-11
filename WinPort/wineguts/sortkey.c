#include <stddef.h>
#include "../WinCompat.h"
#define inline

/*
 * Unicode sort key generation
 *
 * Copyright 2003 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include "unicode.h"

extern unsigned int wine_decompose( int flags, WCHAR ch, WCHAR *dst, unsigned int dstlen );
extern const unsigned int collation_table[];

/*
 * flags - normalization NORM_* flags
 *
 * FIXME: 'variable' flag not handled
 */
int wine_get_sortkey(int flags, const WCHAR *src, int srclen, char *dst, int dstlen)
{
    WCHAR dummy[4]; /* no decomposition is larger than 4 chars */
    int key_len[4];
    char *key_ptr[4];
    const WCHAR *src_save = src;
    int srclen_save = srclen;

    key_len[0] = key_len[1] = key_len[2] = key_len[3] = 0;
    for (; srclen; srclen--, src++)
    {
        if (!*src && (flags & NORM_STOP_ON_NULL) != 0)
        {
            srclen = 0;
            break;
        }
        unsigned int i, decomposed_len = 1;/*wine_decompose(*src, dummy, 4);*/
        dummy[0] = *src;
        if (decomposed_len)
        {
            for (i = 0; i < decomposed_len; i++)
            {
                WCHAR wch = dummy[i];
                unsigned int ce;

                /* tests show that win2k just ignores NORM_IGNORENONSPACE,
                 * and skips white space and punctuation characters for
                 * NORM_IGNORESYMBOLS.
                 */
                if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
                    continue;

                if (flags & NORM_IGNORECASE) wch = tolowerW(wch);

                ce = collation_table[collation_table[((USHORT)wch) >> 8] + (wch & 0xff)];
                if (ce != (unsigned int)-1)
                {
                    if (ce >> 16) key_len[0] += 2;
                    if ((ce >> 8) & 0xff) key_len[1]++;
                    if ((ce >> 4) & 0x0f) key_len[2]++;
                    if (ce & 1)
                    {
                        if (wch >> 8) key_len[3]++;
                        key_len[3]++;
                    }
                }
                else
                {
                    key_len[0] += 2;
                    if (wch >> 8) key_len[0]++;
                    if (wch & 0xff) key_len[0]++;
                }
            }
        }
    }

    if (!dstlen) /* compute length */
        /* 4 * '\1' + key length */
        return key_len[0] + key_len[1] + key_len[2] + key_len[3] + 4;

    if (dstlen < key_len[0] + key_len[1] + key_len[2] + key_len[3] + 4 + 1)
        return 0; /* overflow */

    src = src_save;
    srclen = srclen_save;

    key_ptr[0] = dst;
    key_ptr[1] = key_ptr[0] + key_len[0] + 1;
    key_ptr[2] = key_ptr[1] + key_len[1] + 1;
    key_ptr[3] = key_ptr[2] + key_len[2] + 1;

    for (; srclen; srclen--, src++)
    {
        if (!*src && (flags & NORM_STOP_ON_NULL) != 0)
        {
            srclen = 0;
            break;
        }
        unsigned int i, decomposed_len = 1;/*wine_decompose(*src, dummy, 4);*/
        dummy[0] = *src;
        if (decomposed_len)
        {
            for (i = 0; i < decomposed_len; i++)
            {
                WCHAR wch = dummy[i];
                unsigned int ce;

                /* tests show that win2k just ignores NORM_IGNORENONSPACE,
                 * and skips white space and punctuation characters for
                 * NORM_IGNORESYMBOLS.
                 */
                if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
                    continue;

                if (flags & NORM_IGNORECASE) wch = tolowerW(wch);

                ce = collation_table[collation_table[((USHORT)wch) >> 8] + (wch & 0xff)];
                if (ce != (unsigned int)-1)
                {
                    WCHAR key;
                    if ((key = ce >> 16))
                    {
                        *key_ptr[0]++ = key >> 8;
                        *key_ptr[0]++ = key & 0xff;
                    }
                    /* make key 1 start from 2 */
                    if ((key = (ce >> 8) & 0xff)) *key_ptr[1]++ = key + 1;
                    /* make key 2 start from 2 */
                    if ((key = (ce >> 4) & 0x0f)) *key_ptr[2]++ = key + 1;
                    /* key 3 is always a character code */
                    if (ce & 1)
                    {
                        if (wch >> 8) *key_ptr[3]++ = wch >> 8;
                        if (wch & 0xff) *key_ptr[3]++ = wch & 0xff;
                    }
                }
                else
                {
                    *key_ptr[0]++ = 0xff;
                    *key_ptr[0]++ = 0xfe;
                    if (wch >> 8) *key_ptr[0]++ = wch >> 8;
                    if (wch & 0xff) *key_ptr[0]++ = wch & 0xff;
                }
            }
        }
    }

    *key_ptr[0] = '\1';
    *key_ptr[1] = '\1';
    *key_ptr[2] = '\1';
    *key_ptr[3]++ = '\1';
    *key_ptr[3] = 0;

    return key_ptr[3] - dst;
}

#define DIACRITIC_WEIGHT 0x01
#define CASE_WEIGHT      0x02

static unsigned int eval_weight(WCHAR ch, unsigned int types)
{
    unsigned int out;
    unsigned int w = collation_table[collation_table[((USHORT)ch) >> 8] + (ch & 0xff)];
    if (w == (unsigned int)-1) {
        w = ch;
    }
    out = (w >> 16) & 0xffff;

    if ((types & DIACRITIC_WEIGHT) != 0) {
        out|= (((w >> 8) & 0xff) << 16);
    }
    if (ch != '.') { // dot is first
        const unsigned short t = get_char_typeW(ch);
        if ((t & C1_DIGIT) != 0) {
            out|= 0x40000000;
        }else if ((t & (C1_PUNCT | C1_SPACE | C1_CNTRL | C1_BLANK)) == 0) {
            out|= 0x60000000;
        } else if ((t & C1_PUNCT) == 0) {
            out|= 0x20000000;
        } else {
            out|= 0x10000000;
        }
        if ((t & C1_UPPER) == 0 && (types & CASE_WEIGHT) != 0) {
            out|= 0x08000000;
        }
    }

    return out;
}

static void inc_str_pos(const WCHAR **str, int *len, int *dpos, int *dlen)
{
    (*dpos)++;
    if (*dpos == *dlen)
    {
        *dpos = *dlen = 0;
        (*str)++;
        (*len)--;
    }
}

static inline int compare_weights(int flags, const WCHAR *str1, int len1,
                                  const WCHAR *str2, int len2, unsigned int types)
{
    int dpos1 = 0, dpos2 = 0, dlen1 = 0, dlen2 = 0;
    WCHAR dstr1[4], dstr2[4];
    unsigned int ce1, ce2;

    /* 32-bit collation element table format:
     * unicode weight - high 16 bit, diacritic weight - high 8 bit of low 16 bit,
     * case weight - high 4 bit of low 8 bit.
     */
    while (len1 > 0 && len2 > 0)
    {
        if (flags & NORM_STOP_ON_NULL)
        {
            if (!*str1) len1 = 0;
            if (!*str2) len2 = 0;
            if (!*str1 || !*str2) break;
        }

        if (!dlen1) dlen1 = wine_decompose(0, *str1, dstr1, 4);
        if (!dlen2) dlen2 = wine_decompose(0, *str2, dstr2, 4);

        if (flags & NORM_IGNORESYMBOLS)
        {
            int skip = 0;
            /* FIXME: not tested */
            if (get_char_typeW(dstr1[dpos1]) & (C1_PUNCT | C1_SPACE))
            {
                inc_str_pos(&str1, &len1, &dpos1, &dlen1);
                skip = 1;
            }
            if (get_char_typeW(dstr2[dpos2]) & (C1_PUNCT | C1_SPACE))
            {
                inc_str_pos(&str2, &len2, &dpos2, &dlen2);
                skip = 1;
            }
            if (skip) continue;
        }

       /* hyphen and apostrophe are treated differently depending on
        * whether SORT_STRINGSORT specified or not
        */
        if (!(flags & SORT_STRINGSORT))
        {
            if (dstr1[dpos1] == '-' || dstr1[dpos1] == '\'')
            {
                if (dstr2[dpos2] != '-' && dstr2[dpos2] != '\'')
                {
                    inc_str_pos(&str1, &len1, &dpos1, &dlen1);
                    continue;
                }
            }
            else if (dstr2[dpos2] == '-' || dstr2[dpos2] == '\'')
            {
                inc_str_pos(&str2, &len2, &dpos2, &dlen2);
                continue;
            }
        }

        ce1 = eval_weight(dstr1[dpos1], types);
        ce2 = eval_weight(dstr2[dpos2], types);
        if (ce1 && ce2 && ce1 != ce2)
            return (ce1 > ce2) ? 1 : -1;
        if (!ce1 || ce2)
          inc_str_pos(&str1, &len1, &dpos1, &dlen1);
        if (!ce2 || ce1)
          inc_str_pos(&str2, &len2, &dpos2, &dlen2);
    }
    while (len1)
    {
        if (!*str1 && (flags & NORM_STOP_ON_NULL) != 0)
        {
            len1 = 0;
            break;
        }
        if (!dlen1) dlen1 = wine_decompose(0, *str1, dstr1, 4);

        ce1 = eval_weight(dstr1[dpos1], types);
        if (ce1) break;
        inc_str_pos(&str1, &len1, &dpos1, &dlen1);
    }
    while (len2)
    {
        if (!*str2 && (flags & NORM_STOP_ON_NULL) != 0)
        {
            len2 = 0;
            break;
        }
        if (!dlen2) dlen2 = wine_decompose(0, *str2, dstr2, 4);

        ce2 = eval_weight(dstr2[dpos2], types);
        if (ce2) break;
        inc_str_pos(&str2, &len2, &dpos2, &dlen2);
    }
    return (len1 > len2) ? 1 : ((len1 < len2) ? -1 : 0);
}

int wine_compare_string(int flags, const WCHAR *str1, int len1,
                        const WCHAR *str2, int len2)
{
    unsigned int types = 0;
    if (!(flags & NORM_IGNORENONSPACE))
        types|= DIACRITIC_WEIGHT;
    if (!(flags & NORM_IGNORECASE))
        types|= CASE_WEIGHT;
    return compare_weights(flags, str1, len1, str2, len2, types);
}
