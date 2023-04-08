#include <limits>
#include "headers.hpp"
#include "filelist.hpp"

ListDataVec::ListDataVec() {}

ListDataVec::~ListDataVec()
{
	Clear();
}

void ListDataVec::Clear()
{
	for (auto &pItem : *this) {
		delete pItem;	//!!! see ~FileListItem
	}
	clear();
	shrink_to_fit();
}

FileListItem *ListDataVec::Add()
{
	if (size() + 1 >= (size_t)std::numeric_limits<int>::max()) {
		// many other places uses int, so need hardlimit here for now
		fprintf(stderr, "ListDataVec::Add: too many items\n");
		return nullptr;
	}

	FileListItem *item = nullptr;
	try {
		item = new FileListItem;
		item->Position = (int)size();
		emplace_back(item);

	} catch (std::exception &e) {
		fprintf(stderr, "ListDataVec::Add: %s\n", e.what());
		delete item;
		return nullptr;
	}

	return item;
}

FileListItem *ListDataVec::AddParentPoint()
{
	FileListItem *item = Add();
	if (item) {
		item->FileAttr = FILE_ATTRIBUTE_DIRECTORY;
		item->FileMode = S_IFDIR | S_IWUSR | S_IRUSR | S_IXUSR | S_IXGRP | S_IXOTH;
		item->strName = L"..";
	}
	return item;
}

FileListItem *ListDataVec::AddParentPoint(const FILETIME *Times, FARString Owner, FARString Group)
{
	FileListItem *item = AddParentPoint();
	if (item) {
		item->CreationTime = Times[0];
		item->AccessTime = Times[1];
		item->WriteTime = Times[2];
		item->ChangeTime = Times[3];
		item->strOwner = Owner;
		item->strGroup = Group;
	}
	return item;
}

////////////////////////////////////////////////////////////////////////////

FileListItem::FileListItem() {}

FileListItem::~FileListItem()
{
	if (CustomColumnNumber > 0 && CustomColumnData) {
		for (int J = 0; J < CustomColumnNumber; J++)
			delete[] CustomColumnData[J];

		delete[] CustomColumnData;
	}

	if (UserFlags & PPIF_USERDATA)
		free((void *)UserData);

	if (DeleteDiz)
		delete[] DizText;
}

///////////////////////////////////////////////////////////////////////////////

PluginPanelItemVec::PluginPanelItemVec() {}

PluginPanelItemVec::~PluginPanelItemVec()
{
	Clear();
}

void PluginPanelItemVec::Add(FileListItem *CreateFrom)
{
	emplace_back();
	FileList::FileListToPluginItem(CreateFrom, &back());
}

void PluginPanelItemVec::Clear()
{
	for (auto &Item : *this) {
		FileList::FreePluginPanelItem(&Item);
	}
	clear();
}
