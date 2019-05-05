#pragma once
#include <string>
#include <mutex>
#include <chrono>

enum XferKind
{
	XK_COPY,
	XK_MOVE
};

enum XferDirection
{
	XK_DOWNLOAD,
	XK_UPLOAD,
	XK_CROSSLOAD
};

enum XferOverwriteAction
{
	XOA_CANCEL = 0, // only with ConfirmOverwrite
	XOA_ASK = 0,    // only with ConfirmXfer
	XOA_SKIP,
	XOA_RESUME,
	XOA_OVERWRITE,
	XOA_OVERWRITE_IF_NEWER,
	XOA_CREATE_DIFFERENT_NAME
};


struct ProgressStateStats
{
	unsigned long long file_complete = 0, file_total = 0;
	unsigned long long all_complete = 0, all_total = 0;
	unsigned long long count_complete = 0, count_total = 0;
	std::chrono::milliseconds total_start = {}, current_start = {};
	std::chrono::milliseconds total_paused = {}, current_paused = {};
};

struct IAbortableOperationsHost
{
	virtual void ForcefullyAbort() = 0;
};

struct ProgressState
{
	std::mutex mtx;
	ProgressStateStats stats;
	std::string path;
	bool paused = false, aborting = false, finished = false;
	IAbortableOperationsHost *ao_host = nullptr;

	void Reset()
	{
		stats = ProgressStateStats();
		path.clear();
		paused = aborting = finished = false;
	}
};
