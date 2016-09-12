#pragma once
#include <string>
#include <unistd.h>

class VTCompletor
{
	int _pipe_stdin, _pipe_stdout;
	pid_t _pid;
	public:
	VTCompletor();
	~VTCompletor();

	void Stop();
	bool EnsureStarted();
	bool ExpandCommand(std::string &cmd);
};
