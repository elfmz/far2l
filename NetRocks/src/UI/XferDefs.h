#pragma once
#include <string>
#include <mutex>

enum XferKind
{
	XK_COPY,
	XK_MOVE
};

enum XferDirection
{
	XK_DOWNLOAD,
	XK_UPLOAD
};

enum XferDefaultOverwriteAction
{
	XDOA_ASK,
	XDOA_SKIP,
	XDOA_RESUME,
	XDOA_OVERWRITE,
	XDOA_OVERWRITE_IF_NEWER,
	XDOA_CREATE_DIFFERENT_NAME
};


struct XferStateStats
{
	volatile long long total_count = 0, current_count = 0;
	volatile long long total_size = 0, current_size = 0;
	clock_t total_start = 0, current_start = 0;
	clock_t total_paused = 0, current_paused = 0;
};

struct XferState
{
	std::mutex mtx;
	XferStateStats stats;
	const std::string *name = nullptr;
	bool paused = false, aborting = false, finished = false;

	void Reset()
	{
		stats = XferStateStats();
		name = nullptr;
		paused = aborting = finished = false;
	}
};
