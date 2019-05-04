#pragma once
#include "../Protocol/Protocol.h"


struct IHost : IProtocol
{
	virtual void ReInitialize() throw (std::runtime_error) = 0;
	virtual void Abort() = 0;
};
