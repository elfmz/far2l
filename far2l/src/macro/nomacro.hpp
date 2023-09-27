#pragma once

/*
nomacro.hpp

Заглушка для сборки без макроподсистемы, минимально необходимая,
чтобы Far собирался и работал.

*/

class KeyMacro
{

public:
	KeyMacro();
	~KeyMacro();

	uint32_t ProcessKey(uint32_t Key);
    int GetKey();
	int PeekKey();
	bool CheckWaitKeyFunc();
    int IsExecutingLastKey();
    void RunStartMacro();
    int PostNewMacro(const wchar_t *PlainText, DWORD Flags = 0, DWORD AKey = 0, BOOL onlyCheck = FALSE);
    int PostNewMacro(struct MacroRecord *MRec, BOOL NeedAddSendFlag = 0, BOOL IsPluginSend = FALSE);
    int LoadMacros(BOOL InitedRAM = TRUE, BOOL LoadAll = TRUE);
	void SaveMacros(BOOL AllSaved = TRUE);
	int GetIndex(uint32_t Key, int Mode, bool UseCommon = true);
    BOOL CheckCurMacroFlags(DWORD Flags);
	int GetCurRecord(struct MacroRecord *RBuf = nullptr, int *KeyPos = nullptr);
    static const wchar_t *GetSubKey(int Mode);
    static int GetSubKey(const wchar_t *Mode);
    static int GetMacroKeyInfo(bool FromReg, int Mode, int Pos, FARString &strKeyName, FARString &strDescription);
	int ParseMacroString(struct MacroRecord *CurMacro, const wchar_t *BufPtr, BOOL onlyCheck = FALSE);
    BOOL GetMacroParseError(DWORD *ErrCode, COORD *ErrPos, FARString *ErrSrc);
    BOOL GetMacroParseError(FARString *Err1, FARString *Err2, FARString *Err3, FARString *Err4);
	static void SetMacroConst(const wchar_t *ConstName, const TVar Value);
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
