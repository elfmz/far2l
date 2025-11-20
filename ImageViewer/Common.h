#pragma once
#include <string>
#include <set>

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

void PurgeAccumulatedKeyPresses();

std::string GetCurrentPanelItem();

bool ShowImageAtFull(const std::string &initial_file, std::set<std::string> &selection);

void ShowImageAtQV();
void DismissImageAtQV();

