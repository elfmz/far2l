#include <stdlib.h>
#include <string>
#include <locale>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "utils.h"
#include "Backend.h"


static IShareBackendOptions *g_share_backend = nullptr;
static std::mutex g_share_backend_mutex;

__attribute__ ((visibility("default"))) IShareBackendOptions *WinPortShareBackendOptions_SetBackend(IShareBackendOptions *share_backend)
{
	std::lock_guard<std::mutex> lock(g_share_backend_mutex);
	auto out = g_share_backend;
	g_share_backend = share_backend;
	return out;
}

extern "C" {

	WINPORT_DECL(ShareBackendOptions, VOID, (PVOID options))
	{
		std::lock_guard<std::mutex> lock(g_share_backend_mutex);
		if(g_share_backend) g_share_backend->ShareBackendOptions(options);
	}
}
