#pragma once
#include <vector>
#include "Globals.h"

struct GetItems : std::vector<PluginPanelItem>
{
	GetItems(bool only_selected);
	~GetItems();

protected:
	std::vector<PluginPanelItem *> _items_to_free;
};


class GetFocusedItem
{
	PluginPanelItem *_ptr = nullptr;

	GetFocusedItem(const GetFocusedItem&) = delete;

public:
	GetFocusedItem(HANDLE plug);
	~GetFocusedItem();


	inline bool IsValid() const
	{
		return _ptr != nullptr;
	}

	inline PluginPanelItem *operator ->()
	{
		return _ptr;
	}

};
