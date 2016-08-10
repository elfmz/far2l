#ifndef _COLORER_SSTRING_H_
#define _COLORER_SSTRING_H_

#include<unicode/String.h>

/**
 * Static unicode string.
 * Static string, do not depend on called character data.
 * @ingroup unicode
*/
class SString : public String{
public:
  /**
   * Static string constructor from String source
   * @param cstring source string, can't be null.
   */
  SString(const String *cstring, int s = 0, int l = -1);

  /**
   * Static string constructor from String source
   * @param cstring source string, can't be null.
   */
  SString(const String &cstring, int s = 0, int l = -1);

  /**
   * Static string constructor from char stream
   * @param str source string, can't be null.
   */
  SString(char *str, int enc = -1);
  SString(wchar *str);

  /**
   * Static string constructor from integer number
   */
  SString(int no);

  /**
   * Empty static string constructor
   */
  ~SString();

  String *substring(int s, int l = -1) const;

  wchar operator[](int i) const;
  int length() const;

protected:
  SString();
  void construct(const String *cstring, int s, int l);

  wchar *wstr;
  int len;
private:
  SString &operator=(SString &cstring);
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
