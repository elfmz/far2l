#include "GetSelectedItems.h"


GetSelectedItems::GetSelectedItems()
{
	PanelInfo pi = {};
	G.info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)(void *)&pi);
	if (pi.SelectedItemsNumber == 0) {
		fprintf(stderr, "GetSelectedItems: no files selected\n");
		return;
	}

	_items_to_free.reserve(pi.SelectedItemsNumber);
	reserve(pi.SelectedItemsNumber);
	for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
		size_t len = G.info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, 0);
		if (len >= sizeof(PluginPanelItem)) {
			PluginPanelItem *pi = (PluginPanelItem *)calloc(1, len + 0x20);
			if (pi == nullptr) {
				fprintf(stderr, "GetSelectedItems: no memory\n");
				break;
			}

			G.info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)(void *)pi);
			_items_to_free.push_back(pi);
			if (pi->FindData.lpwszFileName) {
				push_back(*pi);
			}
		}
	}
}


GetSelectedItems::~GetSelectedItems()
{
	for (auto &pi : _items_to_free) {
		free(pi);
	}
}
