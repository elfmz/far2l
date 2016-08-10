
#include<unicode/DString.h>
#include <stdio.h>
DString& DString::operator=(const DString &cstring){
  if (type == ST_UTF8) delete[] stream_wstr;
  type = cstring.type;
  encodingIdx = cstring.encodingIdx;
  str = cstring.str;
  start = cstring.start;
  len = cstring.len;
  if (type == ST_UTF8){
    stream_wstr = new wchar[len];
    for(int wi = 0; wi < len; wi++)
      stream_wstr[wi] = cstring.stream_wstr[wi];
  };
  return *this;
}

DString::DString(const byte *stream, int size, int def_encoding){
  start = 0;
  len = size;
  str = (char*)stream;

  type = ST_CHAR;

  if (def_encoding == Encodings::ENC_UTF8)    type = ST_UTF8;
  if (def_encoding == Encodings::ENC_UTF16)   type = ST_UTF16;
  if (def_encoding == Encodings::ENC_UTF16BE) type = ST_UTF16_BE;
  if (def_encoding == Encodings::ENC_UTF32)   type = ST_UTF32;
  if (def_encoding == Encodings::ENC_UTF32BE) type = ST_UTF32_BE;
  if (def_encoding > Encodings::getEncodingNamesNum())
    throw UnsupportedEncodingException(SString(def_encoding));
  encodingIdx = def_encoding;

  if (type == ST_CHAR && encodingIdx == -1){
    // check encoding parameter
    if (stream[0] == 0x3C && stream[1] == 0x3F){
      int p;
      int cps = 0, cpe = 0;
      for(p = 2; stream[p] != 0x3F && stream[p+1] != 0x3C && p < 100; p++){
        if (cps && stream[p] == stream[cps-1]){
          cpe = p;
          break;
        };
        if (cps || strncmp((char*)stream+p, "encoding=", 9)) continue;
        p += 9;
        if (!cps && (stream[p] == '\"' || stream[p] == '\'')){
          p++;
          cps = p;
        }else
          break;
      };
      if (cps && cpe){
        DString dcp((char*)stream, cps, cpe-cps);
        encodingIdx = Encodings::getEncodingIndex(dcp.getChars());
        if (encodingIdx == -1)
          throw UnsupportedEncodingException(dcp);
        if (encodingIdx == Encodings::ENC_UTF8)
          type = ST_UTF8;
        else if (encodingIdx < 0) throw UnsupportedEncodingException(StringBuffer("encoding conflict - can't use ")+dcp);
      }else type = ST_UTF8;
    };

    if ((stream[0] == 0xFF && stream[1] == 0xFE && stream[2] == 0x00 && stream[3] == 0x00) ||
    (stream[0] == 0x3C && stream[1] == 0x00 && stream[2] == 0x00 && stream[3] == 0x00) ){
      type = ST_UTF32;
    }else if ((stream[0] == 0x00 && stream[1] == 0x00 && stream[2] == 0xFE && stream[3] == 0xFF) ||
    (stream[0] == 0x00 && stream[1] == 0x00 && stream[2] == 0x00 && stream[3] == 0x3C) ){
      type = ST_UTF32_BE;
    }else if ((stream[0] == 0xFF && stream[1] == 0xFE) ||
    (stream[0] == 0x3C && stream[1] == 0x00 && stream[2] == 0x3F && stream[3] == 0x00)){
      type = ST_UTF16;
    }else if ((stream[0] == 0xFE && stream[1] == 0xFF) ||
    (stream[0] == 0x00 && stream[1] == 0x3C && stream[2] == 0x00 && stream[3] == 0x3F)){
      type = ST_UTF16_BE;
    }else if (stream[0] == 0xEF && stream[1] == 0xBB && stream[2] == 0xBF){
      type = ST_UTF8;
    };
  };

  if (type == ST_UTF16 || type == ST_UTF16_BE){
    wstr = (wchar*)stream;
    len = size/2;
  }else if (type == ST_UTF32 || type == ST_UTF32_BE){
    w4str = (w4char*)stream;
    len = size/4;
  }else if (type == ST_UTF8){
    stream_wstr = new wchar[size];
    int pos;
    for(pos = 0, len = 0; pos < size; pos++, len++){
      wchar wc = 0;
      if (!(stream[pos] >> 7)){
        wc = stream[pos];
      }else if (stream[pos] >> 6 == 2){
        wc = '?'; // bad char in stream
      }else{
        int nextbytes = 0;
        while(stream[pos] << (nextbytes+1)  & 0x80) nextbytes++;
        wc = (stream[pos]  &  0xFF >> (nextbytes+2)) << nextbytes*6;
        while(nextbytes--){
          wc += (stream[++pos]  &  0x3F) << nextbytes*6;
        };
      };
      stream_wstr[len] = wc;
    };
  }else if(type == ST_CHAR && encodingIdx == -1){
    encodingIdx = Encodings::getDefaultEncodingIndex();
  };
}

DString::DString(const char *string, int s, int l, int encoding){
  type = ST_CHAR;
  str = string;
  start = s;
  len = l;
  if (s < 0 || len < -1) throw Exception(DString("bad string constructor parameters"));
  if (len == -1){
    len = 0;
    if (string != null) for(len = 0; str[len+s]; len++);
  };
  encodingIdx = encoding;
  if (encodingIdx == -1) encodingIdx = Encodings::getDefaultEncodingIndex();
}
DString::DString(const wchar *string, int s, int l){
  type = ST_UTF16;
  wstr = string;
  start = s;
  len = l;
  if (s < 0 || len < -1) throw Exception(DString("bad string constructor parameters"));
  if (len == -1)
    for(len = 0; wstr[len+s]; len++);
}
DString::DString(const w4char *string, int s, int l){
  type = ST_UTF32;
  w4str = string;
  start = s;
  len = l;
  if (s < 0 || len < -1) throw Exception(DString("bad string constructor parameters"));
  if (len == -1)
    for(len = 0; w4str[len+s]; len++);
}
DString::DString(const String *cstring, int s, int l){
  type = ST_CSTRING;
  cstr = cstring;
  start = s;
  len = l;
  if (s < 0 || s > cstring->length() || len < -1 || len > cstring->length() - start)
    throw Exception(DString("bad string constructor parameters"));
  if (len == -1)
    len = cstring->length() - start;
}
DString::DString(const String &cstring, int s, int l){
  type = ST_CSTRING;
  cstr = &cstring;
  start = s;
  len = l;
  if (s < 0 || s > cstring.length() || len < -1 || len > cstring.length() - start)
    throw Exception(DString("bad string constructor parameters"));
  if (len == -1)
    len = cstring.length() - start;
}
DString::DString(){
  type = ST_CHAR;
  len = 0;
  start = 0;
}
DString::~DString(){
  if (type == ST_UTF8) delete[] stream_wstr;
}

wchar DString::operator[](int i) const{
  if (start+i >= 0 && i < len) switch(type){
    case ST_CHAR:
      return Encodings::toWChar(encodingIdx, str[start+i]);
    case ST_UTF16:
      return wstr[start+i];
    case ST_UTF16_BE:
      return (wstr[start+i]>>8) | ((wstr[start+i]&0xFF) << 8);
    case ST_CSTRING:
      return (*cstr)[start+i];
    case ST_UTF8:
      return stream_wstr[start+i];
    case ST_UTF32:
      // check for 4byte character - if so, return REPLACEMENT CHARACTER
      if (w4str[start+i]>>16 != 0){
        return 0xFFFD;
      }
      return (wchar)w4str[start+i];
    case ST_UTF32_BE:
      // check for 4byte character - if so, return REPLACEMENT CHARACTER
      if (w4str[start+i]<<16 != 0){
        return 0xFFFD;
      }
      return (wchar)(((w4str[start+i]&0xFF)<<24) + ((w4str[start+i]&0xFF00)<<8) + ((w4str[start+i]&0xFF0000)>>8) + ((w4str[start+i]&0xFF000000)>>24));
  };
  throw StringIndexOutOfBoundsException(SString(i));
}
int DString::length() const{
  return len;
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
