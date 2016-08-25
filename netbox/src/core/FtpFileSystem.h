#pragma once

#ifndef NO_FILEZILLA

#include <time.h>
#include <map>
#include <vector>
#include <FileSystems.h>

class TFileZillaIntf;
class TFileZillaImpl;
class TFTPServerCapabilities;
struct TOverwriteFileParams;
struct TListDataEntry;
struct TFileTransferData;
struct TFtpsCertificateData;
struct TRemoteFileTime;

struct message_t
{
  message_t() : wparam(0), lparam(0)
  {}
  message_t(WPARAM w, LPARAM l) : wparam(w), lparam(l)
  {}
  WPARAM wparam;
  LPARAM lparam;
};

class TMessageQueue : public TObject, public std::vector<message_t>
{
public:
  typedef message_t value_type;
};

class TFTPFileSystem : public TCustomFileSystem
{
friend class TFileZillaImpl;
friend class TFTPFileListHelper;
NB_DISABLE_COPY(TFTPFileSystem)
public:
  explicit TFTPFileSystem(TTerminal * ATerminal);
  virtual ~TFTPFileSystem();

  virtual void Init(void *);

  virtual void Open();
  virtual void Close();
  virtual bool GetActive() const;
  virtual void CollectUsage();
  virtual void Idle();
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local);
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const;
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const UnicodeString & Directory);
  virtual void CachedChangeDirectory(const UnicodeString & Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * AFileList);
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * AFileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void RemoteCreateDirectory(const UnicodeString & ADirName);
  virtual void CreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic);
  virtual void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(intptr_t Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const UnicodeString & AFileName,
    TRemoteFile *& AFile);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& AFile);
  virtual void RemoteRenameFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName);
  virtual void RemoteCopyFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo() const;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString & AFileName);
  virtual bool GetStoredCredentialsTried() const;
  virtual UnicodeString FSGetUserName() const;
  virtual void GetSupportedChecksumAlgs(TStrings * Algs);
  virtual void LockFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  virtual void UnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  virtual void UpdateFromMain(TCustomFileSystem * MainFileSystem);

protected:
  // enum TOverwriteMode { omOverwrite, omResume, omComplete };

  virtual UnicodeString GetCurrDirectory() const;

  const wchar_t * GetOption(intptr_t OptionID) const;
  intptr_t GetOptionVal(intptr_t OptionID) const;

  enum
  {
    REPLY_CONNECT      = 0x01,
    REPLY_2XX_CODE     = 0x02,
    REPLY_ALLOW_CANCEL = 0x04,
    REPLY_3XX_CODE     = 0x08,
    REPLY_SINGLE_LINE  = 0x10,
  };

  bool FTPPostMessage(uintptr_t Type, WPARAM wParam, LPARAM lParam);
  bool ProcessMessage();
  void DiscardMessages();
  void WaitForMessages();
  uintptr_t WaitForReply(bool Command, bool WantLastCode);
  uintptr_t WaitForCommandReply(bool WantLastCode = true);
  void WaitForFatalNonCommandReply();
  void PoolForFatalNonCommandReply();
  void GotNonCommandReply(uintptr_t Reply);
  UnicodeString GotReply(uintptr_t Reply, uintptr_t Flags = 0,
    const UnicodeString & Error = L"", uintptr_t * Code = nullptr,
    TStrings ** Response = nullptr);
  void ResetReply();
  void HandleReplyStatus(const UnicodeString & Response);
  void DoWaitForReply(uintptr_t &ReplyToAwait, bool WantLastCode);
  bool KeepWaitingForReply(uintptr_t &ReplyToAwait, bool WantLastCode) const;
  inline bool NoFinalLastCode() const;

  bool HandleStatus(const wchar_t * Status, int Type);
  bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    int64_t Size1, int64_t Size2, time_t LocalTime,
    bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData,
    HANDLE & LocalFileHandle,
    int & RequestResult);
  bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult);
  bool HandleAsynchRequestNeedPass(
    struct TNeedPassRequestData & Data, int & RequestResult) const;
  bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    uintptr_t Count);
  bool HandleTransferStatus(bool Valid, int64_t TransferSize,
    int64_t Bytes, bool FileTransfer);
  bool HandleReply(intptr_t Command, uintptr_t Reply);
  bool HandleCapabilities(TFTPServerCapabilities * ServerCapabilities);
  bool CheckError(intptr_t ReturnCode, const wchar_t * Context);
  void PreserveDownloadFileTime(HANDLE AHandle, void * UserData);
  bool GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time);
  void EnsureLocation();
  UnicodeString ActualCurrentDirectory();
  void Discard();
  void DoChangeDirectory(const UnicodeString & Directory);

  void Sink(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t AParams,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TDownloadSessionAction & Action);
  void SinkRobust(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SinkFile(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param);
  void SourceRobust(const UnicodeString & AFileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void Source(const UnicodeString & AFileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TOpenRemoteFileParams * OpenParams,
    TOverwriteFileParams * FileParams,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TUploadSessionAction & Action);
  void DirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, intptr_t Attrs, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  bool ConfirmOverwrite(const UnicodeString & ASourceFullFileName, UnicodeString & ATargetFileName,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    bool AutoResume,
    const TOverwriteFileParams * FileParams,
    const TCopyParamType * CopyParam,
    OUT TOverwriteMode & OverwriteMode);
  void ReadDirectoryProgress(int64_t Bytes);
  void ResetFileTransfer();
  virtual void DoFileTransferProgress(int64_t TransferSize, int64_t Bytes);
  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes);
  void ResetCaches();
  void CaptureOutput(const UnicodeString & Str);
  void DoReadDirectory(TRemoteFileList * FileList);
  void DoReadFile(const UnicodeString & AFileName, TRemoteFile *& AFile);
  void FileTransfer(const UnicodeString & AFileName, const UnicodeString & LocalFile,
    const UnicodeString & RemoteFile, const UnicodeString & RemotePath, bool Get,
    int64_t Size, intptr_t Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);
  TDateTime ConvertLocalTimestamp(time_t Time);
  void RemoteFileTimeToDateTimeAndPrecision(const TRemoteFileTime & Source,
    TDateTime & DateTime, TModificationFmt & ModificationFmt);
  void SetLastCode(intptr_t Code);
  void StoreLastResponse(const UnicodeString & Text);
  void SetCPSLimit(TFileOperationProgressType * OperationProgress);
  bool VerifyCertificateHostName(const TFtpsCertificateData & Data);
  bool SupportsReadingFile() const;
  void AutoDetectTimeDifference(TRemoteFileList * FileList);
  void ApplyTimeDifference(TRemoteFile * File);
  void ApplyTimeDifference(
    const UnicodeString & FileName, TDateTime & Modification, TModificationFmt & ModificationFmt);
  bool LookupUploadModificationTime(
    const UnicodeString & FileName, TDateTime & Modification, TModificationFmt ModificationFmt);
  UnicodeString DoCalculateFileChecksum(bool UsingHashCommand, const UnicodeString & Alg, TRemoteFile * File);
  void DoCalculateFilesChecksum(bool UsingHashCommand, const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType * OperationProgress, bool FirstLevel);
  void HandleFeatReply();
  void ResetFeatures();
  bool SupportsSiteCommand(const UnicodeString & Command) const;
  bool SupportsCommand(const UnicodeString & Command) const;
  void RegisterChecksumAlgCommand(const UnicodeString & Alg, const UnicodeString & Command);
  void SendCommand(const UnicodeString & Command);
  bool CanTransferSkipList(intptr_t Params, uintptr_t Flags, const TCopyParamType * CopyParam) const;

  static bool Unquote(UnicodeString & Str);

private:
  enum TCommand
  {
    CMD_UNKNOWN,
    PASS,
    SYST,
    FEAT
  };

  TFileZillaIntf * FFileZillaIntf;
  TCriticalSection FQueueCriticalSection;
  TCriticalSection FTransferStatusCriticalSection;
  TMessageQueue FQueue;
  HANDLE FQueueEvent;
  TSessionInfo FSessionInfo;
  TFileSystemInfo FFileSystemInfo;
  bool FFileSystemInfoValid;
  uintptr_t FReply;
  uintptr_t FCommandReply;
  TCommand FLastCommand;
  bool FPasswordFailed;
  bool FStoredPasswordTried;
  bool FMultineResponse;
  intptr_t FLastCode;
  intptr_t FLastCodeClass;
  intptr_t FLastReadDirectoryProgress;
  UnicodeString FTimeoutStatus;
  UnicodeString FDisconnectStatus;
  TStrings * FLastResponse;
  TStrings * FLastErrorResponse;
  TStrings * FLastError;
  UnicodeString FSystem;
  TStrings * FFeatures;
  UnicodeString FCurrentDirectory;
  bool FReadCurrentDirectory;
  UnicodeString FHomeDirectory;
  TRemoteFileList * FFileList;
  TRemoteFileList * FFileListCache;
  UnicodeString FFileListCachePath;
  UnicodeString FWelcomeMessage;
  bool FActive;
  bool FOpening;
  bool FWaitingForReply;
  enum
  {
    ftaNone,
    ftaSkip,
    ftaCancel,
  } FFileTransferAbort;
  bool FIgnoreFileList;
  bool FFileTransferCancelled;
  int64_t FFileTransferResumed;
  bool FFileTransferPreserveTime;
  bool FFileTransferRemoveBOM;
  bool FFileTransferNoList;
  uintptr_t FFileTransferCPSLimit;
  bool FAwaitingProgress;
  TCaptureOutputEvent FOnCaptureOutput;
  UnicodeString FUserName;
  TAutoSwitch FListAll;
  bool FDoListAll;
  TFTPServerCapabilities * FServerCapabilities;
  TDateTime FLastDataSent;
  bool FDetectTimeDifference;
  int64_t FTimeDifference;
  std::unique_ptr<TStrings> FChecksumAlgs;
  std::unique_ptr<TStrings> FChecksumCommands;
  std::unique_ptr<TStrings> FSupportedCommands;
  std::unique_ptr<TStrings> FSupportedSiteCommands;
  std::unique_ptr<TStrings> FHashAlgs;
  typedef std::map<UnicodeString, TDateTime> TUploadedTimes;
  TUploadedTimes FUploadedTimes;
  bool FSupportsAnyChecksumFeature;
  UnicodeString FLastCommandSent;
  X509 * FCertificate;
  EVP_PKEY * FPrivateKey;
  bool FTransferActiveImmediately;
  bool FWindowsServer;
  int64_t FBytesAvailable;
  bool FBytesAvailableSupported;
  mutable UnicodeString FOptionScratch;
};

UnicodeString GetOpenSSLVersionText();

#endif // NO_FILEZILLA

