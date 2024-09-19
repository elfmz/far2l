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
#include "ConfigOpt.hpp"
#include "ConfigOptSaveLoad.hpp"
#include "pick_color256.hpp"
#include "pick_colorRGB.hpp"

void SanitizeHistoryCounts();
void SanitizeIndentationCounts();

static bool g_config_ready = false;

// Стандартный набор разделителей
static constexpr const wchar_t *WordDiv0 = L"~!%^&*()+|{}:\"<>?`-=\\[];',./";

// Стандартный набор разделителей для функции Xlat
static constexpr const wchar_t *WordDivForXlat0 = L" \t!#$%^&*()+|=\\/@?";

static constexpr const wchar_t *szCtrlDot = L"Ctrl.";
static constexpr const wchar_t *szCtrlShiftDot = L"CtrlShift.";


// Section
static constexpr const char *NSecColors = "Colors";
static constexpr const char *NSecScreen = "Screen";
static constexpr const char *NSecCmdline = "Cmdline";
static constexpr const char *NSecInterface = "Interface";
static constexpr const char *NSecInterfaceCompletion = "Interface/Completion";
static constexpr const char *NSecViewer = "Viewer";
static constexpr const char *NSecDialog = "Dialog";
static constexpr const char *NSecEditor = "Editor";
static constexpr const char *NSecNotifications = "Notifications";
static constexpr const char *NSecXLat = "XLat";
static constexpr const char *NSecSystem = "System";
static constexpr const char *NSecSystemExecutor = "System/Executor";
static constexpr const char *NSecSystemNowell = "System/Nowell";
static constexpr const char *NSecLanguage = "Language";
static constexpr const char *NSecConfirmations = "Confirmations";
static constexpr const char *NSecPluginConfirmations = "PluginConfirmations";
static constexpr const char *NSecPanel = "Panel";
static constexpr const char *NSecPanelLeft = "Panel/Left";
static constexpr const char *NSecPanelRight = "Panel/Right";
static constexpr const char *NSecPanelLayout = "Panel/Layout";
static constexpr const char *NSecPanelTree = "Panel/Tree";
static constexpr const char *NSecLayout = "Layout";
static constexpr const char *NSecDescriptions = "Descriptions";
static constexpr const char *NSecKeyMacros = "KeyMacros";
static constexpr const char *NSecPolicies = "Policies";
static constexpr const char *NSecSavedHistory = "SavedHistory";
static constexpr const char *NSecSavedViewHistory = "SavedViewHistory";
static constexpr const char *NSecSavedFolderHistory = "SavedFolderHistory";
static constexpr const char *NSecSavedDialogHistory = "SavedDialogHistory";
static constexpr const char *NSecCodePages = "CodePages";
static constexpr const char *NParamHistoryCount = "HistoryCount";
static constexpr const char *NSecVMenu = "VMenu";

static FARString strKeyNameConsoleDetachKey;

const ConfigOpt g_cfg_opts[] {
	{true,  NSecColors, "CurrentPalette", SIZE_ARRAY_PALETTE, (BYTE *)Palette8bit, (BYTE *)DefaultPalette8bit},
	{true,  NSecColors, "CurrentPaletteRGB", SIZE_ARRAY_PALETTE * 8, (BYTE *)Palette, nullptr},
	{true,  NSecColors, "TempColors256", TEMP_COLORS256_SIZE, g_tempcolors256, g_tempcolors256},
	{true,  NSecColors, "TempColorsRGB", TEMP_COLORSRGB_SIZE, (BYTE *)g_tempcolorsRGB, (BYTE *)g_tempcolorsRGB},

	{true,  NSecScreen, "Clock", &Opt.Clock, 1},
	{true,  NSecScreen, "ViewerEditorClock", &Opt.ViewerEditorClock, 0},
	{true,  NSecScreen, "KeyBar", &Opt.ShowKeyBar, 1},
	{true,  NSecScreen, "ScreenSaver", &Opt.ScreenSaver, 0},
	{true,  NSecScreen, "ScreenSaverTime", &Opt.ScreenSaverTime, 5},
	{false, NSecScreen, "DeltaXY", &Opt.ScrSize.dwDeltaXY, 0},
	{true,  NSecScreen, "CursorBlinkInterval", &Opt.CursorBlinkTime, 500},

	{true,  NSecCmdline, "UsePromptFormat", &Opt.CmdLine.UsePromptFormat, 0},
	{true,  NSecCmdline, "PromptFormat", &Opt.CmdLine.strPromptFormat, L"$p$# "},
	{true,  NSecCmdline, "UseShell", &Opt.CmdLine.UseShell, 0},
	{true,  NSecCmdline, "ShellCmd", &Opt.CmdLine.strShell, L"bash -i"},
	{true,  NSecCmdline, "DelRemovesBlocks", &Opt.CmdLine.DelRemovesBlocks, 1},
	{true,  NSecCmdline, "EditBlock", &Opt.CmdLine.EditBlock, 0},
	{true,  NSecCmdline, "AutoComplete", &Opt.CmdLine.AutoComplete, 1},
	{true,  NSecCmdline, "Splitter", &Opt.CmdLine.Splitter, 1},
	{true,  NSecCmdline, "WaitKeypress", &Opt.CmdLine.WaitKeypress, 1},
	{true,  NSecCmdline, "VTLogLimit", &Opt.CmdLine.VTLogLimit, 5000},

	{true,  NSecInterface, "Mouse", &Opt.Mouse, 1},
	{false, NSecInterface, "UseVk_oem_x", &Opt.UseVk_oem_x, 1},
	{true,  NSecInterface, "ShowMenuBar", &Opt.ShowMenuBar, 0},
	{false, NSecInterface, "CursorSize1", &Opt.CursorSize[0], 15},
	{false, NSecInterface, "CursorSize2", &Opt.CursorSize[1], 10},
	{false, NSecInterface, "CursorSize3", &Opt.CursorSize[2], 99},
	{false, NSecInterface, "CursorSize4", &Opt.CursorSize[3], 99},
	{false, NSecInterface, "ShiftsKeyRules", &Opt.ShiftsKeyRules, 1},
	{true,  NSecInterface, "CtrlPgUp", &Opt.PgUpChangeDisk, 1},

	{true,  NSecInterface, "ConsolePaintSharp", &Opt.ConsolePaintSharp, 0},
	{true,  NSecInterface, "ExclusiveCtrlLeft", &Opt.ExclusiveCtrlLeft, 0},
	{true,  NSecInterface, "ExclusiveCtrlRight", &Opt.ExclusiveCtrlRight, 0},
	{true,  NSecInterface, "ExclusiveAltLeft", &Opt.ExclusiveAltLeft, 0},
	{true,  NSecInterface, "ExclusiveAltRight", &Opt.ExclusiveAltRight, 0},
	{true,  NSecInterface, "ExclusiveWinLeft", &Opt.ExclusiveWinLeft, 0},
	{true,  NSecInterface, "ExclusiveWinRight", &Opt.ExclusiveWinRight, 0},

	{true,  NSecInterface, "DateFormat", &Opt.DateFormat, GetDateFormatDefault()},
	{true,  NSecInterface, "DateSeparator", &Opt.strDateSeparator, GetDateSeparatorDefaultStr()},
	{true,  NSecInterface, "TimeSeparator", &Opt.strTimeSeparator, GetTimeSeparatorDefaultStr()},
	{true,  NSecInterface, "DecimalSeparator", &Opt.strDecimalSeparator, GetDecimalSeparatorDefaultStr()},

	{true,  NSecInterface, "OSC52ClipSet", &Opt.OSC52ClipSet, 0},
	{true,  NSecInterface, "TTYPaletteOverride", &Opt.TTYPaletteOverride, 1},

	{false, NSecInterface, "ShowTimeoutDelFiles", &Opt.ShowTimeoutDelFiles, 50},
	{false, NSecInterface, "ShowTimeoutDACLFiles", &Opt.ShowTimeoutDACLFiles, 50},
	{false, NSecInterface, "FormatNumberSeparators", &Opt.FormatNumberSeparators, 0},
	{true,  NSecInterface, "CopyShowTotal", &Opt.CMOpt.CopyShowTotal, 1},
	{true,  NSecInterface, "DelShowTotal", &Opt.DelOpt.DelShowTotal, 0},
	{true,  NSecInterface, "WindowTitle", &Opt.strWindowTitle, L"%State - FAR2L %Ver %Backend %User@%Host"}, // %Platform 
	{true,  NSecInterfaceCompletion, "Exceptions", &Opt.AutoComplete.Exceptions, L"git*reset*--hard;*://*:*@*;\" *\""},
	{true,  NSecInterfaceCompletion, "ShowList", &Opt.AutoComplete.ShowList, 1},
	{true,  NSecInterfaceCompletion, "ModalList", &Opt.AutoComplete.ModalList, 0},
	{true,  NSecInterfaceCompletion, "Append", &Opt.AutoComplete.AppendCompletion, 0},

	{true,  NSecViewer, "ExternalViewerName", &Opt.strExternalViewer, L""},
	{true,  NSecViewer, "UseExternalViewer", &Opt.ViOpt.UseExternalViewer, 0},
	{true,  NSecViewer, "SaveViewerPos", &Opt.ViOpt.SavePos, 1},
	{true,  NSecViewer, "SaveViewerShortPos", &Opt.ViOpt.SaveShortPos, 1},
	{true,  NSecViewer, "AutoDetectCodePage", &Opt.ViOpt.AutoDetectCodePage, 0},
	{true,  NSecViewer, "SearchRegexp", &Opt.ViOpt.SearchRegexp, 0},

	{true,  NSecViewer, "TabSize", &Opt.ViOpt.TabSize, 8},
	{true,  NSecViewer, "ShowKeyBar", &Opt.ViOpt.ShowKeyBar, 1},
	{true,  NSecViewer, "ShowTitleBar", &Opt.ViOpt.ShowTitleBar, 1},
	{true,  NSecViewer, "ShowArrows", &Opt.ViOpt.ShowArrows, 1},
	{true,  NSecViewer, "ShowScrollbar", &Opt.ViOpt.ShowScrollbar, 0},
	{true,  NSecViewer, "IsWrap", &Opt.ViOpt.ViewerIsWrap, 1},
	{true,  NSecViewer, "Wrap", &Opt.ViOpt.ViewerWrap, 0},
	{true,  NSecViewer, "PersistentBlocks", &Opt.ViOpt.PersistentBlocks, 0},
	{true,  NSecViewer, "DefaultCodePage", &Opt.ViOpt.DefaultCodePage, CP_UTF8},

	{true,  NSecDialog, "EditHistory", &Opt.Dialogs.EditHistory, 1},
	{true,  NSecDialog, "EditBlock", &Opt.Dialogs.EditBlock, 0},
	{true,  NSecDialog, "AutoComplete", &Opt.Dialogs.AutoComplete, 1},
	{true,  NSecDialog, "EULBsClear", &Opt.Dialogs.EULBsClear, 0},
	{false, NSecDialog, "SelectFromHistory", &Opt.Dialogs.SelectFromHistory, 0},
	{false, NSecDialog, "EditLine", &Opt.Dialogs.EditLine, 0},
	{true,  NSecDialog, "MouseButton", &Opt.Dialogs.MouseButton, 0xFFFF},
	{true,  NSecDialog, "DelRemovesBlocks", &Opt.Dialogs.DelRemovesBlocks, 1},
	{false, NSecDialog, "CBoxMaxHeight", &Opt.Dialogs.CBoxMaxHeight, 24},

	{true,  NSecEditor, "ExternalEditorName", &Opt.strExternalEditor, L""},
	{true,  NSecEditor, "UseExternalEditor", &Opt.EdOpt.UseExternalEditor, 0},
	{true,  NSecEditor, "ExpandTabs", &Opt.EdOpt.ExpandTabs, 0},
	{true,  NSecEditor, "TabSize", &Opt.EdOpt.TabSize, 8},
	{true,  NSecEditor, "PersistentBlocks", &Opt.EdOpt.PersistentBlocks, 0},
	{true,  NSecEditor, "DelRemovesBlocks", &Opt.EdOpt.DelRemovesBlocks, 1},
	{true,  NSecEditor, "AutoIndent", &Opt.EdOpt.AutoIndent, 0},
	{true,  NSecEditor, "SaveEditorPos", &Opt.EdOpt.SavePos, 1},
	{true,  NSecEditor, "SaveEditorShortPos", &Opt.EdOpt.SaveShortPos, 1},
	{true,  NSecEditor, "AutoDetectCodePage", &Opt.EdOpt.AutoDetectCodePage, 0},
	{true,  NSecEditor, "EditorCursorBeyondEOL", &Opt.EdOpt.CursorBeyondEOL, 1},
	{true,  NSecEditor, "ReadOnlyLock", &Opt.EdOpt.ReadOnlyLock, 0}, // Вернём назад дефолт 1.65 - не предупреждать и не блокировать
	{false, NSecEditor, "EditorUndoSize", &Opt.EdOpt.UndoSize, 0}, // $ 03.12.2001 IS размер буфера undo в редакторе
	{false, NSecEditor, "WordDiv", &Opt.strWordDiv, WordDiv0},
	{false, NSecEditor, "BSLikeDel", &Opt.EdOpt.BSLikeDel, 1},
	{false, NSecEditor, "FileSizeLimit", &Opt.EdOpt.FileSizeLimitLo, 0},
	{false, NSecEditor, "FileSizeLimitHi", &Opt.EdOpt.FileSizeLimitHi, 0},
	{false, NSecEditor, "CharCodeBase", &Opt.EdOpt.CharCodeBase, 1},
	{false, NSecEditor, "AllowEmptySpaceAfterEof", &Opt.EdOpt.AllowEmptySpaceAfterEof, 0},//skv
	{true,  NSecEditor, "DefaultCodePage", &Opt.EdOpt.DefaultCodePage, CP_UTF8},
	{true,  NSecEditor, "ShowKeyBar", &Opt.EdOpt.ShowKeyBar, 1},
	{true,  NSecEditor, "ShowTitleBar", &Opt.EdOpt.ShowTitleBar, 1},
	{true,  NSecEditor, "ShowScrollBar", &Opt.EdOpt.ShowScrollBar, 0},
	{true,  NSecEditor, "UseEditorConfigOrg", &Opt.EdOpt.UseEditorConfigOrg, 1},
	{true,  NSecEditor, "SearchSelFound", &Opt.EdOpt.SearchSelFound, 0},
	{true,  NSecEditor, "SearchRegexp", &Opt.EdOpt.SearchRegexp, 0},
	{true,  NSecEditor, "SearchPickUpWord", &Opt.EdOpt.SearchPickUpWord, 0},
	{true,  NSecEditor, "ShowWhiteSpace", &Opt.EdOpt.ShowWhiteSpace, 0},

	{true,  NSecNotifications, "OnFileOperation", &Opt.NotifOpt.OnFileOperation, 1},
	{true,  NSecNotifications, "OnConsole", &Opt.NotifOpt.OnConsole, 1},
	{true,  NSecNotifications, "OnlyIfBackground", &Opt.NotifOpt.OnlyIfBackground, 1},

	{false, NSecXLat, "Flags", &Opt.XLat.Flags, (DWORD)XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE},
	{true,  NSecXLat, "EnableForFastFileFind", &Opt.XLat.EnableForFastFileFind, 1},
	{true,  NSecXLat, "EnableForDialogs", &Opt.XLat.EnableForDialogs, 1},
	{true,  NSecXLat, "WordDivForXlat", &Opt.XLat.strWordDivForXlat, WordDivForXlat0},
	{true,  NSecXLat, "XLat", &Opt.XLat.XLat, L"ru:qwerty-йцукен"},

	{true,  NSecSavedHistory, NParamHistoryCount, &Opt.HistoryCount, 512},
	{true,  NSecSavedFolderHistory, NParamHistoryCount, &Opt.FoldersHistoryCount, 512},
	{true,  NSecSavedViewHistory, NParamHistoryCount, &Opt.ViewHistoryCount, 512},
	{true,  NSecSavedDialogHistory, NParamHistoryCount, &Opt.DialogsHistoryCount, 512},

	{true,  NSecSystem, "PersonalPluginsPath", &Opt.LoadPlug.strPersonalPluginsPath, L""},
	{true,  NSecSystem, "SaveHistory", &Opt.SaveHistory, 1},
	{true,  NSecSystem, "SaveFoldersHistory", &Opt.SaveFoldersHistory, 1},
	{false, NSecSystem, "SavePluginFoldersHistory", &Opt.SavePluginFoldersHistory, 0},
	{true,  NSecSystem, "SaveViewHistory", &Opt.SaveViewHistory, 1},
	{true,  NSecSystem, "HistoryRemoveDupsRule", &Opt.HistoryRemoveDupsRule, 2},
	{true,  NSecSystem, "AutoHighlightHistory", &Opt.AutoHighlightHistory, 1},
	{true,  NSecSystem, "AutoSaveSetup", &Opt.AutoSaveSetup, 0},
	{true,  NSecSystem, "DeleteToRecycleBin", &Opt.DeleteToRecycleBin, 0},
	{true,  NSecSystem, "DeleteToRecycleBinKillLink", &Opt.DeleteToRecycleBinKillLink, 1},
	{false, NSecSystem, "WipeSymbol", &Opt.WipeSymbol, 0},
	{true,  NSecSystem, "SudoEnabled", &Opt.SudoEnabled, 1},
	{true,  NSecSystem, "SudoConfirmModify", &Opt.SudoConfirmModify, 1},
	{true,  NSecSystem, "SudoPasswordExpiration", &Opt.SudoPasswordExpiration, 15 * 60},

	{true,  NSecSystem, "UseCOW", &Opt.CMOpt.UseCOW, 0},
	{true,  NSecSystem, "SparseFiles", &Opt.CMOpt.SparseFiles, 0},
	{true,  NSecSystem, "HowCopySymlink", &Opt.CMOpt.HowCopySymlink, 1},
	{true,  NSecSystem, "WriteThrough", &Opt.CMOpt.WriteThrough, 0},
	{true,  NSecSystem, "CopyXAttr", &Opt.CMOpt.CopyXAttr, 0},
	{false, NSecSystem, "CopyAccessMode", &Opt.CMOpt.CopyAccessMode, 1},
	{true,  NSecSystem, "MultiCopy", &Opt.CMOpt.MultiCopy, 0},
	{true,  NSecSystem, "CopyTimeRule", &Opt.CMOpt.CopyTimeRule, 3},

	{true,  NSecSystem, "MakeLinkSuggestSymlinkAlways", &Opt.MakeLinkSuggestSymlinkAlways, 1},

	{true,  NSecSystem, "InactivityExit", &Opt.InactivityExit, 0},
	{true,  NSecSystem, "InactivityExitTime", &Opt.InactivityExitTime, 15},
	{true,  NSecSystem, "DriveMenuMode2", &Opt.ChangeDriveMode, -1},
	{true,  NSecSystem, "DriveDisconnectMode", &Opt.ChangeDriveDisconnectMode, 1},

	{true,  NSecSystem, "DriveExceptions", &Opt.ChangeDriveExceptions,
		L"/System/*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron;/tmp/.*"},
	{true,  NSecSystem, "DriveColumn2", &Opt.ChangeDriveColumn2, L"$U$</$>$T"},
	{true,  NSecSystem, "DriveColumn3", &Opt.ChangeDriveColumn3, L"$S$D"},

	{true,  NSecSystem, "AutoUpdateRemoteDrive", &Opt.AutoUpdateRemoteDrive, 1},
	{true,  NSecSystem, "FileSearchMode", &Opt.FindOpt.FileSearchMode, FINDAREA_FROM_CURRENT},
	{false, NSecSystem, "CollectFiles", &Opt.FindOpt.CollectFiles, 1},
	{true,  NSecSystem, "SearchInFirstSize", &Opt.FindOpt.strSearchInFirstSize, L""},
	{true,  NSecSystem, "FindAlternateStreams", &Opt.FindOpt.FindAlternateStreams, 0},
	{true,  NSecSystem, "SearchOutFormat", &Opt.FindOpt.strSearchOutFormat, L"D,S,A"},
	{true,  NSecSystem, "SearchOutFormatWidth", &Opt.FindOpt.strSearchOutFormatWidth, L"14,13,0"},
	{true,  NSecSystem, "FindFolders", &Opt.FindOpt.FindFolders, 1},
	{true,  NSecSystem, "FindSymLinks", &Opt.FindOpt.FindSymLinks, 1},
	{true,  NSecSystem, "FindCaseSensitiveFileMask", &Opt.FindOpt.FindCaseSensitiveFileMask, 1},
	{true,  NSecSystem, "UseFilterInSearch", &Opt.FindOpt.UseFilter, 0},
	{true,  NSecSystem, "FindCodePage", &Opt.FindCodePage, CP_AUTODETECT},
	{false, NSecSystem, "CmdHistoryRule", &Opt.CmdHistoryRule, 0},
	{false, NSecSystem, "SetAttrFolderRules", &Opt.SetAttrFolderRules, 1},
	{false, NSecSystem, "MaxPositionCache", &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS},
	{false, NSecSystem, "ConsoleDetachKey", &strKeyNameConsoleDetachKey, L"CtrlAltTab"},
	{false, NSecSystem, "SilentLoadPlugin", &Opt.LoadPlug.SilentLoadPlugin, 0},
	{true,  NSecSystem, "ScanSymlinks", &Opt.LoadPlug.ScanSymlinks, 1},
	{true,  NSecSystem, "MultiMakeDir", &Opt.MultiMakeDir, 0},
	{false, NSecSystem, "MsWheelDelta", &Opt.MsWheelDelta, 1},
	{false, NSecSystem, "MsWheelDeltaView", &Opt.MsWheelDeltaView, 1},
	{false, NSecSystem, "MsWheelDeltaEdit", &Opt.MsWheelDeltaEdit, 1},
	{false, NSecSystem, "MsWheelDeltaHelp", &Opt.MsWheelDeltaHelp, 1},
	{false, NSecSystem, "MsHWheelDelta", &Opt.MsHWheelDelta, 1},
	{false, NSecSystem, "MsHWheelDeltaView", &Opt.MsHWheelDeltaView, 1},
	{false, NSecSystem, "MsHWheelDeltaEdit", &Opt.MsHWheelDeltaEdit, 1},
	{false, NSecSystem, "SubstNameRule", &Opt.SubstNameRule, 2},
	{false, NSecSystem, "ShowCheckingFile", &Opt.ShowCheckingFile, 0},
	{false, NSecSystem, "QuotedSymbols", &Opt.strQuotedSymbols, L" $&()[]{};|*?!'`\"\\\xA0"}, //xA0 => 160 =>oem(0xFF)
	{false, NSecSystem, "QuotedName", &Opt.QuotedName, QUOTEDNAME_INSERT},
	//{false, NSecSystem, "CPAJHefuayor", &Opt.strCPAJHefuayor, 0},
	{false, NSecSystem, "PluginMaxReadData", &Opt.PluginMaxReadData, 0x40000},
	{false, NSecSystem, "UseNumPad", &Opt.UseNumPad, 1},
	{false, NSecSystem, "CASRule", &Opt.CASRule, -1},
	{false, NSecSystem, "AllCtrlAltShiftRule", &Opt.AllCtrlAltShiftRule, 0x0000FFFF},
	{true,  NSecSystem, "ScanJunction", &Opt.ScanJunction, 1},
	{true,  NSecSystem, "OnlyFilesSize", &Opt.OnlyFilesSize, 0},
	{false, NSecSystem, "UsePrintManager", &Opt.UsePrintManager, 1},

	{false, NSecSystem, "ExcludeCmdHistory", &Opt.ExcludeCmdHistory, 0}, //AN

	{true,  NSecSystem, "FolderInfo", &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{false, NSecSystemNowell, "MoveRO", &Opt.Nowell.MoveRO, 1},

	{false, NSecSystemExecutor, "RestoreCP", &Opt.RestoreCPAfterExecute, 1},
	{false, NSecSystemExecutor, "UseAppPath", &Opt.ExecuteUseAppPath, 1},
	{false, NSecSystemExecutor, "ShowErrorMessage", &Opt.ExecuteShowErrorMessage, 1},
	{false, NSecSystemExecutor, "FullTitle", &Opt.ExecuteFullTitle, 0},
	{false, NSecSystemExecutor, "SilentExternal", &Opt.ExecuteSilentExternal, 0},

	{false, NSecPanelTree, "MinTreeCount", &Opt.Tree.MinTreeCount, 4},
	{false, NSecPanelTree, "TreeFileAttr", &Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN},
	{false, NSecPanelTree, "LocalDisk", &Opt.Tree.LocalDisk, 2},
	{false, NSecPanelTree, "NetDisk", &Opt.Tree.NetDisk, 2},
	{false, NSecPanelTree, "RemovableDisk", &Opt.Tree.RemovableDisk, 2},
	{false, NSecPanelTree, "NetPath", &Opt.Tree.NetPath, 2},
	{true,  NSecPanelTree, "AutoChangeFolder", &Opt.Tree.AutoChangeFolder, 0}, // ???

	{true,  NSecLanguage, "Help", &Opt.strHelpLanguage, L"English"},
	{true,  NSecLanguage, "Main", &Opt.strLanguage, L"English"},

	{true,  NSecConfirmations, "Copy", &Opt.Confirm.Copy, 1},
	{true,  NSecConfirmations, "Move", &Opt.Confirm.Move, 1},
	{true,  NSecConfirmations, "RO", &Opt.Confirm.RO, 1},
	{true,  NSecConfirmations, "Drag", &Opt.Confirm.Drag, 1},
	{true,  NSecConfirmations, "Delete", &Opt.Confirm.Delete, 1},
	{true,  NSecConfirmations, "DeleteFolder", &Opt.Confirm.DeleteFolder, 1},
	{true,  NSecConfirmations, "Esc", &Opt.Confirm.Esc, 1},
	{true,  NSecConfirmations, "RemoveConnection", &Opt.Confirm.RemoveConnection, 1},
	{true,  NSecConfirmations, "ClearVT", &Opt.Confirm.ClearVT, 1},
	{true,  NSecConfirmations, "RemoveHotPlug", &Opt.Confirm.RemoveHotPlug, 1},
	{true,  NSecConfirmations, "AllowReedit", &Opt.Confirm.AllowReedit, 1},
	{true,  NSecConfirmations, "HistoryClear", &Opt.Confirm.HistoryClear, 1},
	{true,  NSecConfirmations, "Exit", &Opt.Confirm.Exit, 1},
	{true,  NSecConfirmations, "ExitOrBknd", &Opt.Confirm.ExitOrBknd, 1},
	{false, NSecConfirmations, "EscTwiceToInterrupt", &Opt.Confirm.EscTwiceToInterrupt, 0},

	{true,  NSecPluginConfirmations,  "OpenFilePlugin", &Opt.PluginConfirm.OpenFilePlugin, 0},
	{true,  NSecPluginConfirmations,  "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0},
	{true,  NSecPluginConfirmations,  "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0},
	{true,  NSecPluginConfirmations,  "SetFindList", &Opt.PluginConfirm.SetFindList, 0},
	{true,  NSecPluginConfirmations,  "Prefix", &Opt.PluginConfirm.Prefix, 0},

	{false, NSecPanel, "ShellRightLeftArrowsRule", &Opt.ShellRightLeftArrowsRule, 0},
	{true,  NSecPanel, "ShowHidden", &Opt.ShowHidden, 1},
	{true,  NSecPanel, "Highlight", &Opt.Highlight, 1},
	{true,  NSecPanel, "SortFolderExt", &Opt.SortFolderExt, 0},
	{true,  NSecPanel, "SelectFolders", &Opt.SelectFolders, 0},
	{true,  NSecPanel, "CaseSensitiveCompareSelect", &Opt.PanelCaseSensitiveCompareSelect, 1},
	{true,  NSecPanel, "ReverseSort", &Opt.ReverseSort, 1},
	{false, NSecPanel, "RightClickRule", &Opt.PanelRightClickRule, 2},
	{false, NSecPanel, "CtrlFRule", &Opt.PanelCtrlFRule, 1},
	{false, NSecPanel, "CtrlAltShiftRule", &Opt.PanelCtrlAltShiftRule, 0},
	{false, NSecPanel, "RememberLogicalDrives", &Opt.RememberLogicalDrives, 0},
	{true,  NSecPanel, "AutoUpdateLimit", &Opt.AutoUpdateLimit, 0},
	{true,  NSecPanel, "ShowFilenameMarks", &Opt.ShowFilenameMarks, 1},
	{true,  NSecPanel, "FilenameMarksAlign", &Opt.FilenameMarksAlign, 1},
	{true,  NSecPanel, "MinFilenameIndentation", &Opt.MinFilenameIndentation, 0},
	{true,  NSecPanel, "MaxFilenameIndentation", &Opt.MaxFilenameIndentation, HIGHLIGHT_MAX_MARK_LENGTH},

	{true,  NSecPanelLeft, "Type", &Opt.LeftPanel.Type, 0},
	{true,  NSecPanelLeft, "Visible", &Opt.LeftPanel.Visible, 1},
	{true,  NSecPanelLeft, "Focus", &Opt.LeftPanel.Focus, 1},
	{true,  NSecPanelLeft, "ViewMode", &Opt.LeftPanel.ViewMode, 2},
	{true,  NSecPanelLeft, "SortMode", &Opt.LeftPanel.SortMode, 1},
	{true,  NSecPanelLeft, "SortOrder", &Opt.LeftPanel.SortOrder, 1},
	{true,  NSecPanelLeft, "SortGroups", &Opt.LeftPanel.SortGroups, 0},
	{true,  NSecPanelLeft, "NumericSort", &Opt.LeftPanel.NumericSort, 0},
	{true,  NSecPanelLeft, "CaseSensitiveSort", &Opt.LeftPanel.CaseSensitiveSort, 0},
	{true,  NSecPanelLeft, "Folder", &Opt.strLeftFolder, L""},
	{true,  NSecPanelLeft, "CurFile", &Opt.strLeftCurFile, L""},
	{true,  NSecPanelLeft, "SelectedFirst", &Opt.LeftSelectedFirst, 0},
	{true,  NSecPanelLeft, "DirectoriesFirst", &Opt.LeftPanel.DirectoriesFirst, 1},

	{true,  NSecPanelRight, "Type", &Opt.RightPanel.Type, 0},
	{true,  NSecPanelRight, "Visible", &Opt.RightPanel.Visible, 1},
	{true,  NSecPanelRight, "Focus", &Opt.RightPanel.Focus, 0},
	{true,  NSecPanelRight, "ViewMode", &Opt.RightPanel.ViewMode, 2},
	{true,  NSecPanelRight, "SortMode", &Opt.RightPanel.SortMode, 1},
	{true,  NSecPanelRight, "SortOrder", &Opt.RightPanel.SortOrder, 1},
	{true,  NSecPanelRight, "SortGroups", &Opt.RightPanel.SortGroups, 0},
	{true,  NSecPanelRight, "NumericSort", &Opt.RightPanel.NumericSort, 0},
	{true,  NSecPanelRight, "CaseSensitiveSort", &Opt.RightPanel.CaseSensitiveSort, 0},
	{true,  NSecPanelRight, "Folder", &Opt.strRightFolder, L""},
	{true,  NSecPanelRight, "CurFile", &Opt.strRightCurFile, L""},
	{true,  NSecPanelRight, "SelectedFirst", &Opt.RightSelectedFirst, 0},
	{true,  NSecPanelRight, "DirectoriesFirst", &Opt.RightPanel.DirectoriesFirst, 1},

	{true,  NSecPanelLayout, "ColumnTitles", &Opt.ShowColumnTitles, 1},
	{true,  NSecPanelLayout, "StatusLine", &Opt.ShowPanelStatus, 1},
	{true,  NSecPanelLayout, "TotalInfo", &Opt.ShowPanelTotals, 1},
	{true,  NSecPanelLayout, "FreeInfo", &Opt.ShowPanelFree, 0},
	{true,  NSecPanelLayout, "Scrollbar", &Opt.ShowPanelScrollbar, 0},
	{false, NSecPanelLayout, "ScrollbarMenu", &Opt.ShowMenuScrollbar, 1},
	{true,  NSecPanelLayout, "ScreensNumber", &Opt.ShowScreensNumber, 1},
	{true,  NSecPanelLayout, "SortMode", &Opt.ShowSortMode, 1},

	{true,  NSecLayout, "LeftHeightDecrement", &Opt.LeftHeightDecrement, 0},
	{true,  NSecLayout, "RightHeightDecrement", &Opt.RightHeightDecrement, 0},
	{true,  NSecLayout, "WidthDecrement", &Opt.WidthDecrement, 0},
	{true,  NSecLayout, "FullscreenHelp", &Opt.FullScreenHelp, 0},

	{true,  NSecDescriptions, "ListNames", &Opt.Diz.strListNames, L"Descript.ion,Files.bbs"},
	{true,  NSecDescriptions, "UpdateMode", &Opt.Diz.UpdateMode, DIZ_UPDATE_IF_DISPLAYED},
	{true,  NSecDescriptions, "ROUpdate", &Opt.Diz.ROUpdate, 0},
	{true,  NSecDescriptions, "SetHidden", &Opt.Diz.SetHidden, 1},
	{true,  NSecDescriptions, "StartPos", &Opt.Diz.StartPos, 0},
	{true,  NSecDescriptions, "AnsiByDefault", &Opt.Diz.AnsiByDefault, 0},
	{true,  NSecDescriptions, "SaveInUTF", &Opt.Diz.SaveInUTF, 0},

	{false, NSecKeyMacros, "MacroReuseRules", &Opt.Macro.MacroReuseRules, 0},
	{false, NSecKeyMacros, "DateFormat", &Opt.Macro.strDateFormat, L"%a %b %d %H:%M:%S %Z %Y"},
	{false, NSecKeyMacros, "CONVFMT", &Opt.Macro.strMacroCONVFMT, L"%.6g"},
	{false, NSecKeyMacros, "CallPluginRules", &Opt.Macro.CallPluginRules, 0},

	{false, NSecPolicies, "ShowHiddenDrives", &Opt.Policies.ShowHiddenDrives, 1},
	{false, NSecPolicies, "DisabledOptions", &Opt.Policies.DisabledOptions, 0},

	{true,  NSecCodePages, "CPMenuMode2", &Opt.CPMenuMode, 1},

	{true,  NSecVMenu, "MenuStopWrapOnEdge", &Opt.VMenu.MenuLoopScroll, 1},

	{true,  NSecVMenu, "LBtnClick", &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL},
	{true,  NSecVMenu, "RBtnClick", &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL},
	{true,  NSecVMenu, "MBtnClick", &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY},
	{true,  NSecVMenu, "HistShowTimes", ARRAYSIZE(Opt.HistoryShowTimes), Opt.HistoryShowTimes, nullptr},
	{true,  NSecVMenu, "HistDirsPrefixLen", &Opt.HistoryDirsPrefixLen, 20},
};

size_t ConfigOptCount() noexcept
{
	return ARRAYSIZE(g_cfg_opts);
}

int ConfigOptGetIndex(const wchar_t *name)
{
	auto dot = wcsrchr(name, L'.');
	if (dot)
	{
		std::string s_section = FARString(name, dot-name).GetMB();
		std::string s_key = FARString(dot+1).GetMB();
		const char *section=s_section.c_str(), *key=s_key.c_str();

		for (int i = 0; i < (int)ARRAYSIZE(g_cfg_opts); ++i)
		{
			if (!strcasecmp(g_cfg_opts[i].section,section) && !strcasecmp(g_cfg_opts[i].key,key))
				return i;
		}
	}
	return -1;
}

//////////

struct OptConfigReader : ConfigReader
{
	void LoadOpt(const ConfigOpt &opt)
	{
		SelectSection(opt.section);
		switch (opt.type)
		{
			case ConfigOpt::T_INT:
				*opt.value.i = GetInt(opt.key, opt.def.i);
				break;
			case ConfigOpt::T_DWORD:
				*opt.value.dw = GetUInt(opt.key, opt.def.dw);
				break;
			case ConfigOpt::T_BOOL:
				*opt.value.b = GetInt(opt.key, opt.def.b ? 1 : 0) != 0;
				break;
			case ConfigOpt::T_STR:
				*opt.value.str = GetString(opt.key, opt.def.str);
				break;
			case ConfigOpt::T_BIN:
				{
					const size_t Size = GetBytes(opt.value.bin, opt.bin_size, opt.key, opt.def.bin);
					if (Size < (size_t)opt.bin_size)
						memset(opt.value.bin + Size, 0, (size_t)opt.bin_size - Size);
				}
				break;
			default:
				ABORT_MSG("Wrong option type: %u", opt.type);
		}
	}
};

struct OptConfigWriter : ConfigWriter
{
	void SaveOpt(const ConfigOpt &opt)
	{
		if (!opt.save)
			return;

		SelectSection(opt.section);
		switch (opt.type)
		{
			case ConfigOpt::T_BOOL:
				SetInt(opt.key, *opt.value.b ? 1 : 0);
				break;
			case ConfigOpt::T_INT:
				SetInt(opt.key, *opt.value.i);
				break;
			case ConfigOpt::T_DWORD:
				SetUInt(opt.key, *opt.value.dw);
				break;
			case ConfigOpt::T_STR:
				SetString(opt.key, opt.value.str->CPtr());
				break;
			case ConfigOpt::T_BIN:
				SetBytes(opt.key, opt.value.bin, opt.bin_size);
				break;
			default:
				ABORT_MSG("Wrong option type: %u", opt.type);
		}
	}
};

static void SanitizeXlat()
{
	// ensure Opt.XLat.XLat specifies some known xlat
	//cfg_reader.SelectSection(NSecXLat);
	AllXlats xlats;
	std::string SetXLat;
	for (const auto &xlat : xlats) {
		if (Opt.XLat.XLat == xlat) {
			SetXLat.clear();
			break;
		}
		if (SetXLat.empty()) {
			SetXLat = xlat;
		}
	}
	if (!SetXLat.empty()) {
		Opt.XLat.XLat = SetXLat;
	}
}

static void SanitizePalette()
{
#if 0
	// Уточняем алгоритм "взятия" палитры.
	for (
		size_t I = COL_PRIVATEPOSITION_FOR_DIF165ABOVE - COL_FIRSTPALETTECOLOR + 1;
		I < (COL_LASTPALETTECOLOR - COL_FIRSTPALETTECOLOR);
		++I
	) {
		if (!Palette[I])
		{
			if (!Palette[COL_PRIVATEPOSITION_FOR_DIF165ABOVE - COL_FIRSTPALETTECOLOR])
				Palette[I] = DefaultPalette[I];
			else if (Palette[COL_PRIVATEPOSITION_FOR_DIF165ABOVE - COL_FIRSTPALETTECOLOR] == 1)
				Palette[I] = BlackPalette[I];

			/*
				else
					в других случаях нифига ничего не делаем, т.к.
					есть другие палитры...
			*/
		}
	}
#endif
}

static void MergePalette()
{
	for(size_t i = 0; i < SIZE_ARRAY_PALETTE; i++) {

		Palette[i] &= 0xFFFFFFFFFFFFFF00;
		Palette[i] |= Palette8bit[i];
	}

//	uint32_t basepalette[32];
//	WINPORT(GetConsoleBasePalette)(NULL, basepalette);

/*
	for(size_t i = 0; i < SIZE_ARRAY_PALETTE; i++) {
		uint8_t color = Palette8bit[i];

		Palette[i] &= 0xFFFFFFFFFFFFFF00;

		if (!(Palette[i] & FOREGROUND_TRUECOLOR)) {
			Palette[i] &= 0xFFFFFF000000FFFF;
			Palette[i] += ((uint64_t)basepalette[16 + (color & 0xF)] << 16);
			Palette[i] += FOREGROUND_TRUECOLOR;
		}
		if (!(Palette[i] & BACKGROUND_TRUECOLOR)) {
			Palette[i] &= 0x000000FFFFFFFFFF;
			Palette[i] += ((uint64_t)basepalette[color >> 4] << 40);
			Palette[i] += BACKGROUND_TRUECOLOR;
		}

		Palette[i] += color;
	}
*/
}

void ConfigOptFromCmdLine()
{
	for (auto Str: Opt.CmdLineStrings)
	{
		auto pName = Str.c_str();
		auto pVal = wcschr(pName, L'=');
		if (pVal)
		{
			FARString strName(pName, pVal - pName);
			pVal++;
			int index = ConfigOptGetIndex(strName.CPtr());
			if (index<0)
				continue;
			switch (g_cfg_opts[index].type)
			{
				case ConfigOpt::T_DWORD:
					if (iswdigit(*pVal)) {
						static auto formats = { L"%u%lc", L"0x%x%lc", L"0X%x%lc" };
						unsigned int ui; wchar_t wc;
						for (auto fmt: formats) {
							if (1 == swscanf(pVal, fmt, &ui, &wc)) {
								*g_cfg_opts[index].value.dw = (DWORD) ui;
								break;
							}
						}
					}
					break;
				case ConfigOpt::T_INT:
					if (iswdigit(*pVal) || (*pVal == L'-' && iswdigit(pVal[1]))) {
						static auto formats = { L"%d%lc", L"0x%x%lc", L"0X%x%lc" };
						int i; wchar_t wc;
						for (auto fmt: formats) {
							if (1 == swscanf(pVal, fmt, &i, &wc)) {
								*g_cfg_opts[index].value.i = i;
								break;
							}
						}
					}
					break;
				case ConfigOpt::T_BOOL:
					if ( !StrCmpI(pVal,L"1") || !StrCmpI(pVal,L"true") )
						*g_cfg_opts[index].value.b = true;
					else if ( !StrCmpI(pVal,L"0") || !StrCmpI(pVal,L"false") )
						*g_cfg_opts[index].value.b = false;
					break;
				case ConfigOpt::T_STR:
					*g_cfg_opts[index].value.str = pVal;
					break;
				//case ConfigOpt::REG_BINARY:
				default:
					break;
			}
		}
	}
	Opt.CmdLineStrings.clear();
}

void ConfigOptLoad()
{
	OptConfigReader cfg_reader;

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	//Opt.LCIDSort=LOCALE_USER_DEFAULT; // проинициализируем на всякий случай
	/* *************************************************** </ПРЕПРОЦЕССЫ> */
	for (size_t i = ConfigOptCount(); i--;)
		cfg_reader.LoadOpt(g_cfg_opts[i]);

	/* Command line directives */
	ConfigOptFromCmdLine();

	/* <ПОСТПРОЦЕССЫ> *************************************************** */

	SanitizeHistoryCounts();
	SanitizeIndentationCounts();

	if (Opt.CursorBlinkTime < 100)
		Opt.CursorBlinkTime = 100;

	if (Opt.CursorBlinkTime > 500)
		Opt.CursorBlinkTime = 500;

	if (Opt.ShowMenuBar)
		Opt.ShowMenuBar = 1;

	if (Opt.PluginMaxReadData < 0x1000) // || Opt.PluginMaxReadData > 0x80000)
		Opt.PluginMaxReadData = 0x20000;

	Opt.HelpTabSize = 8; // пока жестко пропишем...
//	SanitizePalette();
	MergePalette();

	Opt.ViOpt.ViewerIsWrap&= 1;
	Opt.ViOpt.ViewerWrap&= 1;

	// Исключаем случайное стирание разделителей ;-)
	if (Opt.strWordDiv.IsEmpty())
		Opt.strWordDiv = WordDiv0;

	// Исключаем случайное стирание разделителей
	if (Opt.XLat.strWordDivForXlat.IsEmpty())
		Opt.XLat.strWordDivForXlat = WordDivForXlat0;

	Opt.PanelRightClickRule%= 3;
	Opt.PanelCtrlAltShiftRule%= 3;
	Opt.ConsoleDetachKey = KeyNameToKey(strKeyNameConsoleDetachKey);

	if (Opt.EdOpt.TabSize < 1 || Opt.EdOpt.TabSize > 512)
		Opt.EdOpt.TabSize = 8;

	if (Opt.ViOpt.TabSize < 1 || Opt.ViOpt.TabSize > 512)
		Opt.ViOpt.TabSize = 8;

	cfg_reader.SelectSection(NSecKeyMacros);

	FARString strKeyNameFromReg = cfg_reader.GetString("KeyRecordCtrlDot", szCtrlDot);
	Opt.Macro.KeyMacroCtrlDot = KeyNameToKey(strKeyNameFromReg, KEY_CTRLDOT);

	strKeyNameFromReg = cfg_reader.GetString("KeyRecordCtrlShiftDot", szCtrlShiftDot);
	Opt.Macro.KeyMacroCtrlShiftDot = KeyNameToKey(strKeyNameFromReg, KEY_CTRLSHIFTDOT);

	Opt.EdOpt.strWordDiv = Opt.strWordDiv;
	FileList::ReadPanelModes(cfg_reader);

	SanitizeXlat();

	ZeroFill(Opt.FindOpt.OutColumnTypes);
	ZeroFill(Opt.FindOpt.OutColumnWidths);
	ZeroFill(Opt.FindOpt.OutColumnWidthType);
	Opt.FindOpt.OutColumnCount = 0;

	if (!Opt.FindOpt.strSearchOutFormat.IsEmpty())
	{
		if (Opt.FindOpt.strSearchOutFormatWidth.IsEmpty())
			Opt.FindOpt.strSearchOutFormatWidth=L"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";

		TextToViewSettings(Opt.FindOpt.strSearchOutFormat.CPtr(),
			Opt.FindOpt.strSearchOutFormatWidth.CPtr(),
			Opt.FindOpt.OutColumnTypes, Opt.FindOpt.OutColumnWidths,
			Opt.FindOpt.OutColumnWidthType, Opt.FindOpt.OutColumnCount);
	}

	FileFilter::InitFilter(cfg_reader);

	// avoid negative decrement for now as hiding command line by Ctrl+Down is a new feature and may confuse
	// some users, so let this state be not persistent for now so such users may recover by simple restart
	Opt.LeftHeightDecrement = std::max(Opt.LeftHeightDecrement, 0);
	Opt.RightHeightDecrement = std::max(Opt.RightHeightDecrement, 0);

	g_config_ready = true;
	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}

void ConfigOptAssertLoaded()
{
	ASSERT(g_config_ready);
}

void ConfigOptSave(bool Ask)
{
	if (Opt.Policies.DisabledOptions&0x20000) // Bit 17 - Сохранить параметры
		return;

	if (Ask && Message(0, 2, Msg::SaveSetupTitle, Msg::SaveSetupAsk1, Msg::SaveSetupAsk2, Msg::SaveSetup, Msg::Cancel))
		return;

	WINPORT(SaveConsoleWindowState)();

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	Panel *LeftPanel = CtrlObject->Cp()->LeftPanel;
	Panel *RightPanel = CtrlObject->Cp()->RightPanel;
	Opt.LeftPanel.Focus = LeftPanel->GetFocus();
	Opt.LeftPanel.Visible = LeftPanel->IsVisible();
	Opt.RightPanel.Focus = RightPanel->GetFocus();
	Opt.RightPanel.Visible = RightPanel->IsVisible();

	if (LeftPanel->GetMode() == NORMAL_PANEL)
	{
		Opt.LeftPanel.Type = LeftPanel->GetType();
		Opt.LeftPanel.ViewMode = LeftPanel->GetViewMode();
		Opt.LeftPanel.SortMode = LeftPanel->GetSortMode();
		Opt.LeftPanel.SortOrder = LeftPanel->GetSortOrder();
		Opt.LeftPanel.SortGroups = LeftPanel->GetSortGroups();
		Opt.LeftPanel.NumericSort = LeftPanel->GetNumericSort();
		Opt.LeftPanel.CaseSensitiveSort = LeftPanel->GetCaseSensitiveSort();
		Opt.LeftSelectedFirst = LeftPanel->GetSelectedFirstMode();
		Opt.LeftPanel.DirectoriesFirst = LeftPanel->GetDirectoriesFirst();
	}

	LeftPanel->GetCurDir(Opt.strLeftFolder);
	LeftPanel->GetCurBaseName(Opt.strLeftCurFile);

	if (RightPanel->GetMode() == NORMAL_PANEL)
	{
		Opt.RightPanel.Type = RightPanel->GetType();
		Opt.RightPanel.ViewMode = RightPanel->GetViewMode();
		Opt.RightPanel.SortMode = RightPanel->GetSortMode();
		Opt.RightPanel.SortOrder = RightPanel->GetSortOrder();
		Opt.RightPanel.SortGroups = RightPanel->GetSortGroups();
		Opt.RightPanel.NumericSort = RightPanel->GetNumericSort();
		Opt.RightPanel.CaseSensitiveSort = RightPanel->GetCaseSensitiveSort();
		Opt.RightSelectedFirst = RightPanel->GetSelectedFirstMode();
		Opt.RightPanel.DirectoriesFirst = RightPanel->GetDirectoriesFirst();
	}

	RightPanel->GetCurDir(Opt.strRightFolder);
	RightPanel->GetCurBaseName(Opt.strRightCurFile);
	CtrlObject->HiFiles->SaveHiData();
	/* *************************************************** </ПРЕПРОЦЕССЫ> */

	OptConfigWriter cfg_writer;
	for (size_t i = ConfigOptCount(); i--;)
		cfg_writer.SaveOpt(g_cfg_opts[i]);

	/* <ПОСТПРОЦЕССЫ> *************************************************** */
	FileFilter::SaveFilters(cfg_writer);
	FileList::SavePanelModes(cfg_writer);

	if (Ask)
		CtrlObject->Macro.SaveMacros();

	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}
