#include <string.h>
#include <colorer/unicode/CString.h>
#include <colorer/unicode/SString.h>
#include <colorer/unicode/Encodings.h>

StringIndexOutOfBoundsException::StringIndexOutOfBoundsException() noexcept:
  Exception("[StringIndexOutOfBoundsException] ")
{}

StringIndexOutOfBoundsException::StringIndexOutOfBoundsException(const String &msg) noexcept : StringIndexOutOfBoundsException()
{
  what_str.append(msg);
}

CString &CString::operator=(const CString &cstring)
{
  if (type == ST_UTF8) delete[] stream_wstr;
  type = cstring.type;
  encodingIdx = cstring.encodingIdx;
  str = cstring.str;
  start = cstring.start;
  len = cstring.len;
  if (type == ST_UTF8) {
    stream_wstr = new wchar[len];
    for (size_t wi = 0; wi < len; wi++)
      stream_wstr[wi] = cstring.stream_wstr[wi];
  }
  return *this;
}

CString::CString(const byte* stream, size_t size, int def_encoding)
{
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

  if (type == ST_CHAR && encodingIdx == -1) {
    // check encoding parameter
    if (stream[0] == 0x3C && stream[1] == 0x3F) {
      size_t p;
      size_t cps = 0, cpe = 0;
      for (p = 2; stream[p] != 0x3F && stream[p + 1] != 0x3C && p < 100; p++) {
        if (cps && stream[p] == stream[cps - 1]) {
          cpe = p;
          break;
        }
        if (cps || strncmp((char*)stream + p, "encoding=", 9) != 0) continue;
        p += 9;
        if (!cps && (stream[p] == '\"' || stream[p] == '\'')) {
          p++;
          cps = p;
        } else
          break;
      }
      if (cps && cpe) {
        CString dcp((char*)stream, cps, cpe - cps);
        encodingIdx = Encodings::getEncodingIndex(dcp.getChars());
        if (encodingIdx == -1)
          throw UnsupportedEncodingException(dcp);
        if (encodingIdx == Encodings::ENC_UTF8)
          type = ST_UTF8;
        else if (encodingIdx < 0) throw UnsupportedEncodingException(SString("encoding conflict - can't use ") + dcp);
      } else type = ST_UTF8;
    }

    if ((stream[0] == 0xFF && stream[1] == 0xFE && stream[2] == 0x00 && stream[3] == 0x00) ||
        (stream[0] == 0x3C && stream[1] == 0x00 && stream[2] == 0x00 && stream[3] == 0x00)) {
      type = ST_UTF32;
    } else if ((stream[0] == 0x00 && stream[1] == 0x00 && stream[2] == 0xFE && stream[3] == 0xFF) ||
               (stream[0] == 0x00 && stream[1] == 0x00 && stream[2] == 0x00 && stream[3] == 0x3C)) {
      type = ST_UTF32_BE;
    } else if ((stream[0] == 0xFF && stream[1] == 0xFE) ||
               (stream[0] == 0x3C && stream[1] == 0x00 && stream[2] == 0x3F && stream[3] == 0x00)) {
      type = ST_UTF16;
    } else if ((stream[0] == 0xFE && stream[1] == 0xFF) ||
               (stream[0] == 0x00 && stream[1] == 0x3C && stream[2] == 0x00 && stream[3] == 0x3F)) {
      type = ST_UTF16_BE;
    } else if (stream[0] == 0xEF && stream[1] == 0xBB && stream[2] == 0xBF) {
      type = ST_UTF8;
    }
  }

  if (type == ST_UTF16 || type == ST_UTF16_BE) {
    w2str = (w2char*)stream;
    len = size / 2;
  } else if (type == ST_UTF32 || type == ST_UTF32_BE) {
    w4str = (w4char*)stream;
    len = size / 4;
  } else if (type == ST_UTF8) {
    stream_wstr = new wchar[size];
    size_t pos;
    for (pos = 0, len = 0; pos < size; pos++, len++) {
      wchar wc = 0;
      if (!(stream[pos] >> 7)) {
        wc = stream[pos];
      } else if (stream[pos] >> 6 == 2) {
        wc = '?'; // bad char in stream
      } else {
        int nextbytes = 0;
        while (stream[pos] << (nextbytes + 1)  & 0x80) nextbytes++;
        wc = (stream[pos]  &  0xFF >> (nextbytes + 2)) << (nextbytes * 6);
        while (nextbytes--) {
          wc += (stream[++pos]  &  0x3F) << (nextbytes * 6);
        }
      }
      stream_wstr[len] = wc;
    }
  } else if (type == ST_CHAR && encodingIdx == -1) {
    encodingIdx = Encodings::getDefaultEncodingIndex();
  }
}

CString::CString(const char* string, size_t s, size_t l, int encoding)
{
  type = ST_CHAR;
  str = string;
  start = s;
  len = l;
  if (len == npos) {
    len = 0;
    if (string != nullptr) for (len = 0; str[len + s]; len++);
  }
  encodingIdx = encoding;
  if (encodingIdx == -1) encodingIdx = Encodings::getDefaultEncodingIndex();
}

CString::CString(const w2char* string, size_t s, size_t l)
{
  type = ST_UTF16;
  w2str = string;
  start = s;
  len = l;
  encodingIdx = -1;
  if (len == npos)
    for (len = 0; w2str[len + s]; len++);
}

CString::CString(const w4char* string, size_t s, size_t l)
{
  type = ST_UTF32;
  w4str = string;
  start = s;
  len = l;
  encodingIdx = -1;
  if (len == npos)
    for (len = 0; w4str[len + s]; len++);
}

CString::CString(const wchar* string, size_t s, size_t l)
{
#if (__WCHAR_MAX__ > 0xffff)
  type = ST_UTF32;
  w4str = (const w4char*)string;
  if (len == npos)
    for (len = 0; w4str[len + s]; len++);
#else
  type = ST_UTF16;
  w2str = (const w2char*)string;
  if (len == npos)
    for (len = 0; w2str[len + s]; len++);
#endif

  start = s;
  len = l;
  encodingIdx = -1;
}

CString::CString(const String* cstring, size_t s, size_t l)
{
  type = ST_CSTRING;
  cstr = cstring;
  start = s;
  len = l;
  encodingIdx = -1;
  if (s > cstring->length() || (len != npos && len > cstring->length() - start))
    throw Exception(CString("bad string constructor parameters"));
  if (len == npos)
    len = cstring->length() - start;
}

CString::CString(const String &cstring, size_t s, size_t l)
{
  type = ST_CSTRING;
  cstr = &cstring;
  start = s;
  len = l;
  encodingIdx = -1;
  if (s > cstring.length() || (len != npos && len > cstring.length() - start))
    throw Exception(CString("bad string constructor parameters"));
  if (len == npos)
    len = cstring.length() - start;
}

CString::CString()
{
  type = ST_CHAR;
  len = 0;
  start = 0;
  encodingIdx = -1;
}

CString::~CString()
{
  if (type == ST_UTF8) delete[] stream_wstr;
}

wchar CString::operator[](size_t i) const
{
  if (i < len) switch (type) {
      case ST_CHAR:
        return Encodings::toWChar(encodingIdx, str[start + i]);
      case ST_UTF16:
        return w2str[start + i];
      case ST_UTF16_BE:
        return (w2str[start + i] >> 8) | ((w2str[start + i] & 0xFF) << 8);
      case ST_CSTRING:
        return (*cstr)[start + i];
      case ST_UTF8:
        return stream_wstr[start + i];
      case ST_UTF32:
        // check for 4byte character - if so, return REPLACEMENT CHARACTER
        if (w4str[start + i] >> 16 != 0) {
          return 0xFFFD;
        }
        return (wchar)w4str[start + i];
      case ST_UTF32_BE:
        // check for 4byte character - if so, return REPLACEMENT CHARACTER
        if (w4str[start + i] << 16 != 0) {
          return 0xFFFD;
        }
        return (wchar)(((w4str[start + i] & 0xFF) << 24) + ((w4str[start + i] & 0xFF00) << 8) + ((w4str[start + i] & 0xFF0000) >> 8) + ((w4str[start + i] & 0xFF000000) >> 24));
    }
  throw StringIndexOutOfBoundsException(SString(i));
}

size_t CString::length() const
{
  return len;
}



