#pragma once
#include <string>
#include <map>
#include <set>
#include <memory>
#include <Threaded.h>
#include "../Host/Host.h"
#include "./Utils/ProgressStateUpdate.h"
#include "../UI/Defs.h"
#include "../UI/Activities/WhatOnError.h"
#include <plugin.hpp>

class OpBase : protected Threaded, protected IAbortableOperationsHost
{
	OpBase(const OpBase &) = delete;
protected:
	int _notify_title_lng = -1;
	int _op_mode;
	std::shared_ptr<IHost> _base_host;
	std::string _base_dir;

	ProgressState _state;
	WhatOnErrorState _wea_state;

	bool WaitThread(unsigned int msec = (unsigned int)-1);

	virtual void *ThreadProc();	// Threaded
	virtual void ForcefullyAbort();	// IAbortableOperationsHost

//
	virtual void Process() = 0;

	void SetNotifyTitle(int title_lng = -1);
public:
	OpBase(int op_mode, std::shared_ptr<IHost> base_host, const std::string &base_dir);
	~OpBase();

};


#define IS_SILENT(v)               ( ((v) & (OPM_FIND|OPM_VIEW|OPM_EDIT|OPM_SILENT)) != 0 )
