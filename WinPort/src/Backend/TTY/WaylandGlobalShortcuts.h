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

private:
	void WorkerThread();

	std::thread _thread;
	std::atomic<bool> _running{ false };
	struct DBusState;
	DBusState* _dbus = nullptr;
};