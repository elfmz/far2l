#pragma once
#ifndef __FAR2SDK_FARDLGBUILDER_H__
#define __FAR2SDK_FARDLGBUILDER_H__

typedef FarLangMsgID FarLangMsg;

#include "fardlgbuilderbase.hpp"

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
Версия класса для динамического построения диалогов, используемая в плагинах к FAR.
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

		virtual const TCHAR *GetLangString(FarLangMsg MessageID)
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
		PluginDialogBuilder(const PluginStartupInfo &aInfo, FarLangMsg TitleMessageID, const TCHAR *aHelpTopic)
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

		virtual FarDialogItem *AddIntEditField(int *Value, int Width, int Flags = 0)
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
#endif // __FAR2SDK_FARDLGBUILDER_H__
