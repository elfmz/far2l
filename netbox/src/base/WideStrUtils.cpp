#include "WideStrUtils.hpp"

TEncodeType DetectUTF8Encoding(const RawByteString & S)
{
  const uint8_t *buf = reinterpret_cast<const uint8_t *>(S.c_str());
  intptr_t len = S.Length();
  const uint8_t * endbuf = buf + len;
  uint8_t byte2mask = 0x00;
  int trailing = 0;  // trailing (continuation) bytes to follow

  while (buf != endbuf)
  {
    uint8_t c = *buf++;
    if (trailing)
    {
      if ((c & 0xC0) == 0x80)  // Does trailing byte follow UTF-8 format?
      {
      if (byte2mask)        // Need to check 2nd byte for proper range?
      {
        if (c & byte2mask)     // Are appropriate bits set?
          byte2mask = 0x00;
        else
          return etANSI;
      }
        trailing--;
      }
      else
        return etANSI;
    }
    else
    {
      if ((c & 0x80) == 0x00)
        continue;      // valid 1 byte UTF-8
      else if ((c & 0xE0) == 0xC0)            // valid 2 byte UTF-8
      {
        if (c & 0x1E)                     // Is UTF-8 byte in
          // proper range?
          trailing = 1;
        else
          return etANSI;
      }
      else if ((c & 0xF0) == 0xE0)           // valid 3 byte UTF-8
      {
        if (!(c & 0x0F))                // Is UTF-8 byte in
          // proper range?
          byte2mask = 0x20;              // If not set mask
        // to check next byte
        trailing = 2;
      }
      else if ((c & 0xF8) == 0xF0)           // valid 4 byte UTF-8
      {
        if (!(c & 0x07))                // Is UTF-8 byte in
        {
          // proper range?

          byte2mask = 0x30;              // If not set mask
        }
        // to check next byte
        trailing = 3;
      }
      else if ((c & 0xFC) == 0xF8)           // valid 5 byte UTF-8
      {
        if (!(c & 0x03))                // Is UTF-8 byte in
          // proper range?
          byte2mask = 0x38;              // If not set mask
        // to check next byte
        trailing = 4;
      }
      else if ((c & 0xFE) == 0xFC)           // valid 6 byte UTF-8
      {
        if (!(c & 0x01))                // Is UTF-8 byte in
          // proper range?
          byte2mask = 0x3C;              // If not set mask
        // to check next byte
        trailing = 5;
      }
      else
        return etANSI;
    }
  }
  return trailing == 0 ? etUTF8 : etANSI;
}
