#ifndef _COLORER_ENCODINGS_H_
#define _COLORER_ENCODINGS_H_

#include<unicode/String.h>

/** Encodings information.
    - All codepage definitions are read from list of ANSI -> Unicode
      associative files. These codepage files could be taken from
      www.unicode.org too.
    - Two-stage tables are used to access Unicode -> ANSI transformation.
    @ingroup unicode
*/
class Encodings{
public:
  // reverse order int integer notation!
  const static int ENC_UTF8_BOM;
  const static int ENC_UTF16_BOM;
  const static int ENC_UTF16BE_BOM;
  const static int ENC_UTF32_BOM;
  const static int ENC_UTF32BE_BOM;

  const static int ENC_UTF32BE;
  const static int ENC_UTF32;
  const static int ENC_UTF16BE;
  const static int ENC_UTF16;
  const static int ENC_UTF8;

  /** returns Byte Order Mark bytes for specified Unicode encoding */
  static byte* getEncodingBOM(int encoding);
  /** returns Byte Order Mark bytes <b>Length</b> for specified Unicode encoding */
  static int getEncodingBOMSize(int encoding);
  static int isMultibyteEncoding(int encoding);
  static int toBytes(int encoding, wchar, byte*);

  static char toChar(int, wchar);
  static wchar toWChar(int, char);

  static int getEncodingNamesNum();
  static int getDefaultEncodingIndex();
  static const char *getDefaultEncodingName();

  static int getEncodingIndex(const char *enc);
  static const char* getEncodingName(int enc);
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
