#pragma once
#include <string>
#include <map>
#include <set>
#include <memory>
#include <Threaded.h>
#include "../Host/Host.h"
#include "./Utils/ProgressStateUpdate.h"
#include "../UI/Progress.h"
#include "../UI/WhatOnError.h"
#include <plugin.hpp>

class OpBase : protected Threaded, protected IAbortableOperationsHost
{
protected:
	int _op_name_lng;
	int _op_mode;
	std::shared_ptr<IHost> _base_host;
	std::string _base_dir;
	volatile bool _succeded;

	ProgressState _state;
	WhatOnErrorState _wea_state;

	bool WaitThread(unsigned int msec = (unsigned int)-1);

	virtual void *ThreadProc();	// Threaded
	virtual void ForcefullyAbort();	// IAbortableOperationsHost

//
	virtual void Process() = 0;

public:
	OpBase(int op_mode, std::shared_ptr<IHost> base_host, const std::string &base_dir, int op_name_lng = -1);
	~OpBase();
};


#define IS_SILENT(v)               ( ((v) & (OPM_FIND|OPM_VIEW|OPM_EDIT)) != 0 )
