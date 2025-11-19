#include <set>
#include <fstream>

#include <string>
#include <locale>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"

void *g_dummy_hkl[2] = {0};
#define HKL_DUMMY ((HKL)(&g_dummy_hkl[0]));

static const uint8_t vk_to_vsc[] =
{
   0,   0,   0, 105,   0,   0,   0,   0,  14,  15,   0,   0,  76,  28,   0,   0,
  42,  29,  56,  69,  58, 103,   0,   0,   0, 104,   0,   1, 121, 123,   0,   0,
  57,  73,  81,  79,  71,  75,  72,  77,  80,   0,   0,   0,  55,  82,  83,  56,
  11,   2,   3,   4,   5,   6,   7,   8,   9,  10,   0,   0,   0,   0,   0,   0,
   0,  30,  48,  46,  32,  18,  33,  34,  35,  23,  36,  37,  38,  50,  49,  24,
  25,  16,  19,  31,  20,  22,  47,  17,  45,  21,  44,  91,  92,  93,   0,   0,
  82,  79,  80,  81,  75,  76,  77,  71,  72,  73,  55,  78,   0,  74,  83,  53,
  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  87,  88,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  69,  70, 101,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  42,  54,  29,  29,  56,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  39,  13,  51,  12,  52,  53,
  41,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  26,  43,  27,  40,  96,
  97,  98,  86,  99, 100,   0,   0,   0,   0, 102,   0, 106, 107, 108, 109,   0,
   0,   0, 112, 110,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

WINPORT_DECL(GetKeyboardLayoutList, int, (int nBuff, HKL *lpList))
{
	if (nBuff<2) return 1;
	lpList[0] = HKL_DUMMY;
	return 1;
}

WINPORT_DECL(MapVirtualKey, UINT, (UINT uCode, UINT uMapType))
{
	if (uMapType == MAPVK_VK_TO_VSC && uCode < ARRAYSIZE(vk_to_vsc)) {
		return vk_to_vsc[uCode];
	}

	fprintf(stderr, "MapVirtualKey(0x%x, 0x%x)\n", uCode, uMapType);
	return 0;
}

WINPORT_DECL(VkKeyScan, SHORT, (WCHAR ch))
{
	fprintf(stderr, "VkKeyScan(0x%x)\n", ch);
	return 0;
}

WINPORT_DECL(ToUnicodeEx, int, (UINT wVirtKey, UINT wScanCode, CONST BYTE *lpKeyState,
		LPWSTR pwszBuff, int cchBuff, UINT wFlags, HKL dwhkl))
{
	//fprintf(stderr, "ToUnicodeEx(0x%x)\n", wVirtKey);
	return 0;
}

