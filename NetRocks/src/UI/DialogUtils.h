#pragma once
#include <vector>
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

struct FarListWrapper
{
	void Add(const char *text, DWORD flags = 0);
	void Add(int text_lng, DWORD flags = 0);

	FarList *Get() { return &_list; }

	bool Select(const char *text);
	const char *GetSelection();

private:
	FarList _list{};
	std::vector<FarListItem> _items;
};


class BaseDialog
{
protected:
	FarDialogItems _di;

	static LONG_PTR WINAPI sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);
	virtual LONG_PTR DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);

	int Show(int extra_width, int extra_height, const char *title);
	int Show(int extra_width, int extra_height, int title_lng);

public:
	virtual ~BaseDialog(){};
};

