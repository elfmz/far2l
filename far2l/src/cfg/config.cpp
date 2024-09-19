/*
config.cpp

Конфигурация
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

#include <algorithm>
#include <langinfo.h> // for nl_langinfo
#include "config.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "panel.hpp"
#include "help.hpp"
#include "filefilter.hpp"
#include "poscache.hpp"
#include "findfile.hpp"
#include "hilight.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "panelmix.hpp"
#include "strmix.hpp"
#include "udlist.hpp"
#include "datetime.hpp"
#include "DialogBuilder.hpp"
#include "vtshell.h"
#include "ConfigRW.hpp"
#include "AllXLats.hpp"

Options Opt = {0};

int &Confirmation::ExitEffective()
{
	return WINPORT(ConsoleBackgroundMode)(FALSE) ? ExitOrBknd : Exit;
}

static DWORD ApplyConsoleTweaks()
{
	DWORD64 tweaks = 0;
	if (Opt.ExclusiveCtrlLeft)
		tweaks|= EXCLUSIVE_CTRL_LEFT;
	if (Opt.ExclusiveCtrlRight)
		tweaks|= EXCLUSIVE_CTRL_RIGHT;
	if (Opt.ExclusiveAltLeft)
		tweaks|= EXCLUSIVE_ALT_LEFT;
	if (Opt.ExclusiveAltRight)
		tweaks|= EXCLUSIVE_ALT_RIGHT;
	if (Opt.ExclusiveWinLeft)
		tweaks|= EXCLUSIVE_WIN_LEFT;
	if (Opt.ExclusiveWinRight)
		tweaks|= EXCLUSIVE_WIN_RIGHT;
	if (Opt.ConsolePaintSharp)
		tweaks|= CONSOLE_PAINT_SHARP;
	if (Opt.OSC52ClipSet)
		tweaks|= CONSOLE_OSC52CLIP_SET;
	if (Opt.TTYPaletteOverride)
		tweaks|= CONSOLE_TTY_PALETTE_OVERRIDE;
	return WINPORT(SetConsoleTweaks)(tweaks);
}

static void ApplySudoConfiguration()
{
	const std::string &sudo_app = GetHelperPathName("far2l_sudoapp");
	const std::string &askpass_app = GetHelperPathName("far2l_askpass");

	SudoClientMode mode;
	if (Opt.SudoEnabled) {
		mode = Opt.SudoConfirmModify ? SCM_CONFIRM_MODIFY : SCM_CONFIRM_NONE;
	} else
		mode = SCM_DISABLE;
	sudo_client_configure(mode, Opt.SudoPasswordExpiration, sudo_app.c_str(), askpass_app.c_str(),
			Wide2MB(Msg::SudoTitle).c_str(), Wide2MB(Msg::SudoPrompt).c_str(),
			Wide2MB(Msg::SudoConfirm).c_str());
}

static void AddHistorySettings(DialogBuilder &Builder, FarLangMsg MTitle, int *OptEnabled, int *OptCount)
{
	DialogItemEx *EnabledCheckBox = Builder.AddCheckbox(MTitle, OptEnabled);
	DialogItemEx *CountEdit = Builder.AddIntEditField(OptCount, 6);
	DialogItemEx *CountText = Builder.AddTextBefore(CountEdit, Msg::ConfigMaxHistoryCount);
	CountEdit->Indent(4);
	CountText->Indent(4);
	Builder.LinkFlags(EnabledCheckBox, CountEdit, DIF_DISABLE);
	Builder.LinkFlags(EnabledCheckBox, CountText, DIF_DISABLE);
}

void SanitizeHistoryCounts()
{
	Opt.HistoryCount = std::max(Opt.HistoryCount, 16);
	Opt.FoldersHistoryCount = std::max(Opt.FoldersHistoryCount, 16);
	Opt.ViewHistoryCount = std::max(Opt.ViewHistoryCount, 16);
	Opt.DialogsHistoryCount = std::max(Opt.DialogsHistoryCount, 16);
}

void SanitizeIndentationCounts()
{
	if (Opt.MaxFilenameIndentation > HIGHLIGHT_MAX_MARK_LENGTH)
		Opt.MaxFilenameIndentation = HIGHLIGHT_MAX_MARK_LENGTH;
	if (Opt.MinFilenameIndentation > HIGHLIGHT_MAX_MARK_LENGTH)
		Opt.MinFilenameIndentation = HIGHLIGHT_MAX_MARK_LENGTH;
}

void SystemSettings()
{
	DialogBuilder Builder(Msg::ConfigSystemTitle, L"SystemSettings");

	DialogItemEx *SudoEnabledItem = Builder.AddCheckbox(Msg::ConfigSudoEnabled, &Opt.SudoEnabled);
	DialogItemEx *SudoPasswordExpirationEdit = Builder.AddIntEditField(&Opt.SudoPasswordExpiration, 4);
	DialogItemEx *SudoPasswordExpirationText =
			Builder.AddTextBefore(SudoPasswordExpirationEdit, Msg::ConfigSudoPasswordExpiration);

	SudoPasswordExpirationText->Indent(4);
	SudoPasswordExpirationEdit->Indent(4);

	DialogItemEx *SudoConfirmModifyItem =
			Builder.AddCheckbox(Msg::ConfigSudoConfirmModify, &Opt.SudoConfirmModify);
	SudoConfirmModifyItem->Indent(4);

	Builder.LinkFlags(SudoEnabledItem, SudoConfirmModifyItem, DIF_DISABLE);
	Builder.LinkFlags(SudoEnabledItem, SudoPasswordExpirationEdit, DIF_DISABLE);

	DialogItemEx *DeleteToRecycleBin = Builder.AddCheckbox(Msg::ConfigRecycleBin, &Opt.DeleteToRecycleBin);
	DialogItemEx *DeleteLinks =
			Builder.AddCheckbox(Msg::ConfigRecycleBinLink, &Opt.DeleteToRecycleBinKillLink);
	DeleteLinks->Indent(4);
	Builder.LinkFlags(DeleteToRecycleBin, DeleteLinks, DIF_DISABLE);

	//	Builder.AddCheckbox(MSudoParanoic, &Opt.SudoParanoic);
	//	Builder.AddCheckbox(CopyWriteThrough, &Opt.CMOpt.WriteThrough);
	Builder.AddCheckbox(Msg::ConfigScanJunction, &Opt.ScanJunction);
	Builder.AddCheckbox(Msg::ConfigOnlyFilesSize, &Opt.OnlyFilesSize);

	DialogItemEx *InactivityExit = Builder.AddCheckbox(Msg::ConfigInactivity, &Opt.InactivityExit);
	DialogItemEx *InactivityExitTime = Builder.AddIntEditField(&Opt.InactivityExitTime, 2);
	InactivityExitTime->Indent(4);
	Builder.AddTextAfter(InactivityExitTime, Msg::ConfigInactivityMinutes);
	Builder.LinkFlags(InactivityExit, InactivityExitTime, DIF_DISABLE);

	DialogBuilderListItem CAListItems[] = {
			{Msg::ConfigMakeLinkSuggestByFileDir, 0},
			{Msg::ConfigMakeLinkSuggestSymlinkAlways,  1},
	};
	Builder.AddText(Msg::ConfigMakeLinkSuggest);
	DialogItemEx *MakeLinkSuggest =
		Builder.AddComboBox((int *)&Opt.MakeLinkSuggestSymlinkAlways, 48, CAListItems, ARRAYSIZE(CAListItems),
				DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);
	MakeLinkSuggest->Indent(4);

	Builder.AddSeparator();
	AddHistorySettings(Builder, Msg::ConfigSaveHistory, &Opt.SaveHistory, &Opt.HistoryCount);
	AddHistorySettings(Builder, Msg::ConfigSaveFoldersHistory, &Opt.SaveFoldersHistory,
			&Opt.FoldersHistoryCount);
	AddHistorySettings(Builder, Msg::ConfigSaveViewHistory, &Opt.SaveViewHistory, &Opt.ViewHistoryCount);
	DialogBuilderListItem CAHistRemoveListItems[] = {
			{Msg::ConfigHistoryRemoveDupsRuleNever, 0},
			{Msg::ConfigHistoryRemoveDupsRuleByName, 1},
			{Msg::ConfigHistoryRemoveDupsRuleByNameExtra, 2},
	};
	DialogItemEx *HistRemove =
		Builder.AddComboBox((int *)&Opt.HistoryRemoveDupsRule, 20, CAHistRemoveListItems, ARRAYSIZE(CAHistRemoveListItems),
				DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);
	Builder.AddTextBefore(HistRemove, Msg::ConfigHistoryRemoveDupsRule);

	Builder.AddCheckbox(Msg::ConfigAutoHighlightHistory, &Opt.AutoHighlightHistory);
	Builder.AddSeparator();

	Builder.AddCheckbox(Msg::ConfigAutoSave, &Opt.AutoSaveSetup);
	Builder.AddOKCancel();

	if (Builder.ShowDialog()) {
		SanitizeHistoryCounts();
		ApplySudoConfiguration();
	}
}

void PanelSettings()
{
	DialogBuilder Builder(Msg::ConfigPanelTitle, L"PanelSettings");
	BOOL AutoUpdate = (Opt.AutoUpdateLimit);

	Builder.AddCheckbox(Msg::ConfigHidden, &Opt.ShowHidden);

	DialogItemEx *CbHighlight = Builder.AddCheckbox(Msg::ConfigHighlight, &Opt.Highlight);
	DialogItemEx *CbShowFilenameMarks = Builder.AddCheckbox(Msg::ConfigFilenameMarks, &Opt.ShowFilenameMarks);
	CbShowFilenameMarks->Indent(1);
	Builder.LinkFlags(CbHighlight, CbShowFilenameMarks, DIF_DISABLE);
	DialogItemEx *CbFilenameMarksAlign = Builder.AddCheckbox(Msg::ConfigFilenameMarksAlign, &Opt.FilenameMarksAlign);
	CbFilenameMarksAlign->Indent(2);
	Builder.LinkFlags(CbHighlight, CbFilenameMarksAlign, DIF_DISABLE);
	DialogItemEx *IndentationMinEdit = Builder.AddIntEditField((int *)&Opt.MinFilenameIndentation, 2);
	Builder.AddTextAfter(IndentationMinEdit, Msg::ConfigFilenameMinIndentation);
	DialogItemEx *IndentationMaxEdit = Builder.AddIntEditField((int *)&Opt.MaxFilenameIndentation, 2);
	Builder.AddTextAfter(IndentationMaxEdit, Msg::ConfigFilenameMaxIndentation);

	Builder.AddCheckbox(Msg::ConfigAutoChange, &Opt.Tree.AutoChangeFolder);
	Builder.AddCheckbox(Msg::ConfigSelectFolders, &Opt.SelectFolders);
	Builder.AddCheckbox(Msg::ConfigCaseSensitiveCompareSelect, &Opt.PanelCaseSensitiveCompareSelect);
	Builder.AddCheckbox(Msg::ConfigSortFolderExt, &Opt.SortFolderExt);
	Builder.AddCheckbox(Msg::ConfigReverseSort, &Opt.ReverseSort);

	DialogItemEx *AutoUpdateEnabled = Builder.AddCheckbox(Msg::ConfigAutoUpdateLimit, &AutoUpdate);
	DialogItemEx *AutoUpdateLimit = Builder.AddIntEditField((int *)&Opt.AutoUpdateLimit, 6);
	Builder.LinkFlags(AutoUpdateEnabled, AutoUpdateLimit, DIF_DISABLE, false);
	DialogItemEx *AutoUpdateText = Builder.AddTextBefore(AutoUpdateLimit, Msg::ConfigAutoUpdateLimit2);
	AutoUpdateLimit->Indent(4);
	AutoUpdateText->Indent(4);
	Builder.AddCheckbox(Msg::ConfigAutoUpdateRemoteDrive, &Opt.AutoUpdateRemoteDrive);

	Builder.AddSeparator();
	Builder.AddCheckbox(Msg::ConfigShowColumns, &Opt.ShowColumnTitles);
	Builder.AddCheckbox(Msg::ConfigShowStatus, &Opt.ShowPanelStatus);
	Builder.AddCheckbox(Msg::ConfigShowTotal, &Opt.ShowPanelTotals);
	Builder.AddCheckbox(Msg::ConfigShowFree, &Opt.ShowPanelFree);
	Builder.AddCheckbox(Msg::ConfigShowScrollbar, &Opt.ShowPanelScrollbar);
	Builder.AddCheckbox(Msg::ConfigShowScreensNumber, &Opt.ShowScreensNumber);
	Builder.AddCheckbox(Msg::ConfigShowSortMode, &Opt.ShowSortMode);
	Builder.AddOKCancel();

	if (Builder.ShowDialog()) {
		if (!AutoUpdate)
			Opt.AutoUpdateLimit = 0;

		SanitizeIndentationCounts();

		// FrameManager->RefreshFrame();
		CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
		CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
		CtrlObject->Cp()->Redraw();
	}
}

void InputSettings()
{
	const DWORD supported_tweaks = ApplyConsoleTweaks();

	std::vector<DialogBuilderListItem> XLatItems;
	AllXlats xlats;

	int SelectedXLat = -1;
	for (int i = 0; i < (int)xlats.size(); ++i) {
		if (Opt.XLat.XLat == xlats[i]) {
			SelectedXLat = i;
		}
		XLatItems.emplace_back(DialogBuilderListItem{FarLangMsg{::Lang.InternMsg(xlats[i].c_str())}, i});
	}

	DialogBuilder Builder(Msg::ConfigInputTitle, L"InputSettings");
	Builder.AddCheckbox(Msg::ConfigMouse, &Opt.Mouse);

	Builder.AddText(Msg::ConfigXLats);
	DialogItemEx *Item = Builder.AddComboBox(&SelectedXLat, 40, XLatItems.data(), XLatItems.size(),
			DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);
	Item->Indent(4);

	Builder.AddCheckbox(Msg::ConfigXLatFastFileFind, &Opt.XLat.EnableForFastFileFind);
	Builder.AddCheckbox(Msg::ConfigXLatDialogs, &Opt.XLat.EnableForDialogs);

	if (supported_tweaks & TWEAK_STATUS_SUPPORT_EXCLUSIVE_KEYS) {
		Builder.AddText(Msg::ConfigExclusiveKeys);
		Item = Builder.AddCheckbox(Msg::ConfigExclusiveCtrlLeft, &Opt.ExclusiveCtrlLeft);
		Item->Indent(4);
		Builder.AddCheckboxAfter(Item, Msg::ConfigExclusiveCtrlRight, &Opt.ExclusiveCtrlRight);

		Item = Builder.AddCheckbox(Msg::ConfigExclusiveAltLeft, &Opt.ExclusiveAltLeft);
		Item->Indent(4);
		Builder.AddCheckboxAfter(Item, Msg::ConfigExclusiveAltRight, &Opt.ExclusiveAltRight);

		Item = Builder.AddCheckbox(Msg::ConfigExclusiveWinLeft, &Opt.ExclusiveWinLeft);
		Item->Indent(4);
		Builder.AddCheckboxAfter(Item, Msg::ConfigExclusiveWinRight, &Opt.ExclusiveWinRight);
	}

	Builder.AddOKCancel();

	if (Builder.ShowDialog()) {
		if (size_t(SelectedXLat) < xlats.size()) {
			Opt.XLat.XLat = xlats[SelectedXLat];
		}
		ApplyConsoleTweaks();
	}
}

/*
	$ 17.12.2001 IS
	Настройка средней кнопки мыши для панелей. Воткнем пока сюда, потом надо
	переехать в специальный диалог по программированию мыши.
*/
void InterfaceSettings()
{
	int DateFormatIndex = GetDateFormat(); //Opt.DateFormat
	FARString strDateSeparator; //Opt.strDateSeparator
	FARString strTimeSeparator; //Opt.strTimeSeparator
	FARString strDecimalSeparator; //Opt.strDecimalSeparator
	strDateSeparator = GetDateSeparator();
	strTimeSeparator = GetTimeSeparator();
	strDecimalSeparator = GetDecimalSeparator();

	for (;;) {
		DialogBuilder Builder(Msg::ConfigInterfaceTitle, L"InterfSettings");

		Builder.AddCheckbox(Msg::ConfigClock, &Opt.Clock);
		Builder.AddCheckbox(Msg::ConfigViewerEditorClock, &Opt.ViewerEditorClock);
		Builder.AddCheckbox(Msg::ConfigKeyBar, &Opt.ShowKeyBar);
		Builder.AddCheckbox(Msg::ConfigMenuBar, &Opt.ShowMenuBar);
		DialogItemEx *SaverCheckbox = Builder.AddCheckbox(Msg::ConfigSaver, &Opt.ScreenSaver);

		DialogItemEx *SaverEdit = Builder.AddIntEditField(&Opt.ScreenSaverTime, 2);
		SaverEdit->Indent(4);
		Builder.AddTextAfter(SaverEdit, Msg::ConfigSaverMinutes);
		Builder.LinkFlags(SaverCheckbox, SaverEdit, DIF_DISABLE);

		Builder.AddCheckbox(Msg::ConfigCopyTotal, &Opt.CMOpt.CopyShowTotal);
		Builder.AddCheckbox(Msg::ConfigCopyTimeRule, &Opt.CMOpt.CopyTimeRule);
		Builder.AddCheckbox(Msg::ConfigDeleteTotal, &Opt.DelOpt.DelShowTotal);
		Builder.AddCheckbox(Msg::ConfigPgUpChangeDisk, &Opt.PgUpChangeDisk);

		Builder.AddSeparator(Msg::ConfigDateFormat);

		/*DialogBuilderListItem CAListItems[] = {
				{Msg::ConfigDateFormatMDY, 0},
				{Msg::ConfigDateFormatDMY, 1},
				{Msg::ConfigDateFormatYMD, 2},
		};
		DialogItemEx *DateFormatComboBox = Builder.AddComboBox((int *)&DateFormatIndex, 10,
				CAListItems, ARRAYSIZE(CAListItems),
				DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);
		DialogItemEx *DateFormatText = Builder.AddTextAfter(DateFormatComboBox, Msg::ConfigDateFormat);
		DateFormatText->Indent(1);
		*/
		static FarLangMsg DateFormatOptions[] = {Msg::ConfigDateFormatMDY, Msg::ConfigDateFormatDMY,
				Msg::ConfigDateFormatYMD};
		Builder.AddRadioButtonsHorz(&DateFormatIndex, ARRAYSIZE(DateFormatOptions), DateFormatOptions);

		Builder.StartColumns();
		DialogItemEx *DateSeparatorEdit = Builder.AddEditField(&strDateSeparator, 0);
		DateSeparatorEdit->Type = DI_FIXEDIT;
		DateSeparatorEdit->Flags |= DIF_MASKEDIT;
		DateSeparatorEdit->strMask = L"X";
		Builder.AddTextAfter(DateSeparatorEdit, Msg::ConfigDateSeparator);

		DialogItemEx *TimeSeparatorEdit = Builder.AddEditField(&strTimeSeparator, 0);
		TimeSeparatorEdit->Type = DI_FIXEDIT;
		TimeSeparatorEdit->Flags |= DIF_MASKEDIT;
		TimeSeparatorEdit->strMask = L"X";
		Builder.AddTextAfter(TimeSeparatorEdit, Msg::ConfigTimeSeparator);

		DialogItemEx *DecimalSeparatorEdit = Builder.AddEditField(&strDecimalSeparator, 0);
		DecimalSeparatorEdit->Type = DI_FIXEDIT;
		DecimalSeparatorEdit->Flags |= DIF_MASKEDIT;
		DecimalSeparatorEdit->strMask = L"X";
		Builder.AddTextAfter(DecimalSeparatorEdit, Msg::ConfigDecimalSeparator);
        
		Builder.ColumnBreak();
		int DateTimeDefaultID = -1;
		Builder.AddButton(Msg::ConfigDateTimeDefault, DateTimeDefaultID);
		int DateTimeCurrentID = -1;
		Builder.AddButton(Msg::ConfigDateTimeCurrent, DateTimeCurrentID);
		int DateTimeFromSystemID = -1;
		Builder.AddButton(Msg::ConfigDateTimeFromSystem, DateTimeFromSystemID);
		Builder.EndColumns();

		Builder.AddSeparator();

		const DWORD supported_tweaks = ApplyConsoleTweaks();
		if (supported_tweaks & TWEAK_STATUS_SUPPORT_BLINK_RATE) {

			DialogItemEx *CursorEdit = Builder.AddIntEditField(&Opt.CursorBlinkTime, 3);
			Builder.AddTextAfter(CursorEdit, Msg::ConfigCursorBlinkInt);
		}

		int ChangeFontID = -1;
		DialogItemEx *ChangeFontItem = nullptr;
		if (supported_tweaks & TWEAK_STATUS_SUPPORT_PAINT_SHARP) {
			ChangeFontItem = Builder.AddButton(Msg::ConfigConsoleChangeFont, ChangeFontID);
		}

		if (supported_tweaks & TWEAK_STATUS_SUPPORT_PAINT_SHARP) {
			if (ChangeFontItem)
				Builder.AddCheckboxAfter(ChangeFontItem, Msg::ConfigConsolePaintSharp,
						&Opt.ConsolePaintSharp);
			else
				Builder.AddCheckbox(Msg::ConfigConsolePaintSharp, &Opt.ConsolePaintSharp);
		}

		if (supported_tweaks & TWEAK_STATUS_SUPPORT_OSC52CLIP_SET) {
			Builder.AddCheckbox(Msg::ConfigOSC52ClipSet, &Opt.OSC52ClipSet);
		}

		if (supported_tweaks & TWEAK_STATUS_SUPPORT_TTY_PALETTE) {
			Builder.AddCheckbox(Msg::ConfigTTYPaletteOverride, &Opt.TTYPaletteOverride);
		}

		Builder.AddText(Msg::ConfigWindowTitle);
		Builder.AddEditField(&Opt.strWindowTitle, 47);

		// OKButton->Flags = DIF_CENTERGROUP;
		// OKButton->DefaultButton = TRUE;
		// OKButton->Y1 = OKButton->Y2 = NextY++;
		// OKButtonID = DialogItemsCount-1;

		Builder.AddOKCancel();

		int clicked_id = -1;
		if (Builder.ShowDialog(&clicked_id)) {
			if (Opt.CMOpt.CopyTimeRule)
				Opt.CMOpt.CopyTimeRule = 3;

			Opt.DateFormat = DateFormatIndex;
			Opt.strDateSeparator = strDateSeparator;
			Opt.strTimeSeparator = strTimeSeparator;
			Opt.strDecimalSeparator = strDecimalSeparator;
			ConvertDate_ResetInit();

			SetFarConsoleMode();
			CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->SetScreenPosition();
			// $ 10.07.2001 SKV ! надо это делать, иначе если кейбар спрятали, будет полный рамс.
			CtrlObject->Cp()->Redraw();

			ApplyConsoleTweaks();
			WINPORT(SetConsoleCursorBlinkTime)(NULL, Opt.CursorBlinkTime);
			break;
		}

		if (ChangeFontID != -1 && clicked_id == ChangeFontID)
			WINPORT(ConsoleChangeFont)();

		else if (clicked_id == DateTimeDefaultID) {
			DateFormatIndex = GetDateFormatDefault();
			strDateSeparator = GetDateSeparatorDefault();
			strTimeSeparator = GetTimeSeparatorDefault();
			strDecimalSeparator = GetDecimalSeparatorDefault();
		}

		else if (clicked_id == DateTimeCurrentID) {
			DateFormatIndex = GetDateFormat();
			strDateSeparator = GetDateSeparator();
			strTimeSeparator = GetTimeSeparator();
			strDecimalSeparator = GetDecimalSeparator();
		}

		else if (clicked_id == DateTimeFromSystemID) {
			// parcing part of possible https://help.gnome.org/users/gthumb/stable/gthumb-date-formats.html
			std::string::size_type
					pos_date_2 = std::string::npos,
					pos_day = std::string::npos,
					pos_month = std::string::npos,
					pos_year = std::string::npos,
					pos_time_2 = std::string::npos;
			size_t length_decimal;
			std::string format_date = nl_langinfo(D_FMT);
			std::string format_time = nl_langinfo(T_FMT);
			std::string format_decimal = nl_langinfo(RADIXCHAR/*DECIMAL_POINT*/);
			if (format_date=="%D") { // %D Equivalent to %m/%d/%y
				DateFormatIndex = 0;
				strTimeSeparator = "/";
				pos_date_2 = 0; // for not error in message
			}
			if (format_date=="%F") { // %F Equivalent to %Y-%m-%d
				DateFormatIndex = 2;
				strTimeSeparator = "-";
				pos_date_2 = 0; // for not error in message
			}
			else if (format_date.length() >= 8) {
				std::vector<const char*> codes_day = { "%d", "%e", "%Ed", "%Ee", "%Od", "%Oe" };
				for (const auto &code : codes_day) {
					pos_day = format_date.find(code);
					if (pos_day != std::string::npos)
						break;
				}
				std::vector<const char*> codes_month = {
					"%m", "%B", "%b", "%h", "%Em", "%EB", "%Eb", "%Eh", "%Om", "%OB", "%Ob", "%Oh" };
				for (const auto &code : codes_month) {
					pos_month = format_date.find(code);
					if (pos_month != std::string::npos)
						break;
				}
				std::vector<const char*> codes_year = {
					"%Y", "%y", "%G", "%g", "%EY", "%Ey", "%EG", "%Eg", "%OY", "%Oy", "%OG", "%Og" };
				for (const auto &code : codes_year) {
					pos_year = format_date.find(code);
					if (pos_year != std::string::npos)
						break;
				}
				if (pos_day != std::string::npos && pos_month != std::string::npos && pos_year != std::string::npos) {
					if (pos_day < pos_month && pos_month < pos_year) // day-month-year
					{ DateFormatIndex = 1; pos_date_2 = pos_month; }
					else if (pos_year < pos_month && pos_month < pos_day) // year-month-day
					{ DateFormatIndex = 2; pos_date_2 = pos_month; }
					else if (pos_month < pos_day  && pos_month < pos_year) // month-day-year
					{ DateFormatIndex = 0; pos_date_2 = pos_day; }
				}
			}
			if (pos_date_2 != std::string::npos)
				strDateSeparator = format_date[pos_date_2-1];

			if (format_time=="%T") { // %T The time in 24-hour notation (%H:%M:%S).
				strTimeSeparator = ":";
				pos_time_2 = 0; // for not error in message
			}
			else {
				pos_day = format_time.find("%");
				if (pos_day != std::string::npos) {
					pos_time_2 = format_time.find("%", pos_day+2);
					if (pos_time_2 != std::string::npos)
						strTimeSeparator = format_time[pos_time_2-1];
				}
			}

			length_decimal = format_decimal.length();
			if (length_decimal > 0)
				strDecimalSeparator = format_decimal[0];

			ExMessager em;
			em.Add(L"From system locale");
			em.AddFormat(L"Date format from locale:      \"%s\"", format_date.c_str());
			em.AddFormat(L"  Date order:        %s (order %d)",
				(pos_date_2 != std::string::npos) ? "imported" : "did not changed",
				DateFormatIndex);
			em.AddFormat(L"  Date separator:    %s (\'%ls\')",
				(pos_date_2 != std::string::npos) ? "imported" : "did not changed",
				strDateSeparator.CPtr());
			em.AddFormat(L"Time format from locale:      \"%s\"", format_time.c_str());
			em.AddFormat(L"  Time separator:    %s (\'%ls\')",
				(pos_time_2 != std::string::npos) ? "imported" : "did not changed",
				 strTimeSeparator.CPtr());
			em.AddFormat(L"DecimalSeparator from locale: \"%s\"", format_decimal.c_str());
			em.AddFormat(L"  Decimal separator: %s (\'%ls\')",
				length_decimal>0 ? "imported" : "did not changed",
				strDecimalSeparator.CPtr());
			em.Add(Msg::Ok);
			em.Show(MSG_LEFTALIGN |
					( (pos_date_2 == std::string::npos
						|| pos_time_2 == std::string::npos
						|| length_decimal<=0 )
					? MSG_WARNING : 0),
				1);
		}
		else
			break;
	}
}

void AutoCompleteSettings()
{
	DialogBuilder Builder(Msg::ConfigAutoCompleteTitle, L"AutoCompleteSettings");
	DialogItemEx *ListCheck =
			Builder.AddCheckbox(Msg::ConfigAutoCompleteShowList, &Opt.AutoComplete.ShowList);
	DialogItemEx *ModalModeCheck =
			Builder.AddCheckbox(Msg::ConfigAutoCompleteModalList, &Opt.AutoComplete.ModalList);
	ModalModeCheck->Indent(4);
	Builder.AddCheckbox(Msg::ConfigAutoCompleteAutoAppend, &Opt.AutoComplete.AppendCompletion);
	Builder.LinkFlags(ListCheck, ModalModeCheck, DIF_DISABLE);

	Builder.AddText(Msg::ConfigAutoCompleteExceptions);
	Builder.AddEditField(&Opt.AutoComplete.Exceptions, 47);

	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void InfoPanelSettings()
{

	DialogBuilder Builder(Msg::ConfigInfoPanelTitle, L"InfoPanelSettings");
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void DialogSettings()
{
	DialogBuilder Builder(Msg::ConfigDlgSetsTitle, L"DialogSettings");

	AddHistorySettings(Builder, Msg::ConfigDialogsEditHistory, &Opt.Dialogs.EditHistory,
			&Opt.DialogsHistoryCount);
	Builder.AddCheckbox(Msg::ConfigDialogsEditBlock, &Opt.Dialogs.EditBlock);
	Builder.AddCheckbox(Msg::ConfigDialogsDelRemovesBlocks, &Opt.Dialogs.DelRemovesBlocks);
	Builder.AddCheckbox(Msg::ConfigDialogsAutoComplete, &Opt.Dialogs.AutoComplete);
	Builder.AddCheckbox(Msg::ConfigDialogsEULBsClear, &Opt.Dialogs.EULBsClear);
	Builder.AddCheckbox(Msg::ConfigDialogsMouseButton, &Opt.Dialogs.MouseButton);
	Builder.AddOKCancel();

	if (Builder.ShowDialog()) {
		SanitizeHistoryCounts();
		if (Opt.Dialogs.MouseButton)
			Opt.Dialogs.MouseButton = 0xFFFF;
	}
}

void VMenuSettings()
{
	DialogBuilderListItem CAListItems[] = {
			{Msg::ConfigVMenuClickCancel, VMENUCLICK_CANCEL},
			{Msg::ConfigVMenuClickApply,  VMENUCLICK_APPLY },
			{Msg::ConfigVMenuClickIgnore, VMENUCLICK_IGNORE},
	};

	DialogBuilder Builder(Msg::ConfigVMenuTitle, L"VMenuSettings");

	Builder.AddText(Msg::ConfigVMenuLBtnClick);
	Builder.AddComboBox((int *)&Opt.VMenu.LBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems),
			DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);
	Builder.AddText(Msg::ConfigVMenuRBtnClick);
	Builder.AddComboBox((int *)&Opt.VMenu.RBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems),
			DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);
	Builder.AddText(Msg::ConfigVMenuMBtnClick);
	Builder.AddComboBox((int *)&Opt.VMenu.MBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems),
			DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);

	Builder.AddCheckbox(Msg::ConfigVMenuStopEdge, (BOOL *)&Opt.VMenu.MenuLoopScroll);

	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void CmdlineSettings()
{
	DialogBuilderListItem CMWListItems[] = {
			{Msg::ConfigCmdlineWaitKeypress_Never,   0},
			{Msg::ConfigCmdlineWaitKeypress_OnError, 1},
			{Msg::ConfigCmdlineWaitKeypress_Always,  2},
	};

	DialogBuilder Builder(Msg::ConfigCmdlineTitle, L"CmdlineSettings");
	AddHistorySettings(Builder, Msg::ConfigSaveHistory, &Opt.SaveHistory, &Opt.HistoryCount);
	Builder.AddCheckbox(Msg::ConfigCmdlineEditBlock, &Opt.CmdLine.EditBlock);
	Builder.AddCheckbox(Msg::ConfigCmdlineDelRemovesBlocks, &Opt.CmdLine.DelRemovesBlocks);
	Builder.AddCheckbox(Msg::ConfigCmdlineAutoComplete, &Opt.CmdLine.AutoComplete);
	Builder.AddCheckbox(Msg::ConfigCmdlineSplitter, &Opt.CmdLine.Splitter);

	DialogItemEx *LimitEdit = Builder.AddIntEditField(&Opt.CmdLine.VTLogLimit, 6);
	Builder.AddTextBefore(LimitEdit, Msg::ConfigCmdlineVTLogLimit);

	Builder.AddText(Msg::ConfigCmdlineWaitKeypress);
	Builder.AddComboBox((int *)&Opt.CmdLine.WaitKeypress, 40, CMWListItems, ARRAYSIZE(CMWListItems),
			DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);

	DialogItemEx *UsePromptFormat =
			Builder.AddCheckbox(Msg::ConfigCmdlineUsePromptFormat, &Opt.CmdLine.UsePromptFormat);
	DialogItemEx *PromptFormat = Builder.AddEditField(&Opt.CmdLine.strPromptFormat, 19);
	PromptFormat->Indent(4);
	Builder.LinkFlags(UsePromptFormat, PromptFormat, DIF_DISABLE);
	DialogItemEx *UseShell = Builder.AddCheckbox(Msg::ConfigCmdlineUseShell, &Opt.CmdLine.UseShell);
	DialogItemEx *Shell = Builder.AddEditField(&Opt.CmdLine.strShell, 19);
	Shell->Indent(4);
	Builder.LinkFlags(UseShell, Shell, DIF_DISABLE);
	Builder.AddOKCancel();

	int oldUseShell = Opt.CmdLine.UseShell;
	FARString oldShell = FARString(Opt.CmdLine.strShell);

	if (Builder.ShowDialog()) {
		SanitizeHistoryCounts();

		CtrlObject->CmdLine->SetPersistentBlocks(Opt.CmdLine.EditBlock);
		CtrlObject->CmdLine->SetDelRemovesBlocks(Opt.CmdLine.DelRemovesBlocks);
		CtrlObject->CmdLine->SetAutoComplete(Opt.CmdLine.AutoComplete);

		if (Opt.CmdLine.UseShell != oldUseShell || Opt.CmdLine.strShell != oldShell)
			VTShell_Shutdown();
	}
}

void SetConfirmations()
{
	DialogBuilder Builder(Msg::SetConfirmTitle, L"ConfirmDlg");

	Builder.AddCheckbox(Msg::SetConfirmCopy, &Opt.Confirm.Copy);
	Builder.AddCheckbox(Msg::SetConfirmMove, &Opt.Confirm.Move);
	Builder.AddCheckbox(Msg::SetConfirmRO, &Opt.Confirm.RO);
	Builder.AddCheckbox(Msg::SetConfirmDelete, &Opt.Confirm.Delete);
	Builder.AddCheckbox(Msg::SetConfirmDeleteFolders, &Opt.Confirm.DeleteFolder);
	Builder.AddCheckbox(Msg::SetConfirmClearVT, &Opt.Confirm.ClearVT);
	Builder.AddCheckbox(Msg::SetConfirmEsc, &Opt.Confirm.Esc);
	Builder.AddCheckbox(Msg::SetConfirmRemoveConnection, &Opt.Confirm.RemoveConnection);
	Builder.AddCheckbox(Msg::SetConfirmRemoveHotPlug, &Opt.Confirm.RemoveHotPlug);
	Builder.AddCheckbox(Msg::SetConfirmAllowReedit, &Opt.Confirm.AllowReedit);
	Builder.AddCheckbox(Msg::SetConfirmHistoryClear, &Opt.Confirm.HistoryClear);
	Builder.AddCheckbox(Msg::SetConfirmExit, &Opt.Confirm.ExitEffective());
	Builder.AddOKCancel();

	Builder.ShowDialog();
}

void PluginsManagerSettings()
{
	DialogBuilder Builder(Msg::PluginsManagerSettingsTitle, L"PluginsManagerSettings");

	Builder.AddCheckbox(Msg::PluginsManagerScanSymlinks, &Opt.LoadPlug.ScanSymlinks);
	Builder.AddText(Msg::PluginsManagerPersonalPath);
	Builder.AddEditField(&Opt.LoadPlug.strPersonalPluginsPath, 45, L"PersPath", DIF_EDITPATH);

	Builder.AddSeparator(Msg::PluginConfirmationTitle);
	DialogItemEx *ConfirmOFP = Builder.AddCheckbox(Msg::PluginsManagerOFP, &Opt.PluginConfirm.OpenFilePlugin);
	ConfirmOFP->Flags|= DIF_3STATE;
	DialogItemEx *StandardAssoc =
			Builder.AddCheckbox(Msg::PluginsManagerStdAssoc, &Opt.PluginConfirm.StandardAssociation);
	DialogItemEx *EvenIfOnlyOne =
			Builder.AddCheckbox(Msg::PluginsManagerEvenOne, &Opt.PluginConfirm.EvenIfOnlyOnePlugin);
	StandardAssoc->Indent(2);
	EvenIfOnlyOne->Indent(4);

	Builder.AddCheckbox(Msg::PluginsManagerSFL, &Opt.PluginConfirm.SetFindList);
	Builder.AddCheckbox(Msg::PluginsManagerPF, &Opt.PluginConfirm.Prefix);
	Builder.AddOKCancel();

	Builder.ShowDialog();
}

void SetDizConfig()
{
	DialogBuilder Builder(Msg::CfgDizTitle, L"FileDiz");

	Builder.AddText(Msg::CfgDizListNames);
	Builder.AddEditField(&Opt.Diz.strListNames, 65);
	Builder.AddSeparator();

	Builder.AddCheckbox(Msg::CfgDizSetHidden, &Opt.Diz.SetHidden);
	Builder.AddCheckbox(Msg::CfgDizROUpdate, &Opt.Diz.ROUpdate);
	DialogItemEx *StartPos = Builder.AddIntEditField(&Opt.Diz.StartPos, 2);
	Builder.AddTextAfter(StartPos, Msg::CfgDizStartPos);
	Builder.AddSeparator();

	static FarLangMsg DizOptions[] = {Msg::CfgDizNotUpdate, Msg::CfgDizUpdateIfDisplayed,
			Msg::CfgDizAlwaysUpdate};
	Builder.AddRadioButtons(&Opt.Diz.UpdateMode, 3, DizOptions);
	Builder.AddSeparator();

	Builder.AddCheckbox(Msg::CfgDizAnsiByDefault, &Opt.Diz.AnsiByDefault);
	Builder.AddCheckbox(Msg::CfgDizSaveInUTF, &Opt.Diz.SaveInUTF);
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void ViewerConfig(ViewerOptions &ViOpt, bool Local)
{
	DialogBuilder Builder(Msg::ViewConfigTitle, L"ViewerSettings");

	if (!Local) {
		Builder.AddCheckbox(Msg::ViewConfigExternalF3, &Opt.ViOpt.UseExternalViewer);
		Builder.AddText(Msg::ViewConfigExternalCommand);
		Builder.AddEditField(&Opt.strExternalViewer, 64, L"ExternalViewer", DIF_EDITPATH);
		Builder.AddSeparator(Msg::ViewConfigInternal);
	}

	Builder.StartColumns();
	Builder.AddCheckbox(Msg::ViewConfigPersistentSelection, &ViOpt.PersistentBlocks);
	DialogItemEx *SavePos = Builder.AddCheckbox(Msg::ViewConfigSavePos, &Opt.ViOpt.SavePos);
	DialogItemEx *TabSize = Builder.AddIntEditField(&ViOpt.TabSize, 3);
	Builder.AddTextAfter(TabSize, Msg::ViewConfigTabSize);
	Builder.AddCheckbox(Msg::ViewShowKeyBar, &ViOpt.ShowKeyBar);
	Builder.ColumnBreak();
	Builder.AddCheckbox(Msg::ViewConfigArrows, &ViOpt.ShowArrows);
	DialogItemEx *SaveShortPos = Builder.AddCheckbox(Msg::ViewConfigSaveShortPos, &Opt.ViOpt.SaveShortPos);
	Builder.LinkFlags(SavePos, SaveShortPos, DIF_DISABLE);
	Builder.AddCheckbox(Msg::ViewConfigScrollbar, &ViOpt.ShowScrollbar);
	Builder.AddCheckbox(Msg::ViewShowTitleBar, &ViOpt.ShowTitleBar);
	Builder.EndColumns();

	if (!Local) {
		Builder.AddEmptyLine();
		Builder.AddCheckbox(Msg::ViewAutoDetectCodePage, &ViOpt.AutoDetectCodePage);
		Builder.AddText(Msg::ViewConfigDefaultCodePage);
		Builder.AddCodePagesBox(&ViOpt.DefaultCodePage, 40, false, false);
	}
	Builder.AddOKCancel();
	if (Builder.ShowDialog()) {
		if (ViOpt.TabSize < 1 || ViOpt.TabSize > 512)
			ViOpt.TabSize = 8;
	}
}

void EditorConfig(EditorOptions &EdOpt, bool Local, int EdCfg_ExpandTabs, int EdCfg_TabSize)
{
	DialogBuilder Builder(Msg::EditConfigTitle, L"EditorSettings");
	if (!Local) {
		Builder.AddCheckbox(Msg::EditConfigEditorF4, &Opt.EdOpt.UseExternalEditor);
		Builder.AddText(Msg::EditConfigEditorCommand);
		Builder.AddEditField(&Opt.strExternalEditor, 64, L"ExternalEditor", DIF_EDITPATH);
		Builder.AddSeparator(Msg::EditConfigInternal);
	}

	Builder.AddText(Msg::EditConfigExpandTabsTitle);
	DialogBuilderListItem ExpandTabsItems[] = {
		{Msg::EditConfigDoNotExpandTabs,        EXPAND_NOTABS },
		{Msg::EditConfigExpandTabs,             EXPAND_NEWTABS},
		{Msg::EditConfigConvertAllTabsToSpaces, EXPAND_ALLTABS}
	};
	Builder.AddComboBox(&EdOpt.ExpandTabs, 64, ExpandTabsItems, 3,
			DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE
			| (Local && EdCfg_ExpandTabs >= 0 ? DIF_DISABLE : 0) );

	Builder.StartColumns();
	Builder.AddCheckbox(Msg::EditConfigPersistentBlocks, &EdOpt.PersistentBlocks);
	DialogItemEx *SavePos = Builder.AddCheckbox(Msg::EditConfigSavePos, &EdOpt.SavePos);
	Builder.AddCheckbox(Msg::EditConfigAutoIndent, &EdOpt.AutoIndent);
	DialogItemEx *TabSize = Builder.AddIntEditField(&EdOpt.TabSize, 3,
		(Local && EdCfg_TabSize > 0 ? DIF_DISABLE : 0) );
	Builder.AddTextAfter(TabSize, Msg::EditConfigTabSize);
	Builder.AddCheckbox(Msg::EditShowWhiteSpace, &EdOpt.ShowWhiteSpace);
	Builder.AddCheckbox(Msg::EditShowKeyBar, &EdOpt.ShowKeyBar);
	Builder.ColumnBreak();
	Builder.AddCheckbox(Msg::EditConfigDelRemovesBlocks, &EdOpt.DelRemovesBlocks);
	DialogItemEx *SaveShortPos = Builder.AddCheckbox(Msg::EditConfigSaveShortPos, &EdOpt.SaveShortPos);
	Builder.LinkFlags(SavePos, SaveShortPos, DIF_DISABLE);
	Builder.AddCheckbox(Msg::EditCursorBeyondEnd, &EdOpt.CursorBeyondEOL);
	Builder.AddCheckbox(Msg::EditConfigScrollbar, &EdOpt.ShowScrollBar);
	Builder.AddCheckbox(Msg::EditConfigPickUpWord, &EdOpt.SearchPickUpWord);
	Builder.AddCheckbox(Msg::EditShowTitleBar, &EdOpt.ShowTitleBar);
	Builder.EndColumns();

	if (!Local) {
		Builder.AddEmptyLine();
		Builder.AddCheckbox(Msg::EditUseEditorConfigOrg, &EdOpt.UseEditorConfigOrg);
		Builder.AddCheckbox(Msg::EditLockROFileModification, &EdOpt.ReadOnlyLock, 1);
		Builder.AddCheckbox(Msg::EditWarningBeforeOpenROFile, &EdOpt.ReadOnlyLock, 2);
		Builder.AddCheckbox(Msg::EditAutoDetectCodePage, &EdOpt.AutoDetectCodePage);
		Builder.AddText(Msg::EditConfigDefaultCodePage);
		Builder.AddCodePagesBox(&EdOpt.DefaultCodePage, 40, false, false);
	}

	Builder.AddOKCancel();

	if (Builder.ShowDialog()) {
		if (EdOpt.TabSize < 1 || EdOpt.TabSize > 512)
			EdOpt.TabSize = 8;
	}
}

void NotificationsConfig(NotificationsOptions &NotifOpt)
{
	DialogBuilder Builder(Msg::NotifConfigTitle, L"NotificationsSettings");

	Builder.AddCheckbox(Msg::NotifConfigOnFileOperation, &NotifOpt.OnFileOperation);
	Builder.AddCheckbox(Msg::NotifConfigOnConsole, &NotifOpt.OnConsole);
	Builder.AddEmptyLine();
	Builder.AddCheckbox(Msg::NotifConfigOnlyIfBackground, &NotifOpt.OnlyIfBackground);
	Builder.AddOKCancel();

	if (Builder.ShowDialog()) {
		// nothing to do/sanitize here
	}
}

void SetFolderInfoFiles()
{
	FARString strFolderInfoFiles;

	if (GetString(Msg::SetFolderInfoTitle, Msg::SetFolderInfoNames, L"FolderInfoFiles",
				Opt.InfoPanel.strFolderInfoFiles, strFolderInfoFiles, L"OptMenu",
				FIB_ENABLEEMPTY | FIB_BUTTONS)) {
		Opt.InfoPanel.strFolderInfoFiles = strFolderInfoFiles;

		if (CtrlObject->Cp()->LeftPanel->GetType() == INFO_PANEL)
			CtrlObject->Cp()->LeftPanel->Update(0);

		if (CtrlObject->Cp()->RightPanel->GetType() == INFO_PANEL)
			CtrlObject->Cp()->RightPanel->Update(0);
	}
}

void ApplyConfig()
{
	ApplySudoConfiguration();
	ApplyConsoleTweaks();
}

void LanguageSettings()
{
	VMenu *LangMenu, *HelpMenu;

	if (Select(FALSE, &LangMenu)) {
		Lang.Close();

		if (!Lang.Init(g_strFarPath, true, Msg::NewFileName.ID())) {
			Message(MSG_WARNING, 1, L"Error", L"Cannot load language data", L"Ok");
			exit(0);
		}

		Select(TRUE, &HelpMenu);
		delete HelpMenu;
		LangMenu->Hide();
		CtrlObject->Plugins.ReloadLanguage();
		setenv("FARLANG", Opt.strLanguage.GetMB().c_str(), 1);
		PrepareStrFTime();
		PrepareUnitStr();
		FrameManager->InitKeyBar();
		CtrlObject->Cp()->RedrawKeyBar();
		CtrlObject->Cp()->SetScreenPosition();
		ApplySudoConfiguration();
	}
	delete LangMenu;	//???? BUGBUG
}
