#pragma once
#include <vector>
#include "Globals.h"


struct GetSelectedItems : std::vector<PluginPanelItem>
{
	GetSelectedItems();
	~GetSelectedItems();

private:
	std::vector<PluginPanelItem *> _items_to_free;
};
