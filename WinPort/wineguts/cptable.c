#include <stddef.h>
#include "../WinCompat.h"
#define inline

/*
 * Codepage tables
 *
 * Copyright 2000 Alexandre Julliard
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

#include <stdlib.h>

#include "unicode.h"

/* Everything below this line is generated automatically by make_unicode */
/* ### cpmap begin ### */
extern const struct sbcs_table cptable_037;
extern const struct sbcs_table cptable_424;
extern const struct sbcs_table cptable_437;
extern const struct sbcs_table cptable_500;
extern const struct sbcs_table cptable_737;
extern const struct sbcs_table cptable_775;
extern const struct sbcs_table cptable_850;
extern const struct sbcs_table cptable_852;
extern const struct sbcs_table cptable_855;
extern const struct sbcs_table cptable_856;
extern const struct sbcs_table cptable_857;
extern const struct sbcs_table cptable_860;
extern const struct sbcs_table cptable_861;
extern const struct sbcs_table cptable_862;
extern const struct sbcs_table cptable_863;
extern const struct sbcs_table cptable_864;
extern const struct sbcs_table cptable_865;
extern const struct sbcs_table cptable_866;
extern const struct sbcs_table cptable_866uk;
extern const struct sbcs_table cptable_869;
extern const struct sbcs_table cptable_874;
extern const struct sbcs_table cptable_875;
#ifndef NO_EACP
extern const struct dbcs_table cptable_932;
extern const struct dbcs_table cptable_936;
extern const struct dbcs_table cptable_949;
extern const struct dbcs_table cptable_950;
#endif
extern const struct sbcs_table cptable_1006;
extern const struct sbcs_table cptable_1026;
extern const struct sbcs_table cptable_1250;
extern const struct sbcs_table cptable_1251;
extern const struct sbcs_table cptable_1252;
extern const struct sbcs_table cptable_1253;
extern const struct sbcs_table cptable_1254;
extern const struct sbcs_table cptable_1255;
extern const struct sbcs_table cptable_1256;
extern const struct sbcs_table cptable_1257;
extern const struct sbcs_table cptable_1258;
#ifndef NO_EACP
extern const struct dbcs_table cptable_1361;
#endif
extern const struct sbcs_table cptable_10000;
#ifndef NO_EACP
extern const struct dbcs_table cptable_10001;
extern const struct dbcs_table cptable_10002;
extern const struct dbcs_table cptable_10003;
#endif
extern const struct sbcs_table cptable_10004;
extern const struct sbcs_table cptable_10005;
extern const struct sbcs_table cptable_10006;
extern const struct sbcs_table cptable_10007;
#ifndef NO_EACP
extern const struct dbcs_table cptable_10008;
#endif
extern const struct sbcs_table cptable_10010;
extern const struct sbcs_table cptable_10017;
extern const struct sbcs_table cptable_10021;
extern const struct sbcs_table cptable_10029;
extern const struct sbcs_table cptable_10079;
extern const struct sbcs_table cptable_10081;
extern const struct sbcs_table cptable_10082;
extern const struct sbcs_table cptable_20127;
extern const struct sbcs_table cptable_20866;
extern const struct sbcs_table cptable_20880;
#ifndef NO_EACP
extern const struct dbcs_table cptable_20932;
#endif
extern const struct sbcs_table cptable_21866;
extern const struct sbcs_table cptable_28591;
extern const struct sbcs_table cptable_28592;
extern const struct sbcs_table cptable_28593;
extern const struct sbcs_table cptable_28594;
extern const struct sbcs_table cptable_28595;
extern const struct sbcs_table cptable_28596;
extern const struct sbcs_table cptable_28597;
extern const struct sbcs_table cptable_28598;
extern const struct sbcs_table cptable_28599;
extern const struct sbcs_table cptable_28600;
extern const struct sbcs_table cptable_28603;
extern const struct sbcs_table cptable_28604;
extern const struct sbcs_table cptable_28605;
extern const struct sbcs_table cptable_28606;

static const union cptable * const cptables[] =
{
    (const union cptable *)&cptable_037,
    (const union cptable *)&cptable_424,
    (const union cptable *)&cptable_437,
    (const union cptable *)&cptable_500,
    (const union cptable *)&cptable_737,
    (const union cptable *)&cptable_775,
    (const union cptable *)&cptable_850,
    (const union cptable *)&cptable_852,
    (const union cptable *)&cptable_855,
    (const union cptable *)&cptable_856,
    (const union cptable *)&cptable_857,
    (const union cptable *)&cptable_860,
    (const union cptable *)&cptable_861,
    (const union cptable *)&cptable_862,
    (const union cptable *)&cptable_863,
    (const union cptable *)&cptable_864,
    (const union cptable *)&cptable_865,
    (const union cptable *)&cptable_866,
    (const union cptable *)&cptable_866uk,
    (const union cptable *)&cptable_869,
    (const union cptable *)&cptable_874,
    (const union cptable *)&cptable_875,
#ifndef NO_EACP
    (const union cptable *)&cptable_932,
    (const union cptable *)&cptable_936,
    (const union cptable *)&cptable_949,
    (const union cptable *)&cptable_950,
#endif
    (const union cptable *)&cptable_1006,
    (const union cptable *)&cptable_1026,
    (const union cptable *)&cptable_1250,
    (const union cptable *)&cptable_1251,
    (const union cptable *)&cptable_1252,
    (const union cptable *)&cptable_1253,
    (const union cptable *)&cptable_1254,
    (const union cptable *)&cptable_1255,
    (const union cptable *)&cptable_1256,
    (const union cptable *)&cptable_1257,
    (const union cptable *)&cptable_1258,
#ifndef NO_EACP
    (const union cptable *)&cptable_1361,
#endif
    (const union cptable *)&cptable_10000,
#ifndef NO_EACP
    (const union cptable *)&cptable_10001,
    (const union cptable *)&cptable_10002,
    (const union cptable *)&cptable_10003,
#endif
    (const union cptable *)&cptable_10004,
    (const union cptable *)&cptable_10005,
    (const union cptable *)&cptable_10006,
    (const union cptable *)&cptable_10007,
#ifndef NO_EACP
    (const union cptable *)&cptable_10008,
#endif
    (const union cptable *)&cptable_10010,
    (const union cptable *)&cptable_10017,
    (const union cptable *)&cptable_10021,
    (const union cptable *)&cptable_10029,
    (const union cptable *)&cptable_10079,
    (const union cptable *)&cptable_10081,
    (const union cptable *)&cptable_10082,
    (const union cptable *)&cptable_20127,
    (const union cptable *)&cptable_20866,
    (const union cptable *)&cptable_20880,
#ifndef NO_EACP
    (const union cptable *)&cptable_20932,
#endif
    (const union cptable *)&cptable_21866,
    (const union cptable *)&cptable_28591,
    (const union cptable *)&cptable_28592,
    (const union cptable *)&cptable_28593,
    (const union cptable *)&cptable_28594,
    (const union cptable *)&cptable_28595,
    (const union cptable *)&cptable_28596,
    (const union cptable *)&cptable_28597,
    (const union cptable *)&cptable_28598,
    (const union cptable *)&cptable_28599,
    (const union cptable *)&cptable_28600,
    (const union cptable *)&cptable_28603,
    (const union cptable *)&cptable_28604,
    (const union cptable *)&cptable_28605,
    (const union cptable *)&cptable_28606,
};
/* ### cpmap end ### */
/* Everything above this line is generated automatically by make_unicode */

#define NB_CODEPAGES  (sizeof(cptables)/sizeof(cptables[0]))


/* get the table of a given code page */
const union cptable *wine_cp_get_table( unsigned int codepage )
{
    const union cptable * const *base = cptables;
    size_t cnt = NB_CODEPAGES;

    while (cnt) {
        const union cptable * const *mid = base + (cnt >> 1);

        if (codepage == (*mid)->info.codepage) {
            return *mid;
        }

        if (codepage > (*mid)->info.codepage) { /* key > p: move right */
            base = mid + 1;
            cnt--;
        } /* else move left */

        cnt>>= 1;
    }

    return (NULL);
}


/* enum valid codepages */
const union cptable *wine_cp_enum_table( unsigned int index )
{
    if (index >= NB_CODEPAGES) return NULL;
    return cptables[index];
}
