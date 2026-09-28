#include <all_far.h>

#include "Int.h"

#if 0
#undef Log
#undef PROC
#define Log(v)  INProc::Say v
#define PROC(v) INProc _inp v;
#endif

//---------------------------------------------------------------------------------
EnumHost::EnumHost(char *HostsPath)
{
	Log(("EnumHost::EnumHost", "%p %s", this, HostsPath));
	Assign(HostsPath);
}

EnumHost::~EnumHost()
{
	Log(("EnumHost::~EnumHost"));

	if (hEnum != NULL)
		WINPORT(RegCloseKey)(hEnum);
}

BOOL EnumHost::Assign(char *HostsPath)
{
	PROC(("EnumHost::Assign", "%s", HostsPath))
	HostPos = 0;
	StrCpy(RootKey, FTPHost::MkHost(NULL, HostsPath), ARRAYSIZE(RootKey));
	hEnum = FP_OpenRegKey(RootKey);
	Log(("rc = %d", hEnum != NULL));
	return hEnum != NULL;
}

BOOL EnumHost::GetNextHost(FTPHost *p)
{
	PROC(("EnumHost::GetNextHost", NULL))
	WCHAR SubKey[FAR_MAX_REG] = {0};
	DWORD Size = ARRAYSIZE(SubKey) - 1;
	FILETIME lw;

	if (!hEnum)
		return FALSE;

	if (WINPORT(RegEnumKeyEx)(hEnum, HostPos, SubKey, &Size, NULL, NULL, NULL, &lw) != ERROR_SUCCESS) {
		Log(("!enum keys"));
		return FALSE;
	}
	p->Init();
	strncpy(p->RegKey, p->MkHost(RootKey, Wide2MB(SubKey).c_str()), sizeof(p->RegKey));
	p->LastWrite = lw;
	HostPos++;
	Log(("SetKey %p to: %s", p, p->RegKey));
	return TRUE;
}
