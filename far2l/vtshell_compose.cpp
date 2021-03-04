#include "headers.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "vtshell_compose.h"
#include <utils.h>
#include <errno.h>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>

void VT_ComposeMarker(std::string &marker)
{
	marker.clear();

	for (size_t l = 8 + (rand() % 9); l; --l) {
		switch (rand() % 3) {
			case 0:
				marker+= 'A' + (rand() % ('Z' + 1 - 'A'));
				break;
			case 1:
				marker+= 'a' + (rand() % ('z' + 1 - 'a'));
				break;
			case 2:
				marker+= '0' + (rand() % ('9' + 1 - '0'));
				break;
		}
	}
}

std::string VT_ComposeMarkerCommand(const std::string &marker)
{
	// marker contains $FARVTRESULT and thus must be in double quotes
	std::string out = "echo -ne $'\\x1b'\"_far2l_";
	out+= marker;
	out+= "\"$'\\x07'";
	return out;
}

std::string VT_ComposeInitialTitle(const char *cd, const char *cmd, bool using_sudo)
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

	return title;
}
	
///////////////////////////////////////////////////////////////////////////////////////

static std::atomic<bool> s_shown_tip_init;
static std::atomic<bool> s_shown_tip_exit;
static std::atomic<unsigned int> s_vt_script_id;

VT_ComposeCommandScript::VT_ComposeCommandScript(const char *cd, const char *cmd, bool need_sudo, const std::string &start_marker)
{
	if (!need_sudo) {
		need_sudo = (chdir(cd) == -1 && (errno == EACCES || errno == EPERM));
	}

	unsigned int id = ++s_vt_script_id;
	char name[128]; 
	sprintf(name, "vtcmd/%x_%u", getpid(), id);
	_cmd_script = InMyTemp(name);
	_pwd_file = _cmd_script + ".pwd";
	Create(cd, cmd, need_sudo, start_marker);
	if (!_created) {
		Cleanup();
		_cmd_script = InMyCache(name);
		_pwd_file = _cmd_script + ".pwd";
		Create(cd, cmd, need_sudo, start_marker);
	}
}

VT_ComposeCommandScript::~VT_ComposeCommandScript()
{
	Cleanup();
}

std::string VT_ComposeCommandScript::GetResultOfPWD() const
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

void VT_ComposeCommandScript::Create(const char *cd, const char *cmd, bool need_sudo, const std::string &start_marker)
{
	std::string content;
	content+= "trap \"echo ''\" SIGINT\n"; // need marker to be printed even after Ctrl+C pressed
	content+= "PS1=''\n"; // reduce risk of glitches
	content+= VT_ComposeMarkerCommand(start_marker);
	content+= '\n';
	if (strcmp(cmd, "exit")==0) {
		content+= StrPrintf(
			"echo \"%ls%ls%ls\"\n",  MSG(MVTStop),
			s_shown_tip_exit ? L"" : L" ",
			s_shown_tip_exit ? L"" : MSG(MVTStopTip)
		);
		s_shown_tip_exit = true;

	} else if (!s_shown_tip_init && !Opt.OnlyEditorViewerUsed) {
		content+= StrPrintf("echo -ne \"\x1b_push-attr\x07\x1b[36m\"\n");
		content+= StrPrintf("echo \"%ls\"\n", MSG(MVTStartTipNoCmdTitle));
		content+= StrPrintf("echo \" %ls\"\n", MSG(MVTStartTipNoCmdShiftTAB));
		content+= StrPrintf("echo \" %ls\"\n", MSG(MVTStartTipNoCmdFn));
		content+= StrPrintf("echo \" %ls\"\n", MSG(MVTStartTipNoCmdMouse));
		content+= StrPrintf("echo \"%ls\"\n", MSG(MVTStartTipPendCmdTitle));
		content+= StrPrintf("echo \" %ls\"\n", MSG(MVTStartTipPendCmdFn));
		content+= StrPrintf("echo \" %ls\"\n", MSG(MVTStartTipPendCmdCtrlAltC));
		if (WINPORT(ConsoleBackgroundMode)(FALSE)) {
			content+= StrPrintf("echo \" %ls\"\n", MSG(MVTStartTipPendCmdCtrlAltZ));
		}
		content+= StrPrintf(" echo \" %ls\"\n", MSG(MVTStartTipPendCmdMouse));
		content+= "echo ════════════════════════════════════════════════════════════════════\x1b_pop-attr\x07\n";
		s_shown_tip_init = true;

	}
	if (need_sudo) {
		content+= StrPrintf("sudo sh -c \"cd \\\"%s\\\" && %s && pwd >'%s'\"\n",
			EscapeEscapes(EscapeCmdStr(cd)).c_str(), EscapeCmdStr(cmd).c_str(), _pwd_file.c_str());
	} else {
		content+= StrPrintf("cd \"%s\" && %s && pwd >'%s'\n",
			EscapeCmdStr(cd).c_str(), cmd, _pwd_file.c_str());
	}

	content+= "FARVTRESULT=$?\n"; // it will be echoed to caller from outside
	content+= "cd ~\n"; // avoid locking arbitrary directory
	content+= "if [ $FARVTRESULT -eq 0 ]; then\n";
	content+= "echo \"\x1b_push-attr\x07\x1b_set-blank=-\x07\x1b[32m\x1b[K\x1b_pop-attr\x07\"\n";
	content+= "else\n";
	content+= "echo \"\x1b_push-attr\x07\x1b_set-blank=~\x07\x1b[33m\x1b[K\x1b_pop-attr\x07\"\n";
	content+= "fi\n";
	unlink(_pwd_file.c_str());
	_fd = open(_cmd_script.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (_fd.Valid()) {
		if (WriteAll(_fd, content.c_str(), content.size()) == content.size()) {
			_created = true;
		}
	}
}

void VT_ComposeCommandScript::Cleanup()
{
	_fd.CheckedClose();
	if (!_cmd_script.empty()) {
		unlink(_cmd_script.c_str());
	}

	if (!_pwd_file.empty()) {
		unlink(_pwd_file.c_str());
	}
}
