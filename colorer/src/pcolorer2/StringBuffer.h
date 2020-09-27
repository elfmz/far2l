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
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
