#ifndef COLORER_ENCODINGS_H
#define COLORER_ENCODINGS_H

#include "colorer/Common.h"

/** Encodings information.
    - All codepage definitions are read from list of ANSI -> Unicode
      associative files. These codepage files could be taken from
      www.unicode.org too.
    - Two-stage tables are used to access Unicode -> ANSI transformation.
    @ingroup unicode
*/
class Encodings
{
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
  static int toUTF8Bytes(wchar, byte*);

  static char toChar(int, wchar);
  static wchar toWChar(int, char);

  static int getEncodingNamesNum();
  static int getDefaultEncodingIndex();
  static const char* getDefaultEncodingName();

  static int getEncodingIndex(const char* enc);
  static const char* getEncodingName(int enc);

  static uUnicodeString toUnicodeString(char* data, int32_t len);
  static uUnicodeString fromUTF8(char* data, int32_t len);
};

#endif
