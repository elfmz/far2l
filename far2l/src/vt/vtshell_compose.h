#pragma once
#include <string>
#include <ScopeHelpers.h>

void VT_ComposeMarker(std::string &marker);
std::string VT_ComposeMarkerCommand(const std::string &marker);

struct VT_ComposeCommandExec
{
	VT_ComposeCommandExec(const char *cd, const char *cmd, bool need_sudo, const std::string &start_marker);
	~VT_ComposeCommandExec();

	inline bool Created() const { return _created; }
	inline const std::string &ScriptFile() const { return _cmd_script; }

	std::string ResultedWorkingDirectory() const;

private:
	FDScope _fd;
	std::string _cmd_script;
	std::string _pwd_file;
	bool _created = false;

	void Create(const char *cd, const char *cmd, bool need_sudo, const std::string &start_marker);
	void Cleanup();
};
