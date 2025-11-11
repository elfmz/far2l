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
	int _exit_signal{-1}, _exit_code{-1};
	int _kill_fd[2]{-1, -1};
	pid_t _pid{-1};

	virtual void *ThreadProc();

	void Kill(int sig);

public:
	ExecAsync(const char *program);
	~ExecAsync();
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


	bool Wait(int timeout_msec = -1);

	void KillSoftly();
	void KillHardly();

	int ExitSignal();
	int ExitCode();

	void FetchStdout(std::vector<char> &content);
	void FetchStderr(std::vector<char> &content);

	std::string FetchStdout();
	std::string FetchStderr();
};
