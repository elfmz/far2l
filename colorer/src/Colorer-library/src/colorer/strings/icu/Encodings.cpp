#include "colorer/strings/icu/Encodings.h"
#include <cstring>
#include "colorer/Exception.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"

uUnicodeString Encodings::toUnicodeString(char* data, int32_t len)
{
  const char* encoding;

  // try to detect encoding
  UErrorCode status {U_ZERO_ERROR};
  int32_t signatureLength;
  encoding = ucnv_detectUnicodeSignature(data, len, &signatureLength, &status);
  if (U_FAILURE(status)) {
    COLORER_LOG_ERROR("Encodings: Error \"%\" from ucnv_detectUnicodeSignature()\n", u_errorName(status));
    throw Exception("Error from ucnv_detectUnicodeSignature");
  }
  if (encoding == nullptr) {
    encoding = ENC_UTF8;
  }

  return std::make_unique<UnicodeString>(data + signatureLength, len - signatureLength, encoding);
}

uUnicodeString Encodings::fromUTF8(char* data, int32_t len)
{
  return std::make_unique<UnicodeString>(data, len, ENC_UTF8);
}

uUnicodeString Encodings::fromUTF8(unsigned char* data)
{
  const auto c = reinterpret_cast<char*>(data);
  return fromUTF8(c,(int32_t)strlen(c));
}

int Encodings::toUTF8Bytes(UChar wc, byte* dest)
{
  int32_t len = 0;
  UErrorCode err = U_ZERO_ERROR;
  u_strToUTF8((char*) dest, 8, &len, &wc, 1, &err);
  return len;
}