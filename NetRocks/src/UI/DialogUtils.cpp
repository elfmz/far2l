#include "DialogUtils.h"
#include "../Globals.h"

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const char *data, const char *history, FarDialogItemState state)
{
	int index = (int)size();
	resize(index + 1);
	auto &item = back();

	item.Type = type;
	item.X1 = x1;
	item.Y1 = y1;
	item.X2 = x2;
	item.Y2 = y2;
	item.Focus = (state == FDIS_FOCUSED || state == FDIS_DEFAULT_FOCUSED);
	item.History = history;
	item.Flags = flags;
	item.DefaultButton = (state == FDIS_DEFAULT || state == FDIS_DEFAULT_FOCUSED);
	strncpy(item.Data, data ? data : "", sizeof(item.Data) );

	return index;
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, int data_lng, const char *history, FarDialogItemState state)
{
	return Add(type, x1, y1, x2, y2, flags, (data_lng != -1) ? G.GetMsg(data_lng) : nullptr, history, state);
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

void FarListWrapper::Add(int text_lng, DWORD flags)
{
	Add(G.GetMsg(text_lng), flags);
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


///////////////////////
LONG_PTR WINAPI BaseDialog::sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	BaseDialog *it = (BaseDialog *)G.info.SendDlgMessage(dlg, DM_GETDLGDATA, 0, 0);
	if (it) {
		return it->DlgProc(dlg, msg, param1, param2);
	}

	return G.info.DefDlgProc(dlg, msg, param1, param2);
}

LONG_PTR BaseDialog::DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	return G.info.DefDlgProc(dlg, msg, param1, param2);
}

int BaseDialog::Show(int extra_width, int extra_height, const char *title)
{
	return G.info.DialogEx(G.info.ModuleNumber, -1, -1,
		_di.EstimateWidth() + extra_width, _di.EstimateHeight() + extra_height,
		title, &_di[0], _di.size(), 0, 0, &sDlgProc, (LONG_PTR)(uintptr_t)this);
}

int BaseDialog::Show(int extra_width, int extra_height, int title_lng)
{
	return Show(extra_width, extra_height, G.GetMsg(title_lng));
}
