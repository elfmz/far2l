/*
nomacro.hpp

Заглушка для сборки без макроподсистемы, минимально необходимая,
чтобы Far собирался и работал.

*/

#include "headers.hpp"
#include "lockscrn.hpp"
#include "tvar.hpp"
#include "macroopcode.hpp"
#include "nomacro.hpp"

KeyMacro::KeyMacro() {}
KeyMacro::~KeyMacro() {}
uint32_t KeyMacro::ProcessKey(uint32_t Key) { return 0; }
int KeyMacro::GetKey() { return 0; }
int KeyMacro::PeekKey() { return 0; }
bool KeyMacro::IsOpCode(DWORD p) { return false; }
bool KeyMacro::CheckWaitKeyFunc() { return false; }
int KeyMacro::PushState(bool CopyLocalVars) { return 0; }
int KeyMacro::PopState() { return 0; }
int KeyMacro::GetLevelState() { return 0; }
int KeyMacro::IsRecording() { return 0; }
int KeyMacro::IsExecuting() { return 0; }
int KeyMacro::IsExecutingLastKey() { return 0; }
int KeyMacro::IsDsableOutput() { return 0; }
void KeyMacro::SetMode(int Mode) {}
int KeyMacro::GetMode() { return 0; }
void KeyMacro::DropProcess() {}
void KeyMacro::RunStartMacro() {}
int KeyMacro::PostNewMacro(const wchar_t *PlainText, DWORD Flags, DWORD AKey, BOOL onlyCheck) { return 0; }
int KeyMacro::PostNewMacro(MacroRecord *MRec, BOOL NeedAddSendFlag, BOOL IsPluginSend) { return 0; }
int KeyMacro::LoadMacros(BOOL InitedRAM, BOOL LoadAll) { return 0; }
void KeyMacro::SaveMacros(BOOL AllSaved) {}
int KeyMacro::GetStartIndex(int Mode) { return 0; }
int KeyMacro::GetIndex(uint32_t Key, int Mode, bool UseCommon) { return 0; }
int KeyMacro::GetRecordSize(int Key, int Mode) { return 0; }
bool KeyMacro::GetPlainText(FARString &Dest) { return false; }
int KeyMacro::GetPlainTextSize() { return 0; }
void KeyMacro::SetRedrawEditor(int Sets) {}
void KeyMacro::RestartAutoMacro(int Mode) {}
int KeyMacro::GetCurRecord(MacroRecord *RBuf, int *KeyPos) { return 0; }
BOOL KeyMacro::CheckCurMacroFlags(DWORD Flags) { return FALSE; }
const wchar_t *KeyMacro::GetSubKey(int Mode) { return L""; }
int KeyMacro::GetSubKey(const wchar_t *Mode) { return 0; }
int KeyMacro::GetMacroKeyInfo(bool FromReg, int Mode, int Pos, FARString &strKeyName, FARString &strDescription)
	{ return 0; }
wchar_t *KeyMacro::MkTextSequence(DWORD *Buffer, int BufferSize, const wchar_t *Src) { return nullptr; }
int KeyMacro::ParseMacroString(MacroRecord *CurMacro, const wchar_t *BufPtr, BOOL onlyCheck) { return 0; }
BOOL KeyMacro::GetMacroParseError(DWORD *ErrCode, COORD *ErrPos, FARString *ErrSrc) { return FALSE; }
BOOL KeyMacro::GetMacroParseError(FARString *Err1, FARString *Err2, FARString *Err3, FARString *Err4)
	{ return FALSE; }
void KeyMacro::SetMacroConst(const wchar_t *ConstName, const TVar Value) {}
DWORD KeyMacro::GetNewOpCode() { return 0; }
size_t KeyMacro::GetCountMacroFunction() { return 0; }
const TMacroFunction *KeyMacro::GetMacroFunction(size_t Index) { return nullptr; }
void KeyMacro::RegisterMacroIntFunction() {}
TMacroFunction *KeyMacro::RegisterMacroFunction(const TMacroFunction *tmfunc) { return nullptr; }
bool KeyMacro::UnregMacroFunction(size_t Index) { return false; }

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
