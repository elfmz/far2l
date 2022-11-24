/*
flmodes.cpp

Файловая панель - работа с режимами
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

#include "headers.hpp"


#include "filelist.hpp"
#include "lang.hpp"
#include "filepanels.hpp"
#include "ctrlobj.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"

#define PANELMODES_INI	"panel_modes.ini"

PanelViewSettings ViewSettingsArray[]=
{
	/* 00 */{{COLUMN_MARK|NAME_COLUMN,SIZE_COLUMN|COLUMN_COMMAS,DATE_COLUMN},{0,10,0},3,{COLUMN_RIGHTALIGN|NAME_COLUMN},{0},1,0,1,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH}},
	/* 01 */{{NAME_COLUMN,NAME_COLUMN,NAME_COLUMN},{0,0,0},3,{COLUMN_RIGHTALIGN|NAME_COLUMN,OWNER_COLUMN,GROUP_COLUMN,SIZE_COLUMN,DATE_COLUMN,TIME_COLUMN},{0,6,6,6,0,5},6,0,1,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH}},
	/* 02 */{{NAME_COLUMN,NAME_COLUMN},{0,0},2,{COLUMN_RIGHTALIGN|NAME_COLUMN,OWNER_COLUMN,GROUP_COLUMN,SIZE_COLUMN,DATE_COLUMN,TIME_COLUMN},{0,6,6,6,0,5},6,0,0,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH}},
	/* 03 */{{NAME_COLUMN,SIZE_COLUMN,DATE_COLUMN,TIME_COLUMN},{0,6,0,5},4,{COLUMN_RIGHTALIGN|NAME_COLUMN,OWNER_COLUMN,GROUP_COLUMN},{0,6,6},3,0,1,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH}},
	/* 04 */{{NAME_COLUMN,SIZE_COLUMN},{0,6},2,{COLUMN_RIGHTALIGN|NAME_COLUMN,SIZE_COLUMN,OWNER_COLUMN,GROUP_COLUMN,DATE_COLUMN,TIME_COLUMN},{0,0,6,6,0,5},6,0,0,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH}},
	/* 05 */{{NAME_COLUMN,SIZE_COLUMN,PHYSICAL_COLUMN,WDATE_COLUMN,CDATE_COLUMN,ADATE_COLUMN,OWNER_COLUMN,GROUP_COLUMN,ATTR_COLUMN},{0,6,6,14,14,14,6,6,0},9,{COLUMN_RIGHTALIGN|NAME_COLUMN},{0},1,1,1,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH}},
	/* 06 */{{NAME_COLUMN,DIZ_COLUMN},{40,0},2,{COLUMN_RIGHTALIGN|NAME_COLUMN,OWNER_COLUMN,GROUP_COLUMN,SIZE_COLUMN,DATE_COLUMN,TIME_COLUMN},{0,6,6,6,0,5},6,0,1,0,0,0,0,{PERCENT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH}},
	/* 07 */{{NAME_COLUMN,SIZE_COLUMN,DIZ_COLUMN},{0,6,54},3,{COLUMN_RIGHTALIGN|NAME_COLUMN},{0},1,1,1,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH}},
	/* 08 */{{NAME_COLUMN,SIZE_COLUMN,OWNER_COLUMN,GROUP_COLUMN},{0,6,6,15},4,{COLUMN_RIGHTALIGN|NAME_COLUMN,OWNER_COLUMN,GROUP_COLUMN,SIZE_COLUMN,DATE_COLUMN,TIME_COLUMN},{0,6,6,6,0,5},6,0,1,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH}},
	/* 09 */{{NAME_COLUMN,SIZE_COLUMN,NUMLINK_COLUMN},{0,6,3},3,{COLUMN_RIGHTALIGN|NAME_COLUMN,OWNER_COLUMN,GROUP_COLUMN,SIZE_COLUMN,DATE_COLUMN,TIME_COLUMN},{0,6,6,6,0,5},6,0,1,0,0,0,0,{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH},{COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH,COUNT_WIDTH}}
};

size_t SizeViewSettingsArray=ARRAYSIZE(ViewSettingsArray);

void FileList::SetFilePanelModes()
{
	int CurMode=0;

	if (CtrlObject->Cp()->ActivePanel->GetType()==FILE_PANEL)
	{
		CurMode=CtrlObject->Cp()->ActivePanel->GetViewMode();
		CurMode=CurMode?CurMode-1:9;
	}

	for(;;)
	{
		MenuDataEx ModeListMenu[]=
		{
			{Msg::EditPanelModesBrief,0,0},
			{Msg::EditPanelModesMedium,0,0},
			{Msg::EditPanelModesFull,0,0},
			{Msg::EditPanelModesWide,0,0},
			{Msg::EditPanelModesDetailed,0,0},
			{Msg::EditPanelModesDiz,0,0},
			{Msg::EditPanelModesLongDiz,0,0},
			{Msg::EditPanelModesOwners,0,0},
			{Msg::EditPanelModesLinks,0,0},
			{Msg::EditPanelModesAlternative,0,0}
		};
		int ModeNumber;
		ModeListMenu[CurMode].SetSelect(1);
		{
			VMenu ModeList(Msg::EditPanelModes,ModeListMenu,ARRAYSIZE(ModeListMenu),ScrY-4);
			ModeList.SetPosition(-1,-1,0,0);
			ModeList.SetHelp(L"PanelViewModes");
			ModeList.SetFlags(VMENU_WRAPMODE);
			ModeList.Process();
			ModeNumber=ModeList.Modal::GetExitCode();
		}

		if (ModeNumber<0)
			return;

		CurMode=ModeNumber;

		enum ModeItems
		{
			MD_DOUBLEBOX,
			MD_TEXTTYPES,
			MD_EDITTYPES,
			MD_TEXTWIDTHS,
			MD_EDITWIDTHS,
			MD_TEXTSTATUSTYPES,
			MD_EDITSTATUSTYPES,
			MD_TEXTSTATUSWIDTHS,
			MD_EDITSTATUSWIDTHS,
			MD_SEPARATOR1,
			MD_CHECKBOX_FULLSCREEN,
			MD_CHECKBOX_ALIGNFILEEXT,
			MD_CHECKBOX_ALIGNFOLDEREXT,
			MD_CHECKBOX_FOLDERUPPERCASE,
			MD_CHECKBOX_FILESLOWERCASE,
			MD_CHECKBOX_UPPERTOLOWERCASE,
			MD_SEPARATOR2,
			MD_BUTTON_OK,
			MD_BUTTON_CANCEL,
		} ;
		DialogDataEx ModeDlgData[]=
		{
			{DI_DOUBLEBOX, 3, 1,72,15,{},0,ModeListMenu[ModeNumber].Name},
			{DI_TEXT,      5, 2, 0, 2,{},0,Msg::EditPanelModeTypes},
			{DI_EDIT,      5, 3,35, 3,{},DIF_FOCUS,L""},
			{DI_TEXT,      5, 4, 0, 4,{},0,Msg::EditPanelModeWidths},
			{DI_EDIT,      5, 5,35, 5,{},0,L""},
			{DI_TEXT,     38, 2, 0, 2,{},0,Msg::EditPanelModeStatusTypes},
			{DI_EDIT,     38, 3,70, 3,{},0,L""},
			{DI_TEXT,     38, 4, 0, 4,{},0,Msg::EditPanelModeStatusWidths},
			{DI_EDIT,     38, 5,70, 5,{},0,L""},
			{DI_TEXT,      3, 6, 0, 6,{},DIF_SEPARATOR,Msg::EditPanelReadHelp},
			{DI_CHECKBOX,  5, 7, 0, 7,{},0,Msg::EditPanelModeFullscreen},
			{DI_CHECKBOX,  5, 8, 0, 8,{},0,Msg::EditPanelModeAlignExtensions},
			{DI_CHECKBOX,  5, 9, 0, 9,{},0,Msg::EditPanelModeAlignFolderExtensions},
			{DI_CHECKBOX,  5,10, 0,10,{},0,Msg::EditPanelModeFoldersUpperCase},
			{DI_CHECKBOX,  5,11, 0,11,{},0,Msg::EditPanelModeFilesLowerCase},
			{DI_CHECKBOX,  5,12, 0,12,{},0,Msg::EditPanelModeUpperToLowerCase},
			{DI_TEXT,      3,13, 0,13,{},DIF_SEPARATOR,L""},
			{DI_BUTTON,    0,14, 0,14,{},DIF_DEFAULT|DIF_CENTERGROUP,Msg::Ok},
			{DI_BUTTON,    0,14, 0,14,{},DIF_CENTERGROUP,Msg::Cancel}
		};
		MakeDialogItemsEx(ModeDlgData,ModeDlg);
		int ExitCode;
		RemoveHighlights(ModeDlg[MD_DOUBLEBOX].strData);

		if (ModeNumber==9)
			ModeNumber=0;
		else
			ModeNumber++;

		PanelViewSettings NewSettings=ViewSettingsArray[ModeNumber];
		ModeDlg[MD_CHECKBOX_FULLSCREEN].Selected=NewSettings.FullScreen;
		ModeDlg[MD_CHECKBOX_ALIGNFILEEXT].Selected=NewSettings.AlignExtensions;
		ModeDlg[MD_CHECKBOX_ALIGNFOLDEREXT].Selected=NewSettings.FolderAlignExtensions;
		ModeDlg[MD_CHECKBOX_FOLDERUPPERCASE].Selected=NewSettings.FolderUpperCase;
		ModeDlg[MD_CHECKBOX_FILESLOWERCASE].Selected=NewSettings.FileLowerCase;
		ModeDlg[MD_CHECKBOX_UPPERTOLOWERCASE].Selected=NewSettings.FileUpperToLowerCase;
		ViewSettingsToText(NewSettings.ColumnType,NewSettings.ColumnWidth,NewSettings.ColumnWidthType,
		                   NewSettings.ColumnCount,ModeDlg[2].strData,ModeDlg[4].strData);
		ViewSettingsToText(NewSettings.StatusColumnType,NewSettings.StatusColumnWidth,NewSettings.StatusColumnWidthType,
		                   NewSettings.StatusColumnCount,ModeDlg[6].strData,ModeDlg[8].strData);
		{
			Dialog Dlg(ModeDlg,ARRAYSIZE(ModeDlg));
			Dlg.SetPosition(-1,-1,76,17);
			Dlg.SetHelp(L"PanelViewModes");
			Dlg.Process();
			ExitCode=Dlg.GetExitCode();
		}

		if (ExitCode!=MD_BUTTON_OK)
			continue;

		memset(&NewSettings,0,sizeof(NewSettings));
		NewSettings.FullScreen=ModeDlg[MD_CHECKBOX_FULLSCREEN].Selected;
		NewSettings.AlignExtensions=ModeDlg[MD_CHECKBOX_ALIGNFILEEXT].Selected;
		NewSettings.FolderAlignExtensions=ModeDlg[MD_CHECKBOX_ALIGNFOLDEREXT].Selected;
		NewSettings.FolderUpperCase=ModeDlg[MD_CHECKBOX_FOLDERUPPERCASE].Selected;
		NewSettings.FileLowerCase=ModeDlg[MD_CHECKBOX_FILESLOWERCASE].Selected;
		NewSettings.FileUpperToLowerCase=ModeDlg[MD_CHECKBOX_UPPERTOLOWERCASE].Selected;
		TextToViewSettings(ModeDlg[MD_EDITTYPES].strData,ModeDlg[MD_EDITWIDTHS].strData,NewSettings.ColumnType,
		                   NewSettings.ColumnWidth,NewSettings.ColumnWidthType,NewSettings.ColumnCount);
		TextToViewSettings(ModeDlg[MD_EDITSTATUSTYPES].strData,ModeDlg[MD_EDITSTATUSWIDTHS].strData,NewSettings.StatusColumnType,
		                   NewSettings.StatusColumnWidth,NewSettings.StatusColumnWidthType,NewSettings.StatusColumnCount);
		ViewSettingsArray[ModeNumber]=NewSettings;
		CtrlObject->Cp()->LeftPanel->SortFileList(TRUE);
		CtrlObject->Cp()->RightPanel->SortFileList(TRUE);
		CtrlObject->Cp()->SetScreenPosition();
		int LeftMode=CtrlObject->Cp()->LeftPanel->GetViewMode();
		int RightMode=CtrlObject->Cp()->RightPanel->GetViewMode();
//    CtrlObject->Cp()->LeftPanel->SetViewMode(ModeNumber);
//    CtrlObject->Cp()->RightPanel->SetViewMode(ModeNumber);
		CtrlObject->Cp()->LeftPanel->SetViewMode(LeftMode);
		CtrlObject->Cp()->RightPanel->SetViewMode(RightMode);
		CtrlObject->Cp()->LeftPanel->Redraw();
		CtrlObject->Cp()->RightPanel->Redraw();
	}
}


void FileList::ReadPanelModes(ConfigReader &cfg_reader)
{
	for (int I=0; I<10; I++)
	{
		cfg_reader.SelectSectionFmt("Panel/ViewModes/Mode%d", I);
		FARString strColumnTitles = cfg_reader.GetString("Columns", L"");
		FARString strColumnWidths = cfg_reader.GetString("ColumnWidths", L"");
		FARString strStatusColumnTitles = cfg_reader.GetString("StatusColumns", L"");
		FARString strStatusColumnWidths = cfg_reader.GetString("StatusColumnWidths", L"");

		if (strColumnTitles.IsEmpty() || strColumnWidths.IsEmpty())
			continue;

		PanelViewSettings NewSettings = ViewSettingsArray[VIEW_0+I];

		if (!strColumnTitles.IsEmpty())
			TextToViewSettings(strColumnTitles,strColumnWidths,NewSettings.ColumnType,
			                   NewSettings.ColumnWidth,NewSettings.ColumnWidthType,NewSettings.ColumnCount);

		if (!strStatusColumnTitles.IsEmpty())
			TextToViewSettings(strStatusColumnTitles,strStatusColumnWidths,NewSettings.StatusColumnType,
			                   NewSettings.StatusColumnWidth,NewSettings.StatusColumnWidthType,NewSettings.StatusColumnCount);

		NewSettings.FullScreen = cfg_reader.GetInt("FullScreen", 0);
		NewSettings.AlignExtensions = cfg_reader.GetInt("AlignExtensions", 1);
		NewSettings.FolderAlignExtensions = cfg_reader.GetInt("FolderAlignExtensions", 0);
		NewSettings.FolderUpperCase = cfg_reader.GetInt("FolderUpperCase", 0);
		NewSettings.FileLowerCase = cfg_reader.GetInt("FileLowerCase", 0);
		NewSettings.FileUpperToLowerCase = cfg_reader.GetInt("FileUpperToLowerCase", 0);
		ViewSettingsArray[VIEW_0 + I] = NewSettings;
	}
}


void FileList::SavePanelModes(ConfigWriter &cfg_writer)
{
	for (int I=0; I<10; I++)
	{
		cfg_writer.SelectSectionFmt("Panel/ViewModes/Mode%d", I);

		FARString strColumnTitles, strColumnWidths, strStatusColumnTitles, strStatusColumnWidths;

		PanelViewSettings NewSettings = ViewSettingsArray[VIEW_0+I];
		ViewSettingsToText(NewSettings.ColumnType,NewSettings.ColumnWidth,NewSettings.ColumnWidthType,
		                   NewSettings.ColumnCount,strColumnTitles,strColumnWidths);
		ViewSettingsToText(NewSettings.StatusColumnType,NewSettings.StatusColumnWidth,NewSettings.StatusColumnWidthType,
		                   NewSettings.StatusColumnCount,strStatusColumnTitles,strStatusColumnWidths);

		cfg_writer.SetString("Columns", strColumnTitles.CPtr());
		cfg_writer.SetString("ColumnWidths", strColumnWidths.CPtr());
		cfg_writer.SetString("StatusColumns", strStatusColumnTitles.CPtr());
		cfg_writer.SetString("StatusColumnWidths", strStatusColumnWidths.CPtr());
		cfg_writer.SetInt("FullScreen", NewSettings.FullScreen);
		cfg_writer.SetInt("AlignExtensions", NewSettings.AlignExtensions);
		cfg_writer.SetInt("FolderAlignExtensions", NewSettings.FolderAlignExtensions);
		cfg_writer.SetInt("FolderUpperCase", NewSettings.FolderUpperCase);
		cfg_writer.SetInt("FileLowerCase", NewSettings.FileLowerCase);
		cfg_writer.SetInt("FileUpperToLowerCase", NewSettings.FileUpperToLowerCase);
	}
}
