#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <wordexp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <utils.h>
#include "vtcompletor.h"

static const char *vtc_inputrc = "set completion-query-items 0\n"
									"set page-completions off\n";


VTCompletor::VTCompletor()
	: _vtc_inputrc(InMyTemp("vtc_inputrc")),
	_pipe_stdin(-1), _pipe_stdout(-1), _pid(-1)
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
	if (_pid!=-1)
		return true;

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

	fcntl(pipe_in[0], F_SETFD, FD_CLOEXEC);
	fcntl(pipe_in[1], F_SETFD, FD_CLOEXEC);
	fcntl(pipe_out[0], F_SETFD, FD_CLOEXEC);
	fcntl(pipe_out[1], F_SETFD, FD_CLOEXEC);

	if (_pid==0) {
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);
		dup2(pipe_out[1], STDERR_FILENO);
		CheckedCloseFDPair(pipe_in);
		CheckedCloseFDPair(pipe_out);
		execl("/bin/bash", "bash", "--noprofile", "-i",  NULL);
		perror("execl");
		exit(0);
	}

	CheckedCloseFD(pipe_in[0]);
	CheckedCloseFD(pipe_out[1]);
	_pipe_stdin = pipe_in[1];
	_pipe_stdout = pipe_out[0];
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
	}
}

bool VTCompletor::TalkWithShell(const std::string &cmd, std::string &reply, const char *tabs)
{	
	if (!EnsureStarted())
		return false;

	std::string done = "K2Ld8Gfg";//most unique string in Universe
	if (cmd.find(done)!=std::string::npos) {
		srand(time(NULL));
		do { //if it still not enough unique
			done+= 'a' + (rand() % ('z' + 1 - 'a'));		
		} while (cmd.find(done)!=std::string::npos);
	}

	std::string sendline = "PS1=''\n";
	if (!_vtc_inputrc.empty()) {
		sendline+= "bind -f \"";
		sendline+= _vtc_inputrc;
		sendline+= "\"\n";
	}
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
		char buf[0x1000];
    		ssize_t r = read(_pipe_stdout, buf, sizeof(buf)); /* there was data to read */
		if (r <= 0) {
			perror("VTCompletor: read");
			break;
		}
		reply.append(buf, r);
		size_t p = reply.rfind(done);
		if (p!=std::string::npos) {
			reply.resize(p);
			break;
		}
	}
	Stop();
	
	for (;;) {
		 size_t p = reply.find('\a');
		 if (p==std::string::npos) break;
		 reply.erase(p, 1);
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
	std::string reply;
	if (!TalkWithShell(cmd, reply, "\t\t"))
		return false;

	size_t p = reply.find(cmd);
	if (p == std::string::npos || p + cmd.size() >= reply.size() )
		return false;

	reply.erase(0, p + cmd.size());
	
	for (;;) {
		p = reply.find('\n');
		size_t pt = reply.find('\t');
		size_t ps = reply.find(' ');
		if (p==std::string::npos || (pt!=std::string::npos && pt < p)) p = pt;
		if (p==std::string::npos || (ps!=std::string::npos && ps < p)) p = ps;
		
		if (p==std::string::npos ) break;
		if (p > 0)
			possibilities.emplace_back(reply.substr(0, p));
		reply.erase(0, p + 1);
	}
	
	return true;
}
