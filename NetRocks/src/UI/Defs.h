#pragma once
#include <string>
#include <mutex>
#include <chrono>

enum XferKind
{
	XK_NONE,
	XK_COPY,
	XK_MOVE,
	XK_RENAME
};

enum XferDirection
{
	XD_DOWNLOAD,
	XD_UPLOAD,
	XD_CROSSLOAD
};

enum XferOverwriteAction
{
	XOA_CANCEL = 0, // only with ConfirmOverwrite
	XOA_ASK = 0,    // only with ConfirmXfer
	XOA_SKIP,
	XOA_RESUME,
	XOA_OVERWRITE,
	XOA_OVERWRITE_IF_NEWER,
	XOA_CREATE_DIFFERENT_NAME,
	XOA_OVERWRITE_IF_NEWER_OTHERWISE_ASK
};

enum WhatOnErrorKind
{
	WEK_DOWNLOAD = 0,
	WEK_UPLOAD,
	WEK_CROSSLOAD,
	WEK_QUERYINFO,
	WEK_CHECKDIR,
	WEK_ENUMDIR,
	WEK_MAKEDIR,
	WEK_RENAME,
	WEK_REMOVE,
	WEK_SETTIMES,
	WEK_CHMODE,
	WEK_SYMLINK_QUERY,
	WEK_SYMLINK_CREATE,
	WEKS_COUNT
};



enum WhatOnErrorAction
{
	WEA_CANCEL = 0, // meaningful as dialog result
	WEA_ASK = 0,    // meaningful as default action
	WEA_SKIP,
	WEA_RETRY,

	WEA_RECOVERY,	// for directory validation - allows traversing to first valid parent directory
};

struct ProgressStateStats
{
	unsigned long long file_complete = 0, file_total = 0;
	unsigned long long all_complete = 0, all_total = 0;
	unsigned long long count_complete = 0, count_total = 0;
	unsigned long long count_skips = 0, count_retries = 0;
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
};

