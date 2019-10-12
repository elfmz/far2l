#pragma once
#include <time.h>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "Host/Host.h"

class ConnectionsPool
{
	struct PooledHost
	{
		time_t ts;
		std::shared_ptr<IHost> host;
	};

	std::map<std::string, PooledHost> _server_2_pooled_host;
	std::mutex _mutex;

	void PurgeTimedOutEntries(std::vector<std::shared_ptr<IHost> > &purgeds);

public:

	void Put(const std::string &server, std::shared_ptr<IHost> &host);
	bool Get(const std::string &server, std::shared_ptr<IHost> &host);

	void Purge();
};
