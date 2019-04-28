#pragma once
#include <string>
#include <map>
#include <set>
#include <memory>
#include <Threaded.h>
#include <all_far.h>
#include <fstdlib.h>
#include "../SiteConnection.h"
#include "./Utils/ProgressStateUpdate.h"
#include "../UI/Progress.h"

class OpBase : protected Threaded, protected IAbortableOperationsHost
{
protected:
	std::shared_ptr<SiteConnection> _connection;
	int _op_mode;
	std::string _base_dir;

	ProgressState _state;

	virtual void *ThreadProc();	// Threaded
	virtual void ForcefullyAbort();	// IAbortableOperationsHost

//
	virtual void Process() = 0;

public:
	OpBase(std::shared_ptr<SiteConnection> connection, int op_mode, const std::string &base_dir);
};


