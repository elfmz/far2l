#include "headers.hpp"
#include "lockscrn.hpp"

#include "tvar.hpp"
#include "macroopcode.hpp"

#include "nomacro.hpp"

// ---

// Constructors and Destructor
KeyMacro::KeyMacro() {}
KeyMacro::~KeyMacro() {}

// Public Methods
uint32_t KeyMacro::ProcessKey(uint32_t Key) { 
    return 0; 
}

int KeyMacro::GetKey() { 
    return 0; 
}

int KeyMacro::PeekKey() { 
    return 0; 
}

bool KeyMacro::IsOpCode(DWORD p) { 
    return false; 
}

bool KeyMacro::CheckWaitKeyFunc() { 
    return false; 
}

int KeyMacro::PushState(bool CopyLocalVars) { 
    return 0; 
}

int KeyMacro::PopState() { 
    return 0; 
}

int KeyMacro::GetLevelState() { 
    return 0; // Replace with the actual value of CurPCStack
}

int KeyMacro::IsRecording() { 
    return 0; // Replace with the actual value of Recording
}

int KeyMacro::IsExecuting() { 
    return 0; // Replace with the actual value of Work.Executing
}

int KeyMacro::IsExecutingLastKey() { 
    return 0; 
}

int KeyMacro::IsDsableOutput() { 
    return 0; // Call CheckCurMacroFlags(MFLAGS_DISABLEOUTPUT) here
}

void KeyMacro::SetMode(int Mode) { 
    // Assign Mode here
}

int KeyMacro::GetMode() { 
    return 0; // Replace with the actual value of Mode
}

void KeyMacro::DropProcess() {
    // Implementation here
}

void KeyMacro::RunStartMacro() {
    // Implementation here
}

int KeyMacro::PostNewMacro(const wchar_t *PlainText, DWORD Flags, DWORD AKey, BOOL onlyCheck) {
    return 0;
}

int KeyMacro::PostNewMacro(MacroRecord *MRec, BOOL NeedAddSendFlag, BOOL IsPluginSend) {
    return 0;
}

int KeyMacro::LoadMacros(BOOL InitedRAM, BOOL LoadAll) {
    return 0;
}

void KeyMacro::SaveMacros(BOOL AllSaved) {
    // Implementation here
}

int KeyMacro::GetStartIndex(int Mode) {
    return 0; // Replace with IndexMode[Mode < MACRO_LAST - 1 ? Mode : MACRO_LAST - 1][0]
}

int KeyMacro::GetIndex(uint32_t Key, int Mode, bool UseCommon) {
    return 0;
}

int KeyMacro::GetRecordSize(int Key, int Mode) {
    return 0;
}

bool KeyMacro::GetPlainText(FARString &Dest) {
    return false;
}

int KeyMacro::GetPlainTextSize() {
    return 0;
}

void KeyMacro::SetRedrawEditor(int Sets) {
    // Assign Sets to IsRedrawEditor here
}

void KeyMacro::RestartAutoMacro(int Mode) {
    // Implementation here
}

int KeyMacro::GetCurRecord(MacroRecord *RBuf, int *KeyPos) {
    return 0;
}

BOOL KeyMacro::CheckCurMacroFlags(DWORD Flags) {
    return FALSE;
}

// Static Methods
const wchar_t *KeyMacro::GetSubKey(int Mode) {
    return L"";
}

int KeyMacro::GetSubKey(const wchar_t *Mode) {
    return 0;
}

int KeyMacro::GetMacroKeyInfo(bool FromReg, int Mode, int Pos, FARString &strKeyName, FARString &strDescription) {
    return 0;
}

wchar_t *KeyMacro::MkTextSequence(DWORD *Buffer, int BufferSize, const wchar_t *Src) {
    return nullptr;
}

int KeyMacro::ParseMacroString(MacroRecord *CurMacro, const wchar_t *BufPtr, BOOL onlyCheck) {
    return 0;
}

BOOL KeyMacro::GetMacroParseError(DWORD *ErrCode, COORD *ErrPos, FARString *ErrSrc) {
    return FALSE;
}

BOOL KeyMacro::GetMacroParseError(FARString *Err1, FARString *Err2, FARString *Err3, FARString *Err4) {
    return FALSE;
}

void KeyMacro::SetMacroConst(const wchar_t *ConstName, const TVar Value) {
    // Implementation here
}

DWORD KeyMacro::GetNewOpCode() {
    return 0;
}

size_t KeyMacro::GetCountMacroFunction() {
    return 0;
}

const TMacroFunction *KeyMacro::GetMacroFunction(size_t Index) {
    return nullptr;
}

void KeyMacro::RegisterMacroIntFunction() {
    // Implementation here
}

TMacroFunction *KeyMacro::RegisterMacroFunction(const TMacroFunction *tmfunc) {
    return nullptr;
}

bool KeyMacro::UnregMacroFunction(size_t Index) {
    return false;
}

// ---

// Constructor
ChangeMacroMode::ChangeMacroMode(int NewMode) {
    // Do nothing
}

// Destructor
ChangeMacroMode::~ChangeMacroMode() {
    // Do nothing
}

// ---

const wchar_t *eStackAsString(int Pos)
{
	return L"";
}

void initMacroVarTable(int global)
{
}

void doneMacroVarTable(int global)
{
}

// ---

#ifndef FAR2TVAR
const wchar_t *TVar::s() const { return L""; }
TVar::~TVar() {}
TVar::TVar(int64_t v): vType(vtInteger), inum(v), dnum(0.0), str(nullptr) {}
#endif

