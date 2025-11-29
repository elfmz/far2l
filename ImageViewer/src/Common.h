#pragma once
#include <string>
#include <unordered_set>
#include <assert.h>

#include <farplug-wide.h>
#include <WinPort.h>
#include <farkeys.h>
#include <farcolor.h>

#define WINPORT_IMAGE_ID "image_viewer"

enum EXITED_DUE
{
	EXITED_DUE_ERROR  = 11,
	EXITED_DUE_ENTER  = 42,
	EXITED_DUE_ESCAPE = 24,
	EXITED_DUE_RESIZE = 37
};

extern PluginStartupInfo g_far;
extern FarStandardFunctions g_fsf;

void PurgeAccumulatedInputEvents();
bool CheckForEscAndPurgeAccumulatedInputEvents();

EXITED_DUE ShowImageAtFull(size_t initial_file, std::vector<std::pair<std::string, bool> > &all_files, std::unordered_set<std::string> &selection, bool silent_exit_on_error);
EXITED_DUE ShowImageAtFull(const std::string &file, bool silent_exit_on_error);

void ShowImageAtQV(const std::string &file, const SMALL_RECT &area);
bool IsShowingImageAtQV();
void DismissImageAtQV();

void RectReduce(SMALL_RECT &rc);
