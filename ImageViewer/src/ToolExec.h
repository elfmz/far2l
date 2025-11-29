#pragma once
#include <atomic>
#include <string>
#include "utils.h"
#include "ExecAsync.h"

class ToolExec : public ExecAsync
{
	std::atomic<bool> _exited{false};
	volatile bool *_cancel{nullptr};

	void KillAndWait();

public:
	ToolExec(volatile bool *cancel);
	void ErrorDialog(const char *pkg, int err);
	void InfoDialog(const char *pkg);
	void ProgressDialog(const std::string &file, const std::string &size_str, const char *pkg, const std::string &info);
	static VOID sCallback(VOID *Context);
	bool FN_PRINTF_ARGS(5) Run(const std::string &file, const std::string &size_str, const char *pkg, const char *info_fmt, ...);
	virtual void *ThreadProc();
};
