#include "MultiArc.hpp"
#include <farkeys.h>
#include "marclng.hpp"

// #define MAX_PASSW_LEN 256

struct PutDlgData
{
	PluginClass *Self{};
	std::string ArcFormat;
	// char OriginalName[512];   //$ AA 26.11.2001
	std::string Password1;
	// char Password2[256];      //$ AA 28.11.2001
	std::string DefExt;
	BOOL DefaultPluginNotFound{};	//$ AA 2?.11.2001
	BOOL NoChangeArcName{};			//$ AA 23.11.2001
	BOOL OldExactState{};			//$ AA 26.11.2001
									// BOOL ArcNameChanged;        //$ AA 27.11.2001
};

#define MAM_SETDISABLE  DM_USER + 1
#define MAM_ARCSWITCHES DM_USER + 2
// #define MAM_SETNAME      DM_USER+3
#define MAM_SELARC    DM_USER + 4
#define MAM_ADDDEFEXT DM_USER + 5
#define MAM_DELDEFEXT DM_USER + 6

// номера элементов диалога PutFiles
#define PDI_DOUBLEBOX   0
#define PDI_ARCNAMECAPT 1
#define PDI_ARCNAMEEDT  2

// #define PDI_SELARCCAPT      3
// #define PDI_SELARCCOMB      4
// #define PDI_SWITCHESCAPT    5
// #define PDI_SWITCHESEDT     6
#define PDI_SWITCHESCAPT 3
#define PDI_SWITCHESEDT  4
#define PDI_SELARCCAPT   5
#define PDI_SELARCCOMB   6

#define PDI_SEPARATOR0     7
#define PDI_PASS0WCAPT     8
#define PDI_PASS0WEDT      9
#define PDI_PASS1WCAPT     10
#define PDI_PASS1WEDT      11
#define PDI_SEPARATOR1     12
#define PDI_ADDDELCHECK    13
#define PDI_EXACTNAMECHECK 14
#define PDI_PRIORLABEL     15
#define PDI_PRIORCBOX      16
#define PDI_BGROUNDCHECK   17
#define PDI_SEPARATOR2     18
#define PDI_ADDBTN         19
#define PDI_SELARCBTN      20
#define PDI_SAVEBTN        21
#define PDI_CANCELBTN      22

class SelectFormatComboBox
{
private:
	static int __cdecl Compare(FarListItem *Item1, FarListItem *Item2);
	FarList ListItems;

public:
	SelectFormatComboBox(FarDialogItem *DialogItem, const std::string &ArcFormat);
	~SelectFormatComboBox() { free(ListItems.Items); }
};

int SelectFormatComboBox::Compare(FarListItem *Item1, FarListItem *Item2)
{
	return strcmp(Item1->Text, Item2->Text);
}

SelectFormatComboBox::SelectFormatComboBox(FarDialogItem *DialogItem, const std::string &ArcFormat)
{
	typedef int(__cdecl * FCmp)(const void *, const void *);
	struct FarListItem *NewItems;
	int &Count = ListItems.ItemsNumber;
	FarListItem *&Items = ListItems.Items;

	DialogItem->ListItems = NULL;
	Items = NULL;
	Count = 0;
	for (int i = 0; i < ArcPlugin->FmtCount(); i++) {
		for (int j = 0;; j++) {
			std::string Format, DefExt, DefCommand;
			if (!ArcPlugin->GetFormatName(i, j, Format, DefExt))
				break;

			//*Buffer=0; //$ AA сбросится в GetDefaultCommands
			ArcPlugin->GetDefaultCommands(i, j, CMD_ADD, DefCommand);
			// хитрый финт - подстановка Buffer в качестве дефолта для самого Buffer
			DefCommand = KeyFileReadSection(INI_LOCATION, Format)
					.GetString(CmdNames[CMD_ADD], DefCommand.c_str());

			if (DefCommand.empty())
				continue;

			NewItems = (FarListItem *)realloc(Items, (Count + 1) * sizeof(FarListItem));
			if (NewItems == NULL) {
				free(Items);
				Items = NULL;
				return;
			}
			Items = NewItems;
			strncpy(Items[Count].Text, Format.c_str(), sizeof(Items[Count].Text) - 1);
			Items[Count].Flags = ((Count == 0 && ArcFormat.empty())
				|| !strcasecmp(ArcFormat.c_str(), Format.c_str())) ? MIF_SELECTED : 0;
			Count++;
		}
	}

	if (Count == 0) {
		// free(Items);
		// Items=NULL;
		return;
	}

	FSF.qsort(Items, Count, sizeof(struct FarListItem), (FCmp)Compare);

	DialogItem->ListItems = &ListItems;
}

LONG_PTR WINAPI PluginClass::PutDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	PutDlgData *pdd = (struct PutDlgData *)Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

	if (Msg == DN_INITDIALOG) {
		if (pdd->DefaultPluginNotFound)
			Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_SAVEBTN, 1);

		Info.SendDlgMessage(hDlg, DM_SETTEXTLENGTH, PDI_PASS0WEDT, 255);
		Info.SendDlgMessage(hDlg, DM_SETTEXTLENGTH, PDI_PASS1WEDT, 255);
		Info.SendDlgMessage(hDlg, MAM_SETDISABLE, 0, 0);
		Info.SendDlgMessage(hDlg, MAM_ARCSWITCHES, 0, 0);

		// GetRegKey(HKEY_CURRENT_USER,pdd->ArcFormat,"AddSwitches",Buffer,"",sizeof(Buffer));
		// Info.SendDlgMessage(hDlg,DM_SETTEXTPTR, PDI_SWITCHESEDT, (long)Buffer);

		SetDialogControlText(hDlg, 0, StrPrintf(GetMsg(MAddTitle), pdd->ArcFormat.c_str()));

		// Info.SendDlgMessage(hDlg,MAM_SETNAME,0,0);

		if (OLD_DIALOG_STYLE) {
			Info.SendDlgMessage(hDlg, DM_SHOWITEM, PDI_SELARCCOMB, 0);
			Info.SendDlgMessage(hDlg, DM_SHOWITEM, PDI_SELARCCAPT, 0);

			FarDialogItem Item;
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, PDI_SWITCHESCAPT, (LONG_PTR)&Item);
			Item.X1 = 5;
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, PDI_SWITCHESCAPT, (LONG_PTR)&Item);
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, PDI_SWITCHESEDT, (LONG_PTR)&Item);
			Item.X1 = 5;
			Item.X2 = 70;
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, PDI_SWITCHESEDT, (LONG_PTR)&Item);

			Info.SendDlgMessage(hDlg, DM_SHOWITEM, PDI_SELARCBTN, 1);
		} else
			Info.SendDlgMessage(hDlg, DM_SHOWITEM, PDI_SELARCBTN, 0);

		Info.SendDlgMessage(hDlg, DM_SETCHECK, PDI_EXACTNAMECHECK,
				pdd->OldExactState ? BSTATE_CHECKED : BSTATE_UNCHECKED);
		// pdd->ArcNameChanged=0;
		if (pdd->NoChangeArcName)
			Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_EXACTNAMECHECK, 0);

		return TRUE;
	} else if (Msg == DN_EDITCHANGE) {
		if (Param1 == PDI_ARCNAMEEDT) {
			FarDialogItem *Item = (FarDialogItem *)Param2;
			Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_ADDBTN, Item->Data[0] != 0);
			// pdd->ArcNameChanged=TRUE;
			Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_EXACTNAMECHECK, 1);
		} else if (Param1 == PDI_SELARCCOMB) {
			pdd->ArcFormat = ((FarDialogItem *)Param2)->Data;
			Info.SendDlgMessage(hDlg, MAM_SELARC, 0, 0);
		} else if (Param1 == PDI_SWITCHESEDT) {
			Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_SAVEBTN, 1);
			// return TRUE;
		}

	} else if (Msg == DN_KEY && Param2 == KEY_SHIFTF1)		// Select archiver
	{
		if (OLD_DIALOG_STYLE) {
			Info.SendDlgMessage(hDlg, DN_BTNCLICK, PDI_SELARCBTN, 0);
		} else {
			;			// здесь код для раскрытия комбобокса выборора архиватора
		}
		return TRUE;	// не обрабатывать эту клавишу
	} else if (Msg == DN_BTNCLICK) {
		switch (Param1) {
			case PDI_CANCELBTN:
				// break;
			case PDI_ADDBTN: {
				/*// проверка совпадения введенного пароля и подтверждения
				char Password1[256],Password2[256];
				Info.SendDlgMessage(hDlg, DM_GETTEXTPTR, PDI_PASS0WEDT, (long)Password1);
				Info.SendDlgMessage(hDlg, DM_GETTEXTPTR, PDI_PASS1WEDT, (long)Password2);
				if (lstrcmp(Password1,Password2))
				{
				  const char *MsgItems[]={GetMsg(MError),GetMsg(MAddPswNotMatch),GetMsg(MOk)};
				  Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
				  return TRUE;
				}
				break;*/
				Info.SendDlgMessage(hDlg, DM_CLOSE, -1, 0);
				return TRUE;
			}

			case PDI_SAVEBTN: {
				KeyFileHelper kfh(INI_LOCATION);
				kfh.SetString(INI_SECTION, "DefaultFormat", pdd->ArcFormat.c_str());
				kfh.SetString(INI_SECTION, "AddSwitches", GetDialogControlText(hDlg, PDI_SWITCHESEDT));

				// Info.SendDlgMessage(hDlg, DM_GETTEXTPTR, PDI_SWITCHESEDT, (LONG_PTR)Buffer);
				// Info.SendDlgMessage(hDlg, DM_ADDHISTORY, PDI_SWITCHESEDT, (LONG_PTR)Buffer);

				Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_SAVEBTN, 0);
				Info.SendDlgMessage(hDlg, DM_SETFOCUS, PDI_ARCNAMEEDT, 0);

				return TRUE;
			}
			case PDI_SELARCBTN:
				if (pdd->Self->SelectFormat(pdd->ArcFormat, TRUE)) {
					Info.SendDlgMessage(hDlg, MAM_SELARC, 0, 0);
				}
				Info.SendDlgMessage(hDlg, DM_SETFOCUS, PDI_ARCNAMEEDT, 0);
				return TRUE;
			case PDI_EXACTNAMECHECK:
				if (Param2) {
					BOOL UnChanged =
							(BOOL)Info.SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, PDI_ARCNAMEEDT, -1);
					if (!pdd->OldExactState && /*!pdd->ArcNameChanged*/ UnChanged)	// 0->1
						Info.SendDlgMessage(hDlg, MAM_ADDDEFEXT, 0, 0);
				} else if (pdd->OldExactState)										// 1->0
					Info.SendDlgMessage(hDlg, MAM_DELDEFEXT, 0, 0);
				pdd->OldExactState = (BOOL)Param2;
				return TRUE;
		}
	} else if (Msg == DN_CLOSE) {
		if (Param1 == PDI_ADDBTN && Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_ADDBTN, -1)) {
			// проверка совпадения введенного пароля и подтверждения
			if (GetDialogControlText(hDlg, PDI_PASS0WEDT) != GetDialogControlText(hDlg, PDI_PASS1WEDT)) {
				const char *MsgItems[] = {GetMsg(MError), GetMsg(MAddPswNotMatch), GetMsg(MOk)};
				Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, ARRAYSIZE(MsgItems), 1);
				return FALSE;
			}
			return TRUE;
		}

		return	/*Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_ADDBTN, -1) == TRUE ||*/
				Param1 < 0 || Param1 == PDI_CANCELBTN;
	} else if (Msg == MAM_SETDISABLE) {
		std::string str;
		ArcPlugin->GetDefaultCommands(pdd->Self->ArcPluginNumber, pdd->Self->ArcPluginType, CMD_ADD, str);
		str = KeyFileReadSection(INI_LOCATION, pdd->ArcFormat).GetString(CmdNames[CMD_ADD], str.c_str());
		Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_ADDBTN, !str.empty());

	} else if (Msg == MAM_ARCSWITCHES) {
		// Выставляем данные из AddSwitches
		const auto &SwitchesStr = KeyFileReadSection(INI_LOCATION, pdd->ArcFormat).GetString("AddSwitches", "");
		SetDialogControlText(hDlg, PDI_SWITCHESEDT, SwitchesStr);
		if (!SwitchesStr.empty() && Opt.UseLastHistory) {
			SetDialogControlText(hDlg, PDI_SWITCHESEDT, "");
		}
		// если AddSwitches пустой и юзается UseLastHistory, то...
		std::string SwHistoryName("ArcSwitches/");
		SwHistoryName+= pdd->ArcFormat;
		// ...следующая команда заставит выставить LastHistory
		Info.SendDlgMessage(hDlg, DM_SETHISTORY, PDI_SWITCHESEDT, (LONG_PTR)SwHistoryName.c_str());
		// если история была пустая то всё таки надо выставить это поле из настроек
		if (!SwitchesStr.empty() && !Info.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, PDI_SWITCHESEDT, 0)) {
			SetDialogControlText(hDlg, PDI_SWITCHESEDT, SwitchesStr);
		}

		// Info.SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, PDI_SWITCHESEDT, 1);
		return TRUE;
	} else if (Msg == MAM_SELARC) {
		Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_SAVEBTN, 1);
		Info.SendDlgMessage(hDlg, DM_ENABLE, PDI_EXACTNAMECHECK, 1);

		pdd->Self->FormatToPlugin(pdd->ArcFormat, pdd->Self->ArcPluginNumber, pdd->Self->ArcPluginType);

		BOOL IsDelOldDefExt = (BOOL)Info.SendDlgMessage(hDlg, MAM_DELDEFEXT, 0, 0);
		IsDelOldDefExt = IsDelOldDefExt && Info.SendDlgMessage(hDlg, DM_GETCHECK, PDI_EXACTNAMECHECK, 0);

		pdd->DefExt = KeyFileReadSection(INI_LOCATION, pdd->ArcFormat).GetString("DefExt", "");
		if (pdd->DefExt.empty()) {
			ArcPlugin->GetFormatName(pdd->Self->ArcPluginNumber, pdd->Self->ArcPluginType, pdd->ArcFormat, pdd->DefExt);
		}

		if (IsDelOldDefExt)
			Info.SendDlgMessage(hDlg, MAM_ADDDEFEXT, 0, 0);

		SetDialogControlText(hDlg, 0, StrPrintf(GetMsg(MAddTitle), pdd->ArcFormat.c_str()));

		Info.SendDlgMessage(hDlg, MAM_SETDISABLE, 0, 0);
		Info.SendDlgMessage(hDlg, MAM_ARCSWITCHES, 0, 0);
		Info.SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, PDI_SWITCHESEDT, 1);
		// Info.SendDlgMessage(hDlg,MAM_SETNAME,0,0);
		Info.SendDlgMessage(hDlg, DM_SETFOCUS, PDI_ARCNAMEEDT, 0);
		// Info.SendDlgMessage(hDlg,MAM_ARCSWITCHES,0,0);
		return TRUE;
	} else if (Msg == MAM_ADDDEFEXT) {
		std::string Name = GetDialogControlText(hDlg, PDI_ARCNAMEEDT);
		AddExt(Name, pdd->DefExt);
		SetDialogControlText(hDlg, PDI_ARCNAMEEDT, Name);
		return TRUE;
	} else if (Msg == MAM_DELDEFEXT) {
		std::string Name = GetDialogControlText(hDlg, PDI_ARCNAMEEDT);
		if (DelExt(Name, pdd->DefExt)) {
			SetDialogControlText(hDlg, PDI_ARCNAMEEDT, Name);
			return TRUE;
		}
		return FALSE;
	}

	return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

// новый стиль диалога "Добавить к архиву"
//+----------------------------------------------------------------------------+
//|                                                                            |
//|   +---------------------------- Add to ZIP ----------------------------+   |
//|   | Add to archive                                                     |   |
//|   | backup************************************************************|   |
//|   | Select archiver        Switches                                    |   |
//|   | ZIP****************** *******************************************|   |
//|   |--------------------------------------------------------------------|   |
//|   | Archive password                  Reenter password                 |   |
//|   | ********************************  ******************************** |   |
//|   |--------------------------------------------------------------------|   |
//|   | [ ] Delete files after archiving                                   |   |
//|   | [ ] Exact archive filename                                         |   |
//|   | [ ] Background                                                     |   |
//|   |--------------------------------------------------------------------|   |
//|   |               [ Add ]  [ Save settings ]  [ Cancel ]               |   |
//|   +--------------------------------------------------------------------+   |
//|                                                                            |
//+----------------------------------------------------------------------------+

int PluginClass::PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode)
{
	if (ItemsNumber == 0)
		return 0;

	std::string Command, AllFilesMask;
	int ArcExitCode = 1;
	BOOL OldExactState = Opt.AdvFlags.AutoResetExactArcName ? FALSE : Opt.AdvFlags.ExactArcName;
	BOOL RestoreExactState = FALSE, NewArchive = TRUE;
	struct PutDlgData pdd;
	BOOL Ret = TRUE;

	pdd.Self = this;

	if (ArcPluginNumber == -1) {
		std::string DefaultFormat = KeyFileReadSection(INI_LOCATION, INI_SECTION).GetString("DefaultFormat", "TARGZ");
		if (!FormatToPlugin(DefaultFormat, ArcPluginNumber, ArcPluginType)) {
			ArcPluginNumber = ArcPluginType = 0;
			pdd.DefaultPluginNotFound = TRUE;
		} else {
			pdd.ArcFormat = DefaultFormat;
			pdd.DefaultPluginNotFound = FALSE;
		}
	}

	/* $ 14.02.2001 raVen
	   сброс галки "фоновая архивация" */
	/* $ 13.04.2001 DJ
	   перенесен в более подходящее место */
	Opt.UserBackground = 0;
	/* DJ $ */
	/* raVen $ */
	Opt.PriorityClass = 2;

	while (1) {
		pdd.DefExt = KeyFileReadSection(INI_LOCATION, pdd.ArcFormat).GetString("DefExt");
		if (pdd.DefExt.empty())
			Ret = ArcPlugin->GetFormatName(ArcPluginNumber, ArcPluginType, pdd.ArcFormat, pdd.DefExt);
		if (!Ret) {
			Opt.PriorityClass = 2;
			return 0;
		}

		const char *ArcHistoryName = "ArcName";

		/*
		  г============================ Add to RAR ============================¬
		  ¦ Add to archive                                                     ¦
		  ¦ arcput                                                            ¦
		  ¦ Switches                                                           ¦
		  ¦ -md1024 -mm -m5 -s -rr -av                                        ¦
		  ¦--------------------------------------------------------------------¦
		  | Archive password                  Reenter password                 |
		  | ********************************  ******************************** |
		  ¦--------------------------------------------------------------------¦
		  ¦ [ ] Delete files after archiving             Priority of process   ¦
		  ¦ [ ] Exact archive filename                   ******************** ¦
		  ¦ [ ] Background                                                     |
		  ¦--------------------------------------------------------------------¦
		  ¦    [ Add ]  [ Select archiver ]  [ Save settings ]  [ Cancel ]     ¦
		  L====================================================================-
		*/
		struct InitDialogItem InitItems[] = {
				/* 0*/ {DI_DOUBLEBOX, 3, 1, 72, 15, 0, 0, 0, 0, ""},
				/* 1*/ {DI_TEXT, 5, 2, 0, 0, 0, 0, 0, 0, (char *)MAddToArc},
				/* 2*/ {DI_EDIT, 5, 3, 70, 3, 1, (DWORD_PTR)ArcHistoryName, DIF_HISTORY, 0, ""},

				//* 3*/{DI_TEXT,5,4,0,0,0,0,0,0,(char *)MAddSelect},
				//* 4*/{DI_COMBOBOX,5,5,25,3,0,0,DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTNOAMPERSAND,0,""},
				//* 5*/{DI_TEXT,28,4,0,0,0,0,0,0,(char *)MAddSwitches},
				//* 6*/{DI_EDIT,28,5,70,3,0,0,DIF_HISTORY,0,""},
				/* 3*/ {DI_TEXT, 5, 4, 0, 0, 0, 0, 0, 0, (char *)MAddSwitches},
				/* 4*/ {DI_EDIT, 5, 5, 47, 3, 0, 0, DIF_HISTORY, 0, ""},
				/* 5*/ {DI_TEXT, 50, 4, 0, 0, 0, 0, 0, 0, (char *)MAddSelect},
				/* 6*/
				{DI_COMBOBOX, 50, 5, 70, 3, 0, 0,
						DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, 0, ""},

				/* 7*/ {DI_TEXT, 3, 6, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, 0, ""},
				/* 8*/ {DI_TEXT, 5, 7, 0, 0, 0, 0, 0, 0, (char *)MAddPassword},
				/* 9*/ {DI_PSWEDIT, 5, 8, 36, 8, 0, 0, 0, 0, ""},
				/*10*/ {DI_TEXT, 39, 7, 0, 0, 0, 0, 0, 0, (char *)MAddReenterPassword},
				/*11*/ {DI_PSWEDIT, 39, 8, 70, 8, 0, 0, 0, 0, ""},
				/*12*/ {DI_TEXT, 3, 9, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, 0, ""},
				/*13*/ {DI_CHECKBOX, 5, 10, 0, 0, 0, 0, 0, 0, (char *)MAddDelete},
				/*14*/ {DI_CHECKBOX, 5, 11, 0, 0, 0, 0, 0, 0, (char *)MExactArcName},
				/*15*/ {DI_TEXT, 50, 10, 0, 0, 0, 0, 0, 0, (char *)MPriorityOfProcess},
				/*16*/ {DI_COMBOBOX, 50, 11, 70, 11, 0, 0, DIF_DROPDOWNLIST, 0, ""},
				/*17*/ {DI_CHECKBOX, 5, 12, 0, 0, 0, 0, 0, 0, (char *)MBackground},
				/*18*/ {DI_TEXT, 3, 13, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, 0, ""},
				/*19*/ {DI_BUTTON, 0, 14, 0, 0, 0, 0, DIF_CENTERGROUP, 1, (char *)MAddAdd},
				/*20*/ {DI_BUTTON, 0, 14, 0, 0, 0, 0, DIF_CENTERGROUP, 0, (char *)MAddSelect},
				/*21*/ {DI_BUTTON, 0, 14, 0, 0, 0, 0, DIF_CENTERGROUP | DIF_DISABLE, 0, (char *)MAddSave},
				/*22*/ {DI_BUTTON, 0, 14, 0, 0, 0, 0, DIF_CENTERGROUP, 0, (char *)MAddCancel},
		};
		struct FarDialogItem DialogItems[ARRAYSIZE(InitItems)] = {};
		InitDialogItems(InitItems, DialogItems, ARRAYSIZE(InitItems));

		/*    if(OLD_DIALOG_STYLE)
			{
			  DialogItems[PDI_SWITCHESCAPT].X1=5;
			  DialogItems[PDI_SWITCHESEDT].X1=5;
			}*/

		SelectFormatComboBox Box(&DialogItems[PDI_SELARCCOMB], pdd.ArcFormat);

		// <Prior>
		FarListItem ListPriorItem[5];
		for (size_t I = 0; I < ARRAYSIZE(ListPriorItem); ++I) {
			ListPriorItem[I].Flags = 0;
			CharArrayCpyZ(ListPriorItem[I].Text, GetMsg((int)(MIdle_Priority_Class + I)));
		}
		ListPriorItem[Opt.PriorityClass].Flags = LIF_SELECTED;
		FarList ListPrior;
		ListPrior.ItemsNumber = ARRAYSIZE(ListPriorItem);
		ListPrior.Items = &ListPriorItem[0];
		DialogItems[PDI_PRIORCBOX].ListItems = &ListPrior;
		// </Prior>

		if (Opt.UseLastHistory)
			DialogItems[PDI_SWITCHESEDT].Flags|= DIF_USELASTHISTORY;

		if (!ArcName.empty()) {
			pdd.NoChangeArcName = TRUE;
			pdd.OldExactState = TRUE;
			RestoreExactState = TRUE;
			CharArrayCpyZ(DialogItems[PDI_ARCNAMEEDT].Data, ArcName.c_str());
		} else {
			PanelInfo pi;
			Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi);
#ifdef _ARC_UNDER_CURSOR_
			std::string ArcName(DialogItems[PDI_ARCNAMEEDT].Data);
			if (GetCursorName(ArcName, pdd.ArcFormat, pdd.DefExt, &pi)) {
				CharArrayCpyZ(DialogItems[PDI_ARCNAMEEDT].Data, ArcName.c_str());
				// pdd.NoChangeArcName=TRUE;
				RestoreExactState = TRUE;
				pdd.OldExactState = TRUE;
			} else {
#endif		//_ARC_UNDER_CURSOR_
			// pdd.OldExactState=Opt.AdvFlags.AutoResetExactArcName?FALSE:Opt.AdvFlags.ExactArcName;
				pdd.OldExactState = OldExactState;
#ifdef _GROUP_NAME_
				if (ItemsNumber == 1 && pi.SelectedItemsNumber == 1
						&& (pi.SelectedItems[0].Flags & PPIF_SELECTED)) {
					char CurDir[NM] = {0};
					if (sdc_getcwd(CurDir, sizeof(CurDir)))
						CharArrayCpyZ(DialogItems[PDI_ARCNAMEEDT].Data, FSF.PointToName(CurDir));
				} else {
					const auto &group = GetGroupName(PanelItem, ItemsNumber);
					CharArrayCpyZ(DialogItems[PDI_ARCNAMEEDT].Data, group.c_str());
				}
#else	//_GROUP_NAME_
			if (ItemsNumber == 1 && pi.SelectedItemsNumber == 1
					&& !(pi.SelectedItems[0].Flags & PPIF_SELECTED)) {
				CharArrayCpyZ(DialogItems[PDI_ARCNAMEEDT].Data, PanelItem->FindData.cFileName);
				char *Dot = strrchr(DialogItems[PDI_ARCNAMEEDT].Data, '.');
				if (Dot != NULL)
					*Dot = 0;
			} else {
				char CurDir[NM];
				GetCurrentDirectory(sizeof(CurDir), CurDir);
				CharArrayCpyZ(DialogItems[PDI_ARCNAMEEDT].Data, FSF.PointToName(CurDir));
			}
#endif	// else _GROUP_NAME_
				if (pdd.OldExactState && ArcName.empty()) {
					std::string ArcNameTmp = DialogItems[PDI_ARCNAMEEDT].Data;
					if (AddExt(ArcNameTmp, pdd.DefExt)) {
						CharArrayCpyZ(DialogItems[PDI_ARCNAMEEDT].Data, ArcNameTmp.c_str());
					}
				}
#ifdef _ARC_UNDER_CURSOR_
			}
#endif		//_ARC_UNDER_CURSOR_
			/*    $ AA 29.11.2001 //нафига нам имя усреднять?
				  char AnsiName[NM];
				  OemToAnsi(DialogItems[PDI_ARCNAMEEDT].Data,AnsiName);
				  if(!IsCaseMixed(AnsiName))
				  {
					CharLower(AnsiName);
					AnsiToOem(AnsiName, DialogItems[PDI_ARCNAMEEDT].Data);
				  }
				  AA 29.11.2001 $ */
		}

		DialogItems[PDI_ADDDELCHECK].Selected = Move;
		/* $ 13.04.2001 DJ
		   UserBackground instead of Background
		*/
		DialogItems[PDI_BGROUNDCHECK].Selected = Opt.UserBackground;
		/* DJ $ */
		// strcpy(pdd.OriginalName,DialogItems[PDI_ARCNAMEEDT].Data);

		if ((OpMode & OPM_SILENT) == 0) {
			int AskCode = Info.DialogEx(Info.ModuleNumber, -1, -1, 76, 17, "AddToArc", DialogItems,
					ARRAYSIZE(DialogItems), 0, 0, PluginClass::PutDlgProc, (LONG_PTR)&pdd);

			pdd.Password1 = DialogItems[PDI_PASS0WEDT].Data;
			// strcpy(pdd.Password2,DialogItems[PDI_PASS1WEDT].Data); //$ AA 28.11.2001
			Opt.UserBackground = DialogItems[PDI_BGROUNDCHECK].Selected;
			Opt.PriorityClass = DialogItems[PDI_PRIORCBOX].ListPos;

			if (RestoreExactState)
				Opt.AdvFlags.ExactArcName = OldExactState;
			else
				Opt.AdvFlags.ExactArcName = DialogItems[PDI_EXACTNAMECHECK].Selected;
			// SetRegKey(HKEY_CURRENT_USER, "", "ExactArcName", Opt.ExactArcName);
			{
				KeyFileHelper(INI_LOCATION).SetInt(INI_SECTION, "AdvFlags", (int)Opt.AdvFlags);
			}

			FSF.Unquote(DialogItems[PDI_ARCNAMEEDT].Data);
			if (AskCode != PDI_ADDBTN || *DialogItems[PDI_ARCNAMEEDT].Data == 0) {
				Opt.PriorityClass = 2;
				return -1;
			}
			// SetRegKey(HKEY_CURRENT_USER,"","Background",Opt.UserBackground); // $ 06.02.2002 AA
		}

		std::string Tmp = DialogItems[PDI_ARCNAMEEDT].Data;
		if (DialogItems[PDI_EXACTNAMECHECK].Selected) {
			size_t p = Tmp.find_last_of("/.");
			if (p == std::string::npos || Tmp[p] != '.') {
				Tmp+= '.';
			}
		} else
			AddExt(Tmp, pdd.DefExt);
		CharArrayCpyZ(DialogItems[PDI_ARCNAMEEDT].Data, Tmp.c_str());

		int Recurse = FALSE;
		for (int I = 0; I < ItemsNumber; I++)
			if (PanelItem[I].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				Recurse = TRUE;
				break;
			}

		for (int I = 0; I < ItemsNumber; I++)	//!! $ 22.03.2002 AA временный фикс !!
			PanelItem[I].UserData = 0;			// CHECK FOR BUGS!!!

		int CommandType;
		if (DialogItems[PDI_ADDDELCHECK].Selected)
			CommandType = Recurse ? CMD_MOVERECURSE : CMD_MOVE;
		else
			CommandType = Recurse ? CMD_ADDRECURSE : CMD_ADD;

		Opt.Background = OpMode & OPM_SILENT ? 0 : Opt.UserBackground;
		{
			KeyFileReadSection kfh(INI_LOCATION, pdd.ArcFormat);
			ArcPlugin->GetDefaultCommands(ArcPluginNumber, ArcPluginType, CommandType, Command);
			Command = kfh.GetString(CmdNames[CommandType], Command.c_str());
			ArcPlugin->GetDefaultCommands(ArcPluginNumber, ArcPluginType, CMD_ALLFILESMASK, AllFilesMask);
			AllFilesMask = kfh.GetString("AllFilesMask", AllFilesMask.c_str());
		}
		if (*CurDir && Command.find("%%R") == std::string::npos && Command.find("%%r") == std::string::npos) {
			const char *MsgItems[] = {GetMsg(MWarning), GetMsg(MCannotPutToFolder), GetMsg(MPutToRoot),
					GetMsg(MOk), GetMsg(MCancel)};
			if (Info.Message(Info.ModuleNumber, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 2) != 0) {
				Opt.PriorityClass = 2;
				return -1;
			} else
				*CurDir = 0;
		}

		size_t SwPos = Command.find("%%S");
		if (SwPos != std::string::npos) {
			if (SwPos > 0 && SwPos + 3 < Command.size() && Command[SwPos - 1] == '{'
					&& Command[SwPos + 3] == '}')
				Command.replace(SwPos - 1, 5, DialogItems[PDI_SWITCHESEDT].Data);
			else
				Command.replace(SwPos, 3, DialogItems[PDI_SWITCHESEDT].Data);

		} else if (*DialogItems[PDI_SWITCHESEDT].Data) {
			SwPos = Command.find(" -- ");
			if (SwPos == std::string::npos)
				SwPos = Command.size();

			Command.insert(SwPos, DialogItems[PDI_SWITCHESEDT].Data);
			Command.insert(SwPos, " ");
		}

		int IgnoreErrors = (CurArcInfo.Flags & AF_IGNOREERRORS);
		FSF.Unquote(DialogItems[PDI_ARCNAMEEDT].Data);
		NewArchive = !FileExists(DialogItems[PDI_ARCNAMEEDT].Data);
		ArcCommand ArcCmd(PanelItem, ItemsNumber, Command.c_str(), DialogItems[PDI_ARCNAMEEDT].Data, "",
				pdd.Password1, AllFilesMask.c_str(), IgnoreErrors, CommandType, 0, CurDir, ItemsInfo.Codepage);

		// последующие операции (тестирование и тд) не должны быть фоновыми
		Opt.Background = 0;		// $ 06.02.2002 AA

		if (!IgnoreErrors && ArcCmd.GetExecCode() != 0)
			ArcExitCode = 0;
		if (ArcCmd.GetExecCode() == RETEXEC_ARCNOTFOUND)
			continue;

		std::string fullname = MakeFullName(DialogItems[PDI_ARCNAMEEDT].Data);
		if (!fullname.empty())
			ArcName = std::move(fullname);
		break;
	}

	Opt.PriorityClass = 2;
	if (Opt.UpdateDescriptions && ArcExitCode)
		for (int I = 0; I < ItemsNumber; I++)
			PanelItem[I].Flags|= PPIF_PROCESSDESCR;
	if (!Opt.UserBackground && ArcExitCode && NewArchive && GoToFile(ArcName.c_str(), Opt.AllowChangeDir))
		ArcExitCode = 2;
	return ArcExitCode;
}

#ifdef _ARC_UNDER_CURSOR_
BOOL PluginClass::GetCursorName(std::string &ArcName, std::string &ArcFormat, std::string &ArcExt, PanelInfo *pi)
{
	// if(!GetRegKey(HKEY_CURRENT_USER,"","ArcUnderCursor",0))
	if (!Opt.AdvFlags.ArcUnderCursor)
		return FALSE;

	PluginPanelItem *Items = pi->PanelItems;
	PluginPanelItem *SelItems = pi->SelectedItems;
	PluginPanelItem *CurItem = Items + pi->CurrentItem;

	// под курсором должна быть не папка
	if (CurItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return FALSE;

	// должно быть непустое расширение
	char *Dot = CharArrayRChr(CurItem->FindData.cFileName, '.');
	if (!Dot || !*(++Dot))
		return FALSE;

	int i, j;

	// курсор должен быть вне выделения
	for (i = 0; i < pi->SelectedItemsNumber; i++)
		if (!CharArrayCmp(CurItem->FindData.cFileName, SelItems[i].FindData.cFileName))
			return FALSE;

	// под курсором должен быть файл с расширением архива
	std::string Format, DefExt;
	for (i = 0; i < ArcPlugin->FmtCount(); i++)
		for (j = 0;; j++) {
			if (!ArcPlugin->GetFormatName(i, j, Format, DefExt))
				break;
			// хитрый финт, чтение ключа с дефолтом из DefExt
			DefExt = KeyFileReadSection(INI_LOCATION, Format).GetString("DefExt", DefExt.c_str());

			if (!strcasecmp(Dot, DefExt.c_str())) {
				CharArrayAssignToStr(ArcName, CurItem->FindData.cFileName);
				// выбрать соответствующий архиватор
				ArcPluginNumber = i;
				ArcPluginType = j;
				ArcFormat = std::move(Format);
				ArcExt = std::move(DefExt);
				return TRUE;
			}
		}
	return FALSE;
}
#endif	//_ARC_UNDER_CURSOR_

#ifdef _GROUP_NAME_
std::string PluginClass::GetGroupName(PluginPanelItem *Items, int Count)
{
	BOOL NoGroup = !/*GetRegKey(HKEY_CURRENT_USER,"","GroupName",0)*/ Opt.AdvFlags.GroupName;
	auto &FindData = Items->FindData;
	const char *Dot = CharArrayRChr(FindData.cFileName, '.');
	const int Len = (Dot && !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			? (int)(Dot - &FindData.cFileName[0])
			: CharArrayLen(FindData.cFileName);

	for (int i = 1; i < Count; i++) {
		if (NoGroup || strncmp(FindData.cFileName, Items[i].FindData.cFileName, Len)) {
			// взять имя папки
			char CurDir[NM + 1] = {0};
			if (sdc_getcwd(CurDir, ARRAYSIZE(CurDir) - 1)) {
				const char *CurDirName = FSF.PointToName(CurDir);
				return std::string(CurDirName ? CurDirName : CurDir);
			}
			return std::string(".");
		}
	}
	return std::string(FindData.cFileName, Len);
}
#endif	//_GROUP_NAME_
