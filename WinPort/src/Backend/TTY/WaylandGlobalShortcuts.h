#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

class WaylandGlobalShortcuts
{
public:
	WaylandGlobalShortcuts();
	~WaylandGlobalShortcuts();

	void Start();
	void Stop();
	void SetFocused(bool focused);
	void SetPaused(bool paused);
	bool IsRecentlyActive(uint32_t window_ms);

private:
	void WorkerThread();

	std::thread _thread;
	std::atomic<bool> _running{ false };
	std::atomic<bool> _focused{ true };
	std::atomic<bool> _paused{ false };
	std::atomic<uint64_t> _last_activity_ts{ 0 };
	struct DBusState;
	DBusState* _dbus = nullptr;
};