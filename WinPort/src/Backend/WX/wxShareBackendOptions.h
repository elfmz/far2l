#pragma once
#include "Backend.h"

class wxShareBackendOptionsBackend : public IShareBackendOptions
{
public:
	wxShareBackendOptionsBackend();
	virtual ~wxShareBackendOptionsBackend();
	virtual void ShareBackendOptions(void* options);
};
