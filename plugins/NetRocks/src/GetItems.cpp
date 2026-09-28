#include "GetItems.h"

GetItems::GetItems(bool only_selected)
{
	PanelInfo pi = {};
	G.info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)(void *)&pi);
	int n = only_selected ? pi.SelectedItemsNumber : pi.ItemsNumber;

	if (n <= 0) {
		fprintf(stderr, "GetItems(%d): empty\n", only_selected);
		return;
	}

	_items_to_free.reserve(n);
	reserve(n);

	int getitem_cmd = only_selected ? FCTL_GETSELECTEDPANELITEM : FCTL_GETPANELITEM;
	for (int i = 0; i < n; ++i) {
		size_t len = G.info.Control(PANEL_ACTIVE, getitem_cmd, i, 0);
		if (len >= sizeof(PluginPanelItem)) {
			PluginPanelItem *pi = (PluginPanelItem *)calloc(1, len + 0x20);
			if (pi == nullptr) {
				fprintf(stderr, "GetItems: no memory\n");
				break;
			}

			G.info.Control(PANEL_ACTIVE, getitem_cmd, i, (LONG_PTR)(void *)pi);
			_items_to_free.push_back(pi);
			if (pi->FindData.lpwszFileName) {
				push_back(*pi);
			}
		}
	}
}

GetItems::~GetItems()
{
	for (auto &pi : _items_to_free) {
		free(pi);
	}
}

///

GetFocusedItem::GetFocusedItem(HANDLE plug)
{
	intptr_t size = G.info.Control(plug, FCTL_GETSELECTEDPANELITEM, 0, 0);
	if (size < (intptr_t)sizeof(PluginPanelItem)) {
		return;
	}

	_ptr = (PluginPanelItem *) malloc(size + 0x100);
	G.info.Control(plug, FCTL_GETSELECTEDPANELITEM, 0, (LONG_PTR)(void *)_ptr);
}

GetFocusedItem::~GetFocusedItem()
{
	free(_ptr);
}
