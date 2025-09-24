#pragma once
#include <vector>
#include <string>

enum VT_State
{
	VTS_IDLE = 0,
	VTS_NORMAL_SCREEN,
	VTS_ALTERNATE_SCREEN
};

VT_State VTShell_State();

int VTShell_Execute(const char *cmd, bool need_sudo, bool may_bgnd, bool may_notify);
void VTShell_Shutdown();

struct VTInfo
{
	std::string title;
	HANDLE con_hnd;
	bool done;
	int exit_code;
};

typedef std::vector<VTInfo> VTInfos;

size_t VTShell_Count();
void VTShell_Enum(VTInfos &vts);
int VTShell_Switch(size_t index);

const char *GetSystemShell();
