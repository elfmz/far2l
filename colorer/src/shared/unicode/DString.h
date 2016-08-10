#ifndef _COLORER_DSTRING_H_
#define _COLORER_DSTRING_H_

#include<unicode/String.h>

/** Dynamic string class.
    Simple unicode wrapper over any other source.
    @ingroup unicode
*/
class DString : public String{
public:
  /** String clone operator */
  DString &operator=(const DString &cstring);

  /** Creates string from byte stream with encoding autodetecting
      @param stream Input raw byte stream, can't be null.
      @param size Size of input buffer
      @param def_encoding Default encoding to be used, if no other
             variants found.
  */
  DString(const byte *stream, int size, int def_encoding = -1);

  /** String from single-byte character buffer.
      @param string Character buffer, can't be null.
      @param s Start string position. Zero - create from start of buffer.
      @param l Length of created string. If -1, autodetects string length with
             last zero byte.
      @param encoding Encoding, to use for char2unicode transformations.
             If -1, default encoding will be used.
  */
  DString(const char *string, int s = 0, int l = -1, int encoding = -1);

  /** String from unicode two-byte character buffer.
      @param string Unicode character buffer, can't be null.
      @param s Start string position. Zero - create from start of buffer.
      @param l Length of created string. If -1, autodetects string length with
             last zero char.
  */
  DString(const wchar *string, int s = 0, int l = -1);

  /** String from UCS4 four-byte character buffer.
      @param string UCS4 unicode character buffer, can't be null.
      @param s Starting string position. Zero - create from start of buffer.
      @param l Length of created string. If -1, autodetects string length with
             last zero char.
  */
  DString(const w4char *string, int s = 0, int l = -1);

  /** String from any @c String implementing interface.
      @param cstring String class instance, can't be null.
      @param s Starting string position.
      @param l Length of created string. If -1, autodetects string length with
             cstring.length() call.
  */
  DString(const String *cstring, int s = 0, int l = -1);

  /** String from any @c String implementing interface.
      @param cstring String class instance.
      @param s Starting string position.
      @param l Length of created string. If -1, autodetects string length with
             cstring.length() call.
  */
  DString(const String &cstring, int s = 0, int l = -1);

  /** Empty String */
  DString();
  ~DString();

  wchar operator[](int i) const;
  int length() const;

  /** Creates Dynamic string as substring of called object.
      @param s Starting string position.
      @param l Length of created string. If -1, creates string
             till end of current.
  */
  String *substring(int s, int l = -1) const;
protected:
  enum EStreamType{
    ST_CHAR = 0,
    ST_UTF16,
    ST_UTF16_BE,
    ST_CSTRING,
    ST_UTF8,
    ST_UTF32,
    ST_UTF32_BE
  };

  EStreamType type;
  int encodingIdx;
  union{
    const char *str;
    const wchar *wstr;
    const w4char *w4str;
    const String *cstr;
    wchar *stream_wstr;
  };
  int start, len;

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
