#pragma once
#include <stdint.h>

#if defined(_MSC_VER)
#pragma pack(push, 2)
#endif

struct TFarPluginStrings
{
  uint16_t Id;
  uint16_t FarPluginStringId;
};

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

extern const TFarPluginStrings FarPluginStrings[];
