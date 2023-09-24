#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <utils.h>
#include <MakePTYAndFork.h>
#include <RandomString.h>
#include "vtcompletor.h"

static const ssize_t VeryLongTerminalLine = 0x1000;

static const char *vtc_inputrc = "set completion-query-items 0\n"
	"set completion-display-width 0\n"
	"set colored-stats off\n"
	"set colored-completion-prefix off\n"
	"set page-completions off\n"
	"set colored-stats off\n"
	"set colored-completion-prefix off\n";

std::string VTSanitizeHistcontrol();

VTCompletor::VTCompletor()
	: _vtc_inputrc(InMyTemp("vtc_inputrc")),
	_pipe_stdin(-1), _pipe_stdout(-1), _pid(-1), _pty_used(false)
{
	int fd = open(_vtc_inputrc.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0622);
	if (fd!=-1) {
		if (write(fd, vtc_inputrc, strlen(vtc_inputrc))<=0) {
			perror("VTCompletor: write vtc_inputrc");
			_vtc_inputrc.clear();
		}
		close(fd);
	} else {
		perror("VTCompletor: open vtc_inputrc");
		_vtc_inputrc.clear();
	}
}

VTCompletor::~VTCompletor()
{
	Stop();
}

bool VTCompletor::EnsureStarted()
{
	if (_pid != -1)
		return true;

	const std::string &hc_override = VTSanitizeHistcontrol();

	int pty_master = -1;
	_pid = MakePTYAndFork(pty_master);
	if (_pid != -1) {
		_pty_used = true;
		if (_pid == 0) {
			// child process
			// terminal with one very long line...
			struct winsize ws = {1, VeryLongTerminalLine, 0, 0};
			if (ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws) == -1) {
				perror("VTCompletor: ioctl(TIOCSWINSZ)");
				_exit(1);
				exit(1);
			}
			execlp("bash", "bash", "--noprofile", "-i", NULL);
			perror("VTCompletor: execlp");
			_exit(6);
			exit(6);
		}
		// parent process
		_pipe_stdin = pty_master;
		_pipe_stdout = dup(pty_master);
		MakeFDCloexec(_pipe_stdout);
	} else {
		CheckedCloseFD(pty_master);
	}

	if (!_pty_used) {
		fprintf(stderr, "VTCompletor: fallback to pipes\n");
		int pipe_in[2] = {}, pipe_out[2] = {};

		if (pipe(pipe_in)<0) {
			perror("VTCompletor: pipe_in");
			return false;
		}
		if (pipe(pipe_out)<0) {
			perror("VTCompletor: pipe_out");
			CheckedCloseFDPair(pipe_in);
			return false;
		};

		_pid = fork();
		if (_pid==-1) {
			CheckedCloseFDPair(pipe_in);
			CheckedCloseFDPair(pipe_out);
			return false;
		}

		MakeFDCloexec(pipe_in[0]);
		MakeFDCloexec(pipe_in[1]);
		MakeFDCloexec(pipe_out[0]);
		MakeFDCloexec(pipe_out[1]);

		if (_pid==0) {
			std::cin.sync();
			std::cout.flush();
			std::cerr.flush();
			std::clog.flush();
			dup2(pipe_in[0], STDIN_FILENO);
			dup2(pipe_out[1], STDOUT_FILENO);
			dup2(pipe_out[1], STDERR_FILENO);
			CheckedCloseFDPair(pipe_in);
			CheckedCloseFDPair(pipe_out);
			if (!hc_override.empty()) {
				setenv("HISTCONTROL", hc_override.c_str(), 1);
			}
			setsid();
			execlp("bash", "bash", "--noprofile", "-i", NULL);
			perror("execlp");
			_exit(0);
			exit(0);
		}

		CheckedCloseFD(pipe_in[0]);
		CheckedCloseFD(pipe_out[1]);
		_pipe_stdin = pipe_in[1];
		_pipe_stdout = pipe_out[0];
	}
	return true;
}

void VTCompletor::Stop()
{
	CheckedCloseFD(_pipe_stdin);
	CheckedCloseFD(_pipe_stdout);
	if (_pid!=-1) {
		int s;
		if (waitpid(_pid, &s, 0)!=_pid) perror("VTCompletor: waitpid");
		_pid = -1;
		_pty_used = false;
	}
}

static void AvoidMarkerCollision(std::string &marker, const std::string &cmd)
{
	for (const size_t orig_len = marker.size(); cmd.find(marker) != std::string::npos; ) {
		marker.resize(orig_len);
		RandomStringAppend(marker, 4, 10, RNDF_ALNUM);
	}
}

bool VTCompletor::TalkWithShell(const std::string &cmd, std::string &reply, const char *tabs)
{	
	if (!EnsureStarted())
		return false;

	std::string begin = " true jkJHYvgT"; // most unique string in Universe
	std::string done = "K2Ld8Gfg"; // another most unique string in Universe
	AvoidMarkerCollision(done, cmd);  // if it still not enough unique
	AvoidMarkerCollision(begin, cmd);  // if it still not enough unique
	begin+= '\n';
	// dont do that: done+= '\n'; otherwise proposed command is executed, see https://github.com/elfmz/far2l/issues/1244

	std::string sendline = " PS1=''; PS2=''; PS3=''; PS4=''; PROMPT_COMMAND=''";
//	sendline+= " set +o history\n";
	if (!_vtc_inputrc.empty()) {
		sendline+= "; bind -f \"";
		sendline+= _vtc_inputrc;
		sendline+= "\"";
	}
	sendline+= '\n';

	sendline+= begin;
	sendline+= cmd;
	sendline+= tabs;
	sendline+= done;
	
	if (write(_pipe_stdin, sendline.c_str(), sendline.size()) != (ssize_t)sendline.size()) {
		perror("VTCompletor: write");
		Stop();
		return false;
	}

	reply.clear();
	fd_set fds;
	struct timeval tv;
	bool skip_echo = true;
	for (;;) {
		FD_ZERO(&fds);
		FD_SET(_pipe_stdout, &fds);
		tv.tv_sec = reply.empty() ? 2 : 1;
		tv.tv_usec = 0;
		int rv = select(_pipe_stdout + 1, &fds, NULL, NULL, &tv);
		if(rv == -1) {
			perror("VTCompletor: select");
			break;
		}
		if(rv == 0) { //timeout
			fprintf(stderr, "VTCompletor: timeout\n");
			break;
		}
		char buf[VeryLongTerminalLine];
		ssize_t r = read(_pipe_stdout, buf, sizeof(buf)); /* there was data to read */
		if (r <= 0) {
			perror("VTCompletor: read");
			break;
		}
		reply.append(buf, r);
		size_t p = reply.rfind(done);
		if (p!=std::string::npos) {
			if (_pty_used && skip_echo) {
				reply.erase(0, p + done.size());
				skip_echo = false;
			} else {
				reply.resize(p);
				break;
			}
		}
	}
	Stop();

	const std::string &vtc_log = InMyTemp("vtc.log");
	FILE *f = fopen(vtc_log.c_str(), "w");
	if (f) {
		fwrite(cmd.c_str(), cmd.size(), 1, f);
		fwrite(tabs, strlen(tabs), 1, f);
		fwrite("\n---\n", 5, 1, f);
		fwrite(reply.c_str(), reply.size(), 1, f);
		fclose(f);
	}

	size_t p = reply.find(begin);
	if (p != std::string::npos) {
		reply.erase(0, p + begin.size());
	}
	for (;;) {
		p = reply.rfind('\a');
		if (p == std::string::npos) break;
		reply.erase(p, 1);
		if (p >= cmd.size() && reply.substr(p - cmd.size(), cmd.size()) == cmd) {
			reply.erase(0, p - cmd.size());
			break;
		}
	}

	return true;
}


bool VTCompletor::ExpandCommand(std::string &cmd)
{
	std::string reply;
	if (!TalkWithShell(cmd, reply, "\t"))
		return false;

	size_t p = reply.find(cmd);
	if (p == std::string::npos)
		return false;

	reply.erase(0, p);
	if (reply.empty())
		return false;

	cmd.swap(reply);
	return true;
}

bool VTCompletor::GetPossibilities(const std::string &cmd, std::vector<std::string> &possibilities)
{
	std::string eval_cmd = cmd;
	Environment::Arguments args;
	Environment::ParseCommandLine(eval_cmd, args, false);
	if (args.empty()) {
		return false;
	}
	eval_cmd = cmd;
	for (auto rit = args.rbegin(); rit != args.rend(); ++rit) {
		const auto &a = cmd.substr(rit->orig_begin, rit->orig_len);
		if (a == "|") {
			if (rit == args.rbegin()) {
				return false;
			}
			--rit;
			eval_cmd = cmd.substr(rit->orig_begin);
			break;
		}
	}

	if (eval_cmd.empty()) {
		return false;
	}

//fprintf(stderr, "!!! eval_cmd='%s'\n", eval_cmd.c_str());
	std::string reply;
	if (!TalkWithShell(eval_cmd, reply, "\t\t")) {
		return false;
	}

	const auto &last_a = cmd.substr(args.back().orig_begin, args.back().orig_len);
	const bool whole_next_arg = (eval_cmd.back() == ' ' && args.back().quot == Environment::QUOT_NONE);

	size_t p = reply.find(eval_cmd);
	if (p == std::string::npos || p + eval_cmd.size() >= reply.size() ) {
		return false;
	}

	reply.erase(0, p + eval_cmd.size());

	if (StrEndsBy(reply, eval_cmd.c_str())) {
		reply.resize(reply.size() - eval_cmd.size());
	}

	for (;;) {
		p = reply.find('\n');
		if (p == std::string::npos ) break;
		possibilities.emplace_back(reply.substr(0, p));
		StrTrim(possibilities.back(), " \r");
		if (possibilities.back().empty()) {
			possibilities.pop_back();
		}
		reply.erase(0, p + 1);
	}

	if (!possibilities.empty() && possibilities.back() == eval_cmd) {
		possibilities.pop_back();
	}

	std::sort(possibilities.begin(), possibilities.end());

	const size_t last_a_slash_pos = last_a.rfind('/');
	const size_t args_orig_begin_slash_pos = cmd.find('/', args.back().orig_begin);

	for (auto &possibility : possibilities) {
		QuoteCmdArgIfNeed(possibility);
		if (!whole_next_arg && StrStartsFrom(possibility, last_a.c_str())) {
			possibility.insert(0, cmd.substr(0, args.back().orig_begin));

		} else if (!whole_next_arg && last_a_slash_pos != std::string::npos
				&& args_orig_begin_slash_pos != std::string::npos
				&& last_a_slash_pos + 1 < last_a.size()
				&& StrStartsFrom(possibility, last_a.substr(last_a_slash_pos + 1).c_str())) {
			// #1660
			possibility.insert(0, cmd.substr(0, args_orig_begin_slash_pos + 1));

		} else {
			possibility.insert(0, cmd);
		}
	}

	return true;
}
