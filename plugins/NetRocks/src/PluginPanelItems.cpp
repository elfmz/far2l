#include <limits>
#include <utils.h>
#include "PluginPanelItems.h"

static PluginPanelItem *PluginPanelItems_ReAlloc(PluginPanelItem *prev, int capacity)
{
	size_t alloc_size = ((size_t)capacity) * sizeof(PluginPanelItem);
	if (alloc_size < (size_t)capacity)
		return nullptr;

	return (PluginPanelItem *)realloc(prev, alloc_size);
}

void PluginPanelItems_Free(PluginPanelItem *items, int count)
{
	if (items != nullptr) {
		for (int i = 0; i < count; ++i) {
			free(items[i].FindData.lpwszFileName);
		}
		free(items);
	}
}

PluginPanelItems::~PluginPanelItems()
{
	PluginPanelItems_Free(items, count);
}


void PluginPanelItems::Detach()
{
	items = nullptr;
	capacity = count = 0;
}

void PluginPanelItems::Shrink(int new_count)
{
	ASSERT_MSG(new_count <= count,
		"new_count=%d while count=%d", new_count, count);

	for (int i = new_count; i < count; ++i) {
		free(items[i].FindData.lpwszFileName);
	}
	count = new_count;
}

PluginPanelItem *PluginPanelItems::Add(const wchar_t *name)
{
	if (count == capacity) {
		if (capacity == std::numeric_limits<int>::max() - 16)
			throw std::runtime_error("PluginPanelItems: capacity overflow");;

		int new_capacity = (capacity < (std::numeric_limits<int>::max() / 2) )
			? capacity + capacity/2 + 0x100 : capacity + 16;

		PluginPanelItem *new_items = PluginPanelItems_ReAlloc(items, new_capacity);
		if (new_items == nullptr) {
			new_capacity = capacity + 1;
			new_items = PluginPanelItems_ReAlloc(items, new_capacity);
			if (new_items == nullptr) {
				throw std::runtime_error("PluginPanelItems: can't allocate new items");
			}
		}

		items = new_items;
		capacity = new_capacity;
	}

	ASSERT_MSG(count <= capacity,
		"count=%d while capacity=%d", count, capacity);

	PluginPanelItem *out = &items[count];
	ZeroFill(*out);
	out->FindData.lpwszFileName = wcsdup(name);
	if (out->FindData.lpwszFileName == nullptr) {
		throw std::runtime_error("PluginPanelItems: can't allocate filename");
	}

	++count;
	return out;
}

PluginPanelItem *PluginPanelItems::Add(const char *name)
{
	MB2Wide(name, _tmp);
	return Add(_tmp.c_str());
}
