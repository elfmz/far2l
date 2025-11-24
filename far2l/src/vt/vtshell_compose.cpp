#include "headers.hpp"
#include "lang.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "vtshell_compose.h"
#include <utils.h>
#include <RandomString.h>
#include <TestPath.h>
#include <errno.h>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>

void VT_ComposeMarker(std::string &marker)
{
	marker.clear();
	RandomStringAppend(marker, 8, 16, RNDF_ALNUM);
}

std::string VT_ComposeMarkerCommand(const std::string &marker)
{
	// marker contains $FARVTRESULT and thus must be in double quotes
	std::string out = "printf '\\033_far2l_%s\\007' \"";
	out+= marker;
	out+= "\"";
	return out;
}

static std::string VT_ComposeInitialTitleCommand(const char *cd, const char *cmd, bool using_sudo)
{
	std::string title = cmd;
	StrTrim(title);

	if (StrStartsFrom(title, "sudo ")) {
		using_sudo = true;
		title.erase(0, 5);
		StrTrim(title);
	}

	if (title.size() > 2 && (title[0] == '\'' || title[0] == '\"')) {
		size_t p = title.find(title[0], 1);
		if (p != std::string::npos) {
			title = title.substr(1, p - 1);
		}

	} else {
		size_t p = title.find(' ');
		if (p != std::string::npos) {
			title.resize(p);
		}
	}

	size_t p = title.rfind('/');
	if (p != std::string::npos) {
		title.erase(0, p + 1);
	}

	title+= '@';
	title+= cd;

	if (using_sudo) {
		title.insert(0, "sudo ");
	}

	std::string out = "printf '\\033]2;";

	for (auto &ch : title) {
		if ((ch >= 0 && ch < 0x20)) {
			out+= '\x01';

		} else if (ch == '\'') {
			out+= "'\\''";

		} else {
			out+= ch;
		}
	}
	out+= "\\007'\n";

	return out;
}

///////////////////////////////////////////////////////////////////////////////////////

static std::atomic<unsigned int> s_vt_script_id{0};

VT_ComposeCommandExec::VT_ComposeCommandExec(const IVTShellBackend &backend, const char *cd, const char *cmd, bool need_sudo,
	const std::string &start_marker, const std::string &exit_marker, const std::string &user_profile)
{
	if (!need_sudo) {
		need_sudo = (chdir(cd) == -1 && (errno == EACCES || errno == EPERM));
	}
	const char *pwd_file_ext = need_sudo ? ".spwd" : ".pwd";

	unsigned int id = ++s_vt_script_id;
	const auto &name = StrPrintf("vtcmd/%x_%u", (unsigned int)getpid(), id);
	_cmd_script = InMyTemp(name.c_str());
	_pwd_file = _cmd_script + pwd_file_ext;
	Create(backend, cd, cmd, need_sudo, start_marker, exit_marker, user_profile);
	if (!_created) {
		Cleanup();
		_cmd_script = InMyCache(name.c_str());
		_pwd_file = _cmd_script + pwd_file_ext;
		Create(backend, cd, cmd, need_sudo, start_marker, exit_marker, user_profile);
	}
}

VT_ComposeCommandExec::~VT_ComposeCommandExec()
{
	Cleanup();
}

std::string VT_ComposeCommandExec::ResultedWorkingDirectory() const
{
	if (!_created)
		return std::string();

	FDScope fd(open(_pwd_file.c_str(), O_RDONLY));
	if (!fd.Valid()) {
		fprintf(stderr, "%s: error %u opening %s\n",
			__FUNCTION__, errno, _pwd_file.c_str());
		return std::string();
	}

	char buf[PATH_MAX + 1] = {};
	ReadAll(fd, buf, sizeof(buf) - 1);
	size_t len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\n') {
		buf[--len] = 0;
	}

	return buf;
}

void VT_ComposeCommandExec::Create(const IVTShellBackend &backend, const char *cd, const char *cmd, bool need_sudo,
	const std::string &start_marker, const std::string &exit_marker, const std::string &user_profile)
{
	std::string content = backend.MakeScript(cd, cmd, need_sudo, start_marker, _pwd_file, exit_marker, user_profile);

	unlink(_pwd_file.c_str());
	_fd = open(_cmd_script.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (_fd.Valid()) {
		if (WriteAll(_fd, content.c_str(), content.size()) == content.size()) {
			_created = true;
		}
	}
}

void VT_ComposeCommandExec::Cleanup()
{
	_fd.CheckedClose();
	if (!_cmd_script.empty()) {
		unlink(_cmd_script.c_str());
	}

	if (!_pwd_file.empty()) {
		unlink(_pwd_file.c_str());
	}
}
