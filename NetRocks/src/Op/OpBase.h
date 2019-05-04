#pragma once
#include <string>
#include <map>
#include <set>
#include <memory>
#include <Threaded.h>
#include <all_far.h>
#include <fstdlib.h>
#include "../Host/Host.h"
#include "./Utils/ProgressStateUpdate.h"
#include "../UI/Progress.h"

class OpBase : protected Threaded, protected IAbortableOperationsHost
{
protected:
	std::shared_ptr<IHost> _base_host;
	int _op_mode;
	std::string _base_dir;
	int _op_name_lng;
	volatile bool _succeded;

	ProgressState _state;

	bool WaitThread(unsigned int msec = (unsigned int)-1);

	virtual void *ThreadProc();	// Threaded
	virtual void ForcefullyAbort();	// IAbortableOperationsHost

//
	virtual void Process() = 0;

public:
	OpBase(std::shared_ptr<IHost> base_host, int op_mode, const std::string &base_dir, int op_name_lng = -1);
	~OpBase();
};


