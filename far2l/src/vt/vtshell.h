#pragma once
#include <vector>
#include <string>

int VTShell_Execute(const char *cmd, bool need_sudo, bool may_bgnd, bool may_notify);
bool VTShell_Busy();
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

