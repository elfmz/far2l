#pragma once
#include "../Protocol/Protocol.h"


struct IHost : IProtocol
{
	virtual std::string SiteName() const = 0;
	virtual void ReInitialize() throw (std::runtime_error) = 0;
	virtual void Abort() = 0;
};
