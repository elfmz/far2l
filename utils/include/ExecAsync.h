#pragma once
#include <vector>
#include <string>
#include <mutex>
#include "Threaded.h"

class ExecAsync : Threaded
{
	std::string _program;
	std::vector<std::string> _args;

	std::vector<char> _stdin, _stdout, _stderr;
	std::mutex _mtx;
	bool _dont_care{false};
	bool _started{false};
	int _exec_error{0};
	int _exit_signal{0}, _exit_code{-1};
	int _kill_fd[2]{-1, -1};
	pid_t _pid{-1};

	void Kill(int sig);

protected:
	virtual void *ThreadProc();

public:
	ExecAsync(const char *program = "");
	~ExecAsync();

	const std::vector<std::string> &GetArguments() const
	{
		return _args;
	}

	void AddArgument(const char *a);

	// use if command stdout/stderr is out of interest,
	// so only tailing parts of them will be available from FetchStdout/FetchStderr
	// however stderr is still fully mirrored to own stderr
	void DontCare();

	ExecAsync &AddArguments(const char *a)
	{
		AddArgument(a);
		return *this;
	}

	ExecAsync &AddArguments(const std::string &a)
	{
		AddArgument(a.c_str());
		return *this;
	}

	template <class NUM_T>
		ExecAsync &AddArguments(const NUM_T &n)
	{
		AddArgument(std::to_string(n).c_str());
		return *this;
	}

	template <class FirstItemT, class SecondItemT, class... OtherItemsT>
		ExecAsync &AddArguments(const FirstItemT &FirstItem, const SecondItemT &SecondItem, OtherItemsT... OtherItems)
	{
		return AddArguments(FirstItem).AddArguments(SecondItem, OtherItems...);
	}

	void Stdin(const std::vector<char> &v);

	bool Start();


	template <class FirstItemT, class... OtherItemsT>
		bool StartWithArguments(const FirstItemT &FirstItem, OtherItemsT... OtherItems)
	{
		AddArguments(FirstItem, OtherItems...);
		return Start();
	}


	// following functions can be used only after Start() returned true
	bool Wait(int timeout_msec = -1);

	void KillSoftly(); // send SIGINT to process
	void KillHardly(); // send SIGKILL to process and disrupt stdio dispatcher loop

	void FetchStdout(std::vector<char> &content); // get currently buffered-in stdout content and clear that buffer
	std::string FetchStdout(); // same as above

	void FetchStderr(std::vector<char> &content); // get currently buffered-in stderr content and clear that buffer
	std::string FetchStderr(); // same as above

	// following functions can be used only after Wait(..) returned true
	int ExecError();  // nonzero error code if execvp failed (e.g. program not found)
	int ExitSignal(); // nonzero signal number if app exited due to signal
	int ExitCode();   // program exit code in case ExecError() and ExitSignal() both returned zeroes
};
