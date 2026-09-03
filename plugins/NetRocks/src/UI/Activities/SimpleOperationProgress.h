#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"
#include "../Defs.h"

class SimpleOperationProgress : protected BaseDialog
{
public:
	enum Kind
	{
		K_CONNECT,
		K_GETMODE,
		K_CHANGEMODE,
		K_ENUMDIR,
		K_CREATEDIR,
		K_EXECUTE,
		K_GETLINK,
	};

	SimpleOperationProgress(Kind kind, const std::string &object, ProgressState &state);
	void Show();

protected:
	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2);

private:
	Kind _kind;
	ProgressState &_state;
	int _finished = 0;
	int _i_dblbox = -1;
	int _i_errstats_separator = -1;
	bool _errstats_colored = false;
	ProgressStateStats _last_stats;
	std::string _title;
};
