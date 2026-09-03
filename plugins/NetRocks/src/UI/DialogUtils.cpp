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
	return _str_pool.emplace(_str_pool_tmp).first->c_str();
}

const wchar_t *FarDialogItems::WidePooled(const wchar_t *wz)
{
	if (!wz)
		return nullptr;

	if (!*wz)
		return L"";


	return _str_pool.emplace(wz).first->c_str();
}

int FarDialogItems::AddInternal(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t *data, const wchar_t *history)
{
	int index = (int)size();
	resize(index + 1);
	auto &item = back();

	item.Type = type;
	item.X1 = x1;
	item.Y1 = y1;
	item.X2 = x2;
	item.Y2 = y2;
	item.Focus = 0;
	item.History = history;
	item.Flags = flags;
	item.DefaultButton = 0;
	item.PtrData = data;

//	strncpy(item.Data, data ? data : "", sizeof(item.Data) );

	if (index > 0) {
		auto &box = operator[](0);
		if (box.Y2 < y2 + 1) {
			box.Y2 = y2 + 1;
		}
		if (box.X2 < x2 + 2) {
			box.X2 = x2 + 2;
		}
	}

	return index;
}


FarDialogItems::FarDialogItems()
{
	AddInternal(DI_DOUBLEBOX, 3, 1, 5, 3, 0, L"", nullptr);
}

int FarDialogItems::SetBoxTitleItem(const char *title)
{
	if (empty()) {
		return -1;
	}

	operator[](0).PtrData = MB2WidePooled(title);
	return 0;
}

int FarDialogItems::SetBoxTitleItem(int title_lng)
{
	if (empty()) {
		return -1;
	}

	operator[](0).PtrData = G.GetMsgWide(title_lng);
	return 0;
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t *data, const char *history)
{
	return AddInternal(type, x1, y1, x2, y2, flags, WidePooled(data), MB2WidePooled(history));
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const char *data, const char *history)
{
	return AddInternal(type, x1, y1, x2, y2, flags, MB2WidePooled(data), MB2WidePooled(history));
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, int data_lng, const char *history)
{
	return AddInternal(type, x1, y1, x2, y2, flags, (data_lng != -1) ? G.GetMsgWide(data_lng) : nullptr, MB2WidePooled(history));
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

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, const char *data, const char *history)
{
	return Add(type, x1, _y, x2, _y, flags, data, history);
}

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, int data_lng, const char *history)
{
	return Add(type, x1, _y, x2, _y, flags, data_lng, history);
}

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, const wchar_t *data, const char *history)
{
	return Add(type, x1, _y, x2, _y, flags, data, history);
}

/////////////////


void FarListWrapper::Add(const char *text, DWORD flags)
{
	FarListItem fli = {flags, nullptr, {}};
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
			return -2;
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

void BaseDialog::SetDefaultDialogControl(int ctl)
{
	ASSERT(_dlg == INVALID_HANDLE_VALUE);

	if (ctl == -1) {
		if (!_di.empty()) {
			SetDefaultDialogControl((int)(_di.size() - 1));
		}
		return;
	}

	for (size_t i = 0; i < _di.size(); ++i) {
		_di[i].DefaultButton = ((int)i == ctl) ? 1 : 0;

	}
}

void BaseDialog::SetFocusedDialogControl(int ctl)
{
	ASSERT(_dlg == INVALID_HANDLE_VALUE);

	if (ctl == -1) {
		if (!_di.empty()) {
			SetFocusedDialogControl((int)(_di.size() - 1));
		}
		return;
	}

	for (size_t i = 0; i < _di.size(); ++i) {
		_di[i].Focus = ((int)i == ctl) ? 1 : 0;

	}
}

void BaseDialog::TextFromDialogControl(int ctl, std::wstring &wstr)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	if (_dlg == INVALID_HANDLE_VALUE) {
		if (_di[ctl].PtrData) {
			wstr = _di[ctl].PtrData;
		} else {
			wstr.clear();
		}
		return;
	}

	wstr.resize(0x1000);
	FarDialogItemData dd = { wstr.size() - 1, &wstr[0] };
	LONG_PTR rv = SendDlgMessage(DM_GETTEXT, ctl, (LONG_PTR)&dd);
	if (rv <= 0) {
		wstr.clear();

	} else if ((size_t)rv < wstr.size()) {
		wstr.resize((size_t)rv);
	}
}

void BaseDialog::TextFromDialogControl(int ctl, std::string &str)
{
	std::wstring wstr;
	TextFromDialogControl(ctl, wstr);
	StrWide2MB(wstr, str);
}

long long BaseDialog::LongLongFromDialogControl(int ctl)
{
	std::string str;
	TextFromDialogControl(ctl, str);
	const char *p = str.c_str();
	while (*p == ' ') {
		++p;
	}
	return atoll(p);
}

void BaseDialog::SetEnabledDialogControl(int ctl, bool en)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	SendDlgMessage(DM_ENABLE, ctl, en ? TRUE : FALSE);
}

void BaseDialog::SetVisibleDialogControl(int ctl, bool vis)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	SendDlgMessage(DM_SHOWITEM, ctl, vis ? TRUE : FALSE);
}

int BaseDialog::Get3StateDialogControl(int ctl)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return BSTATE_UNCHECKED;

	if (_dlg == INVALID_HANDLE_VALUE)
		return _di[ctl].Selected;

	return SendDlgMessage(DM_GETCHECK, ctl, 0);
}

bool BaseDialog::IsCheckedDialogControl(int ctl)
{
	return (Get3StateDialogControl(ctl) == BSTATE_CHECKED);
}

void BaseDialog::Set3StateDialogControl(int ctl, int state)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	if (_dlg == INVALID_HANDLE_VALUE) {
		_di[ctl].Selected = state;
		return;
	}

	SendDlgMessage(DM_SETCHECK, ctl, state);
}

void BaseDialog::SetCheckedDialogControl(int ctl, bool checked)
{
	Set3StateDialogControl(ctl, checked ? BSTATE_CHECKED : BSTATE_UNCHECKED);
}

void BaseDialog::TextToDialogControl(int ctl, const std::wstring &str)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	if (_dlg == INVALID_HANDLE_VALUE) {
		_di[ctl].PtrData = _di.WidePooled(str.c_str());
		return;
	}

	FarDialogItemData dd = { str.size(), (wchar_t*)str.c_str() };
	SendDlgMessage(DM_SETTEXT, ctl, (LONG_PTR)&dd);
}

void BaseDialog::TextToDialogControl(int ctl, const char *str)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	if (_dlg == INVALID_HANDLE_VALUE) {
		_di[ctl].PtrData = _di.MB2WidePooled(str);
		return;
	}

	std::wstring tmp;
	MB2Wide(str, tmp);
	FarDialogItemData dd = { tmp.size(), (wchar_t*)tmp.c_str() };
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

void BaseDialog::AbbreviableTextToDialogControl(int ctl, std::string str)
{
	if (ctl >= 0 && (size_t)ctl < _di.size() && _di[ctl].X2 > _di[ctl].X1) {
		size_t max_width = _di[ctl].X2 + 1 - _di[ctl].X1;
		AbbreviateString(str, max_width);
	}

	TextToDialogControl(ctl, str);
}

void BaseDialog::LongLongToDialogControl(int ctl, long long value)
{
	TextToDialogControl(ctl, ToDec(value));
}

void BaseDialog::LongLongToDialogControlThSeparated(int ctl, long long value)
{
	TextToDialogControl(ctl, ThousandSeparatedString(value));
}

void BaseDialog::FileSizeToDialogControl(int ctl, unsigned long long value)
{
	if (ctl < 0 || (size_t)ctl >= _di.size())
		return;

	TextToDialogControl(ctl, FileSizeString(value));
}

bool BaseDialog::DateTimeToDialogControl(int ctl, const FILETIME *ft)
{
	if (!ft->dwHighDateTime) {
		//TextToDialogControl(ctl, "");
		return false;
	}

	FILETIME ct;
	SYSTEMTIME st;
	WINPORT(FileTimeToLocalFileTime)(ft, &ct);
	WINPORT(FileTimeToSystemTime)(&ct, &st);

	char str[0x100] = {};
	snprintf(str, sizeof(str) - 1, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	TextToDialogControl(ctl, str);

	return true;
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

	if (_progress_bg == 0) {
		if (G.fsf.BoxSymbols) {
			_progress_bg = G.fsf.BoxSymbols[BS_X_B0];
		}
		if (_progress_bg == 0) {
			_progress_bg = '=';
		}
	}
	if (_progress_fg == 0) {
		if (G.fsf.BoxSymbols) {
			_progress_fg = G.fsf.BoxSymbols[BS_X_DB];
		}
		if (_progress_fg == 0) {
			_progress_fg = '#';
		}
	}


	std::wstring str;
	int width = _di[ctl].X2 + 1 - _di[ctl].X1;
	str.resize(width);
	if (percents >= 0) {
		int filled = (percents * width) / 100;
		for (int i = 0; i < filled; ++i) {
			str[i] = _progress_fg;
		}
		for (int i = filled; i < width; ++i) {
			str[i] = _progress_bg;
		}
	} else {
		for (auto &c : str) {
			c = '-';
		}
	}
	TextToDialogControl(ctl, str);
}


int BaseDialog::GetDialogListPosition(int ctl)
{
	if (_dlg == INVALID_HANDLE_VALUE) {
		const auto *items = _di[ctl].ListItems;
		for (int i = 0; i < items->ItemsNumber; ++i) {
			if (items->Items[i].Flags & LIF_SELECTED) {
				return i;
			}
		}
		return -1;
	}

	return SendDlgMessage(DM_LISTGETCURPOS, ctl, 0);
}

void BaseDialog::SetDialogListPosition(int ctl, int pos)
{
	if (_dlg == INVALID_HANDLE_VALUE) {
		auto *items = _di[ctl].ListItems;
		for (int i = 0; i < items->ItemsNumber; ++i) {
			if (i == pos) {
				items->Items[i].Flags|= LIF_SELECTED;
			} else
				items->Items[i].Flags&= ~LIF_SELECTED;
		}
		return;
	}

//TODO:	SendDlgMessage(DM_LISTGETCURPOS, ctl, 0);
}
