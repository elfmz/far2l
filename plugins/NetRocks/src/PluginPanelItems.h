#pragma once
#include <string>
#include <exception>
#include <stdexcept>

#include <farplug-wide.h>

void PluginPanelItems_Free(PluginPanelItem *items, int count);

struct PluginPanelItems
{
	PluginPanelItem *items = nullptr;
	int count = 0;

	~PluginPanelItems();

	void Shrink(int new_count);

	void Detach();
	PluginPanelItem *Add(const wchar_t *name);
	PluginPanelItem *Add(const char *name);

private:
	int capacity = 0;
	std::wstring _tmp;
};
