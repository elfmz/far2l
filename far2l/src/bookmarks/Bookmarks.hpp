#pragma once
#include <KeyFileHelper.h>
#include "FARString.hpp"

void CheckForImportLegacyShortcuts();

class Bookmarks
{
	KeyFileHelper _kfh;

public:
	Bookmarks();

	bool Set(int index, const FARString *path, const FARString *plugin = nullptr,
		const FARString *plugin_file = nullptr, const FARString *plugin_data = nullptr);

	bool Get(int index, FARString *path, FARString *plugin = nullptr,
		FARString *plugin_file = nullptr, FARString *plugin_data = nullptr);

	bool Clear(int index);
};

void ShowBookmarksMenu(int Pos = 0);
