#pragma once

int VTShell_Execute(const char *cmd, bool need_sudo);
bool VTShell_Busy();
void VTShell_Shutdown();
