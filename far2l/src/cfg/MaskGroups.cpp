/*
MaskGroups.cpp

Groups of file masks
*/

#include "headers.hpp"

#include "config.hpp"
#include "ConfigRW.hpp"
#include "ctrlobj.hpp"
#include "DialogBuilder.hpp"
#include "dialog.hpp"
#include "DlgGuid.hpp"
#include "hilight.hpp"
#include "interf.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "vmenu.hpp"
#include "MaskGroups.hpp"

/*
MMaskGroupRestore
MMaskGroupFindMask
MMaskGroupTotal
========================================

MaskGroupRestore
"Вы хотите восстановить наборы масок по умолчанию?"
"Do you wish to restore default mask sets?"
*/

static const wchar_t *Help = L"MaskGroupsSettings";

struct FileMaskStrings
{
	const char *TypeFmt, *Type0, *MaskName, *MaskValue;
};

static const FileMaskStrings FMS=
{
	"MaskGroups/Type%d",
	"MaskGroups/Type",
	"Name",
	"Value",
};

static int FillMasksMenu(VMenu *TypesMenu, int MenuPos)
{
	ConfigReader cfg_reader;
	int DizWidth = 10;
	MenuItemEx TypesMenuItem;
	TypesMenu->DeleteItems();
	int NumLine = 0;

	for (;; NumLine++)
	{
		cfg_reader.SelectSectionFmt(FMS.TypeFmt, NumLine);
		FARString strMask;
		if (!cfg_reader.GetString(strMask, FMS.MaskValue))
			break;

		FARString strMenuText;

		if (DizWidth)
		{
			FARString strName = cfg_reader.GetString(FMS.MaskName);
			if (static_cast<int>(strName.GetLength()) > DizWidth)
			{
				strName.Truncate(DizWidth - (Opt.NoGraphics ? 3 : 1));
				strName += (Opt.NoGraphics ? L"..." : L"…");
			}
			strMenuText.Format(L"%-*.*ls %lc ", DizWidth, DizWidth, strName.CPtr(), BoxSymbols[BS_V1]);
		}

		strMenuText += strMask;
		TypesMenuItem.Clear();
		TypesMenuItem.strName = strMenuText;
		TypesMenuItem.SetSelect(NumLine == MenuPos);
		TypesMenu->AddItem(&TypesMenuItem);
	}

	TypesMenuItem.strName.Clear();
	TypesMenuItem.SetSelect(NumLine == MenuPos);
	TypesMenu->AddItem(&TypesMenuItem);
	return NumLine;
}

static bool EditMaskRecord (int EditPos, bool NewRec)
{
	bool Result = false;
	FARString strName, strMasks;

	if (!NewRec)
	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FMS.TypeFmt, EditPos);
		strMasks = cfg_reader.GetString(FMS.MaskValue);
		strName = cfg_reader.GetString(FMS.MaskName);
	}

	DialogBuilder Builder(Msg::MaskGroupTitle, Help);
//	Builder.SetId(EditMaskGroupId);
	Builder.AddText(Msg::MaskGroupName);
	Builder.AddEditField(&strName, 60);
	Builder.AddText(Msg::MaskGroupMasks);
	Builder.AddEditField(&strMasks, 60);
	Builder.AddOKCancel();

	if (Builder.ShowDialog() && !strName.IsEmpty() && !strMasks.IsEmpty())
	{
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FMS.TypeFmt, EditPos);

		if (NewRec)
		{
			cfg_writer.ReserveIndexedSection(FMS.Type0, (unsigned int)EditPos);
		}

		cfg_writer.SetString(FMS.MaskValue, strMasks);
		cfg_writer.SetString(FMS.MaskName, strName);
		Result = true;
	}

	return Result;
}

static bool DeleteMaskRecord(int DeletePos)
{
	bool Result = false;
	FARString strItemName;

	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FMS.TypeFmt, DeletePos);
		strItemName = cfg_reader.GetString(FMS.MaskName);
	}

	if (!Message(MSG_WARNING, 2, Msg::MaskGroupTitle, Msg::MaskGroupAskDelete, strItemName, Msg::Delete, Msg::Cancel))
	{
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FMS.TypeFmt, DeletePos);
		cfg_writer.RemoveSection();
		cfg_writer.DefragIndexedSections(FMS.Type0);
		Result = true;
	}

	return Result;
}

void MaskGroupsSettings()
{
	int NumLine = 0;
	int MenuPos = 0;
	VMenu MasksMenu(Msg::MaskGroupTitle, nullptr, 0, ScrY-4);
	MasksMenu.SetHelp(Help);
	MasksMenu.SetFlags(VMENU_WRAPMODE);
	MasksMenu.SetPosition(-1, -1, 0, 0);
//	MasksMenu.SetId(MaskGroupsMenuId);
	//MasksMenu.SetBottomTitle(L"Ins Del F4 F7 Ctrl+R");
	MasksMenu.SetBottomTitle(L"Ins Del F4");
	while (1)
	{
		bool OuterLoop = true;
		bool MenuModified = false;

		while (!MasksMenu.Done())
		{
			if (OuterLoop || MenuModified)
			{
				if (MenuModified)
				{
					CtrlObject->HiFiles->UpdateHighlighting(true);
				}
				MasksMenu.Hide();
				NumLine = FillMasksMenu(&MasksMenu, MenuPos);
				MasksMenu.SetPosition(-1, -1, -1, -1);
				MasksMenu.Show();
				MenuModified = false;
				OuterLoop = false;
			}

			FarKey Key = MasksMenu.ReadInput();
			MenuPos = MasksMenu.GetSelectPos();

			switch (Key)
			{
				case KEY_NUMDEL:
				case KEY_DEL:
					MenuModified = (MenuPos < NumLine) && DeleteMaskRecord(MenuPos);
					break;

				case KEY_NUMPAD0:
				case KEY_INS:
					MenuModified = EditMaskRecord(MenuPos, true);
					break;

				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4:
					MenuModified = (MenuPos < NumLine) && EditMaskRecord(MenuPos, false);
					break;

				default:
					MasksMenu.ProcessInput();
					break;
			}
		}

		int ExitCode = MasksMenu.Modal::GetExitCode();

		if (ExitCode != -1)
		{
			MenuPos = ExitCode;
			MasksMenu.ClearDone();
			MasksMenu.WriteInput(KEY_F4);
			continue;
		}

		break;
	}
}

bool GetMaskGroup(const FARString &MaskName, FARString &MaskValue)
{
	ConfigReader cfg_reader;
	FARString strMaskName;

	for (int Num = 0;
		cfg_reader.SelectSectionFmt(FMS.TypeFmt, Num),
		cfg_reader.GetString(strMaskName, FMS.MaskName);
			Num++)
	{
		if (!StrCmpI(strMaskName, MaskName))
			return cfg_reader.GetString(MaskValue, FMS.MaskValue);
	}
	return false;
}
