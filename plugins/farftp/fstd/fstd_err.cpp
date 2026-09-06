#include <all_far.h>

#include "fstdlib.h"

const char *_cdecl __WINError(void)
{
	return "Unknown error";
#if 0
	static char *WinEBuff = NULL;
	DWORD err = WINPORT(GetLastError)();

	if(WinEBuff) LocalFree(WinEBuff);

	if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &WinEBuff,0,NULL) == 0)
		return "Unknown error";

	char *m;

	if((m=strchr(WinEBuff,'\n')) != 0) *m = 0;

	if((m=strchr(WinEBuff,'\r')) != 0) *m = 0;

	CharToOem(WinEBuff, WinEBuff);
	return WinEBuff;
#endif
}
