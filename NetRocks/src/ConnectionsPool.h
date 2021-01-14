#pragma once
#include <time.h>
#include <string>
#include <map>
#include <memory>
#include <condition_variable>
#include <mutex>
#include "Host/Host.h"

#include <Threaded.h>

class ConnectionsPool : Threaded
{
	struct PooledHost
	{
		time_t ts;
		std::shared_ptr<IHost> host;
	};

	std::map<std::string, PooledHost> _id_2_pooled_host;
	std::mutex _mutex;
	std::condition_variable _cond;

	void UpdateThreadState();
	void PurgeExpired(std::vector<std::shared_ptr<IHost> > &purgeds);
	time_t EstimateTimeToSleep();

protected:
	virtual void *ThreadProc();

public:
	~ConnectionsPool();

	void Put(const std::string &id, std::shared_ptr<IHost> &host);
	std::shared_ptr<IHost> Get(const std::string &id);

	void PurgeAll();
	void OnGlobalSettingsChanged();
};
