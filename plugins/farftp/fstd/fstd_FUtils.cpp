#include <all_far.h>

#include "fstdlib.h"

//------------------------------------------------------------------------
SaveConsoleTitle::SaveConsoleTitle(BOOL rest /*= TRUE*/)
{
	Usage = rest;
	NeedToRestore = rest;
	GET_TIME(LastChange);
	WINPORT(GetConsoleTitle)(NULL, SaveTitle, ARRAYSIZE(SaveTitle));
	Log(("TITLE: Save"));
}
SaveConsoleTitle::~SaveConsoleTitle()
{
	Restore();
}

void SaveConsoleTitle::Set(LPCSTR buff)
{
	GET_TIME(LastChange);
	Text(buff);
	NeedToRestore = TRUE;
}

void SaveConsoleTitle::Text(LPCSTR buff)
{

	Log(("TITLE: Set [%s]", buff));
	/*
		char _buff[ 1024 ];
		if(FP_WinVer->dwPlatformId != VER_PLATFORM_WIN32_NT)
		{
			OemToCharBuff(buff, _buff, sizeof(_buff));
			buff = _buff;
		}
	*/
	WINPORT(SetConsoleTitle)(NULL, MB2Wide(buff).c_str());
}

void SaveConsoleTitle::Restore(void) {}

void SaveConsoleTitle::Using(void)
{
	Usage++;
	Log(("TITLE: Use[%d]", Usage));
}

double SaveConsoleTitle::Changed(void)
{
	DWORD t;
	GET_TIME(t);
	return CMP_TIME(t, LastChange);
}

//------------------------------------------------------------------------
int WINAPI CheckForKeyPressed(WORD *Codes, int NumCodes)
{
	return WINPORT(CheckForKeyPress)(NULL, Codes, NumCodes, CFKP_KEEP_OTHER_EVENTS);
}
//------------------------------------------------------------------------
int WINAPI FP_Color(int tp)
{
	return (int)FP_Info->AdvControl(FP_Info->ModuleNumber, ACTL_GETCOLOR, (void *)(INT_PTR)tp);
}
