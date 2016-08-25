#pragma once

#include "FileMasks.h"
#include "RemoteFiles.h"

// When adding new options, mind TCopyParamType::GetLogStr()
enum TOperationSide
{
  osLocal,
  osRemote,
  osCurrent,
};

enum TFileNameCase
{
  ncNoChange,
  ncUpperCase,
  ncLowerCase,
  ncFirstUpperCase,
  ncLowerCaseShort,
};

// TScript::OptionProc depend on the order
enum TTransferMode
{
  tmBinary,
  tmAscii,
  tmAutomatic,
};

enum TResumeSupport
{
  rsOn,
  rsSmart,
  rsOff,
};

class THierarchicalStorage;
const int cpaIncludeMaskOnly = 0x01;
const int cpaNoTransferMode  = 0x02;
const int cpaNoIncludeMask   = 0x04;
const int cpaNoClearArchive  = 0x08;
const int cpaNoPreserveTime  = 0x10;
const int cpaNoRights        = 0x20;
const int cpaNoPreserveReadOnly = 0x40;
const int cpaNoIgnorePermErrors = 0x80;
const int cpaNoNewerOnly        = 0x100;
const int cpaNoRemoveCtrlZ      = 0x200;
const int cpaNoRemoveBOM        = 0x400;
const int cpaNoPreserveTimeDirs = 0x800;
const int cpaNoResumeSupport    = 0x1000;
//---------------------------------------------------------------------------
struct TUsableCopyParamAttrs
{
  int General;
  int Upload;
  int Download;
};

class TCopyParamType : public TObject
{
NB_DECLARE_CLASS(TCopyParamType)
private:
  TFileMasks FAsciiFileMask;
  TFileNameCase FFileNameCase;
  bool FPreserveReadOnly;
  bool FPreserveTime;
  bool FPreserveTimeDirs;
  TRights FRights;
  TTransferMode FTransferMode;
  bool FAddXToDirectories;
  bool FPreserveRights;
  bool FIgnorePermErrors;
  TResumeSupport FResumeSupport;
  int64_t FResumeThreshold;
  wchar_t FInvalidCharsReplacement;
  UnicodeString FLocalInvalidChars;
  UnicodeString FTokenizibleChars;
  bool FCalculateSize;
  UnicodeString FFileMask;
  TFileMasks FIncludeFileMask;
  std::unique_ptr<TStringList> FTransferSkipList;
  UnicodeString FTransferResumeFile;
  bool FClearArchive;
  bool FRemoveCtrlZ;
  bool FRemoveBOM;
  uintptr_t FCPSLimit;
  bool FNewerOnly;

public:
  static const wchar_t TokenPrefix = L'%';
  static const wchar_t NoReplacement = wchar_t(0);
  static const wchar_t TokenReplacement = wchar_t(1);

public:
  void SetLocalInvalidChars(const UnicodeString & Value);
  bool GetReplaceInvalidChars() const;
  void SetReplaceInvalidChars(bool Value);
  UnicodeString RestoreChars(const UnicodeString & AFileName) const;
  void DoGetInfoStr(const UnicodeString & Separator, intptr_t Attrs,
    UnicodeString & Result, bool & SomeAttrIncluded,
    const UnicodeString & Link, UnicodeString & ScriptArgs, bool & NoScriptArgs,
    /*TAssemblyLanguage Language, UnicodeString & AssemblyCode, */bool & NoCodeProperties) const;
  TStrings * GetTransferSkipList() const;
  void SetTransferSkipList(TStrings * Value);
  UnicodeString GetTransferResumeFile() const { return FTransferResumeFile; }
  void SetTransferResumeFile(const UnicodeString & Value) { FTransferResumeFile = Value; }

public:
  TCopyParamType();
  TCopyParamType(const TCopyParamType & Source);
  virtual ~TCopyParamType();
  TCopyParamType & operator =(const TCopyParamType & rhs);
  virtual void Assign(const TCopyParamType * Source);
  virtual void Default();
  UnicodeString ChangeFileName(const UnicodeString & AFileName,
    TOperationSide Side, bool FirstLevel) const;
  DWORD LocalFileAttrs(const TRights & Rights) const;
  TRights RemoteFileRights(uintptr_t Attrs) const;
  bool UseAsciiTransfer(const UnicodeString & AFileName, TOperationSide Side,
    const TFileMasks::TParams & Params) const;
  bool AllowResume(int64_t Size) const;
  bool ResumeTransfer(const UnicodeString & AFileName) const;
  UnicodeString ValidLocalFileName(const UnicodeString & AFileName) const;
  UnicodeString ValidLocalPath(const UnicodeString & APath) const;
  bool AllowAnyTransfer() const;
  bool AllowTransfer(const UnicodeString & AFileName, TOperationSide Side,
    bool Directory, const TFileMasks::TParams & Params) const;
  bool SkipTransfer(const UnicodeString & AFileName, bool Directory) const;

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;
  UnicodeString GetInfoStr(const UnicodeString & Separator, intptr_t Options) const;
  bool AnyUsableCopyParam(intptr_t Attrs) const;
  UnicodeString GenerateTransferCommandArgs(
    int Attrs, const UnicodeString & Link, bool & NoScriptArgs) const;
  //UnicodeString GenerateAssemblyCode(TAssemblyLanguage Language, int Attrs, bool & NoCodeProperties) const;

  bool operator==(const TCopyParamType & rhp) const;

  /*__property TFileMasks AsciiFileMask = { read = FAsciiFileMask, write = FAsciiFileMask };
  __property TFileNameCase FileNameCase = { read = FFileNameCase, write = FFileNameCase };
  __property bool PreserveReadOnly = { read = FPreserveReadOnly, write = FPreserveReadOnly };
  __property bool PreserveTime = { read = FPreserveTime, write = FPreserveTime };
  __property bool PreserveTimeDirs = { read = FPreserveTimeDirs, write = FPreserveTimeDirs };
  __property TRights Rights = { read = FRights, write = FRights };
  __property TTransferMode TransferMode = { read = FTransferMode, write = FTransferMode };
  __property UnicodeString LogStr  = { read=GetLogStr };
  __property bool AddXToDirectories  = { read=FAddXToDirectories, write=FAddXToDirectories };
  __property bool PreserveRights = { read = FPreserveRights, write = FPreserveRights };
  __property bool IgnorePermErrors = { read = FIgnorePermErrors, write = FIgnorePermErrors };
  __property TResumeSupport ResumeSupport = { read = FResumeSupport, write = FResumeSupport };
  __property __int64 ResumeThreshold = { read = FResumeThreshold, write = FResumeThreshold };
  __property wchar_t InvalidCharsReplacement = { read = FInvalidCharsReplacement, write = FInvalidCharsReplacement };
  __property bool ReplaceInvalidChars = { read = GetReplaceInvalidChars, write = SetReplaceInvalidChars };
  __property UnicodeString LocalInvalidChars = { read = FLocalInvalidChars, write = SetLocalInvalidChars };
  __property bool CalculateSize = { read = FCalculateSize, write = FCalculateSize };
  __property UnicodeString FileMask = { read = FFileMask, write = FFileMask };
  __property TFileMasks IncludeFileMask = { read = FIncludeFileMask, write = FIncludeFileMask };
  __property TStrings * TransferSkipList = { read = GetTransferSkipList, write = SetTransferSkipList };
  __property UnicodeString TransferResumeFile = { read = FTransferResumeFile, write = FTransferResumeFile };
  __property bool ClearArchive = { read = FClearArchive, write = FClearArchive };
  __property bool RemoveCtrlZ = { read = FRemoveCtrlZ, write = FRemoveCtrlZ };
  __property bool RemoveBOM = { read = FRemoveBOM, write = FRemoveBOM };
  __property unsigned long CPSLimit = { read = FCPSLimit, write = FCPSLimit };
  __property bool NewerOnly = { read = FNewerOnly, write = FNewerOnly };*/
  const TFileMasks & GetAsciiFileMask() const { return FAsciiFileMask; }
  TFileMasks & GetAsciiFileMask() { return FAsciiFileMask; }
  void SetAsciiFileMask(const TFileMasks & Value) { FAsciiFileMask = Value; }
  const TFileNameCase & GetFileNameCase() const { return FFileNameCase; }
  void SetFileNameCase(TFileNameCase Value) { FFileNameCase = Value; }
  bool GetPreserveReadOnly() const { return FPreserveReadOnly; }
  void SetPreserveReadOnly(bool Value) { FPreserveReadOnly = Value; }
  bool GetPreserveTime() const { return FPreserveTime; }
  void SetPreserveTime(bool Value) { FPreserveTime = Value; }
  bool GetPreserveTimeDirs() const { return FPreserveTimeDirs; }
  void SetPreserveTimeDirs(bool Value) { FPreserveTimeDirs = Value; }
  const TRights & GetRights() const { return FRights; }
  TRights & GetRights() { return FRights; }
  void SetRights(const TRights & Value) { FRights.Assign(&Value); }
  TTransferMode GetTransferMode() const { return FTransferMode; }
  void SetTransferMode(TTransferMode Value) { FTransferMode = Value; }
  UnicodeString GetLogStr() const;
  bool GetAddXToDirectories() const { return FAddXToDirectories; }
  void SetAddXToDirectories(bool Value) { FAddXToDirectories = Value; }
  bool GetPreserveRights() const { return FPreserveRights; }
  void SetPreserveRights(bool Value) { FPreserveRights = Value; }
  bool GetIgnorePermErrors() const { return FIgnorePermErrors; }
  void SetIgnorePermErrors(bool Value) { FIgnorePermErrors = Value; }
  TResumeSupport GetResumeSupport() const { return FResumeSupport; }
  void SetResumeSupport(TResumeSupport Value) { FResumeSupport = Value; }
  int64_t GetResumeThreshold() const { return FResumeThreshold; }
  void SetResumeThreshold(int64_t Value) { FResumeThreshold = Value; }
  wchar_t GetInvalidCharsReplacement() const { return FInvalidCharsReplacement; }
  void SetInvalidCharsReplacement(wchar_t Value) { FInvalidCharsReplacement = Value; }
  UnicodeString GetLocalInvalidChars() const { return FLocalInvalidChars; }
  bool GetCalculateSize() const { return FCalculateSize; }
  void SetCalculateSize(bool Value) { FCalculateSize = Value; }
  UnicodeString GetFileMask() const { return FFileMask; }
  void SetFileMask(const UnicodeString & Value) { FFileMask = Value; }
  const TFileMasks & GetIncludeFileMask() const { return FIncludeFileMask; }
  TFileMasks & GetIncludeFileMask() { return FIncludeFileMask; }
  void SetIncludeFileMask(const TFileMasks & Value) { FIncludeFileMask = Value; }
  bool GetClearArchive() const { return FClearArchive; }
  void SetClearArchive(bool Value) { FClearArchive = Value; }
  bool GetRemoveCtrlZ() const { return FRemoveCtrlZ; }
  void SetRemoveCtrlZ(bool Value) { FRemoveCtrlZ = Value; }
  bool GetRemoveBOM() const { return FRemoveBOM; }
  void SetRemoveBOM(bool Value) { FRemoveBOM = Value; }
  uintptr_t GetCPSLimit() const { return FCPSLimit; }
  void SetCPSLimit(uintptr_t Value) { FCPSLimit = Value; }
  bool GetNewerOnly() const { return FNewerOnly; }
  void SetNewerOnly(bool Value) { FNewerOnly = Value; }

};

uintptr_t GetSpeedLimit(const UnicodeString & Text);
UnicodeString SetSpeedLimit(uintptr_t Limit);
void CopySpeedLimits(TStrings * Source, TStrings * Dest);
