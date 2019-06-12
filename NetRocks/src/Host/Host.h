#pragma once
#include "../Protocol/Protocol.h"

// all methods of this interface are NOT thread-safe unless explicitely marked as MT-safe
struct IHost : IProtocol
{
	virtual std::string SiteName() = 0; // MT-safe, human-readable site's name
	virtual std::string Identity() = 0; // MT-safe, use returned string to check if two hosts refer same server/credentials

	virtual std::shared_ptr<IHost> Clone() = 0; // MT-safe, creates clone of this host that will init automatically with same creds
	virtual void ReInitialize() throw (std::runtime_error) = 0;
	virtual void Abort() = 0; // MT-safe, forcefully aborts connection and any outstanding operation
};
