#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "pcolorer.h"

wchar_t* rtrim(wchar_t* str);
wchar_t* ltrim(wchar_t* str);
wchar_t* trim(wchar_t* str);

wchar_t* PathToFull(const wchar_t* path, bool unc);
UnicodeString* PathToFullS(const wchar_t* path, bool unc);

#endif

/* ***** BEGIN LICENSE BLOCK *****
 * Copyright (C) 1999-2009 Cail Lomecb <irusskih at gmail dot com>.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 * ***** END LICENSE BLOCK ***** */
