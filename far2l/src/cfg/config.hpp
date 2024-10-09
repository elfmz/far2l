#pragma once

/*
config.hpp

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

#include "FARString.hpp"

enum
{
	DRIVE_SHOW_TYPE       = 0x00000001,
	DRIVE_SHOW_NETNAME    = 0x00000002,
	DRIVE_SHOW_LABEL      = 0x00000004,
	DRIVE_SHOW_FILESYSTEM = 0x00000008,
	DRIVE_SHOW_SIZE       = 0x00000010,
	DRIVE_SHOW_MOUNTS     = 0x00000020,
	DRIVE_SHOW_PLUGINS    = 0x00000040,
	DRIVE_SHOW_BOOKMARKS  = 0x00000080,
	DRIVE_SHOW_SIZE_FLOAT = 0x00000100,
	DRIVE_SHOW_REMOTE     = 0x00000200,
};

//  +CASR_* Поведение Ctrl-Alt-Shift для AllCtrlAltShiftRule
enum
{
	CASR_PANEL  = 0x0001,
	CASR_EDITOR = 0x0002,
	CASR_VIEWER = 0x0004,
	CASR_HELP   = 0x0008,
	CASR_DIALOG = 0x0010,
};

enum ExcludeCmdHistoryType
{
	EXCLUDECMDHISTORY_NOTWINASS  = 0x00000001,		// не помещать в историю команды ассоциаций Windows
	EXCLUDECMDHISTORY_NOTFARASS  = 0x00000002,		// не помещать в историю команды выполнения ассоциаций файлов
	EXCLUDECMDHISTORY_NOTPANEL   = 0x00000004,		// не помещать в историю команды выполнения с панели
	EXCLUDECMDHISTORY_NOTCMDLINE = 0x00000008,		// не помещать в историю команды выполнения с ком.строки
													// EXCLUDECMDHISTORY_NOTAPPLYCMD   = 0x00000010,  // не помещать в историю команды выполнения из "Apply Commang"
};

// для Opt.QuotedName
enum QUOTEDNAMETYPE
{
	QUOTEDNAME_INSERT    = 0x00000001,		// кавычить при сбросе в командную строку, в диалогах и редакторе
	QUOTEDNAME_CLIPBOARD = 0x00000002,		// кавычить при помещении в буфер обмена
};

// Для Opt.Dialogs.MouseButton
#define DMOUSEBUTTON_LEFT  0x00000001
#define DMOUSEBUTTON_RIGHT 0x00000002

// Для Opt.VMenu.xBtnClick
#define VMENUCLICK_IGNORE 0
#define VMENUCLICK_CANCEL 1
#define VMENUCLICK_APPLY  2

// Для Opt.Diz.UpdateMode
enum DIZUPDATETYPE
{
	DIZ_NOT_UPDATE,
	DIZ_UPDATE_IF_DISPLAYED,
	DIZ_UPDATE_ALWAYS
};

struct PanelOptions
{
	int Type;
	int Visible;
	int Focus;
	int ViewMode;
	int SortMode;
	int SortOrder;
	int SortGroups;
	int NumericSort;
	int CaseSensitiveSort;
	int DirectoriesFirst;
};

struct AutoCompleteOptions
{
	int ShowList;
	int ModalList;
	int AppendCompletion;
	FARString Exceptions;
};

struct PluginConfirmation
{
	int OpenFilePlugin;
	int StandardAssociation;
	int EvenIfOnlyOnePlugin;
	int SetFindList;
	int Prefix;
};

struct Confirmation
{
	int Copy;
	int Move;
	int RO;
	int Drag;
	int Delete;
	int DeleteFolder;

	int Exit;			// see ExitEffective()
	int ExitOrBknd;		// see ExitEffective()
	/// returns reference to Exit or ExitOrBknd - depending of background mode availability
	int &ExitEffective();

	int Esc;	// Для CheckForEsc
	/*
		$ 12.03.2002 VVM
		+ Opt.EscTwiceToInterrupt
		Определяет поведение при прерывании длительной операции
		0 - второй ESC продолжает операцию
		1 - второй ESC прерывает операцию
	*/
	int EscTwiceToInterrupt;
	int RemoveConnection;
	/*
		$ 23.05.2001
		+ Opt.Confirmation.AllowReedit - Флаг, который изменяет поведение открытия
		файла на редактирование если, данный файл уже редактируется. По умолчанию - 1
		0 - Если уже открытый файл не был изменен, то происходит переход к открытому редактору
		без дополнительных вопросов. Если файл был изменен, то задается вопрос, и в случае
		если выбран вариант Reload, то загружается новая копия файла, при этом сделанные
		изменения теряются.
		1 - Так как было раньше. Задается вопрос и происходит переход либо уже к открытому файлу
		либо загружается новая версия редактора.
	*/
	int AllowReedit;
	int HistoryClear;
	int ClearVT;
	int RemoveHotPlug;
};

struct DizOptions
{
	FARString strListNames;
	int ROUpdate;
	int UpdateMode;
	int SetHidden;
	int StartPos;
	int AnsiByDefault;
	int SaveInUTF;
};

struct CodeXLAT
{
	int EnableForFastFileFind = 1;
	int EnableForDialogs      = 1;
	DWORD Flags               = XLAT_SWITCHKEYBLAYOUT;

	/*
		$ 25.11.2000 IS
		Разграничитель слов из реестра для функции Xlat
	*/
	FARString strWordDivForXlat;
	FARString XLat;
};

struct NotificationsOptions
{
	int OnFileOperation;
	int OnConsole;

	int OnlyIfBackground;
};

struct EditorOptions
{
	int TabSize;
	int ExpandTabs;
	int PersistentBlocks;
	int DelRemovesBlocks;
	int AutoIndent;
	int AutoDetectCodePage;
	UINT DefaultCodePage;
	int CursorBeyondEOL;
	int BSLikeDel;
	int CharCodeBase;
	int SavePos;
	int SaveShortPos;
	int AllowEmptySpaceAfterEof;	// $ 21.06.2005 SKV - разрешить показывать пустое пространство после последней строки редактируемого файла.
	int ReadOnlyLock;				// $ 29.11.2000 SVS - лочить файл при открытии в редакторе, если он имеет атрибуты R|S|H
	int UndoSize;					// $ 03.12.2001 IS - размер буфера undo в редакторе
	int UseExternalEditor;
	DWORD FileSizeLimitLo;
	DWORD FileSizeLimitHi;
	int ShowKeyBar;
	int ShowTitleBar;
	int ShowScrollBar;
	int UseEditorConfigOrg;
	int SearchSelFound;
	int SearchRegexp;
	int SearchPickUpWord;
	int ShowWhiteSpace;

	FARString strWordDiv;
};

/*
	$ 29.03.2001 IS
	Тут следует хранить "локальные" настройки для программы просмотра
*/
struct ViewerOptions
{
	int TabSize;
	int AutoDetectCodePage;
	int ShowScrollbar;		// $ 18.07.2000 tran пара настроек для viewer
	int ShowArrows;
	int PersistentBlocks;	// $ 14.05.2002 VVM Постоянные блоки во вьюере
	int ViewerIsWrap;		// (Wrap|WordWarp)=1 | UnWrap=0
	int ViewerWrap;			// Wrap=0|WordWarp=1
	int SavePos;
	int SaveShortPos;
	int UseExternalViewer;
	int ShowKeyBar;		// $ 15.07.2000 tran + ShowKeyBar
	UINT DefaultCodePage;
	int ShowTitleBar;
	int SearchRegexp;
};

// "Полиция"
struct PoliciesOptions
{
	int DisabledOptions;	// разрешенность меню конфигурации
	int ShowHiddenDrives;	// показывать скрытые логические диски
};

struct DialogsOptions
{
	int EditBlock;			// Постоянные блоки в строках ввода
	int EditHistory;		// Добавлять в историю?
	int AutoComplete;		// Разрешено автодополнение?
	int EULBsClear;			// = 1 - BS в диалогах для UnChanged строки удаляет такую строку также, как и Del
	int SelectFromHistory;	// = 0 then (ctrl-down в строке с историей курсор устанавливался на самую верхнюю строку)
	DWORD EditLine;			// общая информация о строке ввода (сейчас это пока... позволяет управлять выделением)
	int MouseButton;		// Отключение восприятия правой/левой кнопки мыши как команд закрытия окна диалога
	int DelRemovesBlocks;
	int CBoxMaxHeight;		// максимальный размер открываемого списка (по умолчанию=8)
};

struct VMenuOptions
{
	int LBtnClick;
	int RBtnClick;
	int MBtnClick;
	bool MenuLoopScroll;
};

struct CommandLineOptions
{
	int EditBlock;
	int DelRemovesBlocks;
	int AutoComplete;
	int Splitter;
	int UsePromptFormat;
	int UseShell;
	int WaitKeypress;
	int VTLogLimit;
	FARString strPromptFormat;
	FARString strShell;
};

struct NowellOptions
{
	int MoveRO;		// перед операцией Move снимать R/S/H атрибуты, после переноса - выставлять обратно
};

struct ScreenSizes
{
	union
	{
		COORD DeltaXY;	// на сколько поз. изменить размеры для распахнутого экрана
		DWORD dwDeltaXY;
	};
	int WScreenSizeSet;
	COORD WScreenSize[4];
};

struct LoadPluginsOptions
{
	//  DWORD TypeLoadPlugins;       // see TYPELOADPLUGINSOPTIONS
	int MainPluginDir;		// TRUE - использовать стандартный путь к основным плагинам
	int PluginsCacheOnly;	// setting by '/co' switch, not saved in registry
	int PluginsPersonal;

	FARString strCustomPluginsPath;		// путь для поиска плагинов, указанный в /p
	FARString strPersonalPluginsPath;
	int SilentLoadPlugin;				// при загрузке плагина с кривым...
	int ScanSymlinks;
};

struct FindFileOptions
{
	int FileSearchMode;
	bool FindFolders;
	bool FindSymLinks;
	bool FindCaseSensitiveFileMask;
	bool CollectFiles;
	bool UseFilter;
	bool FindAlternateStreams;
	FARString strSearchInFirstSize;

	FARString strSearchOutFormat;
	FARString strSearchOutFormatWidth;
	int OutColumnCount;
	unsigned int OutColumnTypes[20];
	int OutColumnWidths[20];
	int OutColumnWidthType[20];
};

struct InfoPanelOptions
{
	FARString strFolderInfoFiles;
};

struct TreeOptions
{
	int LocalDisk;			// Хранить файл структуры папок для локальных дисков
	int NetDisk;			// Хранить файл структуры папок для сетевых дисков
	int NetPath;			// Хранить файл структуры папок для сетевых путей
	int RemovableDisk;		// Хранить файл структуры папок для сменных дисков
	int MinTreeCount;		// Минимальное количество папок для сохранения дерева в файле.
	int AutoChangeFolder;	// автосмена папок при перемещении по дереву
	DWORD TreeFileAttr;		// файловые атрибуты для файлов-деревях
};

struct CopyMoveOptions
{
	int WriteThrough;		// disable write caching
	int CopyXAttr;			// copy extended attributes if any
	int CopyAccessMode;		// copy files access mode
	int CopyOpened;			// копировать открытые на запись файлы
	int CopyShowTotal;		// показать общий индикатор копирования
	int MultiCopy;			// "разрешить мультикопирование/перемещение/создание связей"
	int CopyTimeRule;		// $ 30.01.2001 VVM  Показывает время копирования,оставшееся время и среднюю скорость
	int HowCopySymlink;
	int SparseFiles;
	int UseCOW;
};

struct DeleteOptions
{
	int DelShowTotal;	// показать общий индикатор удаления
};

struct MacroOptions
{
	int MacroReuseRules;			// Правило на счет повторно использования забинденных клавиш
	DWORD DisableMacro;				// параметры /m или /ma или /m....
	DWORD KeyMacroCtrlDot;			// аля KEY_CTRLDOT
	DWORD KeyMacroCtrlShiftDot;		// аля KEY_CTRLSHIFTDOT
	int CallPluginRules;			// 0 - блокировать макросы при вызове плагина, 1 - разрешить макросы (ахтунг!)
	FARString strMacroCONVFMT;		// формат преобразования double в строку
	FARString strDateFormat;		// Для $Date
};

struct Options
{
	int Clock;
	int Mouse;
	int ShowKeyBar;
	int ScreenSaver;
	int ScreenSaverTime;
	int UseVk_oem_x;
	int InactivityExit;
	int InactivityExitTime;
	int ShowHidden;

	int ShowFilenameMarks;
	int FilenameMarksAlign;
	DWORD MinFilenameIndentation, MaxFilenameIndentation;

	int Highlight;
	int CursorBlinkTime;

	FARString strLeftFolder;
	FARString strRightFolder;

	FARString strLeftCurFile;
	FARString strRightCurFile;

	int RightSelectedFirst;
	int LeftSelectedFirst;
	int SelectFolders;
	int PanelCaseSensitiveCompareSelect;
	int ReverseSort;
	int SortFolderExt;
	int DeleteToRecycleBin;				// удалять в корзину?
	int DeleteToRecycleBinKillLink;		// перед удалением папки в корзину кильнем вложенные симлинки.
	int WipeSymbol;						// символ заполнитель для "ZAP-операции"
	int SudoEnabled;
	int SudoConfirmModify;
	int SudoPasswordExpiration;

	CopyMoveOptions CMOpt;

	int MakeLinkSuggestSymlinkAlways;	// по Alt-F6 предлагать всегда symlink или hardink/symlink в зависимости от того среди выбранных только файлы или и каталоги

	DeleteOptions DelOpt;

	int MultiMakeDir;	// Опция создания нескольких каталогов за один сеанс

	int ViewerEditorClock;

	enum OnlyEditorViewerUsedT
	{
		NOT_ONLY_EDITOR_VIEWER = 0,
		ONLY_EDITOR,
		ONLY_VIEWER,
		ONLY_EDITOR_ON_CMDOUT,
		ONLY_VIEWER_ON_CMDOUT
	} OnlyEditorViewerUsed;

	int SaveViewHistory;
	int ViewHistoryCount;

	FARString strExternalEditor;
	EditorOptions EdOpt;
	NotificationsOptions NotifOpt;
	FARString strExternalViewer;
	ViewerOptions ViOpt;

	FARString strWordDiv;	// $ 03.08.2000 SVS Разграничитель слов из реестра
	FARString strQuotedSymbols;
	DWORD QuotedName;
	int AutoSaveSetup;
	int SetupArgv;	// количество каталогов в ком.строке ФАРа
	int ChangeDriveMode;
	int ChangeDriveDisconnectMode;
	FARString ChangeDriveExceptions;
	FARString ChangeDriveColumn2, ChangeDriveColumn3;

	int SaveHistory;
	int HistoryCount;
	int SaveFoldersHistory;
	int SavePluginFoldersHistory;
	int FoldersHistoryCount;
	int DialogsHistoryCount;
	int HistoryRemoveDupsRule;
	int AutoHighlightHistory;

	BYTE HistoryShowTimes[8];
	DWORD HistoryDirsPrefixLen;

	FindFileOptions FindOpt;

	int LeftHeightDecrement;
	int RightHeightDecrement;
	int WidthDecrement;

	int ShowColumnTitles;
	int ShowPanelStatus;
	int ShowPanelTotals;
	int ShowPanelFree;
	int ShowPanelScrollbar;
	int ShowMenuScrollbar;	// $ 29.06.2000 SVS Добавлен атрибут показа Scroll Bar в меню.
	int ShowScreensNumber;
	int ShowSortMode;
	int ShowMenuBar;
	int FormatNumberSeparators;
	int CleanAscii;
	int NoGraphics;
	int NoBoxes;
	int ConsolePaintSharp, ExclusiveCtrlLeft, ExclusiveCtrlRight, ExclusiveAltLeft, ExclusiveAltRight,
			ExclusiveWinLeft, ExclusiveWinRight;
	int OSC52ClipSet;
	int TTYPaletteOverride;

	Confirmation Confirm;
	PluginConfirmation PluginConfirm;

	DizOptions Diz;

	int ShellRightLeftArrowsRule;
	PanelOptions LeftPanel;
	PanelOptions RightPanel;

	AutoCompleteOptions AutoComplete;

	DWORD AutoUpdateLimit;	// выше этого количество автоматически не обновлять панели.
	int AutoUpdateRemoteDrive;

	FARString strLanguage;
	int SmallIcon;
	FARString strRegRoot;
	int PanelRightClickRule;	// задает поведение правой клавиши мыши
	int PanelCtrlAltShiftRule;	// задает поведение Ctrl-Alt-Shift для панелей.
	// Panel/CtrlFRule в реестре - задает поведение Ctrl-F. Если = 0, то штампуется файл как есть, иначе - с учетом отображения на панели
	int PanelCtrlFRule;
	/*
		битовые флаги, задают поведение Ctrl-Alt-Shift
			бит установлен - функция включена:
			0 - Panel
			1 - Edit
			2 - View
			3 - Help
			4 - Dialog
	*/
	int AllCtrlAltShiftRule;

	int CASRule;	// 18.12.2003 - Пробуем различать левый и правый CAS (попытка #1).
	/*
	задает поведение Esc для командной строки:
	=1 - Не изменять положение в History, если после Ctrl-E/Ctrl/-X
	нажали ESC (поведение - аля VC).

	=0 - поведение как и было - изменять положение в History
	*/
	int CmdHistoryRule;

	DWORD ExcludeCmdHistory;
	int MaxPositionCache;		// количество позиций в кэше сохранения
	int SetAttrFolderRules;		// Правило на счет установки атрибутов на каталоги
	/*
		+ Opt.ShiftsKeyRules - Правило на счет выбора механизма трансляции
		Alt-Буква для нелатинских буковок и символов "`-=[]\;',./" с
		модификаторами Alt-, Ctrl-, Alt-Shift-, Ctrl-Shift-, Ctrl-Alt-
	*/
	int ShiftsKeyRules;
	int CursorSize[4];	// Размер курсора ФАРа

	CodeXLAT XLat;

	int ConsoleDetachKey;	// Комбинация клавиш для детача Far'овской консоли от длительного неинтерактивного процесса в ней запущенного.

	int UsePrintManager;

	FARString strHelpLanguage;
	int FullScreenHelp;
	int HelpTabSize;

	// запоминать логические диски и не опрашивать каждый раз. Для предотвращения "просыпания" "зеленых" винтов.
	int RememberLogicalDrives;
	/*
		будет влиять на:
			добавление файлов в историю с разным регистром
			добавление LastPositions в редакторе и вьюере
	*/
	int MsWheelDelta;	// задает смещение для прокрутки
	int MsWheelDeltaView;
	int MsWheelDeltaEdit;
	int MsWheelDeltaHelp;
	// горизонтальная прокрутка
	int MsHWheelDelta;
	int MsHWheelDeltaView;
	int MsHWheelDeltaEdit;

	/*
		$ 28.04.2001 VVM
		+ Opt.SubstNameRule битовая маска:
		0 - если установлен, то опрашивать сменные диски при GetSubstName()
		1 - если установлен, то опрашивать все остальные при GetSubstName()
	*/
	int SubstNameRule;

	int PgUpChangeDisk;
	int ShowCheckingFile;

	DWORD LCIDSort;
	int RestoreCPAfterExecute;
	int ExecuteShowErrorMessage;
	int ExecuteUseAppPath;
	int ExecuteFullTitle;
	int ExecuteSilentExternal;

	DWORD PluginMaxReadData;
	int UseNumPad;
	int ScanJunction;
	int OnlyFilesSize;

	DWORD ShowTimeoutDelFiles;	// таймаут в процессе удаления (в ms)
	DWORD ShowTimeoutDACLFiles;

	// int CPAJHefuayor; // производное от "Close Plugin And Jump:
	//  Highly experimental feature, use at your own risk"

	LoadPluginsOptions LoadPlug;

	DialogsOptions Dialogs;
	VMenuOptions VMenu;
	CommandLineOptions CmdLine;
	PoliciesOptions Policies;
	NowellOptions Nowell;
	ScreenSizes ScrSize;
	MacroOptions Macro;

	DWORD FindCodePage;

	TreeOptions Tree;
	InfoPanelOptions InfoPanel;

	DWORD CPMenuMode;

	bool IsUserAdmin;
	FARString strWindowTitle;

	int DateFormat;
	FARString strDateSeparator;
	FARString strTimeSeparator;
	FARString strDecimalSeparator;

	bool IsFirstStart;

	std::vector<std::wstring> CmdLineStrings;
};

extern Options Opt;

void SystemSettings();
void PanelSettings();
void InputSettings();
void InterfaceSettings();
void DialogSettings();
void VMenuSettings();
void CmdlineSettings();
void SetConfirmations();
void PluginsManagerSettings();
void SetDizConfig();
void ViewerConfig(ViewerOptions &ViOpt, bool Local = false);
//void EditorConfig(EditorOptions &EdOpt, bool Local = false);
void EditorConfig(EditorOptions &EdOpt, bool Local = false, int EdCfg_ExpandTabs = -1, int EdCfg_TabSize = -1);
void NotificationsConfig(NotificationsOptions &NotifOpt);
void ApplyConfig();
void SetFolderInfoFiles();
void InfoPanelSettings();
void AutoCompleteSettings();
void LanguageSettings();
