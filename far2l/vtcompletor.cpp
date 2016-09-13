#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "vtcompletor.h"




static void CheckedCloseFD(int &fd)
{
	if (fd!=-1) {
		close(fd);
		fd = -1;
	}
}

static void CheckedCloseFDPair(int *fd)
{
	CheckedCloseFD(fd[0]);
	CheckedCloseFD(fd[1]);
}

VTCompletor::VTCompletor()
	:_pipe_stdin(-1), _pipe_stdout(-1), _pid(-1)
{
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
		execl("/bin/bash", "bash", "-i",  NULL);
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

bool VTCompletor::ExpandCommand(std::string &cmd)
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

	std::string sendline = cmd;
	sendline+= "\t";
	sendline+= done;
	
	
	if (write(_pipe_stdin, sendline.c_str(), sendline.size()) != (ssize_t)sendline.size()) {
		perror("VTCompletor: write");
		Stop();
		return false;
	}

	std::string reply;
	fd_set fds;
	struct timeval tv;
	for (;;) {
		FD_ZERO(&fds);
		FD_SET(_pipe_stdout, &fds);
		tv.tv_sec = 0;
		tv.tv_usec = reply.empty() ? 2000000 : 1000000;
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
	size_t p = reply.rfind(cmd);
	if (p == std::string::npos)
		return false;

	reply.erase(0, p);
	cmd = reply;
	return true;
}
