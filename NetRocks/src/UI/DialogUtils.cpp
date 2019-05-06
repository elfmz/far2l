#include <utils.h>
#include "DialogUtils.h"
#include "../Globals.h"
#include "PooledStrings.h"

const wchar_t *FarDialogItems::MB2WidePooled(const char *sz)
{
	if (!sz)
		return nullptr;

	if (!*sz)
		return L"";

	MB2Wide(sz, _str_pool_tmp);
	return _str_pool.insert(_str_pool_tmp).first->c_str();
}

int FarDialogItems::AddInternal(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t *data, const wchar_t *history, FarDialogItemState state)
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
	item.PtrData = data;

//	strncpy(item.Data, data ? data : "", sizeof(item.Data) );

	return index;
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const char *data, const char *history, FarDialogItemState state)
{
	return AddInternal(type, x1, y1, x2, y2, flags, MB2WidePooled(data), MB2WidePooled(history), state);
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, int data_lng, const char *history, FarDialogItemState state)
{
	return AddInternal(type, x1, y1, x2, y2, flags, (data_lng != -1) ? G.GetMsgWide(data_lng) : nullptr, MB2WidePooled(history), state);
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
	fli.Text = MB2WidePooled(text);
	_items.emplace_back(fli);
	_list.ItemsNumber = _items.size();
	_list.Items = &_items[0];
}

void FarListWrapper::Add(int text_lng, DWORD flags)
{
	Add(G.GetMsgMB(text_lng), flags);
}

bool FarListWrapper::Select(const char *text)
{
	bool out = false;
	const std::wstring text_wide = MB2Wide(text);
	for (auto &item : _items) {
		if (item.Text && wcscmp(item.Text, text_wide.c_str()) == 0) {
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
			return PooledString(item.Text);
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
BaseDialog::~BaseDialog()
{
	if (_dlg != INVALID_HANDLE_VALUE) {
		G.info.DialogFree(_dlg);
		// _dlg = INVALID_HANDLE_VALUE;
	}
}

LONG_PTR BaseDialog::sSendDlgMessage(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	if (dlg == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "BaseDialog::sSendDlgMessage: invalid dlg, msg=0x%x param1=0x%x\n", msg, param1);
	}
	return G.info.SendDlgMessage(dlg, msg, param1, param2);
}

LONG_PTR BaseDialog::SendDlgMessage(int msg, int param1, LONG_PTR param2)
{
	return sSendDlgMessage(_dlg, msg, param1, param2);
}

LONG_PTR WINAPI BaseDialog::sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	BaseDialog *it = (BaseDialog *)sSendDlgMessage(dlg, DM_GETDLGDATA, 0, 0);
	if (it) {
		if (dlg == it->_dlg) {
			return it->DlgProc(msg, param1, param2);
		}

		fprintf(stderr, "BaseDialog::sDlgProc: wrong dlg: %p != %p\n", dlg, it->_dlg);
	}

	return G.info.DefDlgProc(dlg, msg, param1, param2);
}

LONG_PTR BaseDialog::DlgProc(int msg, int param1, LONG_PTR param2)
{
	return G.info.DefDlgProc(_dlg, msg, param1, param2);
}

int BaseDialog::Show(const wchar_t *help_topic, int extra_width, int extra_height, unsigned int flags)
{
	if (_dlg == INVALID_HANDLE_VALUE) {
		_dlg = G.info.DialogInit(G.info.ModuleNumber, -1, -1,
			_di.EstimateWidth() + extra_width, _di.EstimateHeight() + extra_height,
			help_topic, &_di[0], _di.size(), 0, flags, &sDlgProc, (LONG_PTR)(uintptr_t)this);
		if (_dlg == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}

	return G.info.DialogRun(_dlg);
}

int BaseDialog::Show(const char *help_topic, int extra_width, int extra_height, unsigned int flags)
{
	return Show(MB2Wide(help_topic).c_str(), extra_width, extra_height, flags);
//	fprintf(stderr, "[%ld] BaseDialog::Show: %p %d\n", time(NULL), &_di[0], _di.size());
}

void BaseDialog::Close(int code)
{
	SendDlgMessage(DM_CLOSE, code, 0);
}


void BaseDialog::TextFromDialogControl(int ctl, std::string &str)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	static wchar_t buf[ 0x1000 ] = {};
	FarDialogItemData dd = { ARRAYSIZE(buf) - 1, buf };
	LONG_PTR rv = SendDlgMessage(DM_GETTEXT, ctl, (LONG_PTR)&dd);
	if (rv > 0 && rv < (LONG_PTR)sizeof(buf))
		buf[rv] = 0;

	Wide2MB(buf, str);
}

void BaseDialog::TextToDialogControl(int ctl, const char *str)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	const std::wstring &tmp = MB2Wide(str);
	FarDialogItemData dd = { tmp.size(), (wchar_t*)tmp.c_str()};
	SendDlgMessage(DM_SETTEXT, ctl, (LONG_PTR)&dd);
}

void BaseDialog::TextToDialogControl(int ctl, const std::string &str)
{
	TextToDialogControl(ctl, str.c_str());
}

void BaseDialog::TextToDialogControl(int ctl, int lng_str)
{
	const char *str = G.GetMsgMB(lng_str);
	TextToDialogControl(ctl, str ? str : "");
}

void BaseDialog::LongLongToDialogControl(int ctl, long long value)
{
	char str[0x100] = {};
	snprintf(str, sizeof(str) - 1, "%lld", value);
	TextToDialogControl(ctl, str);
}

void BaseDialog::FileSizeToDialogControl(int ctl, unsigned long long value)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	TextToDialogControl(ctl, FileSizeString(value));
}

void BaseDialog::TimePeriodToDialogControl(int ctl, unsigned long long msec_ull)
{
//	unsigned long long msec_ull = msec.count();
	unsigned int hrs = (unsigned int)(msec_ull / 3600000ll);
	msec_ull-= 3600000ll * hrs;
	unsigned int mins = (unsigned int)(msec_ull / 60000ll);
	msec_ull -= 60000ll * mins;
	unsigned int secs = (unsigned int)(msec_ull / 1000ll);
	//msec-= 60000l * hrs;

	char str[0x100] = {};
	snprintf(str, sizeof(str) - 1, "%02u:%02u.%02u", hrs, mins, secs);
	TextToDialogControl(ctl, str);
}

void BaseDialog::ProgressBarToDialogControl(int ctl, int percents)
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
	TextToDialogControl(ctl, str);
}

void BaseDialog::SetEnabledDialogControl(int ctl, bool en)
{
	SendDlgMessage(DM_ENABLE, ctl, en ? TRUE : FALSE);
}

bool BaseDialog::IsCheckedDialogControl(int ctl)
{
	return (SendDlgMessage(DM_GETCHECK, ctl, 0) == BSTATE_CHECKED);
}
