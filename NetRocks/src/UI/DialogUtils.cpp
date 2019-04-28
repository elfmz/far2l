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

void FarDialogItemsLineGrouped::SetLine(int y)
{
	_y = y;
}

void FarDialogItemsLineGrouped::NextLine()
{
	++_y;
}

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, const char *data, const char *history, FarDialogItemState state)
{
	return Add(type, x1, _y, x2, _y, flags, data, history, state);
}

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, int data_lng, const char *history, FarDialogItemState state)
{
	return Add(type, x1, _y, x2, _y, flags, data_lng, history, state);
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

const char *FarListWrapper::GetSelection() const
{
	for (auto &item : _items) {
		if (item.Flags & LIF_SELECTED)
			return item.Text;
	}
	return nullptr;
}

bool FarListWrapper::SelectIndex(ssize_t index)
{
	for (size_t i = 0; i < _items.size(); ++i) {
		auto &item = _items[i];
		if ((ssize_t)i == index) {
			item.Flags|= LIF_SELECTED;
		} else {
			item.Flags&= ~LIF_SELECTED;
		}
	}
	return (index >= 0 && (size_t)index < _items.size());
}

ssize_t FarListWrapper::GetSelectionIndex() const
{
	for (size_t i = 0; i < _items.size(); ++i) {
		const auto &item = _items[i];
		if (item.Flags & LIF_SELECTED)
			return i;
	}
	return -1;
}


///////////////////////
LONG_PTR BaseDialog::SendDlgMessage(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	return G.info.SendDlgMessage(dlg, msg, param1, param2);
}

LONG_PTR WINAPI BaseDialog::sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	BaseDialog *it = (BaseDialog *)SendDlgMessage(dlg, DM_GETDLGDATA, 0, 0);
	if (it) {
		return it->DlgProc(dlg, msg, param1, param2);
	}

	return G.info.DefDlgProc(dlg, msg, param1, param2);
}

LONG_PTR BaseDialog::DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	return G.info.DefDlgProc(dlg, msg, param1, param2);
}

int BaseDialog::Show(const char *title, int extra_width, int extra_height, unsigned int flags)
{
//	fprintf(stderr, "[%ld] BaseDialog::Show: %p %d\n", time(NULL), &_di[0], _di.size());
	return G.info.DialogEx(G.info.ModuleNumber, -1, -1,
		_di.EstimateWidth() + extra_width, _di.EstimateHeight() + extra_height,
		title, &_di[0], _di.size(), 0, flags, &sDlgProc, (LONG_PTR)(uintptr_t)this);
}

int BaseDialog::Show(int title_lng, int extra_width, int extra_height, unsigned int flags)
{
	return Show(G.GetMsg(title_lng), extra_width, extra_height, flags);
}

void BaseDialog::Close(HANDLE dlg, int code)
{
	SendDlgMessage(dlg, DM_CLOSE, code, 0);
}


void BaseDialog::TextFromDialogControl(HANDLE dlg, int ctl, std::string &str)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	static char buf[ 0x1000 ] = {};
	FarDialogItemData dd = { sizeof(buf) - 1, buf };
	LONG_PTR rv = SendDlgMessage(dlg, DM_GETTEXT, ctl, (LONG_PTR)&dd);
	if (rv > 0 && rv < (LONG_PTR)sizeof(buf))
		buf[rv] = 0;
	str = buf;
}

void BaseDialog::TextToDialogControl(HANDLE dlg, int ctl, const char *str)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	FarDialogItemData dd = { (int)strlen(str), (char*)str };
	SendDlgMessage(dlg, DM_SETTEXT, ctl, (LONG_PTR)&dd);
}

void BaseDialog::TextToDialogControl(HANDLE dlg, int ctl, const std::string &str)
{
	TextToDialogControl(dlg, ctl, str.c_str());
}

void BaseDialog::TextToDialogControl(HANDLE dlg, int ctl, int lng_str)
{
	const char *str = G.GetMsg(lng_str);
	TextToDialogControl(dlg, ctl, str ? str : "");
}

void BaseDialog::LongLongToDialogControl(HANDLE dlg, int ctl, long long value)
{
	char str[0x100] = {};
	snprintf(str, sizeof(str) - 1, "%lld", value);
	TextToDialogControl(dlg, ctl, str);
}

const char *BaseDialog::FileSizeToFractionAndUnits(unsigned long long &value)
{
	if (value > 100ll * 1024ll * 1024ll * 1024ll * 1024ll) {
		value = (1024ll * 1024ll * 1024ll * 1024ll);
		return "TB";
	}

	if (value > 100ll * 1024ll * 1024ll * 1024ll) {
		value = (1024ll * 1024ll * 1024ll);
		return "GB";
	}

	if (value > 100ll * 1024ll * 1024ll ) {
		value = (1024ll * 1024ll);
		return "MB";

	}

	if (value > 100ll * 1024ll ) {
		value = (1024ll);
		return "KB";
	}

	value = 1;
	return "B";
}

void BaseDialog::FileSizeToDialogControl(HANDLE dlg, int ctl, unsigned long long value)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	unsigned long long fraction = value;
	const char *units = FileSizeToFractionAndUnits(fraction);
	value/= fraction;

	char str[0x100] = {};
	snprintf(str, sizeof(str) - 1, "%lld %s", value, units);
	TextToDialogControl(dlg, ctl, str);
}

void BaseDialog::TimePeriodToDialogControl(HANDLE dlg, int ctl, unsigned long long msec_ull)
{
//	unsigned long long msec_ull = msec.count();
	unsigned int hrs = (unsigned int)(msec_ull / 3600000ll);
	msec_ull-= 3600000ll * hrs;
	unsigned int mins = (unsigned int)(msec_ull / 60000ll);
	msec_ull -= 60000ll * hrs;
	unsigned int secs = (unsigned int)(msec_ull / 1000ll);
	//msec-= 60000l * hrs;

	char str[0x100] = {};
	snprintf(str, sizeof(str) - 1, "%02u:%02u.%02u", hrs, mins, secs);
	TextToDialogControl(dlg, ctl, str);
}

void BaseDialog::ProgressBarToDialogControl(HANDLE dlg, int ctl, int percents)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	std::string str;
	int width = _di[ctl].X2 + 1 - _di[ctl].X1;
	str.resize(width);
	if (percents >= 0) {
		int filled = (percents * width) / 100;
		for (int i = 0; i < filled; ++i) {
			str[i] = '#';
		}
		for (int i = filled; i < width; ++i) {
			str[i] = '=';
		}
	} else {
		for (auto &c : str) {
			c = '-';
		}
	}
	TextToDialogControl(dlg, ctl, str);
}

void BaseDialog::SetEnabledDialogControl(HANDLE dlg, int ctl, bool en)
{
	SendDlgMessage(dlg, DM_ENABLE, ctl, en ? TRUE : FALSE);
}
