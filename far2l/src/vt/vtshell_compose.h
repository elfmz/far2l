#pragma once
#include <string>
#include <ScopeHelpers.h>
#include "VTShellBackend.h"

void VT_ComposeMarker(std::string &marker);
std::string VT_ComposeMarkerCommand(const std::string &marker);

struct VT_ComposeCommandExec
{
	VT_ComposeCommandExec(const IVTShellBackend &backend, const char *cd, const char *cmd, bool need_sudo,
		const std::string &start_marker, const std::string &exit_marker, const std::string &user_profile);
	~VT_ComposeCommandExec();

	inline bool Created() const { return _created; }
	inline const std::string &ScriptFile() const { return _cmd_script; }

	std::string ResultedWorkingDirectory() const;

private:
	FDScope _fd;
	std::string _cmd_script;
	std::string _pwd_file;
	bool _created = false;

	void Create(const IVTShellBackend &backend, const char *cd, const char *cmd, bool need_sudo,
		const std::string &start_marker, const std::string &exit_marker, const std::string &user_profile);
	void Cleanup();
};
