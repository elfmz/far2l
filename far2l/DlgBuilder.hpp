#pragma once

/*
DlgBuilder.hpp

Äèíàìè÷åñêîå êîíñòðóèðîâàíèå äèàëîãîâ
*/
/*
Copyright (c) 2009 Far Group
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

#ifdef UNICODE
#define EMPTY_TEXT L""
#else
#define EMPTY_TEXT ""
#endif

// Ýëåìåíò âûïàäàþùåãî ñïèñêà â äèàëîãå.
struct DialogBuilderListItem
{
	// Ñòðî÷êà èç LNG-ôàéëà, êîòîðàÿ áóäåò ïîêàçàíà â äèàëîãå.
	int MessageId;

	// Çíà÷åíèå, êîòîðîå áóäåò çàïèñàíî â ïîëå Value ïðè âûáîðå ýòîé ñòðî÷êè.
	int ItemValue;
};

template<class T>
struct DialogItemBinding
{
	int BeforeLabelID;
	int AfterLabelID;

	DialogItemBinding()
		: BeforeLabelID(-1), AfterLabelID(-1)
	{
	}

	virtual void SaveValue(T *Item, int RadioGroupIndex)
	{
	}
};

template<class T>
struct CheckBoxBinding: public DialogItemBinding<T>
{
	private:
		BOOL *Value;
		int Mask;

	public:
		CheckBoxBinding(BOOL *aValue, int aMask) : Value(aValue), Mask(aMask) { }

		virtual void SaveValue(T *Item, int RadioGroupIndex)
		{
			if (!Mask)
			{
				*Value = Item->Selected;
			}
			else
			{
				if (Item->Selected)
					*Value |= Mask;
				else
					*Value &= ~Mask;
			}
		}
};

template<class T>
struct RadioButtonBinding: public DialogItemBinding<T>
{
	private:
		int *Value;

	public:
		RadioButtonBinding(int *aValue) : Value(aValue) { }

		virtual void SaveValue(T *Item, int RadioGroupIndex)
		{
			if (Item->Selected)
				*Value = RadioGroupIndex;
		}
};

template<class T>
struct ComboBoxBinding: public DialogItemBinding<T>
{
	int *Value;
	FarList *List;

	ComboBoxBinding(int *aValue, FarList *aList)
		: Value(aValue), List(aList)
	{
	}

	~ComboBoxBinding()
	{
		delete [] List->Items;
		delete List;
	}

	virtual void SaveValue(T *Item, int RadioGroupIndex)
	{
		FarListItem &ListItem = List->Items[Item->ListPos];
		*Value = ListItem.Reserved[0];
	}
};

/*
Êëàññ äëÿ äèíàìè÷åñêîãî ïîñòðîåíèÿ äèàëîãîâ. Àâòîìàòè÷åñêè âû÷èñëÿåò ïîëîæåíèå è ðàçìåð
äëÿ äîáàâëÿåìûõ êîíòðîëîâ, à òàêæå ðàçìåð ñàìîãî äèàëîãà. Àâòîìàòè÷åñêè çàïèñûâàåò âûáðàííûå
çíà÷åíèÿ â óêàçàííîå ìåñòî ïîñëå çàêðûòèÿ äèàëîãà ïî OK.

Ïî óìîë÷àíèþ êàæäûé êîíòðîë ðàçìåùàåòñÿ â íîâîé ñòðîêå äèàëîãà. Øèðèíà äëÿ òåêñòîâûõ ñòðîê,
checkbox è radio button âû÷èñëÿåòñÿ àâòîìàòè÷åñêè, äëÿ äðóãèõ ýëåìåíòîâ ïåðåäà¸òñÿ ÿâíî.
Åñòü òàêæå âîçìîæíîñòü äîáàâèòü ñòàòè÷åñêèé òåêñò ñëåâà èëè ñïðàâà îò êîíòðîëà, ïðè ïîìîùè
ìåòîäîâ AddTextBefore è AddTextAfter.

Ïîääåðæèâàåòñÿ òàêæå âîçìîæíîñòü ðàñïîëîæåíèÿ êîíòðîëîâ â äâå êîëîíêè. Èñïîëüçóåòñÿ ñëåäóþùèì
îáðàçîì:
- StartColumns()
- äîáàâëÿþòñÿ êîíòðîëû äëÿ ïåðâîé êîëîíêè
- ColumnBreak()
- äîáàâëÿþòñÿ êîíòðîëû äëÿ âòîðîé êîëîíêè
- EndColumns()

Áàçîâàÿ âåðñèÿ êëàññà èñïîëüçóåòñÿ êàê âíóòðè êîäà FAR, òàê è â ïëàãèíàõ.
*/

template<class T>
class DialogBuilderBase
{
	protected:
		T *DialogItems;
		DialogItemBinding<T> **Bindings;
		int DialogItemsCount;
		int DialogItemsAllocated;
		int NextY;
		int OKButtonID;
		int ColumnStartIndex;
		int ColumnBreakIndex;
		int ColumnStartY;
		int ColumnEndY;
		int ColumnMinWidth;

		static const int SECOND_COLUMN = -2;

		void ReallocDialogItems()
		{
			// ðåàëëîêàöèÿ èíâàëèäèðóåò óêàçàòåëè íà DialogItemEx, âîçâðàù¸ííûå èç
			// AddDialogItem è àíàëîãè÷íûõ ìåòîäîâ, ïîýòîìó ðàçìåð ìàññèâà ïîäáèðàåì òàêîé,
			// ÷òîáû âñå íîðìàëüíûå äèàëîãè ïîìåùàëèñü áåç ðåàëëîêàöèè
			// TODO õîðîøî áû, ÷òîáû îíè âîîáùå íå èíâàëèäèðîâàëèñü
			DialogItemsAllocated += 32;
			if (!DialogItems)
			{
				DialogItems = new T[DialogItemsAllocated];
				Bindings = new DialogItemBinding<T> * [DialogItemsAllocated];
			}
			else
			{
				T *NewDialogItems = new T[DialogItemsAllocated];
				DialogItemBinding<T> **NewBindings = new DialogItemBinding<T> * [DialogItemsAllocated];
				for(int i=0; i<DialogItemsCount; i++)
				{
					NewDialogItems [i] = DialogItems [i];
					NewBindings [i] = Bindings [i];
				}
				delete [] DialogItems;
				delete [] Bindings;
				DialogItems = NewDialogItems;
				Bindings = NewBindings;
			}
		}

		T *AddDialogItem(int Type, const TCHAR *Text)
		{
			if (DialogItemsCount == DialogItemsAllocated)
			{
				ReallocDialogItems();
			}
			int Index = DialogItemsCount++;
			T *Item = &DialogItems [Index];
			InitDialogItem(Item, Text);
			Item->Type = Type;
			Bindings [Index] = nullptr;
			return Item;
		}

		void SetNextY(T *Item)
		{
			Item->X1 = 5;
			Item->Y1 = Item->Y2 = NextY++;
		}

		int ItemWidth(const T &Item)
		{
			switch(Item.Type)
			{
			case DI_TEXT:
				return TextWidth(Item);

			case DI_CHECKBOX:
			case DI_RADIOBUTTON:
				return TextWidth(Item) + 4;

			case DI_EDIT:
			case DI_FIXEDIT:
			case DI_COMBOBOX:
				int Width = Item.X2 - Item.X1 + 1;
				/* ñòðåëêà history çàíèìàåò äîïîëíèòåëüíîå ìåñòî, íî ðàíüøå îíà ðèñîâàëàñü ïîâåðõ ðàìêè
				if (Item.Flags & DIF_HISTORY)
					Width++;
				*/
				return Width;
				break;
			}
			return 0;
		}

		void AddBorder(const TCHAR *TitleText)
		{
			T *Title = AddDialogItem(DI_DOUBLEBOX, TitleText);
			Title->X1 = 3;
			Title->Y1 = 1;
		}

		void UpdateBorderSize()
		{
			T *Title = &DialogItems[0];
			Title->X2 = Title->X1 + MaxTextWidth() + 3;
			Title->Y2 = DialogItems [DialogItemsCount-1].Y2 + 1;
		}

		int MaxTextWidth()
		{
			int MaxWidth = 0;
			for(int i=1; i<DialogItemsCount; i++)
			{
				if (DialogItems [i].X1 == SECOND_COLUMN) continue;
				int Width = ItemWidth(DialogItems [i]);
				int Indent = DialogItems [i].X1 - 5;
				Width += Indent;

				if (MaxWidth < Width)
					MaxWidth = Width;
			}
			int ColumnsWidth = 2*ColumnMinWidth+1;
			if (MaxWidth < ColumnsWidth)
				return ColumnsWidth;
			return MaxWidth;
		}

		void UpdateSecondColumnPosition()
		{
			int SecondColumnX1 = 6 + (DialogItems [0].X2 - DialogItems [0].X1 - 1)/2;
			for(int i=0; i<DialogItemsCount; i++)
			{
				if (DialogItems [i].X1 == SECOND_COLUMN)
				{
					int Width = DialogItems [i].X2 - DialogItems [i].X1;
					DialogItems [i].X1 = SecondColumnX1;
					DialogItems [i].X2 = DialogItems [i].X1 + Width;
				}
			}
		}

		virtual void InitDialogItem(T *NewDialogItem, const TCHAR *Text)
		{
		}

		virtual int TextWidth(const T &Item)
		{
			return -1;
		}

		void SetLastItemBinding(DialogItemBinding<T> *Binding)
		{
			Bindings [DialogItemsCount-1] = Binding;
		}

		int GetItemID(T *Item)
		{
			int Index = static_cast<int>(Item - DialogItems);
			if (Index >= 0 && Index < DialogItemsCount)
				return Index;
			return -1;
		}

		DialogItemBinding<T> *FindBinding(T *Item)
		{
			int Index = static_cast<int>(Item - DialogItems);
			if (Index >= 0 && Index < DialogItemsCount)
				return Bindings [Index];
			return nullptr;
		}

		void SaveValues()
		{
			int RadioGroupIndex = 0;
			for(int i=0; i<DialogItemsCount; i++)
			{
				if (DialogItems [i].Flags & DIF_GROUP)
					RadioGroupIndex = 0;
				else
					RadioGroupIndex++;

				if (Bindings [i])
					Bindings [i]->SaveValue(&DialogItems [i], RadioGroupIndex);
			}
		}

		virtual const TCHAR *GetLangString(int MessageID)
		{
			return nullptr;
		}

		virtual int DoShowDialog()
		{
			return -1;
		}

		virtual DialogItemBinding<T> *CreateCheckBoxBinding(BOOL *Value, int Mask)
		{
			return nullptr;
		}

		virtual DialogItemBinding<T> *CreateRadioButtonBinding(int *Value)
		{
			return nullptr;
		}

		DialogBuilderBase()
			: DialogItems(nullptr), DialogItemsCount(0), DialogItemsAllocated(0), NextY(2),
			  ColumnStartIndex(-1), ColumnBreakIndex(-1), ColumnMinWidth(0)
		{
		}

		~DialogBuilderBase()
		{
			for(int i=0; i<DialogItemsCount; i++)
			{
				if (Bindings [i])
					delete Bindings [i];
			}
			delete [] DialogItems;
			delete [] Bindings;
		}

	public:
		// Äîáàâëÿåò ñòàòè÷åñêèé òåêñò, ðàñïîëîæåííûé íà îòäåëüíîé ñòðîêå â äèàëîãå.
		T *AddText(int LabelId)
		{
			T *Item = AddDialogItem(DI_TEXT, GetLangString(LabelId));
			SetNextY(Item);
			return Item;
		}

		// Äîáàâëÿåò ÷åêáîêñ.
		T *AddCheckbox(int TextMessageId, BOOL *Value, int Mask=0)
		{
			T *Item = AddDialogItem(DI_CHECKBOX, GetLangString(TextMessageId));
			SetNextY(Item);
			Item->X2 = Item->X1 + ItemWidth(*Item);
			if (!Mask)
				Item->Selected = *Value;
			else
				Item->Selected = (*Value & Mask) ;
			SetLastItemBinding(CreateCheckBoxBinding(Value, Mask));
			return Item;
		}

		// Äîáàâëÿåò ãðóïïó ðàäèîêíîïîê.
		void AddRadioButtons(int *Value, int OptionCount, int MessageIDs[])
		{
			for(int i=0; i<OptionCount; i++)
			{
				T *Item = AddDialogItem(DI_RADIOBUTTON, GetLangString(MessageIDs[i]));
				SetNextY(Item);
				Item->X2 = Item->X1 + ItemWidth(*Item);
				if (!i)
					Item->Flags |= DIF_GROUP;
				if (*Value == i)
					Item->Selected = TRUE;
				SetLastItemBinding(CreateRadioButtonBinding(Value));
			}
		}

		// Äîáàâëÿåò ïîëå òèïà DI_FIXEDIT äëÿ ðåäàêòèðîâàíèÿ óêàçàííîãî ÷èñëîâîãî çíà÷åíèÿ.
		virtual T *AddIntEditField(int *Value, int Width)
		{
			return nullptr;
		}

		// Äîáàâëÿåò óêàçàííóþ òåêñòîâóþ ñòðîêó ñëåâà îò ýëåìåíòà RelativeTo.
		T *AddTextBefore(T *RelativeTo, int LabelId)
		{
			T *Item = AddDialogItem(DI_TEXT, GetLangString(LabelId));
			Item->Y1 = Item->Y2 = RelativeTo->Y1;
			Item->X1 = 5;
			Item->X2 = Item->X1 + ItemWidth(*Item) - 1;

			int RelativeToWidth = RelativeTo->X2 - RelativeTo->X1;
			RelativeTo->X1 = Item->X2 + 2;
			RelativeTo->X2 = RelativeTo->X1 + RelativeToWidth;

			DialogItemBinding<T> *Binding = FindBinding(RelativeTo);
			if (Binding)
				Binding->BeforeLabelID = GetItemID(Item);

			return Item;
		}

		// Äîáàâëÿåò óêàçàííóþ òåêñòîâóþ ñòðîêó ñïðàâà îò ýëåìåíòà RelativeTo.
		T *AddTextAfter(T *RelativeTo, int LabelId)
		{
			T *Item = AddDialogItem(DI_TEXT, GetLangString(LabelId));
			Item->Y1 = Item->Y2 = RelativeTo->Y1;
			Item->X1 = RelativeTo->X2 + 2;

			DialogItemBinding<T> *Binding = FindBinding(RelativeTo);
			if (Binding)
				Binding->AfterLabelID = GetItemID(Item);

			return Item;
		}

		// Íà÷èíàåò ðàñïîëàãàòü ïîëÿ äèàëîãà â äâå êîëîíêè.
		void StartColumns()
		{
			ColumnStartIndex = DialogItemsCount;
			ColumnStartY = NextY;
		}

		// Çàâåðøàåò êîëîíêó ïîëåé â äèàëîãå è ïåðåõîäèò ê ñëåäóþùåé êîëîíêå.
		void ColumnBreak()
		{
			ColumnBreakIndex = DialogItemsCount;
			ColumnEndY = NextY;
			NextY = ColumnStartY;
		}

		// Çàâåðøàåò ðàñïîëîæåíèå ïîëåé äèàëîãà â äâå êîëîíêè.
		void EndColumns()
		{
			for(int i=ColumnStartIndex; i<DialogItemsCount; i++)
			{
				int Width = ItemWidth(DialogItems [i]);
				if (Width > ColumnMinWidth)
					ColumnMinWidth = Width;
				if (i >= ColumnBreakIndex)
				{
					DialogItems [i].X1 = SECOND_COLUMN;
					DialogItems [i].X2 = SECOND_COLUMN + Width;
				}
			}

			ColumnStartIndex = -1;
			ColumnBreakIndex = -1;
		}

		// Äîáàâëÿåò ïóñòóþ ñòðîêó.
		void AddEmptyLine()
		{
			NextY++;
		}

		// Äîáàâëÿåò ñåïàðàòîð.
		void AddSeparator(int MessageId=-1)
		{
			T *Separator = AddDialogItem(DI_TEXT, MessageId == -1 ? EMPTY_TEXT : GetLangString(MessageId));
			Separator->Flags = DIF_SEPARATOR;
			Separator->X1 = 3;
			Separator->Y1 = Separator->Y2 = NextY++;
		}

		// Äîáàâëÿåò ñåïàðàòîð, êíîïêè OK è Cancel.
		void AddOKCancel(int OKMessageId, int CancelMessageId)
		{
			AddSeparator();

			T *OKButton = AddDialogItem(DI_BUTTON, GetLangString(OKMessageId));
			OKButton->Flags = DIF_CENTERGROUP;
			OKButton->DefaultButton = TRUE;
			OKButton->Y1 = OKButton->Y2 = NextY++;
			OKButtonID = DialogItemsCount-1;

			T *CancelButton = AddDialogItem(DI_BUTTON, GetLangString(CancelMessageId));
			CancelButton->Flags = DIF_CENTERGROUP;
			CancelButton->Y1 = CancelButton->Y2 = OKButton->Y1;
		}

		bool ShowDialog()
		{
			UpdateBorderSize();
			UpdateSecondColumnPosition();
			int Result = DoShowDialog();
			if (Result == OKButtonID)
			{
				SaveValues();
				return true;
			}
			return false;
		}
};

class PluginDialogBuilder;

class DialogAPIBinding: public DialogItemBinding<FarDialogItem>
{
protected:
	const PluginStartupInfo &Info;
	HANDLE *DialogHandle;
	int ID;

	DialogAPIBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID)
		: Info(aInfo), DialogHandle(aHandle), ID(aID)
	{
	}
};

class PluginCheckBoxBinding: public DialogAPIBinding
{
	BOOL *Value;
	int Mask;

public:
	PluginCheckBoxBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, BOOL *aValue, int aMask)
		: DialogAPIBinding(aInfo, aHandle, aID),
		  Value(aValue), Mask(aMask)
	{
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		BOOL Selected = static_cast<BOOL>(Info.SendDlgMessage(*DialogHandle, DM_GETCHECK, ID, 0));
		if (!Mask)
		{
			*Value = Selected;
		}
		else
		{
			if (Selected)
				*Value |= Mask;
			else
				*Value &= ~Mask;
		}
	}
};

class PluginRadioButtonBinding: public DialogAPIBinding
{
	private:
		int *Value;

	public:
		PluginRadioButtonBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, int *aValue)
			: DialogAPIBinding(aInfo, aHandle, aID),
			  Value(aValue)
		{
		}

		virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
		{
			if (Info.SendDlgMessage(*DialogHandle, DM_GETCHECK, ID, 0))
				*Value = RadioGroupIndex;
		}
};

#ifdef UNICODE

class PluginEditFieldBinding: public DialogAPIBinding
{
private:
	TCHAR *Value;
	int MaxSize;

public:
	PluginEditFieldBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, TCHAR *aValue, int aMaxSize)
		: DialogAPIBinding(aInfo, aHandle, aID), Value(aValue), MaxSize(aMaxSize)
	{
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		const TCHAR *DataPtr = (const TCHAR *) Info.SendDlgMessage(*DialogHandle, DM_GETCONSTTEXTPTR, ID, 0);
		wcsncpy(Value, DataPtr, MaxSize);
	}
};

class PluginIntEditFieldBinding: public DialogAPIBinding
{
private:
	int *Value;
	TCHAR Buffer[32];
	TCHAR Mask[32];

public:
	PluginIntEditFieldBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, int *aValue, int Width)
		: DialogAPIBinding(aInfo, aHandle, aID),
		  Value(aValue)
	{
		aInfo.FSF->itoa(*aValue, Buffer, 10);
		int MaskWidth = Width < 31 ? Width : 31;
		for(int i=0; i<MaskWidth; i++)
			Mask[i] = '9';
		Mask[MaskWidth] = '\0';
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		const TCHAR *DataPtr = (const TCHAR *) Info.SendDlgMessage(*DialogHandle, DM_GETCONSTTEXTPTR, ID, 0);
		*Value = Info.FSF->atoi(DataPtr);
	}

	TCHAR *GetBuffer()
	{
		return Buffer;
	}

	const TCHAR *GetMask()
	{
		return Mask;
	}
};

#else

class PluginEditFieldBinding: public DialogItemBinding<FarDialogItem>
{
private:
	TCHAR *Value;
	int MaxSize;

public:
	PluginEditFieldBinding(TCHAR *aValue, int aMaxSize)
		: Value(aValue), MaxSize(aMaxSize)
	{
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		strncpy(Value, Item->Data, MaxSize);
	}
};

class PluginIntEditFieldBinding: public DialogItemBinding<FarDialogItem>
{
private:
	const PluginStartupInfo &Info;
	int *Value;
	TCHAR Mask[32];

public:
	PluginIntEditFieldBinding(const PluginStartupInfo &aInfo, int *aValue, int Width)
		: Info(aInfo), Value(aValue)
	{
		int MaskWidth = Width < 31 ? Width : 31;
		for(int i=0; i<MaskWidth; i++)
			Mask[i] = '9';
		Mask[MaskWidth] = '\0';
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		*Value = Info.FSF->atoi(Item->Data);
	}

	const TCHAR *GetMask()
	{
		return Mask;
	}
};

#endif

/*
Âåðñèÿ êëàññà äëÿ äèíàìè÷åñêîãî ïîñòðîåíèÿ äèàëîãîâ, èñïîëüçóåìàÿ â ïëàãèíàõ ê FAR.
*/
class PluginDialogBuilder: public DialogBuilderBase<FarDialogItem>
{
	protected:
		const PluginStartupInfo &Info;
		HANDLE DialogHandle;
		const TCHAR *HelpTopic;

		virtual void InitDialogItem(FarDialogItem *Item, const TCHAR *Text)
		{
			memset(Item, 0, sizeof(FarDialogItem));
#ifdef UNICODE
			Item->PtrData = Text;
#else
			lstrcpyn(Item->Data, Text, sizeof(Item->Data)/sizeof(Item->Data[0]));
#endif
		}

		virtual int TextWidth(const FarDialogItem &Item)
		{
#ifdef UNICODE
			return wcslen(Item.PtrData);
#else
			return wcslen(Item.Data);
#endif
		}

		virtual const TCHAR *GetLangString(int MessageID)
		{
			return Info.GetMsg(Info.ModuleNumber, MessageID);
		}

		virtual int DoShowDialog()
		{
			int Width = DialogItems [0].X2+4;
			int Height = DialogItems [0].Y2+2;
#ifdef UNICODE
			DialogHandle = Info.DialogInit(Info.ModuleNumber, -1, -1, Width, Height,
				HelpTopic, DialogItems, DialogItemsCount, 0, 0, nullptr, 0);
			return Info.DialogRun(DialogHandle);
#else
			return Info.Dialog(Info.ModuleNumber, -1, -1, Width, Height,
				HelpTopic, DialogItems, DialogItemsCount);
#endif
		}

		virtual DialogItemBinding<FarDialogItem> *CreateCheckBoxBinding(BOOL *Value, int Mask)
		{
#ifdef UNICODE
			return new PluginCheckBoxBinding(Info, &DialogHandle, DialogItemsCount-1, Value, Mask);
#else
			return new CheckBoxBinding<FarDialogItem>(Value, Mask);
#endif
		}

		virtual DialogItemBinding<FarDialogItem> *CreateRadioButtonBinding(BOOL *Value)
		{
#ifdef UNICODE
			return new PluginRadioButtonBinding(Info, &DialogHandle, DialogItemsCount-1, Value);
#else
			return new RadioButtonBinding<FarDialogItem>(Value);
#endif
		}

public:
		PluginDialogBuilder(const PluginStartupInfo &aInfo, int TitleMessageID, const TCHAR *aHelpTopic)
			: Info(aInfo), HelpTopic(aHelpTopic)
		{
			AddBorder(GetLangString(TitleMessageID));
		}

		~PluginDialogBuilder()
		{
#ifdef UNICODE
			Info.DialogFree(DialogHandle);
#endif
		}

		virtual FarDialogItem *AddIntEditField(int *Value, int Width)
		{
			FarDialogItem *Item = AddDialogItem(DI_FIXEDIT, EMPTY_TEXT);
			Item->Flags |= DIF_MASKEDIT;
			PluginIntEditFieldBinding *Binding;
			Binding = new PluginIntEditFieldBinding(Info, &DialogHandle, DialogItemsCount-1, Value, Width);
			Item->PtrData = Binding->GetBuffer();


#ifdef _FAR_NO_NAMELESS_UNIONS
			Item->Param.Mask = Binding->GetMask();
#else
			Item->Mask = Binding->GetMask();
#endif
			SetNextY(Item);
			Item->X2 = Item->X1 + Width - 1;
			SetLastItemBinding(Binding);
			return Item;
		}

		FarDialogItem *AddEditField(TCHAR *Value, int MaxSize, int Width, const TCHAR *HistoryID = nullptr)
		{
			FarDialogItem *Item = AddDialogItem(DI_EDIT, Value);
			SetNextY(Item);
			Item->X2 = Item->X1 + Width;
			if (HistoryID)
			{
#ifdef _FAR_NO_NAMELESS_UNIONS
				Item->Param.History = HistoryID;
#else
				Item->History = HistoryID;
#endif
				Item->Flags |= DIF_HISTORY;
			}

#ifdef UNICODE
			SetLastItemBinding(new PluginEditFieldBinding(Info, &DialogHandle, DialogItemsCount-1, Value, MaxSize));
#else
			SetLastItemBinding(new PluginEditFieldBinding(Value, MaxSize));
#endif
			return Item;
		}
};
