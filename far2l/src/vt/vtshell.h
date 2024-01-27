#pragma once
#include <vector>
#include <string>

int VTShell_Execute(const char *cmd, bool need_sudo, bool may_bgnd, bool may_notify);
bool VTShell_Busy();
void VTShell_Shutdown();

size_t VTShell_Count();
void VTShell_Enum(std::vector<std::string> &vts);
int VTShell_Switch(size_t index);

