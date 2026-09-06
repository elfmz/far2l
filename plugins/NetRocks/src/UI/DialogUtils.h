#pragma once
#include <vector>
#include <string>
#include <set>
#include <chrono>
#include <windows.h>
#include <farplug-wide.h>


struct FarDialogItems : std::vector<struct FarDialogItem>
{
	FarDialogItems();

	int SetBoxTitleItem(const char *title);
	int SetBoxTitleItem(int title_lng);

	int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags = 0, const char *data = nullptr, const char *history = nullptr);
	int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, int data_lng, const char *history = nullptr);
	int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t *data, const char *history = nullptr);

	int EstimateWidth() const;
	int EstimateHeight() const;

	// use local strings pool to do not pollute global one
	const wchar_t *MB2WidePooled(const char *sz);
	const wchar_t *WidePooled(const wchar_t *wz);

private:

	int AddInternal(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t *data, const wchar_t *history);

	std::set<std::wstring> _str_pool;
	std::wstring _str_pool_tmp;
};

struct FarDialogItemsLineGrouped : FarDialogItems
{
	void SetLine(int y);
	void NextLine();

	int AddAtLine(int type, int x1, int x2, unsigned int flags = 0, const char *data = nullptr, const char *history = nullptr);
	int AddAtLine(int type, int x1, int x2, unsigned int flags, int data_lng, const char *history = nullptr);
	int AddAtLine(int type, int x1, int x2, unsigned int flags, const wchar_t *data, const char *history = nullptr);

private:
	int _y = 1;
};

struct FarListWrapper
{
	void Add(const char *text, DWORD flags = 0);
	void Add(int text_lng, DWORD flags = 0);

	FarList *Get() { return &_list; }
	FarListItem *At(size_t i) { return &_items[i]; }

	bool Select(const char *text);
	const char *GetSelection() const;

	bool SelectIndex(ssize_t index = -1);
	ssize_t GetSelectionIndex() const;

private:
	FarList _list{};
	std::vector<FarListItem> _items;
};


class BaseDialog
{
	HANDLE _dlg = INVALID_HANDLE_VALUE;
	wchar_t _progress_bg = 0, _progress_fg = 0;

protected:
	FarDialogItemsLineGrouped _di;

	static LONG_PTR sSendDlgMessage(HANDLE dlg, int msg, int param1, LONG_PTR param2);
	static LONG_PTR WINAPI sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);

	LONG_PTR SendDlgMessage(int msg, int param1, LONG_PTR param2);
	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2);

	int Show(const wchar_t *help_topic, int extra_width, int extra_height, unsigned int flags = 0);
	int Show(const char *help_topic, int extra_width, int extra_height, unsigned int flags = 0);

	void Close(int code = -1);

	void SetDefaultDialogControl(int ctl = -1);
	void SetFocusedDialogControl(int ctl = -1);

	void SetEnabledDialogControl(int ctl, bool en = true);
	void SetVisibleDialogControl(int ctl, bool vis = true);

	void Set3StateDialogControl(int ctl, int state);
	void SetCheckedDialogControl(int ctl, bool checked = true);

	int Get3StateDialogControl(int ctl);
	bool IsCheckedDialogControl(int ctl);

	void TextFromDialogControl(int ctl, std::wstring &wstr);
	void TextFromDialogControl(int ctl, std::string &str);
	long long LongLongFromDialogControl(int ctl);

	void TextToDialogControl(int ctl, const std::wstring &str);
	void TextToDialogControl(int ctl, const char *str);
	void TextToDialogControl(int ctl, const std::string &str);
	void TextToDialogControl(int ctl, int lng_str);
	void AbbreviableTextToDialogControl(int ctl, std::string str);
	void LongLongToDialogControl(int ctl, long long value);
	void LongLongToDialogControlThSeparated(int ctl, long long value);

	void FileSizeToDialogControl(int ctl, unsigned long long value);

	bool DateTimeToDialogControl(int ctl, const FILETIME *ft);

	void TimePeriodToDialogControl(int ctl, unsigned long long msec);

	void ProgressBarToDialogControl(int ctl, int percents = -1);

	int GetDialogListPosition(int ctl);
	void SetDialogListPosition(int ctl, int pos);

public:
	virtual ~BaseDialog();
};

