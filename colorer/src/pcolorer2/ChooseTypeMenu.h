#ifndef FARCOLORER_CHOOSE_TYPE_MENU_H
#define FARCOLORER_CHOOSE_TYPE_MENU_H

#include <colorer/FileType.h>
#include "pcolorer.h"

class ChooseTypeMenu
{
 public:
  ChooseTypeMenu(const wchar_t* AutoDetect, const wchar_t* Favorites);
  ~ChooseTypeMenu();
  [[nodiscard]] FarMenuItemEx* getItems() const;
  [[nodiscard]] size_t getItemsCount() const { return ItemCount; }

  size_t AddItem(const FileType* fType, size_t PosAdd = 0x7FFFFFFF);
  size_t AddItemInGroup(FileType* fType);
  size_t AddGroup(const wchar_t* Text);
  void SetSelected(size_t index);
  [[nodiscard]] size_t GetNext(size_t index) const;
  [[nodiscard]] FileType* GetFileType(size_t index) const;
  void MoveToFavorites(size_t index);
  size_t AddFavorite(const FileType* fType);
  void DeleteItem(size_t index);

  void HideEmptyGroup() const;
  void DelFromFavorites(size_t index);
  [[nodiscard]] bool IsFavorite(size_t index) const;
  void RefreshItemCaption(size_t index);
  static UnicodeString* GenerateName(const FileType* fType);

 private:
  size_t ItemCount;
  FarMenuItemEx* Item;

  size_t ItemSelected;  // Index of selected item

  size_t AddItem(const wchar_t* Text, MENUITEMFLAGS Flags, const FileType* UserData = nullptr,
                 size_t PosAdd = 0x7FFFFFFF);

  static const size_t favorite_idx = 2;
};

#endif  // FARCOLORER_CHOOSE_TYPE_MENU_H
