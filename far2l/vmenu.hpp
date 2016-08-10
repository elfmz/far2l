#pragma once

/*
vmenu.hpp

Îáû÷íîå âåðòèêàëüíîå ìåíþ
  à òàê æå:
    * ñïèñîê â DI_COMBOBOX
    * ñïèñîê â DI_LISTBOX
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
#include "plugin.hpp"
#include "manager.hpp"
#include "frame.hpp"
#include "bitflags.hpp"
#include "CriticalSections.hpp"


// Öâåòîâûå àòðèáóòû - èíäåêñû â ìàññèâå öâåòîâ
enum
{
	VMenuColorBody                = 0,     // ïîäëîæêà
	VMenuColorBox                 = 1,     // ðàìêà
	VMenuColorTitle               = 2,     // çàãîëîâîê - âåðõíèé è íèæíèé
	VMenuColorText                = 3,     // Òåêñò ïóíêòà
	VMenuColorHilite              = 4,     // HotKey
	VMenuColorSeparator           = 5,     // separator
	VMenuColorSelected            = 6,     // Âûáðàííûé
	VMenuColorHSelect             = 7,     // Âûáðàííûé - HotKey
	VMenuColorScrollBar           = 8,     // ScrollBar
	VMenuColorDisabled            = 9,     // Disabled
	VMenuColorArrows              =10,     // '<' & '>' îáû÷íûå
	VMenuColorArrowsSelect        =11,     // '<' & '>' âûáðàííûå
	VMenuColorArrowsDisabled      =12,     // '<' & '>' Disabled
	VMenuColorGrayed              =13,     // "ñåðûé"
	VMenuColorSelGrayed           =14,     // âûáðàííûé "ñåðûé"

	VMENU_COLOR_COUNT,                     // âñåãäà ïîñëåäíÿÿ - ðàçìåðíîñòü ìàññèâà
};

enum VMENU_FLAGS
{
	VMENU_ALWAYSSCROLLBAR      =0x00000100, // âñåãäà ïîêàçûâàòü ñêðîëëáàð
	VMENU_LISTBOX              =0x00000200, // Ýòî ñïèñîê â äèàëîãå
	VMENU_SHOWNOBOX            =0x00000400, // ïîêàçàòü áåç ðàìêè
	VMENU_AUTOHIGHLIGHT        =0x00000800, // àâòîìàòè÷åñêè âûáèðàòü ñèìîëû ïîäñâåòêè
	VMENU_REVERSEHIGHLIGHT     =0x00001000, // ... òîëüêî ñ êîíöà
	VMENU_UPDATEREQUIRED       =0x00002000, // ëèñò íåîáõîäèìî îáíîâèòü (ïåðåðèñîâàòü)
	VMENU_DISABLEDRAWBACKGROUND=0x00004000, // ïîäëîæêó íå ðèñîâàòü
	VMENU_WRAPMODE             =0x00008000, // çàöèêëåííûé ñïèñîê (ïðè ïåðåìåùåíèè)
	VMENU_SHOWAMPERSAND        =0x00010000, // ñèìâîë '&' ïîêàçûâàòü AS IS
	VMENU_WARNDIALOG           =0x00020000, //
	VMENU_NOTCENTER            =0x00040000, // íå öèòðîâàòü
	VMENU_LEFTMOST             =0x00080000, // "êðàéíèé ñëåâà" - íàðèñîâàòü íà 5 ïîçèöèé âïðàâî îò öåíòðà (X1 => (ScrX+1)/2+5)
	VMENU_NOTCHANGE            =0x00100000, //
	VMENU_LISTHASFOCUS         =0x00200000, // ìåíþ ÿâëÿåòñÿ ñïèñêîì â äèàëîãå è èìååò ôîêóñ
	VMENU_COMBOBOX             =0x00400000, // ìåíþ ÿâëÿåòñÿ êîìáîáîêñîì è îáðàáàòûâàåòñÿ ìåíåäæåðîì ïî-îñîáîìó.
	VMENU_MOUSEDOWN            =0x00800000, //
	VMENU_CHANGECONSOLETITLE   =0x01000000, //
	VMENU_MOUSEREACTION        =0x02000000, // ðåàãèðîâàòü íà äâèæåíèå ìûøè? (ïåðåìåùàòü ïîçèöèþ ïðè ïåðåìåùåíèè êóðñîðà ìûøè?)
	VMENU_DISABLED             =0x04000000, //
};

class Dialog;
class SaveScreen;


struct MenuItemEx
{
	DWORD  Flags;                  // Ôëàãè ïóíêòà

	string strName;

	DWORD  AccelKey;
	int    UserDataSize;           // Ðàçìåð ïîëüçîâàòåëüñêèõ äàííûõ
	union                          // Ïîëüçîâàòåëüñêèå äàííûå:
	{
		char  *UserData;             // - óêàçàòåëü!
		char   Str4[sizeof(char*)];  // - strlen(ñòðîêà)+1 <= sizeof(char*)
	};

	short AmpPos;                  // Ïîçèöèÿ àâòîíàçíà÷åííîé ïîäñâåòêè
	short Len[2];                  // ðàçìåðû 2-õ ÷àñòåé
	short Idx2;                    // íà÷àëî 2-é ÷àñòè

	int   ShowPos;

	DWORD SetCheck(int Value)
	{
		if (Value)
		{
			Flags|=LIF_CHECKED;
			Flags &= ~0xFFFF;

			if (Value!=1) Flags|=Value&0xFFFF;
		}
		else
		{
			Flags&=~(0xFFFF|LIF_CHECKED);
		}

		return Flags;
	}

	DWORD SetSelect(int Value) { if (Value) Flags|=LIF_SELECTED; else Flags&=~LIF_SELECTED; return Flags;}
	DWORD SetDisable(int Value) { if (Value) Flags|=LIF_DISABLE; else Flags&=~LIF_DISABLE; return Flags;}

	void Clear()
	{
		Flags = 0;
		strName.Clear();
		AccelKey = 0;
		UserDataSize = 0;
		UserData = nullptr;
		AmpPos = 0;
		Len[0] = 0;
		Len[1] = 0;
		Idx2 = 0;
		ShowPos = 0;
	}

	//UserData íå êîïèðóåòñÿ.
	const MenuItemEx& operator=(const MenuItemEx &srcMenu)
	{
		if (this != &srcMenu)
		{
			Flags = srcMenu.Flags;
			strName = srcMenu.strName;
			AccelKey = srcMenu.AccelKey;
			UserDataSize = 0;
			UserData = nullptr;
			AmpPos = srcMenu.AmpPos;
			Len[0] = srcMenu.Len[0];
			Len[1] = srcMenu.Len[1];
			Idx2 = srcMenu.Idx2;
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

	DWORD SetCheck(int Value)
	{
		if (Value)
		{
			Flags &= ~0xFFFF;
			Flags|=((Value&0xFFFF)|LIF_CHECKED);
		}
		else
			Flags&=~(0xFFFF|LIF_CHECKED);

		return Flags;
	}

	DWORD SetSelect(int Value) { if (Value) Flags|=LIF_SELECTED; else Flags&=~LIF_SELECTED; return Flags;}
	DWORD SetDisable(int Value) { if (Value) Flags|=LIF_DISABLE; else Flags&=~LIF_DISABLE; return Flags;}
	DWORD SetGrayed(int Value) { if (Value) Flags|=LIF_GRAYED; else Flags&=~LIF_GRAYED; return Flags;}
};


class ConsoleTitle;

class VMenu: public Modal
{
	private:
		string strTitle;
		string strBottomTitle;

		int SelectPos;
		int TopPos;
		int MaxHeight;
		int MaxLength;
		int BoxType;
		bool PrevCursorVisible;
		DWORD PrevCursorSize;
		int PrevMacroMode;

		// ïåðåìåííàÿ, îòâå÷àþùàÿ çà îòîáðàæåíèå scrollbar â DI_LISTBOX & DI_COMBOBOX
		BitFlags VMFlags;
		BitFlags VMOldFlags;

		Dialog *ParentDialog;         // Äëÿ LisBox - ðîäèòåëü â âèäå äèàëîãà
		int DialogItemID;
		FARWINDOWPROC VMenuProc;      // ôóíêöèÿ îáðàáîòêè ìåíþ

		ConsoleTitle *OldTitle;     // ïðåäûäóùèé çàãîëîâîê

		CriticalSection CS;
		bool *Used;

		bool bFilterEnabled;
		bool bFilterLocked;
		string strFilter;

		MenuItemEx **Item;

		int ItemCount;
		int ItemHiddenCount;
		int ItemSubMenusCount;

		BYTE Colors[VMENU_COLOR_COUNT];

		int MaxLineWidth;
		bool bRightBtnPressed;

	private:
		virtual void DisplayObject();
		void ShowMenu(bool IsParent=false);
		void DrawTitles();
		int GetItemPosition(int Position);
		static int _SetUserData(MenuItemEx *PItem,const void *Data,int Size);
		static void* _GetUserData(MenuItemEx *PItem,void *Data,int Size);
		bool CheckKeyHiOrAcc(DWORD Key,int Type,int Translate);
		int CheckHighlights(wchar_t Chr,int StartPos=0);
		wchar_t GetHighlights(const struct MenuItemEx *_item);
		bool ShiftItemShowPos(int Pos,int Direct);
		bool ItemCanHaveFocus(DWORD Flags);
		bool ItemCanBeEntered(DWORD Flags);
		bool ItemIsVisible(DWORD Flags);
		void UpdateMaxLengthFromTitles();
		void UpdateMaxLength(int Length);
		void UpdateItemFlags(int Pos, DWORD NewFlags);
		void UpdateInternalCounters(DWORD OldFlags, DWORD NewFlags);
		void RestoreFilteredItems();
		void FilterStringUpdated(bool bLonger);
		bool IsFilterEditKey(int Key);
		bool ShouldSendKeyToFilter(int Key);
		bool AddToFilter(const wchar_t *str);
		//êîðåêòèðîâêà òåêóùåé ïîçèöèè è ôëàãîâ SELECTED
		void UpdateSelectPos();

	public:

		VMenu(const wchar_t *Title,
		      MenuDataEx *Data,
		      int ItemCount,
		      int MaxHeight=0,
		      DWORD Flags=0,
		      FARWINDOWPROC Proc=nullptr,
		      Dialog *ParentDialog=nullptr);


		virtual ~VMenu();

		void FastShow() {ShowMenu();}
		virtual void Show();
		virtual void Hide();
		void ResetCursor();

		void SetTitle(const wchar_t *Title);
		virtual string &GetTitle(string &strDest,int SubLen=-1,int TruncSize=0);
		const wchar_t *GetPtrTitle() { return strTitle.CPtr(); }

		void SetBottomTitle(const wchar_t *BottomTitle);
		string &GetBottomTitle(string &strDest);
		void SetDialogStyle(int Style) {ChangeFlags(VMENU_WARNDIALOG,Style); SetColors(nullptr);}
		void SetUpdateRequired(int SetUpdate) {ChangeFlags(VMENU_UPDATEREQUIRED,SetUpdate);}
		void SetBoxType(int BoxType);

		void SetFlags(DWORD Flags) { VMFlags.Set(Flags); }
		void ClearFlags(DWORD Flags) { VMFlags.Clear(Flags); }
		BOOL CheckFlags(DWORD Flags) const { return VMFlags.Check(Flags); }
		DWORD GetFlags() const { return VMFlags.Flags; }
		DWORD ChangeFlags(DWORD Flags,BOOL Status) {return VMFlags.Change(Flags,Status);}

		void AssignHighlights(int Reverse);

		void SetColors(struct FarListColors *ColorsIn=nullptr);
		void GetColors(struct FarListColors *ColorsOut);
		void SetOneColor(int Index, short Color);

		virtual int ProcessKey(int Key);
		virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
		virtual int64_t VMProcess(int OpCode,void *vParam=nullptr,int64_t iParam=0);
		virtual int ReadInput(INPUT_RECORD *GetReadRec=nullptr);

		void DeleteItems();
		int  DeleteItem(int ID,int Count=1);

		int  AddItem(const MenuItemEx *NewItem,int PosAdd=0x7FFFFFFF);
		int  AddItem(const FarList *NewItem);
		int  AddItem(const wchar_t *NewStrItem);

		int  InsertItem(const FarListInsert *NewItem);
		int  UpdateItem(const FarListUpdate *NewItem);
		int  FindItem(const FarListFind *FindItem);
		int  FindItem(int StartIndex,const wchar_t *Pattern,DWORD Flags=0);

		int  GetItemCount() { return ItemCount; };
		int  GetShowItemCount() { return ItemCount-ItemHiddenCount; };
		int  GetVisualPos(int Pos);
		int  VisualPosToReal(int VPos);

		void *GetUserData(void *Data,int Size,int Position=-1);
		int  GetUserDataSize(int Position=-1);
		int  SetUserData(LPCVOID Data,int Size=0,int Position=-1);

		int  GetSelectPos() { return SelectPos; }
		int  GetSelectPos(struct FarListPos *ListPos);
		int  SetSelectPos(struct FarListPos *ListPos);
		int  SetSelectPos(int Pos, int Direct);
		int  GetCheck(int Position=-1);
		void SetCheck(int Check, int Position=-1);

		bool UpdateRequired();

		virtual void ResizeConsole();

		struct MenuItemEx *GetItemPtr(int Position=-1);

		void SortItems(int Direction=0,int Offset=0,BOOL SortForDataDWORD=FALSE);
		BOOL GetVMenuInfo(struct FarListInfo* Info);

		virtual const wchar_t *GetTypeName() {return L"[VMenu]";};
		virtual int GetTypeAndName(string &strType, string &strName);

		virtual int GetType() { return CheckFlags(VMENU_COMBOBOX)?MODALTYPE_COMBOBOX:MODALTYPE_VMENU; }

		void SetMaxHeight(int NewMaxHeight);

		int GetVDialogItemID() const {return DialogItemID;};
		void SetVDialogItemID(int NewDialogItemID) {DialogItemID=NewDialogItemID;};

		static MenuItemEx *FarList2MenuItem(const FarListItem *Item,MenuItemEx *ListItem);
		static FarListItem *MenuItem2FarList(const MenuItemEx *ListItem,FarListItem *Item);

		static LONG_PTR WINAPI DefMenuProc(HANDLE hVMenu,int Msg,int Param1,LONG_PTR Param2);
		static LONG_PTR WINAPI SendMenuMessage(HANDLE hVMenu,int Msg,int Param1,LONG_PTR Param2);
};

void EnumFiles(VMenu& Menu, const wchar_t* Str);
