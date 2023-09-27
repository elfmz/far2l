#pragma once

/*
nomacro.hpp

Заглушка для сборки без макроподсистемы, минимально необходимая,
чтобы Far собирался и работал.

*/

class Panel;

struct TMacroFunction;
typedef bool (*INTMACROFUNC)(const TMacroFunction *);

enum INTMF_FLAGS
{
    IMFF_UNLOCKSCREEN = 0x00000001,
    IMFF_DISABLEINTINPUT = 0x00000002,
};

struct TMacroFunction
{
    const wchar_t *Name;
    int nParam;
    int oParam;
    TMacroOpCode Code;
    const wchar_t *fnGUID;

    int BufferSize;
    DWORD *Buffer;
    const wchar_t *Syntax;

    DWORD IntFlags;
    INTMACROFUNC Func;
};

struct MacroRecord
{
    DWORD Flags;
    uint32_t Key;
    int BufferSize;
    DWORD *Buffer;
    wchar_t *Src;
    wchar_t *Description;
    DWORD Reserved[2];
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
    struct MacroRecord *MacroWORK;
    INPUT_RECORD cRec;

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

enum MACROMODEAREA
{
    MACRO_FUNCS = -3,
    MACRO_CONSTS = -2,
    MACRO_VARS = -1,

    MACRO_OTHER = 0,
    MACRO_SHELL = 1,
    MACRO_VIEWER = 2,
    MACRO_EDITOR = 3,
    MACRO_DIALOG = 4,
    MACRO_SEARCH = 5,
    MACRO_DISKS = 6,
    MACRO_MAINMENU = 7,
    MACRO_MENU = 8,
    MACRO_HELP = 9,
    MACRO_INFOPANEL = 10,
    MACRO_QVIEWPANEL = 11,
    MACRO_TREEPANEL = 12,
    MACRO_FINDFOLDER = 13,
    MACRO_USERMENU = 14,
    MACRO_AUTOCOMPLETION = 15,

    MACRO_COMMON,
    MACRO_LAST
};

enum MACROFLAGS_MFLAGS
{
    MFLAGS_MODEMASK = 0x000000FF,

    MFLAGS_DISABLEOUTPUT = 0x00000100,
    MFLAGS_NOSENDKEYSTOPLUGINS = 0x00000200,
    MFLAGS_RUNAFTERFARSTARTED = 0x00000400,
    MFLAGS_RUNAFTERFARSTART = 0x00000800,
    MFLAGS_EMPTYCOMMANDLINE = 0x00001000,
    MFLAGS_NOTEMPTYCOMMANDLINE = 0x00002000,
    MFLAGS_EDITSELECTION = 0x00004000,
    MFLAGS_EDITNOSELECTION = 0x00008000,
    MFLAGS_SELECTION = 0x00010000,
    MFLAGS_PSELECTION = 0x00020000,
    MFLAGS_NOSELECTION = 0x00040000,
    MFLAGS_PNOSELECTION = 0x00080000,
    MFLAGS_NOFILEPANELS = 0x00100000,
    MFLAGS_PNOFILEPANELS = 0x00200000,
    MFLAGS_NOPLUGINPANELS = 0x00400000,
    MFLAGS_PNOPLUGINPANELS = 0x00800000,
    MFLAGS_NOFOLDERS = 0x01000000,
    MFLAGS_PNOFOLDERS = 0x02000000,
    MFLAGS_NOFILES = 0x04000000,
    MFLAGS_PNOFILES = 0x08000000,
    MFLAGS_NEEDSAVEMACRO = 0x40000000,
    MFLAGS_DISABLEMACRO = 0x80000000,
};

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

    int PostNewMacro(const wchar_t *PlainText, DWORD Flags = 0, DWORD AKey = 0, BOOL onlyCheck = FALSE);
    int PostNewMacro(struct MacroRecord *MRec, BOOL NeedAddSendFlag = 0, BOOL IsPluginSend = FALSE);

    int LoadMacros(BOOL InitedRAM = TRUE, BOOL LoadAll = TRUE);
    void SaveMacros(BOOL AllSaved = TRUE);

    int GetStartIndex(int Mode);
    int GetIndex(uint32_t Key, int Mode, bool UseCommon = true);
    int GetRecordSize(int Key, int Mode);

    bool GetPlainText(FARString &Dest);
    int GetPlainTextSize();

    void SetRedrawEditor(int Sets);

    void RestartAutoMacro(int Mode);

    int GetCurRecord(struct MacroRecord *RBuf = nullptr, int *KeyPos = nullptr);
    BOOL CheckCurMacroFlags(DWORD Flags);

    static const wchar_t *GetSubKey(int Mode);
    static int GetSubKey(const wchar_t *Mode);
    static int GetMacroKeyInfo(bool FromReg, int Mode, int Pos, FARString &strKeyName, FARString &strDescription);
    static wchar_t *MkTextSequence(DWORD *Buffer, int BufferSize, const wchar_t *Src = nullptr);
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
