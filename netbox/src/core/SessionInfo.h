#pragma once

#include "SessionData.h"
#include "Interface.h"

enum TSessionStatus
{
  ssClosed,
  ssOpening,
  ssOpened,
};

struct TSessionInfo : public TObject
{
  TSessionInfo();

  TDateTime LoginTime;
  UnicodeString ProtocolBaseName;
  UnicodeString ProtocolName;
  UnicodeString SecurityProtocolName;

  UnicodeString CSCipher;
  UnicodeString CSCompression;
  UnicodeString SCCipher;
  UnicodeString SCCompression;

  UnicodeString SshVersionString;
  UnicodeString SshImplementation;
  UnicodeString HostKeyFingerprint;

  UnicodeString CertificateFingerprint;
  UnicodeString Certificate;
};

enum TFSCapability
{
  fcUserGroupListing = 0, fcModeChanging, fcGroupChanging,
  fcOwnerChanging, fcGroupOwnerChangingByID, fcAnyCommand, fcHardLink,
  fcSymbolicLink,
  // With WebDAV this is always true, to avoid double-click on
  // file try to open the file as directory. It does no harm atm as
  // WebDAV never produce a symlink in listing.
  fcResolveSymlink,
  fcTextMode, fcRename, fcNativeTextMode, fcNewerOnlyUpload, fcRemoteCopy,
  fcTimestampChanging, fcRemoteMove, fcLoadingAdditionalProperties,
  fcCheckingSpaceAvailable, fcIgnorePermErrors, fcCalculatingChecksum,
  fcModeChangingUpload, fcPreservingTimestampUpload, fcShellAnyCommand,
  fcSecondaryShell, fcRemoveCtrlZUpload, fcRemoveBOMUpload, fcMoveToQueue,
  fcLocking, fcPreservingTimestampDirs, fcResumeSupport,
  fcCount,
};

struct TFileSystemInfo : public TObject
{
  TFileSystemInfo();

  UnicodeString ProtocolBaseName;
  UnicodeString ProtocolName;
  UnicodeString RemoteSystem;
  UnicodeString AdditionalInfo;
  bool IsCapable[fcCount];
};

class TSessionUI
{
//NB_DECLARE_CLASS(TSessionUI)
public:
  explicit TSessionUI() {}
  virtual ~TSessionUI() {}
  virtual void Information(const UnicodeString & Str, bool Status) = 0;
  virtual uintptr_t QueryUser(const UnicodeString & Query,
    TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual uintptr_t QueryUserException(const UnicodeString & Query,
    Exception * E, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Results) = 0;
  virtual void DisplayBanner(const UnicodeString & Banner) = 0;
  virtual void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"") = 0;
  virtual void HandleExtendedException(Exception * E) = 0;
  virtual void Closed() = 0;
  virtual void ProcessGUI() = 0;
};

// Duplicated in LogMemo.h for design-time-only purposes
enum TLogLineType
{
  llOutput,
  llInput,
  llStdError,
  llMessage,
  llException,
};

enum TLogAction
{
  laUpload,
  laDownload,
  laTouch,
  laChmod,
  laMkdir,
  laRm,
  laMv,
  laCall,
  laLs,
  laStat,
  laChecksum,
  laCwd,
};

enum TCaptureOutputType
{
  cotOutput,
  cotError,
  cotExitCode,
};

//typedef void (__closure *TCaptureOutputEvent)(
//  const UnicodeString & Str, TCaptureOutputType OutputType);
DEFINE_CALLBACK_TYPE2(TCaptureOutputEvent, void,
  const UnicodeString & /*Str*/, TCaptureOutputType /*OutputType*/);
//typedef void (__closure *TCalculatedChecksumEvent)(
//  const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
DEFINE_CALLBACK_TYPE3(TCalculatedChecksumEvent, void,
  const UnicodeString & /*FileName*/, const UnicodeString & /*Alg*/,
  const UnicodeString & /*Hash*/);

class TSessionActionRecord;
class TActionLog;

class TSessionAction : public TObject
{
NB_DISABLE_COPY(TSessionAction)
public:
  explicit TSessionAction(TActionLog * Log, TLogAction Action);
  virtual ~TSessionAction();

  void Restart();

  void Commit();
  void Rollback(Exception * E = nullptr);
  void Cancel();

protected:
  TSessionActionRecord * FRecord;
};

class TFileSessionAction : public TSessionAction
{
public:
  explicit TFileSessionAction(TActionLog * Log, TLogAction Action);
  explicit TFileSessionAction(TActionLog * Log, TLogAction Action, const UnicodeString & AFileName);

  void SetFileName(const UnicodeString & AFileName);
};

class TFileLocationSessionAction : public TFileSessionAction
{
public:
  explicit TFileLocationSessionAction(TActionLog * Log, TLogAction Action);
  explicit TFileLocationSessionAction(TActionLog * Log, TLogAction Action, const UnicodeString & AFileName);

  void Destination(const UnicodeString & Destination);
};

class TUploadSessionAction : public TFileLocationSessionAction
{
public:
  explicit TUploadSessionAction(TActionLog * Log);
};

class TDownloadSessionAction : public TFileLocationSessionAction
{
public:
  explicit TDownloadSessionAction(TActionLog * Log);
};

class TRights;

class TChmodSessionAction : public TFileSessionAction
{
public:
  explicit TChmodSessionAction(TActionLog * Log, const UnicodeString & AFileName);
  explicit TChmodSessionAction(TActionLog * Log, const UnicodeString & AFileName,
    const TRights & Rights);

  void Rights(const TRights & Rights);
  void Recursive();
};

class TTouchSessionAction : public TFileSessionAction
{
public:
  explicit TTouchSessionAction(TActionLog * Log, const UnicodeString & AFileName,
    const TDateTime & Modification);
};

class TMkdirSessionAction : public TFileSessionAction
{
public:
  explicit TMkdirSessionAction(TActionLog * Log, const UnicodeString & AFileName);
};

class TRmSessionAction : public TFileSessionAction
{
public:
  explicit TRmSessionAction(TActionLog * Log, const UnicodeString & AFileName);

  void Recursive();
};

class TMvSessionAction : public TFileLocationSessionAction
{
public:
  explicit TMvSessionAction(TActionLog * Log, const UnicodeString & AFileName,
    const UnicodeString & Destination);
};

class TCallSessionAction : public TSessionAction
{
public:
  explicit TCallSessionAction(TActionLog * Log, const UnicodeString & Command,
    const UnicodeString & Destination);

  void AddOutput(const UnicodeString & Output, bool StdError);
  void ExitCode(int ExitCode);
};

class TLsSessionAction : public TSessionAction
{
public:
  explicit TLsSessionAction(TActionLog * Log, const UnicodeString & Destination);

  void FileList(TRemoteFileList * FileList);
};

class TStatSessionAction : public TFileSessionAction
{
public:
  explicit TStatSessionAction(TActionLog * Log, const UnicodeString & AFileName);

  void File(TRemoteFile * AFile);
};

class TChecksumSessionAction : public TFileSessionAction
{
public:
  explicit TChecksumSessionAction(TActionLog * Log);

  void Checksum(const UnicodeString & Alg, const UnicodeString & Checksum);
};

class TCwdSessionAction : public TSessionAction
{
public:
  TCwdSessionAction(TActionLog * Log, const UnicodeString & Path);
};

// void (__closure *f)(TLogLineType Type, const UnicodeString & Line));
DEFINE_CALLBACK_TYPE2(TDoAddLogEvent, void,
  TLogLineType /*Type*/, const UnicodeString & /*Line*/);

class TSessionLog : protected TStringList
{
CUSTOM_MEM_ALLOCATION_IMPL
friend class TSessionAction;
friend class TSessionActionRecord;
NB_DISABLE_COPY(TSessionLog)
public:
  explicit TSessionLog(TSessionUI * UI, TSessionData * SessionData,
    TConfiguration * Configuration);
  virtual ~TSessionLog();
  HIDESBASE void Add(TLogLineType Type, const UnicodeString & Line);
  void AddSystemInfo();
  void AddStartupInfo();
  void AddException(Exception * E);
  void AddSeparator();

  virtual void Clear();
  void ReflectSettings();
  void Lock();
  void Unlock();

/*
  __property TSessionLog * Parent = { read = FParent, write = FParent };
  __property bool Logging = { read = FLogging };
  __property int BottomIndex = { read = GetBottomIndex };
  __property UnicodeString Line[int Index]  = { read=GetLine };
  __property TLogLineType Type[int Index]  = { read=GetType };
  __property OnChange;
  __property TNotifyEvent OnStateChange = { read = FOnStateChange, write = FOnStateChange };
  __property UnicodeString CurrentFileName = { read = FCurrentFileName };
  __property bool LoggingToFile = { read = GetLoggingToFile };
  __property int TopIndex = { read = FTopIndex };
  __property UnicodeString SessionName = { read = GetSessionName };
  __property UnicodeString Name = { read = FName, write = FName };
  __property Count;
*/
  TSessionLog * GetParent();
  void SetParent(TSessionLog * Value);
  bool GetLogging() const;
  TNotifyEvent & GetOnChange();
  void SetOnChange(TNotifyEvent Value);
  TNotifyEvent & GetOnStateChange();
  void SetOnStateChange(TNotifyEvent Value);
  UnicodeString GetCurrentFileName() const;
  intptr_t GetTopIndex() const;
  UnicodeString GetName() const;
  void SetName(const UnicodeString & Value);
  virtual intptr_t GetCount() const;

protected:
  void CloseLogFile();
  bool LogToFile() const;

private:
  TConfiguration * FConfiguration;
  TSessionLog * FParent;
  TCriticalSection FCriticalSection;
  bool FLogging;
  void * FFile;
  UnicodeString FCurrentLogFileName;
  UnicodeString FCurrentFileName;
  int FLoggedLines;
  intptr_t FTopIndex;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  UnicodeString FName;
  bool FClosed;
  TNotifyEvent FOnStateChange;

public:
  UnicodeString GetLine(intptr_t Index) const;
  TLogLineType GetType(intptr_t Index) const;
  void DeleteUnnecessary();
  void StateChange();
  void OpenLogFile();
  intptr_t GetBottomIndex() const;
  UnicodeString GetLogFileName() const;
  bool GetLoggingToFile() const;
  UnicodeString GetSessionName() const;

private:
  void DoAdd(TLogLineType AType, const UnicodeString & ALine,
    TDoAddLogEvent Event);
  void DoAddToParent(TLogLineType AType, const UnicodeString & ALine);
  void DoAddToSelf(TLogLineType AType, const UnicodeString & ALine);
  void AddStartupInfo(bool System);
  void DoAddStartupInfo(TSessionData * Data);
  UnicodeString GetTlsVersionName(TTlsVersion TlsVersion);
  UnicodeString LogSensitive(const UnicodeString & Str);
  void AddOption(const UnicodeString & LogStr);
  void AddOptions(TOptions * Options);
  UnicodeString GetCmdLineLog();
};

class TActionLog : public TObject
{
friend class TSessionAction;
friend class TSessionActionRecord;
NB_DISABLE_COPY(TActionLog)
public:
  explicit TActionLog(TSessionUI * UI, TSessionData * SessionData,
    TConfiguration * Configuration);
  explicit TActionLog(TConfiguration * Configuration);
  virtual ~TActionLog();

  void ReflectSettings();
  void AddFailure(Exception * E);
  void AddFailure(TStrings * Messages);
  void BeginGroup(const UnicodeString & Name);
  void EndGroup();

/*
  __property UnicodeString CurrentFileName = { read = FCurrentFileName };
  __property bool Enabled = { read = FEnabled, write = SetEnabled };
*/
  UnicodeString GetCurrentFileName() const { return FCurrentFileName; }
  bool GetEnabled() const { return FEnabled; }

protected:
  void CloseLogFile();
  inline void AddPendingAction(TSessionActionRecord * Action);
  void RecordPendingActions();
  void Add(const UnicodeString & Line);
  void AddIndented(const UnicodeString & Line);
  void AddMessages(const UnicodeString & Indent, TStrings * Messages);
  void Init(TSessionUI * UI, TSessionData * SessionData,
    TConfiguration * Configuration);

private:
  TConfiguration * FConfiguration;
  TCriticalSection FCriticalSection;
  bool FLogging;
  void * FFile;
  UnicodeString FCurrentLogFileName;
  UnicodeString FCurrentFileName;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  TList * FPendingActions;
  bool FClosed;
  bool FInGroup;
  UnicodeString FIndent;
  bool FEnabled;

  void OpenLogFile();

public:
  UnicodeString GetLogFileName() const;
  void SetEnabled(bool Value);
};

