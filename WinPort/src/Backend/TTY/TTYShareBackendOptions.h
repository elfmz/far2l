#pragma once
#include "Backend.h"

class ttyShareBackendOptionsBackend : public IShareBackendOptions
{
public:
	ttyShareBackendOptionsBackend();
	virtual ~ttyShareBackendOptionsBackend();

	virtual void ShareBackendOptions(void* options);
};
