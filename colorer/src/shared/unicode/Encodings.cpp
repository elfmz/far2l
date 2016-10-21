
#include<unicode/Encodings.h>
#include<unicode/x_encodings.h>
#include<string.h>

UnsupportedEncodingException::UnsupportedEncodingException(){};
UnsupportedEncodingException::UnsupportedEncodingException(const String& msg){
  message = new StringBuffer("UnsupportedEncodingException: ");
  message->append(msg);
}

const int Encodings::ENC_UTF8_BOM    = 0xBFBBEF;
const int Encodings::ENC_UTF16_BOM   = 0xFEFF;
const int Encodings::ENC_UTF16BE_BOM = 0xFFFE;
const int Encodings::ENC_UTF32_BOM   = 0x0000FEFF;
const int Encodings::ENC_UTF32BE_BOM = 0xFFFE0000;

const int Encodings::ENC_UTF32BE = (-6);
const int Encodings::ENC_UTF32   = (-5);
const int Encodings::ENC_UTF16BE = (-4);
const int Encodings::ENC_UTF16   = (-3);
const int Encodings::ENC_UTF8    = (-2);


#define ENC_UTF8_BOM    ((byte*)&Encodings::ENC_UTF8_BOM)
#define ENC_UTF16_BOM   ((byte*)&Encodings::ENC_UTF16_BOM)
#define ENC_UTF16BE_BOM ((byte*)&Encodings::ENC_UTF16BE_BOM)
#define ENC_UTF32_BOM   ((byte*)&Encodings::ENC_UTF32_BOM)
#define ENC_UTF32BE_BOM ((byte*)&Encodings::ENC_UTF32BE_BOM)
static byte *bomArray[5] = { ENC_UTF8_BOM, ENC_UTF16_BOM, ENC_UTF16BE_BOM, ENC_UTF32_BOM, ENC_UTF32BE_BOM };
static int bomArraySize[5] = { 3, 2, 2, 4, 4 };

byte* Encodings::getEncodingBOM(int encoding){
  if (encoding >= -1 || encoding < -6) throw UnsupportedEncodingException(DString("getEncodingBOM was called for bad encoding"));
  return bomArray[-encoding-2];
}

int Encodings::getEncodingBOMSize(int encoding){
  if (encoding >= -1 || encoding < -6) throw UnsupportedEncodingException(DString("getEncodingBOM was called for bad encoding"));
  return bomArraySize[-encoding-2];
}
int Encodings::isMultibyteEncoding(int encoding){
  return (encoding < -1);
}

int Encodings::getEncodingIndex(const char *enc){
  if (!enc) return -1;
  for(int i = 0; i < encAliasesNum; i++) {
	if (!stricmp(arr_idxEncodings[i].name, enc)){
		return arr_idxEncodings[i].pos;
	};
  }
  return -1;
}
const char* Encodings::getEncodingName(int enc){
  if (enc >= encNamesNum || enc < -6 || enc == -1) return null;
  for(int i = 0; i < encAliasesNum; i++)
    if (arr_idxEncodings[i].pos == enc)
      return arr_idxEncodings[i].name;
  return null;
}
int Encodings::getEncodingNamesNum(){
  return encNamesNum;
}
int Encodings::getDefaultEncodingIndex(){
  return defEncodingIdx;
}
const char *Encodings::getDefaultEncodingName(){
  return defEncoding;
}

int Encodings::toBytes(int encoding, wchar wc, byte *dest){
  if (encoding < -6 || encoding == -1 || encoding >= encNamesNum)
    throw UnsupportedEncodingException(SString(encoding));
  if (encoding >= 0){
    dest[0] = TO_CHAR(encoding, wc);
    return 1;
  };
  if (encoding == ENC_UTF8){
    int dpos = 0;
    if (wc <= 0x7F){
      dest[dpos] = wc & 0x7F;
    };
    if (wc > 0x7F && wc <= 0x7FF){
      dest[dpos] = 0xC0 + (wc>>6);
      dpos++;
      dest[dpos] = 0x80 + (wc&0x3F);
    };
    if (wc > 0x7FF && wc <= 0xFFFF){
      dest[dpos] = 0xE0 + (wc>>12);
      dpos++;
      dest[dpos] = 0x80 + ((wc>>6)&0x3F);
      dpos++;
      dest[dpos] = 0x80 + (wc&0x3F);
    };
    if (wc > 0xFFFF){
      dest[dpos] = 0xF0 + (wc>>14);
      dpos++;
      dest[dpos] = 0x80 + ((wc>>12)&0x3F);
      dpos++;
      dest[dpos] = 0x80 + ((wc>>6)&0x3F);
      dpos++;
      dest[dpos] = 0x80 + (wc&0x3F);
    };
    return dpos+1;
  };
  if (encoding == ENC_UTF16){
    dest[0] = wc&0xFF;
    dest[1] = (wc>>8)&0xFF;
    return 2;
  };
  if (encoding == ENC_UTF16BE){
    dest[1] = wc&0xFF;
    dest[0] = (wc>>8)&0xFF;
    return 2;
  };
#if (__WCHAR_MAX__ > 0xffff) // impossible for 16-bit wchar
  if (encoding == ENC_UTF32){
    dest[0] = wc&0xFF;
    dest[1] = (wc>>8)&0xFF;
    dest[2] = (wc>>16)&0xFF;
    dest[3] = (wc>>14)&0xFF;
    return 4;
  };

  if (encoding == ENC_UTF32BE){
    dest[3] = wc&0xFF;
    dest[2] = (wc>>8)&0xFF;
    dest[1] = (wc>>16)&0xFF;
    dest[0] = (wc>>14)&0xFF;
    return 4;
  };
#endif
  throw UnsupportedEncodingException(SString(encoding));
}

// no checks for encoding index validness!!!!
char Encodings::toChar(int eidx, wchar c){
//  if (eidx < 0 || eidx >= encNamesNum) throw UnsupportedEncodingException(SString(eidx));
  return TO_CHAR(eidx, c);
}
wchar Encodings::toWChar(int eidx, char c){
//  if (eidx < 0 || eidx >= encNamesNum) throw UnsupportedEncodingException(SString(eidx));
  return TO_WCHAR(eidx, c);
}

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
