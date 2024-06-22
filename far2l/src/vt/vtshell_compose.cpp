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

static std::atomic<bool> s_shown_tip_exit{false};
static std::atomic<unsigned int> s_vt_script_id{0};

VT_ComposeCommandExec::VT_ComposeCommandExec(const char *cd, const char *cmd, bool need_sudo, const std::string &start_marker)
{
	if (!need_sudo) {
		need_sudo = (chdir(cd) == -1 && (errno == EACCES || errno == EPERM));
	}
	const char *pwd_file_ext = need_sudo ? ".spwd" : ".pwd";

	unsigned int id = ++s_vt_script_id;
	const auto &name = StrPrintf("vtcmd/%x_%u", (unsigned int)getpid(), id);
	_cmd_script = InMyTemp(name.c_str());
	_pwd_file = _cmd_script + pwd_file_ext;
	Create(cd, cmd, need_sudo, start_marker);
	if (!_created) {
		Cleanup();
		_cmd_script = InMyCache(name.c_str());
		_pwd_file = _cmd_script + pwd_file_ext;
		Create(cd, cmd, need_sudo, start_marker);
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

void VT_ComposeCommandExec::Create(const char *cd, const char *cmd, bool need_sudo, const std::string &start_marker)
{
	std::string content;
	content+= "trap \"printf ''\" INT\n"; // need marker to be printed even after Ctrl+C pressed
	content+= "PS1=''; PS2=''; PS3=''; PS4=''; PROMPT_COMMAND=''\n"; // reduce risk of glitches
	if (strcmp(cmd, "exit")!=0) {
		content+= VT_ComposeInitialTitleCommand(cd, cmd, need_sudo);
	}
	content+= VT_ComposeMarkerCommand(start_marker);
	content+= '\n';
	if (strcmp(cmd, "exit")==0) {
		content+= StrPrintf(
			"echo \"%ls%ls%ls\"\n",  Msg::VTStop.CPtr(),
			s_shown_tip_exit ? L"" : L" ",
			s_shown_tip_exit ? L"" : Msg::VTStopTip.CPtr()
		);
		s_shown_tip_exit = true;
	}

	std::string pwd_suffix;
	const char *last_ch = cmd + strlen(cmd);
	while (last_ch != cmd && (*last_ch == ' ' || *last_ch == '\t' || *last_ch == 0)) {
		--last_ch;
	}

	if (*last_ch != '&') { // don't update curdir in case of background command
		pwd_suffix = StrPrintf(" && pwd >'%s'", _pwd_file.c_str());
	}

	if (need_sudo) {
		content+= Opt.SudoEnabled ? "sudo -A " : "sudo ";
		content+= StrPrintf("sh -c \"cd \\\"%s\\\" && %s%s\"\n",
			EscapeEscapes(EscapeCmdStr(cd)).c_str(), EscapeCmdStr(cmd).c_str(), pwd_suffix.c_str());
	} else {
		content+= StrPrintf("cd \"%s\" && %s%s\n",
			EscapeCmdStr(cd).c_str(), cmd, pwd_suffix.c_str());
	}

	content+= "FARVTRESULT=$?\n"; // it will be echoed to caller from outside

	static std::string vthook = InMyConfig("/vtcmd.sh");
	if (TestPath(vthook).Exists()) {
		content.append(vthook)
			.append((need_sudo && !StrStartsFrom(cmd, "sudo ")) ? " sudo " : " ")
				.append(cmd).append("\n");
	}

	content+= "cd ~\n"; // avoid locking arbitrary directory
	if (Opt.CmdLine.Splitter) {
		content+= "if [ $FARVTRESULT -eq 0 ]; then\n";
		content+= "printf '\\033_push-attr\\007\\033_set-blank=-\\007\\033[32m\\033[K\\033_pop-attr\\007\\012'\n";
		content+= "else\n";
		content+= "printf '\\033_push-attr\\007\\033_set-blank=~\\007\\033[33m\\033[K\\033_pop-attr\\007\\012'\n";
		content+= "fi\n";
	}
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
