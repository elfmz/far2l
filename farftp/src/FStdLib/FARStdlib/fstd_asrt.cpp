#include <all_far.h>
#pragma hdrstop

#include "fstdlib.h"

void _cdecl __WinAbort(LPCSTR msg,...)
{
	va_list a;

	if(!msg) exit(1);

//Message
	va_start(a,msg);
	vfprintf(stderr, msg,a);
	va_end(a);
	exit(1);
}
