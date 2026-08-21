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
	{OST_COMMON, NSecColors, "TempColors256", TEMP_COLORS256_SIZE, g_tempcolors256, nullptr,
		L"OptMenu", L"Stores temporary 256-color palette values used by the color configuration UI" },
	{OST_COMMON, NSecColors, "TempColorsRGB", TEMP_COLORSRGB_SIZE, (BYTE *)g_tempcolorsRGB, nullptr,
		L"OptMenu", L"Stores temporary RGB palette values used by the color configuration UI" },

	{OST_COMMON, NSecColors, "CurrentTheme", &Opt.CurrentTheme, L"" ,
		L"OptMenu", L"The name of the currently selected color theme" },
	{OST_COMMON, NSecColors, "CurrentThemeIsSystemWide", &Opt.IsSystemTheme, 0 ,
		L"OptMenu", L"Marks whether the current color theme is loaded from the system-wide theme location" },

	{OST_COMMON, NSecScreen, "Clock", &Opt.Clock, 1,
		L"InterfSettings", L"Shows the clock in the main interface" },
	{OST_COMMON, NSecScreen, "ViewerEditorClock", &Opt.ViewerEditorClock, 0,
		L"InterfSettings", L"Shows the clock in the viewer and editor title area when available" },
	{OST_COMMON, NSecScreen, "KeyBar", &Opt.ShowKeyBar, 1,
		L"InterfSettings", L"Shows the function-key bar at the bottom of the main screen" },
	{OST_COMMON, NSecScreen, "ScreenSaver", &Opt.ScreenSaver, 0,
		L"InterfSettings", L"Enables the built-in screen saver after a period of inactivity" },
	{OST_COMMON, NSecScreen, "ScreenSaverTime", &Opt.ScreenSaverTime, 5,
		L"InterfSettings", L"The inactivity timeout, in minutes, before the screen saver starts" },
	{OST_NONE,   NSecScreen, "DeltaXY", &Opt.ScrSize.dwDeltaXY, 0,
		nullptr, L"Stores the console size delta used when toggling the expanded screen layout; low word is the X delta and high word is the Y delta" },
	{OST_COMMON, NSecScreen, "CursorBlinkInterval", &Opt.CursorBlinkTime, 500,
		L"InterfSettings", L"The cursor blink interval in milliseconds (GUI-backend only)" },

	{OST_COMMON, NSecCmdline, "UsePromptFormat", &Opt.CmdLine.UsePromptFormat, 0,
		L"CmdlineSettings", L"Uses the custom command-line prompt format instead of the default prompt" },
	{OST_COMMON, NSecCmdline, "PromptFormat", &Opt.CmdLine.strPromptFormat, L"$p$# ",
		L"CommandPrompt", L"The format string used to build the command-line prompt; available values see in help" },
	{OST_COMMON, NSecCmdline, "UseShell", &Opt.CmdLine.UseShell, 0,
		L"CmdlineSettings", L"Runs commands through the configured interactive shell" },
	{OST_COMMON, NSecCmdline, "ShellCmd", &Opt.CmdLine.strShell, L"bash -i",
		L"CmdlineSettings", L"The shell command used when command-line shell execution is enabled" },
	{OST_COMMON, NSecCmdline, "DelRemovesBlocks", &Opt.CmdLine.DelRemovesBlocks, 1,
		L"CmdlineSettings", L"Allows Delete to remove selected text blocks in the command line" },
	{OST_COMMON, NSecCmdline, "EditBlock", &Opt.CmdLine.EditBlock, 0,
		L"CmdlineSettings", L"Enables block selection behavior in the command-line edit field" },
	{OST_COMMON, NSecCmdline, "AutoComplete", &Opt.CmdLine.AutoComplete, 1,
		L"CmdlineSettings", L"Enables autocompletion in the command line" },
	{OST_COMMON, NSecCmdline, "Splitter", &Opt.CmdLine.Splitter, 1,
		L"CmdlineSettings", L"Shows a visual splitter between the command line and terminal output" },
	{OST_COMMON, NSecCmdline, "WaitKeypress", &Opt.CmdLine.WaitKeypress, 1,
		L"CmdlineSettings", L"Controls whether FAR2L waits for a key press after an executed command finishes: 0=never, 1=only on error, 2=always" },
	{OST_COMMON, NSecCmdline, "VTLogLimitKB", &Opt.CmdLine.VTLogLimit, 1024,
		L"Terminal", L"The maximum terminal log size kept for the command line, in kilobytes" },
	{OST_COMMON, NSecCmdline, "ShowStartupBanner",&Opt.ShowStartupBanner, 1,
		L"CmdlineSettings", L"Display a text block with version, copyright and keyboard tips in build-in terminal at FAR2L starts" },
	{OST_NONE,   NSecCmdline, "AskOnMultilinePaste", &Opt.CmdLine.AskOnMultilinePaste, 1,
		L"CmdLineCmd", L"Asks for confirmation before pasting multi-line text into the command line" },

	{OST_COMMON, NSecInterface, "Mouse", &Opt.Mouse, 1,
		L"InputSettings", L"Enables mouse support in the interface" },
	{OST_NONE,   NSecInterface, "UseVk_oem_x", &Opt.UseVk_oem_x, 1,
		nullptr, L"Uses OEM virtual key codes when decoding punctuation and layout-dependent keys" },
	{OST_COMMON, NSecInterface, "ShowMenuBar", &Opt.ShowMenuBar, 0,
		L"InterfSettings", L"Shows the main menu bar" },
	{OST_NONE,   NSecInterface, "CursorSize1", &Opt.CursorSize[0], 15,
		nullptr, L"Cursor size percentage 1-99 used for normal insert-mode editing; 0 lets the backend keep its default size" },
	{OST_NONE,   NSecInterface, "CursorSize2", &Opt.CursorSize[1], 10,
		nullptr, L"Cursor size percentage 1-99 used by color pickers and similar controls; 0 lets the backend keep its default size" },
	{OST_NONE,   NSecInterface, "CursorSize3", &Opt.CursorSize[2], 99,
		nullptr, L"Cursor size percentage 1-99 used for overtype-mode editing; 0 falls back to a full-height cursor" },
	{OST_NONE,   NSecInterface, "CursorSize4", &Opt.CursorSize[3], 99,
		nullptr, L"Reserved cursor size preset" },
	{OST_NONE,   NSecInterface, "ShiftsKeyRules", &Opt.ShiftsKeyRules, 1,
		L"InputSettings", L"Selects the rule used to translate Shift-modified key combinations" },
	{OST_COMMON, NSecInterface, "CtrlPgUp", &Opt.PgUpChangeDisk, 1,
		L"InterfSettings", L"Allows Ctrl+PgUp to open the drive or location menu from panels" },

	{OST_COMMON, NSecInterface, "ConsolePaintSharp", &Opt.ConsolePaintSharp, 0,
		L"InterfSettings", L"Antialiasing: uses sharper console painting (GUI-backend only)" },
	{OST_COMMON, NSecInterface, "ExclusiveCtrlLeft", &Opt.ExclusiveCtrlLeft, 0,
		L"InputSettings", L"Captures the left Ctrl key exclusively when the backend supports it" },
	{OST_COMMON, NSecInterface, "ExclusiveCtrlRight", &Opt.ExclusiveCtrlRight, 0,
		L"InputSettings", L"Captures the right Ctrl key exclusively when the backend supports it" },
	{OST_COMMON, NSecInterface, "ExclusiveAltLeft", &Opt.ExclusiveAltLeft, 0,
		L"InputSettings", L"Captures the left Alt key exclusively when the backend supports it" },
	{OST_COMMON, NSecInterface, "ExclusiveAltRight", &Opt.ExclusiveAltRight, 0,
		L"InputSettings", L"Captures the right Alt key exclusively when the backend supports it" },
	{OST_COMMON, NSecInterface, "ExclusiveWinLeft", &Opt.ExclusiveWinLeft, 0,
		L"InputSettings", L"Captures the left Win or Super key exclusively when the backend supports it" },
	{OST_COMMON, NSecInterface, "ExclusiveWinRight", &Opt.ExclusiveWinRight, 0,
		L"InputSettings", L"Captures the right Win or Super key exclusively when the backend supports it" },

	{OST_COMMON, NSecInterface, "DateFormat", &Opt.DateFormat, GetDateFormatDefault(),
		L"InterfSettings", L"Selects the date display format used by panels and dialogs: 0=MM-DD-YYYY, 1=DD-MM-YYYY, 2=YYYY-MM-DD" },
	{OST_COMMON, NSecInterface, "DateSeparator", &Opt.strDateSeparator, GetDateSeparatorDefaultStr(),
		L"InterfSettings", L"The character or string used between date fields" },
	{OST_COMMON, NSecInterface, "TimeSeparator", &Opt.strTimeSeparator, GetTimeSeparatorDefaultStr(),
		L"InterfSettings", L"The character or string used between time fields" },
	{OST_COMMON, NSecInterface, "DecimalSeparator", &Opt.strDecimalSeparator, GetDecimalSeparatorDefaultStr(),
		L"InterfSettings", L"The character or string used as the decimal separator" },

	{OST_COMMON, NSecInterface, "OSC52ClipSet", &Opt.OSC52ClipSet, 0,
		L"InterfSettings", L"Set the local clipboard using OSC52 escape sequences (TTY-backend only)" },
	{OST_COMMON, NSecInterface, "TTYPaletteOverride", &Opt.TTYPaletteOverride, 1,
		L"InterfSettings", L"Override the terminal palette (TTY-backend only)" },

	{OST_COMMON,  NSecInterface, "EnforceColorCorrection", &Opt.Dialogs.EnforceColorCorrection, 1,
		L"InterfSettings", L"Update RGB colors to make it more contrast in interface and dialogs" },

	{OST_NONE,   NSecInterface, "ShowTimeoutDelFiles", &Opt.ShowTimeoutDelFiles, 50,
		nullptr, L"Delay before showing progress for delete operations, in milliseconds" },
	{OST_NONE,   NSecInterface, "ShowTimeoutDACLFiles", &Opt.ShowTimeoutDACLFiles, 50,
		nullptr, L"Delay before showing progress for access-control operations, in milliseconds" },
	{OST_NONE,   NSecInterface, "FormatNumberSeparators", &Opt.FormatNumberSeparators, 0,
		nullptr, L"Overrides decimal and thousands separators used when formatting numbers; low word is the decimal separator character and high word is the thousands separator character" },
	{OST_COMMON, NSecInterface, "CopyShowTotal", &Opt.CMOpt.CopyShowTotal, 1,
		L"InterfSettings", L"Shows total progress information during copy and move operations" },
	{OST_COMMON, NSecInterface, "DelShowTotal", &Opt.DelOpt.DelShowTotal, 0,
		L"InterfSettings", L"Shows total progress information during delete operations" },
	{OST_COMMON, NSecInterface, "WindowTitle", &Opt.strWindowTitle, // %Platform
		L"%State - FAR2L %Ver %Backend %User@%Host",
		L"InterfSettings", L"The format string used for the terminal or window title; available variables see in help" },
	{OST_COMMON, NSecInterfaceCompletion, "Exceptions", &Opt.AutoComplete.Exceptions,
		L"git*reset*--hard;*://*:*@*;\" *\"",
		L"AutoCompleteSettings", L"Semicolon-separated wildcard patterns excluded from autocomplete suggestions and command history rules" },
	{OST_COMMON, NSecInterfaceCompletion, "ShowList", &Opt.AutoComplete.ShowList, 1,
		L"AutoCompleteSettings", L"Shows the autocomplete suggestion list" },
	{OST_COMMON, NSecInterfaceCompletion, "ModalList", &Opt.AutoComplete.ModalList, 0,
		L"AutoCompleteSettings", L"Uses a modal list for autocomplete suggestions" },
	{OST_COMMON, NSecInterfaceCompletion, "Append", &Opt.AutoComplete.AppendCompletion, 0,
		L"AutoCompleteSettings", L"Automatically appends the selected autocomplete suggestion" },

	{OST_COMMON, NSecViewer, "ExternalViewerName", &Opt.strExternalViewer, L"",
		L"ViewerSettings", L"The command used to run an external viewer" },
	{OST_COMMON, NSecViewer, "UseExternalViewer", &Opt.ViOpt.UseExternalViewer, 0,
		L"ViewerSettings", L"Uses the configured external viewer instead of the internal viewer" },
	{OST_COMMON, NSecViewer, "SaveViewerPos", &Opt.ViOpt.SavePos, 1,
		L"ViewerSettings", L"Saves and restores the viewer position for previously opened files" },
	{OST_COMMON, NSecViewer, "SaveViewerShortPos", &Opt.ViOpt.SaveShortPos, 1,
		L"ViewerSettings", L"Saves a compact viewer position history for faster restoration" },
	{OST_COMMON, NSecViewer, "AutoDetectCodePage", &Opt.ViOpt.AutoDetectCodePage, 0,
		L"ViewerSettings", L"Automatically detects the code page when opening files in the viewer" },
	{OST_COMMON, NSecViewer, "SearchRegexp", &Opt.ViOpt.SearchRegexp, 0,
		L"ViewerSearch", L"Uses regular expressions by default in viewer searches (not work, reserve for future)" },

	{OST_COMMON, NSecViewer, "TabSize", &Opt.ViOpt.TabSize, 8,
		L"ViewerSettings", L"The number of columns used to display a tab in the viewer" },
	{OST_COMMON, NSecViewer, "ShowKeyBar", &Opt.ViOpt.ShowKeyBar, 1,
		L"ViewerSettings", L"Shows the function-key bar in the viewer" },
	{OST_COMMON, NSecViewer, "ShowTitleBar", &Opt.ViOpt.ShowTitleBar, 1,
		L"ViewerSettings", L"Shows the title bar in the viewer" },
	{OST_COMMON, NSecViewer, "ShowArrows", &Opt.ViOpt.ShowArrows, 1,
		L"ViewerSettings", L"Shows navigation arrows in the viewer" },
	{OST_COMMON, NSecViewer, "ClickableURLs", &Opt.ViOpt.ClickableURLs, 1,
		L"ViewerSettings", L"Detects URLs in the viewer and allows opening them by click" },
	{OST_COMMON, NSecViewer, "ShowScrollbar", &Opt.ViOpt.ShowScrollbar, 0,
		L"ViewerSettings", L"Shows the scrollbar in the viewer" },
	{OST_COMMON, NSecViewer, "IsWrap", &Opt.ViOpt.ViewerIsWrap, 1,
		L"Viewer", L"Enables line wrapping mode in the viewer" },
	{OST_COMMON, NSecViewer, "Wrap", &Opt.ViOpt.ViewerWrap, 0,
		L"Viewer", L"Uses word wrapping in the viewer instead of simple line wrapping" },
	{OST_COMMON, NSecViewer, "PersistentBlocks", &Opt.ViOpt.PersistentBlocks, 0,
		L"ViewerSettings", L"Keeps viewer selections visible after cursor movement and commands" },
	{OST_COMMON, NSecViewer, "DefaultCodePage", &Opt.ViOpt.DefaultCodePage, CP_UTF8,
		L"ViewerSettings", L"The default code page used by the viewer when no better code page is detected" },
	{OST_COMMON, NSecViewer, "ShowMenuBar", &Opt.ViOpt.ShowMenuBar, 0,
		L"ViewerSettings", L"Shows the menu bar in the viewer" },

	{OST_COMMON, NSecDialog, "EditHistory", &Opt.Dialogs.EditHistory, 1,
		L"DialogSettings", L"Enables history in dialog edit controls" },
	{OST_COMMON, NSecDialog, "EditBlock", &Opt.Dialogs.EditBlock, 0,
		L"DialogSettings", L"Enables block selection behavior in dialog edit controls" },
	{OST_COMMON, NSecDialog, "AutoComplete", &Opt.Dialogs.AutoComplete, 1,
		L"DialogSettings", L"Enables autocomplete in dialog edit controls" },
	{OST_COMMON, NSecDialog, "EULBsClear", &Opt.Dialogs.EULBsClear, 0,
		nullptr, L"If is 1 in dialogs BS for UnChanged strings delete string as Del" },
	{OST_NONE,   NSecDialog, "SelectFromHistory", &Opt.Dialogs.SelectFromHistory, 0,
		nullptr, L"Selects values from history for dialog edit controls" },
	{OST_NONE,   NSecDialog, "EditLine", &Opt.Dialogs.EditLine, 0,
		nullptr, L"Enables extended edit-line behavior in dialogs" },
	{OST_COMMON, NSecDialog, "MouseButton", &Opt.Dialogs.MouseButton, 0xFFFF,
		L"DialogSettings", L"Bit mask of mouse buttons accepted by dialogs: 0x0001=left button, 0x0002=right button; any non-zero value is normalized to 0xFFFF by the dialog settings UI" },
	{OST_COMMON, NSecDialog, "DelRemovesBlocks", &Opt.Dialogs.DelRemovesBlocks, 1,
		L"DialogSettings", L"Allows Delete to remove selected text blocks in dialog edit controls" },
	{OST_NONE,   NSecDialog, "CBoxMaxHeight", &Opt.Dialogs.CBoxMaxHeight, 24,
		nullptr, L"Maximum height of combo-box dropdown lists in dialogs" },
	{OST_COMMON, NSecDialog, "ShowArrowsInEdit", &Opt.Dialogs.ShowArrowsInEdit, 1,
		L"DialogSettings", L"Shows scroll arrows in dialog edit controls when text is clipped" },

	{OST_COMMON, NSecEditor, "ExternalEditorName", &Opt.strExternalEditor, L"",
		L"EditorSettings", L"The command used to run an external editor" },
	{OST_COMMON, NSecEditor, "UseExternalEditor", &Opt.EdOpt.UseExternalEditor, 0,
		L"EditorSettings", L"Uses the configured external editor instead of the internal editor" },
	{OST_COMMON, NSecEditor, "ExpandTabs", &Opt.EdOpt.ExpandTabs, 0,
		L"EditorSettings", L"Inserts spaces instead of tab characters in the editor" },
	{OST_COMMON, NSecEditor, "TabSize", &Opt.EdOpt.TabSize, 8,
		L"EditorSettings", L"The number of columns used for a tab in the editor" },
	{OST_COMMON, NSecEditor, "PersistentBlocks", &Opt.EdOpt.PersistentBlocks, 0,
		L"EditorSettings", L"Keeps editor selections visible after cursor movement and commands" },
	{OST_COMMON, NSecEditor, "DelRemovesBlocks", &Opt.EdOpt.DelRemovesBlocks, 1,
		L"EditorSettings", L"Allows Delete to remove selected text blocks in the editor" },
	{OST_COMMON, NSecEditor, "AutoIndent", &Opt.EdOpt.AutoIndent, 0,
		L"EditorSettings", L"Automatically indents new lines in the editor" },
	{OST_COMMON, NSecEditor, "SaveEditorPos", &Opt.EdOpt.SavePos, 1,
		L"EditorSettings", L"Saves and restores the editor position for previously opened files" },
	{OST_COMMON, NSecEditor, "SaveEditorShortPos", &Opt.EdOpt.SaveShortPos, 1,
		L"EditorSettings", L"Saves a compact editor position history for faster restoration" },
	{OST_COMMON, NSecEditor, "AutoDetectCodePage", &Opt.EdOpt.AutoDetectCodePage, 0,
		L"EditorSettings", L"Automatically detects the code page when opening files in the editor" },
	{OST_COMMON, NSecEditor, "EditorCursorBeyondEOL", &Opt.EdOpt.CursorBeyondEOL, 1,
		L"EditorSettings", L"Allows the cursor to move beyond the end of a line in the editor" },
	{OST_COMMON, NSecEditor, "ReadOnlyLock", &Opt.EdOpt.ReadOnlyLock, 0, // Вернём назад дефолт 1.65 - не предупреждать и не блокировать
		L"EditorSettings", L"Locks read-only files against editing in the internal editor" },
	{OST_NONE,   NSecEditor, "EditorUndoSize", &Opt.EdOpt.UndoSize, 0, // $ 03.12.2001 IS размер буфера undo в редакторе
		nullptr, L"The undo buffer size for the editor" },
	{OST_NONE,   NSecEditor, "WordDiv", &Opt.strWordDiv, WordDiv0,
		nullptr, L"Characters treated as word separators by the editor" },
	{OST_NONE,   NSecEditor, "BSLikeDel", &Opt.EdOpt.BSLikeDel, 1,
		nullptr, L"Makes Backspace behave like Delete when a block is selected" },
	{OST_NONE,   NSecEditor, "FileSizeLimit", &Opt.EdOpt.FileSizeLimitLo, 0,
		nullptr, L"Low 32 bits of the maximum file size allowed for the internal editor (unlimit if both FileSizeLimitLo=0 and FileSizeLimitHi=0)" },
	{OST_NONE,   NSecEditor, "FileSizeLimitHi", &Opt.EdOpt.FileSizeLimitHi, 0,
		nullptr, L"High 32 bits of the maximum file size allowed for the internal editor (unlimit if both FileSizeLimitLo=0 and FileSizeLimitHi=0)" },
	{OST_NONE,   NSecEditor, "CharCodeBase", &Opt.EdOpt.CharCodeBase, 1,
		nullptr, L"The numeric base used when showing character codes in the editor status line: 0=octal, 1=decimal, 2=hexadecimal; other values are reduced modulo 3" },
	{OST_NONE,   NSecEditor, "AllowEmptySpaceAfterEof", &Opt.EdOpt.AllowEmptySpaceAfterEof, 0, //skv
		nullptr, L"Allows empty visual space after the end of file in the editor" },
	{OST_COMMON, NSecEditor, "DefaultCodePage", &Opt.EdOpt.DefaultCodePage, CP_UTF8,
		L"EditorSettings", L"The default code page used by the editor when no better code page is detected; use a numeric code page such as 65001 for UTF-8" },
	{OST_COMMON, NSecEditor, "ShowKeyBar", &Opt.EdOpt.ShowKeyBar, 1,
		L"EditorSettings", L"Shows the function-key bar in the editor" },
	{OST_COMMON, NSecEditor, "ShowTitleBar", &Opt.EdOpt.ShowTitleBar, 1,
		L"EditorSettings", L"Shows the title bar in the editor" },
	{OST_COMMON, NSecEditor, "ShowMenuBar", &Opt.EdOpt.ShowMenuBar, 0,
		L"EditorSettings", L"Shows the menu bar in the editor" },
	{OST_COMMON, NSecEditor, "ShowScrollBar", &Opt.EdOpt.ShowScrollBar, 0,
		L"EditorSettings", L"Shows the scrollbar in the editor" },
	{OST_COMMON, NSecEditor, "UseEditorConfigOrg", &Opt.EdOpt.UseEditorConfigOrg, 1,
		L"EditorSettings", L"Reads applicable .editorconfig files and applies supported editor settings" },
	{OST_COMMON, NSecEditor, "SearchSelFound", &Opt.EdOpt.SearchSelFound, 0,
		L"EditorSearch", L"Selects found text after editor searches" },
	{OST_COMMON, NSecEditor, "SearchRegexp", &Opt.EdOpt.SearchRegexp, 0,
		L"EditorSearch", L"Uses regular expressions by default in editor searches" },
	{OST_COMMON, NSecEditor, "SearchPickUpWord", &Opt.EdOpt.SearchPickUpWord, 0,
		L"EditorSettings", L"Uses the word under the cursor as the initial editor search text" },
	{OST_COMMON, NSecEditor, "WordWrap", &Opt.EdOpt.WordWrap, 0,
		L"EditorSettings", L"Enables word wrapping in the editor" },
	{OST_COMMON, NSecEditor, "ShowWhiteSpace", &Opt.EdOpt.ShowWhiteSpace, 0,
		L"EditorSettings", L"Shows whitespace markers in the editor" },
	{OST_COMMON, NSecEditor, "ShowLineNumbers", &Opt.EdOpt.ShowLineNumbers, 0,
		L"EditorSettings", L"Shows line numbers in the editor" },

	{OST_COMMON, NSecNotifications, "OnFileOperation", &Opt.NotifOpt.OnFileOperation, 1,
		L"NotificationsSettings", L"Shows desktop notifications for completed file operations" },
	{OST_COMMON, NSecNotifications, "OnConsole", &Opt.NotifOpt.OnConsole, 1,
		L"NotificationsSettings", L"Shows desktop notifications for console events" },
	{OST_COMMON, NSecNotifications, "OnlyIfBackground", &Opt.NotifOpt.OnlyIfBackground, 1,
		L"NotificationsSettings", L"Shows notifications only when FAR2L is in the background" },

	{OST_NONE,   NSecXLat, "Flags", &Opt.XLat.Flags, (DWORD)XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE,
		L"InputSettings", L"Bit mask controlling keyboard-layout switching and text transliteration behavior: 0x00000001=switch keyboard layout, 0x00000002=beep on switch, 0x00000004=use keyboard layout name, 0x00010000=convert the whole command line" },
	{OST_COMMON, NSecXLat, "EnableForFastFileFind", &Opt.XLat.EnableForFastFileFind, 1,
		L"InputSettings", L"Enables transliteration support in fast file find" },
	{OST_COMMON, NSecXLat, "EnableForDialogs", &Opt.XLat.EnableForDialogs, 1,
		L"InputSettings", L"Enables transliteration support in dialog edit controls" },
	{OST_COMMON, NSecXLat, "WordDivForXlat", &Opt.XLat.strWordDivForXlat, WordDivForXlat0,
		L"InputSettings", L"Characters treated as word separators by transliteration" },
	{OST_COMMON, NSecXLat, "XLat", &Opt.XLat.XLat, L"ru:qwerty-йцукен",
		L"InputSettings", L"Keyboard layout transliteration mapping definitions in layout:source-target form, for example ru:<latin-keys>-<target-keys>" },

	{OST_COMMON, NSecSavedHistory, NParamHistoryCount, &Opt.HistoryCount, 512,
		L"SystemSettings", L"Maximum number of command history entries to keep" },
	{OST_COMMON, NSecSavedFolderHistory, NParamHistoryCount, &Opt.FoldersHistoryCount, 512,
		L"SystemSettings", L"Maximum number of folder history entries to keep" },
	{OST_COMMON, NSecSavedViewHistory, NParamHistoryCount, &Opt.ViewHistoryCount, 512,
		L"SystemSettings", L"Maximum number of viewer and editor history entries to keep" },
	{OST_COMMON, NSecSavedDialogHistory, NParamHistoryCount, &Opt.DialogsHistoryCount, 512,
		L"SystemSettings", L"Maximum number of dialog history entries to keep" },

	{OST_COMMON, NSecSystem, "PersonalPluginsPath", &Opt.LoadPlug.strPersonalPluginsPath, L"",
		L"Plugins", L"Additional user plugin directory searched when loading plugins" },
	{OST_COMMON, NSecSystem, "SaveHistory", &Opt.SaveHistory, 1,
		L"SystemSettings", L"Saves command history between sessions" },
	{OST_COMMON, NSecSystem, "SaveFoldersHistory", &Opt.SaveFoldersHistory, 1,
		L"SystemSettings", L"Saves folder history between sessions" },
	{OST_NONE,   NSecSystem, "SavePluginFoldersHistory", &Opt.SavePluginFoldersHistory, 0,
		nullptr, L"Saves plugin folder history between sessions" },
	{OST_COMMON, NSecSystem, "SaveViewHistory", &Opt.SaveViewHistory, 1,
		L"SystemSettings", L"Saves viewer and editor history between sessions" },
	{OST_COMMON, NSecSystem, "HistoryRemoveDupsRule", &Opt.HistoryRemoveDupsRule, HISTORY_REMOVE_DUPS_BY_NAME_EXTRA,
		L"SystemSettings", L"Controls how duplicate history entries are removed: 0=never remove duplicates, 1=remove by name, 2=remove by name and path" },
	{OST_COMMON, NSecSystem, "AutoHighlightHistory", &Opt.AutoHighlightHistory, 1,
		L"SystemSettings", L"Highlights matching entries automatically in history lists" },
	{OST_COMMON, NSecSystem, NParamAutoSaveSetup, &Opt.AutoSaveSetup, 0,
		L"SystemSettings", L"Automatically saves all (common and panels) setup options" },
	{OST_COMMON, NSecSystem, NParamAutoSavePanels, &Opt.AutoSavePanels, 0,
		L"SystemSettings", L"Automatically saves panel state" },
	{OST_COMMON, NSecSystem, "DeleteToRecycleBin", &Opt.DeleteToRecycleBin, 0,
		L"SystemSettings", L"Moves deleted files to the recycle bin or trash when possible" },
	{OST_COMMON, NSecSystem, "DeleteToRecycleBinKillLink", &Opt.DeleteToRecycleBinKillLink, 1,
		L"SystemSettings", L"Deletes symbolic links themselves instead of moving link targets to the recycle bin" },
	{OST_NONE,   NSecSystem, "WipeSymbol", &Opt.WipeSymbol, 0,
		L"DeleteFile", L"Byte value used to overwrite file contents during wipe operations; 0 uses NUL bytes, otherwise the low byte of the integer is used" },
	{OST_COMMON, NSecSystem, "SudoEnabled", &Opt.SudoEnabled, 1,
		L"SystemSettings", L"Enables elevated operations through sudo where supported" },
	{OST_COMMON, NSecSystem, "SudoConfirmModify", &Opt.SudoConfirmModify, 1,
		L"SystemSettings", L"Asks for confirmation before using elevated privileges for modifying operations" },
	{OST_COMMON, NSecSystem, "SudoPasswordExpiration", &Opt.SudoPasswordExpiration, 15 * 60,
		L"SystemSettings", L"How long, in seconds, cached sudo credentials are considered valid" },

	{OST_COMMON, NSecSystem, "UseCOW", &Opt.CMOpt.UseCOW, 0,
		L"CopyFiles", L"Uses copy-on-write cloning for copy operations when the filesystem supports it" },
	{OST_COMMON, NSecSystem, "SparseFiles", &Opt.CMOpt.SparseFiles, 0,
		L"CopyFiles", L"Creates sparse destination files when copying sparse sources" },
	{OST_COMMON, NSecSystem, "HowCopySymlink", &Opt.CMOpt.HowCopySymlink, 1,
		L"CopyFiles", L"Controls how symlinks are copied: 0=always copy the link itself, 1=smartly copy the link or target, 2=always copy the target file contents" },
	{OST_COMMON, NSecSystem, "WriteThrough", &Opt.CMOpt.WriteThrough, 0,
		L"CopyFiles", L"Uses write-through mode for copy operations" },
	{OST_COMMON, NSecSystem, "CopyXAttr", &Opt.CMOpt.CopyXAttr, 0,
		L"CopyFiles", L"Copies extended file attributes when supported" },
	{OST_NONE,   NSecSystem, "CopyAccessMode", &Opt.CMOpt.CopyAccessMode, 1,
		L"CopyFiles", L"Copies file access mode and permission bits when supported" },
	{OST_COMMON, NSecSystem, "MultiCopy", &Opt.CMOpt.MultiCopy, 0,
		L"CopyFiles", L"Allows multiple copy or move targets in one operation" },
	{OST_COMMON, NSecSystem, "CopyTimeRule", &Opt.CMOpt.CopyTimeRule, 3,
		L"InterfSettings", L"Bit mask controlling whether copy progress shows elapsed time, remaining time, and average speed: 0=disabled, 0x0002=normal file copies; the settings dialog writes 3 when enabled for compatibility" },

	{OST_COMMON, NSecSystem, "MakeLinkSuggestSymlinkAlways", &Opt.MakeLinkSuggestSymlinkAlways, 1,
		L"HardSymLink", L"Controls the default make-link suggestion: 0=suggest by source and destination type, 1=always suggest a symbolic link" },

	{OST_COMMON, NSecSystem, "InactivityExit", &Opt.InactivityExit, 0,
		L"SystemSettings", L"Exits FAR2L after a period of inactivity" },
	{OST_COMMON, NSecSystem, "InactivityExitTime", &Opt.InactivityExitTime, 15,
		L"SystemSettings", L"The inactivity timeout, in minutes, before FAR2L exits" },
	{OST_COMMON, NSecSystem, "DriveMenuMode2", &Opt.ChangeDriveMode, -1,
		L"DriveDlg", L"Bit mask controlling which entries and columns are shown in the drive or location menu: 0x0001=disk type, 0x0002=network/SUBST/VHD name, 0x0004=label, 0x0008=file system, 0x0010=size, 0x0020=mountpoints, 0x0040=plugins, 0x0080=bookmarks, 0x0100=Explorer-style size, 0x0200=network drives" },
	{OST_COMMON, NSecSystem, "DriveDisconnectMode", &Opt.ChangeDriveDisconnectMode, 1,
		L"DisconnectDrive", L"Reserved legacy setting for drive-menu disconnect handling; currently saved but not used by the disconnect code" },

	{OST_COMMON, NSecSystem, "DriveExceptions", &Opt.ChangeDriveExceptions,
		L"/System*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron;/tmp/.*",
		L"ChangeLocationConfig", L"Semicolon-separated path masks excluded from the drive or location menu" },
	{OST_COMMON, NSecSystem, "DriveColumn2", &Opt.ChangeDriveColumn2, L"$U$</$>$T",
		L"ChangeLocationConfig", L"Format string for the second custom column in the drive or location menu" },
	{OST_COMMON, NSecSystem, "DriveColumn3", &Opt.ChangeDriveColumn3, L"$S$D",
		L"ChangeLocationConfig", L"Format string for the third custom column in the drive or location menu" },

	{OST_COMMON, NSecSystem, "AutoUpdateRemoteDrive", &Opt.AutoUpdateRemoteDrive, 1,
		L"PanelSettings", L"Automatically updates panels located on remote drives" },
	{OST_COMMON, NSecSystem, "FileSearchMode", &Opt.FindOpt.FileSearchMode, FINDAREA_FROM_CURRENT,
		L"FindFile", L"Default search area used by the Find File command: 0=all disks, 1=all but network drives, 2=PATH folders, 3=drive root, 4=from current folder, 5=current folder only, 6=selected items" },
	{OST_NONE,   NSecSystem, "CollectFiles", &Opt.FindOpt.CollectFiles, 1,
		L"FindFileResult", L"Collects found files into the search results list" },
	{OST_COMMON, NSecSystem, "SearchInFirstSize", &Opt.FindOpt.strSearchInFirstSize, L"",
		L"FindFileAdvanced", L"Limits text search to the first part of each file, using a size string" },
	{OST_COMMON, NSecSystem, "FindAlternateStreams", &Opt.FindOpt.FindAlternateStreams, 0,
		L"FindFileAdvanced", L"Searches alternate file streams when supported" },
	{OST_COMMON, NSecSystem, "SearchOutFormat", &Opt.FindOpt.strSearchOutFormat, L"D,S,A",
		L"FindFileAdvanced", L"Comma-separated column type list used for Find File search results, using the same column codes as file panel view modes" },
	{OST_COMMON, NSecSystem, "SearchOutFormatWidth", &Opt.FindOpt.strSearchOutFormatWidth, L"14,13,0",
		L"FindFileAdvanced", L"Comma-separated column widths used for Find File search results; each item corresponds to System.SearchOutFormat" },
	{OST_COMMON, NSecSystem, "FindFolders", &Opt.FindOpt.FindFolders, 1,
		L"FindFile", L"Includes folders in Find File results" },
	{OST_COMMON, NSecSystem, "FindSymLinks", &Opt.FindOpt.FindSymLinks, 1,
		L"FindFile", L"Includes symbolic links in Find File results" },
	{OST_COMMON, NSecSystem, "FindCaseSensitiveFileMask", &Opt.FindOpt.FindCaseSensitiveFileMask, 1,
		L"FindFile", L"Uses case-sensitive matching for Find File masks" },
	{OST_COMMON, NSecSystem, "UseFilterInSearch", &Opt.FindOpt.UseFilter, 0,
		L"FindFile", L"Applies the current file filter during Find File operations" },
	{OST_COMMON, NSecSystem, "FindCodePage", &Opt.FindCodePage, CP_AUTODETECT,
		L"FindFile", L"Default code page used for Find File text searches; 0xffffffff enables automatic detection, otherwise use a numeric code page such as 65001 for UTF-8" },
	{OST_NONE,   NSecSystem, "CmdHistoryRule", &Opt.CmdHistoryRule, 0,
		L"AutoCompleteSettings", L"Controls which executed commands are recorded in command history; 0 uses the default command-history behavior" },
	{OST_NONE,   NSecSystem, "SetAttrFolderRules", &Opt.SetAttrFolderRules, 1,
		L"FileAttrDlg", L"Controls how Set Attributes treats folders: 0=legacy folder behavior with subfolders selected by default, 1=treat the folder itself as an item and leave subfolders unchecked by default" },
	{OST_NONE,   NSecSystem, "MaxPositionCache", &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS,
		nullptr, L"Maximum number of file positions stored in the viewer and editor position cache" },
	{OST_NONE,   NSecSystem, "ConsoleDetachKey", &strKeyNameConsoleDetachKey, L"CtrlAltTab",
		L"Terminal", L"Key name used to detach or switch console focus" },
	{OST_NONE,   NSecSystem, "SilentLoadPlugin", &Opt.LoadPlug.SilentLoadPlugin, 0,
		L"Plugins", L"Suppresses non-critical messages while loading plugins" },
	{OST_COMMON, NSecSystem, "ScanSymlinks", &Opt.LoadPlug.ScanSymlinks, 1,
		L"Plugins", L"Scans symbolic links while discovering plugins" },
	{OST_COMMON, NSecSystem, "MultiMakeDir", &Opt.MultiMakeDir, 0,
		L"MakeFolder", L"Allows creating several directories from one Make Folder command" },
	{OST_NONE,   NSecSystem, "MsWheelDelta", &Opt.MsWheelDelta, 1,
		L"MsWheel", L"Number of rows scrolled by one vertical mouse-wheel step in panels and trees" },
	{OST_NONE,   NSecSystem, "MsWheelDeltaView", &Opt.MsWheelDeltaView, 1,
		L"MsWheel", L"Number of rows scrolled by one vertical mouse-wheel step in the viewer" },
	{OST_NONE,   NSecSystem, "MsWheelDeltaEdit", &Opt.MsWheelDeltaEdit, 1,
		L"MsWheel", L"Number of rows scrolled by one vertical mouse-wheel step in the editor" },
	{OST_NONE,   NSecSystem, "MsWheelDeltaHelp", &Opt.MsWheelDeltaHelp, 1,
		L"MsWheel", L"Number of rows scrolled by one vertical mouse-wheel step in help" },
	{OST_NONE,   NSecSystem, "MsHWheelDelta", &Opt.MsHWheelDelta, 1,
		L"MsWheel", L"Number of columns scrolled by one horizontal mouse-wheel step in panels and trees" },
	{OST_NONE,   NSecSystem, "MsHWheelDeltaView", &Opt.MsHWheelDeltaView, 1,
		L"MsWheel", L"Number of columns scrolled by one horizontal mouse-wheel step in the viewer" },
	{OST_NONE,   NSecSystem, "MsHWheelDeltaEdit", &Opt.MsHWheelDeltaEdit, 1,
		L"MsWheel", L"Number of columns scrolled by one horizontal mouse-wheel step in the editor" },
	{OST_NONE,   NSecSystem, "SubstNameRule", &Opt.SubstNameRule, 2,
		nullptr, L"Bit mask controlling substituted drive name lookup: bit 0=query removable drives, bit 1=query all other drives" },
	{OST_NONE,   NSecSystem, "ShowCheckingFile", &Opt.ShowCheckingFile, 0,
		nullptr, L"Shows the name of the file currently being checked during file operations" },
	{OST_NONE,   NSecSystem, "QuotedSymbols", &Opt.strQuotedSymbols, L" $&()[]{};|*?!'`\"\\\xA0", //xA0 => 160 =>oem(0xFF)
		nullptr, L"Characters that cause file names to be quoted or escaped" },
	{OST_NONE,   NSecSystem, "QuotedName", &Opt.QuotedName, QUOTEDNAME_INSERT,
		nullptr, L"Bit mask controlling filename quoting: 0x0001=quote when inserting into the command line, dialogs, or editor; 0x0002=quote when copying to the clipboard" },
	//{OST_NONE,   NSecSystem, "CPAJHefuayor", &Opt.strCPAJHefuayor, 0 },
	{OST_NONE,   NSecSystem, "PluginMaxReadData", &Opt.PluginMaxReadData, 0x40000,
		L"Plugins", L"Maximum block size read from plugins at once" },
	{OST_NONE,   NSecSystem, "UseNumPad", &Opt.UseNumPad, 1,
		L"InputSettings", L"Enables numpad-specific key handling" },
	{OST_NONE,   NSecSystem, "CASRule", &Opt.CASRule, -1,
		nullptr, L"Bit mask controlling how Ctrl+Alt+Shift key combinations are distinguished: bit 0=left Ctrl+Alt+Shift, bit 1=right Ctrl+Alt+Shift" },
	{OST_NONE,   NSecSystem, "AllCtrlAltShiftRule", &Opt.AllCtrlAltShiftRule, 0x0000FFFF,
		nullptr, L"Bit mask of areas where Ctrl+Alt+Shift is treated as a special modifier state: 0x01=panels, 0x02=editor, 0x04=viewer, 0x08=help, 0x10=dialogs" },
	{OST_COMMON, NSecSystem, "ScanJunction", &Opt.ScanJunction, 1,
		L"SystemSettings", L"Scans junctions and reparse-point directories when walking folders" },
	{OST_COMMON, NSecSystem, "OnlyFilesSize", &Opt.OnlyFilesSize, 0,
		L"SystemSettings", L"Counts only file sizes when calculating selected or folder sizes" },
	{OST_NONE,   NSecSystem, "UsePrintManager", &Opt.UsePrintManager, 1,
		nullptr, L"Enables the legacy print manager integration where available" },

	{OST_COMMON, NSecSystem, "ExcludeCmdHistory", &Opt.ExcludeCmdHistory, 0, //AN
		L"AutoCompleteSettings", L"Bit mask of command sources excluded from command history: 0x01=system associations, 0x02=far2l associations, 0x04=panel execution, 0x08=command-line execution" },

	{OST_COMMON, NSecSystem, "FolderInfo", &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me",
		L"InfoPanel", L"Comma-separated file masks used by the information panel as folder description files" },

	{OST_COMMON, NSecSystem, "OwnerGroupShowId", &Opt.OwnerGroupShowId, 0,
		L"FileAttrDlg", L"Shows numeric owner and group IDs with owner and group names" },

	{OST_NONE,   NSecSystemNowell, "MoveRO", &Opt.Nowell.MoveRO, 1,
		nullptr, L"Allows moving read-only files on Novell-style filesystems" },

	{OST_NONE,   NSecSystemExecutor, "RestoreCP", &Opt.RestoreCPAfterExecute, 1,
		L"OSCommands", L"Restores the code page after executing an external command" },
	{OST_NONE,   NSecSystemExecutor, "UseAppPath", &Opt.ExecuteUseAppPath, 1,
		L"OSCommands", L"Searches application paths when executing external commands" },
	{OST_NONE,   NSecSystemExecutor, "ShowErrorMessage", &Opt.ExecuteShowErrorMessage, 1,
		L"OSCommands", L"Shows an error message when an external command cannot be executed" },
	{OST_NONE,   NSecSystemExecutor, "FullTitle", &Opt.ExecuteFullTitle, 0,
		L"OSCommands", L"Uses the full command title while executing external commands" },
	{OST_NONE,   NSecSystemExecutor, "SilentExternal", &Opt.ExecuteSilentExternal, 0,
		L"OSCommands", L"Suppresses extra UI while executing external commands" },

	{OST_NONE,   NSecPanelTree, "MinTreeCount", &Opt.Tree.MinTreeCount, 4,
		L"TreePanel", L"Minimum number of folders required before saving a folder tree cache" },
	{OST_NONE,   NSecPanelTree, "TreeFileAttr", &Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN,
		L"TreePanel", L"File attributes applied to saved folder tree cache files" },
	{OST_NONE,   NSecPanelTree, "LocalDisk", &Opt.Tree.LocalDisk, 2,
		L"TreePanel", L"Legacy setting for storing folder tree cache files for local disks; current cache decision code does not read this value" },
	{OST_NONE,   NSecPanelTree, "NetDisk", &Opt.Tree.NetDisk, 2,
		L"TreePanel", L"Legacy setting for storing folder tree cache files for network disks; current cache decision code does not read this value" },
	{OST_NONE,   NSecPanelTree, "RemovableDisk", &Opt.Tree.RemovableDisk, 2,
		L"TreePanel", L"Legacy setting for storing folder tree cache files for removable disks; current cache decision code does not read this value" },
	{OST_NONE,   NSecPanelTree, "NetPath", &Opt.Tree.NetPath, 2,
		L"TreePanel", L"Legacy setting for storing folder tree cache files for network paths; current cache decision code does not read this value" },
	{OST_COMMON, NSecPanelTree, "AutoChangeFolder", &Opt.Tree.AutoChangeFolder, 0, // ???
		L"PanelSettings", L"Automatically changes the active folder while moving through the tree panel" },
	{OST_COMMON, NSecPanelTree, "ExclSubTreeMask", &Opt.Tree.ExclSubTreeMask, L".*",
		L"PanelSettings", L"Folder masks excluded when scanning tree panels" },
	{OST_COMMON, NSecPanelTree, "ScanDepthEnabled", &Opt.Tree.ScanDepthEnabled, 1,
		L"PanelSettings", L"Limits tree panel scanning to a default depth" },
	{OST_COMMON, NSecPanelTree, "DefaultScanDepth", &Opt.Tree.DefaultScanDepth, 4,
		L"PanelSettings", L"Default maximum depth used when scanning tree panels" },

	{OST_COMMON, NSecLanguage, "Help", &Opt.strHelpLanguage, L"English",
		L"OptMenu", L"Language used for help files" },
	{OST_COMMON, NSecLanguage, "Main", &Opt.strLanguage, L"English",
		L"OptMenu", L"Language used for the main interface" },

	{OST_COMMON, NSecConfirmations, "Copy", &Opt.Confirm.Copy, 1,
		L"ConfirmDlg", L"Asks for confirmation before copying files" },
	{OST_COMMON, NSecConfirmations, "Move", &Opt.Confirm.Move, 1,
		L"ConfirmDlg", L"Asks for confirmation before moving or renaming files" },
	{OST_COMMON, NSecConfirmations, "RO", &Opt.Confirm.RO, 1,
		L"ConfirmDlg", L"Asks for confirmation before modifying read-only files" },
	{OST_COMMON, NSecConfirmations, "Drag", &Opt.Confirm.Drag, 1,
		L"ConfirmDlg", L"Asks for confirmation before drag-and-drop file operations" },
	{OST_COMMON, NSecConfirmations, "Delete", &Opt.Confirm.Delete, 1,
		L"ConfirmDlg", L"Asks for confirmation before deleting files" },
	{OST_COMMON, NSecConfirmations, "DeleteFolder", &Opt.Confirm.DeleteFolder, 1,
		L"ConfirmDlg", L"Asks for confirmation before deleting folders" },
	{OST_COMMON, NSecConfirmations, "Esc", &Opt.Confirm.Esc, 1,
		L"ConfirmDlg", L"Asks for confirmation before interrupting an operation with Esc" },
	{OST_COMMON, NSecConfirmations, "RemoveConnection", &Opt.Confirm.RemoveConnection, 1,
		L"ConfirmDlg", L"Asks for confirmation before removing a connection" },
	{OST_COMMON, NSecConfirmations, "ClearVT", &Opt.Confirm.ClearVT, 1,
		L"ConfirmDlg", L"Asks for confirmation before clearing the terminal output" },
	{OST_COMMON, NSecConfirmations, "RemoveHotPlug", &Opt.Confirm.RemoveHotPlug, 1,
		L"ConfirmDlg", L"Asks for confirmation before removing a hot-plug device" },
	{OST_COMMON, NSecConfirmations, "AllowReedit", &Opt.Confirm.AllowReedit, 1,
		L"ConfirmDlg", L"Asks for confirmation before removing a hot-plug device" },
	{OST_COMMON, NSecConfirmations, "HistoryClear", &Opt.Confirm.HistoryClear, 1,
		L"ConfirmDlg", L"Asks before reopening an already edited file" },
	{OST_COMMON, NSecConfirmations, "Exit", &Opt.Confirm.Exit, 1,
		L"ConfirmDlg", L"Asks for confirmation before clearing history" },
	{OST_COMMON, NSecConfirmations, "ExitOrBknd", &Opt.Confirm.ExitOrBknd, 1,
		L"ConfirmDlg", L"Asks for confirmation before exiting FAR2L" },
	{OST_NONE,   NSecConfirmations, "EscTwiceToInterrupt", &Opt.Confirm.EscTwiceToInterrupt, 0,
		nullptr, L"Asks for confirmation before exiting or closing the backend session" },

	{OST_COMMON, NSecPluginConfirmations,  "OpenFilePlugin", &Opt.PluginConfirm.OpenFilePlugin, 0,
		L"ChoosePluginMenu", L"Asks before opening a file with a plugin" },
	{OST_COMMON, NSecPluginConfirmations,  "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0,
		L"ChoosePluginMenu", L"Asks before using a standard file association" },
	{OST_COMMON, NSecPluginConfirmations,  "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0,
		L"ChoosePluginMenu", L"Shows plugin selection confirmation even when only one plugin is available" },
	{OST_COMMON, NSecPluginConfirmations,  "SetFindList", &Opt.PluginConfirm.SetFindList, 0,
		L"ChoosePluginMenu", L"Asks before sending Find File results to a plugin panel" },
	{OST_COMMON, NSecPluginConfirmations,  "Prefix", &Opt.PluginConfirm.Prefix, 0,
		L"ChoosePluginMenu", L"Asks before executing a plugin prefix command" },

	{OST_NONE,   NSecPanel, "ShellRightLeftArrowsRule", &Opt.ShellRightLeftArrowsRule, 0,
		L"PanelCmd", L"Controls left/right arrow behavior in a single-column panel when the command line is not empty: 0=let the command line handle the key, 1=move in the panel" },
	{OST_COMMON, NSecPanel, "ShowHidden", &Opt.ShowHidden, 1,
		L"PanelSettings", L"Shows hidden and system files in panels" },
	{OST_COMMON, NSecPanel, "Highlight", &Opt.Highlight, 1,
		L"PanelSettings", L"Enables file highlighting rules in panels" },
	{OST_COMMON, NSecPanel, "SortFolderExt", &Opt.SortFolderExt, 0,
		L"PanelSettings", L"Uses folder extensions when sorting folders" },
	{OST_COMMON, NSecPanel, "SelectFolders", &Opt.SelectFolders, 0,
		L"PanelSettings", L"Allows folders to be selected together with files" },
	{OST_COMMON, NSecPanel, "AttrStrStyle", &Opt.AttrStrStyle, 1,
		L"PanelViewModes", L"Controls the display style of file attribute strings; bit 0 toggles the compact/alternate attribute marker style used in filters and highlighting" },
	{OST_COMMON, NSecPanel, "CaseSensitiveCompareSelect", &Opt.PanelCaseSensitiveCompareSelect, 1,
		L"PanelSettings", L"Uses case-sensitive comparisons for compare and select operations" },
	{OST_COMMON, NSecPanel, "ReverseSort", &Opt.ReverseSort, 1,
		L"PanelSettings", L"Enables reverse sort order handling in panels" },
	{OST_NONE,   NSecPanel, "RightClickRule", &Opt.PanelRightClickRule, 2,
		nullptr, L"Controls how right click in an empty panel column is interpreted: 0=always empty area, 1=column past item count is empty, 2=right-click past item count keeps current item and treats the area as empty" },
	{OST_NONE,   NSecPanel, "CtrlAltShiftRule", &Opt.PanelCtrlAltShiftRule, 0,
		nullptr, L"Controls what remains visible while holding Ctrl+Alt+Shift in panels: 0=show command line and key bar, 1=show key bar only, 2=hide both; values are reduced modulo 3 on load" },
	{OST_NONE,   NSecPanel, "RememberLogicalDrives", &Opt.RememberLogicalDrives, 0,
		nullptr, L"Remembers logical drives in panel state" },
	{OST_COMMON, NSecPanel, "AutoUpdateLimit", &Opt.AutoUpdateLimit, 0,
		L"PanelSettings", L"Maximum number of file changes processed by automatic panel update" },
	{OST_COMMON, NSecPanel, "ShowFilenameMarks", &Opt.ShowFilenameMarks, 1,
		L"PanelSettings", L"Shows file name marking indicators from highlighting rules" },
	{OST_COMMON, NSecPanel, "FilenameMarksAlign", &Opt.FilenameMarksAlign, 1,
		L"PanelSettings", L"Aligns file name marking indicators in panel columns" },
	{OST_COMMON, NSecPanel, "FilenameMarksInStatusBar", &Opt.FilenameMarksAlign, 1,
		L"PanelSettings", L"Shows file name marking indicators in the panel status bar" },
	{OST_COMMON, NSecPanel, "MinFilenameIndentation", &Opt.MinFilenameIndentation, 0,
		L"PanelSettings", L"Minimum indentation reserved for file name marking indicators" },
	{OST_COMMON, NSecPanel, "MaxFilenameIndentation", &Opt.MaxFilenameIndentation, HIGHLIGHT_MAX_MARK_LENGTH,
		L"PanelSettings", L"Maximum indentation reserved for file name marking indicators" },
	{OST_COMMON, NSecPanel, "DirNameStyle", &Opt.DirNameStyle, 0,
		L"PanelSettings", L"Bit mask controlling directory name styling: bits 0-1=label style, bits 2-3=surrounding character style, 0x10=show surrounding characters, 0x20=center the label" },
	{OST_COMMON, NSecPanel, "DirNameStyleColumnWidthAlways", &Opt.DirNameStyleColumnWidthAlways, 0,
		L"PanelSettings", L"Controls directory-name style column width: 0=reserve extra width only when needed, 1=always reserve it" },
	{OST_COMMON, NSecPanel, "ShowSymlinkSize", &Opt.ShowSymlinkSize, 0,
		L"PanelSettings", L"Shows the size of symbolic links in panels" },
	{OST_COMMON, NSecPanel, "ClassicHotkeyLinkResolving", &Opt.ClassicHotkeyLinkResolving, 1,
		L"PanelSettings", L"Uses classic link resolving behavior for panel hotkeys" },

	{OST_PANELS, NSecPanelLeft, "Type", &Opt.LeftPanel.Type, 0,
		L"Panels", L"The saved panel type for the left panel: 0=file panel, 1=tree panel, 2=quick view panel, 3=information panel" },
	{OST_PANELS, NSecPanelLeft, "Visible", &Opt.LeftPanel.Visible, 1,
		L"Panels", L"Whether the left panel is visible when panel state is restored" },
	{OST_PANELS, NSecPanelLeft, "Focus", &Opt.LeftPanel.Focus, 1,
		L"Panels", L"Whether the left panel has focus when panel state is restored" },
	{OST_PANELS, NSecPanelLeft, "ViewMode", &Opt.LeftPanel.ViewMode, 2,
		L"PanelViewModes", L"The saved view mode for the left panel: 0-9 correspond to panel view modes 0 through 9" },
	{OST_PANELS, NSecPanelLeft, "SortMode", &Opt.LeftPanel.SortMode, 1,
		L"PanelCmdSort", L"The saved sort mode for the left panel: 0=unsorted, 1=name, 2=extension, 3=modification time, 4=creation time, 5=access time, 6=size, 7=description, 8=owner, 9=physical size, 10=number of links, 11=full name, 12=change time, 13=custom data" },
	{OST_PANELS, NSecPanelLeft, "SortOrder", &Opt.LeftPanel.SortOrder, 1,
		L"PanelCmdSort", L"The saved sort direction for the left panel: 0=descending/reverse, 1=ascending/normal" },
	{OST_PANELS, NSecPanelLeft, "SortGroups", &Opt.LeftPanel.SortGroups, 0,
		L"PanelCmdSort", L"Whether sort groups are enabled for the left panel" },
	{OST_PANELS, NSecPanelLeft, "NumericSort", &Opt.LeftPanel.NumericSort, 0,
		L"PanelCmdSort", L"Whether numeric sorting is enabled for the left panel" },
	{OST_PANELS, NSecPanelLeft, "CaseSensitiveSort", &Opt.LeftPanel.CaseSensitiveSort, 0,
		L"PanelCmdSort", L"Whether case-sensitive sorting is enabled for the left panel" },
	{OST_PANELS, NSecPanelLeft, "Folder", &Opt.strLeftFolder, L"",
		L"FilePanel", L"The saved folder path for the left panel" },
	{OST_PANELS, NSecPanelLeft, "CurFile", &Opt.strLeftCurFile, L"",
		L"FilePanel", L"The saved current file name for the left panel" },
	{OST_PANELS, NSecPanelLeft, "SelectedFirst", &Opt.LeftSelectedFirst, 0,
		L"PanelCmdSort", L"Whether selected items are shown first in the left panel" },
	{OST_PANELS, NSecPanelLeft, "DirectoriesFirst", &Opt.LeftPanel.DirectoriesFirst, 1,
		L"PanelCmdSort", L"Whether directories are shown before files in the left panel" },
	{OST_PANELS, NSecPanelLeft, "ExecutablesFirst", &Opt.LeftPanel.ExecutablesFirst, 0,
		L"PanelCmdSort", L"Whether executable files are shown before other files in the left panel" },

	{OST_PANELS, NSecPanelRight, "Type", &Opt.RightPanel.Type, 0,
		L"Panels", L"The saved panel type for the right panel: 0=file panel, 1=tree panel, 2=quick view panel, 3=information panel" },
	{OST_PANELS, NSecPanelRight, "Visible", &Opt.RightPanel.Visible, 1,
		L"Panels", L"Whether the right panel is visible when panel state is restored" },
	{OST_PANELS, NSecPanelRight, "Focus", &Opt.RightPanel.Focus, 0,
		L"Panels", L"Whether the right panel has focus when panel state is restored" },
	{OST_PANELS, NSecPanelRight, "ViewMode", &Opt.RightPanel.ViewMode, 2,
		L"PanelViewModes", L"The saved view mode for the right panel: 0-9 correspond to panel view modes 0 through 9" },
	{OST_PANELS, NSecPanelRight, "SortMode", &Opt.RightPanel.SortMode, 1,
		L"PanelCmdSort", L"The saved sort mode for the right panel: 0=unsorted, 1=name, 2=extension, 3=modification time, 4=creation time, 5=access time, 6=size, 7=description, 8=owner, 9=physical size, 10=number of links, 11=full name, 12=change time, 13=custom data" },
	{OST_PANELS, NSecPanelRight, "SortOrder", &Opt.RightPanel.SortOrder, 1,
		L"PanelCmdSort", L"The saved sort direction for the right panel: 0=descending/reverse, 1=ascending/normal" },
	{OST_PANELS, NSecPanelRight, "SortGroups", &Opt.RightPanel.SortGroups, 0,
		L"PanelCmdSort", L"Whether sort groups are enabled for the right panel" },
	{OST_PANELS, NSecPanelRight, "NumericSort", &Opt.RightPanel.NumericSort, 0,
		L"PanelCmdSort", L"Whether numeric sorting is enabled for the right panel" },
	{OST_PANELS, NSecPanelRight, "CaseSensitiveSort", &Opt.RightPanel.CaseSensitiveSort, 0,
		L"PanelCmdSort", L"Whether case-sensitive sorting is enabled for the right panel" },
	{OST_PANELS, NSecPanelRight, "Folder", &Opt.strRightFolder, L"",
		L"FilePanel", L"The saved folder path for the right panel" },
	{OST_PANELS, NSecPanelRight, "CurFile", &Opt.strRightCurFile, L"",
		L"FilePanel", L"The saved current file name for the right panel" },
	{OST_PANELS, NSecPanelRight, "SelectedFirst", &Opt.RightSelectedFirst, 0,
		L"PanelCmdSort", L"Whether selected items are shown first in the right panel" },
	{OST_PANELS, NSecPanelRight, "DirectoriesFirst", &Opt.RightPanel.DirectoriesFirst, 1,
		L"PanelCmdSort", L"Whether directories are shown before files in the right panel" },
	{OST_PANELS, NSecPanelRight, "ExecutablesFirst", &Opt.RightPanel.ExecutablesFirst, 0,
		L"PanelCmdSort", L"Whether executable files are shown before other files in the right panel" },

	{OST_COMMON, NSecPanelLayout, "ColumnTitles", &Opt.ShowColumnTitles, 1,
		L"PanelSettings", L"Shows column titles in file panels" },
	{OST_COMMON, NSecPanelLayout, "StatusLine", &Opt.ShowPanelStatus, 1,
		L"PanelSettings", L"Shows the status line in file panels" },
	{OST_COMMON, NSecPanelLayout, "TotalInfo", &Opt.ShowPanelTotals, 1,
		L"PanelSettings", L"Shows total information in file panels" },
	{OST_COMMON, NSecPanelLayout, "FreeInfo", &Opt.ShowPanelFree, 0,
		L"PanelSettings", L"Shows free-space information in file panels" },
	{OST_COMMON, NSecPanelLayout, "Scrollbar", &Opt.ShowPanelScrollbar, 0,
		L"PanelSettings", L"Shows scrollbars in file panels" },
	{OST_NONE,   NSecPanelLayout, "ScrollbarMenu", &Opt.ShowMenuScrollbar, 1,
		nullptr, L"Shows scrollbars in menus" },
	{OST_COMMON, NSecPanelLayout, "ScreensNumber", &Opt.ShowScreensNumber, 1,
		L"PanelSettings", L"Shows the screen number indicator in panels" },
	{OST_COMMON, NSecPanelLayout, "SortMode", &Opt.ShowSortMode, 1,
		L"PanelSettings", L"Shows the current sort mode indicator in panels" },

	{OST_COMMON, NSecLayout, "LeftHeightDecrement", &Opt.LeftHeightDecrement, 0,
		nullptr, L"Reduces the left panel height by the specified number of rows" },
	{OST_COMMON, NSecLayout, "RightHeightDecrement", &Opt.RightHeightDecrement, 0,
		nullptr, L"Reduces the right panel height by the specified number of rows" },
	{OST_COMMON, NSecLayout, "WidthDecrement", &Opt.WidthDecrement, 0,
		nullptr, L"Reduces panel width by the specified number of columns" },
	{OST_COMMON, NSecLayout, "FullscreenHelp", &Opt.FullScreenHelp, 0,
		L"Help", L"Opens help in full-screen mode" },
	{OST_COMMON, NSecLayout, "PanelsDisposition", &Opt.PanelsDisposition, 0,
		L"PanelCmd", L"Controls panel arrangement: 0=vertical left/right panels, 1=horizontal top/bottom panels" },

	{OST_COMMON, NSecDescriptions, "ListNames", &Opt.Diz.strListNames, L"Descript.ion,Files.bbs",
		L"FileDiz", L"Comma-separated names of files that store file descriptions" },
	{OST_COMMON, NSecDescriptions, "UpdateMode", &Opt.Diz.UpdateMode, DIZ_UPDATE_IF_DISPLAYED,
		L"FileDiz", L"Controls when file description files are updated: 0=do not update descriptions, 1=update only if descriptions are displayed, 2=always update" },
	{OST_COMMON, NSecDescriptions, "ROUpdate", &Opt.Diz.ROUpdate, 0,
		L"FileDiz", L"Allows updating read-only description files" },
	{OST_COMMON, NSecDescriptions, "SetHidden", &Opt.Diz.SetHidden, 1,
		L"FileDiz", L"Sets the hidden attribute on description files when possible" },
	{OST_COMMON, NSecDescriptions, "StartPos", &Opt.Diz.StartPos, 0,
		L"FileDiz", L"Initial cursor position used when editing file descriptions" },
	{OST_COMMON, NSecDescriptions, "AnsiByDefault", &Opt.Diz.AnsiByDefault, 0,
		L"FileDiz", L"Uses ANSI encoding by default for description files" },
	{OST_COMMON, NSecDescriptions, "SaveInUTF", &Opt.Diz.SaveInUTF, 0,
		L"FileDiz", L"Saves description files in UTF encoding" },

	{OST_NONE,   NSecKeyMacros, "MacroReuseRules", &Opt.Macro.MacroReuseRules, 0,
		L"KeyMacroSetting", L"Controls how existing macro records are reused when recording or loading macros; 0 uses the default reuse behavior" },
	{OST_NONE,   NSecKeyMacros, "DateFormat", &Opt.Macro.strDateFormat, L"%a %b %d %H:%M:%S %Z %Y",
		L"KeyMacroLang", L"Date and time format used by macro functions" },
	{OST_NONE,   NSecKeyMacros, "CONVFMT", &Opt.Macro.strMacroCONVFMT, L"%.6g",
		L"KeyMacroLang", L"Floating-point conversion format used by macro expressions" },
	{OST_NONE,   NSecKeyMacros, "CallPluginRules", &Opt.Macro.CallPluginRules, 0,
		L"KeyMacroSetting", L"Controls macro behavior when calling plugins: 0=block macros while a plugin is called, 1=allow macros during plugin calls" },
	{OST_COMMON, NSecKeyMacros, "KeyRecordCtrlDot", &Opt.Macro.strKeyMacroCtrlDot, szCtrlDot,
		L"KeyMacroRecPlay", L"Key sequence used to start or stop macro recording" },
	{OST_COMMON, NSecKeyMacros, "KeyRecordCtrlShiftDot", &Opt.Macro.strKeyMacroCtrlShiftDot, szCtrlShiftDot,
		L"KeyMacroRecPlay", L"Key sequence used to start or stop recording a macro with alternate options" },

	{OST_NONE,   NSecPolicies, "ShowHiddenDrives", &Opt.Policies.ShowHiddenDrives, 1,
		nullptr, L"Allows hidden drives to be shown in drive or location lists" },
	{OST_NONE,   NSecPolicies, "DisabledOptions", &Opt.Policies.DisabledOptions, 0,
		nullptr, L"Bit mask of options disabled by policy" },

	{OST_COMMON, NSecCodePages, "CPMenuMode2", &Opt.CPMenuMode, 1,
		L"CodePagesMenu", L"Controls code page menu mode: 0=show the full code page list, 1=show the shorter/favorites-oriented menu mode" },

	{OST_COMMON, NSecVMenu, "MenuStopWrapOnEdge", &Opt.VMenu.MenuLoopScroll, 1,
		L"VMenuSettings", L"Controls menu selection at list edges: 0=wrap around, 1=stop at the first or last item" },

	{OST_COMMON, NSecVMenu, "LBtnClick", &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL,
		L"VMenuSettings", L"Action performed by a left mouse click outside a vertical menu: 0=do nothing, 1=cancel menu, 2=execute selected item" },
	{OST_COMMON, NSecVMenu, "RBtnClick", &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL,
		L"VMenuSettings", L"Action performed by a right mouse click outside a vertical menu: 0=do nothing, 1=cancel menu, 2=execute selected item" },
	{OST_COMMON, NSecVMenu, "MBtnClick", &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY,
		L"VMenuSettings", L"Action performed by a middle mouse click outside a vertical menu: 0=do nothing, 1=cancel menu, 2=execute selected item" },
	{OST_COMMON, NSecVMenu, "HistShowTimes", ARRAYSIZE(Opt.HistoryShowTimes), Opt.HistoryShowTimes, nullptr,
		L"HistoryCmd", L"Binary settings controlling which history lists show timestamps" },
	{OST_COMMON, NSecVMenu, "HistDirsPrefixLen", &Opt.HistoryDirsPrefixLen, 20,
		L"HistoryCmd", L"Number of leading path characters shown for directories in history menus" },
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


static bool ConfigOptSaveAsk(BOOL &SaveCommon, BOOL &SavePanels, BOOL &SaveMacros)
{
	// Msg::SaveSetupTitle, Msg::SaveSetupAsk1, Msg::SaveSetupAsk2, Msg::SaveSetup, Msg::Cancel
	DialogBuilder Builder(Msg::SaveSetupTitle, nullptr);

	Builder.AddText(Msg::SaveSetupAsk1);
	Builder.AddText(Msg::SaveSetupAsk2);
	Builder.AddCheckbox(Msg::SaveSetupCommon, &SaveCommon);
	Builder.AddCheckbox(Msg::SaveSetupPanels, &SavePanels);
	Builder.AddCheckbox(Msg::SaveSetupMacros, &SaveMacros);

	Builder.DialogBuilderBase<DialogItemEx>::AddOKCancel(Msg::SaveSetup, Msg::Cancel);

	return Builder.ShowDialog();
}

void ConfigOptSave(bool Ask)
{
	if (Opt.Policies.DisabledOptions&0x20000) // Bit 17 - Сохранить параметры
		return;

   	BOOL SavePanels = Ask || Opt.AutoSaveSetup || Opt.AutoSavePanels;
   	BOOL SaveCommon = Ask || Opt.AutoSaveSetup;

	if (!SavePanels && !SaveCommon)
		return;

   	BOOL SaveMacros = Ask;

	if (Ask && !ConfigOptSaveAsk(SaveCommon, SavePanels, SaveMacros))
			return;

	const unsigned SaveFlags
			= (SaveCommon ? OST_COMMON : 0) | (SavePanels ? OST_PANELS : 0);

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	if (SaveCommon)
	{
		WINPORT(SaveConsoleWindowState)();
		CtrlObject->HiFiles->SaveHiData();
	}

	if (SavePanels)
		SavePanelsToOpt();
	/* *************************************************** </ПРЕПРОЦЕССЫ> */

	OptConfigWriter cfg_writer;
	for (size_t i = ConfigOptCount(); i--;)
		cfg_writer.SaveOpt(g_cfg_opts[i], SaveFlags );

	/* <ПОСТПРОЦЕССЫ> *************************************************** */
	if (SaveCommon) {
		FileFilter::SaveFilters(cfg_writer);
		FileList::SavePanelModes(cfg_writer);

	    if (Opt.IsColorsChanged || Ask)
	    {
			FarColors::SaveFarColors();
			Opt.IsColorsChanged = false;
		}
	}
	/* *************************************************** </ПОСТПРОЦЕССЫ> */

	if (SaveMacros)
		CtrlObject->Macro.SaveMacros();
}
