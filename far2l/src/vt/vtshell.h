#pragma once
#include <vector>
#include <string>

int VTShell_Execute(const char *cmd, bool need_sudo, bool may_bgnd, bool may_notify);
void VTShell_Shutdown();

bool VTShell_Busy();

enum VTState
{
	VT_INVALID = 0,
	VT_EXITED,
	VT_NORMAL_SCREEN,
	VT_ALTERNATE_SCREEN
};

struct VTInfo
{
	std::string title;
	HANDLE con_hnd;
	bool exited;
	int exit_code;
};

typedef std::vector<VTInfo> VTInfos;

size_t VTShell_Count();
void VTShell_Enum(VTInfos &vts);
VTState VTShell_LookupState(HANDLE hConsole);
int VTShell_Switch(size_t index);

const char *GetSystemShell();
