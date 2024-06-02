#pragma once

/*
vmenu.hpp

Обычное вертикальное меню
	а так же:
		* список в DI_COMBOBOX
		* список в DI_LISTBOX
		* ...
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "modal.hpp"
#include <farplug-wide.h>
#include "manager.hpp"
#include "frame.hpp"
#include "bitflags.hpp"
#include "CriticalSections.hpp"

// Цветовые атрибуты - индексы в массиве цветов
enum
{
	VMenuColorBody           = 0,		// подложка
	VMenuColorBox            = 1,		// рамка
	VMenuColorTitle          = 2,		// заголовок - верхний и нижний
	VMenuColorText           = 3,		// Текст пункта
	VMenuColorHilite         = 4,		// HotKey
	VMenuColorSeparator      = 5,		// separator
	VMenuColorSelected       = 6,		// Выбранный
	VMenuColorHSelect        = 7,		// Выбранный - HotKey
	VMenuColorScrollBar      = 8,		// ScrollBar
	VMenuColorDisabled       = 9,		// Disabled
	VMenuColorArrows         = 10,		// '<' & '>' обычные
	VMenuColorArrowsSelect   = 11,		// '<' & '>' выбранные
	VMenuColorArrowsDisabled = 12,		// '<' & '>' Disabled
	VMenuColorGrayed         = 13,		// "серый"
	VMenuColorSelGrayed      = 14,		// выбранный "серый"

	VMENU_COLOR_COUNT,					// всегда последняя - размерность массива
};

enum VMENU_FLAGS
{
	VMENU_ALWAYSSCROLLBAR       = 0x00000100,		// всегда показывать скроллбар
	VMENU_LISTBOX               = 0x00000200,		// Это список в диалоге
	VMENU_SHOWNOBOX             = 0x00000400,		// показать без рамки
	VMENU_AUTOHIGHLIGHT         = 0x00000800,		// автоматически выбирать симолы подсветки
	VMENU_REVERSEHIGHLIGHT      = 0x00001000,		// ... только с конца
	VMENU_UPDATEREQUIRED        = 0x00002000,		// лист необходимо обновить (перерисовать)
	VMENU_DISABLEDRAWBACKGROUND = 0x00004000,		// подложку не рисовать
	VMENU_WRAPMODE              = 0x00008000,		// зацикленный список (при перемещении)
	VMENU_SHOWAMPERSAND         = 0x00010000,		// символ '&' показывать AS IS
	VMENU_WARNDIALOG = 0x00020000,					//
	VMENU_NOTCENTER  = 0x00040000,					// не цитровать
	VMENU_LEFTMOST   = 0x00080000,					// "крайний слева" - нарисовать на 5 позиций вправо от центра (X1 => (ScrX+1)/2+5)
	VMENU_NOTCHANGE    = 0x00100000,				//
	VMENU_LISTHASFOCUS = 0x00200000,				// меню является списком в диалоге и имеет фокус
	VMENU_COMBOBOX     = 0x00400000,				// меню является комбобоксом и обрабатывается менеджером по-особому.
	VMENU_MOUSEDOWN          = 0x00800000,			//
	VMENU_CHANGECONSOLETITLE = 0x01000000,			//
	VMENU_MOUSEREACTION      = 0x02000000,			// реагировать на движение мыши? (перемещать позицию при перемещении курсора мыши?)
	VMENU_DISABLED = 0x04000000,					//
	VMENU_IGNORE_SINGLECLICK = 0x08000000,			// по щелчку не ENTER, а только выбор строки (полезно при снятом VMENU_MOUSEREACTION)
};

class Dialog;
class SaveScreen;

struct MenuItemEx
{
	DWORD Flags;	// Флаги пункта
	DWORD AccelKey;
	FARString strName;

	union							// Пользовательские данные:
	{
		char *UserData;				// - указатель!
		char Str4[sizeof(char *)];	// - strlen(строка)+1 <= sizeof(char*)
	};

	int UserDataSize;				// Размер пользовательских данных
	int PrefixLen;	// Length of 'grayed' unimportant prefix
	int ShowPos;
	short AmpPos;	// Позиция автоназначенной подсветки
	bool FilteredOut;

	DWORD SetCheck(uint32_t Value)
	{
		if (Value) {
			Flags|= LIF_CHECKED;
			Flags&= ~0xFFFF;

			if (Value != 1)
				Flags|= Value & 0xFFFF;
		} else {
			Flags&= ~(0xFFFF | LIF_CHECKED);
		}

		return Flags;
	}

	DWORD SetSelect(int Value)
	{
		if (Value)
			Flags|= LIF_SELECTED;
		else
			Flags&= ~LIF_SELECTED;
		return Flags;
	}
	DWORD SetDisable(int Value)
	{
		if (Value)
			Flags|= LIF_DISABLE;
		else
			Flags&= ~LIF_DISABLE;
		return Flags;
	}

	void Clear()
	{
		Flags = 0;
		AccelKey = 0;
		strName.Clear();
		FilteredOut = false;
		UserDataSize = 0;
		UserData = nullptr;
		AmpPos = 0;
		PrefixLen = 0;
		ShowPos = 0;
	}

	MenuItemEx() { Clear(); }

	MenuItemEx(const MenuItemEx &srcMenu)
	{
		Clear();
		operator=(srcMenu);
	}

	// UserData не копируется.
	const MenuItemEx &operator=(const MenuItemEx &srcMenu)
	{
		if (this != &srcMenu) {
			Flags = srcMenu.Flags;
			strName = srcMenu.strName;
			AccelKey = srcMenu.AccelKey;
			UserDataSize = 0;
			UserData = nullptr;
			AmpPos = srcMenu.AmpPos;
			PrefixLen = srcMenu.PrefixLen;
			ShowPos = srcMenu.ShowPos;
		}

		return *this;
	}
};

struct MenuDataEx
{
	const wchar_t *Name;

	DWORD Flags;
	DWORD AccelKey;

	DWORD SetCheck(uint32_t Value)
	{
		if (Value) {
			Flags&= ~0xFFFF;
			Flags|= ((Value & 0xFFFF) | LIF_CHECKED);
		} else
			Flags&= ~(0xFFFF | LIF_CHECKED);

		return Flags;
	}

	DWORD SetSelect(int Value)
	{
		if (Value)
			Flags|= LIF_SELECTED;
		else
			Flags&= ~LIF_SELECTED;
		return Flags;
	}
	DWORD SetDisable(int Value)
	{
		if (Value)
			Flags|= LIF_DISABLE;
		else
			Flags&= ~LIF_DISABLE;
		return Flags;
	}
	DWORD SetGrayed(int Value)
	{
		if (Value)
			Flags|= LIF_GRAYED;
		else
			Flags&= ~LIF_GRAYED;
		return Flags;
	}
};

class ConsoleTitle;

class VMenu : public Modal
{
private:
	FARString strTitle;
	FARString strBottomTitle;

	int SelectPos;
	int TopPos;
	int MaxHeight;
	int MaxLength;
	int BoxType;
	bool PrevCursorVisible;
	DWORD PrevCursorSize;
	int PrevMacroMode;

	// переменная, отвечающая за отображение scrollbar в DI_LISTBOX & DI_COMBOBOX
	BitFlags VMFlags;
	BitFlags VMOldFlags;

	Dialog *ParentDialog;		// Для LisBox - родитель в виде диалога
	int DialogItemID;
	FARWINDOWPROC VMenuProc;	// функция обработки меню

	ConsoleTitle *OldTitle;		// предыдущий заголовок

	CriticalSection CS;

	bool bFilterEnabled;
	bool bFilterLocked;
	FARString strFilter;

	MenuItemEx **Item;

	int ItemCount;
	int ItemHiddenCount;
	int ItemSubMenusCount;

	uint64_t Colors[VMENU_COLOR_COUNT];

	int MaxLineWidth;
	bool bRightBtnPressed;
	int WrappedSeparatorIndex;

private:
	virtual void DisplayObject();
	void ShowMenu(bool IsParent, bool ForceFrameRedraw);
	void DrawEdges();
	void DrawTitles();
	int GetItemPosition(int Position);
	static int _SetUserData(MenuItemEx *PItem, const void *Data, int Size);
	static void *_GetUserData(MenuItemEx *PItem, void *Data, int Size);
	bool CheckKeyHiOrAcc(DWORD Key, int Type, int Translate);
	int CheckHighlights(wchar_t Chr, int StartPos = 0);
	wchar_t GetHighlights(const struct MenuItemEx *_item);
	bool ShiftItemShowPos(int Pos, int Direct);
	bool ItemCanHaveFocus(DWORD Flags);
	bool ItemCanBeEntered(DWORD Flags);
	bool ItemIsVisible(DWORD Flags);
	bool ItemIsSeparator(DWORD Flags);
	void UpdateMaxLengthFromTitles();
	void UpdateMaxLength(int Length);
	void UpdateItemFlags(int Pos, DWORD NewFlags);
	void UpdateInternalCounters(DWORD OldFlags, DWORD NewFlags);
	void RestoreFilteredItems();
	void FilterStringUpdated(bool bLonger);
	bool IsFilterEditKey(FarKey Key);
	bool ShouldSendKeyToFilter(FarKey Key);
	bool AddToFilter(const wchar_t *str);
	// коректировка текущей позиции и флагов SELECTED
	void UpdateSelectPos();
	void EnableFilter(bool Enable);

public:
	VMenu(const wchar_t *Title, MenuDataEx *Data, int ItemCount, int MaxHeight = 0, DWORD Flags = 0,
			FARWINDOWPROC Proc = nullptr, Dialog *ParentDialog = nullptr);

	virtual ~VMenu();

	void FastShow() { ShowMenu(false, false); }
	virtual void Show();
	virtual void Hide();
	void ResetCursor();

	void SetTitle(const wchar_t *Title);
	virtual FARString &GetTitle(FARString &strDest, int SubLen = -1, int TruncSize = 0);
	const wchar_t *GetPtrTitle() { return strTitle.CPtr(); }

	void SetBottomTitle(const wchar_t *BottomTitle);
	FARString &GetBottomTitle(FARString &strDest);
	void SetDialogStyle(int Style)
	{
		ChangeFlags(VMENU_WARNDIALOG, Style);
		SetColors(nullptr);
	}
	void SetUpdateRequired(int SetUpdate) { ChangeFlags(VMENU_UPDATEREQUIRED, SetUpdate); }
	void SetBoxType(int BoxType);

	void SetFlags(DWORD Flags) { VMFlags.Set(Flags); }
	void ClearFlags(DWORD Flags) { VMFlags.Clear(Flags); }
	BOOL CheckFlags(DWORD Flags) const { return VMFlags.Check(Flags); }
	DWORD GetFlags() const { return VMFlags.Flags; }
	DWORD ChangeFlags(DWORD Flags, BOOL Status) { return VMFlags.Change(Flags, Status); }

	void AssignHighlights(int Reverse);

	void SetColors(struct FarListColors *ColorsIn = nullptr);
	void GetColors(struct FarListColors *ColorsOut);
	void SetOneColor(int Index, uint64_t Color);

	virtual int ProcessKey(FarKey Key);
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual int64_t VMProcess(MacroOpcode OpCode, void *vParam = nullptr, int64_t iParam = 0);
	virtual FarKey ReadInput(INPUT_RECORD *GetReadRec = nullptr);

	void DeleteItems();
	int DeleteItem(int ID, int Count = 1);

	int AddItem(const MenuItemEx *NewItem, int PosAdd = 0x7FFFFFFF);
	int AddItem(const FarList *NewItem);
	int AddItem(const wchar_t *NewStrItem);

	int InsertItem(const FarListInsert *NewItem);
	int UpdateItem(const FarListUpdate *NewItem);
	int FindItem(const FarListFind *FindItem);
	int FindItem(int StartIndex, const wchar_t *Pattern, DWORD Flags = 0);

	int GetItemCount() { return ItemCount; }
	int GetShowItemCount() { return ItemCount - ItemHiddenCount; }
	int GetVisualPos(int Pos);
	int VisualPosToReal(int VPos);

	void *GetUserData(void *Data, int Size, int Position = -1);
	int GetUserDataSize(int Position = -1);
	int SetUserData(LPCVOID Data, int Size = 0, int Position = -1);

	int GetSelectPos() { return SelectPos; }
	int GetSelectPos(struct FarListPos *ListPos);
	int SetSelectPos(struct FarListPos *ListPos);
	int SetSelectPos(int Pos, int Direct, bool stop_on_edge = false);
	uint32_t GetCheck(int Position = -1);
	void SetCheck(uint32_t Check, int Position = -1);

	bool UpdateRequired();

	virtual void ResizeConsole();

	struct MenuItemEx *GetItemPtr(int Position = -1);

	void SortItems(int Direction = 0, int Offset = 0, BOOL SortForDataDWORD = FALSE);
	BOOL GetVMenuInfo(struct FarListInfo *Info);

	virtual const wchar_t *GetTypeName() { return L"[VMenu]"; }
	virtual int GetTypeAndName(FARString &strType, FARString &strName);

	virtual int GetType() { return CheckFlags(VMENU_COMBOBOX) ? MODALTYPE_COMBOBOX : MODALTYPE_VMENU; }

	void SetMaxHeight(int NewMaxHeight);

	int GetVDialogItemID() const { return DialogItemID; }
	void SetVDialogItemID(int NewDialogItemID) { DialogItemID = NewDialogItemID; }

	static MenuItemEx *FarList2MenuItem(const FarListItem *Item, MenuItemEx *ListItem);
	static FarListItem *MenuItem2FarList(const MenuItemEx *ListItem, FarListItem *Item);

	static LONG_PTR WINAPI DefMenuProc(HANDLE hVMenu, int Msg, int Param1, LONG_PTR Param2);
	static LONG_PTR WINAPI SendMenuMessage(HANDLE hVMenu, int Msg, int Param1, LONG_PTR Param2);
};
