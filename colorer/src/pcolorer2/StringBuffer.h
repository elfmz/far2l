#ifndef _COLORER_STRINGBUFFER_H_
#define _COLORER_STRINGBUFFER_H_

#include<colorer/unicode/SString.h>

/** Unicode growable StringBuffer.
    @ingroup unicode
*/
class StringBuffer : public SString{
public:
  /** Creates empty string buffer */
  StringBuffer();
  /** Creates empty string buffer */
  StringBuffer(int alloc);
  /** Creates string buffer with @c string */
  StringBuffer(const char *string, int s = 0, int l = -1);
  /** Creates string buffer with @c string */
  StringBuffer(const w2char *string, int s = 0, int l = -1);
  StringBuffer(const w4char *string, int s = 0, int l = -1);
  /** Creates string buffer with @c string */
  StringBuffer(const String *cstring, int s = 0, int l = -1);
  /** Creates string buffer with @c string */
  StringBuffer(const String &cstring, int s = 0, int l = -1);
  /** Creates string buffer with wchar_t string */
  StringBuffer(const wchar_t *wstring, int s = 0, int l = -1);
  /** Destructor */
  ~StringBuffer();

  /** Appends to this string buffer @c string */
  StringBuffer &append(const String &string);
  /** Appends to this string buffer @c string */
  StringBuffer &append(const String* string);
  /** Appends to this string buffer @c string */
  StringBuffer &append(wchar c);

  StringBuffer &append(const wchar_t* wstring);

private:
#if 0
  int alloc;
#endif // if 0
};

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
