#pragma once
#include <string>
#include <unordered_set>
#include <assert.h>

#include <farplug-wide.h>
#include <WinPort.h>
#include <farkeys.h>
#include <farcolor.h>

#define WINPORT_IMAGE_ID "image_viewer"
#define PLUGIN_TITLE L"Image Viewer"

#define EXITED_DUE_ERROR      -1
#define EXITED_DUE_ENTER      42
#define EXITED_DUE_ESCAPE     24
#define EXITED_DUE_RESIZE     37


extern PluginStartupInfo g_far;
extern FarStandardFunctions g_fsf;

enum DefaultScale {
	DS_EQUAL_SCREEN,
	DS_LESSOREQUAL_SCREEN,
	DS_EQUAL_IMAGE
};

extern DefaultScale g_def_scale;

void PurgeAccumulatedInputEvents();

bool ShowImageAtFull(size_t initial_file, std::vector<std::pair<std::string, bool> > &all_files, std::unordered_set<std::string> &selection);

void ShowImageAtQV(const std::string &file, const SMALL_RECT &area);
bool IsShowingImageAtQV();
void DismissImageAtQV();

