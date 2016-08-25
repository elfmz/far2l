
#include <vcl.h>
#pragma hdrstop

#include "Terminal.h"

#include <Sysutils.hpp>
#include <FileCtrl.hpp>
#include <Common.h>
#include <FileBuffer.h>

#include "PuttyTools.h"
#include "Interface.h"
#include "RemoteFiles.h"
#include "SecureShell.h"
#include "ScpFileSystem.h"
#include "SftpFileSystem.h"
#ifndef NO_FILEZILLA
#include "FtpFileSystem.h"
#endif
#include "WebDAVFileSystem.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "CoreMain.h"
#include "Queue.h"
#include <openssl/pkcs12.h>
#include <openssl/err.h>

#if !defined(AUTO_WINSOCK) && !defined(__linux__)
#include <winsock2.h>
#endif

///* TODO : Better user interface (query to user) */
void FileOperationLoopCustom(TTerminal * Terminal,
  TFileOperationProgressType * OperationProgress,
  bool AllowSkip, const UnicodeString & Message,
  const UnicodeString & HelpKeyword,
  const std::function<void()> & Operation)
{
  bool DoRepeat;
  do
  {
    DoRepeat = false;
    try
    {
      Operation();
    }
    catch (EAbort &)
    {
      throw;
    }
    catch (ESkipFile &)
    {
      throw;
    }
    catch (EFatal &)
    {
      throw;
    }
    catch (EFileNotFoundError &)
    {
      throw;
    }
    catch (EOSError &)
    {
      throw;
    }
    catch (Exception & E)
    {
      Terminal->FileOperationLoopQuery(
        E, OperationProgress, Message, AllowSkip, L"", HelpKeyword);
      DoRepeat = true;
    }
  }
  while (DoRepeat);
}

class TLoopDetector : public TObject
{
public:
  TLoopDetector();
  void RecordVisitedDirectory(const UnicodeString & Directory);
  bool IsUnvisitedDirectory(const UnicodeString & Directory);

private:
  std::unique_ptr<TStringList> FVisitedDirectories;
};

TLoopDetector::TLoopDetector()
{
  FVisitedDirectories.reset(CreateSortedStringList());
}

void TLoopDetector::RecordVisitedDirectory(const UnicodeString & Directory)
{
  UnicodeString VisitedDirectory = ::ExcludeTrailingBackslash(Directory);
  FVisitedDirectories->Add(VisitedDirectory);
}

bool TLoopDetector::IsUnvisitedDirectory(const UnicodeString & Directory)
{
  bool Result = (FVisitedDirectories->IndexOf(Directory) < 0);

  if (Result)
  {
    RecordVisitedDirectory(Directory);
  }

  return Result;
}

struct TMoveFileParams : public TObject
{
NB_DECLARE_CLASS(TMoveFileParams)
public:
  UnicodeString Target;
  UnicodeString FileMask;
};

struct TFilesFindParams : public TObject
{
NB_DECLARE_CLASS(TFilesFindParams)
public:
  TFilesFindParams() :
    OnFileFound(nullptr),
    OnFindingFile(nullptr),
    Cancel(false)
  {
  }
  TFileMasks FileMask;
  TFileFoundEvent OnFileFound;
  TFindingFileEvent OnFindingFile;
  bool Cancel;
  TLoopDetector LoopDetector;
  UnicodeString RealDirectory;
};

TCalculateSizeStats::TCalculateSizeStats() :
  Files(0),
  Directories(0),
  SymLinks(0)
{
//  memset(this, 0, sizeof(*this));
}

TSynchronizeOptions::TSynchronizeOptions() :
  Filter(0)
{
//  memset(this, 0, sizeof(*this));
}

TSynchronizeOptions::~TSynchronizeOptions()
{
  SAFE_DESTROY(Filter);
}

bool TSynchronizeOptions::MatchesFilter(const UnicodeString & AFileName)
{
  bool Result = false;
  if (Filter == nullptr)
  {
    Result = true;
  }
  else
  {
    intptr_t FoundIndex = 0;
    Result = Filter->Find(AFileName, FoundIndex);
  }
  return Result;
}

TSpaceAvailable::TSpaceAvailable() :
  BytesOnDevice(0),
  UnusedBytesOnDevice(0),
  BytesAvailableToUser(0),
  UnusedBytesAvailableToUser(0),
  BytesPerAllocationUnit(0)
{
//  memset(this, 0, sizeof(*this));
}

TChecklistItem::TChecklistItem() :
  Action(saNone),
  IsDirectory(false),
  ImageIndex(-1),
  Checked(true),
  RemoteFile(nullptr)
{
  Local.ModificationFmt = mfFull;
  Local.Modification = 0;
  Local.Size = 0;
  Remote.ModificationFmt = mfFull;
  Remote.Modification = 0;
  Remote.Size = 0;
  FLocalLastWriteTime.dwHighDateTime = 0;
  FLocalLastWriteTime.dwLowDateTime = 0;
}

TChecklistItem::~TChecklistItem()
{
  SAFE_DESTROY(RemoteFile);
}

const UnicodeString TChecklistItem::GetFileName() const
{
  if (!Remote.FileName.IsEmpty())
  {
    return Remote.FileName;
  }
  else
  {
    DebugAssert(!Local.FileName.IsEmpty());
    return Local.FileName;
  }
}

TSynchronizeChecklist::TSynchronizeChecklist()
{
}

TSynchronizeChecklist::~TSynchronizeChecklist()
{
  for (intptr_t Index = 0; Index < FList.GetCount(); ++Index)
  {
    TChecklistItem * Item = NB_STATIC_DOWNCAST(TChecklistItem, static_cast<void *>(FList.GetItem(Index)));
    SAFE_DESTROY(Item);
  }
}

void TSynchronizeChecklist::Add(TChecklistItem * Item)
{
  FList.Add(Item);
}

intptr_t TSynchronizeChecklist::Compare(const void * AItem1, const void * AItem2)
{
  const TChecklistItem * Item1 = NB_STATIC_DOWNCAST_CONST(TChecklistItem, AItem1);
  const TChecklistItem * Item2 = NB_STATIC_DOWNCAST_CONST(TChecklistItem, AItem2);

  intptr_t Result;
  if (!Item1->Local.Directory.IsEmpty())
  {
    Result = ::AnsiCompareText(Item1->Local.Directory, Item2->Local.Directory);
  }
  else
  {
    DebugAssert(!Item1->Remote.Directory.IsEmpty());
    Result = ::AnsiCompareText(Item1->Remote.Directory, Item2->Remote.Directory);
  }

  if (Result == 0)
  {
    Result = ::AnsiCompareText(Item1->GetFileName(), Item2->GetFileName());
  }

  return Result;
}

void TSynchronizeChecklist::Sort()
{
  FList.Sort(Compare);
}

intptr_t TSynchronizeChecklist::GetCount() const
{
  return FList.GetCount();
}

const TChecklistItem * TSynchronizeChecklist::GetItem(intptr_t Index) const
{
  return NB_STATIC_DOWNCAST(TChecklistItem, FList.GetItem(Index));
}

void TSynchronizeChecklist::Update(const TChecklistItem * Item, bool Check, TChecklistAction Action)
{
  // TSynchronizeChecklist owns non-const items so it can manipulate them freely,
  // const_cast here is just an optimization
  TChecklistItem * MutableItem = const_cast<TChecklistItem *>(Item);
  DebugAssert(FList.IndexOf(MutableItem) >= 0);
  MutableItem->Checked = Check;
  MutableItem->Action = Action;
}

TChecklistAction TSynchronizeChecklist::Reverse(TChecklistAction Action)
{
  switch (Action)
  {
    case saUploadNew:
      return saDeleteLocal;

    case saDownloadNew:
      return saDeleteRemote;

    case saUploadUpdate:
      return saDownloadUpdate;

    case saDownloadUpdate:
      return saUploadUpdate;

    case saDeleteRemote:
      return saDownloadNew;

    case saDeleteLocal:
      return saUploadNew;

    default:
    case saNone:
      DebugFail();
      return saNone;
  }
}


class TTunnelThread : public TSimpleThread
{
NB_DISABLE_COPY(TTunnelThread)
public:
  explicit TTunnelThread(TSecureShell * SecureShell);
  virtual ~TTunnelThread();

  virtual void Init();
  virtual void Terminate();

protected:
  virtual void Execute();

private:
  TSecureShell * FSecureShell;
  bool FTerminated;
};

TTunnelThread::TTunnelThread(TSecureShell * SecureShell) :
  TSimpleThread(),
  FSecureShell(SecureShell),
  FTerminated(false)
{
}

void TTunnelThread::Init()
{
  TSimpleThread::Init();
  Start();
}

TTunnelThread::~TTunnelThread()
{
  // close before the class's virtual functions (Terminate particularly) are lost
  Close();
}

void TTunnelThread::Terminate()
{
  FTerminated = true;
}

void TTunnelThread::Execute()
{
  try
  {
    while (!FTerminated)
    {
      FSecureShell->Idle(250);
    }
  }
  catch (...)
  {
    if (FSecureShell->GetActive())
    {
      FSecureShell->Close();
    }
    // do not pass exception out of thread's proc
  }
}


class TTunnelUI : public TSessionUI
{
NB_DISABLE_COPY(TTunnelUI)
public:
  explicit TTunnelUI(TTerminal * Terminal);
  virtual ~TTunnelUI() {}
  virtual void Information(const UnicodeString & Str, bool Status);
  virtual uintptr_t QueryUser(const UnicodeString & Query,
    TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType);
  virtual uintptr_t QueryUserException(const UnicodeString & Query,
    Exception * E, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType);
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & AInstructions, TStrings * Prompts,
    TStrings * Results);
  virtual void DisplayBanner(const UnicodeString & Banner);
  virtual void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpContext);
  virtual void HandleExtendedException(Exception * E);
  virtual void Closed();
  virtual void ProcessGUI();

private:
  TTerminal * FTerminal;
  uint32_t FTerminalThread;
};

TTunnelUI::TTunnelUI(TTerminal * Terminal)
{
  FTerminal = Terminal;
  FTerminalThread = GetCurrentThreadId();
}

void TTunnelUI::Information(const UnicodeString & Str, bool Status)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->Information(Str, Status);
  }
}

uintptr_t TTunnelUI::QueryUser(const UnicodeString & Query,
  TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uintptr_t Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    Result = FTerminal->QueryUser(Query, MoreMessages, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(Answers);
  }
  return Result;
}

uintptr_t TTunnelUI::QueryUserException(const UnicodeString & Query,
  Exception * E, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uintptr_t Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    Result = FTerminal->QueryUserException(Query, E, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(static_cast<intptr_t>(Answers));
  }
  return Result;
}

bool TTunnelUI::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & AName, const UnicodeString & AInstructions, TStrings * Prompts, TStrings * Results)
{
  bool Result = false;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    UnicodeString Instructions = AInstructions;
    if (IsAuthenticationPrompt(Kind))
    {
      Instructions = LoadStr(TUNNEL_INSTRUCTION) +
        (AInstructions.IsEmpty() ? L"" : L"\n") +
        AInstructions;
    }

    Result = FTerminal->PromptUser(Data, Kind, AName, Instructions, Prompts, Results);
  }
  return Result;
}

void TTunnelUI::DisplayBanner(const UnicodeString & Banner)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->DisplayBanner(Banner);
  }
}

void TTunnelUI::FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword)
{
  throw ESshFatal(E, Msg, HelpKeyword);
}

void TTunnelUI::HandleExtendedException(Exception * E)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->HandleExtendedException(E);
  }
}

void TTunnelUI::Closed()
{
  // noop
}

void TTunnelUI::ProcessGUI()
{
  // noop
}

class TCallbackGuard : public TObject
{
NB_DISABLE_COPY(TCallbackGuard)
public:
  explicit TCallbackGuard(TTerminal * FTerminal);
  inline ~TCallbackGuard();

  void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword);
  inline void Verify();
  void Dismiss();

private:
  ExtException * FFatalError;
  TTerminal * FTerminal;
  bool FGuarding;
};

TCallbackGuard::TCallbackGuard(TTerminal * Terminal) :
  FFatalError(nullptr),
  FTerminal(Terminal),
  FGuarding(FTerminal->FCallbackGuard == nullptr)
{
  if (FGuarding)
  {
    FTerminal->FCallbackGuard = this;
  }
}

TCallbackGuard::~TCallbackGuard()
{
  if (FGuarding)
  {
    DebugAssert((FTerminal->FCallbackGuard == this) || (FTerminal->FCallbackGuard == nullptr));
    FTerminal->FCallbackGuard = nullptr;
  }

  SAFE_DESTROY(FFatalError);
}

void TCallbackGuard::FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword)
{
  DebugAssert(FGuarding);

  // make sure we do not bother about getting back the silent abort exception
  // we issued ourselves. this may happen when there is an exception handler
  // that converts any exception to fatal one (such as in TTerminal::Open).
  if (NB_STATIC_DOWNCAST(ECallbackGuardAbort, E) == nullptr)
  {
    SAFE_DESTROY(FFatalError);
    FFatalError = new ExtException(E, Msg, HelpKeyword);
  }

  // silently abort what we are doing.
  // non-silent exception would be caught probably by default application
  // exception handler, which may not do an appropriate action
  // (particularly it will not resume broken transfer).
  throw ECallbackGuardAbort();
}

void TCallbackGuard::Dismiss()
{
  DebugAssert(FFatalError == nullptr);
  FGuarding = false;
}

void TCallbackGuard::Verify()
{
  if (FGuarding)
  {
    FGuarding = false;
    DebugAssert(FTerminal->FCallbackGuard == this);
    FTerminal->FCallbackGuard = nullptr;

    if (FFatalError != nullptr)
    {
      throw ESshFatal(FFatalError, L"");
    }
  }
}


TRobustOperationLoop::TRobustOperationLoop(TTerminal * Terminal, TFileOperationProgressType * OperationProgress) :
  FTerminal(Terminal),
  FOperationProgress(OperationProgress),
  FRetry(false)
{
}

bool TRobustOperationLoop::TryReopen(Exception & E)
{
  FRetry = FTerminal &&
    !FTerminal->GetActive() &&
    FTerminal->QueryReopen(&E, ropNoReadDirectory, FOperationProgress);
  return FRetry;
}

bool TRobustOperationLoop::ShouldRetry() const
{
  return FRetry;
}

bool TRobustOperationLoop::Retry()
{
  bool Result = FRetry;
  FRetry = false;
  return Result;
}

class TRetryOperationLoop
{
public:
  explicit TRetryOperationLoop(TTerminal * Terminal);

  void Error(Exception & E);
  void Error(Exception & E, TSessionAction & Action);
  void Error(Exception & E, const UnicodeString & Message);
  void Error(Exception & E, TSessionAction & Action, const UnicodeString & Message);
  bool Retry();

private:
  TTerminal * FTerminal;
  bool FRetry;

  void DoError(Exception & E, TSessionAction * Action, const UnicodeString & Message);
};

TRetryOperationLoop::TRetryOperationLoop(TTerminal * Terminal)
{
  FTerminal = Terminal;
  FRetry = false;
}

void TRetryOperationLoop::DoError(Exception & E, TSessionAction * Action, const UnicodeString & Message)
{
  // Note that the action may already be canceled when RollbackAction is called
  uintptr_t Result;
  try
  {
    Result = FTerminal->CommandError(&E, Message, qaRetry | qaSkip | qaAbort);
  }
  catch (Exception & E2)
  {
    if (Action != nullptr)
    {
      FTerminal->RollbackAction(*Action, nullptr, &E2);
    }
    throw;
  }

  switch (Result)
  {
    case qaRetry:
      FRetry = true;
      if (Action != nullptr)
      {
        Action->Cancel();
      }
      break;

    case qaAbort:
      if (Action != nullptr)
      {
        FTerminal->RollbackAction(*Action, nullptr, &E);
      }
      Abort();
      break;

    case qaSkip:
      if (Action != nullptr)
      {
        Action->Cancel();
      }
      break;

    default:
      DebugFail();
      break;
  }
}

void TRetryOperationLoop::Error(Exception & E)
{
  DoError(E, nullptr, UnicodeString());
}

void TRetryOperationLoop::Error(Exception & E, TSessionAction & Action)
{
  DoError(E, &Action, UnicodeString());
}

void TRetryOperationLoop::Error(Exception & E, const UnicodeString & Message)
{
  DoError(E, nullptr, Message);
}

void TRetryOperationLoop::Error(Exception & E, TSessionAction & Action, const UnicodeString & Message)
{
  DoError(E, &Action, Message);
}

bool TRetryOperationLoop::Retry()
{
  bool Result = FRetry;
  FRetry = false;
  return Result;
}


TTerminal::TTerminal() :
  TObject(),
  TSessionUI(),
  FReadCurrentDirectoryPending(false),
  FReadDirectoryPending(false),
  FTunnelOpening(false),
  FSessionData(nullptr),
  FLog(nullptr),
  FActionLog(nullptr),
  FConfiguration(nullptr),
  FExceptionOnFail(0),
  FFiles(nullptr),
  FInTransaction(0),
  FSuspendTransaction(false),
  FOnChangeDirectory(nullptr),
  FOnReadDirectory(nullptr),
  FOnStartReadDirectory(nullptr),
  FOnReadDirectoryProgress(nullptr),
  FOnDeleteLocalFile(nullptr),
  FOnCreateLocalFile(nullptr),
  FOnGetLocalFileAttributes(nullptr),
  FOnSetLocalFileAttributes(nullptr),
  FOnMoveLocalFile(nullptr),
  FOnRemoveLocalDirectory(nullptr),
  FOnCreateLocalDirectory(nullptr),
  FOnInitializeLog(nullptr),
  FUsersGroupsLookedup(false),
  FOperationProgress(nullptr),
  FUseBusyCursor(false),
  FDirectoryCache(nullptr),
  FDirectoryChangesCache(nullptr),
  FFileSystem(nullptr),
  FSecureShell(nullptr),
  FFSProtocol(cfsUnknown),
  FCommandSession(nullptr),
  FAutoReadDirectory(false),
  FReadingCurrentDirectory(false),
  FClosedOnCompletion(nullptr),
  FStatus(ssClosed),
  FOpening(0),
  FTunnelThread(nullptr),
  FTunnel(nullptr),
  FTunnelData(nullptr),
  FTunnelLog(nullptr),
  FTunnelUI(nullptr),
  FTunnelLocalPortNumber(0),
  FCallbackGuard(nullptr),
  FEnableSecureShellUsage(false),
  FCollectFileSystemUsage(false),
  FRememberedPasswordTried(false),
  FRememberedTunnelPasswordTried(false),
  FNesting(0)
{
  FOldFiles = new TRemoteDirectory(this);
}

TTerminal::~TTerminal()
{
  if (GetActive())
  {
    Close();
  }

  if (FCallbackGuard != nullptr)
  {
    // see TTerminal::HandleExtendedException
    FCallbackGuard->Dismiss();
  }
  DebugAssert(FTunnel == nullptr);

  SAFE_DESTROY(FCommandSession);

  if (GetSessionData()->GetCacheDirectoryChanges() && GetSessionData()->GetPreserveDirectoryChanges() &&
      (FDirectoryChangesCache != nullptr))
  {
    FConfiguration->SaveDirectoryChangesCache(GetSessionData()->GetSessionKey(),
      FDirectoryChangesCache);
  }

  SAFE_DESTROY_EX(TCustomFileSystem, FFileSystem);
  SAFE_DESTROY_EX(TSessionLog, FLog);
  SAFE_DESTROY_EX(TActionLog, FActionLog);
  SAFE_DESTROY(FFiles);
  SAFE_DESTROY_EX(TRemoteDirectoryCache, FDirectoryCache);
  SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
  SAFE_DESTROY(FSessionData);
  SAFE_DESTROY(FOldFiles);
}

void TTerminal::Init(TSessionData * SessionData, TConfiguration * Configuration)
{
  FConfiguration = Configuration;
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(SessionData);
  FLog = new TSessionLog(this, FSessionData, FConfiguration);
  FActionLog = new TActionLog(this, FSessionData, FConfiguration);
  FFiles = new TRemoteDirectory(this);
  FExceptionOnFail = 0;
  FInTransaction = 0;
  FReadCurrentDirectoryPending = false;
  FReadDirectoryPending = false;
  FUsersGroupsLookedup = False;
  FTunnelLocalPortNumber = 0;
  FFileSystem = nullptr;
  FSecureShell = nullptr;
  FOnProgress = nullptr;
  FOnFinished = nullptr;
  FOnDeleteLocalFile = nullptr;
  FOnCreateLocalFile = nullptr;
  FOnGetLocalFileAttributes = nullptr;
  FOnSetLocalFileAttributes = nullptr;
  FOnMoveLocalFile = nullptr;
  FOnRemoveLocalDirectory = nullptr;
  FOnCreateLocalDirectory = nullptr;
  FOnReadDirectoryProgress = nullptr;
  FOnQueryUser = nullptr;
  FOnPromptUser = nullptr;
  FOnDisplayBanner = nullptr;
  FOnShowExtendedException = nullptr;
  FOnInformation = nullptr;
  FOnClose = nullptr;
  FOnFindingFile = nullptr;

  FUseBusyCursor = True;
  FLockDirectory.Clear();
  FDirectoryCache = new TRemoteDirectoryCache();
  FDirectoryChangesCache = nullptr;
  FFSProtocol = cfsUnknown;
  FCommandSession = nullptr;
  FAutoReadDirectory = true;
  FReadingCurrentDirectory = false;
  FStatus = ssClosed;
  FTunnelThread = nullptr;
  FTunnel = nullptr;
  FTunnelData = nullptr;
  FTunnelLog = nullptr;
  FTunnelUI = nullptr;
  FTunnelOpening = false;
  FCallbackGuard = nullptr;
  FEnableSecureShellUsage = false;
  FCollectFileSystemUsage = false;
  FSuspendTransaction = false;
  FOperationProgress = nullptr;
  FClosedOnCompletion = nullptr;
}

void TTerminal::Idle()
{
  // Once we disconnect, do nothing, until reconnect handler
  // "receives the information".
  // Never go idle when called from within ::ProcessGUI() call
  // as we may recurse for good, timeouting eventually.
  if (GetActive() && (FNesting == 0))
  {
    TAutoNestingCounter NestingCounter(FNesting);

    if (FConfiguration->GetActualLogProtocol() >= 1)
    {
      // LogEvent("Session upkeep");
    }

    DebugAssert(FFileSystem != nullptr);
    FFileSystem->Idle();

    if (GetCommandSessionOpened())
    {
      try
      {
        FCommandSession->Idle();
      }
      catch (Exception & E)
      {
        // If the secondary session is dropped, ignore the error and let
        // it be reconnected when needed.
        // BTW, non-fatal error can hardly happen here, that's why
        // it is displayed, because it can be useful to know.
        if (FCommandSession->GetActive())
        {
          FCommandSession->HandleExtendedException(&E);
        }
      }
    }
  }
}

RawByteString TTerminal::EncryptPassword(const UnicodeString & APassword) const
{
  return FConfiguration->EncryptPassword(APassword, GetSessionData()->GetSessionName());
}

UnicodeString TTerminal::DecryptPassword(const RawByteString & APassword) const
{
  UnicodeString Result;
  try
  {
    Result = FConfiguration->DecryptPassword(APassword, GetSessionData()->GetSessionName());
  }
  catch (EAbort &)
  {
    // silently ignore aborted prompts for master password and return empty password
  }
  return Result;
}

void TTerminal::RecryptPasswords()
{
  FSessionData->RecryptPasswords();
  FRememberedPassword = EncryptPassword(DecryptPassword(FRememberedPassword));
  FRememberedTunnelPassword = EncryptPassword(DecryptPassword(FRememberedTunnelPassword));
}

UnicodeString TTerminal::ExpandFileName(const UnicodeString & APath,
  const UnicodeString & BasePath)
{
  // replace this by AbsolutePath()
  UnicodeString Result = core::UnixExcludeTrailingBackslash(APath);
  if (!core::UnixIsAbsolutePath(Result) && !BasePath.IsEmpty())
  {
    TODO("Handle more complicated cases like '../../xxx'");
    if (Result == PARENTDIRECTORY)
    {
      Result = core::UnixExcludeTrailingBackslash(core::UnixExtractFilePath(
        core::UnixExcludeTrailingBackslash(BasePath)));
    }
    else
    {
      Result = core::UnixIncludeTrailingBackslash(BasePath) + APath;
    }
  }
  return Result;
}

bool TTerminal::GetActive() const
{
  return (this != nullptr) && (FFileSystem != nullptr) && FFileSystem->GetActive();
}

void TTerminal::Close()
{
  FFileSystem->Close();

  // Cannot rely on CommandSessionOpened here as Status is set to ssClosed
  // only after the OnClose is called
  if ((FCommandSession != nullptr) && FCommandSession->GetActive())
  {
    // prevent recursion
    FCommandSession->SetOnClose(nullptr);
    FCommandSession->Close();
  }
}

void TTerminal::ResetConnection()
{
  // used to be called from Reopen(), why?
  FTunnelError.Clear();

  FRememberedPasswordTried = false;
  // Particularly to prevent reusing a wrong client certificate passphrase
  // from a previous login attempt
  FRememberedPassword = UnicodeString();
  FRememberedTunnelPasswordTried = false;
  FRememberedTunnelPassword = UnicodeString();

  if (FDirectoryChangesCache != nullptr)
  {
    SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
  }

  FFiles->SetDirectory(L"");
  // note that we cannot clear contained files
  // as they can still be referenced in the GUI atm
}

UnicodeString TTerminal::FingerprintScan()
{
  GetSessionData()->SetFingerprintScan(true);
  try
  {
    Open();
    // we should never get here
    Abort();
  }
  catch (...)
  {
    if (!FFingerprintScanned.IsEmpty())
    {
      return FFingerprintScanned;
    }
    throw;
  }
  DebugFail();
  return UnicodeString();
}

void TTerminal::Open()
{
  TAutoNestingCounter OpeningCounter(FOpening);
  ReflectSettings();
  bool Reopen = false;
  do
  {
    Reopen = false;
    DoInformation(L"", true, 1);
    try
    {
      SCOPE_EXIT
      {
        DoInformation(L"", true, 0);
      };
      {
        InternalTryOpen();
      }
    }
    catch (EFatal & E)
    {
      Reopen = DoQueryReopen(&E);
      if (Reopen)
      {
        SAFE_DESTROY(FFileSystem);
        SAFE_DESTROY(FSecureShell);
        SAFE_DESTROY(FTunnelData);
        FStatus = ssClosed;
        SAFE_DESTROY(FTunnel);
      }
      else
      {
        throw;
      }
    }
    /*catch (EFatal &)
    {
      throw;
    }*/
    catch (Exception & E)
    {
      LogEvent(FORMAT(L"Got error: \"%s\"", E.Message.c_str()));
      // any exception while opening session is fatal
      FatalError(&E, L"");
    }
  }
  while (Reopen);
  FSessionData->SetNumberOfRetries(0);
}

void TTerminal::InternalTryOpen()
{
  try
  {
    ResetConnection();
    FStatus = ssOpening;

    {
      SCOPE_EXIT
      {
        if (FSessionData->GetTunnel())
        {
          FSessionData->RollbackTunnel();
        }
      };
      InternalDoTryOpen();
    }

    if (GetSessionData()->GetCacheDirectoryChanges())
    {
      DebugAssert(FDirectoryChangesCache == nullptr);
      FDirectoryChangesCache = new TRemoteDirectoryChangesCache(
        FConfiguration->GetCacheDirectoryChangesMaxSize());
      if (GetSessionData()->GetPreserveDirectoryChanges())
      {
        FConfiguration->LoadDirectoryChangesCache(GetSessionData()->GetSessionKey(),
            FDirectoryChangesCache);
      }
    }

    DoStartup();

    if (FCollectFileSystemUsage)
    {
      FFileSystem->CollectUsage();
      FCollectFileSystemUsage = false;
    }

    DoInformation(LoadStr(STATUS_READY), true);
    FStatus = ssOpened;
  }
  catch (...)
  {
    // rollback
    if (FDirectoryChangesCache != nullptr)
    {
      SAFE_DESTROY_EX(TRemoteDirectoryChangesCache, FDirectoryChangesCache);
    }
    if (GetSessionData()->GetFingerprintScan() && (FFileSystem != nullptr) &&
        DebugAlwaysTrue(GetSessionData()->GetFtps() != ftpsNone))
    {
      FFingerprintScanned = FFileSystem->GetSessionInfo().CertificateFingerprint;
    }
    throw;
  }
}

void TTerminal::InternalDoTryOpen()
{
  if (FFileSystem == nullptr)
  {
    GetLog()->AddSystemInfo();
    DoInitializeLog();
    GetLog()->AddStartupInfo();
  }

  DebugAssert(FTunnel == nullptr);
  if (FSessionData->GetTunnel())
  {
    DoInformation(LoadStr(OPEN_TUNNEL), true);
    LogEvent("Opening tunnel.");
    OpenTunnel();
    GetLog()->AddSeparator();

    FSessionData->ConfigureTunnel(FTunnelLocalPortNumber);

    DoInformation(LoadStr(USING_TUNNEL), false);
    LogEvent(FORMAT(L"Connecting via tunnel interface %s:%d.",
      FSessionData->GetHostNameExpanded().c_str(), FSessionData->GetPortNumber()));
  }
  else
  {
    DebugAssert(FTunnelLocalPortNumber == 0);
  }

  if (FFileSystem == nullptr)
  {
    InitFileSystem();
  }
  else
  {
    FFileSystem->Open();
  }
}

void TTerminal::InitFileSystem()
{
  DebugAssert(FFileSystem == nullptr);
  try
  {
    TFSProtocol FSProtocol = GetSessionData()->GetFSProtocol();
    if ((FSProtocol == fsFTP) && (GetSessionData()->GetFtps() == ftpsNone))
    {
#ifdef NO_FILEZILLA
      LogEvent("FTP protocol is not supported by this build.");
      FatalError(nullptr, LoadStr(FTP_UNSUPPORTED));
#else
      FFSProtocol = cfsFTP;
      FFileSystem = new TFTPFileSystem(this);
      FFileSystem->Init(nullptr);
      FFileSystem->Open();
      GetLog()->AddSeparator();
      LogEvent("Using FTP protocol.");
#endif
    }
    else if ((FSProtocol == fsFTP) && (GetSessionData()->GetFtps() != ftpsNone))
    {
#if defined(NO_FILEZILLA) || defined(MPEXT_NO_SSLDLL)
      LogEvent("FTPS protocol is not supported by this build.");
      FatalError(nullptr, LoadStr(FZ_NOTSUPPORTED));
#else
      FFSProtocol = cfsFTPS;
      FFileSystem = new TFTPFileSystem(this);
      FFileSystem->Init(nullptr);
      FFileSystem->Open();
      GetLog()->AddSeparator();
      LogEvent("Using FTPS protocol.");
#endif
    }
    else if (FSProtocol == fsWebDAV)
    {
      FFSProtocol = cfsWebDAV;
      FFileSystem = new TWebDAVFileSystem(this);
      FFileSystem->Init(nullptr);
      FFileSystem->Open();
      GetLog()->AddSeparator();
      LogEvent("Using WebDAV protocol.");
    }
    else
    {
      DebugAssert(FSecureShell == nullptr);
      {
        SCOPE_EXIT
        {
          SAFE_DESTROY(FSecureShell);
        };
        FSecureShell = new TSecureShell(this, FSessionData, GetLog(), FConfiguration);
        try
        {
          // there will be only one channel in this session
          FSecureShell->SetSimple(true);
          FSecureShell->Open();
        }
        catch (Exception & E)
        {
          DebugAssert(!FSecureShell->GetActive());
          if (FSessionData->GetFingerprintScan())
          {
            FFingerprintScanned = FSecureShell->GetHostKeyFingerprint();
          }
          if (!FSecureShell->GetActive() && !FTunnelError.IsEmpty())
          {
            // the only case where we expect this to happen
            UnicodeString ErrorMessage = LoadStr(UNEXPECTED_CLOSE_ERROR);
            DebugAssert(E.Message == ErrorMessage);
            FatalError(&E, FMTLOAD(TUNNEL_ERROR, FTunnelError.c_str()));
          }
          else
          {
            throw;
          }
        }

        GetLog()->AddSeparator();

        if ((FSProtocol == fsSCPonly) ||
            (FSProtocol == fsSFTP && FSecureShell->SshFallbackCmd()))
        {
          FFSProtocol = cfsSCP;
          FFileSystem= new TSCPFileSystem(this);
          FFileSystem->Init(FSecureShell);
          FSecureShell = nullptr; // ownership passed
          LogEvent("Using SCP protocol.");
        }
        else
        {
          FFSProtocol = cfsSFTP;
          FFileSystem = new TSFTPFileSystem(this);
          FFileSystem->Init(FSecureShell);
          FSecureShell = nullptr; // ownership passed
          LogEvent("Using SFTP protocol.");
        }
      }
    }
  }
  catch (EFatal &)
  {
    SAFE_DESTROY(FFileSystem);
    throw;
  }
  catch (Exception & E)
  {
    // any exception while opening session is fatal
    FatalError(&E, L"");
  }
}

bool TTerminal::IsListenerFree(uintptr_t PortNumber) const
{
#ifndef __linux__
  SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
  bool Result = (Socket != INVALID_SOCKET);
  if (Result)
  {
    SOCKADDR_IN Address;

    ClearStruct(Address);
    Address.sin_family = AF_INET;
    Address.sin_port = htons(static_cast<short>(PortNumber));
    Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Result = (bind(Socket, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)) == 0);
    closesocket(Socket);
  }
  return Result;
#else
  return true;
#endif
}

void TTerminal::SetupTunnelLocalPortNumber()
{
  DebugAssert(FTunnelData == nullptr);

  FTunnelLocalPortNumber = FSessionData->GetTunnelLocalPortNumber();
  if (FTunnelLocalPortNumber == 0)
  {
    FTunnelLocalPortNumber = FConfiguration->GetTunnelLocalPortNumberLow();
    while (!IsListenerFree(FTunnelLocalPortNumber))
    {
      FTunnelLocalPortNumber++;
      if (FTunnelLocalPortNumber > FConfiguration->GetTunnelLocalPortNumberHigh())
      {
        FTunnelLocalPortNumber = 0;
        FatalError(nullptr, FMTLOAD(TUNNEL_NO_FREE_PORT,
          FConfiguration->GetTunnelLocalPortNumberLow(), FConfiguration->GetTunnelLocalPortNumberHigh()));
      }
    }
    LogEvent(FORMAT(L"Autoselected tunnel local port number %d", FTunnelLocalPortNumber));
  }
}

void TTerminal::OpenTunnel()
{
  SetupTunnelLocalPortNumber();

  try
  {
    FTunnelData = new TSessionData(L"");
    FTunnelData->Assign(StoredSessions->GetDefaultSettings());
    FTunnelData->SetName(FMTLOAD(TUNNEL_SESSION_NAME, FSessionData->GetSessionName().c_str()));
    FTunnelData->SetTunnel(false);
    FTunnelData->SetHostName(FSessionData->GetTunnelHostName());
    FTunnelData->SetPortNumber(FSessionData->GetTunnelPortNumber());
    FTunnelData->SetUserName(FSessionData->GetTunnelUserName());
    FTunnelData->SetPassword(FSessionData->GetTunnelPassword());
    FTunnelData->SetPublicKeyFile(FSessionData->GetTunnelPublicKeyFile());
    FTunnelData->SetTunnelPortFwd(FORMAT(L"L%d\t%s:%d",
      FTunnelLocalPortNumber, FSessionData->GetHostNameExpanded().c_str(), FSessionData->GetPortNumber()));
    FTunnelData->SetHostKey(FSessionData->GetTunnelHostKey());
    FTunnelData->SetProxyMethod(FSessionData->GetProxyMethod());
    FTunnelData->SetProxyHost(FSessionData->GetProxyHost());
    FTunnelData->SetProxyPort(FSessionData->GetProxyPort());
    FTunnelData->SetProxyUsername(FSessionData->GetProxyUsername());
    FTunnelData->SetProxyPassword(FSessionData->GetProxyPassword());
    FTunnelData->SetProxyTelnetCommand(FSessionData->GetProxyTelnetCommand());
    FTunnelData->SetProxyLocalCommand(FSessionData->GetProxyLocalCommand());
    FTunnelData->SetProxyDNS(FSessionData->GetProxyDNS());
    FTunnelData->SetProxyLocalhost(FSessionData->GetProxyLocalhost());

    FTunnelLog = new TSessionLog(this, FTunnelData, FConfiguration);
    FTunnelLog->SetParent(FLog);
    FTunnelLog->SetName(L"Tunnel");
    FTunnelLog->ReflectSettings();
    FTunnelUI = new TTunnelUI(this);
    FTunnel = new TSecureShell(FTunnelUI, FTunnelData, FTunnelLog, FConfiguration);

    FTunnelOpening = true;
    try__finally
    {
      SCOPE_EXIT
      {
        FTunnelOpening = false;
      };
      FTunnel->Open();
    }
    __finally
    {
      FTunnelOpening = false;
    };

    FTunnelThread = new TTunnelThread(FTunnel);
    FTunnelThread->Init();
  }
  catch (...)
  {
    CloseTunnel();
    throw;
  }
}

void TTerminal::CloseTunnel()
{
  SAFE_DESTROY_EX(TTunnelThread, FTunnelThread);
  FTunnelError = FTunnel->GetLastTunnelError();
  SAFE_DESTROY_EX(TSecureShell, FTunnel);
  SAFE_DESTROY_EX(TTunnelUI, FTunnelUI);
  SAFE_DESTROY_EX(TSessionLog, FTunnelLog);
  SAFE_DESTROY(FTunnelData);

  FTunnelLocalPortNumber = 0;
}

void TTerminal::Closed()
{
  if (FTunnel != nullptr)
  {
    CloseTunnel();
  }

  if (GetOnClose())
  {
    TCallbackGuard Guard(this);
    GetOnClose()(this);
    Guard.Verify();
  }

  FStatus = ssClosed;
}

void TTerminal::ProcessGUI()
{
  // Do not process GUI here, as we are called directly from a GUI loop and may
  // recurse for good.
  // Alternatively we may check for (FOperationProgress == NULL)
  if (FNesting == 0)
  {
    TAutoNestingCounter NestingCounter(FNesting);
    ::ProcessGUI();
  }
}

void TTerminal::Progress(TFileOperationProgressType * OperationProgress)
{
  if (FNesting == 0)
  {
    TAutoNestingCounter NestingCounter(FNesting);
    OperationProgress->Progress();
  }
}

void TTerminal::Reopen(intptr_t Params)
{
  TFSProtocol OrigFSProtocol = GetSessionData()->GetFSProtocol();
  UnicodeString PrevRemoteDirectory = GetSessionData()->GetRemoteDirectory();
  bool PrevReadCurrentDirectoryPending = FReadCurrentDirectoryPending;
  bool PrevReadDirectoryPending = FReadDirectoryPending;
  DebugAssert(!FSuspendTransaction);
  bool PrevAutoReadDirectory = FAutoReadDirectory;
  // here used to be a check for FExceptionOnFail being 0
  // but it can happen, e.g. when we are downloading file to execute it.
  // however I'm not sure why we mind having exception-on-fail enabled here
  Integer PrevExceptionOnFail = FExceptionOnFail;
  {
    SCOPE_EXIT
    {
      GetSessionData()->SetRemoteDirectory(PrevRemoteDirectory);
      GetSessionData()->SetFSProtocol(OrigFSProtocol);
      FAutoReadDirectory = PrevAutoReadDirectory;
      FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
      FReadDirectoryPending = PrevReadDirectoryPending;
      FSuspendTransaction = false;
      FExceptionOnFail = PrevExceptionOnFail;
    };
    FReadCurrentDirectoryPending = false;
    FReadDirectoryPending = false;
    FSuspendTransaction = true;
    FExceptionOnFail = 0;
    // typically, we avoid reading directory, when there is operation ongoing,
    // for file list which may reference files from current directory
    if (FLAGSET(Params, ropNoReadDirectory))
    {
      SetAutoReadDirectory(false);
    }

    // only peek, we may not be connected at all atm,
    // so make sure we do not try retrieving current directory from the server
    // (particularly with FTP)
    UnicodeString ACurrentDirectory = PeekCurrentDirectory();
    if (!ACurrentDirectory.IsEmpty())
    {
      GetSessionData()->SetRemoteDirectory(ACurrentDirectory);
    }
    if (GetSessionData()->GetFSProtocol() == fsSFTP)
    {
      GetSessionData()->SetFSProtocol(FFSProtocol == cfsSCP ? fsSCPonly : fsSFTPonly);
    }

    // Could be active before, if fatal error occured in the secondary terminal.
    // But now, since we handle the secondary terminal's OnClose,
    // by closing outselves, it should not happen anymore.
    if (DebugAlwaysFalse(GetActive()))
    {
      Close();
    }

    Open();
  }
  __finally
  {
    GetSessionData()->SetRemoteDirectory(PrevRemoteDirectory);
    GetSessionData()->SetFSProtocol(OrigFSProtocol);
    FAutoReadDirectory = PrevAutoReadDirectory;
    FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
    FReadDirectoryPending = PrevReadDirectoryPending;
    FSuspendTransaction = false;
    FExceptionOnFail = PrevExceptionOnFail;
  };
}

bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & AName, const UnicodeString & Instructions,
  const UnicodeString & Prompt,
  bool Echo, intptr_t MaxLen, UnicodeString & AResult)
{
  bool Result = false;
  std::unique_ptr<TStrings> Prompts(new TStringList());
  std::unique_ptr<TStrings> Results(new TStringList());
  try__finally
  {
    Prompts->AddObject(Prompt, reinterpret_cast<TObject *>(FLAGMASK(Echo, pupEcho)));
    Results->AddObject(AResult, reinterpret_cast<TObject *>(MaxLen));
    Result = PromptUser(Data, Kind, AName, Instructions, Prompts.get(), Results.get());
    AResult = Results->GetString(0);
  }
  __finally
  {
//    delete Prompts;
//    delete Results;
  };
  return Result;
}

bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & AName, const UnicodeString & Instructions, TStrings * Prompts,
  TStrings * Results)
{
  // If PromptUser is overridden in descendant class, the overridden version
  // is not called when accessed via TSessionIU interface.
  // So this is workaround.
  // Actually no longer needed as we do not override DoPromptUser
  // anymore in TSecondaryTerminal.
  return DoPromptUser(Data, Kind, AName, Instructions, Prompts, Results);
}

TTerminal * TTerminal::GetPasswordSource()
{
  return this;
}

bool TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
  const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts, TStrings * Results)
{
  bool Result = false;

  bool PasswordOrPassphrasePrompt = ::IsPasswordOrPassphrasePrompt(Kind, Prompts);
  if (PasswordOrPassphrasePrompt)
  {
    bool & PasswordTried =
      FTunnelOpening ? FRememberedTunnelPasswordTried : FRememberedPasswordTried;
    if (!PasswordTried)
    {
      // let's expect that the main session is already authenticated and its password
      // is not written after, so no locking is necessary
      // (no longer true, once the main session can be reconnected)
      UnicodeString Password;
      if (FTunnelOpening)
      {
        Password = GetPasswordSource()->GetRememberedTunnelPassword();
      }
      else
      {
        Password = GetPasswordSource()->GetRememberedPassword();
      }
      Results->SetString(0, Password);
      if (!Results->GetString(0).IsEmpty())
      {
        LogEvent("Using remembered password.");
        Result = true;
      }
      PasswordTried = true;
    }
  }

  if (!Result)
  {
    if (PasswordOrPassphrasePrompt && !GetConfiguration()->GetRememberPassword())
    {
      Prompts->SetObj(0, reinterpret_cast<TObject*>(reinterpret_cast<intptr_t>(Prompts->GetObj(0)) | pupRemember));
    }

    if (GetOnPromptUser() != nullptr)
    {
      TCallbackGuard Guard(this);
      GetOnPromptUser()(this, Kind, Name, Instructions, Prompts, Results, Result, nullptr);
      Guard.Verify();
    }

    if (Result && PasswordOrPassphrasePrompt &&
        (GetConfiguration()->GetRememberPassword() || FLAGSET(reinterpret_cast<intptr_t>(Prompts->GetObj(0)), pupRemember)))
    {
      RawByteString EncryptedPassword = EncryptPassword(Results->GetString(0));
      if (FTunnelOpening)
      {
        GetPasswordSource()->SetRememberedTunnelPassword(EncryptedPassword);
      }
      else
      {
        GetPasswordSource()->SetRememberedPassword(EncryptedPassword);
      }
    }
  }

  return Result;
}

uintptr_t TTerminal::QueryUser(const UnicodeString & Query,
  TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  LogEvent(FORMAT(L"Asking user:\n%s (%s)", Query.c_str(), UnicodeString(MoreMessages ? MoreMessages->GetCommaText() : L"").c_str()));
  uintptr_t Answer = AbortAnswer(Answers);
  if (FOnQueryUser)
  {
    TCallbackGuard Guard(this);
    FOnQueryUser(this, Query, MoreMessages, Answers, Params, Answer, QueryType, nullptr);
    Guard.Verify();
  }
  return Answer;
}

uintptr_t TTerminal::QueryUserException(const UnicodeString & Query,
  Exception * E, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uintptr_t Result = 0;
  UnicodeString ExMessage;
  if (DebugAlwaysTrue(ExceptionMessage(E, ExMessage) || !Query.IsEmpty()))
  {
    std::unique_ptr<TStrings> MoreMessages(new TStringList());
    try__finally
    {
      if (!ExMessage.IsEmpty() && !Query.IsEmpty())
      {
        MoreMessages->Add(UnformatMessage(ExMessage));
      }

      ExtException * EE = NB_STATIC_DOWNCAST(ExtException, E);
      if ((EE != nullptr) && (EE->GetMoreMessages() != nullptr))
      {
        MoreMessages->AddStrings(EE->GetMoreMessages());
      }

      // We know MoreMessages not to be NULL here,
      // AppendExceptionStackTraceAndForget should never return true
      // (indicating it had to create the string list)
      TStrings * MoreMessagesPtr = MoreMessages.release();
      DebugAlwaysFalse(AppendExceptionStackTraceAndForget(MoreMessagesPtr));
      MoreMessages.reset(MoreMessagesPtr);

      TQueryParams HelpKeywordOverrideParams;
      if (Params != nullptr)
      {
        HelpKeywordOverrideParams.Assign(*Params);
      }
      HelpKeywordOverrideParams.HelpKeyword =
        MergeHelpKeyword(HelpKeywordOverrideParams.HelpKeyword, GetExceptionHelpKeyword(E));

      Result = QueryUser(!Query.IsEmpty() ? Query : ExMessage,
        MoreMessages->GetCount() ? MoreMessages.get() : nullptr,
        Answers, &HelpKeywordOverrideParams, QueryType);
    }
    __finally
    {
//      delete MoreMessages;
    };
  }
  return Result;
}

void TTerminal::DisplayBanner(const UnicodeString & Banner)
{
  if (GetOnDisplayBanner() != nullptr)
  {
    if (FConfiguration->GetForceBanners() ||
        FConfiguration->ShowBanner(GetSessionData()->GetSessionKey(), Banner))
    {
      bool NeverShowAgain = false;
      intptr_t Options =
        FLAGMASK(FConfiguration->GetForceBanners(), boDisableNeverShowAgain);
      TCallbackGuard Guard(this);
      GetOnDisplayBanner()(this, GetSessionData()->GetSessionName(), Banner,
        NeverShowAgain, Options);
      Guard.Verify();
      if (!FConfiguration->GetForceBanners() && NeverShowAgain)
      {
        FConfiguration->NeverShowBanner(GetSessionData()->GetSessionKey(), Banner);
      }
    }
  }
}

void TTerminal::HandleExtendedException(Exception * E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != nullptr)
  {
    TCallbackGuard Guard(this);
    // the event handler may destroy 'this' ...
    GetOnShowExtendedException()(this, E, nullptr);
    // .. hence guard is dismissed from destructor, to make following call no-op
    Guard.Verify();
  }
}

void TTerminal::ShowExtendedException(Exception * E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != nullptr)
  {
    GetOnShowExtendedException()(this, E, nullptr);
  }
}

void TTerminal::DoInformation(const UnicodeString & Str, bool Status,
  intptr_t Phase)
{
  if (GetOnInformation())
  {
    TCallbackGuard Guard(this);
    GetOnInformation()(this, Str, Status, Phase);
    Guard.Verify();
  }
}

void TTerminal::Information(const UnicodeString & Str, bool Status)
{
  DoInformation(Str, Status);
}

void TTerminal::DoProgress(TFileOperationProgressType & ProgressData)
{
  if (GetOnProgress() != nullptr)
  {
    TCallbackGuard Guard(this);
    GetOnProgress()(ProgressData);
    Guard.Verify();
  }
}

void TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
  const UnicodeString & AFileName, bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  if (GetOnFinished() != nullptr)
  {
    TCallbackGuard Guard(this);
    GetOnFinished()(Operation, Side, Temp, AFileName, Success, OnceDoneOperation);
    Guard.Verify();
  }
}

void TTerminal::SaveCapabilities(TFileSystemInfo & FileSystemInfo)
{
  for (intptr_t Index = 0; Index < fcCount; ++Index)
  {
    FileSystemInfo.IsCapable[Index] = GetIsCapable(static_cast<TFSCapability>(Index));
  }
}

bool TTerminal::GetIsCapable(TFSCapability Capability) const
{
  DebugAssert(FFileSystem);
  return FFileSystem->IsCapable(Capability);
}

UnicodeString TTerminal::GetAbsolutePath(const UnicodeString & APath, bool Local) const
{
  return FFileSystem->GetAbsolutePath(APath, Local);
}

void TTerminal::ReactOnCommand(intptr_t Cmd)
{
  bool ChangesDirectory = false;
  bool ModifiesFiles = false;

  switch (static_cast<TFSCommand>(Cmd))
  {
    case fsChangeDirectory:
    case fsHomeDirectory:
      ChangesDirectory = true;
      break;

    case fsCopyToRemote:
    case fsDeleteFile:
    case fsRenameFile:
    case fsMoveFile:
    case fsCopyFile:
    case fsCreateDirectory:
    case fsChangeMode:
    case fsChangeGroup:
    case fsChangeOwner:
    case fsChangeProperties:
    case fsLock:
      ModifiesFiles = true;
      break;

    case fsAnyCommand:
      ChangesDirectory = true;
      ModifiesFiles = true;
      break;
  }

  if (ChangesDirectory)
  {
    if (!InTransaction())
    {
      ReadCurrentDirectory();
      if (GetAutoReadDirectory())
      {
        ReadDirectory(false);
      }
    }
    else
    {
      FReadCurrentDirectoryPending = true;
      if (GetAutoReadDirectory())
      {
        FReadDirectoryPending = true;
      }
    }
  }
  else if (ModifiesFiles && GetAutoReadDirectory() && FConfiguration->GetAutoReadDirectoryAfterOp())
  {
    if (!InTransaction())
    {
      ReadDirectory(true);
    }
    else
    {
      FReadDirectoryPending = true;
    }
  }
}

void TTerminal::TerminalError(const UnicodeString & Msg, const UnicodeString & HelpKeyword)
{
  TerminalError(nullptr, Msg, HelpKeyword);
}

void TTerminal::TerminalError(
  Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword)
{
  throw ETerminal(E, Msg, HelpKeyword);
}

bool TTerminal::DoQueryReopen(Exception * E)
{
  EFatal * Fatal = NB_STATIC_DOWNCAST(EFatal, E);
  DebugAssert(Fatal != nullptr);
  bool Result = false;
  if ((Fatal != nullptr) && Fatal->GetReopenQueried())
  {
    Result = false;
  }
  else
  {
    intptr_t NumberOfRetries = FSessionData->GetNumberOfRetries();
    if (FConfiguration->GetSessionReopenAutoMaximumNumberOfRetries() > 0 && NumberOfRetries >= FConfiguration->GetSessionReopenAutoMaximumNumberOfRetries())
    {
      LogEvent(FORMAT(L"Reached maximum number of retries: %d", FConfiguration->GetSessionReopenAutoMaximumNumberOfRetries()));
    }
    else
    {
      LogEvent("Connection was lost, asking what to do.");

      NumberOfRetries++;
      FSessionData->SetNumberOfRetries(NumberOfRetries);

      TQueryParams Params(qpAllowContinueOnError);
      Params.Timeout = FConfiguration->GetSessionReopenAuto();
      Params.TimeoutAnswer = qaRetry;
      TQueryButtonAlias Aliases[1];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(RECONNECT_BUTTON);
      Aliases[0].Default = true;
      Params.Aliases = Aliases;
      Params.AliasesCount = _countof(Aliases);
      Result = (QueryUserException(L"", E, qaRetry | qaAbort, &Params, qtError) == qaRetry);
    }

    if (Fatal != nullptr)
    {
      Fatal->SetReopenQueried(true);
    }
  }
  return Result;
}

bool TTerminal::QueryReopen(Exception * E, intptr_t Params,
  TFileOperationProgressType * OperationProgress)
{
  TSuspendFileOperationProgress Suspend(OperationProgress);

  bool Result = DoQueryReopen(E);

  if (Result)
  {
    TDateTime Start = Now();
    do
    {
      try
      {
        Reopen(Params);
        FSessionData->SetNumberOfRetries(0);
      }
      catch (Exception & E)
      {
        if (!GetActive())
        {
          Result =
            ((FConfiguration->GetSessionReopenTimeout() == 0) ||
             (static_cast<intptr_t>((double)(Now() - Start) * MSecsPerDay) < FConfiguration->GetSessionReopenTimeout())) &&
            DoQueryReopen(&E);
        }
        else
        {
          throw;
        }
      }
    }
    while (!GetActive() && Result);
  }

  return Result;
}

bool TTerminal::FileOperationLoopQuery(Exception & E,
  TFileOperationProgressType * OperationProgress, const UnicodeString & Message,
  bool AllowSkip, const UnicodeString & SpecialRetry, const UnicodeString & HelpKeyword)
{
  bool Result = false;
  GetLog()->AddException(&E);
  uintptr_t Answer;
  bool SkipToAllPossible = AllowSkip && (OperationProgress != nullptr);

  if (SkipToAllPossible && OperationProgress->SkipToAll)
  {
    Answer = qaSkip;
  }
  else
  {
    uintptr_t Answers = qaRetry | qaAbort |
      FLAGMASK(AllowSkip, qaSkip) |
      FLAGMASK(SkipToAllPossible, qaAll) |
      FLAGMASK(!SpecialRetry.IsEmpty(), qaYes);
    TQueryParams Params(qpAllowContinueOnError | FLAGMASK(!AllowSkip, qpFatalAbort));
    Params.HelpKeyword = HelpKeyword;
    TQueryButtonAlias Aliases[2];
    intptr_t AliasCount = 0;

    if (FLAGSET(Answers, qaAll))
    {
      Aliases[AliasCount].Button = qaAll;
      Aliases[AliasCount].Alias = LoadStr(SKIP_ALL_BUTTON);
      AliasCount++;
    }
    if (FLAGSET(Answers, qaYes))
    {
      Aliases[AliasCount].Button = qaYes;
      Aliases[AliasCount].Alias = SpecialRetry;
      AliasCount++;
    }

    if (AliasCount > 0)
    {
      Params.Aliases = Aliases;
      Params.AliasesCount = AliasCount;
    }

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      Answer = QueryUserException(Message, &E, Answers, &Params, qtError);
    }

    if (Answer == qaAll)
    {
      DebugAssert(OperationProgress != nullptr);
      OperationProgress->SkipToAll = true;
      Answer = qaSkip;
    }
    if (Answer == qaYes)
    {
      Result = true;
      Answer = qaRetry;
    }
  }

  if (Answer != qaRetry)
  {
    if ((Answer == qaAbort) && (OperationProgress != nullptr))
    {
      OperationProgress->Cancel = csCancel;
    }

    if (AllowSkip)
    {
      ThrowSkipFile(&E, Message);
    }
    else
    {
      // this can happen only during file transfer with SCP
      throw ExtException(&E, Message);
    }
  }

  return Result;
}

intptr_t TTerminal::FileOperationLoop(TFileOperationEvent CallBackFunc,
  TFileOperationProgressType * OperationProgress, bool AllowSkip,
  const UnicodeString & Message, void * Param1, void * Param2)
{
  DebugAssert(CallBackFunc);
  intptr_t Result = 0;
  FileOperationLoopCustom(this, OperationProgress, AllowSkip, Message, "",
  [&]()
  {
    Result = CallBackFunc(Param1, Param2);
  });

  return Result;
}

UnicodeString TTerminal::TranslateLockedPath(const UnicodeString & APath, bool Lock)
{
  UnicodeString Result = APath;
  if (GetSessionData()->GetLockInHome() && !Result.IsEmpty() && (Result[1] == L'/'))
  {
    if (Lock)
    {
      if (Result.SubString(1, FLockDirectory.Length()) == FLockDirectory)
      {
        Result.Delete(1, FLockDirectory.Length());
        if (Result.IsEmpty())
        {
          Result = ROOTDIRECTORY;
        }
      }
    }
    else
    {
      Result = core::UnixExcludeTrailingBackslash(FLockDirectory + Result);
    }
  }
  return Result;
}

void TTerminal::ClearCaches()
{
  FDirectoryCache->Clear();
  if (FDirectoryChangesCache != nullptr)
  {
    FDirectoryChangesCache->Clear();
  }
  if (FCommandSession != nullptr)
  {
    FCommandSession->ClearCaches();
  }
}

void TTerminal::ClearCachedFileList(const UnicodeString & APath,
  bool SubDirs)
{
  FDirectoryCache->ClearFileList(APath, SubDirs);
}

void TTerminal::AddCachedFileList(TRemoteFileList * FileList)
{
  FDirectoryCache->AddFileList(FileList);
}

bool TTerminal::DirectoryFileList(const UnicodeString & APath,
  TRemoteFileList *& FileList, bool CanLoad)
{
  bool Result = false;
  if (core::UnixSamePath(FFiles->GetDirectory(), APath))
  {
    Result = (FileList == nullptr) || (FileList->GetTimestamp() < FFiles->GetTimestamp());
    if (Result)
    {
      if (FileList == nullptr)
      {
        FileList = new TRemoteFileList();
      }
      FFiles->DuplicateTo(FileList);
    }
  }
  else
  {
    if (((FileList == nullptr) && FDirectoryCache->HasFileList(APath)) ||
        ((FileList != nullptr) && FDirectoryCache->HasNewerFileList(APath, FileList->GetTimestamp())))
    {
      bool Created = (FileList == nullptr);
      if (Created)
      {
        FileList = new TRemoteFileList();
      }

      Result = FDirectoryCache->GetFileList(APath, FileList);
      if (!Result && Created)
      {
        SAFE_DESTROY(FileList);
      }
    }
    // do not attempt to load file list if there is cached version,
    // only absence of cached version indicates that we consider
    // the directory content obsolete
    else if (CanLoad && !FDirectoryCache->HasFileList(APath))
    {
      bool Created = (FileList == nullptr);
      if (Created)
      {
        FileList = new TRemoteFileList();
      }
      FileList->SetDirectory(APath);

      try
      {
        ReadDirectory(FileList);
        Result = true;
      }
      catch (...)
      {
        if (Created)
        {
          SAFE_DESTROY(FileList);
        }
        throw;
      }
    }
  }

  return Result;
}

void TTerminal::TerminalSetCurrentDirectory(const UnicodeString & AValue)
{
  DebugAssert(FFileSystem);
  UnicodeString Value = TranslateLockedPath(AValue, false);
  if (Value != FFileSystem->GetCurrDirectory())
  {
    RemoteChangeDirectory(Value);
  }
}

UnicodeString TTerminal::GetCurrDirectory()
{
  if (FFileSystem != nullptr)
  {
    // there's occasional crash when assigning FFileSystem->CurrentDirectory
    // to FCurrentDirectory, splitting the assignment to two statements
    // to locate the crash more closely
    UnicodeString CurrentDirectory = FFileSystem->GetCurrDirectory();
    if (FCurrentDirectory != CurrentDirectory)
    {
      FCurrentDirectory = CurrentDirectory;
      if (FCurrentDirectory.IsEmpty())
      {
        ReadCurrentDirectory();
      }
    }
  }

  UnicodeString Result = TranslateLockedPath(FCurrentDirectory, true);
  return Result;
}

UnicodeString TTerminal::PeekCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->GetCurrDirectory();
  }

  UnicodeString Result = TranslateLockedPath(FCurrentDirectory, true);
  return Result;
}

TRemoteTokenList * TTerminal::GetGroups()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FGroups;
}

TRemoteTokenList * TTerminal::GetUsers()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FUsers;
}

TRemoteTokenList * TTerminal::GetMembership()
{
  DebugAssert(FFileSystem);
  LookupUsersGroups();
  return &FMembership;
}

UnicodeString TTerminal::TerminalGetUserName() const
{
  // in future might also be implemented to detect username similar to GetUserGroups
  DebugAssert(FFileSystem != nullptr);
  UnicodeString Result = FFileSystem->FSGetUserName();
  // Is empty also when stored username was used
  if (Result.IsEmpty())
  {
    Result = GetSessionData()->GetUserNameExpanded();
  }
  return Result;
}

bool TTerminal::GetAreCachesEmpty() const
{
  return FDirectoryCache->GetIsEmpty() &&
    ((FDirectoryChangesCache == nullptr) || FDirectoryChangesCache->GetIsEmpty());
}

void TTerminal::DoInitializeLog()
{
  if (FOnInitializeLog)
  {
    TCallbackGuard Guard(this);
    FOnInitializeLog(this);
    Guard.Verify();
  }
}

void TTerminal::DoChangeDirectory()
{
  if (FOnChangeDirectory)
  {
    TCallbackGuard Guard(this);
    FOnChangeDirectory(this);
    Guard.Verify();
  }
}

void TTerminal::DoReadDirectory(bool ReloadOnly)
{
  if (FOnReadDirectory)
  {
    TCallbackGuard Guard(this);
    FOnReadDirectory(this, ReloadOnly);
    Guard.Verify();
  }
}

void TTerminal::DoStartReadDirectory()
{
  if (FOnStartReadDirectory)
  {
    TCallbackGuard Guard(this);
    FOnStartReadDirectory(this);
    Guard.Verify();
  }
}

void TTerminal::DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool & Cancel)
{
  if (FReadingCurrentDirectory && (FOnReadDirectoryProgress != nullptr))
  {
    TCallbackGuard Guard(this);
    FOnReadDirectoryProgress(this, Progress, ResolvedLinks, Cancel);
    Guard.Verify();
  }
  if (FOnFindingFile != nullptr)
  {
    TCallbackGuard Guard(this);
    FOnFindingFile(this, L"", Cancel);
    Guard.Verify();
  }
}

bool TTerminal::InTransaction() const
{
  return (FInTransaction > 0) && !FSuspendTransaction;
}

void TTerminal::BeginTransaction()
{
  if (FInTransaction == 0)
  {
    FReadCurrentDirectoryPending = false;
    FReadDirectoryPending = false;
  }
  FInTransaction++;

  if (FCommandSession != nullptr)
  {
    FCommandSession->BeginTransaction();
  }
}

void TTerminal::EndTransaction()
{
  DoEndTransaction(false);
}

void TTerminal::DoEndTransaction(bool Inform)
{
  if (FInTransaction == 0)
    TerminalError(L"Can't end transaction, not in transaction");
  DebugAssert(FInTransaction > 0);
  FInTransaction--;

  // it connection was closed due to fatal error during transaction, do nothing
  if (GetActive())
  {
    if (FInTransaction == 0)
    {
      try__finally
      {
        SCOPE_EXIT
        {
          FReadCurrentDirectoryPending = false;
          FReadDirectoryPending = false;
        };
        if (FReadCurrentDirectoryPending)
        {
          ReadCurrentDirectory();
        }

        if (FReadDirectoryPending)
        {
          if (Inform)
          {
            DoInformation(LoadStr(STATUS_OPEN_DIRECTORY), true);
          }
          ReadDirectory(!FReadCurrentDirectoryPending);
        }
      }
      __finally
      {
        FReadCurrentDirectoryPending = false;
        FReadDirectoryPending = false;
      };
    }
  }

  if (FCommandSession != nullptr)
  {
    FCommandSession->EndTransaction();
  }
}

void TTerminal::SetExceptionOnFail(bool Value)
{
  if (Value)
  {
    FExceptionOnFail++;
  }
  else
  {
    if (FExceptionOnFail == 0)
      throw Exception(L"ExceptionOnFail is already zero.");
    FExceptionOnFail--;
  }

  if (FCommandSession != nullptr)
  {
    FCommandSession->FExceptionOnFail = FExceptionOnFail;
  }
}

bool TTerminal::GetExceptionOnFail() const
{
  return static_cast<bool>(FExceptionOnFail > 0);
}

void TTerminal::FatalAbort()
{
  FatalError(nullptr, L"");
}

void TTerminal::FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword)
{
  bool SecureShellActive = (FSecureShell != nullptr) && FSecureShell->GetActive();
  if (GetActive() || SecureShellActive)
  {
    // We log this instead of exception handler, because Close() would
    // probably cause exception handler to loose pointer to TShellLog()
    LogEvent("Attempt to close connection due to fatal exception:");
    GetLog()->Add(llException, Msg);
    GetLog()->AddException(E);

    if (GetActive())
    {
      Close();
    }

    // this may happen if failure of authentication of SSH, owned by terminal yet
    // (because the protocol was not decided yet), is detected by us (not by putty).
    // e.g. not verified host key
    if (SecureShellActive)
    {
      FSecureShell->Close();
    }
  }

  if (FCallbackGuard != nullptr)
  {
    FCallbackGuard->FatalError(E, Msg, HelpKeyword);
  }
  else
  {
    throw ESshFatal(E, Msg, HelpKeyword);
  }
}

void TTerminal::CommandError(Exception * E, const UnicodeString & Msg)
{
  CommandError(E, Msg, 0);
}

uintptr_t TTerminal::CommandError(Exception * E, const UnicodeString & Msg,
  uintptr_t Answers, const UnicodeString & HelpKeyword)
{
  // may not be, particularly when TTerminal::Reopen is being called
  // from within OnShowExtendedException handler
  DebugAssert(FCallbackGuard == nullptr);
  uintptr_t Result = 0;
  if (E && (NB_STATIC_DOWNCAST(EFatal, E) != nullptr))
  {
    FatalError(E, Msg, HelpKeyword);
  }
  else if (E && (NB_STATIC_DOWNCAST(EAbort, E) != nullptr))
  {
    // resent EAbort exception
    Abort();
  }
  else if (GetExceptionOnFail())
  {
    throw ECommand(E, Msg, HelpKeyword);
  }
  else if (!Answers)
  {
    ECommand ECmd(E, Msg, HelpKeyword);
    try__finally
    {
      HandleExtendedException(&ECmd);
    }
    __finally
    {
//      delete ECmd;
    };
  }
  else
  {
    // small hack to enable "skip to all" for TRetryOperationLoop
    bool CanSkip = FLAGSET(Answers, qaSkip) && (GetOperationProgress() != nullptr);
    if (CanSkip && GetOperationProgress()->SkipToAll)
    {
      Result = qaSkip;
    }
    else
    {
      TQueryParams Params(qpAllowContinueOnError, HelpKeyword);
      TQueryButtonAlias Aliases[1];
      if (CanSkip)
      {
        Aliases[0].Button = qaAll;
        Aliases[0].Alias = LoadStr(SKIP_ALL_BUTTON);
        Params.Aliases = Aliases;
        Params.AliasesCount = _countof(Aliases);
        Answers |= qaAll;
      }
      Result = QueryUserException(Msg, E, Answers, &Params, qtError);
      if (Result == qaAll)
      {
        DebugAssert(GetOperationProgress() != nullptr);
        GetOperationProgress()->SkipToAll = true;
        Result = qaSkip;
      }
    }
  }
  return Result;
}

bool TTerminal::HandleException(Exception * E)
  {
  if (GetExceptionOnFail())
  {
    return false;
  }
  else
  {
    GetLog()->AddException(E);
    return true;
  }
}

void TTerminal::CloseOnCompletion(TOnceDoneOperation Operation, const UnicodeString & Message)
{
//  Configuration->Usage->Inc(L"ClosesOnCompletion");
  LogEvent("Closing session after completed operation (as requested by user)");
  Close();
  throw ESshTerminate(nullptr,
    Message.IsEmpty() ? UnicodeString(LoadStr(CLOSED_ON_COMPLETION)) : Message,
    Operation);
}

TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
  const UnicodeString & ASourceFullFileName, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress, bool Special)
{
  TBatchOverwrite Result;
  if (Special &&
      (FLAGSET(Params, cpResume) || CopyParam->ResumeTransfer(ASourceFullFileName)))
  {
    Result = boResume;
  }
  else if (FLAGSET(Params, cpAppend))
  {
    Result = boAppend;
  }
  else if (CopyParam->GetNewerOnly() &&
           (((OperationProgress->Side == osLocal) && GetIsCapable(fcNewerOnlyUpload)) ||
            (OperationProgress->Side != osLocal)))
  {
    // no way to change batch overwrite mode when CopyParam->NewerOnly is on
    Result = boOlder;
  }
  else if (FLAGSET(Params, cpNoConfirmation) || !FConfiguration->GetConfirmOverwriting())
  {
    // no way to change batch overwrite mode when overwrite confirmations are off
    DebugAssert(OperationProgress->BatchOverwrite == boNo);
    Result = boAll;
  }
  else
  {
    Result = OperationProgress->BatchOverwrite;
    if (!Special &&
        ((Result == boOlder) || (Result == boAlternateResume) || (Result == boResume)))
    {
      Result = boNo;
    }
  }
  return Result;
}

bool TTerminal::CheckRemoteFile(
   const UnicodeString & AFileName, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress)
{
  return (EffectiveBatchOverwrite(AFileName, CopyParam, Params, OperationProgress, true) != boAll);
}

uintptr_t TTerminal::ConfirmFileOverwrite(const UnicodeString & ASourceFullFileName, const UnicodeString & ATargetFileName,
  const TOverwriteFileParams * FileParams, uintptr_t Answers, TQueryParams * QueryParams,
  TOperationSide Side, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress,
  const UnicodeString & AMessage)
{
  UnicodeString Message = AMessage;
  uintptr_t Result = 0;
  // duplicated in TSFTPFileSystem::SFTPConfirmOverwrite
  bool CanAlternateResume =
    (FileParams != nullptr) &&
    (FileParams->DestSize < FileParams->SourceSize) &&
    !OperationProgress->AsciiTransfer;
  TBatchOverwrite BatchOverwrite = EffectiveBatchOverwrite(ASourceFullFileName, CopyParam, Params, OperationProgress, true);
  bool Applicable = true;
  switch (BatchOverwrite)
  {
    case boOlder:
      Applicable = (FileParams != nullptr);
      break;

    case boAlternateResume:
      Applicable = CanAlternateResume;
      break;

    case boResume:
      Applicable = CanAlternateResume;
      break;
  }

  if (!Applicable)
  {
    TBatchOverwrite EffBatchOverwrite = EffectiveBatchOverwrite(ASourceFullFileName, CopyParam, Params, OperationProgress, false);
    DebugAssert(BatchOverwrite != EffBatchOverwrite);
    BatchOverwrite = EffBatchOverwrite;
  }

  if (BatchOverwrite == boNo)
  {
    if (Message.IsEmpty())
    {
      // Side refers to destination side here
      Message = FMTLOAD((Side == osLocal ? LOCAL_FILE_OVERWRITE2 :
        REMOTE_FILE_OVERWRITE2), ATargetFileName.c_str(), ATargetFileName.c_str());
    }
    if (FileParams != nullptr)
    {
      Message = FMTLOAD(FILE_OVERWRITE_DETAILS, Message.c_str(),
        ::Int64ToStr(FileParams->SourceSize).c_str(),
        core::UserModificationStr(FileParams->SourceTimestamp, FileParams->SourcePrecision).c_str(),
        ::Int64ToStr(FileParams->DestSize).c_str(),
        core::UserModificationStr(FileParams->DestTimestamp, FileParams->DestPrecision).c_str());
    }
    if (DebugAlwaysTrue(QueryParams->HelpKeyword.IsEmpty()))
    {
      QueryParams->HelpKeyword = HELP_OVERWRITE;
    }
    Result = QueryUser(Message, nullptr, Answers, QueryParams);
    switch (Result)
    {
      case qaNeverAskAgain:
        FConfiguration->SetConfirmOverwriting(false);
        Result = qaYes;
        break;

      case qaYesToAll:
        BatchOverwrite = boAll;
        break;

      case qaAll:
        BatchOverwrite = boOlder;
        break;

      case qaNoToAll:
        BatchOverwrite = boNone;
        break;
    }

    // we user has not selected another batch overwrite mode,
    // keep the current one. note that we may get here even
    // when batch overwrite was selected already, but it could not be applied
    // to current transfer (see condition above)
    if (BatchOverwrite != boNo)
    {
      GetOperationProgress()->BatchOverwrite = BatchOverwrite;
    }
  }

  if (BatchOverwrite != boNo)
  {
    switch (BatchOverwrite)
    {
      case boAll:
        Result = qaYes;
        break;

      case boNone:
        Result = qaNo;
        break;

      case boOlder:
        if (FileParams == nullptr)
        {
          Result = qaNo;
        }
        else
        {
          TModificationFmt Precision = core::LessDateTimePrecision(FileParams->SourcePrecision, FileParams->DestPrecision);
          TDateTime ReducedSourceTimestamp =
            core::ReduceDateTimePrecision(FileParams->SourceTimestamp, Precision);
          TDateTime ReducedDestTimestamp =
            core::ReduceDateTimePrecision(FileParams->DestTimestamp, Precision);

          Result =
             CompareFileTime(ReducedSourceTimestamp, ReducedDestTimestamp) > 0 ?
            qaYes : qaNo;

          LogEvent(FORMAT(L"Source file timestamp is [%s], destination timestamp is [%s], will%s overwrite",
             StandardTimestamp(ReducedSourceTimestamp).c_str(),
             StandardTimestamp(ReducedDestTimestamp).c_str(),
              UnicodeString(Result == qaYes ? L"" : L" not").c_str()));
        }
        break;

      case boAlternateResume:
        DebugAssert(CanAlternateResume);
        Result = qaSkip; // ugh
        break;

      case boAppend:
        Result = qaRetry;
        break;

      case boResume:
        Result = qaRetry;
        break;
    }
  }

  return Result;
}

void TTerminal::FileModified(const TRemoteFile * AFile,
  const UnicodeString & AFileName, bool ClearDirectoryChange)
{
  UnicodeString ParentDirectory;
  UnicodeString Directory;

  if (GetSessionData()->GetCacheDirectories() || GetSessionData()->GetCacheDirectoryChanges())
  {
    if ((AFile != nullptr) && (AFile->GetDirectory() != nullptr))
    {
      if (AFile->GetIsDirectory())
      {
        Directory = AFile->GetDirectory()->GetFullDirectory() + AFile->GetFileName();
      }
      ParentDirectory = AFile->GetDirectory()->GetDirectory();
    }
    else if (!AFileName.IsEmpty())
    {
      ParentDirectory = core::UnixExtractFilePath(AFileName);
      if (ParentDirectory.IsEmpty())
      {
        ParentDirectory = GetCurrDirectory();
      }

      // this case for scripting
      if ((AFile != nullptr) && AFile->GetIsDirectory())
      {
        Directory = core::UnixIncludeTrailingBackslash(ParentDirectory) +
          base::UnixExtractFileName(AFile->GetFileName());
      }
    }
  }

  if (GetSessionData()->GetCacheDirectories())
  {
    if (!Directory.IsEmpty())
    {
      DirectoryModified(Directory, true);
    }
    if (!ParentDirectory.IsEmpty())
    {
      DirectoryModified(ParentDirectory, false);
    }
  }

  if (GetSessionData()->GetCacheDirectoryChanges() && ClearDirectoryChange)
  {
    if (!Directory.IsEmpty())
    {
      FDirectoryChangesCache->ClearDirectoryChange(Directory);
      FDirectoryChangesCache->ClearDirectoryChangeTarget(Directory);
    }
  }
}

void TTerminal::DirectoryModified(const UnicodeString & APath, bool SubDirs)
{
  if (APath.IsEmpty())
  {
    ClearCachedFileList(GetCurrDirectory(), SubDirs);
  }
  else
  {
    ClearCachedFileList(APath, SubDirs);
  }
}

void TTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  AddCachedFileList(FileList);
}

void TTerminal::ReloadDirectory()
{
  if (GetSessionData()->GetCacheDirectories())
  {
    DirectoryModified(GetCurrDirectory(), false);
  }
  if (GetSessionData()->GetCacheDirectoryChanges())
  {
    DebugAssert(FDirectoryChangesCache != nullptr);
    FDirectoryChangesCache->ClearDirectoryChange(GetCurrDirectory());
  }

  ReadCurrentDirectory();
  FReadCurrentDirectoryPending = false;
  ReadDirectory(true);
  FReadDirectoryPending = false;
}

void TTerminal::RefreshDirectory()
{
  if (GetSessionData()->GetCacheDirectories())
  {
    LogEvent("Not refreshing directory, caching is off.");
  }
  else if (FDirectoryCache->HasNewerFileList(GetCurrDirectory(), FFiles->GetTimestamp()))
  {
    // Second parameter was added to allow (rather force) using the cache.
    // Before, the directory was reloaded always, it seems useless,
    // has it any reason?
    ReadDirectory(true, true);
    FReadDirectoryPending = false;
  }
}

void TTerminal::EnsureNonExistence(const UnicodeString & AFileName, bool IsDirectory)
{
  // if filename doesn't contain path, we check for existence of file
  if ((core::UnixExtractFileDir(AFileName).IsEmpty()) &&
      core::UnixSamePath(GetCurrDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * File = FFiles->FindFile(AFileName);
    if (File)
    {
      if (File->GetIsDirectory() && IsDirectory)
      {
        throw ECommand(nullptr, FMTLOAD(RENAME_CREATE_DIR_EXISTS, AFileName.c_str()));
      }
      else if (!File->GetIsDirectory() && !IsDirectory)
      {
        throw ECommand(nullptr, FMTLOAD(RENAME_CREATE_FILE_EXISTS, AFileName.c_str()));
      }
    }
  }
}

void TTerminal::LogEvent(const UnicodeString & Str)
{
  if (GetLog()->GetLogging())
  {
    GetLog()->Add(llMessage, Str);
  }
}

void TTerminal::RollbackAction(TSessionAction & Action,
  TFileOperationProgressType * OperationProgress, Exception * E)
{
  // ESkipFile without "cancel" is file skip,
  // and we do not want to record skipped actions.
  // But ESkipFile with "cancel" is abort and we want to record that.
  // Note that TSCPFileSystem modifies the logic of RollbackAction little bit.
  if ((NB_STATIC_DOWNCAST(ESkipFile, E) != nullptr) &&
      ((OperationProgress == nullptr) ||
       (OperationProgress->Cancel == csContinue)))
  {
    Action.Cancel();
  }
  else
  {
    Action.Rollback(E);
  }
}

void TTerminal::DoStartup()
{
  LogEvent("Doing startup conversation with host.");
  BeginTransaction();
  try__finally
  {
    SCOPE_EXIT
    {
      DoEndTransaction(true);
    };
    DoInformation(LoadStr(STATUS_STARTUP), true);

    // Make sure that directory would be loaded at last
    FReadCurrentDirectoryPending = true;
    FReadDirectoryPending = GetAutoReadDirectory();

    FFileSystem->DoStartup();

    LookupUsersGroups();

    if (!GetSessionData()->GetRemoteDirectory().IsEmpty())
    {
      RemoteChangeDirectory(GetSessionData()->GetRemoteDirectory());
    }
  }
  __finally
  {
    DoEndTransaction(true);
  };
  LogEvent("Startup conversation with host finished.");
}

void TTerminal::ReadCurrentDirectory()
{
  DebugAssert(FFileSystem);
  try
  {
    // reset flag is case we are called externally (like from console dialog)
    FReadCurrentDirectoryPending = false;

    LogEvent("Getting current directory name.");
    UnicodeString OldDirectory = FFileSystem->GetCurrDirectory();

    FFileSystem->ReadCurrentDirectory();
    ReactOnCommand(fsCurrentDirectory);

    if (GetSessionData()->GetCacheDirectoryChanges())
    {
      DebugAssert(FDirectoryChangesCache != nullptr);
      UnicodeString CurrentDirectory = GetCurrDirectory();
      if (!CurrentDirectory.IsEmpty() && !FLastDirectoryChange.IsEmpty() && (CurrentDirectory != OldDirectory))
      {
        FDirectoryChangesCache->AddDirectoryChange(OldDirectory,
          FLastDirectoryChange, CurrentDirectory);
      }
      // not to broke the cache, if the next directory change would not
      // be initialized by ChangeDirectory(), which sets it
      // (HomeDirectory() particularly)
      FLastDirectoryChange.Clear();
    }

    if (OldDirectory.IsEmpty())
    {
      FLockDirectory = (GetSessionData()->GetLockInHome() ?
        FFileSystem->GetCurrDirectory() : UnicodeString(L""));
    }
    // if (OldDirectory != FFileSystem->GetCurrDirectory())
    {
      DoChangeDirectory();
    }
  }
  catch (Exception & E)
  {
    CommandError(&E, LoadStr(READ_CURRENT_DIR_ERROR));
  }
}

void TTerminal::ReadDirectory(bool ReloadOnly, bool ForceCache)
{
  bool LoadedFromCache = false;

  if (GetSessionData()->GetCacheDirectories() && FDirectoryCache->HasFileList(GetCurrDirectory()))
  {
    if (ReloadOnly && !ForceCache)
    {
      LogEvent("Cached directory not reloaded.");
    }
    else
    {
      DoStartReadDirectory();
      try__finally
      {
        SCOPE_EXIT
        {
          DoReadDirectory(ReloadOnly);
        };
        LoadedFromCache = FDirectoryCache->GetFileList(GetCurrDirectory(), FFiles);
      }
      __finally
      {
        DoReadDirectory(ReloadOnly);
      };

      if (LoadedFromCache)
      {
        LogEvent("Directory content loaded from cache.");
      }
      else
      {
        LogEvent("Cached Directory content has been removed.");
      }
    }
  }

  if (!LoadedFromCache)
  {
    DoStartReadDirectory();
    FReadingCurrentDirectory = true;
    bool Cancel = false; // dummy
    DoReadDirectoryProgress(0, 0, Cancel);

    try
    {
      TRemoteDirectory * Files = new TRemoteDirectory(this, FFiles);
      try__finally
      {
        SCOPE_EXIT
        {
          DoReadDirectoryProgress(-1, 0, Cancel);
          FReadingCurrentDirectory = false;
          FOldFiles->Reset();
          FOldFiles->AddFiles(FFiles);
          FFiles = Files;
          DoReadDirectory(ReloadOnly);
          // delete only after loading new files to dir view,
          // not to destroy the file objects that the view holds
          // (can be issue in multi threaded environment, such as when the
          // terminal is reconnecting in the terminal thread)
          if (GetActive())
          {
            if (GetSessionData()->GetCacheDirectories())
            {
              DirectoryLoaded(FFiles);
            }
          }
        };
        Files->SetDirectory(GetCurrDirectory());
        CustomReadDirectory(Files);
      }
      __finally
      {
        DoReadDirectoryProgress(-1, 0, Cancel);
        FReadingCurrentDirectory = false;
        FOldFiles->Reset();
        FOldFiles->AddFiles(FFiles);
        FFiles = Files;
        try__finally
        {
          DoReadDirectory(ReloadOnly);
        }
        __finally
        {
          // delete only after loading new files to dir view,
          // not to destroy the file objects that the view holds
          // (can be issue in multi threaded environment, such as when the
          // terminal is reconnecting in the terminal thread)
//          delete OldFiles;
        };
        if (GetActive())
        {
          if (GetSessionData()->GetCacheDirectories())
          {
            DirectoryLoaded(FFiles);
          }
        }
      };
    }
    catch (Exception & E)
    {
      CommandError(&E, FMTLOAD(LIST_DIR_ERROR, FFiles->GetDirectory().c_str()));
    }
  }
}

UnicodeString TTerminal::GetRemoteFileInfo(TRemoteFile * File)
{
  return
    FORMAT(L"%s;%c;%lld;%s;%d;%s;%s;%s;%d",
      File->GetFileName().c_str(), File->GetType(), File->GetSize(), StandardTimestamp(File->GetModification()).c_str(), int(File->GetModificationFmt()),
       File->GetFileOwner().GetLogText().c_str(), File->GetFileGroup().GetLogText().c_str(), File->GetRights()->GetText().c_str(),
       File->GetAttr());
}

void TTerminal::LogRemoteFile(TRemoteFile * AFile)
{
  // optimization
  if (GetLog()->GetLogging() && AFile)
  {
    LogEvent(GetRemoteFileInfo(AFile));
  }
}

UnicodeString TTerminal::FormatFileDetailsForLog(const UnicodeString & AFileName, const TDateTime & Modification, int64_t Size)
{
  UnicodeString Result;
  // optimization
  if (GetLog()->GetLogging())
  {
    Result = FORMAT(L"'%s' [%s] [%s]", AFileName.c_str(), UnicodeString(Modification != TDateTime() ? StandardTimestamp(Modification) : UnicodeString(L"n/a")).c_str(), ::Int64ToStr(Size).c_str());
  }
  return Result;
}

void TTerminal::LogFileDetails(const UnicodeString & AFileName, const TDateTime & AModification, int64_t Size)
{
  // optimization
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT(L"File: %s", FormatFileDetailsForLog(AFileName, AModification, Size).c_str()));
  }
}

void TTerminal::LogFileDone(TFileOperationProgressType * OperationProgress)
{
  // optimization
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT(L"Transfer done: '%s' [%s]", OperationProgress->FullFileName.c_str(), Int64ToStr(OperationProgress->TransferedSize).c_str()));
  }
}

void TTerminal::CustomReadDirectory(TRemoteFileList * AFileList)
{
  DebugAssert(AFileList);
  DebugAssert(FFileSystem);

  TRobustOperationLoop RobustLoop(this, FOperationProgress);

  do
  {
    try
    {
      FFileSystem->ReadDirectory(AFileList);
    }
    catch (Exception & E)
    {
      // Do not retry for initial listing of directory,
      // we instead retry whole connection attempt,
      // what would be done anyway (but Open is not ready for recursion).
      if ((FOpening > 0) ||
           !RobustLoop.TryReopen(E))
      {
        throw;
      }
    }
  }
  while (RobustLoop.Retry());

  if (GetLog()->GetLogging())
  {
    for (intptr_t Index = 0; Index < AFileList->GetCount(); ++Index)
    {
      LogRemoteFile(AFileList->GetFile(Index));
    }
  }

  ReactOnCommand(fsListDirectory);
}

TRemoteFileList * TTerminal::ReadDirectoryListing(const UnicodeString & Directory, const TFileMasks & Mask)
{
  TRemoteFileList * FileList = nullptr;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    FileList = nullptr;
    TLsSessionAction Action(GetActionLog(), GetAbsolutePath(Directory, true));

    try
    {
      FileList = DoReadDirectoryListing(Directory, false);
      if (FileList != nullptr)
      {
        intptr_t Index = 0;
        while (Index < FileList->GetCount())
        {
          TRemoteFile * File = FileList->GetFile(Index);
          TFileMasks::TParams Params;
          Params.Size = File->GetSize();
          Params.Modification = File->GetModification();
          // Have to use UnicodeString(), instead of L"", as with that
          // overload with (UnicodeString, bool, bool, TParams*) wins
          if (!Mask.Matches(File->GetFileName(), false,  UnicodeString(), &Params))
          {
            FileList->Delete(Index);
          }
          else
          {
            ++Index;
          }
        }

        Action.FileList(FileList);
      }
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action);
    }
  }
  while (RetryLoop.Retry());
  return FileList;
}

TRemoteFile * TTerminal::ReadFileListing(const UnicodeString & APath)
{
  TRemoteFile * File = nullptr;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    File = nullptr;
    TStatSessionAction Action(GetActionLog(), GetAbsolutePath(APath, true));
    try
    {
      // reset caches
      AnnounceFileListOperation();
      ReadFile(APath, File);
      Action.File(File);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action);
    }
  }
  while (RetryLoop.Retry());
  return File;
}

TRemoteFileList * TTerminal::CustomReadDirectoryListing(const UnicodeString & Directory, bool UseCache)
{
  TRemoteFileList * FileList = nullptr;
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FileList = DoReadDirectoryListing(Directory, UseCache);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E);
    }
  }
  while (RetryLoop.Retry());
  return FileList;
}

TRemoteFileList * TTerminal::DoReadDirectoryListing(const UnicodeString & ADirectory, bool UseCache)
{
  std::unique_ptr<TRemoteFileList> FileList(new TRemoteFileList());
  try__catch
  {
    bool Cache = UseCache && GetSessionData()->GetCacheDirectories();
    bool LoadedFromCache = Cache && FDirectoryCache->HasFileList(ADirectory);
    if (LoadedFromCache)
    {
      LoadedFromCache = FDirectoryCache->GetFileList(ADirectory, FileList.get());
    }

    if (!LoadedFromCache)
    {
      FileList->SetDirectory(ADirectory);

      SetExceptionOnFail(true);
      try__finally
      {
        SCOPE_EXIT
        {
          SetExceptionOnFail(false);
        };
        ReadDirectory(FileList.get());
      }
      __finally
      {
        SetExceptionOnFail(false);
      };

      if (Cache)
      {
        AddCachedFileList(FileList.get());
      }
    }
  }
  /*catch(...)
  {
    delete FileList;
    throw;
  }*/
  return FileList.release();
}

void TTerminal::ProcessDirectory(const UnicodeString & ADirName,
  TProcessFileEvent CallBackFunc, void * Param, bool UseCache, bool IgnoreErrors)
{
  std::unique_ptr<TRemoteFileList> FileList;
  if (IgnoreErrors)
  {
    SetExceptionOnFail(true);
    try__finally
    {
      SCOPE_EXIT
      {
        SetExceptionOnFail(false);
      };
      try
      {
        FileList.reset(CustomReadDirectoryListing(ADirName, UseCache));
      }
      catch (...)
      {
        if (!GetActive())
        {
          throw;
        }
      }
    }
    __finally
    {
      SetExceptionOnFail(false);
    };
  }
  else
  {
    FileList.reset(CustomReadDirectoryListing(ADirName, UseCache));
  }

  // skip if directory listing fails and user selects "skip"
  if (FileList.get())
  {
    try__finally
    {
      UnicodeString Directory = core::UnixIncludeTrailingBackslash(ADirName);

      for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
      {
        TRemoteFile * File = FileList->GetFile(Index);
        if (!File->GetIsParentDirectory() && !File->GetIsThisDirectory())
        {
          CallBackFunc(Directory + File->GetFileName(), File, Param);
          // We should catch EScpSkipFile here as we do in ProcessFiles.
          // Now we have to handle EScpSkipFile in every callback implementation.
        }
      }
    }
    __finally
    {
//      delete FileList;
    };
  }
}

void TTerminal::ReadDirectory(TRemoteFileList * AFileList)
{
  try
  {
    CustomReadDirectory(AFileList);
  }
  catch (Exception & E)
  {
    CommandError(&E, FMTLOAD(LIST_DIR_ERROR, AFileList->GetDirectory().c_str()));
  }
}

void TTerminal::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  DebugAssert(FFileSystem);
  try
  {
    LogEvent(FORMAT(L"Reading symlink \"%s\".", SymlinkFile->GetFileName().c_str()));
    FFileSystem->ReadSymlink(SymlinkFile, File);
    ReactOnCommand(fsReadSymlink);
  }
  catch (Exception & E)
  {
    CommandError(&E, FMTLOAD(READ_SYMLINK_ERROR, SymlinkFile->GetFileName().c_str()));
  }
}

void TTerminal::ReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile)
{
  DebugAssert(FFileSystem);
  AFile = nullptr;
  try
  {
    LogEvent(FORMAT(L"Listing file \"%s\".", AFileName.c_str()));
    FFileSystem->ReadFile(AFileName, AFile);
    ReactOnCommand(fsListFile);
    LogRemoteFile(AFile);
  }
  catch (Exception & E)
  {
    if (AFile)
    {
      SAFE_DESTROY(AFile);
    }
    AFile = nullptr;
    CommandError(&E, FMTLOAD(CANT_GET_ATTRS, AFileName.c_str()));
  }
}

bool TTerminal::FileExists(const UnicodeString & AFileName, TRemoteFile ** AFile)
{
  bool Result;
  TRemoteFile * File = nullptr;
  try
  {
    SetExceptionOnFail(true);
    try__finally
    {
      SCOPE_EXIT
      {
        SetExceptionOnFail(false);
      };
      ReadFile(core::UnixExcludeTrailingBackslash(AFileName), File);
    }
    __finally
    {
      SetExceptionOnFail(false);
    };

    if (AFile != nullptr)
    {
      *AFile = File;
    }
    else
    {
      SAFE_DESTROY(File);
    }
    Result = true;
  }
  catch (...)
  {
    if (GetActive())
    {
      Result = false;
    }
    else
    {
      throw;
    }
  }
  return Result;
}

void TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}

bool TTerminal::ProcessFiles(const TStrings * AFileList,
  TFileOperation Operation, TProcessFileEvent ProcessFile, void * Param,
  TOperationSide Side, bool Ex)
{
  DebugAssert(FFileSystem);
  DebugAssert(AFileList);

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  try
  {
    TFileOperationProgressType Progress(MAKE_CALLBACK(TTerminal::DoProgress, this), MAKE_CALLBACK(TTerminal::DoFinished, this));
    Progress.Start(Operation, Side, AFileList->GetCount());

    TFileOperationProgressType * OperationProgress(&Progress);

    FOperationProgress = &Progress; //-V506
    try__finally
    {
      SCOPE_EXIT
      {
        FOperationProgress = nullptr;
        Progress.Stop();
      };
      if (Side == osRemote)
      {
        BeginTransaction();
      }

      try__finally
      {
        SCOPE_EXIT
        {
          if (Side == osRemote)
          {
            EndTransaction();
          }
        };
        intptr_t Index = 0;
        UnicodeString FileName;
        while ((Index < AFileList->GetCount()) && (Progress.Cancel == csContinue))
        {
          FileName = AFileList->GetString(Index);
          try
          {
            bool Success = false;
            try__finally
            {
              SCOPE_EXIT
              {
                Progress.Finish(FileName, Success, OnceDoneOperation);
              };
              if (!Ex)
              {
                TRemoteFile * RemoteFile = NB_STATIC_DOWNCAST(TRemoteFile, AFileList->GetObj(Index));
                ProcessFile(FileName, RemoteFile, Param);
              }
              else
              {
                // not used anymore
#if 0
                TProcessFileEventEx ProcessFileEx = (TProcessFileEventEx)ProcessFile;
                ProcessFileEx(FileName, (TRemoteFile *)FileList->GetObjject(Index), Param, Index);
#endif
              }
              Success = true;
            }
            __finally
            {
              Progress.Finish(FileName, Success, OnceDoneOperation);
            };
          }
          catch (ESkipFile & E)
          {
            DEBUG_PRINTF("before HandleException");
            TSuspendFileOperationProgress Suspend(OperationProgress);
            if (!HandleException(&E))
            {
              throw;
            }
          }
          ++Index;
        }
      }
      __finally
      {
        if (Side == osRemote)
        {
          EndTransaction();
        }
      };

      if (Progress.Cancel == csContinue)
      {
        Result = true;
      }
    }
    __finally
    {
      FOperationProgress = nullptr;
      Progress.Stop();
    };
  }
  catch (...)
  {
    OnceDoneOperation = odoIdle;
    // this was missing here. was it by purpose?
    // without it any error message is lost
    throw;
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}

// not used anymore
#if 0
bool TTerminal::ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
  TProcessFileEventEx ProcessFile, void * Param, TOperationSide Side)
{
  return ProcessFiles(FileList, Operation, TProcessFileEvent(ProcessFile),
    Param, Side, true);
}
#endif

TStrings * TTerminal::GetFixedPaths()
{
  DebugAssert(FFileSystem != nullptr);
  return FFileSystem->GetFixedPaths();
}

bool TTerminal::GetResolvingSymlinks() const
{
  return GetSessionData()->GetResolveSymlinks() && GetIsCapable(fcResolveSymlink);
}

TUsableCopyParamAttrs TTerminal::UsableCopyParamAttrs(intptr_t Params)
{
  TUsableCopyParamAttrs Result;
  Result.General =
    FLAGMASK(!GetIsCapable(fcTextMode), cpaNoTransferMode) |
    FLAGMASK(!GetIsCapable(fcModeChanging), cpaNoRights) |
    FLAGMASK(!GetIsCapable(fcModeChanging), cpaNoPreserveReadOnly) |
    FLAGMASK(FLAGSET(Params, cpDelete), cpaNoClearArchive) |
    FLAGMASK(!GetIsCapable(fcIgnorePermErrors), cpaNoIgnorePermErrors) |
    // the following three are never supported for download,
    // so when they are not suppored for upload too,
    // set them in General flags, so that they do not get enabled on
    // Synchronize dialog.
    FLAGMASK(!GetIsCapable(fcModeChangingUpload), cpaNoRights) |
    FLAGMASK(!GetIsCapable(fcRemoveCtrlZUpload), cpaNoRemoveCtrlZ) |
    FLAGMASK(!GetIsCapable(fcRemoveBOMUpload), cpaNoRemoveBOM) |
    FLAGMASK(!GetIsCapable(fcPreservingTimestampDirs), cpaNoPreserveTimeDirs) |
    FLAGMASK(!GetIsCapable(fcResumeSupport), cpaNoResumeSupport);
  Result.Download = Result.General | cpaNoClearArchive |
    cpaNoIgnorePermErrors |
    // May be already set in General flags, but it's unconditional here
    cpaNoRights | cpaNoRemoveCtrlZ | cpaNoRemoveBOM;
  Result.Upload = Result.General | cpaNoPreserveReadOnly |
    FLAGMASK(!GetIsCapable(fcPreservingTimestampUpload), cpaNoPreserveTime);
  return Result;
}

bool TTerminal::IsRecycledFile(const UnicodeString & AFileName)
{
  bool Result = !GetSessionData()->GetRecycleBinPath().IsEmpty();
  if (Result)
  {
    UnicodeString Path = core::UnixExtractFilePath(AFileName);
    if (Path.IsEmpty())
    {
      Path = GetCurrDirectory();
    }
    Result = core::UnixSamePath(Path, GetSessionData()->GetRecycleBinPath());
  }
  return Result;
}

void TTerminal::RecycleFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile)
{
  UnicodeString FileName = AFileName;
  if (FileName.IsEmpty())
  {
    DebugAssert(AFile != nullptr);
    if (AFile)
      FileName = AFile->GetFileName();
  }

  if (!IsRecycledFile(FileName))
  {
    LogEvent(FORMAT(L"Moving file \"%s\" to remote recycle bin '%s'.",
      FileName.c_str(), GetSessionData()->GetRecycleBinPath().c_str()));

    TMoveFileParams Params;
    Params.Target = GetSessionData()->GetRecycleBinPath();
#if defined(__BORLANDC__)
    Params.FileMask = FORMAT("*-%s.*", (FormatDateTime(L"yyyymmdd-hhnnss", Now())));
#else
    uint16_t Y, M, D, H, N, S, MS;
    TDateTime DateTime = Now();
    DateTime.DecodeDate(Y, M, D);
    DateTime.DecodeTime(H, N, S, MS);
    UnicodeString dt = FORMAT(L"%04d%02d%02d-%02d%02d%02d", Y, M, D, H, N, S);
    // Params.FileMask = FORMAT(L"*-%s.*", FormatDateTime(L"yyyymmdd-hhnnss", Now()).c_str());
    Params.FileMask = FORMAT(L"*-%s.*", dt.c_str());
#endif
    TerminalMoveFile(FileName, AFile, &Params);
  }
}

bool TTerminal::TryStartOperationWithFile(
  const UnicodeString & AFileName, TFileOperation Operation1, TFileOperation Operation2)
{
  bool Result = true;
  if ((GetOperationProgress() != nullptr) &&
      ((GetOperationProgress()->Operation == Operation1) ||
       ((Operation2 != foNone) && (GetOperationProgress()->Operation == Operation2))))
  {
    if (GetOperationProgress()->Cancel != csContinue)
    {
      Result = false;
    }
    else
    {
      GetOperationProgress()->SetFile(AFileName);
    }
  }
  return Result;
}

void TTerminal::StartOperationWithFile(
  const UnicodeString & AFileName, TFileOperation Operation1, TFileOperation Operation2)
{
  if (!TryStartOperationWithFile(AFileName, Operation1, Operation2))
  {
    Abort();
  }
}

void TTerminal::RemoteDeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * AParams)
{
  UnicodeString FileName = AFileName;
  if (AFileName.IsEmpty() && AFile)
  {
    FileName = AFile->GetFileName();
  }
  StartOperationWithFile(FileName, foDelete);
  intptr_t Params = (AParams != nullptr) ? *(static_cast<intptr_t *>(AParams)) : 0;
  bool Recycle =
    FLAGCLEAR(Params, dfForceDelete) &&
    (GetSessionData()->GetDeleteToRecycleBin() != FLAGSET(Params, dfAlternative)) &&
    !GetSessionData()->GetRecycleBinPath().IsEmpty();
  if (Recycle && !IsRecycledFile(FileName))
  {
    RecycleFile(FileName, AFile);
  }
  else
  {
    LogEvent(FORMAT(L"Deleting file \"%s\".", FileName.c_str()));
    FileModified(AFile, FileName, true);
    DoDeleteFile(FileName, AFile, Params);
    ReactOnCommand(fsDeleteFile);
  }
}

void TTerminal::DoDeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, intptr_t Params)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TRmSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true));
    try
    {
      DebugAssert(FFileSystem);
      // 'File' parameter: SFTPFileSystem needs to know if file is file or directory
      FFileSystem->RemoteDeleteFile(AFileName, AFile, Params, Action);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(DELETE_FILE_ERROR, AFileName.c_str()));
    }
  }
  while (RetryLoop.Retry());
}

bool TTerminal::RemoteDeleteFiles(TStrings * AFilesToDelete, intptr_t Params)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  TODO("avoid resolving symlinks while reading subdirectories.");
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return ProcessFiles(AFilesToDelete, foDelete, MAKE_CALLBACK(TTerminal::RemoteDeleteFile, this), &Params);
}

void TTerminal::DeleteLocalFile(const UnicodeString & AFileName,
  const TRemoteFile * /*AFile*/, void * Params)
{
  StartOperationWithFile(AFileName, foDelete);
  if (GetOnDeleteLocalFile() == nullptr)
  {
    RecursiveDeleteFileChecked(AFileName, false);
  }
  else
  {
    GetOnDeleteLocalFile()(AFileName, FLAGSET(*(static_cast<intptr_t *>(Params)), dfAlternative));
  }
}

bool TTerminal::DeleteLocalFiles(TStrings * AFileList, intptr_t Params)
{
  return ProcessFiles(AFileList, foDelete, MAKE_CALLBACK(TTerminal::DeleteLocalFile, this), &Params, osLocal);
}

void TTerminal::CustomCommandOnFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * AParams)
{
  TCustomCommandParams * Params = NB_STATIC_DOWNCAST(TCustomCommandParams, AParams);
  UnicodeString LocalFileName = AFileName;
  if (AFileName.IsEmpty() && AFile)
  {
    LocalFileName = AFile->GetFileName();
  }
  StartOperationWithFile(LocalFileName, foCustomCommand);
  LogEvent(FORMAT(L"Executing custom command \"%s\" (%d) on file \"%s\".",
    Params->Command.c_str(), Params->Params, LocalFileName.c_str()));
  FileModified(AFile, LocalFileName);
  DoCustomCommandOnFile(LocalFileName, AFile, Params->Command, Params->Params,
    Params->OutputEvent);
  ReactOnCommand(fsAnyCommand);
}

void TTerminal::DoCustomCommandOnFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params,
  TCaptureOutputEvent OutputEvent)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      if (GetIsCapable(fcAnyCommand))
      {
        DebugAssert(FFileSystem);
        DebugAssert(fcShellAnyCommand);
        FFileSystem->CustomCommandOnFile(AFileName, AFile, Command, Params, OutputEvent);
      }
      else
      {
        DebugAssert(GetCommandSessionOpened());
        DebugAssert(FCommandSession->GetFSProtocol() == cfsSCP);
        LogEvent("Executing custom command on command session.");

        if (FCommandSession->GetCurrDirectory() != GetCurrDirectory())
        {
          FCommandSession->TerminalSetCurrentDirectory(GetCurrDirectory());
          // We are likely in transaction, so ReadCurrentDirectory won't get called
          // until transaction ends. But we need to know CurrentDirectory to
          // expand !/ pattern.
          // Doing this only, when current directory of the main and secondary shell differs,
          // what would be the case before the first file in transaction.
          // Otherwise we would be reading pwd before every time as the
          // CustomCommandOnFile on its own sets FReadCurrentDirectoryPending
          if (FCommandSession->FReadCurrentDirectoryPending)
          {
            FCommandSession->ReadCurrentDirectory();
          }
        }
        FCommandSession->FFileSystem->CustomCommandOnFile(AFileName, AFile, Command,
          Params, OutputEvent);
      }
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(CUSTOM_COMMAND_ERROR, Command.c_str(), AFileName.c_str()));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::CustomCommandOnFiles(const UnicodeString & Command,
  intptr_t Params, TStrings * AFiles, TCaptureOutputEvent OutputEvent)
{
  if (!TRemoteCustomCommand().IsFileListCommand(Command))
  {
    TCustomCommandParams AParams;
    AParams.Command = Command;
    AParams.Params = Params;
    AParams.OutputEvent = OutputEvent;
    ProcessFiles(AFiles, foCustomCommand, MAKE_CALLBACK(TTerminal::CustomCommandOnFile, this), &AParams);
  }
  else
  {
    UnicodeString FileList;
    for (intptr_t Index = 0; Index < AFiles->GetCount(); ++Index)
    {
      TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, AFiles->GetObj(Index));
      bool Dir = File->GetIsDirectory() && CanRecurseToDirectory(File);

      if (!Dir || FLAGSET(Params, ccApplyToDirectories))
      {
        if (!FileList.IsEmpty())
        {
          FileList += L" ";
        }

        FileList += L"\"" + ShellDelimitStr(AFiles->GetString(Index), L'"') + L"\"";
      }
    }

    TCustomCommandData Data(this);
    UnicodeString Cmd =
      TRemoteCustomCommand(Data, GetCurrDirectory(), L"", FileList).
        Complete(Command, true);
    DoAnyCommand(Cmd, OutputEvent, nullptr);
  }
}

void TTerminal::ChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*const TRemoteProperties*/ void * Properties)
{
  TRemoteProperties * RProperties = NB_STATIC_DOWNCAST(TRemoteProperties, Properties);
  DebugAssert(RProperties && !RProperties->Valid.Empty());
  UnicodeString LocalFileName = AFileName;
  if (AFileName.IsEmpty() && AFile)
  {
    LocalFileName = AFile->GetFileName();
  }
  StartOperationWithFile(LocalFileName, foSetProperties);
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT(L"Changing properties of \"%s\" (%s)",
      LocalFileName.c_str(), BooleanToEngStr(RProperties->Recursive).c_str()));
    if (RProperties->Valid.Contains(vpRights))
    {
      LogEvent(FORMAT(L" - mode: \"%s\"", RProperties->Rights.GetModeStr().c_str()));
    }
    if (RProperties->Valid.Contains(vpGroup))
    {
      LogEvent(FORMAT(L" - group: %s", RProperties->Group.GetLogText().c_str()));
    }
    if (RProperties->Valid.Contains(vpOwner))
    {
      LogEvent(FORMAT(L" - owner: %s", RProperties->Owner.GetLogText().c_str()));
    }
    if (RProperties->Valid.Contains(vpModification))
    {
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = ::UnixToDateTime(RProperties->Modification, GetSessionData()->GetDSTMode());
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
      LogEvent(FORMAT(L" - modification: \"%s\"",
//       FormatDateTime(L"dddddd tt",
//       ::UnixToDateTime(RProperties->Modification, GetSessionData()->GetDSTMode())).c_str()));
         dt.c_str()));
    }
    if (RProperties->Valid.Contains(vpLastAccess))
    {
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = ::UnixToDateTime(RProperties->LastAccess, GetSessionData()->GetDSTMode());
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
      LogEvent(FORMAT(L" - last access: \"%s\"",
//       FormatDateTime(L"dddddd tt",
//       ::UnixToDateTime(RProperties->LastAccess, GetSessionData()->GetDSTMode())).c_str()));
         dt.c_str()));
    }
  }
  FileModified(AFile, LocalFileName);
  DoChangeFileProperties(LocalFileName, AFile, RProperties);
  ReactOnCommand(fsChangeProperties);
}

void TTerminal::DoChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const TRemoteProperties * Properties)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TChmodSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true));
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->ChangeFileProperties(AFileName, AFile, Properties, Action);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(CHANGE_PROPERTIES_ERROR, AFileName.c_str()));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::ChangeFilesProperties(TStrings * AFileList,
  const TRemoteProperties * Properties)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  AnnounceFileListOperation();
  ProcessFiles(AFileList, foSetProperties, MAKE_CALLBACK(TTerminal::ChangeFileProperties, this), const_cast<void *>(static_cast<const void *>(Properties)));
}

bool TTerminal::LoadFilesProperties(TStrings * AFileList)
{
  // see comment in TSFTPFileSystem::IsCapable
  bool Result =
    GetIsCapable(fcLoadingAdditionalProperties) &&
    FFileSystem->LoadFilesProperties(AFileList);
  if (Result && GetSessionData()->GetCacheDirectories() &&
      (AFileList->GetCount() > 0) &&
      (NB_STATIC_DOWNCAST(TRemoteFile, AFileList->GetObj(0))->GetDirectory() == FFiles))
  {
    AddCachedFileList(FFiles);
  }
  return Result;
}

void TTerminal::CalculateFileSize(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*TCalculateSizeParams*/ void * Param)
{
  DebugAssert(Param);
  DebugAssert(AFile);
  TCalculateSizeParams * AParams = NB_STATIC_DOWNCAST(TCalculateSizeParams, Param);
  UnicodeString LocalFileName = AFileName;
  if (AFileName.IsEmpty())
  {
    LocalFileName = AFile->GetFileName();
  }

  if (!TryStartOperationWithFile(LocalFileName, foCalculateSize))
  {
    AParams->Result = false;
    Abort();
  }

  bool AllowTransfer = (AParams->CopyParam == nullptr);
  if (!AllowTransfer)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = AFile->GetSize();
    MaskParams.Modification = AFile->GetModification();

    UnicodeString BaseFileName =
      GetBaseFileName(core::UnixExcludeTrailingBackslash(AFile->GetFullFileName()));
    AllowTransfer = AParams->CopyParam->AllowTransfer(
      BaseFileName, osRemote, AFile->GetIsDirectory(), MaskParams);
  }

  if (AllowTransfer)
  {
    if (AFile->GetIsDirectory())
    {
      if (CanRecurseToDirectory(AFile))
      {
        if (!AParams->AllowDirs)
        {
          AParams->Result = false;
        }
        else
        {
          LogEvent(FORMAT(L"Getting size of directory \"%s\"", LocalFileName.c_str()));
          // pass in full path so we get it back in file list for AllowTransfer() exclusion
          DoCalculateDirectorySize(AFile->GetFullFileName(), AFile, AParams);
        }
      }

      if (AParams->Stats != nullptr)
      {
        AParams->Stats->Directories++;
      }
    }
    else
    {
      AParams->Size += AFile->GetSize();

      if (AParams->Stats != nullptr)
      {
        AParams->Stats->Files++;
      }
    }

    if ((AParams->Stats != nullptr) && AFile->GetIsSymLink())
    {
      AParams->Stats->SymLinks++;
    }
  }
}

void TTerminal::DoCalculateDirectorySize(const UnicodeString & AFileName,
  const TRemoteFile * AFile, TCalculateSizeParams * Params)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      ProcessDirectory(AFileName, MAKE_CALLBACK(TTerminal::CalculateFileSize, this), Params);
    }
    catch (Exception & E)
    {
      if (!GetActive() || ((Params->Params & csIgnoreErrors) == 0))
      {
        RetryLoop.Error(E, FMTLOAD(CALCULATE_SIZE_ERROR, AFileName.c_str()));
      }
    }
  }
  while (RetryLoop.Retry());
}

bool TTerminal::CalculateFilesSize(const TStrings * AFileList,
  int64_t & Size, intptr_t Params, const TCopyParamType * CopyParam,
  bool AllowDirs, TCalculateSizeStats * Stats)
{
  // With FTP protocol, we may use DSIZ command from
  // draft-peterson-streamlined-ftp-command-extensions-10
  // Implemented by Serv-U FTP.

  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  TCalculateSizeParams Param;
  Param.Size = 0;
  Param.Params = Params;
  Param.CopyParam = CopyParam;
  Param.Stats = Stats;
  Param.AllowDirs = AllowDirs;
  Param.Result = true;
  ProcessFiles(AFileList, foCalculateSize, MAKE_CALLBACK(TTerminal::CalculateFileSize, this), &Param);
  Size = Param.Size;
  return Param.Result;
}

void TTerminal::CalculateFilesChecksum(const UnicodeString & Alg,
  TStrings * AFileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  FFileSystem->CalculateFilesChecksum(Alg, AFileList, Checksums, OnCalculatedChecksum);
}

void TTerminal::TerminalRenameFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName)
{
  LogEvent(FORMAT(L"Renaming file \"%s\" to \"%s\".", AFileName.c_str(), ANewName.c_str()));
  DoRenameFile(AFileName, ANewName, false);
  ReactOnCommand(fsRenameFile);
}

void TTerminal::TerminalRenameFile(const TRemoteFile * AFile,
  const UnicodeString & ANewName, bool CheckExistence)
{
  DebugAssert(AFile && AFile->GetDirectory() == FFiles);
  bool Proceed = true;
  // if filename doesn't contain path, we check for existence of file
  if ((AFile->GetFileName() != ANewName) && CheckExistence &&
      FConfiguration->GetConfirmOverwriting() &&
      core::UnixSamePath(GetCurrDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * DuplicateFile = FFiles->FindFile(ANewName);
    if (DuplicateFile)
    {
      UnicodeString QuestionFmt;
      if (DuplicateFile->GetIsDirectory())
      {
        QuestionFmt = LoadStr(DIRECTORY_OVERWRITE);
      }
      else
      {
        QuestionFmt = LoadStr(PROMPT_FILE_OVERWRITE);
      }
      TQueryParams Params(qpNeverAskAgainCheck);
      UnicodeString Question = MainInstructions(FORMAT(QuestionFmt.c_str(), ANewName.c_str()));
      intptr_t Result = QueryUser(Question, nullptr,
        qaYes | qaNo, &Params);
      if (Result == qaNeverAskAgain)
      {
        Proceed = true;
        FConfiguration->SetConfirmOverwriting(false);
      }
      else
      {
        Proceed = (Result == qaYes);
      }
    }
  }

  if (Proceed)
  {
    FileModified(AFile, AFile->GetFileName());
    TerminalRenameFile(AFile->GetFileName(), ANewName);
  }
}

void TTerminal::DoRenameFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName, bool Move)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TMvSessionAction Action(GetActionLog(), GetAbsolutePath(AFileName, true), GetAbsolutePath(ANewName, true));
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->RemoteRenameFile(AFileName, ANewName);
    }
    catch (Exception & E)
    {
      UnicodeString Message = FMTLOAD(Move ? MOVE_FILE_ERROR : RENAME_FILE_ERROR, AFileName.c_str(), ANewName.c_str());
      RetryLoop.Error(E, Action, Message);
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::TerminalMoveFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*const TMoveFileParams*/ void * Param)
{
  StartOperationWithFile(AFileName, foRemoteMove, foDelete);
  DebugAssert(Param != nullptr);
  const TMoveFileParams & Params = *NB_STATIC_DOWNCAST_CONST(TMoveFileParams, Param);
  UnicodeString NewName = core::UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(base::UnixExtractFileName(AFileName), Params.FileMask);
  LogEvent(FORMAT(L"Moving file \"%s\" to \"%s\".", AFileName.c_str(), NewName.c_str()));
  FileModified(AFile, AFileName);
  DoRenameFile(AFileName, NewName, true);
  ReactOnCommand(fsMoveFile);
}

bool TTerminal::MoveFiles(TStrings * AFileList, const UnicodeString & Target,
  const UnicodeString & FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  bool Result = false;
  BeginTransaction();
  try__finally
  {
    SCOPE_EXIT
    {
      if (GetActive())
      {
        UnicodeString WithTrailing = core::UnixIncludeTrailingBackslash(this->GetCurrDirectory());
        bool PossiblyMoved = false;
        // check if we was moving current directory.
        // this is just optimization to avoid checking existence of current
        // directory after each move operation.
        UnicodeString CurrentDirectory = this->GetCurrDirectory();
        for (intptr_t Index = 0; !PossiblyMoved && (Index < AFileList->GetCount()); ++Index)
        {
          const TRemoteFile * File =
            NB_STATIC_DOWNCAST_CONST(TRemoteFile, AFileList->GetObj(Index));
          // File can be nullptr, and filename may not be full path,
          // but currently this is the only way we can move (at least in GUI)
          // current directory
          const UnicodeString & Str = AFileList->GetString(Index);
          if ((File != nullptr) &&
              File->GetIsDirectory() &&
              ((CurrentDirectory.SubString(1, Str.Length()) == Str) &&
               ((Str.Length() == CurrentDirectory.Length()) ||
                (CurrentDirectory[Str.Length() + 1] == L'/'))))
          {
            PossiblyMoved = true;
          }
        }

        if (PossiblyMoved && !this->FileExists(CurrentDirectory))
        {
          UnicodeString NearestExisting = CurrentDirectory;
          do
          {
            NearestExisting = core::UnixExtractFileDir(NearestExisting);
          }
          while (!core::IsUnixRootPath(NearestExisting) && !this->FileExists(NearestExisting));

          RemoteChangeDirectory(NearestExisting);
        }
      }
      EndTransaction();
    };
    Result = ProcessFiles(AFileList, foRemoteMove, MAKE_CALLBACK(TTerminal::TerminalMoveFile, this), &Params);
  }
  __finally
  {
    if (GetActive())
    {
      UnicodeString WithTrailing = core::UnixIncludeTrailingBackslash(this->GetCurrDirectory());
      bool PossiblyMoved = false;
      // check if we was moving current directory.
      // this is just optimization to avoid checking existence of current
      // directory after each move operation.
      UnicodeString CurrentDirectory = this->GetCurrDirectory();
      for (intptr_t Index = 0; !PossiblyMoved && (Index < AFileList->GetCount()); ++Index)
      {
        const TRemoteFile * File =
          NB_STATIC_DOWNCAST_CONST(TRemoteFile, AFileList->GetObj(Index));
        // File can be nullptr, and filename may not be full path,
        // but currently this is the only way we can move (at least in GUI)
        // current directory
        const UnicodeString & Str = AFileList->GetString(Index);
        if ((File != nullptr) &&
            File->GetIsDirectory() &&
            ((CurrentDirectory.SubString(1, Str.Length()) == Str) &&
             ((Str.Length() == CurrentDirectory.Length()) ||
              (CurrentDirectory[Str.Length() + 1] == L'/'))))
        {
          PossiblyMoved = true;
        }
      }

      if (PossiblyMoved && !FileExists(CurrentDirectory))
      {
        UnicodeString NearestExisting = CurrentDirectory;
        do
        {
          NearestExisting = core::UnixExtractFileDir(NearestExisting);
        }
        while (!core::IsUnixRootPath(NearestExisting) && !FileExists(NearestExisting));

        RemoteChangeDirectory(NearestExisting);
      }
    }
    EndTransaction();
  };
  return Result;
}

void TTerminal::DoCopyFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      DebugAssert(FFileSystem);
      if (GetIsCapable(fcRemoteCopy))
      {
        FFileSystem->RemoteCopyFile(AFileName, ANewName);
      }
      else
      {
        DebugAssert(GetCommandSessionOpened());
        DebugAssert(FCommandSession->GetFSProtocol() == cfsSCP);
        LogEvent("Copying file on command session.");
        FCommandSession->TerminalSetCurrentDirectory(GetCurrDirectory());
        FCommandSession->FFileSystem->RemoteCopyFile(AFileName, ANewName);
      }
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(COPY_FILE_ERROR, AFileName.c_str(), ANewName.c_str()));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::TerminalCopyFile(const UnicodeString & AFileName,
  const TRemoteFile * /*File*/, /*const TMoveFileParams*/ void * Param)
{
  StartOperationWithFile(AFileName, foRemoteCopy);
  DebugAssert(Param != nullptr);
  const TMoveFileParams & Params = *NB_STATIC_DOWNCAST_CONST(TMoveFileParams, Param);
  UnicodeString NewName = core::UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(base::UnixExtractFileName(AFileName), Params.FileMask);
  LogEvent(FORMAT(L"Copying file \"%s\" to \"%s\".", AFileName.c_str(), NewName.c_str()));
  DoCopyFile(AFileName, NewName);
  ReactOnCommand(fsCopyFile);
}

bool TTerminal::CopyFiles(const TStrings * AFileList, const UnicodeString & Target,
  const UnicodeString & FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  return ProcessFiles(AFileList, foRemoteCopy, MAKE_CALLBACK(TTerminal::TerminalCopyFile, this), &Params);
}

void TTerminal::RemoteCreateDirectory(const UnicodeString & ADirName,
  const TRemoteProperties * Properties)
{
  DebugAssert(FFileSystem);
  EnsureNonExistence(ADirName, true);
  FileModified(nullptr, ADirName);

  LogEvent(FORMAT(L"Creating directory \"%s\".", ADirName.c_str()));
  DoCreateDirectory(ADirName);

  if ((Properties != nullptr) && !Properties->Valid.Empty())
  {
    DoChangeFileProperties(ADirName, nullptr, Properties);
  }

  ReactOnCommand(fsCreateDirectory);
}

void TTerminal::DoCreateDirectory(const UnicodeString & ADirName)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    TMkdirSessionAction Action(GetActionLog(), GetAbsolutePath(ADirName, true));
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->RemoteCreateDirectory(ADirName);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, Action, FMTLOAD(CREATE_DIR_ERROR, ADirName.c_str()));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::CreateLink(const UnicodeString & AFileName,
  const UnicodeString & PointTo, bool Symbolic, bool IsDirectory)
{
  DebugAssert(FFileSystem);
  EnsureNonExistence(AFileName, IsDirectory);
  if (GetSessionData()->GetCacheDirectories())
  {
    DirectoryModified(GetCurrDirectory(), false);
  }

  LogEvent(FORMAT(L"Creating link \"%s\" to \"%s\" (symbolic: %s).",
    AFileName.c_str(), PointTo.c_str(), BooleanToEngStr(Symbolic).c_str()));
  DoCreateLink(AFileName, PointTo, Symbolic);
  ReactOnCommand(fsCreateDirectory);
}

void TTerminal::DoCreateLink(const UnicodeString & AFileName,
  const UnicodeString & PointTo, bool Symbolic)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      DebugAssert(FFileSystem);
      FFileSystem->CreateLink(AFileName, PointTo, Symbolic);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(CREATE_LINK_ERROR, AFileName.c_str()));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::HomeDirectory()
{
  DebugAssert(FFileSystem);
  try
  {
    LogEvent("Changing directory to home directory.");
    FFileSystem->HomeDirectory();
    ReactOnCommand(fsHomeDirectory);
  }
  catch (Exception & E)
  {
    CommandError(&E, LoadStr(CHANGE_HOMEDIR_ERROR));
  }
}

void TTerminal::RemoteChangeDirectory(const UnicodeString & Directory)
{
  UnicodeString DirectoryNormalized = core::ToUnixPath(Directory);
  DebugAssert(FFileSystem);
  try
  {
    UnicodeString CachedDirectory;
    DebugAssert(!GetSessionData()->GetCacheDirectoryChanges() || (FDirectoryChangesCache != nullptr));
    // never use directory change cache during startup, this ensures, we never
    // end-up initially in non-existing directory
    if ((GetStatus() == ssOpened) &&
        GetSessionData()->GetCacheDirectoryChanges() &&
        FDirectoryChangesCache->GetDirectoryChange(PeekCurrentDirectory(),
          DirectoryNormalized, CachedDirectory))
    {
      LogEvent(FORMAT(L"Cached directory change via \"%s\" to \"%s\".",
        DirectoryNormalized.c_str(), CachedDirectory.c_str()));
      FFileSystem->CachedChangeDirectory(CachedDirectory);
    }
    else
    {
      LogEvent(FORMAT(L"Changing directory to \"%s\".", DirectoryNormalized.c_str()));
      FFileSystem->ChangeDirectory(DirectoryNormalized);
    }
    FLastDirectoryChange = DirectoryNormalized;
    ReactOnCommand(fsChangeDirectory);
  }
  catch (Exception & E)
  {
    CommandError(&E, FMTLOAD(CHANGE_DIR_ERROR, DirectoryNormalized.c_str()));
  }
}

void TTerminal::LookupUsersGroups()
{
  if (!FUsersGroupsLookedup && GetSessionData()->GetLookupUserGroups() &&
      GetIsCapable(fcUserGroupListing))
  {
    DebugAssert(FFileSystem);

    try
    {
      FUsersGroupsLookedup = true;
      LogEvent("Looking up groups and users.");
      FFileSystem->LookupUsersGroups();
      ReactOnCommand(fsLookupUsersGroups);

      if (GetLog()->GetLogging())
      {
        FGroups.Log(this, L"groups");
        FMembership.Log(this, L"membership");
        FUsers.Log(this, L"users");
      }
    }
    catch (Exception & E)
    {
      if (!GetActive() || (GetSessionData()->GetLookupUserGroups() == asOn))
      {
        CommandError(&E, LoadStr(LOOKUP_GROUPS_ERROR));
      }
    }
  }
}

bool TTerminal::AllowedAnyCommand(const UnicodeString & Command) const
{
  return !Command.Trim().IsEmpty();
}

bool TTerminal::GetCommandSessionOpened() const
{
  // consider secondary terminal open in "ready" state only
  // so we never do keepalives on it until it is completely initialized
  return (FCommandSession != nullptr) &&
    (FCommandSession->GetStatus() == ssOpened);
}

TTerminal * TTerminal::GetCommandSession()
{
  if ((FCommandSession != nullptr) && !FCommandSession->GetActive())
  {
    SAFE_DESTROY(FCommandSession);
  }

  if (FCommandSession == nullptr)
  {
    // transaction cannot be started yet to allow proper matching transaction
    // levels between main and command session
    DebugAssert(FInTransaction == 0);

    try__catch
    {
      std::unique_ptr<TSecondaryTerminal> CommandSession(new TSecondaryTerminal(this));
      CommandSession->Init(GetSessionData(), FConfiguration, L"Shell");

      CommandSession->SetAutoReadDirectory(false);

      TSessionData * CommandSessionData = CommandSession->FSessionData;
      CommandSessionData->SetRemoteDirectory(GetCurrDirectory());
      CommandSessionData->SetFSProtocol(fsSCPonly);
      CommandSessionData->SetClearAliases(false);
      CommandSessionData->SetUnsetNationalVars(false);
      CommandSessionData->SetLookupUserGroups(asOn);

      CommandSession->FExceptionOnFail = FExceptionOnFail;

      CommandSession->SetOnQueryUser(GetOnQueryUser());
      CommandSession->SetOnPromptUser(GetOnPromptUser());
      CommandSession->SetOnShowExtendedException(GetOnShowExtendedException());
      CommandSession->SetOnProgress(GetOnProgress());
      CommandSession->SetOnFinished(GetOnFinished());
      CommandSession->SetOnInformation(GetOnInformation());
      CommandSession->SetOnClose(MAKE_CALLBACK(TTerminal::CommandSessionClose, this));
      // do not copy OnDisplayBanner to avoid it being displayed
      FCommandSession = CommandSession.release();
    }
    /*catch(...)
    {
      SAFE_DESTROY(FCommandSession);
      throw;
    }*/
  }

  return FCommandSession;
}

void TTerminal::CommandSessionClose(TObject * /*Sender*/)
{
  // Keep the states in sync.
  // This is particularly to invoke ours OnClose,
  // So that it is triggered before Reopen is called
  Close();
}

#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated

class TOutputProxy : public TObject
{
public:
  TOutputProxy(TCallSessionAction & Action, TCaptureOutputEvent OutputEvent) :
    FAction(Action),
    FOutputEvent(OutputEvent)
  {
  }

  void Output(const UnicodeString & Str, TCaptureOutputType OutputType)
  {
    // FAction.AddOutput(Str, StdError);
    switch (OutputType)
    {
      case cotOutput:
        FAction.AddOutput(Str, false);
        break;
      case cotError:
        FAction.AddOutput(Str, true);
        break;
      case cotExitCode:
        FAction.ExitCode((int)::StrToInt64(Str));
        break;
    }

    if (FOutputEvent != nullptr)
    {
      FOutputEvent(Str, OutputType);
    }
  }

private:
  TCallSessionAction & FAction;
  TCaptureOutputEvent FOutputEvent;
};

#pragma warning(pop)

void TTerminal::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent OutputEvent)
{
  TCallSessionAction Action(GetActionLog(), Command, GetCurrDirectory());
  TOutputProxy ProxyOutputEvent(Action, OutputEvent);
  DoAnyCommand(Command, MAKE_CALLBACK(TOutputProxy::Output, &ProxyOutputEvent), &Action);
}

void TTerminal::DoAnyCommand(const UnicodeString & ACommand,
  TCaptureOutputEvent OutputEvent, TCallSessionAction * Action)
{
  DebugAssert(FFileSystem);
  try
  {
    DirectoryModified(GetCurrDirectory(), false);
    if (GetIsCapable(fcAnyCommand))
    {
      LogEvent("Executing user defined command.");
      FFileSystem->AnyCommand(ACommand, OutputEvent);
    }
    else
    {
      DebugAssert(GetCommandSessionOpened());
      DebugAssert(FCommandSession->GetFSProtocol() == cfsSCP);
      LogEvent("Executing user defined command on command session.");

      FCommandSession->TerminalSetCurrentDirectory(GetCurrDirectory());
      FCommandSession->FFileSystem->AnyCommand(ACommand, OutputEvent);

      FCommandSession->GetFileSystem()->ReadCurrentDirectory();

      // synchronize pwd (by purpose we lose transaction optimization here)
      RemoteChangeDirectory(FCommandSession->GetCurrDirectory());
    }
    ReactOnCommand(fsAnyCommand);
  }
  catch (Exception & E)
  {
    if (Action != nullptr)
    {
      RollbackAction(*Action, nullptr, &E);
    }
    if (GetExceptionOnFail() || (NB_STATIC_DOWNCAST(EFatal, &E) != nullptr))
    {
      throw;
    }
    else
    {
      HandleExtendedException(&E);
    }
  }
}

bool TTerminal::DoCreateLocalFile(const UnicodeString & AFileName,
  TFileOperationProgressType * OperationProgress,
  bool Resume,
  bool NoConfirmation,
  OUT HANDLE * AHandle)
{
  DebugAssert(OperationProgress);
  DebugAssert(AHandle);
  bool Result = true;
  bool Done;
  DWORD DesiredAccess = GENERIC_WRITE;
  DWORD ShareMode = FILE_SHARE_READ;
  DWORD CreationDisposition = Resume ? OPEN_ALWAYS : CREATE_ALWAYS;
  DWORD FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
  do
  {
    *AHandle = TerminalCreateLocalFile(ApiPath(AFileName).c_str(), DesiredAccess, ShareMode,
      CreationDisposition, FlagsAndAttributes);
    Done = (*AHandle != INVALID_HANDLE_VALUE);
    if (!Done)
    {
      // save the error, otherwise it gets overwritten by call to FileExists
      int LastError = ::GetLastError();
      DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
      if (::FileExists(ApiPath(AFileName)) &&
        (((LocalFileAttrs = GetLocalFileAttributes(ApiPath(AFileName))) & (faReadOnly | faHidden)) != 0))
      {
        if (FLAGSET(LocalFileAttrs, faReadOnly))
        {
          if (OperationProgress->BatchOverwrite == boNone)
          {
            Result = false;
          }
          else if ((OperationProgress->BatchOverwrite != boAll) && !NoConfirmation)
          {
            uintptr_t Answer;

            {
              TSuspendFileOperationProgress Suspend(OperationProgress);
              Answer = QueryUser(
                MainInstructions(FMTLOAD(READ_ONLY_OVERWRITE, AFileName.c_str())), nullptr,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll, 0);
            }

            switch (Answer)
            {
              case qaYesToAll:
                OperationProgress->BatchOverwrite = boAll;
                break;
              case qaCancel:
                OperationProgress->Cancel = csCancel; // continue on next case
                Result = false;
                break;
              case qaNoToAll:
                OperationProgress->BatchOverwrite = boNone;
                Result = false;
                break;
              case qaNo:
                Result = false;
                break;
            }
          }
        }
        else
        {
          DebugAssert(FLAGSET(LocalFileAttrs, faHidden));
          Result = true;
        }

        if (Result)
        {
          FlagsAndAttributes |=
            FLAGMASK(FLAGSET(LocalFileAttrs, faHidden), FILE_ATTRIBUTE_HIDDEN) |
            FLAGMASK(FLAGSET(LocalFileAttrs, faReadOnly), FILE_ATTRIBUTE_READONLY);

          FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, AFileName.c_str()), "",
          [&]()
          {
            if (!this->SetLocalFileAttributes(ApiPath(AFileName), LocalFileAttrs & ~(faReadOnly | faHidden)))
            {
              ::RaiseLastOSError();
            }
          });
        }
        else
        {
          Done = true;
        }
      }
      else
      {
        ::RaiseLastOSError(LastError);
      }
    }
  }
  while (!Done);

  return Result;
}

bool TTerminal::TerminalCreateLocalFile(const UnicodeString & ATargetFileName,
  TFileOperationProgressType * OperationProgress,
  bool Resume,
  bool NoConfirmation,
  OUT HANDLE * AHandle)
{
  DebugAssert(OperationProgress);
  DebugAssert(AHandle);
  bool Result = true;
  FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CREATE_FILE_ERROR, ATargetFileName.c_str()), "",
  [&]()
  {
    Result = DoCreateLocalFile(ATargetFileName, OperationProgress, Resume, NoConfirmation,
      AHandle);
  });

  return Result;
}

void TTerminal::TerminalOpenLocalFile(const UnicodeString & ATargetFileName,
  DWORD Access,
  OUT HANDLE * AHandle, OUT uintptr_t * AAttrs, OUT int64_t * ACTime,
  OUT int64_t * AMTime, OUT int64_t * AATime, OUT int64_t * ASize,
  bool TryWriteReadOnly)
{
  DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
  HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();

  FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(FILE_NOT_EXISTS, ATargetFileName.c_str()), "",
  [&]()
  {
    UnicodeString FileNameApi = ApiPath(ATargetFileName);
    LocalFileAttrs = this->GetLocalFileAttributes(FileNameApi);
    if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
    {
      ::RaiseLastOSError();
    }
  });

  if (FLAGCLEAR(LocalFileAttrs, faDirectory) || (AHandle == INVALID_HANDLE_VALUE))
  {
    bool NoHandle = false;
    if (!TryWriteReadOnly && (Access == GENERIC_WRITE) &&
        ((LocalFileAttrs & faReadOnly) != 0))
    {
      Access = GENERIC_READ;
      NoHandle = true;
    }

    FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(OPENFILE_ERROR, ATargetFileName.c_str()), "",
    [&]()
    {
      DWORD Flags = FLAGMASK(FLAGSET(LocalFileAttrs, faDirectory), FILE_FLAG_BACKUP_SEMANTICS);
      LocalFileHandle = this->TerminalCreateLocalFile(ApiPath(ATargetFileName).c_str(), Access,
        Access == GENERIC_READ ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ,
        OPEN_EXISTING, Flags);
      if (LocalFileHandle == INVALID_HANDLE_VALUE)
      {
        ::RaiseLastOSError();
      }
    });

    try
    {
      if (AATime || AMTime || ACTime)
      {
        FILETIME ATime;
        FILETIME MTime;
        FILETIME CTime;

        // Get last file access and modification time
        FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_GET_ATTRS, ATargetFileName.c_str()), "",
        [&]()
        {
          THROWOSIFFALSE(::GetFileTime(LocalFileHandle, &CTime, &ATime, &MTime));
        });
        if (ACTime)
        {
          *ACTime = ::ConvertTimestampToUnixSafe(CTime, GetSessionData()->GetDSTMode());
        }
        if (AATime)
        {
          *AATime = ::ConvertTimestampToUnixSafe(ATime, GetSessionData()->GetDSTMode());
        }
        if (AMTime)
        {
          *AMTime = ::ConvertTimestampToUnix(MTime, GetSessionData()->GetDSTMode());
        }
      }

      if (ASize)
      {
        // Get file size
        FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_GET_ATTRS, ATargetFileName.c_str()), "",
        [&]()
        {
          uint32_t LSize;
          DWORD HSize;
          LSize = ::GetFileSize(LocalFileHandle, &HSize);
          if ((LSize == static_cast<uint32_t>(-1)) && (::GetLastError() != NO_ERROR))
          {
            ::RaiseLastOSError();
          }
          *ASize = (static_cast<int64_t>(HSize) << 32) + LSize;
        });
      }

      if ((AHandle == nullptr) || NoHandle)
      {
        ::CloseHandle(LocalFileHandle);
        LocalFileHandle = INVALID_HANDLE_VALUE;
      }
    }
    catch (...)
    {
      ::CloseHandle(LocalFileHandle);
      throw;
    }
  }

  if (AAttrs)
  {
    *AAttrs = LocalFileAttrs;
  }
  if (AHandle)
  {
    *AHandle = LocalFileHandle;
  }
}

bool TTerminal::AllowLocalFileTransfer(const UnicodeString & AFileName,
  const TCopyParamType * CopyParam, TFileOperationProgressType * OperationProgress)
{
  bool Result = true;
  // optimization
  if (GetLog()->GetLogging() || !CopyParam->AllowAnyTransfer())
  {
    WIN32_FIND_DATA FindData = {};
    HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
    FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(FILE_NOT_EXISTS, AFileName.c_str()), "",
    [&]()
    {
      LocalFileHandle = ::FindFirstFile(ApiPath(::ExcludeTrailingBackslash(AFileName)).c_str(), &FindData);
      if (LocalFileHandle == INVALID_HANDLE_VALUE)
      {
        ::RaiseLastOSError();
      }
    });
    ::FindClose(LocalFileHandle);
    bool Directory = FLAGSET(FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    TFileMasks::TParams Params;
    // SearchRec.Size in C++B2010 is int64_t,
    // so we should be able to use it instead of FindData.nFileSize*
    Params.Size =
      (static_cast<int64_t>(FindData.nFileSizeHigh) << 32) +
      FindData.nFileSizeLow;
    Params.Modification = ::FileTimeToDateTime(FindData.ftLastWriteTime);
    UnicodeString BaseFileName = GetBaseFileName(AFileName);
    if (!CopyParam->AllowTransfer(BaseFileName, osLocal, Directory, Params))
    {
      LogEvent(FORMAT(L"File \"%s\" excluded from transfer", AFileName.c_str()));
      Result = false;
    }
    else if (CopyParam->SkipTransfer(AFileName, Directory))
    {
      OperationProgress->AddSkippedFileSize(Params.Size);
      Result = false;
    }

    if (Result)
    {
      LogFileDetails(AFileName, Params.Modification, Params.Size);
    }
  }
  return Result;
}

void TTerminal::MakeLocalFileList(const UnicodeString & AFileName,
  const TSearchRec & Rec, void * Param)
{
  TMakeLocalFileListParams & Params = *NB_STATIC_DOWNCAST(TMakeLocalFileListParams, Param);

  bool Directory = FLAGSET(Rec.Attr, faDirectory);
  if (Directory && Params.Recursive)
  {
    ProcessLocalDirectory(AFileName, MAKE_CALLBACK(TTerminal::MakeLocalFileList, this), &Params);
  }

  if (!Directory || Params.IncludeDirs)
  {
    Params.FileList->Add(AFileName);
    if (Params.FileTimes != nullptr)
    {
      TODO("Add TSearchRec::TimeStamp");
      // Params.FileTimes->push_back(const_cast<TSearchRec &>(Rec).TimeStamp);
    }
  }
}

void TTerminal::CalculateLocalFileSize(const UnicodeString & AFileName,
  const TSearchRec & Rec, /*int64_t*/ void * Params)
{
  TCalculateSizeParams * AParams = NB_STATIC_DOWNCAST(TCalculateSizeParams, Params);

  bool Dir = FLAGSET(Rec.Attr, faDirectory);

  bool AllowTransfer = (AParams->CopyParam == nullptr);
  // SearchRec.Size in C++B2010 is int64_t,
  // so we should be able to use it instead of FindData.nFileSize*
  int64_t Size =
    (static_cast<int64_t>(Rec.FindData.nFileSizeHigh) << 32) +
    Rec.FindData.nFileSizeLow;
  if (!AllowTransfer)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = Size;
    MaskParams.Modification = ::FileTimeToDateTime(Rec.FindData.ftLastWriteTime);

    UnicodeString BaseFileName = GetBaseFileName(AFileName);
    AllowTransfer = AParams->CopyParam->AllowTransfer(BaseFileName, osLocal, Dir, MaskParams);
  }

  if (AllowTransfer)
  {
    if (!Dir)
    {
      AParams->Size += Size;
    }
    else
    {
      ProcessLocalDirectory(AFileName, MAKE_CALLBACK(TTerminal::CalculateLocalFileSize, this), Params);
    }
  }

  StartOperationWithFile(AFileName, foCalculateSize);
}

bool TTerminal::CalculateLocalFilesSize(const TStrings * AFileList,
  const TCopyParamType * CopyParam, bool AllowDirs,
  OUT int64_t & Size)
{
  bool Result = true;
  TFileOperationProgressType OperationProgress(MAKE_CALLBACK(TTerminal::DoProgress, this), MAKE_CALLBACK(TTerminal::DoFinished, this));
  TOnceDoneOperation OnceDoneOperation = odoIdle;
  OperationProgress.Start(foCalculateSize, osLocal, AFileList->GetCount());
  try__finally
  {
    SCOPE_EXIT
    {
      FOperationProgress = nullptr;
      OperationProgress.Stop();
    };
    TCalculateSizeParams Params;
    Params.Size = 0;
    Params.Params = 0;
    Params.CopyParam = CopyParam;

    DebugAssert(!FOperationProgress);
    FOperationProgress = &OperationProgress; //-V506
    for (intptr_t Index = 0; Result && (Index < AFileList->GetCount()); ++Index)
    {
      UnicodeString FileName = AFileList->GetString(Index);
      TSearchRec Rec;
      if (FileSearchRec(FileName, Rec))
      {
        if (FLAGSET(Rec.Attr, faDirectory) && !AllowDirs)
        {
          Result = false;
        }
        else
        {
          CalculateLocalFileSize(FileName, Rec, &Params);
          OperationProgress.Finish(FileName, true, OnceDoneOperation);
        }
      }
    }

    Size = Params.Size;
  }
  __finally
  {
    FOperationProgress = nullptr;
    OperationProgress.Stop();
  };

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }
  return Result;
}

struct TSynchronizeFileData : public TObject
{
NB_DECLARE_CLASS(TSynchronizeFileData)
public:
  bool Modified;
  bool New;
  bool IsDirectory;
  TChecklistItem::TFileInfo Info;
  TChecklistItem::TFileInfo MatchingRemoteFile;
  TRemoteFile * MatchingRemoteFileFile;
  intptr_t MatchingRemoteFileImageIndex;
  FILETIME LocalLastWriteTime;
};

const intptr_t sfFirstLevel = 0x01;
struct TSynchronizeData : public TObject
{
NB_DECLARE_CLASS(TSynchronizeData)
public:
  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  TTerminal::TSynchronizeMode Mode;
  intptr_t Params;
  TSynchronizeDirectoryEvent OnSynchronizeDirectory;
  TSynchronizeOptions * Options;
  intptr_t Flags;
  TStringList * LocalFileList;
  const TCopyParamType * CopyParam;
  TSynchronizeChecklist * Checklist;

  void DeleteLocalFileList()
  {
    if (LocalFileList != nullptr)
    {
      for (intptr_t Index = 0; Index < LocalFileList->GetCount(); ++Index)
      {
        TSynchronizeFileData * FileData = NB_STATIC_DOWNCAST(TSynchronizeFileData,
          LocalFileList->GetObj(Index));
        SAFE_DESTROY(FileData);
      }
      SAFE_DESTROY(LocalFileList);
    }
  }
};

TSynchronizeChecklist * TTerminal::SynchronizeCollect(const UnicodeString & LocalDirectory,
  const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory,
  TSynchronizeOptions * Options)
{
  TValueRestorer<bool> UseBusyCursorRestorer(FUseBusyCursor);
  FUseBusyCursor = false;

  std::unique_ptr<TSynchronizeChecklist> Checklist(new TSynchronizeChecklist());
  try__catch
  {
    DoSynchronizeCollectDirectory(LocalDirectory, RemoteDirectory, Mode,
      CopyParam, Params, OnSynchronizeDirectory, Options, sfFirstLevel,
      Checklist.get());
    Checklist->Sort();
  }
  /*catch(...)
  {
    delete Checklist;
    throw;
  }*/
  return Checklist.release();
}

static void AddFlagName(UnicodeString & ParamsStr, intptr_t & Params, intptr_t Param, const UnicodeString & Name)
{
  if (FLAGSET(Params, Param))
  {
    AddToList(ParamsStr, Name, L", ");
  }
  Params &= ~Param;
}

UnicodeString TTerminal::SynchronizeModeStr(TSynchronizeMode Mode)
{
  UnicodeString ModeStr;
  switch (Mode)
  {
    case smRemote:
      ModeStr = L"Remote";
      break;
    case smLocal:
      ModeStr = L"Local";
      break;
    case smBoth:
      ModeStr = L"Both";
      break;
    default:
      ModeStr = L"Unknown";
      break;
  }
  return ModeStr;
}

UnicodeString TTerminal::SynchronizeParamsStr(intptr_t Params)
{
  UnicodeString ParamsStr;
  AddFlagName(ParamsStr, Params, spDelete, L"Delete");
  AddFlagName(ParamsStr, Params, spNoConfirmation, L"NoConfirmation");
  AddFlagName(ParamsStr, Params, spExistingOnly, L"ExistingOnly");
  AddFlagName(ParamsStr, Params, spNoRecurse, L"NoRecurse");
  AddFlagName(ParamsStr, Params, spUseCache, L"UseCache");
  AddFlagName(ParamsStr, Params, spDelayProgress, L"DelayProgress");
  AddFlagName(ParamsStr, Params, spPreviewChanges, L"*PreviewChanges"); // GUI only
  AddFlagName(ParamsStr, Params, spSubDirs, L"SubDirs");
  AddFlagName(ParamsStr, Params, spTimestamp, L"Timestamp");
  AddFlagName(ParamsStr, Params, spNotByTime, L"NotByTime");
  AddFlagName(ParamsStr, Params, spBySize, L"BySize");
  AddFlagName(ParamsStr, Params, spSelectedOnly, L"*SelectedOnly"); // GUI only
  AddFlagName(ParamsStr, Params, spMirror, L"Mirror");
  if (Params > 0)
  {
    AddToList(ParamsStr, FORMAT(L"0x%x", static_cast<int>(Params)), L", ");
  }
  return ParamsStr;
}

void TTerminal::DoSynchronizeCollectDirectory(const UnicodeString & LocalDirectory,
  const UnicodeString & ARemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options,
  intptr_t Flags, TSynchronizeChecklist * Checklist)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  TSynchronizeData Data;

  Data.LocalDirectory = ::IncludeTrailingBackslash(LocalDirectory);
  Data.RemoteDirectory = core::UnixIncludeTrailingBackslash(ARemoteDirectory);
  Data.Mode = Mode;
  Data.Params = Params;
  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;
  Data.LocalFileList = nullptr;
  Data.CopyParam = CopyParam;
  Data.Options = Options;
  Data.Flags = Flags;
  Data.Checklist = Checklist;

  LogEvent(FORMAT(L"Collecting synchronization list for local directory '%s' and remote directory '%s', "
   L"mode = %s, params = 0x%x (%s)", LocalDirectory.c_str(), ARemoteDirectory.c_str(),
    SynchronizeModeStr(Mode).c_str(), int(Params), SynchronizeParamsStr(Params).c_str()));

  if (FLAGCLEAR(Params, spDelayProgress))
  {
    DoSynchronizeProgress(Data, true);
  }

  try__finally
  {
    SCOPE_EXIT
    {
      Data.DeleteLocalFileList();
    };
    bool Found = false;
    TSearchRecChecked SearchRec;
    Data.LocalFileList = CreateSortedStringList();

    FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()), "",
    [&]()
    {
      DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
      Found = ::FindFirstChecked(Data.LocalDirectory + L"*.*", FindAttrs, SearchRec) == 0;
    });

    if (Found)
    {
      try__finally
      {
        SCOPE_EXIT
        {
          FindClose(SearchRec);
        };
        UnicodeString FileName;
        while (Found)
        {
          FileName = SearchRec.Name;
          // add dirs for recursive mode or when we are interested in newly
          // added subdirs
          // SearchRec.Size in C++B2010 is int64_t,
          // so we should be able to use it instead of FindData.nFileSize*
          int64_t Size =
            (static_cast<int64_t>(SearchRec.FindData.nFileSizeHigh) << 32) +
            SearchRec.FindData.nFileSizeLow;
          TDateTime Modification = ::FileTimeToDateTime(SearchRec.FindData.ftLastWriteTime);
          TFileMasks::TParams MaskParams;
          MaskParams.Size = Size;
          MaskParams.Modification = Modification;
          UnicodeString RemoteFileName =
            ChangeFileName(CopyParam, FileName, osLocal, false);
          UnicodeString FullLocalFileName = Data.LocalDirectory + FileName;
          UnicodeString BaseFileName = GetBaseFileName(FullLocalFileName);
          if ((FileName != THISDIRECTORY) && (FileName != PARENTDIRECTORY) &&
              CopyParam->AllowTransfer(BaseFileName, osLocal,
                FLAGSET(SearchRec.Attr, faDirectory), MaskParams) &&
              !FFileSystem->TemporaryTransferFile(FileName) &&
              (FLAGCLEAR(Flags, sfFirstLevel) ||
               (Options == nullptr) ||
               Options->MatchesFilter(FileName) ||
               Options->MatchesFilter(RemoteFileName)))
          {
            TSynchronizeFileData * FileData = new TSynchronizeFileData;

            FileData->IsDirectory = FLAGSET(SearchRec.Attr, faDirectory);
            FileData->Info.FileName = FileName;
            FileData->Info.Directory = Data.LocalDirectory;
            FileData->Info.Modification = Modification;
            FileData->Info.ModificationFmt = mfFull;
            FileData->Info.Size = Size;
            FileData->LocalLastWriteTime = SearchRec.FindData.ftLastWriteTime;
            FileData->New = true;
            FileData->Modified = false;
            Data.LocalFileList->AddObject(FileName, FileData);
            LogEvent(FORMAT(L"Local file %s included to synchronization",
               FormatFileDetailsForLog(FullLocalFileName, Modification, Size).c_str()));
          }
          else
          {
            LogEvent(FORMAT(L"Local file %s excluded from synchronization",
              FormatFileDetailsForLog(FullLocalFileName, Modification, Size).c_str()));
          }

          FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()), "",
          [&]()
          {
            Found = (::FindNextChecked(SearchRec) == 0);
          });
        }
      }
      __finally
      {
        FindClose(SearchRec);
      };

      // can we expect that ProcessDirectory would take so little time
      // that we can postpone showing progress window until anything actually happens?
      bool Cached = FLAGSET(Params, spUseCache) && GetSessionData()->GetCacheDirectories() &&
        FDirectoryCache->HasFileList(ARemoteDirectory);

      if (!Cached && FLAGSET(Params, spDelayProgress))
      {
        DoSynchronizeProgress(Data, true);
      }

      ProcessDirectory(ARemoteDirectory, MAKE_CALLBACK(TTerminal::SynchronizeCollectFile, this), &Data,
        FLAGSET(Params, spUseCache));

      TSynchronizeFileData * FileData;
      for (intptr_t Index = 0; Index < Data.LocalFileList->GetCount(); ++Index)
      {
        FileData = NB_STATIC_DOWNCAST(TSynchronizeFileData,
          Data.LocalFileList->GetObj(Index));
        // add local file either if we are going to upload it
        // (i.e. if it is updated or we want to upload even new files)
        // or if we are going to delete it (i.e. all "new"=obsolete files)
        bool Modified = (FileData->Modified && ((Mode == smBoth) || (Mode == smRemote)));
        bool New = (FileData->New &&
          ((Mode == smLocal) ||
           (((Mode == smBoth) || (Mode == smRemote)) && FLAGCLEAR(Params, spTimestamp))));

        if (New)
        {
          LogEvent(FORMAT(L"Local file %s is new",
            FormatFileDetailsForLog(UnicodeString(FileData->Info.Directory) + UnicodeString(FileData->Info.FileName),
             FileData->Info.Modification, FileData->Info.Size).c_str()));
        }

        if (Modified || New)
        {
          std::unique_ptr<TChecklistItem> ChecklistItem(new TChecklistItem());
          try__finally
          {
            ChecklistItem->IsDirectory = FileData->IsDirectory;

            ChecklistItem->Local = FileData->Info;
            ChecklistItem->FLocalLastWriteTime = FileData->LocalLastWriteTime;

            if (Modified)
            {
              DebugAssert(!FileData->MatchingRemoteFile.Directory.IsEmpty());
              ChecklistItem->Remote = FileData->MatchingRemoteFile;
              ChecklistItem->ImageIndex = FileData->MatchingRemoteFileImageIndex;
              ChecklistItem->RemoteFile = FileData->MatchingRemoteFileFile;
            }
            else
            {
              ChecklistItem->Remote.Directory = Data.RemoteDirectory;
            }

            if ((Mode == smBoth) || (Mode == smRemote))
            {
              ChecklistItem->Action =
                (Modified ? saUploadUpdate : saUploadNew);
              ChecklistItem->Checked =
                (Modified || FLAGCLEAR(Params, spExistingOnly)) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(Params, spNoRecurse) ||
                 FLAGSET(Params, spSubDirs));
            }
            else if ((Mode == smLocal) && FLAGCLEAR(Params, spTimestamp))
            {
              ChecklistItem->Action = saDeleteLocal;
              ChecklistItem->Checked =
                FLAGSET(Params, spDelete) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(Params, spNoRecurse) ||
                 FLAGSET(Params, spSubDirs));
            }

            if (ChecklistItem->Action != saNone)
            {
              Data.Checklist->Add(ChecklistItem.get());
              ChecklistItem.release();
            }
          }
          __finally
          {
//            delete ChecklistItem;
          };
        }
        else
        {
          if (FileData->Modified)
          {
            SAFE_DESTROY(FileData->MatchingRemoteFileFile);
          }
        }
      }
    }
  }
  __finally
  {
    Data.DeleteLocalFileList();
#if 0
    if (Data.LocalFileList != NULL)
    {
      for (int Index = 0; Index < Data.LocalFileList->Count; Index++)
      {
        TSynchronizeFileData * FileData = reinterpret_cast<TSynchronizeFileData*>
          (Data.LocalFileList->Objects[Index]);
        delete FileData;
      }
      delete Data.LocalFileList;
    }
#endif
  };
}

void TTerminal::SynchronizeCollectFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  try
  {
    DoSynchronizeCollectFile(AFileName, AFile, Param);
  }
  catch (ESkipFile & E)
  {
    TSuspendFileOperationProgress Suspend(OperationProgress);
    if (!HandleException(&E))
    {
      throw;
    }
  }
}

void TTerminal::DoSynchronizeCollectFile(const UnicodeString & /*AFileName*/,
  const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param)
{
  TSynchronizeData * Data = NB_STATIC_DOWNCAST(TSynchronizeData, Param);

  TFileMasks::TParams MaskParams;
  MaskParams.Size = AFile->GetSize();
  MaskParams.Modification = AFile->GetModification();
  UnicodeString LocalFileName =
    ChangeFileName(Data->CopyParam, AFile->GetFileName(), osRemote, false);
  UnicodeString FullRemoteFileName =
    core::UnixExcludeTrailingBackslash(AFile->GetFullFileName());
  UnicodeString BaseFileName = GetBaseFileName(FullRemoteFileName);
  if (Data->CopyParam->AllowTransfer(
        BaseFileName, osRemote,
        AFile->GetIsDirectory(), MaskParams) &&
      !FFileSystem->TemporaryTransferFile(AFile->GetFileName()) &&
      (FLAGCLEAR(Data->Flags, sfFirstLevel) ||
       (Data->Options == nullptr) ||
        Data->Options->MatchesFilter(AFile->GetFileName()) ||
        Data->Options->MatchesFilter(LocalFileName)))
  {
    std::unique_ptr<TChecklistItem> ChecklistItem(new TChecklistItem());
    try__finally
    {
      ChecklistItem->IsDirectory = AFile->GetIsDirectory();
      ChecklistItem->ImageIndex = AFile->GetIconIndex();

      ChecklistItem->Remote.FileName = AFile->GetFileName();
      ChecklistItem->Remote.Directory = Data->RemoteDirectory;
      ChecklistItem->Remote.Modification = AFile->GetModification();
      ChecklistItem->Remote.ModificationFmt = AFile->GetModificationFmt();
      ChecklistItem->Remote.Size = AFile->GetSize();

      bool Modified = false;
      bool New = false;
      if (AFile->GetIsDirectory() && !CanRecurseToDirectory(AFile))
      {
        LogEvent(FORMAT(L"Skipping symlink to directory \"%s\".", AFile->GetFileName().c_str()));
      }
      else
      {
        intptr_t LocalIndex = Data->LocalFileList->IndexOf(LocalFileName.c_str());
        New = (LocalIndex < 0);
        if (!New)
        {
          TSynchronizeFileData * LocalData =
            NB_STATIC_DOWNCAST(TSynchronizeFileData, Data->LocalFileList->GetObj(LocalIndex));

          LocalData->New = false;

          if (AFile->GetIsDirectory() != LocalData->IsDirectory)
          {
            LogEvent(FORMAT(L"%s is directory on one side, but file on the another",
              AFile->GetFileName().c_str()));
          }
          else if (!AFile->GetIsDirectory())
          {
            ChecklistItem->Local = LocalData->Info;

            ChecklistItem->Local.Modification =
              core::ReduceDateTimePrecision(ChecklistItem->Local.Modification, AFile->GetModificationFmt());

            bool LocalModified = false;
            // for spTimestamp+spBySize require that the file sizes are the same
            // before comparing file time
            intptr_t TimeCompare;
            if (FLAGCLEAR(Data->Params, spNotByTime) &&
                (FLAGCLEAR(Data->Params, spTimestamp) ||
                 FLAGCLEAR(Data->Params, spBySize) ||
                 (ChecklistItem->Local.Size == ChecklistItem->Remote.Size)))
            {
              TimeCompare = CompareFileTime(ChecklistItem->Local.Modification,
                   ChecklistItem->Remote.Modification);
            }
            else
            {
              TimeCompare = 0;
            }
            if (TimeCompare < 0)
            {
              if ((FLAGCLEAR(Data->Params, spTimestamp) && FLAGCLEAR(Data->Params, spMirror)) ||
                  (Data->Mode == smBoth) || (Data->Mode == smLocal))
              {
                Modified = true;
              }
              else
              {
                LocalModified = true;
              }
            }
            else if (TimeCompare > 0)
            {
              if ((FLAGCLEAR(Data->Params, spTimestamp) && FLAGCLEAR(Data->Params, spMirror)) ||
                  (Data->Mode == smBoth) || (Data->Mode == smRemote))
              {
                LocalModified = true;
              }
              else
              {
                Modified = true;
              }
            }
            else if (FLAGSET(Data->Params, spBySize) &&
                     (ChecklistItem->Local.Size != ChecklistItem->Remote.Size) &&
                     FLAGCLEAR(Data->Params, spTimestamp))
            {
              Modified = true;
              LocalModified = true;
            }

            if (LocalModified)
            {
              LocalData->Modified = true;
              LocalData->MatchingRemoteFile = ChecklistItem->Remote;
              LocalData->MatchingRemoteFileImageIndex = ChecklistItem->ImageIndex;
              // we need this for custom commands over checklist only,
              // not for sync itself
              LocalData->MatchingRemoteFileFile = AFile->Duplicate();
              LogEvent(FORMAT(L"Local file %s is modified comparing to remote file %s",
                 FormatFileDetailsForLog(UnicodeString(LocalData->Info.Directory) + UnicodeString(LocalData->Info.FileName),
                   LocalData->Info.Modification, LocalData->Info.Size).c_str(),
                 FormatFileDetailsForLog(FullRemoteFileName,
                   AFile->GetModification(), AFile->GetSize()).c_str()));
            }

            if (Modified)
            {
              LogEvent(FORMAT(L"Remote file %s is modified comparing to local file %s",
                 FormatFileDetailsForLog(FullRemoteFileName,
                   AFile->GetModification(),
                   AFile->GetSize()).c_str(),
                 FormatFileDetailsForLog(UnicodeString(LocalData->Info.Directory) + UnicodeString(LocalData->Info.FileName),
                   LocalData->Info.Modification, LocalData->Info.Size).c_str()));
            }
          }
          else if (FLAGCLEAR(Data->Params, spNoRecurse))
          {
            DoSynchronizeCollectDirectory(
              Data->LocalDirectory + LocalData->Info.FileName,
              Data->RemoteDirectory + AFile->GetFileName(),
              Data->Mode, Data->CopyParam, Data->Params, Data->OnSynchronizeDirectory,
              Data->Options, (Data->Flags & ~sfFirstLevel),
              Data->Checklist);
          }
        }
        else
        {
          ChecklistItem->Local.Directory = Data->LocalDirectory;
          LogEvent(FORMAT(L"Remote file %s is new",
            FormatFileDetailsForLog(FullRemoteFileName, AFile->GetModification(), AFile->GetSize()).c_str()));
        }
      }

      if (New || Modified)
      {
        DebugAssert(!New || !Modified);

        // download the file if it changed or is new and we want to have it locally
        if ((Data->Mode == smBoth) || (Data->Mode == smLocal))
        {
          if (FLAGCLEAR(Data->Params, spTimestamp) || Modified)
          {
            ChecklistItem->Action =
              (Modified ? saDownloadUpdate : saDownloadNew);
            ChecklistItem->Checked =
              (Modified || FLAGCLEAR(Data->Params, spExistingOnly)) &&
              (!ChecklistItem->IsDirectory || FLAGCLEAR(Data->Params, spNoRecurse) ||
               FLAGSET(Data->Params, spSubDirs));
          }
        }
        else if ((Data->Mode == smRemote) && New)
        {
          if (FLAGCLEAR(Data->Params, spTimestamp))
          {
            ChecklistItem->Action = saDeleteRemote;
            ChecklistItem->Checked =
              FLAGSET(Data->Params, spDelete) &&
              (!ChecklistItem->IsDirectory || FLAGCLEAR(Data->Params, spNoRecurse) ||
               FLAGSET(Data->Params, spSubDirs));
          }
        }

        if (ChecklistItem->Action != saNone)
        {
          ChecklistItem->RemoteFile = AFile->Duplicate();
          Data->Checklist->Add(ChecklistItem.get());
          ChecklistItem.release();
        }
      }
    }
    __finally
    {
//      delete ChecklistItem;
    };
  }
  else
  {
    LogEvent(FORMAT(L"Remote file %s excluded from synchronization",
       FormatFileDetailsForLog(FullRemoteFileName, AFile->GetModification(), AFile->GetSize()).c_str()));
  }
}

void TTerminal::SynchronizeApply(TSynchronizeChecklist * Checklist,
  const UnicodeString & /*LocalDirectory*/, const UnicodeString & /*RemoteDirectory*/,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory)
{
  TSynchronizeData Data;

  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;

  intptr_t CopyParams =
    FLAGMASK(FLAGSET(Params, spNoConfirmation), cpNoConfirmation);

  TCopyParamType SyncCopyParam = *CopyParam;
  // when synchronizing by time, we force preserving time,
  // otherwise it does not make any sense
  if (FLAGCLEAR(Params, spNotByTime))
  {
    SyncCopyParam.SetPreserveTime(true);
  }

  std::unique_ptr<TStringList> DownloadList(new TStringList());
  std::unique_ptr<TStringList> DeleteRemoteList(new TStringList());
  std::unique_ptr<TStringList> UploadList(new TStringList());
  std::unique_ptr<TStringList> DeleteLocalList(new TStringList());

  BeginTransaction();

  try__finally
  {
    SCOPE_EXIT
    {
      EndTransaction();
    };
    intptr_t IIndex = 0;
    while (IIndex < Checklist->GetCount())
    {
      const TChecklistItem * ChecklistItem;

      DownloadList->Clear();
      DeleteRemoteList->Clear();
      UploadList->Clear();
      DeleteLocalList->Clear();

      ChecklistItem = Checklist->GetItem(IIndex);

      UnicodeString CurrentLocalDirectory = ChecklistItem->Local.Directory;
      UnicodeString CurrentRemoteDirectory = ChecklistItem->Remote.Directory;

      LogEvent(FORMAT(L"Synchronizing local directory '%s' with remote directory '%s', "
        L"params = 0x%x (%s)", CurrentLocalDirectory.c_str(), CurrentRemoteDirectory.c_str(),
        int(Params), SynchronizeParamsStr(Params).c_str()));

      intptr_t Count = 0;

      while ((IIndex < Checklist->GetCount()) &&
             (Checklist->GetItem(IIndex)->Local.Directory == CurrentLocalDirectory) &&
             (Checklist->GetItem(IIndex)->Remote.Directory == CurrentRemoteDirectory))
      {
        ChecklistItem = Checklist->GetItem(IIndex);
        if (ChecklistItem->Checked)
        {
          Count++;

          if (FLAGSET(Params, spTimestamp))
          {
            switch (ChecklistItem->Action)
            {
              case saDownloadUpdate:
                DownloadList->AddObject(
                  core::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                    ChecklistItem->Remote.FileName,
                  const_cast<TChecklistItem *>(ChecklistItem));
                break;

              case saUploadUpdate:
                UploadList->AddObject(
                  ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                    ChecklistItem->Local.FileName,
                  const_cast<TChecklistItem *>(ChecklistItem));
                break;

              default:
                DebugFail();
                break;
            }
          }
          else
          {
            switch (ChecklistItem->Action)
            {
              case saDownloadNew:
              case saDownloadUpdate:
                DownloadList->AddObject(
                  core::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                    ChecklistItem->Remote.FileName,
                  ChecklistItem->RemoteFile);
                break;

              case saDeleteRemote:
                DeleteRemoteList->AddObject(
                  core::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                    ChecklistItem->Remote.FileName,
                  ChecklistItem->RemoteFile);
                break;

              case saUploadNew:
              case saUploadUpdate:
                UploadList->Add(
                  ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                    ChecklistItem->Local.FileName);
                break;

              case saDeleteLocal:
                DeleteLocalList->Add(
                  ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                    ChecklistItem->Local.FileName);
                break;

              default:
                DebugFail();
                break;
            }
          }
        }
        ++IIndex;
      }

      // prevent showing/updating of progress dialog if there's nothing to do
      if (Count > 0)
      {
        Data.LocalDirectory = ::IncludeTrailingBackslash(CurrentLocalDirectory);
        Data.RemoteDirectory = core::UnixIncludeTrailingBackslash(CurrentRemoteDirectory);
        DoSynchronizeProgress(Data, false);

        if (FLAGSET(Params, spTimestamp))
        {
          if (DownloadList->GetCount() > 0)
          {
            ProcessFiles(DownloadList.get(), foSetProperties,
              MAKE_CALLBACK(TTerminal::SynchronizeLocalTimestamp, this), nullptr, osLocal);
          }

          if (UploadList->GetCount() > 0)
          {
            ProcessFiles(UploadList.get(), foSetProperties,
              MAKE_CALLBACK(TTerminal::SynchronizeRemoteTimestamp, this));
          }
        }
        else
        {
          if ((DownloadList->GetCount() > 0) &&
              !CopyToLocal(DownloadList.get(), Data.LocalDirectory, &SyncCopyParam, CopyParams))
          {
            Abort();
          }

          if ((DeleteRemoteList->GetCount() > 0) &&
              !RemoteDeleteFiles(DeleteRemoteList.get()))
          {
            Abort();
          }

          if ((UploadList->GetCount() > 0) &&
              !CopyToRemote(UploadList.get(), Data.RemoteDirectory, &SyncCopyParam, CopyParams))
          {
            Abort();
          }

          if ((DeleteLocalList->GetCount() > 0) &&
              !DeleteLocalFiles(DeleteLocalList.get()))
          {
            Abort();
          }
        }
      }
    }
  }
  __finally
  {
#if 0
    delete DownloadList;
    delete DeleteRemoteList;
    delete UploadList;
    delete DeleteLocalList;
#endif

    EndTransaction();
  };
}

void TTerminal::DoSynchronizeProgress(const TSynchronizeData & Data,
  bool Collect)
{
  if (Data.OnSynchronizeDirectory)
  {
    bool Continue = true;
    Data.OnSynchronizeDirectory(Data.LocalDirectory, Data.RemoteDirectory,
      Continue, Collect);

    if (!Continue)
    {
      Abort();
    }
  }
}

void TTerminal::SynchronizeLocalTimestamp(const UnicodeString & /*FileName*/,
  const TRemoteFile * AFile, void * /*Param*/)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();

  const TChecklistItem * ChecklistItem =
    reinterpret_cast<const TChecklistItem *>(AFile);

  UnicodeString LocalFile =
    ::IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
      ChecklistItem->Local.FileName;
  FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, LocalFile.c_str()), "",
  [&]()
  {
    SetLocalFileTime(LocalFile, ChecklistItem->Remote.Modification);
  });
}

void TTerminal::SynchronizeRemoteTimestamp(const UnicodeString & /*AFileName*/,
  const TRemoteFile * AFile, void * /*Param*/)
{
  const TChecklistItem * ChecklistItem =
    reinterpret_cast<const TChecklistItem *>(AFile);

  TRemoteProperties Properties;
  Properties.Valid << vpModification;
  Properties.Modification = ::ConvertTimestampToUnix(ChecklistItem->FLocalLastWriteTime,
    GetSessionData()->GetDSTMode());

  ChangeFileProperties(
    core::UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) + ChecklistItem->Remote.FileName,
    nullptr, &Properties);
}

void TTerminal::FileFind(const UnicodeString & AFileName,
  const TRemoteFile * AFile, /*TFilesFindParams*/ void * Param)
{
  // see DoFilesFind
  FOnFindingFile = nullptr;

  DebugAssert(Param);
  DebugAssert(AFile);
  TFilesFindParams * AParams = NB_STATIC_DOWNCAST(TFilesFindParams, Param);

  if (!AParams->Cancel)
  {
    UnicodeString LocalFileName = AFileName;
    if (AFileName.IsEmpty())
    {
      LocalFileName = AFile->GetFileName();
    }

    TFileMasks::TParams MaskParams;
    MaskParams.Size = AFile->GetSize();
    MaskParams.Modification = AFile->GetModification();

    UnicodeString FullFileName = core::UnixExcludeTrailingBackslash(AFile->GetFullFileName());
    bool ImplicitMatch = false;
    // Do not use recursive include match
    if (AParams->FileMask.Matches(FullFileName, false,
         AFile->GetIsDirectory(), &MaskParams, false, ImplicitMatch))
    {
      if (!ImplicitMatch)
      {
        AParams->OnFileFound(this, LocalFileName, AFile, AParams->Cancel);
      }

      if (AFile->GetIsDirectory())
      {
        UnicodeString RealDirectory;
        if (!AFile->GetIsSymLink() || AFile->GetLinkTo().IsEmpty())
        {
          RealDirectory = core::UnixIncludeTrailingBackslash(AParams->RealDirectory) + AFile->GetFileName();
        }
        else
        {
          RealDirectory = core::AbsolutePath(AParams->RealDirectory, AFile->GetLinkTo());
        }

        if (!AParams->LoopDetector.IsUnvisitedDirectory(RealDirectory))
        {
          LogEvent(FORMAT(L"Already searched \"%s\" directory, link loop detected", FullFileName.c_str()));
        }
        else
        {
          DoFilesFind(FullFileName, *AParams, RealDirectory);
        }
      }
    }
  }
}

void TTerminal::DoFilesFind(const UnicodeString & Directory, TFilesFindParams & Params, const UnicodeString & RealDirectory)
{
  LogEvent(FORMAT(L"Searching directory \"%s\" (real path \"%s\")", Directory.c_str(), RealDirectory.c_str()));
  Params.OnFindingFile(this, Directory, Params.Cancel);
  if (!Params.Cancel)
  {
    DebugAssert(FOnFindingFile == nullptr);
    // ideally we should set the handler only around actually reading
    // of the directory listing, so we at least reset the handler in
    // FileFind
    FOnFindingFile = Params.OnFindingFile;
    UnicodeString PrevRealDirectory = Params.RealDirectory;
    try__finally
    {
      SCOPE_EXIT
      {
        Params.RealDirectory = PrevRealDirectory;
        FOnFindingFile = nullptr;
      };
      Params.RealDirectory = RealDirectory;
      ProcessDirectory(Directory, MAKE_CALLBACK(TTerminal::FileFind, this), &Params, false, true);
    }
    __finally
    {
      Params.RealDirectory = PrevRealDirectory;
      FOnFindingFile = nullptr;
    };
  }
}

void TTerminal::FilesFind(const UnicodeString & Directory, const TFileMasks & FileMask,
  TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)
{
  TFilesFindParams Params;
  Params.FileMask = FileMask;
  Params.OnFileFound = OnFileFound;
  Params.OnFindingFile = OnFindingFile;
  Params.Cancel = false;

  Params.LoopDetector.RecordVisitedDirectory(Directory);

  DoFilesFind(Directory, Params, Directory);
}

void TTerminal::SpaceAvailable(const UnicodeString & APath,
  TSpaceAvailable & ASpaceAvailable)
{
  DebugAssert(GetIsCapable(fcCheckingSpaceAvailable));

  try
  {
    FFileSystem->SpaceAvailable(APath, ASpaceAvailable);
  }
  catch (Exception & E)
  {
    CommandError(&E, FMTLOAD(SPACE_AVAILABLE_ERROR, APath.c_str()));
  }
}

void TTerminal::LockFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foLock);

  LogEvent(FORMAT(L"Locking file \"%s\".", AFileName.c_str()));
  FileModified(AFile, AFileName, true);

  DoLockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}

void TTerminal::DoLockFile(const UnicodeString & AFileName, const TRemoteFile * AFile)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FFileSystem->LockFile(AFileName, AFile);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(LOCK_FILE_ERROR, AFileName.c_str()));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::UnlockFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * /*Param*/)
{
  StartOperationWithFile(AFileName, foUnlock);

  LogEvent(FORMAT(L"Unlocking file \"%s\".", AFileName.c_str()));
  FileModified(AFile, AFileName, true);

  DoUnlockFile(AFileName, AFile);
  ReactOnCommand(fsLock);
}

void TTerminal::DoUnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile)
{
  TRetryOperationLoop RetryLoop(this);
  do
  {
    try
    {
      FFileSystem->UnlockFile(AFileName, AFile);
    }
    catch (Exception & E)
    {
      RetryLoop.Error(E, FMTLOAD(UNLOCK_FILE_ERROR, AFileName.c_str()));
    }
  }
  while (RetryLoop.Retry());
}

void TTerminal::LockFiles(TStrings * AFileList)
{
  BeginTransaction();
  try__finally
  {
    SCOPE_EXIT
    {
      EndTransaction();
    };
    {
      ProcessFiles(AFileList, foLock, MAKE_CALLBACK(TTerminal::LockFile, this), nullptr);
    }
  }
  __finally
  {
    EndTransaction();
  };
}

void TTerminal::UnlockFiles(TStrings * AFileList)
{
  BeginTransaction();
  try__finally
  {
    SCOPE_EXIT
    {
      EndTransaction();
    };
    {
      ProcessFiles(AFileList, foUnlock, MAKE_CALLBACK(TTerminal::UnlockFile, this), nullptr);
    }
  }
  __finally
  {
    EndTransaction();
  };
}

const TSessionInfo & TTerminal::GetSessionInfo() const
{
  return FFileSystem->GetSessionInfo();
}

const TFileSystemInfo & TTerminal::GetFileSystemInfo(bool Retrieve)
{
  return FFileSystem->GetFileSystemInfo(Retrieve);
}

void TTerminal::GetSupportedChecksumAlgs(TStrings * Algs)
{
  FFileSystem->GetSupportedChecksumAlgs(Algs);
}

UnicodeString TTerminal::GetPassword() const
{
  UnicodeString Result;
  // FRememberedPassword is empty also when stored password was used
  if (FRememberedPassword.IsEmpty())
  {
    Result = GetSessionData()->GetPassword();
  }
  else
  {
    Result = GetRememberedPassword();
  }
  return Result;
}

UnicodeString TTerminal::GetRememberedPassword() const
{
  return DecryptPassword(FRememberedPassword);
}

UnicodeString TTerminal::GetRememberedTunnelPassword() const
{
  return DecryptPassword(FRememberedTunnelPassword);
}

bool TTerminal::GetStoredCredentialsTried() const
{
  bool Result;
  if (FFileSystem != nullptr)
  {
    Result = FFileSystem->GetStoredCredentialsTried();
  }
  else if (FSecureShell != nullptr)
  {
    Result = FSecureShell->GetStoredCredentialsTried();
  }
  else
  {
    DebugAssert(FTunnelOpening);
    Result = false;
  }
  return Result;
}

bool TTerminal::CopyToRemote(const TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params)
{
  DebugAssert(FFileSystem);
  DebugAssert(AFilesToCopy);

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  TFileOperationProgressType OperationProgress(MAKE_CALLBACK(TTerminal::DoProgress, this), MAKE_CALLBACK(TTerminal::DoFinished, this));
  try
  {
    int64_t Size = 0;
    // dirty trick: when moving, do not pass copy param to avoid exclude mask
    bool CalculatedSize =
      CalculateLocalFilesSize(
        AFilesToCopy,
        (FLAGCLEAR(Params, cpDelete) ? CopyParam : nullptr),
        CopyParam->GetCalculateSize(),
        Size);

    OperationProgress.Start((Params & cpDelete) ? foMove : foCopy, osLocal,
      AFilesToCopy->GetCount(), (Params & cpTemporary) > 0, TargetDir, CopyParam->GetCPSLimit());

    FOperationProgress = &OperationProgress; //-V506
    //bool CollectingUsage = false;
    {
      SCOPE_EXIT
      {
        OperationProgress.Stop();
        FOperationProgress = nullptr;
      };
      if (CalculatedSize)
      {
#if 0
        if (Configuration->Usage->Collect)
        {
          int CounterSize = TUsage::CalculateCounterSize(Size);
          Configuration->Usage->Inc(L"Uploads");
          Configuration->Usage->Inc(L"UploadedBytes", CounterSize);
          Configuration->Usage->SetMax(L"MaxUploadSize", CounterSize);
          CollectingUsage = true;
        }
#endif
        OperationProgress.SetTotalSize(Size);
      }

      UnicodeString UnlockedTargetDir = TranslateLockedPath(TargetDir, false);
      BeginTransaction();
      try__finally
      {
        SCOPE_EXIT
        {
          if (GetActive())
          {
            ReactOnCommand(fsCopyToRemote);
          }
          EndTransaction();
        };
        if (GetLog()->GetLogging())
        {
          LogEvent(FORMAT(L"Copying %d files/directories to remote directory "
            L"%s\"", AFilesToCopy->GetCount(), TargetDir.c_str()));
          LogEvent(CopyParam->GetLogStr());
        }

        FFileSystem->CopyToRemote(AFilesToCopy, UnlockedTargetDir,
          CopyParam, Params, &OperationProgress, OnceDoneOperation);
      }
      __finally
      {
        if (GetActive())
        {
          ReactOnCommand(fsCopyToRemote);
        }
        EndTransaction();
      };

      if (OperationProgress.Cancel == csContinue)
      {
        Result = true;
      }
    }
    __finally
    {
      /*if (GetCollectingUsage())
      {
        int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
        Configuration->Usage->Inc(L"UploadTime", CounterTime);
        Configuration->Usage->SetMax(L"MaxUploadTime", CounterTime);
      }*/
      OperationProgress.Stop();
      FOperationProgress = nullptr;
    };
  }
  catch (Exception & E)
  {
    if (OperationProgress.Cancel != csCancel)
    {
      CommandError(&E, MainInstructions(LoadStr(TOREMOTE_COPY_ERROR)));
    }
    OnceDoneOperation = odoIdle;
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}

bool TTerminal::CopyToLocal(const TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params)
{
  DebugAssert(FFileSystem);

  // see scp.c: sink(), tolocal()

  bool Result = false;
  bool OwnsFileList = (AFilesToCopy == nullptr);
  std::unique_ptr<TStrings> FilesToCopy;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  try__finally
  {
    if (OwnsFileList)
    {
      FilesToCopy.reset(new TStringList());
      FilesToCopy->Assign(GetFiles()->GetSelectedFiles());
      AFilesToCopy = FilesToCopy.get();
    }

    BeginTransaction();
    try__finally
    {
      SCOPE_EXIT
      {
        // If session is still active (no fatal error) we reload directory
        // by calling EndTransaction
        EndTransaction();
      };
      int64_t TotalSize = 0;
      bool TotalSizeKnown = false;
      TFileOperationProgressType OperationProgress(MAKE_CALLBACK(TTerminal::DoProgress, this), MAKE_CALLBACK(TTerminal::DoFinished, this));

      SetExceptionOnFail(true);
      try__finally
      {
        SCOPE_EXIT
        {
          SetExceptionOnFail(false);
        };
        // dirty trick: when moving, do not pass copy param to avoid exclude mask
        if (CalculateFilesSize(
             AFilesToCopy, TotalSize, csIgnoreErrors,
             (FLAGCLEAR(Params, cpDelete) ? CopyParam : nullptr),
             CopyParam->GetCalculateSize(), nullptr))
        {
          TotalSizeKnown = true;
        }
      }
      __finally
      {
        SetExceptionOnFail(false);
      };

      OperationProgress.Start(((Params & cpDelete) != 0 ? foMove : foCopy), osRemote,
        AFilesToCopy->GetCount(), (Params & cpTemporary) != 0, TargetDir, CopyParam->GetCPSLimit());

      FOperationProgress = &OperationProgress; //-V506
      //bool CollectingUsage = false;
      try__finally
      {
        SCOPE_EXIT
        {
#if 0
          if (CollectingUsage)
          {
            int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
            Configuration->Usage->Inc(L"DownloadTime", CounterTime);
            Configuration->Usage->SetMax(L"MaxDownloadTime", CounterTime);
          }
#endif
          FOperationProgress = nullptr;
          OperationProgress.Stop();
        };
        if (TotalSizeKnown)
        {
#if 0
          if (Configuration->Usage->Collect)
          {
            int CounterTotalSize = TUsage::CalculateCounterSize(TotalSize);
            Configuration->Usage->Inc(L"Downloads");
            Configuration->Usage->Inc(L"DownloadedBytes", CounterTotalSize);
            Configuration->Usage->SetMax(L"MaxDownloadSize", CounterTotalSize);
            CollectingUsage = true;
          }
#endif
          OperationProgress.SetTotalSize(TotalSize);
        }

        try
        {
          try__finally
          {
            SCOPE_EXIT
            {
              if (GetActive())
              {
                ReactOnCommand(fsCopyToLocal);
              }
            };
            if (GetLog()->GetLogging())
            {
              LogEvent(FORMAT(L"Copying %d files/directories to local directory "
                L"\"%s\"", AFilesToCopy->GetCount(), TargetDir.c_str()));
              LogEvent(CopyParam->GetLogStr());
            }

            FFileSystem->CopyToLocal(AFilesToCopy, TargetDir, CopyParam, Params,
              &OperationProgress, OnceDoneOperation);
          }
          __finally
          {
            if (GetActive())
            {
              ReactOnCommand(fsCopyToLocal);
            }
          };
        }
        catch (Exception & E)
        {
          if (OperationProgress.Cancel != csCancel)
          {
            CommandError(&E, MainInstructions(LoadStr(TOLOCAL_COPY_ERROR)));
          }
          OnceDoneOperation = odoIdle;
        }

        if (OperationProgress.Cancel == csContinue)
        {
          Result = true;
        }
      }
      __finally
      {
#if 0
        if (CollectingUsage)
        {
          int CounterTime = TimeToSeconds(OperationProgress.TimeElapsed());
          Configuration->Usage->Inc(L"DownloadTime", CounterTime);
          Configuration->Usage->SetMax(L"MaxDownloadTime", CounterTime);
        }
#endif
        FOperationProgress = nullptr;
        OperationProgress.Stop();
      };
    }
    __finally
    {
      // If session is still active (no fatal error) we reload directory
      // by calling EndTransaction
      EndTransaction();
    };
  }
  __finally
  {
//    if (OwnsFileList) delete FilesToCopy;
  };

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}

void TTerminal::SetLocalFileTime(const UnicodeString & LocalFileName,
  const TDateTime & Modification)
{
  FILETIME WrTime = ::DateTimeToFileTime(Modification,
    GetSessionData()->GetDSTMode());
  SetLocalFileTime(LocalFileName, nullptr, &WrTime);
}

void TTerminal::SetLocalFileTime(const UnicodeString & LocalFileName,
  FILETIME * AcTime, FILETIME * WrTime)
{
#ifndef __linux__
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  FileOperationLoopCustom(this, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, LocalFileName.c_str()), "",
  [&]()
  {
    HANDLE LocalFileHandle;
    this->TerminalOpenLocalFile(LocalFileName, GENERIC_WRITE,
      &LocalFileHandle, nullptr, nullptr, nullptr, nullptr, nullptr);
    bool Result = ::SetFileTime(LocalFileHandle, nullptr, AcTime, WrTime) > 0;
    ::CloseHandle(LocalFileHandle);
    if (!Result)
    {
      Abort();
    }
  });
#endif
}

HANDLE TTerminal::TerminalCreateLocalFile(const UnicodeString & LocalFileName, DWORD DesiredAccess,
  DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  if (GetOnCreateLocalFile())
  {
    return GetOnCreateLocalFile()(ApiPath(LocalFileName), DesiredAccess, ShareMode, CreationDisposition, FlagsAndAttributes);
  }
  else
  {
    return ::CreateFile(ApiPath(LocalFileName).c_str(), DesiredAccess, ShareMode, nullptr, CreationDisposition, FlagsAndAttributes, 0);
  }
}

DWORD TTerminal::GetLocalFileAttributes(const UnicodeString & LocalFileName)
{
  if (GetOnGetLocalFileAttributes())
  {
    return GetOnGetLocalFileAttributes()(ApiPath(LocalFileName));
  }
  else
  {
    return ::FileGetAttrFix(LocalFileName);
  }
}

BOOL TTerminal::SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes)
{
  if (GetOnSetLocalFileAttributes())
  {
    return GetOnSetLocalFileAttributes()(ApiPath(LocalFileName), FileAttributes);
  }
  else
  {
    return ::FileSetAttr(LocalFileName, FileAttributes) != 0;
  }
}

BOOL TTerminal::MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags)
{
  if (GetOnMoveLocalFile())
  {
    return GetOnMoveLocalFile()(LocalFileName, NewLocalFileName, Flags);
  }
  else
  {
    return ::MoveFileEx(ApiPath(LocalFileName).c_str(), ApiPath(NewLocalFileName).c_str(), Flags) != 0;
  }
}

BOOL TTerminal::RemoveLocalDirectory(const UnicodeString & LocalDirName)
{
  if (GetOnRemoveLocalDirectory())
  {
    return GetOnRemoveLocalDirectory()(LocalDirName);
  }
  else
  {
    return ::RemoveDir(LocalDirName);
  }
}

BOOL TTerminal::CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  if (GetOnCreateLocalDirectory())
  {
    return GetOnCreateLocalDirectory()(LocalDirName, SecurityAttributes);
  }
  else
  {
    return ::CreateDir(LocalDirName, SecurityAttributes);
  }
}

void TTerminal::ReflectSettings()
{
  DebugAssert(FLog != nullptr);
  FLog->ReflectSettings();
  DebugAssert(FActionLog != nullptr);
  FActionLog->ReflectSettings();
  // also FTunnelLog ?
}

void TTerminal::CollectUsage()
{
  switch (GetSessionData()->GetFSProtocol())
  {
    case fsSCPonly:
//      Configuration->Usage->Inc(L"OpenedSessionsSCP");
      break;

    case fsSFTP:
    case fsSFTPonly:
//      Configuration->Usage->Inc(L"OpenedSessionsSFTP");
      break;

    case fsFTP:
      if (GetSessionData()->GetFtps() == ftpsNone)
      {
//        Configuration->Usage->Inc(L"OpenedSessionsFTP");
      }
      else
      {
//        Configuration->Usage->Inc(L"OpenedSessionsFTPS");
      }
      break;

    case fsWebDAV:
      if (GetSessionData()->GetFtps() == ftpsNone)
      {
//        Configuration->Usage->Inc(L"OpenedSessionsWebDAV");
      }
      else
      {
//        Configuration->Usage->Inc(L"OpenedSessionsWebDAVS");
      }
      break;
  }

  if (GetConfiguration()->GetLogging() && GetConfiguration()->GetLogToFile())
  {
//    Configuration->Usage->Inc(L"OpenedSessionsLogToFile2");
  }

  if (GetConfiguration()->GetLogActions())
  {
//    Configuration->Usage->Inc(L"OpenedSessionsXmlLog");
  }

  std::unique_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
  if (!GetSessionData()->IsSame(FactoryDefaults.get(), true, nullptr))
  {
//    Configuration->Usage->Inc(L"OpenedSessionsAdvanced");
  }

  if (GetSessionData()->GetProxyMethod() != ::pmNone)
  {
//    Configuration->Usage->Inc(L"OpenedSessionsProxy");
  }
  if (GetSessionData()->GetFtpProxyLogonType() > 0)
  {
//    Configuration->Usage->Inc(L"OpenedSessionsFtpProxy");
  }

  FCollectFileSystemUsage = true;
}

bool TTerminal::CheckForEsc()
{
  if (FOnCheckForEsc)
    return FOnCheckForEsc();
  else
    return (FOperationProgress && FOperationProgress->Cancel == csCancel);
}

static UnicodeString FormatCertificateData(const UnicodeString & Fingerprint, int Failures)
{
  return FORMAT(L"%s;%2.2X", Fingerprint.c_str(), Failures);
}

bool TTerminal::VerifyCertificate(
  const UnicodeString & CertificateStorageKey, const UnicodeString & SiteKey,
  const UnicodeString & Fingerprint,
  const UnicodeString & CertificateSubject, int Failures)
{
  bool Result = false;

  UnicodeString CertificateData = FormatCertificateData(Fingerprint, Failures);

  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
  Storage->SetAccessMode(smRead);

  if (Storage->OpenSubKey(CertificateStorageKey, false))
  {
    if (Storage->ValueExists(SiteKey))
    {
      UnicodeString CachedCertificateData = Storage->ReadString(SiteKey, L"");
      if (CertificateData == CachedCertificateData)
      {
        LogEvent(FORMAT(L"Certificate for \"%s\" matches cached fingerprint and failures", CertificateSubject.c_str()));
        Result = true;
      }
    }
    else if (Storage->ValueExists(Fingerprint))
    {
      LogEvent(FORMAT(L"Certificate for \"%s\" matches legacy cached fingerprint", CertificateSubject.c_str()));
      Result = true;
    }
  }

  if (!Result)
  {
    UnicodeString Buf = GetSessionData()->GetHostKey();
    while (!Result && !Buf.IsEmpty())
    {
      UnicodeString ExpectedKey = CutToChar(Buf, L';', false);
      if (ExpectedKey == L"*")
      {
        UnicodeString Message = LoadStr(ANY_CERTIFICATE);
        Information(Message, true);
        GetLog()->Add(llException, Message);
        Result = true;
      }
      else if (ExpectedKey == Fingerprint)
      {
        LogEvent(FORMAT(L"Certificate for \"%s\" matches configured fingerprint", CertificateSubject.c_str()));
        Result = true;
      }
    }
  }

  return Result;
}

void TTerminal::CacheCertificate(const UnicodeString & CertificateStorageKey,
  const UnicodeString & SiteKey, const UnicodeString & Fingerprint, int Failures)
{
  UnicodeString CertificateData = FormatCertificateData(Fingerprint, Failures);

  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);

  if (Storage->OpenSubKey(CertificateStorageKey, true))
  {
    Storage->WriteString(SiteKey, CertificateData);
  }
}

void TTerminal::CollectTlsUsage(const UnicodeString & TlsVersionStr)
{
  // see SSL_get_version() in OpenSSL ssl_lib.c
  if (TlsVersionStr == L"TLSv1.2")
  {
//    Configuration->Usage->Inc(L"OpenedSessionsTLS12");
  }
  else if (TlsVersionStr == L"TLSv1.1")
  {
//    Configuration->Usage->Inc(L"OpenedSessionsTLS11");
  }
  else if (TlsVersionStr == L"TLSv1")
  {
//    Configuration->Usage->Inc(L"OpenedSessionsTLS10");
  }
  else if (TlsVersionStr == L"SSLv3")
  {
//    Configuration->Usage->Inc(L"OpenedSessionsSSL30");
  }
  else if (TlsVersionStr == L"SSLv2")
  {
//    Configuration->Usage->Inc(L"OpenedSessionsSSL20");
  }
  else
  {
//    DebugFail();
  }
}

bool TTerminal::LoadTlsCertificate(X509 *& Certificate, EVP_PKEY *& PrivateKey)
{
  bool Result = !GetSessionData()->GetTlsCertificateFile().IsEmpty();
  if (Result)
  {
    UnicodeString Passphrase = GetSessionData()->GetPassphrase();

    // Inspired by neon's ne_ssl_clicert_read
    bool Retry;

    do
    {
      Retry = false;

      bool WrongPassphrase = false;
      ParseCertificate(GetSessionData()->GetTlsCertificateFile(), Passphrase, Certificate, PrivateKey, WrongPassphrase);
      if (WrongPassphrase)
      {
        if (Passphrase.IsEmpty())
        {
          LogEvent(L"Certificate is encrypted, need passphrase");
          Information(LoadStr(CLIENT_CERTIFICATE_LOADING), false);
        }
        else
        {
          Information(LoadStr(CERTIFICATE_DECODE_ERROR_INFO), false);
        }

        Passphrase = L"";
        if (PromptUser(
              GetSessionData(), pkPassphrase,
              LoadStr(CERTIFICATE_PASSPHRASE_TITLE), L"",
              LoadStr(CERTIFICATE_PASSPHRASE_PROMPT), false, 0, Passphrase))
        {
          Retry = true;
        }
        else
        {
          Result = false;
        }
      }
    }
    while (Retry);
  }
  return Result;
}

UnicodeString TTerminal::GetBaseFileName(const UnicodeString & AFileName)
{
  UnicodeString FileName = AFileName;

  if (FSessionData->GetTrimVMSVersions())
  {
    intptr_t P = FileName.LastDelimiter(L";");
    if (P > 0)
    {
      FileName.SetLength(P - 1);
    }
  }
  return FileName;
}

UnicodeString TTerminal::ChangeFileName(const TCopyParamType * CopyParam,
  const UnicodeString & AFileName, TOperationSide Side, bool FirstLevel)
{
  UnicodeString FileName = GetBaseFileName(AFileName);
  FileName = CopyParam->ChangeFileName(FileName, Side, FirstLevel);
  return FileName;
}

bool TTerminal::CanRecurseToDirectory(const TRemoteFile * AFile) const
{
  return !AFile->GetIsSymLink() || FSessionData->GetFollowDirectorySymlinks();
}

TSecondaryTerminal::TSecondaryTerminal(TTerminal * MainTerminal) :
  TTerminal(),
  FMainTerminal(MainTerminal)
{
}

void TSecondaryTerminal::Init(
  TSessionData * ASessionData, TConfiguration * AConfiguration, const UnicodeString & Name)
{
  TTerminal::Init(ASessionData, AConfiguration);
  DebugAssert(FMainTerminal != nullptr);
  GetLog()->SetParent(FMainTerminal->GetLog());
  GetLog()->SetName(Name);
  GetActionLog()->SetEnabled(false);
  GetSessionData()->NonPersistant();
  if (!FMainTerminal->TerminalGetUserName().IsEmpty())
  {
    GetSessionData()->SetUserName(FMainTerminal->TerminalGetUserName());
  }
}

void TSecondaryTerminal::UpdateFromMain()
{
  if ((FFileSystem != nullptr) && (FMainTerminal->FFileSystem != nullptr))
  {
    FFileSystem->UpdateFromMain(FMainTerminal->FFileSystem);
  }
}

void TSecondaryTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  FMainTerminal->DirectoryLoaded(FileList);
  DebugAssert(FileList != nullptr);
}

void TSecondaryTerminal::DirectoryModified(const UnicodeString & APath,
  bool SubDirs)
{
  // clear cache of main terminal
  FMainTerminal->DirectoryModified(APath, SubDirs);
}

TTerminal * TSecondaryTerminal::GetPasswordSource()
{
  return FMainTerminal;
}

TTerminalList::TTerminalList(TConfiguration * AConfiguration) :
  TObjectList(),
  FConfiguration(AConfiguration)
{
  DebugAssert(FConfiguration);
}

TTerminalList::~TTerminalList()
{
  DebugAssert(GetCount() == 0);
}

TTerminal * TTerminalList::CreateTerminal(TSessionData * Data)
{
  TTerminal * Result = new TTerminal();
  Result->Init(Data, FConfiguration);
  return Result;
}

TTerminal * TTerminalList::NewTerminal(TSessionData * Data)
{
  TTerminal * Result = CreateTerminal(Data);
  Add(Result);
  return Result;
}

void TTerminalList::FreeTerminal(TTerminal * Terminal)
{
  DebugAssert(IndexOf(Terminal) >= 0);
  Remove(Terminal);
}

void TTerminalList::FreeAndNullTerminal(TTerminal *& Terminal)
{
  TTerminal * T = Terminal;
  Terminal = nullptr;
  FreeTerminal(T);
}

TTerminal * TTerminalList::GetTerminal(intptr_t Index)
{
  return NB_STATIC_DOWNCAST(TTerminal, GetObj(Index));
}

void TTerminalList::Idle()
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TTerminal * Terminal = GetTerminal(Index);
    if (Terminal->GetStatus() == ssOpened)
    {
      Terminal->Idle();
    }
  }
}

void TTerminalList::RecryptPasswords()
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    GetTerminal(Index)->RecryptPasswords();
  }
}

UnicodeString GetSessionUrl(const TTerminal * Terminal, bool WithUserName)
{
  UnicodeString Result;
  const TSessionInfo & SessionInfo = Terminal->GetSessionInfo();
  const TSessionData * SessionData = Terminal->GetSessionData();
  UnicodeString Protocol = SessionInfo.ProtocolBaseName;
  UnicodeString HostName = SessionData->GetHostNameExpanded();
  UnicodeString UserName = SessionData->GetUserNameExpanded();
  intptr_t Port = Terminal->GetSessionData()->GetPortNumber();
  if (WithUserName && !UserName.IsEmpty())
  {
    Result = FORMAT(L"%s://%s:@%s:%d", Protocol.Lower().c_str(), UserName.c_str(), HostName.c_str(), Port);
  }
  else
  {
    Result = FORMAT(L"%s://%s:%d", Protocol.Lower().c_str(), HostName.c_str(), Port);
  }
  return Result;
}

NB_IMPLEMENT_CLASS(TTerminal, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TChecklistItem, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TSynchronizeData, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TCalculateSizeParams, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TMakeLocalFileListParams, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TFilesFindParams, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TCustomCommandParams, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TMoveFileParams, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TSynchronizeFileData, NB_GET_CLASS_INFO(TObject), nullptr)

