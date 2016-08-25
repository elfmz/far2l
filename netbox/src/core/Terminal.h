#pragma once

#include <Classes.hpp>
#include <Common.h>
#include <Exceptions.h>

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"

class TCopyParamType;
class TFileOperationProgressType;
class TRemoteDirectory;
class TRemoteFile;
class TCustomFileSystem;
class TTunnelThread;
class TSecureShell;
struct TCalculateSizeParams;
struct TOverwriteFileParams;
struct TSynchronizeData;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
struct TCalculateSizeStats;
struct TFileSystemInfo;
struct TSpaceAvailable;
struct TFilesFindParams;
class TTunnelUI;
class TCallbackGuard;

DEFINE_CALLBACK_TYPE8(TQueryUserEvent, void,
  TObject * /*Sender*/, const UnicodeString & /*Query*/, TStrings * /*MoreMessages*/ ,
  uintptr_t /*Answers*/,
  const TQueryParams * /*Params*/, uintptr_t & /*Answer*/,
  TQueryType /*QueryType*/, void * /*Arg*/);
DEFINE_CALLBACK_TYPE8(TPromptUserEvent, void,
  TTerminal * /*Terminal*/, TPromptKind /*Kind*/, const UnicodeString & /*Name*/,
  const UnicodeString & /*Instructions*/,
  TStrings * /*Prompts*/, TStrings * /*Results*/, bool & /*Result*/, void * /*Arg*/);
DEFINE_CALLBACK_TYPE5(TDisplayBannerEvent, void,
  TTerminal * /*Terminal*/, const UnicodeString & /*SessionName*/,
  const UnicodeString & /*Banner*/,
  bool & /*NeverShowAgain*/, intptr_t /*Options*/);
DEFINE_CALLBACK_TYPE3(TExtendedExceptionEvent, void,
  TTerminal * /*Terminal*/, Exception * /*E*/, void * /*Arg*/);
DEFINE_CALLBACK_TYPE2(TReadDirectoryEvent, void, TObject * /*Sender*/,
  Boolean /*ReloadOnly*/);
DEFINE_CALLBACK_TYPE4(TReadDirectoryProgressEvent, void,
  TObject * /*Sender*/, intptr_t /*Progress*/, intptr_t /*ResolvedLinks*/,
  bool & /*Cancel*/);
DEFINE_CALLBACK_TYPE3(TProcessFileEvent, void,
  const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/);
DEFINE_CALLBACK_TYPE4(TProcessFileEventEx, void,
  const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/, intptr_t /*Index*/);
DEFINE_CALLBACK_TYPE2(TFileOperationEvent, intptr_t,
  void * /*Param1*/, void * /*Param2*/);
DEFINE_CALLBACK_TYPE4(TSynchronizeDirectoryEvent, void,
  const UnicodeString & /*LocalDirectory*/, const UnicodeString & /*RemoteDirectory*/,
  bool & /*Continue*/, bool /*Collect*/);
DEFINE_CALLBACK_TYPE2(TDeleteLocalFileEvent, void,
  const UnicodeString & /*FileName*/, bool /*Alternative*/);
DEFINE_CALLBACK_TYPE3(TDirectoryModifiedEvent, int,
  TTerminal * /*Terminal*/, const UnicodeString & /*Directory*/, bool /*SubDirs*/);
DEFINE_CALLBACK_TYPE4(TInformationEvent, void,
  TTerminal * /*Terminal*/, const UnicodeString & /*Str*/, bool /*Status*/,
  intptr_t /*Phase*/);
DEFINE_CALLBACK_TYPE5(TCreateLocalFileEvent, HANDLE,
  const UnicodeString & /*FileName*/, DWORD /*DesiredAccess*/,
  DWORD /*ShareMode*/, DWORD /*CreationDisposition*/, DWORD /*FlagsAndAttributes*/);
DEFINE_CALLBACK_TYPE1(TGetLocalFileAttributesEvent, DWORD,
  const UnicodeString & /*FileName*/);
DEFINE_CALLBACK_TYPE2(TSetLocalFileAttributesEvent, BOOL,
  const UnicodeString & /*FileName*/, DWORD /*FileAttributes*/);
DEFINE_CALLBACK_TYPE3(TMoveLocalFileEvent, BOOL,
  const UnicodeString & /*FileName*/, const UnicodeString & /*NewFileName*/,
  DWORD /*Flags*/);
DEFINE_CALLBACK_TYPE1(TRemoveLocalDirectoryEvent, BOOL,
  const UnicodeString & /*LocalDirName*/);
DEFINE_CALLBACK_TYPE2(TCreateLocalDirectoryEvent, BOOL,
  const UnicodeString & /*LocalDirName*/, LPSECURITY_ATTRIBUTES /*SecurityAttributes*/);
DEFINE_CALLBACK_TYPE0(TCheckForEscEvent, bool);

inline void ThrowSkipFile(Exception * Exception, const UnicodeString & Message)
{
  throw ESkipFile(Exception, Message);
}
inline void ThrowSkipFileNull() { ThrowSkipFile(nullptr, L""); }

void FileOperationLoopCustom(TTerminal * Terminal,
  TFileOperationProgressType * OperationProgress,
  bool AllowSkip, const UnicodeString & Message,
  const UnicodeString & HelpKeyword,
  const std::function<void()> & Operation);

enum TCurrentFSProtocol
{
  cfsUnknown,
  cfsSCP,
  cfsSFTP,
  cfsFTP,
  cfsFTPS,
  cfsWebDAV
};

const int cpDelete = 0x01;
const int cpTemporary = 0x04;
const int cpNoConfirmation = 0x08;
const int cpNewerOnly = 0x10;
const int cpAppend = 0x20;
const int cpResume = 0x40;

const int ccApplyToDirectories = 0x01;
const int ccRecursive = 0x02;
const int ccUser = 0x100;

const int csIgnoreErrors = 0x01;

const int ropNoReadDirectory = 0x02;

const int boDisableNeverShowAgain = 0x01;

class TTerminal : public TObject, public TSessionUI
{
NB_DISABLE_COPY(TTerminal)
NB_DECLARE_CLASS(TTerminal)
public:
  // TScript::SynchronizeProc relies on the order
  enum TSynchronizeMode
  {
    smRemote,
    smLocal,
    smBoth,
  };

  static const int spDelete = 0x01; // cannot be combined with spTimestamp
  static const int spNoConfirmation = 0x02; // has no effect for spTimestamp
  static const int spExistingOnly = 0x04; // is implicit for spTimestamp
  static const int spNoRecurse = 0x08;
  static const int spUseCache = 0x10; // cannot be combined with spTimestamp
  static const int spDelayProgress = 0x20; // cannot be combined with spTimestamp
  static const int spPreviewChanges = 0x40; // not used by core
  static const int spSubDirs = 0x80; // cannot be combined with spTimestamp
  static const int spTimestamp = 0x100;
  static const int spNotByTime = 0x200; // cannot be combined with spTimestamp and smBoth
  static const int spBySize = 0x400; // cannot be combined with smBoth, has opposite meaning for spTimestamp
  static const int spSelectedOnly = 0x800; // not used by core
  static const int spMirror = 0x1000;
  static const int spDefault = TTerminal::spNoConfirmation | TTerminal::spPreviewChanges;

// for TranslateLockedPath()
friend class TRemoteFile;
// for ReactOnCommand()
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
friend class TWebDAVFileSystem;
friend class TTunnelUI;
friend class TCallbackGuard;
friend class TSecondaryTerminal;
friend class TRetryOperationLoop;

private:
  TSessionData * FSessionData;
  TSessionLog * FLog;
  TActionLog * FActionLog;
  TConfiguration * FConfiguration;
  UnicodeString FCurrentDirectory;
  UnicodeString FLockDirectory;
  Integer FExceptionOnFail;
  TRemoteDirectory * FFiles;
  intptr_t FInTransaction;
  bool FSuspendTransaction;
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TDeleteLocalFileEvent FOnDeleteLocalFile;
  TCreateLocalFileEvent FOnCreateLocalFile;
  TGetLocalFileAttributesEvent FOnGetLocalFileAttributes;
  TSetLocalFileAttributesEvent FOnSetLocalFileAttributes;
  TMoveLocalFileEvent FOnMoveLocalFile;
  TRemoveLocalDirectoryEvent FOnRemoveLocalDirectory;
  TCreateLocalDirectoryEvent FOnCreateLocalDirectory;
  TNotifyEvent FOnInitializeLog;
  TRemoteTokenList FMembership;
  TRemoteTokenList FGroups;
  TRemoteTokenList FUsers;
  bool FUsersGroupsLookedup;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinishedEvent FOnFinished;
  TFileOperationProgressType * FOperationProgress;
  bool FUseBusyCursor;
  TRemoteDirectoryCache * FDirectoryCache;
  TRemoteDirectoryChangesCache * FDirectoryChangesCache;
  TSecureShell * FSecureShell;
  UnicodeString FLastDirectoryChange;
  TCurrentFSProtocol FFSProtocol;
  TTerminal * FCommandSession;
  bool FAutoReadDirectory;
  bool FReadingCurrentDirectory;
  bool * FClosedOnCompletion;
  TSessionStatus FStatus;
  int FOpening;
  RawByteString FRememberedPassword;
  RawByteString FRememberedTunnelPassword;
  TTunnelThread * FTunnelThread;
  TSecureShell * FTunnel;
  TSessionData * FTunnelData;
  TSessionLog * FTunnelLog;
  TTunnelUI * FTunnelUI;
  intptr_t FTunnelLocalPortNumber;
  UnicodeString FTunnelError;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TDisplayBannerEvent FOnDisplayBanner;
  TExtendedExceptionEvent FOnShowExtendedException;
  TInformationEvent FOnInformation;
  TNotifyEvent FOnClose;
  TCheckForEscEvent FOnCheckForEsc;
  TCallbackGuard * FCallbackGuard;
  TFindingFileEvent FOnFindingFile;
  bool FEnableSecureShellUsage;
  bool FCollectFileSystemUsage;
  bool FRememberedPasswordTried;
  bool FRememberedTunnelPasswordTried;
  int FNesting;
  UnicodeString FFingerprintScanned;
  TRemoteDirectory * FOldFiles;

public:
  void CommandError(Exception * E, const UnicodeString & Msg);
  uintptr_t CommandError(Exception * E, const UnicodeString & Msg,
    uintptr_t Answers, const UnicodeString & HelpKeyword = L"");
  UnicodeString GetCurrDirectory();
  bool GetExceptionOnFail() const;
  const TRemoteTokenList * GetGroups() const { return const_cast<TTerminal *>(this)->GetGroups(); }
  TRemoteTokenList * GetGroups();
  const TRemoteTokenList * GetUsers() const { return const_cast<TTerminal *>(this)->GetUsers(); }
  TRemoteTokenList * GetUsers();
  const TRemoteTokenList * GetMembership() const { return const_cast<TTerminal *>(this)->GetMembership(); }
  TRemoteTokenList * GetMembership();
  void TerminalSetCurrentDirectory(const UnicodeString & AValue);
  void SetExceptionOnFail(bool Value);
  void ReactOnCommand(intptr_t /*TFSCommand*/ Cmd);
  UnicodeString TerminalGetUserName() const;
  bool GetAreCachesEmpty() const;
  void ClearCachedFileList(const UnicodeString & APath, bool SubDirs);
  void AddCachedFileList(TRemoteFileList * FileList);
  bool GetCommandSessionOpened() const;
  TTerminal * GetCommandSession();
  bool GetResolvingSymlinks() const;
  bool GetActive() const;
  UnicodeString GetPassword() const;
  UnicodeString GetRememberedPassword() const;
  UnicodeString GetRememberedTunnelPassword() const;
  bool GetStoredCredentialsTried() const;
  inline bool InTransaction() const;
  void SaveCapabilities(TFileSystemInfo & FileSystemInfo);
  static UnicodeString SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString SynchronizeParamsStr(intptr_t Params);

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;
  TCustomFileSystem * FFileSystem;

public:
  void DoStartReadDirectory();
  void DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool & Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(const UnicodeString & ADirName);
  void DoDeleteFile(const UnicodeString & AFileName, const TRemoteFile * AFile,
    intptr_t Params);
  void DoCustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent);
  void DoRenameFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName, bool Move);
  void DoCopyFile(const UnicodeString & AFileName, const UnicodeString & ANewName);
  void DoChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties);
  void DoChangeDirectory();
  void DoInitializeLog();
  void EnsureNonExistence(const UnicodeString & AFileName, bool IsDirectory);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile * AFile,
    const UnicodeString & AFileName, bool ClearDirectoryChange = false);
  intptr_t FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, bool AllowSkip,
    const UnicodeString & Message, void * Param1 = nullptr, void * Param2 = nullptr);
  bool GetIsCapable(TFSCapability Capability) const;
  bool ProcessFiles(const TStrings * AFileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = nullptr, TOperationSide Side = osRemote,
    bool Ex = false);
  bool ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * Param = nullptr, TOperationSide Side = osRemote);
  void ProcessDirectory(const UnicodeString & ADirName,
    TProcessFileEvent CallBackFunc, void * Param = nullptr, bool UseCache = false,
    bool IgnoreErrors = false);
  void AnnounceFileListOperation();
  UnicodeString TranslateLockedPath(const UnicodeString & APath, bool Lock);
  void ReadDirectory(TRemoteFileList * AFileList);
  void CustomReadDirectory(TRemoteFileList * AFileList);
  void DoCreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic);
  bool TerminalCreateLocalFile(const UnicodeString & AFileName,
    TFileOperationProgressType * OperationProgress,
    bool Resume,
    bool NoConfirmation,
    OUT HANDLE * AHandle);
  HANDLE TerminalCreateLocalFile(const UnicodeString & LocalFileName, DWORD DesiredAccess,
    DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  void TerminalOpenLocalFile(const UnicodeString & AFileName, DWORD Access,
    OUT OPTIONAL HANDLE * AHandle, OUT OPTIONAL uintptr_t * AAttrs, OUT OPTIONAL int64_t * ACTime, OUT OPTIONAL int64_t * AMTime,
    OUT OPTIONAL int64_t * AATime, OUT OPTIONAL int64_t * ASize, bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(const UnicodeString & AFileName,
    const TCopyParamType * CopyParam, TFileOperationProgressType * OperationProgress);
  bool HandleException(Exception * E);
  void CalculateFileSize(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TCalculateSizeParams*/ void * Size);
  void DoCalculateDirectorySize(const UnicodeString & AFileName,
    const TRemoteFile * AFile, TCalculateSizeParams * Params);
  void CalculateLocalFileSize(const UnicodeString & AFileName,
    const TSearchRec & Rec, /*int64_t*/ void * Params);
  bool CalculateLocalFilesSize(const TStrings * AFileList,
    const TCopyParamType * CopyParam, bool AllowDirs,
    OUT int64_t & Size);
  TBatchOverwrite EffectiveBatchOverwrite(
    const UnicodeString & AFileName, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, bool Special);
  bool CheckRemoteFile(
    const UnicodeString & AFileName, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress);
  uintptr_t ConfirmFileOverwrite(
    const UnicodeString & ASourceFullFileName, const UnicodeString & ATargetFileName,
    const TOverwriteFileParams * FileParams, uintptr_t Answers, TQueryParams * QueryParams,
    TOperationSide Side, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, const UnicodeString & AMessage = L"");
  void DoSynchronizeCollectDirectory(const UnicodeString & ALocalDirectory,
    const UnicodeString & ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions * Options, intptr_t Level, TSynchronizeChecklist * Checklist);
  void DoSynchronizeCollectFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param);
  void SynchronizeCollectFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param);
  void SynchronizeRemoteTimestamp(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Param);
  void SynchronizeLocalTimestamp(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Param);
  void DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void DeleteLocalFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Param);
  void RecycleFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  TStrings * GetFixedPaths();
  void DoStartup();
  virtual bool DoQueryReopen(Exception * E);
  virtual void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void ResetConnection();
  virtual bool DoPromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Response);
  void OpenTunnel();
  void CloseTunnel();
  void DoInformation(const UnicodeString & Str, bool Status, intptr_t Phase = -1);
  bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & Instructions, const UnicodeString & Prompt, bool Echo,
    intptr_t MaxLen, OUT UnicodeString & AResult);
  void FileFind(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param);
  void DoFilesFind(const UnicodeString & Directory, TFilesFindParams & Params, const UnicodeString & RealDirectory);
  bool DoCreateLocalFile(const UnicodeString & AFileName,
    TFileOperationProgressType * OperationProgress,
    bool Resume,
    bool NoConfirmation,
    OUT HANDLE * AHandle);
  void LockFile(const UnicodeString & AFileName, const TRemoteFile * AFile, void * AParam);
  void UnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile, void * AParam);
  void DoLockFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  void DoUnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile);

  virtual void Information(const UnicodeString & Str, bool Status);
  virtual uintptr_t QueryUser(const UnicodeString & Query,
    TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual uintptr_t QueryUserException(const UnicodeString & Query,
    Exception * E, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & Instructions, TStrings * Prompts, TStrings * Results);
  virtual void DisplayBanner(const UnicodeString & Banner);
  virtual void Closed();
  virtual void ProcessGUI();
  void Progress(TFileOperationProgressType * OperationProgress);
  virtual void HandleExtendedException(Exception * E);
  bool IsListenerFree(uintptr_t PortNumber) const;
  void DoProgress(TFileOperationProgressType & ProgressData);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & AFileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = nullptr);
  void DoAnyCommand(const UnicodeString & ACommand, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * DoReadDirectoryListing(const UnicodeString & ADirectory, bool UseCache);
  RawByteString EncryptPassword(const UnicodeString & APassword) const;
  UnicodeString DecryptPassword(const RawByteString & APassword) const;
  UnicodeString GetRemoteFileInfo(TRemoteFile * AFile);
  void LogRemoteFile(TRemoteFile * AFile);
  UnicodeString FormatFileDetailsForLog(const UnicodeString & AFileName, const TDateTime & AModification, int64_t Size);
  void LogFileDetails(const UnicodeString & AFileName, const TDateTime & Modification, int64_t Size);
  void LogFileDone(TFileOperationProgressType * OperationProgress);
  virtual const TTerminal * GetPasswordSource() const { return this; }
  virtual TTerminal * GetPasswordSource();
  void DoEndTransaction(bool Inform);
  bool VerifyCertificate(
    const UnicodeString & CertificateStorageKey, const UnicodeString & SiteKey,
    const UnicodeString & Fingerprint,
    const UnicodeString & CertificateSubject, int Failures);
  void CacheCertificate(const UnicodeString & CertificateStorageKey,
    const UnicodeString & SiteKey, const UnicodeString & Fingerprint, int Failures);
  void CollectTlsUsage(const UnicodeString & TlsVersionStr);
  bool LoadTlsCertificate(X509 *& Certificate, EVP_PKEY *& PrivateKey);
  bool TryStartOperationWithFile(
    const UnicodeString & AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  void StartOperationWithFile(
    const UnicodeString & AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  void CommandSessionClose(TObject * Sender);
  bool CanRecurseToDirectory(const TRemoteFile * AFile) const;

  // __property TFileOperationProgressType * OperationProgress = { read=FOperationProgress };
  const TFileOperationProgressType * GetOperationProgress() const { return FOperationProgress; }
  TFileOperationProgressType * GetOperationProgress() { return FOperationProgress; }
  void SetOperationProgress(TFileOperationProgressType * OperationProgress) { FOperationProgress = OperationProgress; }


public:
  explicit TTerminal();
  void Init(TSessionData * SessionData, TConfiguration * Configuration);
  virtual ~TTerminal();
  void Open();
  void Close();
  UnicodeString FingerprintScan();
  void Reopen(intptr_t Params);
  virtual void DirectoryModified(const UnicodeString & APath, bool SubDirs);
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  void ShowExtendedException(Exception * E);
  void Idle();
  void RecryptPasswords();
  bool AllowedAnyCommand(const UnicodeString & Command) const;
  void AnyCommand(const UnicodeString & Command, TCaptureOutputEvent OutputEvent);
  void CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const UnicodeString & Message = L"");
  UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const;
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * ReadDirectoryListing(const UnicodeString & Directory, const TFileMasks & Mask);
  TRemoteFileList * CustomReadDirectoryListing(const UnicodeString & Directory, bool UseCache);
  TRemoteFile * ReadFileListing(const UnicodeString & APath);
  void ReadFile(const UnicodeString & AFileName, TRemoteFile *& AFile);
  bool FileExists(const UnicodeString & AFileName, TRemoteFile ** AFile = nullptr);
  void ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool CopyToLocal(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params);
  bool CopyToRemote(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params);
  void RemoteCreateDirectory(const UnicodeString & ADirName,
    const TRemoteProperties * Properties = nullptr);
  void CreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic, bool IsDirectory);
  void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile = nullptr, void * Params = nullptr);
  bool RemoteDeleteFiles(TStrings * AFilesToDelete, intptr_t Params = 0);
  bool DeleteLocalFiles(TStrings * AFileList, intptr_t Params = 0);
  bool IsRecycledFile(const UnicodeString & AFileName);
  void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * AParams);
  void CustomCommandOnFiles(const UnicodeString & Command, intptr_t Params,
    TStrings * AFiles, TCaptureOutputEvent OutputEvent);
  void RemoteChangeDirectory(const UnicodeString & Directory);
  void EndTransaction();
  void HomeDirectory();
  void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*const TRemoteProperties*/ void * Properties);
  void ChangeFilesProperties(TStrings * AFileList,
    const TRemoteProperties * Properties);
  bool LoadFilesProperties(TStrings * AFileList);
  void TerminalError(const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void TerminalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void ReloadDirectory();
  void RefreshDirectory();
  void TerminalRenameFile(const UnicodeString & AFileName, const UnicodeString & ANewName);
  void TerminalRenameFile(const TRemoteFile * AFile, const UnicodeString & ANewName, bool CheckExistence);
  void TerminalMoveFile(const UnicodeString & AFileName, const TRemoteFile * AFile,
    /*const TMoveFileParams*/ void * Param);
  bool MoveFiles(TStrings * AFileList, const UnicodeString & Target,
    const UnicodeString & FileMask);
  void TerminalCopyFile(const UnicodeString & AFileName, const TRemoteFile * AFile,
    /*const TMoveFileParams*/ void * Param);
  bool CopyFiles(const TStrings * AFileList, const UnicodeString & Target,
    const UnicodeString & FileMask);
  bool CalculateFilesSize(const TStrings * AFileList, int64_t & Size,
    intptr_t Params, const TCopyParamType * CopyParam, bool AllowDirs,
    TCalculateSizeStats * Stats = nullptr);
  void CalculateFilesChecksum(const UnicodeString & Alg, TStrings * AFileList,
    TStrings * Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void SynchronizeApply(TSynchronizeChecklist * Checklist,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    const TCopyParamType * CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory);
  void FilesFind(const UnicodeString & Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void SpaceAvailable(const UnicodeString & APath, TSpaceAvailable & ASpaceAvailable);
  void LockFiles(TStrings * AFileList);
  void UnlockFiles(TStrings * AFileList);
  bool DirectoryFileList(const UnicodeString & APath,
    TRemoteFileList *& FileList, bool CanLoad);
  void MakeLocalFileList(const UnicodeString & AFileName,
    const TSearchRec & Rec, void * Param);
  bool FileOperationLoopQuery(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString & Message,
    bool AllowSkip, const UnicodeString & SpecialRetry = UnicodeString(), const UnicodeString & HelpKeyword = UnicodeString());
  TUsableCopyParamAttrs UsableCopyParamAttrs(intptr_t Params);
  bool QueryReopen(Exception * E, intptr_t Params,
    TFileOperationProgressType * OperationProgress);
  UnicodeString PeekCurrentDirectory();
  void FatalAbort();
  void ReflectSettings();
  void CollectUsage();

  const TSessionInfo & GetSessionInfo() const;
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve = false);
  void inline LogEvent(const UnicodeString & Str);
  void GetSupportedChecksumAlgs(TStrings * Algs);
  UnicodeString ChangeFileName(const TCopyParamType * CopyParam,
    const UnicodeString & AFileName, TOperationSide Side, bool FirstLevel);
  UnicodeString GetBaseFileName(const UnicodeString & AFileName);

  static UnicodeString ExpandFileName(const UnicodeString & APath,
    const UnicodeString & BasePath);

#if 0
  __property TSessionData * SessionData = { read = FSessionData };
  __property TSessionLog * Log = { read = FLog };
  __property TActionLog * ActionLog = { read = FActionLog };
  __property TConfiguration * Configuration = { read = FConfiguration };
  __property bool Active = { read = GetActive };
  __property TSessionStatus Status = { read = FStatus };
  __property UnicodeString CurrentDirectory = { read = GetCurrentDirectory, write = SetCurrentDirectory };
  __property bool ExceptionOnFail = { read = GetExceptionOnFail, write = SetExceptionOnFail };
  __property TRemoteDirectory * Files = { read = FFiles };
  __property TNotifyEvent OnChangeDirectory = { read = FOnChangeDirectory, write = FOnChangeDirectory };
  __property TReadDirectoryEvent OnReadDirectory = { read = FOnReadDirectory, write = FOnReadDirectory };
  __property TNotifyEvent OnStartReadDirectory = { read = FOnStartReadDirectory, write = FOnStartReadDirectory };
  __property TReadDirectoryProgressEvent OnReadDirectoryProgress = { read = FOnReadDirectoryProgress, write = FOnReadDirectoryProgress };
  __property TDeleteLocalFileEvent OnDeleteLocalFile = { read = FOnDeleteLocalFile, write = FOnDeleteLocalFile };
  __property TNotifyEvent OnInitializeLog = { read = FOnInitializeLog, write = FOnInitializeLog };
  __property const TRemoteTokenList * Groups = { read = GetGroups };
  __property const TRemoteTokenList * Users = { read = GetUsers };
  __property const TRemoteTokenList * Membership = { read = GetMembership };
  __property TFileOperationProgressEvent OnProgress  = { read=FOnProgress, write=FOnProgress };
  __property TFileOperationFinished OnFinished  = { read=FOnFinished, write=FOnFinished };
  __property TCurrentFSProtocol FSProtocol = { read = FFSProtocol };
  __property bool UseBusyCursor = { read = FUseBusyCursor, write = FUseBusyCursor };
  __property UnicodeString UserName = { read=GetUserName };
  __property bool IsCapable[TFSCapability Capability] = { read = GetIsCapable };
  __property bool AreCachesEmpty = { read = GetAreCachesEmpty };
  __property bool CommandSessionOpened = { read = GetCommandSessionOpened };
  __property TTerminal * CommandSession = { read = GetCommandSession };
  __property bool AutoReadDirectory = { read = FAutoReadDirectory, write = FAutoReadDirectory };
  __property TStrings * FixedPaths = { read = GetFixedPaths };
  __property bool ResolvingSymlinks = { read = GetResolvingSymlinks };
  __property UnicodeString Password = { read = GetPassword };
  __property UnicodeString RememberedPassword = { read = GetRememberedPassword };
  __property UnicodeString RememberedTunnelPassword = { read = GetRememberedTunnelPassword };
  __property bool StoredCredentialsTried = { read = GetStoredCredentialsTried };
  __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  __property TDisplayBannerEvent OnDisplayBanner = { read = FOnDisplayBanner, write = FOnDisplayBanner };
  __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  __property TInformationEvent OnInformation = { read = FOnInformation, write = FOnInformation };
  __property TNotifyEvent OnClose = { read = FOnClose, write = FOnClose };
  __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber };
#endif

  void SetMasks(const UnicodeString & Value);

  void SetLocalFileTime(const UnicodeString & LocalFileName,
    const TDateTime & Modification);
  void SetLocalFileTime(const UnicodeString & LocalFileName,
    FILETIME * AcTime, FILETIME * WrTime);
  DWORD GetLocalFileAttributes(const UnicodeString & LocalFileName);
  BOOL SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  BOOL MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  BOOL RemoveLocalDirectory(const UnicodeString & LocalDirName);
  BOOL CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

  TSessionData * GetSessionData() const { return FSessionData; }
  TSessionData * GetSessionData() { return FSessionData; }
  TSessionLog * GetLog() const { return FLog; }
  TSessionLog * GetLog() { return FLog; }
  TActionLog * GetActionLog() const { return FActionLog; }
  const TConfiguration * GetConfiguration() const { return FConfiguration; }
  TConfiguration * GetConfiguration() { return FConfiguration; }
  TSessionStatus GetStatus() const { return FStatus; }
  TRemoteDirectory * GetFiles() const { return FFiles; }
  TNotifyEvent & GetOnChangeDirectory() { return FOnChangeDirectory; }
  void SetOnChangeDirectory(TNotifyEvent Value) { FOnChangeDirectory = Value; }
  TReadDirectoryEvent & GetOnReadDirectory() { return FOnReadDirectory; }
  void SetOnReadDirectory(TReadDirectoryEvent Value) { FOnReadDirectory = Value; }
  TNotifyEvent & GetOnStartReadDirectory() { return FOnStartReadDirectory; }
  void SetOnStartReadDirectory(TNotifyEvent Value) { FOnStartReadDirectory = Value; }
  TReadDirectoryProgressEvent & GetOnReadDirectoryProgress() { return FOnReadDirectoryProgress; }
  void SetOnReadDirectoryProgress(TReadDirectoryProgressEvent Value) { FOnReadDirectoryProgress = Value; }
  TDeleteLocalFileEvent & GetOnDeleteLocalFile() { return FOnDeleteLocalFile; }
  void SetOnDeleteLocalFile(TDeleteLocalFileEvent Value) { FOnDeleteLocalFile = Value; }
  TNotifyEvent & GetOnInitializeLog() { return FOnInitializeLog; }
  void SetOnInitializeLog(TNotifyEvent Value) { FOnInitializeLog = Value; }
  TCreateLocalFileEvent & GetOnCreateLocalFile() { return FOnCreateLocalFile; }
  void SetOnCreateLocalFile(TCreateLocalFileEvent Value) { FOnCreateLocalFile = Value; }
  TGetLocalFileAttributesEvent & GetOnGetLocalFileAttributes() { return FOnGetLocalFileAttributes; }
  void SetOnGetLocalFileAttributes(TGetLocalFileAttributesEvent Value) { FOnGetLocalFileAttributes = Value; }
  TSetLocalFileAttributesEvent & GetOnSetLocalFileAttributes() { return FOnSetLocalFileAttributes; }
  void SetOnSetLocalFileAttributes(TSetLocalFileAttributesEvent Value) { FOnSetLocalFileAttributes = Value; }
  TMoveLocalFileEvent & GetOnMoveLocalFile() { return FOnMoveLocalFile; }
  void SetOnMoveLocalFile(TMoveLocalFileEvent Value) { FOnMoveLocalFile = Value; }
  TRemoveLocalDirectoryEvent & GetOnRemoveLocalDirectory() { return FOnRemoveLocalDirectory; }
  void SetOnRemoveLocalDirectory(TRemoveLocalDirectoryEvent Value) { FOnRemoveLocalDirectory = Value; }
  TCreateLocalDirectoryEvent & GetOnCreateLocalDirectory() { return FOnCreateLocalDirectory; }
  void SetOnCreateLocalDirectory(TCreateLocalDirectoryEvent Value) { FOnCreateLocalDirectory = Value; }
  TFileOperationProgressEvent & GetOnProgress() { return FOnProgress; }
  void SetOnProgress(TFileOperationProgressEvent Value) { FOnProgress = Value; }
  TFileOperationFinishedEvent & GetOnFinished() { return FOnFinished; }
  void SetOnFinished(TFileOperationFinishedEvent Value) { FOnFinished = Value; }
  TCurrentFSProtocol GetFSProtocol() const { return FFSProtocol; }
  bool GetUseBusyCursor() const { return FUseBusyCursor; }
  void SetUseBusyCursor(bool Value) { FUseBusyCursor = Value; }
  bool GetAutoReadDirectory() const { return FAutoReadDirectory; }
  void SetAutoReadDirectory(bool Value) { FAutoReadDirectory = Value; }
  TQueryUserEvent & GetOnQueryUser() { return FOnQueryUser; }
  void SetOnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; }
  TPromptUserEvent & GetOnPromptUser() { return FOnPromptUser; }
  void SetOnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; }
  TDisplayBannerEvent & GetOnDisplayBanner() { return FOnDisplayBanner; }
  void SetOnDisplayBanner(TDisplayBannerEvent Value) { FOnDisplayBanner = Value; }
  TExtendedExceptionEvent & GetOnShowExtendedException() { return FOnShowExtendedException; }
  void SetOnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; }
  TInformationEvent & GetOnInformation() { return FOnInformation; }
  void SetOnInformation(TInformationEvent Value) { FOnInformation = Value; }
  TCheckForEscEvent & GetOnCheckForEsc() { return FOnCheckForEsc; }
  void SetOnCheckForEsc(TCheckForEscEvent Value) { FOnCheckForEsc = Value; }
  TNotifyEvent & GetOnClose() { return FOnClose; }
  void SetOnClose(TNotifyEvent Value) { FOnClose = Value; }
  intptr_t GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  void SetRememberedPassword(const UnicodeString & Value) { FRememberedPassword = Value; }
  void SetRememberedTunnelPassword(const UnicodeString & Value) { FRememberedTunnelPassword = Value; }
  void SetTunnelPassword(const UnicodeString & Value) { FRememberedTunnelPassword = Value; }
  TCustomFileSystem * GetFileSystem() const { return FFileSystem; }
  TCustomFileSystem * GetFileSystem() { return FFileSystem; }

  bool CheckForEsc();
  void SetupTunnelLocalPortNumber();

private:
  void InternalTryOpen();
  void InternalDoTryOpen();
  void InitFileSystem();
};

class TSecondaryTerminal : public TTerminal
{
NB_DISABLE_COPY(TSecondaryTerminal)
public:
  explicit TSecondaryTerminal(TTerminal * MainTerminal);
  virtual ~TSecondaryTerminal() {}
  void Init(TSessionData * SessionData, TConfiguration * Configuration,
    const UnicodeString & Name);

  void UpdateFromMain();

  //__property TTerminal * MainTerminal = { read = FMainTerminal };
  TTerminal * GetMainTerminal() const { return FMainTerminal; }

protected:
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  virtual void DirectoryModified(const UnicodeString & APath,
    bool SubDirs);
  virtual const TTerminal * GetPasswordSource() const { return FMainTerminal; }
  virtual TTerminal * GetPasswordSource();

private:
  TTerminal * FMainTerminal;
};

class TTerminalList : public TObjectList
{
NB_DISABLE_COPY(TTerminalList)
public:
  explicit TTerminalList(TConfiguration * AConfiguration);
  virtual ~TTerminalList();

  virtual TTerminal * NewTerminal(TSessionData * Data);
  virtual void FreeTerminal(TTerminal * Terminal);
  void FreeAndNullTerminal(TTerminal *& Terminal);
  virtual void Idle();
  void RecryptPasswords();

  // __property TTerminal * Terminals[int Index]  = { read=GetTerminal };

protected:
  virtual TTerminal * CreateTerminal(TSessionData * Data);

private:
  TConfiguration * FConfiguration;

public:
  TTerminal * GetTerminal(intptr_t Index);
};

struct TCustomCommandParams : public TObject
{
NB_DECLARE_CLASS(TCustomCommandParams)
public:
  UnicodeString Command;
  intptr_t Params;
  TCaptureOutputEvent OutputEvent;
};

struct TCalculateSizeStats : public TObject
{
  TCalculateSizeStats();

  intptr_t Files;
  intptr_t Directories;
  intptr_t SymLinks;
};

struct TCalculateSizeParams : public TObject
{
NB_DECLARE_CLASS(TCalculateSizeParams)
public:
  int64_t Size;
  intptr_t Params;
  const TCopyParamType * CopyParam;
  TCalculateSizeStats * Stats;
  bool AllowDirs;
  bool Result;
};

typedef std::vector<TDateTime> TDateTimes;

struct TMakeLocalFileListParams : public TObject
{
NB_DECLARE_CLASS(TMakeLocalFileListParams)
public:
  TStrings * FileList;
  TDateTimes * FileTimes;
  bool IncludeDirs;
  bool Recursive;
};

struct TSynchronizeOptions : public TObject
{
NB_DISABLE_COPY(TSynchronizeOptions)
public:
  TSynchronizeOptions();
  ~TSynchronizeOptions();

  TStringList * Filter;

  bool FilterFind(const UnicodeString & AFileName);
  bool MatchesFilter(const UnicodeString & AFileName);
};

enum TChecklistAction
{
  saNone,
  saUploadNew,
  saDownloadNew,
  saUploadUpdate,
  saDownloadUpdate,
  saDeleteRemote,
  saDeleteLocal,
};

//---------------------------------------------------------------------------
class TChecklistItem : public TObject
{
friend class TTerminal;
NB_DECLARE_CLASS(TChecklistItem)
NB_DISABLE_COPY(TChecklistItem)
public:
  struct TFileInfo : public TObject
  {
    UnicodeString FileName;
    UnicodeString Directory;
    TDateTime Modification;
    TModificationFmt ModificationFmt;
    int64_t Size;
  };

  TChecklistAction Action;
  bool IsDirectory;
  TFileInfo Local;
  TFileInfo Remote;
  intptr_t ImageIndex;
  bool Checked;
  TRemoteFile * RemoteFile;

  const UnicodeString GetFileName() const;

  ~TChecklistItem();

private:
  FILETIME FLocalLastWriteTime;

  TChecklistItem();
};

class TSynchronizeChecklist : public TObject
{
friend class TTerminal;
NB_DISABLE_COPY(TSynchronizeChecklist)
public:
  static const intptr_t ActionCount = saDeleteLocal;

  ~TSynchronizeChecklist();

  void Update(const TChecklistItem * Item, bool Check, TChecklistAction Action);

  static TChecklistAction Reverse(TChecklistAction Action);

/*
  __property int Count = { read = GetCount };
  __property const TItem * Item[int Index] = { read = GetItem };
*/
protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TChecklistItem * Item);

public:
  void SetMasks(const UnicodeString & Value);

  intptr_t GetCount() const;
  const TChecklistItem * GetItem(intptr_t Index) const;

private:
  TList FList;

  static intptr_t Compare(const void * Item1, const void * Item2);
};

struct TSpaceAvailable : public TObject
{
  TSpaceAvailable();

  int64_t BytesOnDevice;
  int64_t UnusedBytesOnDevice;
  int64_t BytesAvailableToUser;
  int64_t UnusedBytesAvailableToUser;
  uintptr_t BytesPerAllocationUnit;
};

class TRobustOperationLoop : public TObject
{
NB_DISABLE_COPY(TRobustOperationLoop)
public:
  TRobustOperationLoop(TTerminal * Terminal, TFileOperationProgressType * OperationProgress);
  bool TryReopen(Exception & E);
  bool ShouldRetry() const;
  bool Retry();

private:
  TTerminal * FTerminal;
  TFileOperationProgressType * FOperationProgress;
  bool FRetry;
};

UnicodeString GetSessionUrl(const TTerminal * Terminal, bool WithUserName = false);

