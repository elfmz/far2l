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

WINPORT_DECL(GetKeyboardLayoutList, int, (int nBuff, HKL *lpList))
{
	if (nBuff<2) return 1;
	lpList[0] = HKL_DUMMY;
	return 1;
}

WINPORT_DECL(MapVirtualKey, UINT, (UINT uCode, UINT uMapType))
{
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

