#pragma once

// --- misc

class Panel;

struct TMacroFunction;
typedef bool (*INTMACROFUNC)(const TMacroFunction *);

enum INTMF_FLAGS
{
	IMFF_UNLOCKSCREEN    = 0x00000001,
	IMFF_DISABLEINTINPUT = 0x00000002,
};

struct TMacroFunction
{
	const wchar_t *Name;	// имя функции
	int nParam;				// количество параметров
	int oParam;				// необязательные параметры
	TMacroOpCode Code;		// байткод функции
	const wchar_t *fnGUID;	// GUID обработчика функции

	int BufferSize;			// Размер буфера компилированной последовательности
	DWORD *Buffer;			// компилированная последовательность (OpCode) макроса
	// wchar_t  *Src;                 // оригинальный "текст" макроса
	// wchar_t  *Description;         // описание макроса

	const wchar_t *Syntax;	// Синтаксис функции

	DWORD IntFlags;			// флаги из INTMF_FLAGS (в основном отвечающие "как вызывать функцию")
	INTMACROFUNC Func;		// функция
};

struct MacroRecord
{
	DWORD Flags;			// Флаги макропоследовательности
	uint32_t Key;			// Назначенная клавиша
	int BufferSize;			// Размер буфера компилированной последовательности
	DWORD *Buffer;			// компилированная последовательность (OpCode) макроса
	wchar_t *Src;			// оригинальный "текст" макроса
	wchar_t *Description;	// описание макроса
	DWORD Reserved[2];		// зарезервировано
};

#define STACKLEVEL 32

struct MacroState
{
	int KeyProcess;
	int Executing;
	int MacroPC;
	int ExecLIBPos;
	int MacroWORKCount;
	bool UseInternalClipboard;
	struct MacroRecord *MacroWORK;	// т.н. текущее исполнение
	INPUT_RECORD cRec;				// "описание реально нажатой клавиши"

	bool AllocVarTable;
	TVarTable *locVarTable;

	void Init(TVarTable *tbl);
};

struct MacroPanelSelect
{
	int Action;
	DWORD ActionFlags;
	int Mode;
	int64_t Index;
	TVar *Item;
};

// --- misc 2

enum MACROMODEAREA
{
	MACRO_FUNCS  = -3,
	MACRO_CONSTS = -2,
	MACRO_VARS   = -1,

	// see also plugin.hpp # FARMACROAREA
	MACRO_OTHER          = 0,		// Режим копирования текста с экрана, вертикальные меню
	MACRO_SHELL          = 1,		// Файловые панели
	MACRO_VIEWER         = 2,		// Внутренняя программа просмотра
	MACRO_EDITOR         = 3,		// Редактор
	MACRO_DIALOG         = 4,		// Диалоги
	MACRO_SEARCH         = 5,		// Быстрый поиск в панелях
	MACRO_DISKS          = 6,		// Меню выбора дисков
	MACRO_MAINMENU       = 7,		// Основное меню
	MACRO_MENU           = 8,		// Прочие меню
	MACRO_HELP           = 9,		// Система помощи
	MACRO_INFOPANEL      = 10,		// Информационная панель
	MACRO_QVIEWPANEL     = 11,		// Панель быстрого просмотра
	MACRO_TREEPANEL      = 12,		// Панель дерева папок
	MACRO_FINDFOLDER     = 13,		// Поиск папок
	MACRO_USERMENU       = 14,		// Меню пользователя
	MACRO_AUTOCOMPLETION = 15,		// Список автодополнения

	MACRO_COMMON,					// ВЕЗДЕ! - должен быть предпоследним, т.к. приоритет самый низший !!!
	MACRO_LAST						// Должен быть всегда последним! Используется в циклах
};


// --- misc 3

enum MACROFLAGS_MFLAGS
{
	MFLAGS_MODEMASK = 0x000000FF,				// маска для выделения области действия (области начала исполнения) макроса

	MFLAGS_DISABLEOUTPUT       = 0x00000100,	// подавить обновление экрана во время выполнения макроса
	MFLAGS_NOSENDKEYSTOPLUGINS = 0x00000200,	// НЕ передавать плагинам клавиши во время записи/воспроизведения макроса
	MFLAGS_RUNAFTERFARSTARTED  = 0x00000400,	// этот макрос уже запускался при старте ФАРа
	MFLAGS_RUNAFTERFARSTART    = 0x00000800,	// этот макрос запускается при старте ФАРа

	MFLAGS_EMPTYCOMMANDLINE    = 0x00001000,	// запускать, если командная линия пуста
	MFLAGS_NOTEMPTYCOMMANDLINE = 0x00002000,	// запускать, если командная линия не пуста
	MFLAGS_EDITSELECTION       = 0x00004000,	// запускать, если есть выделение в редакторе
	MFLAGS_EDITNOSELECTION     = 0x00008000,	// запускать, если есть нет выделения в редакторе

	MFLAGS_SELECTION       = 0x00010000,		// активная:  запускать, если есть выделение
	MFLAGS_PSELECTION      = 0x00020000,		// пассивная: запускать, если есть выделение
	MFLAGS_NOSELECTION     = 0x00040000,		// активная:  запускать, если есть нет выделения
	MFLAGS_PNOSELECTION    = 0x00080000,		// пассивная: запускать, если есть нет выделения
	MFLAGS_NOFILEPANELS    = 0x00100000,		// активная:  запускать, если это плагиновая панель
	MFLAGS_PNOFILEPANELS   = 0x00200000,		// пассивная: запускать, если это плагиновая панель
	MFLAGS_NOPLUGINPANELS  = 0x00400000,		// активная:  запускать, если это файловая панель
	MFLAGS_PNOPLUGINPANELS = 0x00800000,		// пассивная: запускать, если это файловая панель
	MFLAGS_NOFOLDERS       = 0x01000000,		// активная:  запускать, если текущий объект "файл"
	MFLAGS_PNOFOLDERS      = 0x02000000,		// пассивная: запускать, если текущий объект "файл"
	MFLAGS_NOFILES         = 0x04000000,		// активная:  запускать, если текущий объект "папка"
	MFLAGS_PNOFILES        = 0x08000000,		// пассивная: запускать, если текущий объект "папка"

	MFLAGS_NEEDSAVEMACRO = 0x40000000,			// необходимо этот макрос запомнить
	MFLAGS_DISABLEMACRO  = 0x80000000,			// этот макрос отключен
};


// --- Classes

class KeyMacro
{

public:
	KeyMacro();
	~KeyMacro();

public:
	uint32_t ProcessKey(uint32_t Key);
	int GetKey();
	int PeekKey();
	bool IsOpCode(DWORD p);
	bool CheckWaitKeyFunc();

	int PushState(bool CopyLocalVars = FALSE);
	int PopState();
	int GetLevelState();

	int IsRecording();
	int IsExecuting();
	int IsExecutingLastKey();
	int IsDsableOutput();
	void SetMode(int Mode);
	int GetMode();

	void DropProcess();

	void RunStartMacro();

	// Поместить временное строковое представление макроса
	int PostNewMacro(const wchar_t *PlainText, DWORD Flags = 0, DWORD AKey = 0, BOOL onlyCheck = FALSE);
	// Поместить временный рекорд (бинарное представление)
	int PostNewMacro(struct MacroRecord *MRec, BOOL NeedAddSendFlag = 0, BOOL IsPluginSend = FALSE);

	int LoadMacros(BOOL InitedRAM = TRUE, BOOL LoadAll = TRUE);
	void SaveMacros(BOOL AllSaved = TRUE);

	int GetStartIndex(int Mode);
	// Функция получения индекса нужного макроса в массиве
	int GetIndex(uint32_t Key, int Mode, bool UseCommon = true);
	// получение размера, занимаемого указанным макросом
	int GetRecordSize(int Key, int Mode);

	bool GetPlainText(FARString &Dest);
	int GetPlainTextSize();

	void SetRedrawEditor(int Sets);

	void RestartAutoMacro(int Mode);

	// получить данные о макросе (возвращает статус)
	int GetCurRecord(struct MacroRecord *RBuf = nullptr, int *KeyPos = nullptr);
	// проверить флаги текущего исполняемого макроса.
	BOOL CheckCurMacroFlags(DWORD Flags);

	static const wchar_t *GetSubKey(int Mode);
	static int GetSubKey(const wchar_t *Mode);
	static int
	GetMacroKeyInfo(bool FromReg, int Mode, int Pos, FARString &strKeyName, FARString &strDescription);
	static wchar_t *MkTextSequence(DWORD *Buffer, int BufferSize, const wchar_t *Src = nullptr);
	// из строкового представления макроса сделать MacroRecord
	int ParseMacroString(struct MacroRecord *CurMacro, const wchar_t *BufPtr, BOOL onlyCheck = FALSE);
	BOOL GetMacroParseError(DWORD *ErrCode, COORD *ErrPos, FARString *ErrSrc);
	BOOL GetMacroParseError(FARString *Err1, FARString *Err2, FARString *Err3, FARString *Err4);

	static void SetMacroConst(const wchar_t *ConstName, const TVar Value);
	static DWORD GetNewOpCode();

	static size_t GetCountMacroFunction();
	static const TMacroFunction *GetMacroFunction(size_t Index);
	static void RegisterMacroIntFunction();
	static TMacroFunction *RegisterMacroFunction(const TMacroFunction *tmfunc);
	static bool UnregMacroFunction(size_t Index);
};


class ChangeMacroMode
{
public:
	ChangeMacroMode(int NewMode);
	~ChangeMacroMode();
};


void initMacroVarTable(int global);
void doneMacroVarTable(int global);
const wchar_t *eStackAsString(int Pos = 0);
