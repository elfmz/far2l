#pragma once
#include <vector>
#include <windows.h>
#include <pluginold.hpp>
using namespace oldfar;


struct FarDialogItems : std::vector<struct FarDialogItem>
{
	int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags = 0, const char *data = nullptr, const char *history = nullptr, bool def = false, bool focus = false);
	int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, int data_lng, const char *history = nullptr, bool def = false, bool focus = false);

	int EstimateWidth() const;
	int EstimateHeight() const;
};

struct FarListWrapper
{
	void Add(const char *text, DWORD flags = 0);
	FarList *Get() { return &_list; }

	bool Select(const char *text);
	const char *GetSelection();

private:
	FarList _list{};
	std::vector<FarListItem> _items;
};
