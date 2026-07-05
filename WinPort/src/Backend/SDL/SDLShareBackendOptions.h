#pragma once
#include "Backend.h"

class SDLShareBackendOptionsBackend : public IShareBackendOptions
{
public:
	SDLShareBackendOptionsBackend();
	virtual ~SDLShareBackendOptionsBackend();
	virtual void ShareBackendOptions(void* options);
};
