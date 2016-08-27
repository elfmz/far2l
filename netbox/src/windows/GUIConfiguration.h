#pragma once

#include "Configuration.h"
#include "CopyParam.h"

#define CONST_INVALID_CHARS L"/" WGOOD_SLASH "[]"

class TGUIConfiguration;
class TStoredSessionList;

enum TLogView
{
  lvNone,
  lvWindow,
  pvPanel
};

enum TInterface
{
  ifCommander,
  ifExplorer
};

extern const intptr_t ccLocal;
extern const intptr_t ccShowResults;
extern const intptr_t ccCopyResults;
extern const intptr_t ccSet;

const int soRecurse =        0x01;
const int soSynchronize =    0x02;
const int soSynchronizeAsk = 0x04;
const int soContinueOnError = 0x08;

class TGUICopyParamType : public TCopyParamType
{
NB_DECLARE_CLASS(TGUICopyParamType)
public:
  TGUICopyParamType();
  TGUICopyParamType(const TCopyParamType & Source);
  explicit TGUICopyParamType(const TGUICopyParamType & Source);
  virtual ~TGUICopyParamType() {}

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage);

  virtual void Default();
  virtual void Assign(const TCopyParamType * Source);
  TGUICopyParamType & operator =(const TGUICopyParamType & rhp);
  TGUICopyParamType & operator =(const TCopyParamType & rhp);

/*
  __property bool Queue = { read = FQueue, write = FQueue };
  __property bool QueueNoConfirmation = { read = FQueueNoConfirmation, write = FQueueNoConfirmation };
  __property bool QueueIndividually = { read = FQueueIndividually, write = FQueueIndividually };
*/
  bool GetQueue() const { return FQueue; }
  void SetQueue(bool Value) { FQueue = Value; }
  bool GetQueueNoConfirmation() const { return FQueueNoConfirmation; }
  void SetQueueNoConfirmation(bool Value) { FQueueNoConfirmation = Value; }
  bool GetQueueIndividually() const { return FQueueIndividually; }
  void SetQueueIndividually(bool Value) { FQueueIndividually = Value; }

protected:
  void GUIDefault();
  void GUIAssign(const TGUICopyParamType * Source);

private:
  bool FQueue;
  bool FQueueNoConfirmation;
  bool FQueueIndividually;
};

struct TCopyParamRuleData : public TObject
{
  UnicodeString HostName;
  UnicodeString UserName;
  UnicodeString RemoteDirectory;
  UnicodeString LocalDirectory;

  void Default();
};

class TCopyParamRule : public TObject
{
NB_DECLARE_CLASS(TCopyParamRule)
public:
  explicit TCopyParamRule();
  explicit TCopyParamRule(const TCopyParamRuleData & Data);
  explicit TCopyParamRule(const TCopyParamRule & Source);

  bool Matches(const TCopyParamRuleData & Value) const;
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  UnicodeString GetInfoStr(const UnicodeString & Separator) const;

  bool operator ==(const TCopyParamRule & rhp) const;

  TCopyParamRuleData GetData() const { return FData; }
  void SetData(const TCopyParamRuleData & Value);
  bool GetEmpty() const;

public:
  TCopyParamRule & operator=(const TCopyParamRule & other);

private:
  TCopyParamRuleData FData;

  inline bool Match(const UnicodeString & Mask,
    const UnicodeString & Value, bool Path, bool Local = true) const;
};

class TCopyParamList : public TObject
{
friend class TGUIConfiguration;
public:
  explicit TCopyParamList();
  explicit TCopyParamList(const TCopyParamList & other);

  virtual ~TCopyParamList();
  intptr_t Find(const TCopyParamRuleData & Value) const;

  void Load(THierarchicalStorage * Storage, intptr_t Count);
  void Save(THierarchicalStorage * Storage) const;

  static void ValidateName(const UnicodeString & Name);

  TCopyParamList & operator=(const TCopyParamList & rhl);
  bool operator==(const TCopyParamList & rhl) const;

  void Clear();
  void Add(const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Insert(intptr_t Index, const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Change(intptr_t Index, const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Move(intptr_t CurIndex, intptr_t NewIndex);
  void Delete(intptr_t Index);
  intptr_t IndexOfName(const UnicodeString & Name) const;

/*
  __property int Count = { read = GetCount };
  __property UnicodeString Names[int Index] = { read = GetName };
  __property const TCopyParamRule * Rules[int Index] = { read = GetRule };
  __property const TCopyParamType * CopyParams[int Index] = { read = GetCopyParam };
  __property bool Modified = { read = FModified };
  __property TStrings * NameList = { read = GetNameList };
  __property bool AnyRule = { read = GetAnyRule };
*/
  intptr_t GetCount() const { return FCopyParams ? FCopyParams->GetCount() : 0; }
  UnicodeString GetName(intptr_t Index) const;
  const TCopyParamRule * GetRule(intptr_t Index) const;
  const TCopyParamType * GetCopyParam(intptr_t Index) const;
  bool GetModified() const { return FModified; }
  TStrings * GetNameList() const;
  bool GetAnyRule() const;

private:
  TList * FRules;
  TList * FCopyParams;
  TStrings * FNames;
  mutable TStrings * FNameList;
  bool FModified;

  void Reset();
  void Modify();
  bool CompareItem(intptr_t Index, const TCopyParamType * CopyParam,
    const TCopyParamRule * Rule) const;
};

class TGUIConfiguration : public TConfiguration
{
NB_DISABLE_COPY(TGUIConfiguration)
NB_DECLARE_CLASS(TGUIConfiguration)
public:
  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  virtual LCID GetLocale() const;
  LCID GetLocaleSafe() const { return GetLocale(); }
  void SetLocale(LCID Value);
  void SetLocaleSafe(LCID Value);
  virtual HINSTANCE LoadNewResourceModule(LCID Locale,
    UnicodeString * AFileName = nullptr);
  HANDLE GetResourceModule();
  // virtual void SetResourceModule(HINSTANCE Instance);
  TStrings * GetLocales();
  LCID InternalLocale() const;
  void FreeResourceModule(HANDLE Instance);
  void SetDefaultCopyParam(const TGUICopyParamType & Value);
  virtual bool GetRememberPassword() const;
  const TCopyParamList * GetCopyParamList();
  void SetCopyParamList(const TCopyParamList * Value);
  static UnicodeString PropertyToKey(const UnicodeString & Property);
  virtual void DefaultLocalized();
  intptr_t GetCopyParamIndex() const;
  const TGUICopyParamType GetCurrentCopyParam() const;
  const TGUICopyParamType GetCopyParamPreset(const UnicodeString & Name) const;
  bool GetHasCopyParamPreset(const UnicodeString & Name) const;
  void SetCopyParamIndex(intptr_t Value);
  void SetCopyParamCurrent(const UnicodeString & Value);
  void SetNewDirectoryProperties(const TRemoteProperties & Value);
  virtual void Saved();
  void SetQueueTransfersLimit(intptr_t Value);
  void SetQueueKeepDoneItems(bool Value);
  void SetQueueKeepDoneItemsFor(intptr_t Value);

public:
  explicit TGUIConfiguration();
  virtual ~TGUIConfiguration();
  virtual void Default();
  virtual void UpdateStaticUsage();

  HANDLE ChangeResourceModule(HANDLE Instance);
  TStoredSessionList * SelectPuttySessionsForImport(TStoredSessionList * Sessions);
  bool AnyPuttySessionForImport(TStoredSessionList * Sessions);
  TStoredSessionList * SelectFilezillaSessionsForImport(TStoredSessionList * Sessions);
  bool AnyFilezillaSessionForImport(TStoredSessionList * Sessions);

  bool GetContinueOnError() const { return FContinueOnError; }
  void SetContinueOnError(bool Value) { FContinueOnError = Value; }
  bool GetConfirmCommandSession() const { return FConfirmCommandSession; }
  void SetConfirmCommandSession(bool Value) { FConfirmCommandSession = Value; }
  intptr_t GetSynchronizeParams() const { return FSynchronizeParams; }
  void SetSynchronizeParams(intptr_t Value) { FSynchronizeParams = Value; }
  intptr_t GetSynchronizeOptions() const { return FSynchronizeOptions; }
  void SetSynchronizeOptions(intptr_t Value) { FSynchronizeOptions = Value; }
  intptr_t GetSynchronizeModeAuto() const { return FSynchronizeModeAuto; }
  void SetSynchronizeModeAuto(intptr_t Value) { FSynchronizeModeAuto = Value; }
  intptr_t GetSynchronizeMode() const { return FSynchronizeMode; }
  void SetSynchronizeMode(intptr_t Value) { FSynchronizeMode = Value; }
  intptr_t GetMaxWatchDirectories() const { return FMaxWatchDirectories; }
  void SetMaxWatchDirectories(intptr_t Value) { FMaxWatchDirectories = Value; }
  intptr_t GetQueueTransfersLimit() const { return FQueueTransfersLimit; }
  bool GetQueueKeepDoneItems() const { return FQueueKeepDoneItems; }
  intptr_t GetQueueKeepDoneItemsFor() const { return FQueueKeepDoneItemsFor; }
  bool GetQueueAutoPopup() const { return FQueueAutoPopup; }
  void SetQueueAutoPopup(bool Value) { FQueueAutoPopup = Value; }
  bool GetSessionRememberPassword() const { return FSessionRememberPassword; }
  void SetSessionRememberPassword(bool Value) { FSessionRememberPassword = Value; }
  UnicodeString GetPuttyPath() const;
  void SetPuttyPath(const UnicodeString & Value);
  UnicodeString GetDefaultPuttyPath() const;
  UnicodeString GetPSftpPath() const;
  void SetPSftpPath(const UnicodeString & Value);
  bool GetPuttyPassword() const { return FPuttyPassword; }
  void SetPuttyPassword(bool Value) { FPuttyPassword = Value; }
  bool GetTelnetForFtpInPutty() const { return FTelnetForFtpInPutty; }
  void SetTelnetForFtpInPutty(bool Value) { FTelnetForFtpInPutty = Value; }
  UnicodeString GetPuttySession() const;
  void SetPuttySession(const UnicodeString & Value);
  TDateTime GetIgnoreCancelBeforeFinish() const { return FIgnoreCancelBeforeFinish; }
  void SetIgnoreCancelBeforeFinish(const TDateTime & Value) { FIgnoreCancelBeforeFinish = Value; }
  TGUICopyParamType & GetDefaultCopyParam() { return FDefaultCopyParam; }
  bool GetBeepOnFinish() const { return FBeepOnFinish; }
  void SetBeepOnFinish(bool Value) { FBeepOnFinish = Value; }
  TDateTime GetBeepOnFinishAfter() const { return FBeepOnFinishAfter; }
  void SetBeepOnFinishAfter(const TDateTime & Value) { FBeepOnFinishAfter = Value; }
  UnicodeString GetCopyParamCurrent() const;
  const TRemoteProperties & GetNewDirectoryProperties() const { return FNewDirectoryProperties; }
  intptr_t GetKeepUpToDateChangeDelay() const { return FKeepUpToDateChangeDelay; }
  void SetKeepUpToDateChangeDelay(intptr_t Value) { FKeepUpToDateChangeDelay = Value; }
  UnicodeString GetChecksumAlg() const;
  void SetChecksumAlg(const UnicodeString & Value);
  intptr_t GetSessionReopenAutoIdle() const { return FSessionReopenAutoIdle; }
  void SetSessionReopenAutoIdle(intptr_t Value) { FSessionReopenAutoIdle = Value; }

protected:
  mutable LCID FLocale;

private:
  TStrings * FLocales;
  UnicodeString FLastLocalesExts;
  bool FContinueOnError;
  bool FConfirmCommandSession;
  UnicodeString FPuttyPath;
  UnicodeString FPSftpPath;
  bool FPuttyPassword;
  bool FTelnetForFtpInPutty;
  UnicodeString FPuttySession;
  intptr_t FSynchronizeParams;
  intptr_t FSynchronizeOptions;
  intptr_t FSynchronizeModeAuto;
  intptr_t FSynchronizeMode;
  intptr_t FMaxWatchDirectories;
  TDateTime FIgnoreCancelBeforeFinish;
  bool FQueueAutoPopup;
  bool FSessionRememberPassword;
  intptr_t FQueueTransfersLimit;
  bool FQueueKeepDoneItems;
  intptr_t FQueueKeepDoneItemsFor;
  TGUICopyParamType FDefaultCopyParam;
  bool FBeepOnFinish;
  TDateTime FBeepOnFinishAfter;
  UnicodeString FDefaultPuttyPathOnly;
  UnicodeString FDefaultPuttyPath;
  TCopyParamList * FCopyParamList;
  bool FCopyParamListDefaults;
  UnicodeString FCopyParamCurrent;
  TRemoteProperties FNewDirectoryProperties;
  intptr_t FKeepUpToDateChangeDelay;
  UnicodeString FChecksumAlg;
  intptr_t FSessionReopenAutoIdle;
/*
protected:
  LCID FLocale;

  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);
  virtual LCID __fastcall GetLocale();
  void __fastcall SetLocale(LCID value);
  void __fastcall SetLocaleSafe(LCID value);
  virtual HINSTANCE __fastcall LoadNewResourceModule(LCID Locale,
    UnicodeString * FileName = nullptr);
  HANDLE __fastcall GetResourceModule();
  virtual void __fastcall SetResourceModule(HINSTANCE Instance);
  TStrings * __fastcall GetLocales();
  LCID __fastcall InternalLocale();
  void __fastcall FreeResourceModule(HANDLE Instance);
  void __fastcall SetDefaultCopyParam(const TGUICopyParamType & value);
  virtual bool __fastcall GetRememberPassword();
  const TCopyParamList * __fastcall GetCopyParamList();
  void __fastcall SetCopyParamList(const TCopyParamList * value);
  virtual void __fastcall DefaultLocalized();
  int __fastcall GetCopyParamIndex();
  TGUICopyParamType __fastcall GetCurrentCopyParam();
  TGUICopyParamType __fastcall GetCopyParamPreset(UnicodeString Name);
  bool __fastcall GetHasCopyParamPreset(UnicodeString Name);
  void __fastcall SetCopyParamIndex(int value);
  void __fastcall SetCopyParamCurrent(UnicodeString value);
  void __fastcall SetNewDirectoryProperties(const TRemoteProperties & value);
  virtual void __fastcall Saved();
  void __fastcall SetQueueTransfersLimit(int value);
  void __fastcall SetQueueKeepDoneItems(bool value);
  void __fastcall SetQueueKeepDoneItemsFor(int value);
  void __fastcall SetLocaleInternal(LCID value, bool Safe);
  void __fastcall SetInitialLocale(LCID value);

public:
  __fastcall TGUIConfiguration();
  virtual __fastcall ~TGUIConfiguration();
  virtual void __fastcall Default();
  virtual void __fastcall UpdateStaticUsage();

  HANDLE __fastcall ChangeResourceModule(HANDLE Instance);
  TStoredSessionList * __fastcall SelectPuttySessionsForImport(TStoredSessionList * Sessions, UnicodeString & Error);
  bool __fastcall AnyPuttySessionForImport(TStoredSessionList * Sessions);
  TStoredSessionList * __fastcall SelectFilezillaSessionsForImport(
    TStoredSessionList * Sessions, UnicodeString & Error);
  bool __fastcall AnyFilezillaSessionForImport(TStoredSessionList * Sessions);
  void __fastcall DetectScalingType();

  __property bool ContinueOnError = { read = FContinueOnError, write = FContinueOnError };
  __property bool ConfirmCommandSession = { read = FConfirmCommandSession, write = FConfirmCommandSession };
  __property int SynchronizeParams = { read = FSynchronizeParams, write = FSynchronizeParams };
  __property int SynchronizeOptions = { read = FSynchronizeOptions, write = FSynchronizeOptions };
  __property int SynchronizeModeAuto = { read = FSynchronizeModeAuto, write = FSynchronizeModeAuto };
  __property int SynchronizeMode = { read = FSynchronizeMode, write = FSynchronizeMode };
  __property int MaxWatchDirectories = { read = FMaxWatchDirectories, write = FMaxWatchDirectories };
  __property int QueueTransfersLimit = { read = FQueueTransfersLimit, write = SetQueueTransfersLimit };
  __property bool QueueKeepDoneItems = { read = FQueueKeepDoneItems, write = SetQueueKeepDoneItems };
  __property int QueueKeepDoneItemsFor = { read = FQueueKeepDoneItemsFor, write = SetQueueKeepDoneItemsFor };
  __property bool QueueAutoPopup = { read = FQueueAutoPopup, write = FQueueAutoPopup };
  __property bool SessionRememberPassword = { read = FSessionRememberPassword, write = FSessionRememberPassword };
  __property LCID Locale = { read = GetLocale, write = SetLocale };
  __property LCID LocaleSafe = { read = GetLocale, write = SetLocaleSafe };
  __property TStrings * Locales = { read = GetLocales };
  __property UnicodeString PuttyPath = { read = FPuttyPath, write = FPuttyPath };
  __property UnicodeString DefaultPuttyPath = { read = FDefaultPuttyPath };
  __property UnicodeString PSftpPath = { read = FPSftpPath, write = FPSftpPath };
  __property bool PuttyPassword = { read = FPuttyPassword, write = FPuttyPassword };
  __property bool TelnetForFtpInPutty = { read = FTelnetForFtpInPutty, write = FTelnetForFtpInPutty };
  __property UnicodeString PuttySession = { read = FPuttySession, write = FPuttySession };
  __property TDateTime IgnoreCancelBeforeFinish = { read = FIgnoreCancelBeforeFinish, write = FIgnoreCancelBeforeFinish };
  __property TGUICopyParamType DefaultCopyParam = { read = FDefaultCopyParam, write = SetDefaultCopyParam };
  __property bool BeepOnFinish = { read = FBeepOnFinish, write = FBeepOnFinish };
  __property TDateTime BeepOnFinishAfter = { read = FBeepOnFinishAfter, write = FBeepOnFinishAfter };
  __property const TCopyParamList * CopyParamList = { read = GetCopyParamList, write = SetCopyParamList };
  __property UnicodeString CopyParamCurrent = { read = FCopyParamCurrent, write = SetCopyParamCurrent };
  __property int CopyParamIndex = { read = GetCopyParamIndex, write = SetCopyParamIndex };
  __property TGUICopyParamType CurrentCopyParam = { read = GetCurrentCopyParam };
  __property TGUICopyParamType CopyParamPreset[UnicodeString Name] = { read = GetCopyParamPreset };
  __property bool HasCopyParamPreset[UnicodeString Name] = { read = GetHasCopyParamPreset };
  __property TRemoteProperties NewDirectoryProperties = { read = FNewDirectoryProperties, write = SetNewDirectoryProperties };
  __property int KeepUpToDateChangeDelay = { read = FKeepUpToDateChangeDelay, write = FKeepUpToDateChangeDelay };
  __property UnicodeString ChecksumAlg = { read = FChecksumAlg, write = FChecksumAlg };
  __property int SessionReopenAutoIdle = { read = FSessionReopenAutoIdle, write = FSessionReopenAutoIdle };
  __property bool CanApplyLocaleImmediately = { read = FCanApplyLocaleImmediately, write = FCanApplyLocaleImmediately };
  __property LCID AppliedLocale = { read = FAppliedLocale };
*/
};

TGUIConfiguration * GetGUIConfiguration();

