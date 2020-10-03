#pragma once

#include "pcolorer.h"
#include "FarEditor.h"
#include <colorer/parsers/FileTypeImpl.h>

class ChooseTypeMenu
{
public:
  ChooseTypeMenu(const wchar_t *AutoDetect,const wchar_t *Favorites);
  ~ChooseTypeMenu();
  FarMenuItemEx *getItems();
  size_t getItemsCount(){return ItemCount;};

  int AddItem(const FileType* fType, size_t PosAdd=0x7FFFFFFF);
  int AddItemInGroup(FileType* fType);
  int AddGroup(const wchar_t *Text);
  void SetSelected(size_t index);
  int GetNext(size_t index);
  FileType* GetFileType(size_t index);
  void MoveToFavorites(size_t index);
  int AddFavorite(const FileType* fType);
  void DeleteItem(size_t index);

  void HideEmptyGroup();
  void DelFromFavorites(size_t index);
  bool IsFavorite(size_t index);
  void RefreshItemCaption(size_t index);
  StringBuffer* GenerateName(const FileType* fType);

private:
  size_t ItemCount;
  FarMenuItemEx *Item;

  size_t ItemSelected; // Index of selected item 

  int AddItem(const wchar_t *Text, const MENUITEMFLAGS Flags, const FileType* UserData = nullptr, size_t PosAdd=0x7FFFFFFF);

  static const size_t favorite_idx=2;
};

