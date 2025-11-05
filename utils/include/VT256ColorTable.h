#pragma once
#include <stdint.h>
/*
 * First 16 colors of 256-color table are mapped to base system 16-color palette.
 * So actual table defines 240 elements.
 */

#define VT_256COLOR_TABLE_COUNT (256 - 16)

extern const uint32_t g_VT256ColorTable[VT_256COLOR_TABLE_COUNT];

#define COMPOSE_RGB(R, G, B)           ((uint32_t(R)) | ((uint32_t(G)) << 8) | ((uint32_t(B)) << 16))

