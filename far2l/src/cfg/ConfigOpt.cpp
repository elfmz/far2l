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
#include "farcolors.hpp"
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
#include "MaskGroups.hpp"


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
static constexpr const char *NSecVMenu = "VMenu";

// ValName
static constexpr const char *NParamHistoryCount = "HistoryCount";
static constexpr const char *NParamAutoSaveSetup = "AutoSaveSetup";
static constexpr const char *NParamAutoSavePanels = "AutoSavePanels";


static FARString strKeyNameConsoleDetachKey;

const ConfigOpt g_cfg_opts[] {
//	{OST_NONE,   NSecColors, "CurrentPalette", SIZE_ARRAY_PALETTE, (BYTE *)Palette8bit, nullptr},
//	{OST_COMMON, NSecColors, "CurrentPaletteRGB", SIZE_ARRAY_PALETTE * sizeof(uint64_t), (BYTE *)Palette, nullptr},
	{OST_COMMON, NSecColors, "TempColors256", TEMP_COLORS256_SIZE, g_tempcolors256, nullptr},
	{OST_COMMON, NSecColors, "TempColorsRGB", TEMP_COLORSRGB_SIZE, (BYTE *)g_tempcolorsRGB, nullptr},

	{OST_COMMON, NSecColors, "CurrentTheme", &Opt.CurrentTheme, L"" },
	{OST_COMMON, NSecColors, "CurrentThemeIsSystemWide", &Opt.IsSystemTheme, 0 },

	{OST_COMMON, NSecScreen, "Clock", &Opt.Clock, 1},
	{OST_COMMON, NSecScreen, "ViewerEditorClock", &Opt.ViewerEditorClock, 0},
	{OST_COMMON, NSecScreen, "KeyBar", &Opt.ShowKeyBar, 1},
	{OST_COMMON, NSecScreen, "ScreenSaver", &Opt.ScreenSaver, 0},
	{OST_COMMON, NSecScreen, "ScreenSaverTime", &Opt.ScreenSaverTime, 5},
	{OST_NONE,   NSecScreen, "DeltaXY", &Opt.ScrSize.dwDeltaXY, 0},
	{OST_COMMON, NSecScreen, "CursorBlinkInterval", &Opt.CursorBlinkTime, 500},

	{OST_COMMON, NSecCmdline, "UsePromptFormat", &Opt.CmdLine.UsePromptFormat, 0},
	{OST_COMMON, NSecCmdline, "PromptFormat", &Opt.CmdLine.strPromptFormat, L"$p$# "},
	{OST_COMMON, NSecCmdline, "UseShell", &Opt.CmdLine.UseShell, 0},
	{OST_COMMON, NSecCmdline, "ShellCmd", &Opt.CmdLine.strShell, L"bash -i"},
	{OST_COMMON, NSecCmdline, "DelRemovesBlocks", &Opt.CmdLine.DelRemovesBlocks, 1},
	{OST_COMMON, NSecCmdline, "EditBlock", &Opt.CmdLine.EditBlock, 0},
	{OST_COMMON, NSecCmdline, "AutoComplete", &Opt.CmdLine.AutoComplete, 1},
	{OST_COMMON, NSecCmdline, "Splitter", &Opt.CmdLine.Splitter, 1},
	{OST_COMMON, NSecCmdline, "WaitKeypress", &Opt.CmdLine.WaitKeypress, 1},
	{OST_COMMON, NSecCmdline, "VTLogLimitKB", &Opt.CmdLine.VTLogLimit, 1024},
	{OST_NONE,   NSecCmdline, "AskOnMultilinePaste", &Opt.CmdLine.AskOnMultilinePaste, 1},

	{OST_COMMON, NSecInterface, "Mouse", &Opt.Mouse, 1},
	{OST_NONE,   NSecInterface, "UseVk_oem_x", &Opt.UseVk_oem_x, 1},
	{OST_COMMON, NSecInterface, "ShowMenuBar", &Opt.ShowMenuBar, 0},
	{OST_NONE,   NSecInterface, "CursorSize1", &Opt.CursorSize[0], 15},
	{OST_NONE,   NSecInterface, "CursorSize2", &Opt.CursorSize[1], 10},
	{OST_NONE,   NSecInterface, "CursorSize3", &Opt.CursorSize[2], 99},
	{OST_NONE,   NSecInterface, "CursorSize4", &Opt.CursorSize[3], 99},
	{OST_NONE,   NSecInterface, "ShiftsKeyRules", &Opt.ShiftsKeyRules, 1},
	{OST_COMMON, NSecInterface, "CtrlPgUp", &Opt.PgUpChangeDisk, 1},

	{OST_COMMON, NSecInterface, "ConsolePaintSharp", &Opt.ConsolePaintSharp, 0},
	{OST_COMMON, NSecInterface, "ExclusiveCtrlLeft", &Opt.ExclusiveCtrlLeft, 0},
	{OST_COMMON, NSecInterface, "ExclusiveCtrlRight", &Opt.ExclusiveCtrlRight, 0},
	{OST_COMMON, NSecInterface, "ExclusiveAltLeft", &Opt.ExclusiveAltLeft, 0},
	{OST_COMMON, NSecInterface, "ExclusiveAltRight", &Opt.ExclusiveAltRight, 0},
	{OST_COMMON, NSecInterface, "ExclusiveWinLeft", &Opt.ExclusiveWinLeft, 0},
	{OST_COMMON, NSecInterface, "ExclusiveWinRight", &Opt.ExclusiveWinRight, 0},

	{OST_COMMON,  NSecInterface, "CopyToPrimarySelection", &Opt.CopyToPrimarySelection, 0},
	{OST_COMMON,  NSecInterface, "PasteFromPrimarySelection", &Opt.PasteFromPrimarySelection, 0},

	{OST_COMMON, NSecInterface, "DateFormat", &Opt.DateFormat, GetDateFormatDefault()},
	{OST_COMMON, NSecInterface, "DateSeparator", &Opt.strDateSeparator, GetDateSeparatorDefaultStr()},
	{OST_COMMON, NSecInterface, "TimeSeparator", &Opt.strTimeSeparator, GetTimeSeparatorDefaultStr()},
	{OST_COMMON, NSecInterface, "DecimalSeparator", &Opt.strDecimalSeparator, GetDecimalSeparatorDefaultStr()},

	{OST_COMMON, NSecInterface, "OSC52ClipSet", &Opt.OSC52ClipSet, 0},
	{OST_COMMON, NSecInterface, "TTYPaletteOverride", &Opt.TTYPaletteOverride, 1},

	{OST_NONE,   NSecInterface, "ShowTimeoutDelFiles", &Opt.ShowTimeoutDelFiles, 50},
	{OST_NONE,   NSecInterface, "ShowTimeoutDACLFiles", &Opt.ShowTimeoutDACLFiles, 50},
	{OST_NONE,   NSecInterface, "FormatNumberSeparators", &Opt.FormatNumberSeparators, 0},
	{OST_COMMON, NSecInterface, "CopyShowTotal", &Opt.CMOpt.CopyShowTotal, 1},
	{OST_COMMON, NSecInterface, "DelShowTotal", &Opt.DelOpt.DelShowTotal, 0},
	{OST_COMMON, NSecInterface, "WindowTitle", &Opt.strWindowTitle, L"%State - FAR2L %Ver %Backend %User@%Host"}, // %Platform
	{OST_COMMON, NSecInterfaceCompletion, "Exceptions", &Opt.AutoComplete.Exceptions, L"git*reset*--hard;*://*:*@*;\" *\""},
	{OST_COMMON, NSecInterfaceCompletion, "ShowList", &Opt.AutoComplete.ShowList, 1},
	{OST_COMMON, NSecInterfaceCompletion, "ModalList", &Opt.AutoComplete.ModalList, 0},
	{OST_COMMON, NSecInterfaceCompletion, "Append", &Opt.AutoComplete.AppendCompletion, 0},

	{OST_COMMON, NSecViewer, "ExternalViewerName", &Opt.strExternalViewer, L""},
	{OST_COMMON, NSecViewer, "UseExternalViewer", &Opt.ViOpt.UseExternalViewer, 0},
	{OST_COMMON, NSecViewer, "SaveViewerPos", &Opt.ViOpt.SavePos, 1},
	{OST_COMMON, NSecViewer, "SaveViewerShortPos", &Opt.ViOpt.SaveShortPos, 1},
	{OST_COMMON, NSecViewer, "AutoDetectCodePage", &Opt.ViOpt.AutoDetectCodePage, 0},
	{OST_COMMON, NSecViewer, "SearchRegexp", &Opt.ViOpt.SearchRegexp, 0},

	{OST_COMMON, NSecViewer, "TabSize", &Opt.ViOpt.TabSize, 8},
	{OST_COMMON, NSecViewer, "ShowKeyBar", &Opt.ViOpt.ShowKeyBar, 1},
	{OST_COMMON, NSecViewer, "ShowTitleBar", &Opt.ViOpt.ShowTitleBar, 1},
	{OST_COMMON, NSecViewer, "ShowArrows", &Opt.ViOpt.ShowArrows, 1},
	{OST_COMMON, NSecViewer, "ClickableURLs", &Opt.ViOpt.ClickableURLs, 1},
	{OST_COMMON, NSecViewer, "ShowScrollbar", &Opt.ViOpt.ShowScrollbar, 0},
	{OST_COMMON, NSecViewer, "IsWrap", &Opt.ViOpt.ViewerIsWrap, 1},
	{OST_COMMON, NSecViewer, "Wrap", &Opt.ViOpt.ViewerWrap, 0},
	{OST_COMMON, NSecViewer, "PersistentBlocks", &Opt.ViOpt.PersistentBlocks, 0},
	{OST_COMMON, NSecViewer, "DefaultCodePage", &Opt.ViOpt.DefaultCodePage, CP_UTF8},
	{OST_COMMON, NSecViewer, "ShowMenuBar", &Opt.ViOpt.ShowMenuBar, 0},

	{OST_COMMON, NSecDialog, "EditHistory", &Opt.Dialogs.EditHistory, 1},
	{OST_COMMON, NSecDialog, "EditBlock", &Opt.Dialogs.EditBlock, 0},
	{OST_COMMON, NSecDialog, "AutoComplete", &Opt.Dialogs.AutoComplete, 1},
	{OST_COMMON, NSecDialog, "EULBsClear", &Opt.Dialogs.EULBsClear, 0},
	{OST_NONE,   NSecDialog, "SelectFromHistory", &Opt.Dialogs.SelectFromHistory, 0},
	{OST_NONE,   NSecDialog, "EditLine", &Opt.Dialogs.EditLine, 0},
	{OST_COMMON, NSecDialog, "MouseButton", &Opt.Dialogs.MouseButton, 0xFFFF},
	{OST_COMMON, NSecDialog, "DelRemovesBlocks", &Opt.Dialogs.DelRemovesBlocks, 1},
	{OST_NONE,   NSecDialog, "CBoxMaxHeight", &Opt.Dialogs.CBoxMaxHeight, 24},
	{OST_COMMON, NSecDialog, "ShowArrowsInEdit", &Opt.Dialogs.ShowArrowsInEdit, 1},

	{OST_COMMON, NSecEditor, "ExternalEditorName", &Opt.strExternalEditor, L""},
	{OST_COMMON, NSecEditor, "UseExternalEditor", &Opt.EdOpt.UseExternalEditor, 0},
	{OST_COMMON, NSecEditor, "ExpandTabs", &Opt.EdOpt.ExpandTabs, 0},
	{OST_COMMON, NSecEditor, "TabSize", &Opt.EdOpt.TabSize, 8},
	{OST_COMMON, NSecEditor, "PersistentBlocks", &Opt.EdOpt.PersistentBlocks, 0},
	{OST_COMMON, NSecEditor, "DelRemovesBlocks", &Opt.EdOpt.DelRemovesBlocks, 1},
	{OST_COMMON, NSecEditor, "AutoIndent", &Opt.EdOpt.AutoIndent, 0},
	{OST_COMMON, NSecEditor, "SaveEditorPos", &Opt.EdOpt.SavePos, 1},
	{OST_COMMON, NSecEditor, "SaveEditorShortPos", &Opt.EdOpt.SaveShortPos, 1},
	{OST_COMMON, NSecEditor, "AutoDetectCodePage", &Opt.EdOpt.AutoDetectCodePage, 0},
	{OST_COMMON, NSecEditor, "EditorCursorBeyondEOL", &Opt.EdOpt.CursorBeyondEOL, 1},
	{OST_COMMON, NSecEditor, "ReadOnlyLock", &Opt.EdOpt.ReadOnlyLock, 0}, // Вернём назад дефолт 1.65 - не предупреждать и не блокировать
	{OST_NONE,   NSecEditor, "EditorUndoSize", &Opt.EdOpt.UndoSize, 0}, // $ 03.12.2001 IS размер буфера undo в редакторе
	{OST_NONE,   NSecEditor, "WordDiv", &Opt.strWordDiv, WordDiv0},
	{OST_NONE,   NSecEditor, "BSLikeDel", &Opt.EdOpt.BSLikeDel, 1},
	{OST_NONE,   NSecEditor, "FileSizeLimit", &Opt.EdOpt.FileSizeLimitLo, 0},
	{OST_NONE,   NSecEditor, "FileSizeLimitHi", &Opt.EdOpt.FileSizeLimitHi, 0},
	{OST_NONE,   NSecEditor, "CharCodeBase", &Opt.EdOpt.CharCodeBase, 1},
	{OST_NONE,   NSecEditor, "AllowEmptySpaceAfterEof", &Opt.EdOpt.AllowEmptySpaceAfterEof, 0},//skv
	{OST_COMMON, NSecEditor, "DefaultCodePage", &Opt.EdOpt.DefaultCodePage, CP_UTF8},
	{OST_COMMON, NSecEditor, "ShowKeyBar", &Opt.EdOpt.ShowKeyBar, 1},
	{OST_COMMON, NSecEditor, "ShowTitleBar", &Opt.EdOpt.ShowTitleBar, 1},
	{OST_COMMON, NSecEditor, "ShowMenuBar", &Opt.EdOpt.ShowMenuBar, 0},
	{OST_COMMON, NSecEditor, "ShowScrollBar", &Opt.EdOpt.ShowScrollBar, 0},
	{OST_COMMON, NSecEditor, "UseEditorConfigOrg", &Opt.EdOpt.UseEditorConfigOrg, 1},
	{OST_COMMON, NSecEditor, "SearchSelFound", &Opt.EdOpt.SearchSelFound, 0},
	{OST_COMMON, NSecEditor, "SearchRegexp", &Opt.EdOpt.SearchRegexp, 0},
	{OST_COMMON, NSecEditor, "SearchPickUpWord", &Opt.EdOpt.SearchPickUpWord, 0},
	{OST_COMMON, NSecEditor, "WordWrap", &Opt.EdOpt.WordWrap, 0},
	{OST_COMMON, NSecEditor, "ShowWhiteSpace", &Opt.EdOpt.ShowWhiteSpace, 0},
	{OST_COMMON, NSecEditor, "ShowLineNumbers", &Opt.EdOpt.ShowLineNumbers, 0},

	{OST_COMMON, NSecNotifications, "OnFileOperation", &Opt.NotifOpt.OnFileOperation, 1},
	{OST_COMMON, NSecNotifications, "OnConsole", &Opt.NotifOpt.OnConsole, 1},
	{OST_COMMON, NSecNotifications, "OnlyIfBackground", &Opt.NotifOpt.OnlyIfBackground, 1},

	{OST_NONE,   NSecXLat, "Flags", &Opt.XLat.Flags, (DWORD)XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE},
	{OST_COMMON, NSecXLat, "EnableForFastFileFind", &Opt.XLat.EnableForFastFileFind, 1},
	{OST_COMMON, NSecXLat, "EnableForDialogs", &Opt.XLat.EnableForDialogs, 1},
	{OST_COMMON, NSecXLat, "WordDivForXlat", &Opt.XLat.strWordDivForXlat, WordDivForXlat0},
	{OST_COMMON, NSecXLat, "XLat", &Opt.XLat.XLat, L"ru:qwerty-йцукен"},

	{OST_COMMON, NSecSavedHistory, NParamHistoryCount, &Opt.HistoryCount, 512},
	{OST_COMMON, NSecSavedFolderHistory, NParamHistoryCount, &Opt.FoldersHistoryCount, 512},
	{OST_COMMON, NSecSavedViewHistory, NParamHistoryCount, &Opt.ViewHistoryCount, 512},
	{OST_COMMON, NSecSavedDialogHistory, NParamHistoryCount, &Opt.DialogsHistoryCount, 512},

	{OST_COMMON, NSecSystem, "PersonalPluginsPath", &Opt.LoadPlug.strPersonalPluginsPath, L""},
	{OST_COMMON, NSecSystem, "SaveHistory", &Opt.SaveHistory, 1},
	{OST_COMMON, NSecSystem, "SaveFoldersHistory", &Opt.SaveFoldersHistory, 1},
	{OST_NONE,   NSecSystem, "SavePluginFoldersHistory", &Opt.SavePluginFoldersHistory, 0},
	{OST_COMMON, NSecSystem, "SaveViewHistory", &Opt.SaveViewHistory, 1},
	{OST_COMMON, NSecSystem, "HistoryRemoveDupsRule", &Opt.HistoryRemoveDupsRule, HISTORY_REMOVE_DUPS_BY_NAME_EXTRA},
	{OST_COMMON, NSecSystem, "AutoHighlightHistory", &Opt.AutoHighlightHistory, 1},
	{OST_COMMON, NSecSystem, NParamAutoSaveSetup, &Opt.AutoSaveSetup, 0},
	{OST_COMMON, NSecSystem, NParamAutoSavePanels, &Opt.AutoSavePanels, 0},
	{OST_COMMON, NSecSystem, "DeleteToRecycleBin", &Opt.DeleteToRecycleBin, 0},
	{OST_COMMON, NSecSystem, "DeleteToRecycleBinKillLink", &Opt.DeleteToRecycleBinKillLink, 1},
	{OST_NONE,   NSecSystem, "WipeSymbol", &Opt.WipeSymbol, 0},
	{OST_COMMON, NSecSystem, "SudoEnabled", &Opt.SudoEnabled, 1},
	{OST_COMMON, NSecSystem, "SudoConfirmModify", &Opt.SudoConfirmModify, 1},
	{OST_COMMON, NSecSystem, "SudoPasswordExpiration", &Opt.SudoPasswordExpiration, 15 * 60},

	{OST_COMMON, NSecSystem, "UseCOW", &Opt.CMOpt.UseCOW, 0},
	{OST_COMMON, NSecSystem, "SparseFiles", &Opt.CMOpt.SparseFiles, 0},
	{OST_COMMON, NSecSystem, "HowCopySymlink", &Opt.CMOpt.HowCopySymlink, 1},
	{OST_COMMON, NSecSystem, "WriteThrough", &Opt.CMOpt.WriteThrough, 0},
	{OST_COMMON, NSecSystem, "CopyXAttr", &Opt.CMOpt.CopyXAttr, 0},
	{OST_NONE,   NSecSystem, "CopyAccessMode", &Opt.CMOpt.CopyAccessMode, 1},
	{OST_COMMON, NSecSystem, "MultiCopy", &Opt.CMOpt.MultiCopy, 0},
	{OST_COMMON, NSecSystem, "CopyTimeRule", &Opt.CMOpt.CopyTimeRule, 3},

	{OST_COMMON, NSecSystem, "MakeLinkSuggestSymlinkAlways", &Opt.MakeLinkSuggestSymlinkAlways, 1},

	{OST_COMMON, NSecSystem, "InactivityExit", &Opt.InactivityExit, 0},
	{OST_COMMON, NSecSystem, "InactivityExitTime", &Opt.InactivityExitTime, 15},
	{OST_COMMON, NSecSystem, "DriveMenuMode2", &Opt.ChangeDriveMode, -1},
	{OST_COMMON, NSecSystem, "DriveDisconnectMode", &Opt.ChangeDriveDisconnectMode, 1},

	{OST_COMMON, NSecSystem, "DriveExceptions", &Opt.ChangeDriveExceptions,
		L"/System*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron;/tmp/.*"},
	{OST_COMMON, NSecSystem, "DriveColumn2", &Opt.ChangeDriveColumn2, L"$U$</$>$T"},
	{OST_COMMON, NSecSystem, "DriveColumn3", &Opt.ChangeDriveColumn3, L"$S$D"},

	{OST_COMMON, NSecSystem, "AutoUpdateRemoteDrive", &Opt.AutoUpdateRemoteDrive, 1},
	{OST_COMMON, NSecSystem, "FileSearchMode", &Opt.FindOpt.FileSearchMode, FINDAREA_FROM_CURRENT},
	{OST_NONE,   NSecSystem, "CollectFiles", &Opt.FindOpt.CollectFiles, 1},
	{OST_COMMON, NSecSystem, "SearchInFirstSize", &Opt.FindOpt.strSearchInFirstSize, L""},
	{OST_COMMON, NSecSystem, "FindAlternateStreams", &Opt.FindOpt.FindAlternateStreams, 0},
	{OST_COMMON, NSecSystem, "SearchOutFormat", &Opt.FindOpt.strSearchOutFormat, L"D,S,A"},
	{OST_COMMON, NSecSystem, "SearchOutFormatWidth", &Opt.FindOpt.strSearchOutFormatWidth, L"14,13,0"},
	{OST_COMMON, NSecSystem, "FindFolders", &Opt.FindOpt.FindFolders, 1},
	{OST_COMMON, NSecSystem, "FindSymLinks", &Opt.FindOpt.FindSymLinks, 1},
	{OST_COMMON, NSecSystem, "FindCaseSensitiveFileMask", &Opt.FindOpt.FindCaseSensitiveFileMask, 1},
	{OST_COMMON, NSecSystem, "UseFilterInSearch", &Opt.FindOpt.UseFilter, 0},
	{OST_COMMON, NSecSystem, "FindCodePage", &Opt.FindCodePage, CP_AUTODETECT},
	{OST_NONE,   NSecSystem, "CmdHistoryRule", &Opt.CmdHistoryRule, 0},
	{OST_NONE,   NSecSystem, "SetAttrFolderRules", &Opt.SetAttrFolderRules, 1},
	{OST_NONE,   NSecSystem, "MaxPositionCache", &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS},
	{OST_NONE,   NSecSystem, "ConsoleDetachKey", &strKeyNameConsoleDetachKey, L"CtrlAltTab"},
	{OST_NONE,   NSecSystem, "SilentLoadPlugin", &Opt.LoadPlug.SilentLoadPlugin, 0},
	{OST_COMMON, NSecSystem, "ScanSymlinks", &Opt.LoadPlug.ScanSymlinks, 1},
	{OST_COMMON, NSecSystem, "MultiMakeDir", &Opt.MultiMakeDir, 0},
	{OST_NONE,   NSecSystem, "MsWheelDelta", &Opt.MsWheelDelta, 1},
	{OST_NONE,   NSecSystem, "MsWheelDeltaView", &Opt.MsWheelDeltaView, 1},
	{OST_NONE,   NSecSystem, "MsWheelDeltaEdit", &Opt.MsWheelDeltaEdit, 1},
	{OST_NONE,   NSecSystem, "MsWheelDeltaHelp", &Opt.MsWheelDeltaHelp, 1},
	{OST_NONE,   NSecSystem, "MsHWheelDelta", &Opt.MsHWheelDelta, 1},
	{OST_NONE,   NSecSystem, "MsHWheelDeltaView", &Opt.MsHWheelDeltaView, 1},
	{OST_NONE,   NSecSystem, "MsHWheelDeltaEdit", &Opt.MsHWheelDeltaEdit, 1},
	{OST_NONE,   NSecSystem, "SubstNameRule", &Opt.SubstNameRule, 2},
	{OST_NONE,   NSecSystem, "ShowCheckingFile", &Opt.ShowCheckingFile, 0},
	{OST_NONE,   NSecSystem, "QuotedSymbols", &Opt.strQuotedSymbols, L" $&()[]{};|*?!'`\"\\\xA0"}, //xA0 => 160 =>oem(0xFF)
	{OST_NONE,   NSecSystem, "QuotedName", &Opt.QuotedName, QUOTEDNAME_INSERT},
	//{OST_NONE,   NSecSystem, "CPAJHefuayor", &Opt.strCPAJHefuayor, 0},
	{OST_NONE,   NSecSystem, "PluginMaxReadData", &Opt.PluginMaxReadData, 0x40000},
	{OST_NONE,   NSecSystem, "UseNumPad", &Opt.UseNumPad, 1},
	{OST_NONE,   NSecSystem, "CASRule", &Opt.CASRule, -1},
	{OST_NONE,   NSecSystem, "AllCtrlAltShiftRule", &Opt.AllCtrlAltShiftRule, 0x0000FFFF},
	{OST_COMMON, NSecSystem, "ScanJunction", &Opt.ScanJunction, 1},
	{OST_COMMON, NSecSystem, "OnlyFilesSize", &Opt.OnlyFilesSize, 0},
	{OST_NONE,   NSecSystem, "UsePrintManager", &Opt.UsePrintManager, 1},

	{OST_COMMON, NSecSystem, "ExcludeCmdHistory", &Opt.ExcludeCmdHistory, 0}, //AN

	{OST_COMMON, NSecSystem, "FolderInfo", &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{OST_COMMON, NSecSystem, "OwnerGroupShowId", &Opt.OwnerGroupShowId, 0},

	{OST_NONE,   NSecSystemNowell, "MoveRO", &Opt.Nowell.MoveRO, 1},

	{OST_NONE,   NSecSystemExecutor, "RestoreCP", &Opt.RestoreCPAfterExecute, 1},
	{OST_NONE,   NSecSystemExecutor, "UseAppPath", &Opt.ExecuteUseAppPath, 1},
	{OST_NONE,   NSecSystemExecutor, "ShowErrorMessage", &Opt.ExecuteShowErrorMessage, 1},
	{OST_NONE,   NSecSystemExecutor, "FullTitle", &Opt.ExecuteFullTitle, 0},
	{OST_NONE,   NSecSystemExecutor, "SilentExternal", &Opt.ExecuteSilentExternal, 0},

	{OST_NONE,   NSecPanelTree, "MinTreeCount", &Opt.Tree.MinTreeCount, 4},
	{OST_NONE,   NSecPanelTree, "TreeFileAttr", &Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN},
	{OST_NONE,   NSecPanelTree, "LocalDisk", &Opt.Tree.LocalDisk, 2},
	{OST_NONE,   NSecPanelTree, "NetDisk", &Opt.Tree.NetDisk, 2},
	{OST_NONE,   NSecPanelTree, "RemovableDisk", &Opt.Tree.RemovableDisk, 2},
	{OST_NONE,   NSecPanelTree, "NetPath", &Opt.Tree.NetPath, 2},
	{OST_COMMON, NSecPanelTree, "AutoChangeFolder", &Opt.Tree.AutoChangeFolder, 0}, // ???
	{OST_COMMON, NSecPanelTree, "ExclSubTreeMask", &Opt.Tree.ExclSubTreeMask, L".*"},
	{OST_COMMON, NSecPanelTree, "ScanDepthEnabled", &Opt.Tree.ScanDepthEnabled, 1},
	{OST_COMMON, NSecPanelTree, "DefaultScanDepth", &Opt.Tree.DefaultScanDepth, 4},

	{OST_COMMON, NSecLanguage, "Help", &Opt.strHelpLanguage, L"English"},
	{OST_COMMON, NSecLanguage, "Main", &Opt.strLanguage, L"English"},

	{OST_COMMON, NSecConfirmations, "Copy", &Opt.Confirm.Copy, 1},
	{OST_COMMON, NSecConfirmations, "Move", &Opt.Confirm.Move, 1},
	{OST_COMMON, NSecConfirmations, "RO", &Opt.Confirm.RO, 1},
	{OST_COMMON, NSecConfirmations, "Drag", &Opt.Confirm.Drag, 1},
	{OST_COMMON, NSecConfirmations, "Delete", &Opt.Confirm.Delete, 1},
	{OST_COMMON, NSecConfirmations, "DeleteFolder", &Opt.Confirm.DeleteFolder, 1},
	{OST_COMMON, NSecConfirmations, "Esc", &Opt.Confirm.Esc, 1},
	{OST_COMMON, NSecConfirmations, "RemoveConnection", &Opt.Confirm.RemoveConnection, 1},
	{OST_COMMON, NSecConfirmations, "ClearVT", &Opt.Confirm.ClearVT, 1},
	{OST_COMMON, NSecConfirmations, "RemoveHotPlug", &Opt.Confirm.RemoveHotPlug, 1},
	{OST_COMMON, NSecConfirmations, "AllowReedit", &Opt.Confirm.AllowReedit, 1},
	{OST_COMMON, NSecConfirmations, "HistoryClear", &Opt.Confirm.HistoryClear, 1},
	{OST_COMMON, NSecConfirmations, "Exit", &Opt.Confirm.Exit, 1},
	{OST_COMMON, NSecConfirmations, "ExitOrBknd", &Opt.Confirm.ExitOrBknd, 1},
	{OST_NONE,   NSecConfirmations, "EscTwiceToInterrupt", &Opt.Confirm.EscTwiceToInterrupt, 0},

	{OST_COMMON, NSecPluginConfirmations,  "OpenFilePlugin", &Opt.PluginConfirm.OpenFilePlugin, 0},
	{OST_COMMON, NSecPluginConfirmations,  "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0},
	{OST_COMMON, NSecPluginConfirmations,  "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0},
	{OST_COMMON, NSecPluginConfirmations,  "SetFindList", &Opt.PluginConfirm.SetFindList, 0},
	{OST_COMMON, NSecPluginConfirmations,  "Prefix", &Opt.PluginConfirm.Prefix, 0},

	{OST_NONE,   NSecPanel, "ShellRightLeftArrowsRule", &Opt.ShellRightLeftArrowsRule, 0},
	{OST_COMMON, NSecPanel, "ShowHidden", &Opt.ShowHidden, 1},
	{OST_COMMON, NSecPanel, "Highlight", &Opt.Highlight, 1},
	{OST_COMMON, NSecPanel, "SortFolderExt", &Opt.SortFolderExt, 0},
	{OST_COMMON, NSecPanel, "SelectFolders", &Opt.SelectFolders, 0},
	{OST_COMMON, NSecPanel, "AttrStrStyle", &Opt.AttrStrStyle, 1},
	{OST_COMMON, NSecPanel, "CaseSensitiveCompareSelect", &Opt.PanelCaseSensitiveCompareSelect, 1},
	{OST_COMMON, NSecPanel, "ReverseSort", &Opt.ReverseSort, 1},
	{OST_NONE,   NSecPanel, "RightClickRule", &Opt.PanelRightClickRule, 2},
	{OST_NONE,   NSecPanel, "CtrlAltShiftRule", &Opt.PanelCtrlAltShiftRule, 0},
	{OST_NONE,   NSecPanel, "RememberLogicalDrives", &Opt.RememberLogicalDrives, 0},
	{OST_COMMON, NSecPanel, "AutoUpdateLimit", &Opt.AutoUpdateLimit, 0},
	{OST_COMMON, NSecPanel, "ShowFilenameMarks", &Opt.ShowFilenameMarks, 1},
	{OST_COMMON, NSecPanel, "FilenameMarksAlign", &Opt.FilenameMarksAlign, 1},
	{OST_COMMON, NSecPanel, "FilenameMarksInStatusBar", &Opt.FilenameMarksAlign, 1},
	{OST_COMMON, NSecPanel, "MinFilenameIndentation", &Opt.MinFilenameIndentation, 0},
	{OST_COMMON, NSecPanel, "MaxFilenameIndentation", &Opt.MaxFilenameIndentation, HIGHLIGHT_MAX_MARK_LENGTH},
	{OST_COMMON, NSecPanel, "DirNameStyle", &Opt.DirNameStyle, 0 },
	{OST_COMMON, NSecPanel, "DirNameStyleColumnWidthAlways", &Opt.DirNameStyleColumnWidthAlways, 0 },
	{OST_COMMON, NSecPanel, "ShowSymlinkSize", &Opt.ShowSymlinkSize, 0},
	{OST_COMMON, NSecPanel, "ClassicHotkeyLinkResolving", &Opt.ClassicHotkeyLinkResolving, 1},

	{OST_PANELS, NSecPanelLeft, "Type", &Opt.LeftPanel.Type, 0},
	{OST_PANELS, NSecPanelLeft, "Visible", &Opt.LeftPanel.Visible, 1},
	{OST_PANELS, NSecPanelLeft, "Focus", &Opt.LeftPanel.Focus, 1},
	{OST_PANELS, NSecPanelLeft, "ViewMode", &Opt.LeftPanel.ViewMode, 2},
	{OST_PANELS, NSecPanelLeft, "SortMode", &Opt.LeftPanel.SortMode, 1},
	{OST_PANELS, NSecPanelLeft, "SortOrder", &Opt.LeftPanel.SortOrder, 1},
	{OST_PANELS, NSecPanelLeft, "SortGroups", &Opt.LeftPanel.SortGroups, 0},
	{OST_PANELS, NSecPanelLeft, "NumericSort", &Opt.LeftPanel.NumericSort, 0},
	{OST_PANELS, NSecPanelLeft, "CaseSensitiveSort", &Opt.LeftPanel.CaseSensitiveSort, 0},
	{OST_PANELS, NSecPanelLeft, "Folder", &Opt.strLeftFolder, L""},
	{OST_PANELS, NSecPanelLeft, "CurFile", &Opt.strLeftCurFile, L""},
	{OST_PANELS, NSecPanelLeft, "SelectedFirst", &Opt.LeftSelectedFirst, 0},
	{OST_PANELS, NSecPanelLeft, "DirectoriesFirst", &Opt.LeftPanel.DirectoriesFirst, 1},
	{OST_PANELS, NSecPanelLeft, "ExecutablesFirst", &Opt.LeftPanel.ExecutablesFirst, 0},

	{OST_PANELS, NSecPanelRight, "Type", &Opt.RightPanel.Type, 0},
	{OST_PANELS, NSecPanelRight, "Visible", &Opt.RightPanel.Visible, 1},
	{OST_PANELS, NSecPanelRight, "Focus", &Opt.RightPanel.Focus, 0},
	{OST_PANELS, NSecPanelRight, "ViewMode", &Opt.RightPanel.ViewMode, 2},
	{OST_PANELS, NSecPanelRight, "SortMode", &Opt.RightPanel.SortMode, 1},
	{OST_PANELS, NSecPanelRight, "SortOrder", &Opt.RightPanel.SortOrder, 1},
	{OST_PANELS, NSecPanelRight, "SortGroups", &Opt.RightPanel.SortGroups, 0},
	{OST_PANELS, NSecPanelRight, "NumericSort", &Opt.RightPanel.NumericSort, 0},
	{OST_PANELS, NSecPanelRight, "CaseSensitiveSort", &Opt.RightPanel.CaseSensitiveSort, 0},
	{OST_PANELS, NSecPanelRight, "Folder", &Opt.strRightFolder, L""},
	{OST_PANELS, NSecPanelRight, "CurFile", &Opt.strRightCurFile, L""},
	{OST_PANELS, NSecPanelRight, "SelectedFirst", &Opt.RightSelectedFirst, 0},
	{OST_PANELS, NSecPanelRight, "DirectoriesFirst", &Opt.RightPanel.DirectoriesFirst, 1},
	{OST_PANELS, NSecPanelRight, "ExecutablesFirst", &Opt.RightPanel.ExecutablesFirst, 0},

	{OST_COMMON, NSecPanelLayout, "ColumnTitles", &Opt.ShowColumnTitles, 1},
	{OST_COMMON, NSecPanelLayout, "StatusLine", &Opt.ShowPanelStatus, 1},
	{OST_COMMON, NSecPanelLayout, "TotalInfo", &Opt.ShowPanelTotals, 1},
	{OST_COMMON, NSecPanelLayout, "FreeInfo", &Opt.ShowPanelFree, 0},
	{OST_COMMON, NSecPanelLayout, "Scrollbar", &Opt.ShowPanelScrollbar, 0},
	{OST_NONE,   NSecPanelLayout, "ScrollbarMenu", &Opt.ShowMenuScrollbar, 1},
	{OST_COMMON, NSecPanelLayout, "ScreensNumber", &Opt.ShowScreensNumber, 1},
	{OST_COMMON, NSecPanelLayout, "SortMode", &Opt.ShowSortMode, 1},

	{OST_COMMON, NSecLayout, "LeftHeightDecrement", &Opt.LeftHeightDecrement, 0},
	{OST_COMMON, NSecLayout, "RightHeightDecrement", &Opt.RightHeightDecrement, 0},
	{OST_COMMON, NSecLayout, "WidthDecrement", &Opt.WidthDecrement, 0},
	{OST_COMMON, NSecLayout, "FullscreenHelp", &Opt.FullScreenHelp, 0},
	{OST_COMMON, NSecLayout, "PanelsDisposition", &Opt.PanelsDisposition, 0},

	{OST_COMMON, NSecDescriptions, "ListNames", &Opt.Diz.strListNames, L"Descript.ion,Files.bbs"},
	{OST_COMMON, NSecDescriptions, "UpdateMode", &Opt.Diz.UpdateMode, DIZ_UPDATE_IF_DISPLAYED},
	{OST_COMMON, NSecDescriptions, "ROUpdate", &Opt.Diz.ROUpdate, 0},
	{OST_COMMON, NSecDescriptions, "SetHidden", &Opt.Diz.SetHidden, 1},
	{OST_COMMON, NSecDescriptions, "StartPos", &Opt.Diz.StartPos, 0},
	{OST_COMMON, NSecDescriptions, "AnsiByDefault", &Opt.Diz.AnsiByDefault, 0},
	{OST_COMMON, NSecDescriptions, "SaveInUTF", &Opt.Diz.SaveInUTF, 0},

	{OST_NONE,   NSecKeyMacros, "MacroReuseRules", &Opt.Macro.MacroReuseRules, 0},
	{OST_NONE,   NSecKeyMacros, "DateFormat", &Opt.Macro.strDateFormat, L"%a %b %d %H:%M:%S %Z %Y"},
	{OST_NONE,   NSecKeyMacros, "CONVFMT", &Opt.Macro.strMacroCONVFMT, L"%.6g"},
	{OST_NONE,   NSecKeyMacros, "CallPluginRules", &Opt.Macro.CallPluginRules, 0},
	{OST_COMMON, NSecKeyMacros, "KeyRecordCtrlDot", &Opt.Macro.strKeyMacroCtrlDot, szCtrlDot},
	{OST_COMMON, NSecKeyMacros, "KeyRecordCtrlShiftDot", &Opt.Macro.strKeyMacroCtrlShiftDot, szCtrlShiftDot},

	{OST_NONE,   NSecPolicies, "ShowHiddenDrives", &Opt.Policies.ShowHiddenDrives, 1},
	{OST_NONE,   NSecPolicies, "DisabledOptions", &Opt.Policies.DisabledOptions, 0},

	{OST_COMMON, NSecCodePages, "CPMenuMode2", &Opt.CPMenuMode, 1},

	{OST_COMMON, NSecVMenu, "MenuStopWrapOnEdge", &Opt.VMenu.MenuLoopScroll, 1},

	{OST_COMMON, NSecVMenu, "LBtnClick", &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL},
	{OST_COMMON, NSecVMenu, "RBtnClick", &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL},
	{OST_COMMON, NSecVMenu, "MBtnClick", &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY},
	{OST_COMMON, NSecVMenu, "HistShowTimes", ARRAYSIZE(Opt.HistoryShowTimes), Opt.HistoryShowTimes, nullptr},
	{OST_COMMON, NSecVMenu, "HistDirsPrefixLen", &Opt.HistoryDirsPrefixLen, 20},
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
	void SaveOpt(const ConfigOpt &opt, unsigned SaveFlags)
	{
		if (!(opt.save & SaveFlags))
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
//	MergePalette();

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

	if (KeyNameToKey(Opt.Macro.strKeyMacroCtrlDot) == KEY_INVALID)
		Opt.Macro.strKeyMacroCtrlDot = szCtrlDot;

	if (KeyNameToKey(Opt.Macro.strKeyMacroCtrlShiftDot) == KEY_INVALID)
		Opt.Macro.strKeyMacroCtrlShiftDot = szCtrlShiftDot;

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

	CheckMaskGroups();
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

static void SavePanelsToOpt()
{
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
		Opt.LeftPanel.ExecutablesFirst = LeftPanel->GetExecutablesFirst();
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
		Opt.RightPanel.ExecutablesFirst = RightPanel->GetExecutablesFirst();
	}

	RightPanel->GetCurDir(Opt.strRightFolder);
	RightPanel->GetCurBaseName(Opt.strRightCurFile);
}

// Saved instantly when the "System Settings" dialog is accepted.
void ConfigOptSaveAutoOptions()
{
	ConfigWriter cfg_writer;
	cfg_writer.SelectSection(NSecSystem);
	cfg_writer.SetUInt(NParamAutoSaveSetup, Opt.AutoSaveSetup);
	cfg_writer.SetUInt(NParamAutoSavePanels, Opt.AutoSavePanels);
}

void ConfigOptSave(bool Ask)
{
	if (Opt.Policies.DisabledOptions&0x20000) // Bit 17 - Сохранить параметры
		return;

	const unsigned SaveFlags
			= (Ask || Opt.AutoSaveSetup) ? (OST_COMMON | OST_PANELS)
			: (Opt.AutoSavePanels ? OST_PANELS : OST_NONE);

	if (SaveFlags == OST_NONE)
		return;

	if (Ask && Message(0, 2, Msg::SaveSetupTitle, Msg::SaveSetupAsk1, Msg::SaveSetupAsk2, Msg::SaveSetup, Msg::Cancel))
		return;

	WINPORT(SaveConsoleWindowState)();

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	if (SaveFlags & OST_COMMON)
	{
		WINPORT(SaveConsoleWindowState)();
		CtrlObject->HiFiles->SaveHiData();
	}

	if (SaveFlags & OST_PANELS)
		SavePanelsToOpt();
	/* *************************************************** </ПРЕПРОЦЕССЫ> */

	OptConfigWriter cfg_writer;
	for (size_t i = ConfigOptCount(); i--;)
		cfg_writer.SaveOpt(g_cfg_opts[i], SaveFlags);

	/* <ПОСТПРОЦЕССЫ> *************************************************** */
	if (SaveFlags & OST_COMMON) {
		FileFilter::SaveFilters(cfg_writer);
		FileList::SavePanelModes(cfg_writer);

		if (Ask)
			CtrlObject->Macro.SaveMacros();

	    if (Opt.IsColorsChanged || Ask) 
	    {
			FarColors::SaveFarColors();
			Opt.IsColorsChanged = false;
		}
	}
	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}
