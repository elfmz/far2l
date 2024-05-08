#pragma once
#ifndef __FAR2SDK_FARDLGBUILDERBASE_H__
#define __FAR2SDK_FARDLGBUILDERBASE_H__

#ifdef UNICODE
# define EMPTY_TEXT L""
#else
# define EMPTY_TEXT ""
#endif

#include "farplug-wide.h"

// Элемент выпадающего списка в диалоге.
struct DialogBuilderListItem
{
	// Строчка из LNG-файла, которая будет показана в диалоге.
	FarLangMsg MessageId;

	// Значение, которое будет записано в поле Value при выборе этой строчки.
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

	virtual ~DialogItemBinding()
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
struct CodePageBoxBinding: public DialogItemBinding<T>
{
	private:
		UINT *_Result;
		const UINT *_Chosen;

	public:
		CodePageBoxBinding(UINT *Result, const UINT *Chosen) : _Result(Result), _Chosen(Chosen) { }

		virtual void SaveValue(T *Item, int RadioGroupIndex)
		{
			*_Result = *_Chosen;
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
Класс для динамического построения диалогов. Автоматически вычисляет положение и размер
для добавляемых контролов, а также размер самого диалога. Автоматически записывает выбранные
значения в указанное место после закрытия диалога по OK.

По умолчанию каждый контрол размещается в новой строке диалога. Ширина для текстовых строк,
checkbox и radio button вычисляется автоматически, для других элементов передаётся явно.
Есть также возможность добавить статический текст слева или справа от контрола, при помощи
методов AddTextBefore и AddTextAfter.

Поддерживается также возможность расположения контролов в две колонки. Используется следующим
образом:
- StartColumns()
- добавляются контролы для первой колонки
- ColumnBreak()
- добавляются контролы для второй колонки
- EndColumns()

Базовая версия класса используется как внутри кода FAR, так и в плагинах.
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
			// реаллокация инвалидирует указатели на DialogItemEx, возвращённые из
			// AddDialogItem и аналогичных методов, поэтому размер массива подбираем такой,
			// чтобы все нормальные диалоги помещались без реаллокации
			// TODO хорошо бы, чтобы они вообще не инвалидировались
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
				/* стрелка history занимает дополнительное место, но раньше она рисовалась поверх рамки
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

		virtual const TCHAR *GetLangString(FarLangMsg MessageID)
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
		// Добавляет статический текст, расположенный на отдельной строке в диалоге.
		T *AddText(FarLangMsg LabelId)
		{
			T *Item = AddDialogItem(DI_TEXT, GetLangString(LabelId));
			SetNextY(Item);
			return Item;
		}

		// Добавляет чекбокс.
		T *AddCheckbox(FarLangMsg TextMessageId, BOOL *Value, int Mask=0)
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

		// Добавляет указанную текстовую строку справа от элемента RelativeTo.
		T *AddCheckboxAfter(T *RelativeTo, FarLangMsg TextMessageId, BOOL *Value, int Mask=0)
		{
			T *Item = AddDialogItem(DI_CHECKBOX, GetLangString(TextMessageId));
			Item->X2 = Item->X1 + ItemWidth(*Item);

			Item->Y1 = Item->Y2 = RelativeTo->Y1;
			Item->X1 = RelativeTo->X2 + 2;
			if (!Mask)
				Item->Selected = *Value;
			else
				Item->Selected = (*Value & Mask) ;
			SetLastItemBinding(CreateCheckBoxBinding(Value, Mask));

			DialogItemBinding<T> *Binding = FindBinding(RelativeTo);
			if (Binding)
				Binding->AfterLabelID = GetItemID(Item);

			return Item;
		}


		// Добавляет группу радиокнопок.
		void AddRadioButtons(int *Value, int OptionCount, FarLangMsg MessageIDs[])
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

		// Добавляет горизонтальную группу радиокнопок.
		void AddRadioButtonsHorz(int *Value, int OptionCount, FarLangMsg MessageIDs[])
		{
			T *PrevItem = nullptr;
			for(int i=0; i<OptionCount; i++)
			{
				T *Item = AddDialogItem(DI_RADIOBUTTON, GetLangString(MessageIDs[i]));
				if (!i) {
					SetNextY(Item);
					Item->Flags |= DIF_GROUP;
				}
				else {
					Item->Y1 = Item->Y2 = PrevItem->Y1;
					Item->X1 = PrevItem->X2 + 2;
				}
				Item->X2 = Item->X1 + ItemWidth(*Item);
				if (*Value == i)
					Item->Selected = TRUE;
				PrevItem = Item;
				SetLastItemBinding(CreateRadioButtonBinding(Value));
			}
		}

		// Добавляет поле типа DI_FIXEDIT для редактирования указанного числового значения.
		virtual T *AddIntEditField(int *Value, int Width, int Flags = 0)
		{
			return nullptr;
		}

		// Добавляет указанную текстовую строку слева от элемента RelativeTo.
		T *AddTextBefore(T *RelativeTo, FarLangMsg LabelId)
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

		// Добавляет указанную текстовую строку справа от элемента RelativeTo.
		T *AddTextAfter(T *RelativeTo, FarLangMsg LabelId)
		{
			T *Item = AddDialogItem(DI_TEXT, GetLangString(LabelId));
			Item->Y1 = Item->Y2 = RelativeTo->Y1;
			Item->X1 = RelativeTo->X2 + 2;

			DialogItemBinding<T> *Binding = FindBinding(RelativeTo);
			if (Binding)
				Binding->AfterLabelID = GetItemID(Item);

			return Item;
		}

		// Начинает располагать поля диалога в две колонки.
		void StartColumns()
		{
			ColumnStartIndex = DialogItemsCount;
			ColumnStartY = NextY;
		}

		// Завершает колонку полей в диалоге и переходит к следующей колонке.
		void ColumnBreak()
		{
			ColumnBreakIndex = DialogItemsCount;
			ColumnEndY = NextY;
			NextY = ColumnStartY;
		}

		// Завершает расположение полей диалога в две колонки.
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

		// Добавляет пустую строку.
		void AddEmptyLine()
		{
			NextY++;
		}

		// Добавляет кнопку
		T *AddButton(FarLangMsg MessageId, int &id, T *After = nullptr)
		{
			T *Button = AddDialogItem(DI_BUTTON, GetLangString(MessageId));
			if (After) {
				Button->X1 = After->X2 + 2;
				Button->Y1 = Button->Y2 = NextY - 1;
			} else {
				SetNextY(Button);
			}
			Button->X2 = Button->X1 + 20;//TODO: FIXME: ItemWidth(*Button);

			id = DialogItemsCount - 1;
			return Button;
		}

		// Добавляет сепаратор.
		void AddSeparator(FarLangMsg MessageId=FarLangMsg{-1})
		{
			T *Separator = AddDialogItem(DI_TEXT, MessageId == -1 ? EMPTY_TEXT : GetLangString(MessageId));
			Separator->Flags = DIF_SEPARATOR;
			Separator->X1 = 3;
			Separator->Y1 = Separator->Y2 = NextY++;
		}

		// Добавляет сепаратор, кнопки OK и Cancel.
		void AddOKCancel(FarLangMsg OKMessageId, FarLangMsg CancelMessageId)
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

		bool ShowDialog(int *id = nullptr)
		{
			UpdateBorderSize();
			UpdateSecondColumnPosition();
			int Result = DoShowDialog();
			if (id)
				*id = Result;
			if (Result == OKButtonID)
			{
				SaveValues();
				return true;
			}
			return false;
		}
};
#endif // __FAR2SDK_FARDLGBUILDERBASE_H__
