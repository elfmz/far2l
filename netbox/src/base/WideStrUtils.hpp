#pragma once

#include <Classes.hpp>
#include <UnicodeString.hpp>

enum TEncodeType
{ 
  etUSASCII,
  etUTF8,
  etANSI,
};

TEncodeType DetectUTF8Encoding(const RawByteString & S);
