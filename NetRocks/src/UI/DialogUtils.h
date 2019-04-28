#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include <pluginold.hpp>
using namespace oldfar;


enum FarDialogItemState
{
	FDIS_NORMAL,
	FDIS_DEFAULT,
	FDIS_FOCUSED,
	FDIS_DEFAULT_FOCUSED
};

struct FarDialogItems : std::vector<struct FarDialogItem>
{
	int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags = 0, const char *data = nullptr, const char *history = nullptr, FarDialogItemState state = FDIS_NORMAL);
	int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, int data_lng, const char *history = nullptr, FarDialogItemState state = FDIS_NORMAL);

	int EstimateWidth() const;
	int EstimateHeight() const;
};

struct FarDialogItemsLineGrouped : FarDialogItems
{
	void SetLine(int y);
	void NextLine();

	int AddAtLine(int type, int x1, int x2, unsigned int flags = 0, const char *data = nullptr, const char *history = nullptr, FarDialogItemState state = FDIS_NORMAL);
	int AddAtLine(int type, int x1, int x2, unsigned int flags, int data_lng, const char *history = nullptr, FarDialogItemState state = FDIS_NORMAL);

private:
	int _y = 1;
};

struct FarListWrapper
{
	void Add(const char *text, DWORD flags = 0);
	void Add(int text_lng, DWORD flags = 0);

	FarList *Get() { return &_list; }

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
protected:
	FarDialogItemsLineGrouped _di;

	static LONG_PTR SendDlgMessage(HANDLE dlg, int msg, int param1, LONG_PTR param2);
	static LONG_PTR WINAPI sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);
	virtual LONG_PTR DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);

	int Show(const char *title, int extra_width, int extra_height, unsigned int flags = 0);
	int Show(int title_lng, int extra_width, int extra_height, unsigned int flags = 0);

	void Close(HANDLE dlg);

	void TextFromDialogControl(HANDLE dlg, int ctl, std::string &str);
	void TextToDialogControl(HANDLE dlg, int ctl, const char *str);
	void TextToDialogControl(HANDLE dlg, int ctl, const std::string &str);
	void TextToDialogControl(HANDLE dlg, int ctl, int lng_str);
	void LongLongToDialogControl(HANDLE dlg, int ctl, long long value);
	void FileSizeToDialogControl(HANDLE dlg, int ctl, long long value);

	void ProgressBarToDialogControl(HANDLE dlg, int ctl, int percents = -1);

	void SetEnabledDialogControl(HANDLE dlg, int ctl, bool en = true);

public:
	virtual ~BaseDialog(){};
};

