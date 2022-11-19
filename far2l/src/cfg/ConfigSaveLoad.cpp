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

void SanitizeHistoryCounts();

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
static bool g_config_ready = false;

static constexpr class OptSerializer
{
	const char *_section;
	const char *_key;
	WORD  _bin_size;  // used only with _type == T_BIN
	bool  _save;   // =true - будет записываться в SaveConfig()

	enum T
	{
		T_STR = 0,
		T_BIN,
		T_DWORD,
		T_INT,
		T_BOOL
	} _type : 8;

	union V
	{
		FARString *str;
		BYTE *bin;
		DWORD *dw;
		int *i;
		bool *b;
	} _value;

	union D
	{
		const wchar_t *str;
		const BYTE *bin;
		DWORD dw;
		int i;
		bool b;
	} _default;

public:
	constexpr OptSerializer(bool save, const char *section, const char *key, WORD size, BYTE *data_bin, const BYTE *def_bin)
		: _section{section}, _key{key}, _bin_size{size}, _save{save},
		_type{T_BIN}, _value{.bin = data_bin}, _default{.bin = def_bin}
	{ }

	constexpr OptSerializer(bool save, const char *section, const char *key, FARString *data_str, const wchar_t *def_str)
		: _section{section}, _key{key}, _bin_size{0}, _save{save},
		_type{T_STR}, _value{.str = data_str}, _default{.str = def_str}
	{ }

	constexpr OptSerializer(bool save, const char *section, const char *key, DWORD *data_dw, DWORD def_dw)
		: _section{section}, _key{key}, _bin_size{0}, _save{save},
		_type{T_DWORD}, _value{.dw = data_dw}, _default{.dw = def_dw}
	{ }

	constexpr OptSerializer(bool save, const char *section, const char *key, int *data_i, int def_i)
		: _section{section}, _key{key}, _bin_size{0}, _save{save},
		_type{T_INT}, _value{.i = data_i}, _default{.i = def_i}
	{ }

	constexpr OptSerializer(bool save, const char *section, const char *key, bool *data_b, bool def_b)
		: _section{section}, _key{key}, _bin_size{0}, _save{save},
		_type{T_BOOL}, _value{.b = data_b}, _default{.b = def_b}
	{ }

	void Load(ConfigReader &cfg_reader) const
	{
		cfg_reader.SelectSection(_section);
		switch (_type)
		{
			case T_INT:
				*_value.i = cfg_reader.GetInt(_key, _default.i);
				break;
			case T_DWORD:
				*_value.dw = cfg_reader.GetUInt(_key, _default.dw);
				break;
			case T_BOOL:
				*_value.b = cfg_reader.GetInt(_key, _default.b ? 1 : 0) != 0;
				break;
			case T_STR:
				*_value.str = cfg_reader.GetString(_key, _default.str);
				break;
			case T_BIN:
				{
					const size_t Size = cfg_reader.GetBytes(_value.bin, _bin_size, _key, _default.bin);
					if (Size < (size_t)_bin_size)
						memset(_value.bin + Size, 0, (size_t)_bin_size - Size);
				}
				break;
		}
	}

	void Save(ConfigWriter &cfg_writer) const
	{
		if (!_save)
			return;

		cfg_writer.SelectSection(_section);
		switch (_type)
		{
			case T_BOOL:
				cfg_writer.SetInt(_key, *_value.b ? 1 : 0);
				break;
			case T_INT:
				cfg_writer.SetInt(_key, *_value.i);
				break;
			case T_DWORD:
				cfg_writer.SetUInt(_key, *_value.dw);
				break;
			case T_STR:
				cfg_writer.SetString(_key, _value.str->CPtr());
				break;
			case T_BIN:
				cfg_writer.SetBytes(_key, _value.bin, _bin_size);
				break;
		}
	}

} s_opt_serializers[] =
{
	{true,  NSecColors, "CurrentPalette", SIZE_ARRAY_PALETTE, Palette, DefaultPalette},

	{true,  NSecScreen, "Clock", &Opt.Clock, 1},
	{true,  NSecScreen, "ViewerEditorClock", &Opt.ViewerEditorClock, 0},
	{true,  NSecScreen, "KeyBar", &Opt.ShowKeyBar, 1},
	{true,  NSecScreen, "ScreenSaver", &Opt.ScreenSaver, 0},
	{true,  NSecScreen, "ScreenSaverTime", &Opt.ScreenSaverTime, 5},
	{false, NSecScreen, "DeltaXY", (DWORD *)&Opt.ScrSize.DeltaXY, 0},

	{true,  NSecCmdline, "UsePromptFormat", &Opt.CmdLine.UsePromptFormat, 0},
	{true,  NSecCmdline, "PromptFormat", &Opt.CmdLine.strPromptFormat, L"$p$# "},
	{true,  NSecCmdline, "UseShell", &Opt.CmdLine.UseShell, 0},
	{true,  NSecCmdline, "Shell", &Opt.CmdLine.strShell, L"/bin/bash"},
	{true,  NSecCmdline, "DelRemovesBlocks", &Opt.CmdLine.DelRemovesBlocks, 1},
	{true,  NSecCmdline, "EditBlock", &Opt.CmdLine.EditBlock, 0},
	{true,  NSecCmdline, "AutoComplete", &Opt.CmdLine.AutoComplete, 1},
	{true,  NSecCmdline, "WaitKeypress", &Opt.CmdLine.WaitKeypress, 1},

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

	{false, NSecInterface, "ShowTimeoutDelFiles", &Opt.ShowTimeoutDelFiles, 50},
	{false, NSecInterface, "ShowTimeoutDACLFiles", &Opt.ShowTimeoutDACLFiles, 50},
	{false, NSecInterface, "FormatNumberSeparators", &Opt.FormatNumberSeparators, 0},
	{true,  NSecInterface, "CopyShowTotal", &Opt.CMOpt.CopyShowTotal, 1},
	{true,  NSecInterface, "DelShowTotal", &Opt.DelOpt.DelShowTotal, 0},
	{true,  NSecInterface, "WindowTitle", &Opt.strWindowTitle, L"%State - FAR2L %Ver %Backend %User@%Host"}, // %Platform 
	{true,  NSecInterfaceCompletion, "Exceptions", &Opt.AutoComplete.Exceptions, L"git*reset*--hard;*://*:*@*"},
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
	{false, NSecEditor, "EditorF7Rules", &Opt.EdOpt.F7Rules, 1},
	{false, NSecEditor, "FileSizeLimit", &Opt.EdOpt.FileSizeLimitLo, 0},
	{false, NSecEditor, "FileSizeLimitHi", &Opt.EdOpt.FileSizeLimitHi, 0},
	{false, NSecEditor, "CharCodeBase", &Opt.EdOpt.CharCodeBase, 1},
	{false, NSecEditor, "AllowEmptySpaceAfterEof", &Opt.EdOpt.AllowEmptySpaceAfterEof, 0},//skv
	{true,  NSecEditor, "DefaultCodePage", &Opt.EdOpt.DefaultCodePage, CP_UTF8},
	{true,  NSecEditor, "ShowKeyBar", &Opt.EdOpt.ShowKeyBar, 1},
	{true,  NSecEditor, "ShowTitleBar", &Opt.EdOpt.ShowTitleBar, 1},
	{true,  NSecEditor, "ShowScrollBar", &Opt.EdOpt.ShowScrollBar, 0},
	{true,  NSecEditor, "EditOpenedForWrite", &Opt.EdOpt.EditOpenedForWrite, 1},
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
	{true,  NSecSystem, "AutoSaveSetup", &Opt.AutoSaveSetup, 0},
	{true,  NSecSystem, "DeleteToRecycleBin", &Opt.DeleteToRecycleBin, 0},
	{true,  NSecSystem, "DeleteToRecycleBinKillLink", &Opt.DeleteToRecycleBinKillLink, 1},
	{false, NSecSystem, "WipeSymbol", &Opt.WipeSymbol, 0},
	{true,  NSecSystem, "SudoEnabled", &Opt.SudoEnabled, 1},
	{true,  NSecSystem, "SudoConfirmModify", &Opt.SudoConfirmModify, 1},
	{true,  NSecSystem, "SudoPasswordExpiration", &Opt.SudoPasswordExpiration, 15 * 60},

	{true,  NSecSystem, "UseCOW", &Opt.CMOpt.SparseFiles, 0},
	{true,  NSecSystem, "SparseFiles", &Opt.CMOpt.SparseFiles, 0},
	{true,  NSecSystem, "HowCopySymlink", &Opt.CMOpt.HowCopySymlink, 1},
	{true,  NSecSystem, "WriteThrough", &Opt.CMOpt.WriteThrough, 0},
	{true,  NSecSystem, "CopyXAttr", &Opt.CMOpt.CopyXAttr, 0},
	{false, NSecSystem, "CopyAccessMode", &Opt.CMOpt.CopyAccessMode, 1},
	{true,  NSecSystem, "MultiCopy", &Opt.CMOpt.MultiCopy, 0},
	{true,  NSecSystem, "CopyTimeRule",  &Opt.CMOpt.CopyTimeRule, 3},

	{true,  NSecSystem, "InactivityExit", &Opt.InactivityExit, 0},
	{true,  NSecSystem, "InactivityExitTime", &Opt.InactivityExitTime, 15},
	{true,  NSecSystem, "DriveMenuMode2", &Opt.ChangeDriveMode, -1},
	{true,  NSecSystem, "DriveDisconnetMode", &Opt.ChangeDriveDisconnetMode, 1},

	{true,  NSecSystem, "DriveExceptions", &Opt.ChangeDriveExceptions,
		L"/System/*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron;/tmp/.*"},
	{true,  NSecSystem, "DriveColumn2", &Opt.ChangeDriveColumn2, L"$U/$T"},
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
	{true,  NSecSystem, "UseFilterInSearch", &Opt.FindOpt.UseFilter, 0},
	{true,  NSecSystem, "FindCodePage", &Opt.FindCodePage, CP_AUTODETECT},
	{false, NSecSystem, "CmdHistoryRule", &Opt.CmdHistoryRule, 0},
	{false, NSecSystem, "SetAttrFolderRules", &Opt.SetAttrFolderRules, 1},
	{false, NSecSystem, "MaxPositionCache", &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS},
	{false, NSecSystem, "ConsoleDetachKey", &strKeyNameConsoleDetachKey, L"CtrlAltTab"},
	{false, NSecSystem, "SilentLoadPlugin",  &Opt.LoadPlug.SilentLoadPlugin, 0},
	{true,  NSecSystem, "OEMPluginsSupport",  &Opt.LoadPlug.OEMPluginsSupport, 1},
	{true,  NSecSystem, "ScanSymlinks",  &Opt.LoadPlug.ScanSymlinks, 1},
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
	{false, NSecSystem, "WindowMode", &Opt.WindowMode, 0},

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
	{true,  NSecConfirmations, "RemoveSUBST", &Opt.Confirm.RemoveSUBST, 1},
	{true,  NSecConfirmations, "DetachVHD", &Opt.Confirm.DetachVHD, 1},
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
	{true,  NSecPanel, "ReverseSort", &Opt.ReverseSort, 1},
	{false, NSecPanel, "RightClickRule", &Opt.PanelRightClickRule, 2},
	{false, NSecPanel, "CtrlFRule", &Opt.PanelCtrlFRule, 1},
	{false, NSecPanel, "CtrlAltShiftRule", &Opt.PanelCtrlAltShiftRule, 0},
	{false, NSecPanel, "RememberLogicalDrives", &Opt.RememberLogicalDrives, 0},
	{true,  NSecPanel, "AutoUpdateLimit", &Opt.AutoUpdateLimit, 0},

	{true,  NSecPanelLeft, "Type", &Opt.LeftPanel.Type, 0},
	{true,  NSecPanelLeft, "Visible", &Opt.LeftPanel.Visible, 1},
	{true,  NSecPanelLeft, "Focus", &Opt.LeftPanel.Focus, 1},
	{true,  NSecPanelLeft, "ViewMode", &Opt.LeftPanel.ViewMode, 2},
	{true,  NSecPanelLeft, "SortMode", &Opt.LeftPanel.SortMode, 1},
	{true,  NSecPanelLeft, "SortOrder", &Opt.LeftPanel.SortOrder, 1},
	{true,  NSecPanelLeft, "SortGroups", &Opt.LeftPanel.SortGroups, 0},
	{true,  NSecPanelLeft, "NumericSort", &Opt.LeftPanel.NumericSort, 0},
	{true,  NSecPanelLeft, "CaseSensitiveSortNix", &Opt.LeftPanel.CaseSensitiveSort, 1},
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
	{true,  NSecPanelRight, "CaseSensitiveSortNix", &Opt.RightPanel.CaseSensitiveSort, 1},
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


	{false, NSecSystem, "ExcludeCmdHistory", &Opt.ExcludeCmdHistory, 0}, //AN

	{true,  NSecCodePages, "CPMenuMode2", &Opt.CPMenuMode, 1},

	{true,  NSecSystem, "FolderInfo", &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{true,  NSecVMenu, "LBtnClick", &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL},
	{true,  NSecVMenu, "RBtnClick", &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL},
	{true,  NSecVMenu, "MBtnClick", &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY},
};

///////////////////////////////////////////////////////////////////////////////////////

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
	//   Уточняем алгоритм "взятия" палитры.
	for (size_t I = COL_PRIVATEPOSITION_FOR_DIF165ABOVE - COL_FIRSTPALETTECOLOR + 1;
	        I < (COL_LASTPALETTECOLOR - COL_FIRSTPALETTECOLOR);
	        ++I)
	{
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
}

void LoadConfig()
{
	ConfigReader cfg_reader;

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	bool ExplicitWindowMode = (Opt.WindowMode != FALSE);
	//Opt.LCIDSort=LOCALE_USER_DEFAULT; // проинициализируем на всякий случай
	/* *************************************************** </ПРЕПРОЦЕССЫ> */
	for (const auto &opt_ser : s_opt_serializers)
		opt_ser.Load(cfg_reader);

	/* <ПОСТПРОЦЕССЫ> *************************************************** */

	SanitizeHistoryCounts();

	if (Opt.ShowMenuBar)
		Opt.ShowMenuBar = 1;

	if (Opt.PluginMaxReadData < 0x1000) // || Opt.PluginMaxReadData > 0x80000)
		Opt.PluginMaxReadData = 0x20000;

	if (ExplicitWindowMode)
		Opt.WindowMode = TRUE;

	Opt.HelpTabSize = 8; // пока жестко пропишем...
	SanitizePalette();

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

	g_config_ready = true;
	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}

void AssertConfigLoaded()
{
	if (!g_config_ready)
	{
		fprintf(stderr, "%s: oops\n", __FUNCTION__);
		abort();
	}
}

void SaveConfig(int Ask)
{
	if (Opt.Policies.DisabledOptions&0x20000) // Bit 17 - Сохранить параметры
		return;

	if (Ask && Message(0, 2, Msg::SaveSetupTitle, Msg::SaveSetupAsk1, Msg::SaveSetupAsk2, Msg::SaveSetup, Msg::Cancel))
		return;

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

	ConfigWriter cfg_writer;
	for (const auto &opt_ser : s_opt_serializers)
		opt_ser.Save(cfg_writer);

	/* <ПОСТПРОЦЕССЫ> *************************************************** */
	FileFilter::SaveFilters(cfg_writer);
	FileList::SavePanelModes(cfg_writer);

	if (Ask)
		CtrlObject->Macro.SaveMacros();

	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}

