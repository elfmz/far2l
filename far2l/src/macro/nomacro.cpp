/*
nomacro.hpp

Заглушка для сборки без макроподсистемы, минимально необходимая,
чтобы Far собирался и работал.

*/

#include "headers.hpp"
#include "lockscrn.hpp"

#include "tvar.hpp"
#include "macroopcode.hpp"
#include "chgmmode.hpp"
#include "macro.hpp"

KeyMacro::KeyMacro() {}
KeyMacro::~KeyMacro() {}
int KeyMacro::ProcessKey(FarKey Key) { return 0; }
FarKey KeyMacro::GetKey() { return 0; }
FarKey KeyMacro::PeekKey() { return 0; }
bool KeyMacro::CheckWaitKeyFunc() { return false; }
int KeyMacro::IsExecutingLastKey() { return 0; }
void KeyMacro::RunStartMacro() {}
int KeyMacro::PostNewMacro(const wchar_t *PlainText, DWORD Flags, DWORD AKey, BOOL onlyCheck) { return 0; }
int KeyMacro::PostNewMacro(MacroRecord *MRec, BOOL NeedAddSendFlag, BOOL IsPluginSend) { return 0; }
int KeyMacro::LoadMacros(BOOL InitedRAM, BOOL LoadAll) { return 0; }
void KeyMacro::SaveMacros(BOOL AllSaved) {}
int KeyMacro::GetIndex(uint32_t Key, int Mode, bool UseCommon) { return 0; }
BOOL KeyMacro::CheckCurMacroFlags(DWORD Flags) { return FALSE; }
int KeyMacro::GetCurRecord(struct MacroRecord *RBuf, int *KeyPos) { return 0; }
const wchar_t *KeyMacro::GetSubKey(int Mode) { return L""; }
int KeyMacro::GetSubKey(const wchar_t *Mode) { return 0; }
int KeyMacro::GetMacroKeyInfo(bool FromReg, int Mode, int Pos, FARString &strKeyName, FARString &strDescription)
	{ return 0; }
int KeyMacro::ParseMacroString(MacroRecord *CurMacro, const wchar_t *BufPtr, BOOL onlyCheck) { return 0; }
BOOL KeyMacro::GetMacroParseError(DWORD *ErrCode, COORD *ErrPos, FARString *ErrSrc) { return FALSE; }
BOOL KeyMacro::GetMacroParseError(FARString *Err1, FARString *Err2, FARString *Err3, FARString *Err4)
	{ return FALSE; }
void KeyMacro::SetMacroConst(const wchar_t *ConstName, const TVar Value) {}

ChangeMacroMode::ChangeMacroMode(int NewMode) {}
ChangeMacroMode::~ChangeMacroMode() {}

const wchar_t *eStackAsString(int Pos) { return L""; }
void initMacroVarTable(int global) {}
void doneMacroVarTable(int global) {}

#ifndef FAR2TVAR
const wchar_t *TVar::s() const { return L""; }
TVar::~TVar() {}
TVar::TVar(int64_t v): vType(vtInteger), inum(v), dnum(0.0), str(nullptr) {}
#endif
