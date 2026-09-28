#include <vector>
#include "Globals.h"
#include "ConnectionsPool.h"



static time_t GetGlobalConfigExpiration()
{
	return G.GetGlobalConfigInt("ConnectionsPoolExpiration", 30);
}

ConnectionsPool::~ConnectionsPool()
{
	PurgeAll();
	WaitThread();
}

void ConnectionsPool::Put(const std::string &id, std::shared_ptr<IHost> &host)
{
	if (id.empty() || !host || !host->Alive()) {
		return;
	}

	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	auto &pp = _id_2_pooled_host[id];
	if (pp.host) {
		purgeds.emplace_back(pp.host);
	}
	pp.ts = time(nullptr);
	pp.host = host;

	PurgeExpired(purgeds);
	UpdateThreadState();
}

std::shared_ptr<IHost> ConnectionsPool::Get(const std::string &id)
{
	std::shared_ptr<IHost> out;

	std::lock_guard<std::mutex> locker(_mutex);

	auto it = _id_2_pooled_host.find(id);
	if (it != _id_2_pooled_host.end()) {
		out = it->second.host;
		_id_2_pooled_host.erase(it);
		UpdateThreadState();
	}

	return out;
}

void ConnectionsPool::PurgeAll()
{
	std::vector<std::shared_ptr<IHost> > purgeds; // destroy hosts out of lock
	std::lock_guard<std::mutex> locker(_mutex);

	for (auto &it : _id_2_pooled_host) {
		purgeds.emplace_back(it.second.host);
	}
	_id_2_pooled_host.clear();
	_cond.notify_all();
}

void ConnectionsPool::PurgeExpired(std::vector<std::shared_ptr<IHost> > &purgeds)
{
	const time_t now = time(NULL);
	const time_t expiration = GetGlobalConfigExpiration();

	for (auto it = _id_2_pooled_host.begin(); it != _id_2_pooled_host.end(); ) {
		if (now - it->second.ts >= expiration) {
			purgeds.emplace_back(it->second.host);
			it = _id_2_pooled_host.erase(it);
		} else
			++it;
	}
}

time_t ConnectionsPool::EstimateTimeToSleep()
{
	const time_t now = time(NULL);
	const time_t expiration = GetGlobalConfigExpiration();
	time_t out = expiration / 2;

	for (const auto &it : _id_2_pooled_host) {
		time_t t = (now - it.second.ts);
		if (t >= expiration) {
			out = 0;
			break;
		}
		t = expiration - t;
		if (out > t) {
			out = t;
		}
	}

	return out + 1;
}

void *ConnectionsPool::ThreadProc()
{
	std::vector<std::shared_ptr<IHost> > purgeds;
	std::unique_lock<std::mutex> lock(_mutex);
	while (!_id_2_pooled_host.empty()) {
		time_t sleep_time = EstimateTimeToSleep();

		_cond.wait_for(lock, std::chrono::seconds(sleep_time));

		PurgeExpired(purgeds);
		if (!purgeds.empty()) {
			lock.unlock();
			purgeds.clear(); // destroy hosts out of lock
			lock.lock();
		}
	}
	return nullptr;
}

void ConnectionsPool::UpdateThreadState()
{
	if (!_id_2_pooled_host.empty()) {
		if (StartThread()) {
			return;
		}
	}

	_cond.notify_all();
}

void ConnectionsPool::OnGlobalSettingsChanged()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.notify_all();
}
