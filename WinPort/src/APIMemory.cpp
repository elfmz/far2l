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

WINPORT_DECL(GlobalAlloc, HGLOBAL, ( UINT   uFlags, SIZE_T dwBytes))
{
	PVOID rv = malloc(dwBytes);
	if (rv && (uFlags&GMEM_ZEROINIT)==GMEM_ZEROINIT)
		memset(rv, 0, dwBytes);
	return rv;
}

WINPORT_DECL(GlobalFree, HGLOBAL, ( HGLOBAL hMem))
{
	free(hMem);
	return NULL;
}

WINPORT_DECL(GlobalLock, PVOID, ( HGLOBAL hMem))
{
	return hMem;
}

WINPORT_DECL(GlobalUnlock, BOOL, ( HGLOBAL hMem))
{
	return TRUE;
}

