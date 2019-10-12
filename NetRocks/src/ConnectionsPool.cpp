#include <vector>
#include "Globals.h"
#include "ConnectionsPool.h"

void ConnectionsPool::PurgeTimedOutEntries(std::vector<std::shared_ptr<IHost> > &purgeds)
{
	const time_t now = time(NULL);
	const time_t timeout = G.global_config
		? G.global_config->GetInt("Options", "ConnectionsPoolTimeout", 30) : 30;

	for (auto it = _server_2_pooled_host.begin(); it != _server_2_pooled_host.end(); ) {
		if (it->second.ts - now >= timeout) {
			purgeds.emplace_back(it->second.host);
			it = _server_2_pooled_host.erase(it);
		} else
			++it;
	}
}

void ConnectionsPool::Put(const std::string &server, std::shared_ptr<IHost> &host)
{
	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	if (!server.empty() && host) {
		auto &pp = _server_2_pooled_host[server];
		if (pp.host) {
			purgeds.emplace_back(pp.host);
		}
		pp.ts = time(nullptr);
		pp.host = host;
	}

	PurgeTimedOutEntries(purgeds);
}

bool ConnectionsPool::Get(const std::string &server, std::shared_ptr<IHost> &host)
{
	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	PurgeTimedOutEntries(purgeds);

	auto it = _server_2_pooled_host.find(server);
	if (it == _server_2_pooled_host.end()) {
		return false;
	}

	host = it->second.host;
	_server_2_pooled_host.erase(it);
	return true;
}

void ConnectionsPool::Purge()
{
	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	for (auto &it : _server_2_pooled_host) {
		purgeds.emplace_back(it.second.host);
	}
	_server_2_pooled_host.clear();
}
