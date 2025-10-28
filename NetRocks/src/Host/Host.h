#pragma once
#include <memory>
#include <string>
#include "../Protocol/Protocol.h"

// all methods of this interface are NOT thread-safe unless explicitly marked as MT-safe


struct IHost : IProtocol
{
	struct Identity
	{
		std::string protocol;
		std::string host;
		std::string username;
		unsigned int port = 0;
	};

	virtual std::string SiteName() = 0; // MT-safe, human-readable site's name
	virtual void GetIdentity(Identity &identity) = 0; // MT-safe, returns connection host identity details

	virtual std::shared_ptr<IHost> Clone() = 0; // MT-safe, creates clone of this host that will init automatically with same creds
	virtual void ReInitialize() = 0;
	virtual void Abort() = 0; // MT-safe, forcefully aborts connection and any outstanding operation

	virtual bool Alive() = 0; // MT-safe, returns true if connection looks alive
};
