#include <vector>
#include "Globals.h"
#include "ConnectionsPool.h"

void ConnectionsPool::Put(const std::string &server, std::shared_ptr<IHost> &host)
{
	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	if (!server.empty() && host && host->Alive()) {
		auto &pp = _server_2_pooled_host[server];
		if (pp.host) {
			purgeds.emplace_back(pp.host);
		}
		pp.ts = time(nullptr);
		pp.host = host;
	}

	PurgeExpiredInternal(purgeds);
}

bool ConnectionsPool::Get(const std::string &server, std::shared_ptr<IHost> &host)
{
	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	PurgeExpiredInternal(purgeds);

	auto it = _server_2_pooled_host.find(server);
	if (it == _server_2_pooled_host.end()) {
		return false;
	}

	host = it->second.host;
	_server_2_pooled_host.erase(it);
	return true;
}

void ConnectionsPool::PurgeExpiredInternal(std::vector<std::shared_ptr<IHost> > &purgeds)
{
	const time_t now = time(NULL);
	const time_t expiration = G.global_config
		? G.global_config->GetInt("Options", "ConnectionsPoolExpiration", 30) : 30;

	for (auto it = _server_2_pooled_host.begin(); it != _server_2_pooled_host.end(); ) {
		if (now - it->second.ts >= expiration) {
			purgeds.emplace_back(it->second.host);
			it = _server_2_pooled_host.erase(it);
		} else
			++it;
	}
}

void ConnectionsPool::PurgeExpired()
{
	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	PurgeExpiredInternal(purgeds);
}

void ConnectionsPool::PurgeAll()
{
	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	for (auto &it : _server_2_pooled_host) {
		purgeds.emplace_back(it.second.host);
	}
	_server_2_pooled_host.clear();
}
