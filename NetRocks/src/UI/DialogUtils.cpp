#include "DialogUtils.h"
#include "../Globals.h"

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const char *data, const char *history, bool focus, bool def)
{
	int index = (int)size();
	resize(index + 1);
	auto &item = back();

	item.Type = type;
	item.X1 = x1;
	item.Y1 = y1;
	item.X2 = x2;
	item.Y2 = y2;
	item.Focus = focus ? 1 : 0;
	item.History = history;
	item.Flags = flags;
	item.DefaultButton = def ? 1 : 0;
	strncpy(item.Data, data ? data : "", sizeof(item.Data) );

	return index;
}


int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, int data_lng, const char *history, bool def, bool focus)
{
	return Add(type, x1, y1, x2, y2, flags, (data_lng != -1) ? G.GetMsg(data_lng) : nullptr, history, def, focus);
}

int FarDialogItems::EstimateWidth() const
{
	int min_x = 0, max_x = 0;
	for (const auto &item : *this) {
		if (min_x > item.X1 || &item == &front())
			min_x = item.X1;

		if (max_x < item.X1 || &item == &front())
			max_x = item.X1;

		if (max_x < item.X2)
			max_x = item.X2;
	}

	return max_x + 1 - min_x;
}

int FarDialogItems::EstimateHeight() const
{
	int min_y = 0, max_y = 0;
	for (const auto &item : *this) {
		if (min_y > item.Y1 || &item == &front())
			min_y = item.Y1;

		if (max_y < item.Y1 || &item == &front())
			max_y = item.Y1;

		if (max_y < item.Y2)
			max_y = item.Y2;
	}

	return max_y + 1 - min_y;
}

/////////////////

void FarListWrapper::Add(const char *text, DWORD flags)
{
	FarListItem fli = {flags, {}, {}};
	strncpy(fli.Text, text, sizeof(fli.Text));
	_items.emplace_back(fli);
	_list.ItemsNumber = _items.size();
	_list.Items = &_items[0];
}

bool FarListWrapper::Select(const char *text)
{
	bool out = false;
	for (auto &item : _items) {
		if (strncmp(item.Text, text, sizeof(item.Text) ) == 0) {
			item.Flags|= LIF_SELECTED;
			out = true;
		} else {
			item.Flags&= ~LIF_SELECTED;
		}
	}
	return out;
}

const char *FarListWrapper::GetSelection()
{
	for (auto &item : _items) {
		if (item.Flags)
			return item.Text;
	}
	return nullptr;
}
