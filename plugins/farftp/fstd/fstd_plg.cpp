#include <all_far.h>

#include "fstdlib.h"

PluginStartupInfo *FP_Info = NULL;
FarStandardFunctions *FP_FSF = NULL;
char *FP_PluginRootKey = NULL;
char *FP_PluginStartPath = NULL;
int FP_LastOpMode = 0;
DWORD FP_WinVerDW;

static void _cdecl idAtExit(void)
{
	delete FP_Info;
	FP_Info = NULL;
	delete FP_FSF;
	FP_FSF = NULL;
	delete[] FP_PluginRootKey;
	FP_PluginRootKey = NULL;
	free(FP_PluginStartPath);
	FP_PluginStartPath = NULL;
	//	FP_HModule = NULL;
}

void WINAPI FP_SetStartupInfo(const PluginStartupInfo *Info, const char *KeyName)
{
	// Info
	FP_Info = new PluginStartupInfo;
	memcpy(FP_Info, Info, sizeof(*Info));
	// FSF
	FP_FSF = new FarStandardFunctions;
	memcpy(FP_FSF, Info->FSF, sizeof(*FP_FSF));
	// Version
	// Plugin Reg key
	FP_PluginRootKey = new char[FAR_MAX_REG + 1];
	StrCpy(FP_PluginRootKey, Info->RootKey, FAR_MAX_REG);
	StrCat(FP_PluginRootKey, "/", FAR_MAX_REG);
	StrCat(FP_PluginRootKey, KeyName, FAR_MAX_REG);
	// Start path
	char *m = strrchr(FP_PluginStartPath, '/');

	if (m)
		*m = 0;
}

SHAREDSYMBOL void PluginModuleOpen(const char *path)
{
	if (FP_PluginStartPath)
		free(FP_PluginStartPath);
	FP_PluginStartPath = strdup(path);
}
