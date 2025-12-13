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

private:
	void WorkerThread();

	std::thread _thread;
	std::atomic<bool> _running{ false };
	std::atomic<bool> _focused{ true };
	struct DBusState;
	DBusState* _dbus = nullptr;
};