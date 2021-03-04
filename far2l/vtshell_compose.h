#pragma once
#include <string>
#include <ScopeHelpers.h>

void VT_ComposeMarker(std::string &marker);
std::string VT_ComposeMarkerCommand(const std::string &marker);
std::string VT_ComposeInitialTitle(const char *cd, const char *cmd, bool using_sudo);

struct VT_ComposeCommandScript
{
	VT_ComposeCommandScript(const char *cd, const char *cmd, bool need_sudo, const std::string &start_marker);
	~VT_ComposeCommandScript();

	inline bool Created() const { return _created; }
	inline const std::string &ScriptFile() const { return _cmd_script; }

	std::string GetResultOfPWD() const;

private:
	FDScope _fd;
	std::string _cmd_script;
	std::string _pwd_file;
	bool _created = false;

	void Create(const char *cd, const char *cmd, bool need_sudo, const std::string &start_marker);
	void Cleanup();
};
