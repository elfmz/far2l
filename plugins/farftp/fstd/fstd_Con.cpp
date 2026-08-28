#include <all_far.h>

#include "fstdlib.h"

int WINAPI FP_ConWidth(void)
{
	CONSOLE_SCREEN_BUFFER_INFO ci;
	return WINPORT(GetConsoleScreenBufferInfo)(0, &ci) ? ci.dwSize.X : 0;
}

int WINAPI FP_ConHeight(void)
{
	CONSOLE_SCREEN_BUFFER_INFO ci;
	return WINPORT(GetConsoleScreenBufferInfo)(0, &ci) ? ci.dwSize.Y : 0;
}
