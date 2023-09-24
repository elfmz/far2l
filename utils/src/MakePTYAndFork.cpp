#include "MakePTYAndFork.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <utils.h>

pid_t MakePTYAndFork(int &pty_master)
{
	pty_master = posix_openpt(O_RDWR | O_NOCTTY);
	MakeFDCloexec(pty_master);
	if (pty_master < 0) {
		perror("posix_openpt");
		return -1;
	}
	if (grantpt(pty_master) != 0) {
		perror("grantpt");
		CheckedCloseFD(pty_master);
		return -1;
	}
	if (unlockpt(pty_master) != 0) {
		perror("unlockpt");
		CheckedCloseFD(pty_master);
		return -1;
	}

	const char *pty_name = ptsname(pty_master);
	if (!pty_name) {
		perror("ptsname");
		CheckedCloseFD(pty_master);
		return -1;
	}
	std::string str_pty_name = pty_name;

	std::cin.sync();
	std::cout.flush();
	std::cerr.flush();
	std::clog.flush();
	pid_t pid = fork();
	if (pid == (pid_t)-1) { // fork failed
		perror("fork");
		CheckedCloseFD(pty_master);

	} else if (pid == 0) { // child process
		setsid();
		int pty_slave = open(str_pty_name.c_str(), O_RDWR);
		if (pty_slave == -1
				|| dup2(pty_slave, STDIN_FILENO) == -1
				|| dup2(pty_slave, STDOUT_FILENO) == -1
				|| dup2(pty_slave, STDERR_FILENO) == -1) {
			perror("pty_slave");
			_exit(1);
			exit(1);
		}
		close(pty_slave);
		if ( ioctl( STDIN_FILENO, TIOCSCTTY, 0 ) == -1 ) {
			perror( "VT: ioctl(TIOCSCTTY)" );
		}
	}

	return pid;
}
