#pragma once

#include <Interface.h>
#include "FarPlugin.h"
#include <FileOperationProgress.h>
#include <Terminal.h>
#include <GUIConfiguration.h>
#include <SynchronizeController.h>
#include <Queue.h>
#include <WinInterface.h>

class TTerminal;
class TSessionData;
class TRemoteFile;
class TBookmarkList;
class TWinSCPPlugin;
class TNetBoxPlugin;
class TFarButton;
class TFarDialogItem;
class TFarDialog;
class TTerminalQueue;
class TTerminalQueueStatus;
class TQueueItem;
class TKeepaliveThread;
struct TMessageParams;
#define REMOTE_DIR_HISTORY L"WinscpRemoteDirectory"
#define ASCII_MASK_HISTORY L"WinscpAsciiMask"
#define LINK_FILENAME_HISTORY L"WinscpRemoteLink"
#define LINK_POINT_TO_HISTORY L"WinscpRemoteLinkPointTo"
#define APPLY_COMMAND_HISTORY L"WinscpApplyCmd"
#define APPLY_COMMAND_PARAM_HISTORY L"WinscpApplyCmdParam"
#define LOG_FILE_HISTORY L"WinscpLogFile"
#define REMOTE_SYNC_HISTORY L"WinscpRemoteSync"
#define LOCAL_SYNC_HISTORY L"WinscpLocalSync"
#define MOVE_TO_HISTORY L"WinscpMoveTo"
#define WINSCP_FILE_MASK_HISTORY L"WinscpFileMask"
#define MAKE_SESSION_FOLDER_HISTORY L"WinscpSessionFolder"

// for Properties dialog
//const int cpMode  = 0x01;
//const int cpOwner = 0x02;
//const int cpGroup = 0x04;
// for Copy dialog
//const int coTempTransfer        = 0x01;
//const int coDisableNewerOnly    = 0x04;
//// for Synchronize and FullSynchronize dialogs
//const int spSelectedOnly = 0x800;
//// for Synchronize dialogs
//const int soAllowSelectedOnly = 0x01;
//// for FullSynchronize dialog
//const int fsoDisableTimestamp = 0x01;
//const int fsoAllowSelectedOnly = 0x02;
enum TSessionActionEnum
{
  saAdd,
  saEdit,
  saConnect
};

//DEFINE_CALLBACK_TYPE2(TGetSynchronizeOptionsEvent, void,
//  intptr_t /*Params*/, TSynchronizeOptions & /*Options*/);
DEFINE_CALLBACK_TYPE3(TGetSpaceAvailableEvent, void,
  const UnicodeString & /*Path*/, TSpaceAvailable & /*ASpaceAvailable*/,
  bool & /*Close*/);

struct TMultipleEdit : public TObject
{
  UnicodeString FileName;
  UnicodeString FileTitle;
  UnicodeString Directory;
  UnicodeString LocalFileName;
  bool PendingSave;
};

struct TEditHistory : public TObject
{
  UnicodeString FileName;
  UnicodeString Directory;
  bool operator==(const TEditHistory & rh) const { return (FileName == rh.FileName) && (Directory == rh.Directory); }
};

DEFINE_CALLBACK_TYPE2(TProcessSessionEvent, void, TSessionData * /*Data*/, void * /*Param*/);

class TWinSCPFileSystem : public TCustomFarFileSystem
{
friend class TWinSCPPlugin;
friend class TNetBoxPlugin;
friend class TKeepaliveThread;
friend class TQueueDialog;
NB_DISABLE_COPY(TWinSCPFileSystem)
NB_DECLARE_CLASS(TWinSCPFileSystem)
public:
  explicit TWinSCPFileSystem(TCustomFarPlugin * APlugin);
  void Init(TSecureShell * SecureShell);
  virtual ~TWinSCPFileSystem();

  virtual void Close();

protected:
  bool Connect(TSessionData * Data);
  void Disconnect();
  void SaveSession();

  virtual void GetOpenPluginInfoEx(DWORD & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, int & StartPanelMode,
    int & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData);
  virtual bool GetFindDataEx(TObjectList * PanelItems, int OpMode);
  virtual bool ProcessKeyEx(intptr_t Key, uintptr_t ControlState);
  virtual bool SetDirectoryEx(const UnicodeString & Dir, int OpMode);
  virtual intptr_t MakeDirectoryEx(UnicodeString & Name, int OpMode);
  virtual bool DeleteFilesEx(TObjectList * PanelItems, int OpMode);
  virtual intptr_t GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);
  virtual intptr_t PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode);
  virtual bool ProcessEventEx(intptr_t Event, void * Param);

  void ProcessEditorEvent(intptr_t Event, void * Param);

  virtual void HandleException(Exception * E, int OpMode = 0);
  void KeepaliveThreadCallback();

  inline bool IsSessionList() const;
  inline bool Connected() const;
  TWinSCPPlugin * GetWinSCPPlugin();
  void ShowOperationProgress(TFileOperationProgressType & ProgressData,
    bool Force);
  bool SessionDialog(TSessionData * Data, TSessionActionEnum & Action);
  void EditConnectSession(TSessionData * Data, bool Edit);
  void EditConnectSession(TSessionData * Data, bool Edit, bool NewData, bool FillInConnect);
  void DuplicateOrRenameSession(TSessionData * Data,
    bool Duplicate);
  void FocusSession(const TSessionData * Data);
  void DeleteSession(TSessionData * Data, void * Param);
  void ProcessSessions(TObjectList * PanelItems,
    TProcessSessionEvent ProcessSession, void * Param);
  void ExportSession(TSessionData * Data, void * Param);
  bool ImportSessions(TObjectList * PanelItems, bool Move, int OpMode);
  void FileProperties();
  void CreateLink();
  void TransferFiles(bool Move);
  void RenameFile();
  void ApplyCommand();
  void ShowInformation();
  void InsertTokenOnCommandLine(const UnicodeString & Token, bool Separate);
  void InsertSessionNameOnCommandLine();
  void InsertFileNameOnCommandLine(bool Full);
  UnicodeString GetFullFilePath(const TRemoteFile * AFile) const;
  void InsertPathOnCommandLine();
  void CopyFullFileNamesToClipboard();
  void FullSynchronize(bool Source);
  void Synchronize();
  void OpenDirectory(bool Add);
  void HomeDirectory();
  void ToggleSynchronizeBrowsing();
  bool IsSynchronizedBrowsing() const;
  bool PropertiesDialog(TStrings * AFileList,
    const UnicodeString & Directory,
    const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
    TRemoteProperties * Properties, intptr_t AllowedChanges);
  bool ExecuteCommand(const UnicodeString & Command);
  void TerminalCaptureLog(const UnicodeString & AddedLine, TCaptureOutputType OutputEvent);
  bool CopyDialog(bool ToRemote, bool Move, const TStrings * AFileList,
    intptr_t Options,
    intptr_t CopyParamAttrs,
    OUT UnicodeString & TargetDirectory,
    OUT TGUICopyParamType * Params);
  bool LinkDialog(UnicodeString & AFileName, UnicodeString & PointTo, bool & Symbolic,
    bool Edit, bool AllowSymbolic);
  void FileSystemInfoDialog(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, const UnicodeString & SpaceAvailablePath,
    TGetSpaceAvailableEvent OnGetSpaceAvailable);
  bool OpenDirectoryDialog(bool Add, UnicodeString & Directory,
    TBookmarkList * BookmarkList);
  bool ApplyCommandDialog(UnicodeString & Command, intptr_t & Params);
  bool FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
    intptr_t & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, intptr_t Options,
    const TUsableCopyParamAttrs & CopyParamAttrs);
  bool SynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
    TTerminal::TSynchronizeMode Mode, intptr_t Params,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory);
  bool RemoteTransferDialog(TStrings * AFileList, UnicodeString & Target,
    UnicodeString & FileMask, bool Move);
  bool RenameFileDialog(TRemoteFile * AFile, UnicodeString & NewName);
  uintptr_t MoreMessageDialog(const UnicodeString & Str, TStrings * MoreMessages,
    TQueryType Type, uintptr_t Answers, const TMessageParams * AParams = nullptr);
  bool PasswordDialog(TSessionData * SessionData,
    TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Results, bool StoredCredentialsTried);
  bool BannerDialog(const UnicodeString & SessionName, const UnicodeString & Banner,
    bool & NeverShowAgain, intptr_t Options);
  bool CreateDirectoryDialog(UnicodeString & Directory,
    TRemoteProperties * Properties, bool & SaveSettings);
  bool QueueDialog(TTerminalQueueStatus * Status, bool ClosingPlugin);
  bool SynchronizeDialog(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
    bool & SaveSettings, intptr_t Options, intptr_t CopyParamAttrs,
    TGetSynchronizeOptionsEvent OnGetOptions);
  void DoSynchronize(TSynchronizeController * Sender,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void DoSynchronizeInvalid(TSynchronizeController * Sender,
    const UnicodeString & Directory, const UnicodeString & ErrorStr);
  void DoSynchronizeTooManyDirectories(TSynchronizeController * Sender,
    intptr_t & MaxDirectories);
  void Synchronize(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, TTerminal::TSynchronizeMode Mode,
    const TCopyParamType & CopyParam, intptr_t Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options);
  bool SynchronizeAllowSelectedOnly();
  void GetSynchronizeOptions(intptr_t Params, TSynchronizeOptions & Options);
  void RequireCapability(intptr_t Capability);
  void RequireLocalPanel(TFarPanelInfo * Panel, const UnicodeString & Message);
  bool AreCachesEmpty();
  void ClearCaches();
  void OpenSessionInPutty();
  void QueueShow(bool ClosingPlugin);
  TTerminalQueueStatus * ProcessQueue(bool Hidden);
  bool EnsureCommandSessionFallback(TFSCapability Capability);
  void ConnectTerminal(TTerminal * Terminal);
  void TemporarilyDownloadFiles(TStrings * AFileList,
    TCopyParamType & CopyParam, UnicodeString & TempDir);
  intptr_t UploadFiles(bool Move, int OpMode, bool Edit, UnicodeString & DestPath);
  void UploadOnSave(bool NoReload);
  void UploadFromEditor(bool NoReload, const UnicodeString & AFileName,
    const UnicodeString & RealFileName, UnicodeString & DestPath);
  void LogAuthentication(TTerminal * Terminal, const UnicodeString & Msg);
  void MultipleEdit();
  void MultipleEdit(const UnicodeString & Directory, const UnicodeString & AFileName, TRemoteFile * AFile);
  void EditViewCopyParam(TCopyParamType & CopyParam);
  bool SynchronizeBrowsing(const UnicodeString & NewPath);
  bool IsEditHistoryEmpty() const;
  void EditHistory();
  UnicodeString ProgressBar(intptr_t Percentage, intptr_t Width);
  bool IsLogging() const;
  void ShowLog();

  TTerminal * GetTerminal() const { return FTerminal; }
  TSessionData * GetSessionData() const { return FTerminal ? FTerminal->GetSessionData() : nullptr; }
  TSessionData * GetSessionData() { return FTerminal ? FTerminal->GetSessionData() : nullptr; }

protected:
  virtual UnicodeString GetCurrDirectory() const { return FTerminal ? FTerminal->GetCurrDirectory() : UnicodeString(); }

private:
  bool TerminalCheckForEsc();
  void TerminalClose(TObject * Sender);
  void TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, bool ReloadOnly);
  void TerminalStartReadDirectory(TObject * Sender);
  void TerminalReadDirectoryProgress(TObject * Sender, intptr_t Progress,
    intptr_t ResolvedLinks, bool & Cancel);
  void TerminalInformation(TTerminal * Terminal,
    const UnicodeString & Str, bool Status, intptr_t Phase);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString & AQuery, TStrings * MoreMessages, uintptr_t Answers,
    const TQueryParams * AParams, uintptr_t & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions,
    TStrings * Prompts, TStrings * Results, bool & AResult,
    void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    const UnicodeString & SessionName, const UnicodeString & Banner, bool & NeverShowAgain,
    intptr_t Options);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void TerminalDeleteLocalFile(const UnicodeString & AFileName, bool Alternative);
  HANDLE TerminalCreateLocalFile(const UnicodeString & LocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  inline DWORD TerminalGetLocalFileAttributes(const UnicodeString & LocalFileName);
  inline BOOL TerminalSetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  BOOL TerminalMoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  BOOL TerminalRemoveLocalDirectory(const UnicodeString & LocalDirName);
  BOOL TerminalCreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);
  void OperationProgress(
    TFileOperationProgressType & ProgressData);
  void OperationFinished(TFileOperation Operation,
    TOperationSide Side, bool DragDrop, const UnicodeString & AFileName, bool Success,
    TOnceDoneOperation & DisconnectWhenComplete);
  void CancelConfiguration(TFileOperationProgressType & ProgressData);
  TStrings * CreateFileList(TObjectList * PanelItems,
    TOperationSide Side, bool SelectedOnly = false, const UnicodeString & Directory = L"",
    bool FileNameOnly = false, TStrings * AFileList = nullptr);
  TStrings * CreateSelectedFileList(TOperationSide Side,
    TFarPanelInfo ** PanelInfo = nullptr);
  TStrings * CreateFocusedFileList(TOperationSide Side,
    TFarPanelInfo ** PanelInfo = nullptr);
  void CustomCommandGetParamValue(
    const UnicodeString & AName, UnicodeString & Value);
  void TerminalSynchronizeDirectory(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, bool & Continue, bool Collect);
  void QueueListUpdate(TTerminalQueue * Queue);
  void QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item);
  void QueueEvent(TTerminalQueue * Queue, TQueueEvent Event);
  void GetSpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable, bool & Close);
  void QueueAddItem(TQueueItem * Item);
  UnicodeString GetFileNameHash(const UnicodeString & AFileName) const;
  intptr_t GetFilesRemote(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);

private:
  TTerminalQueue * GetQueue();
  TTerminalQueueStatus * GetQueueStatus();

private:
  TTerminal * FTerminal;
  TTerminalQueue * FQueue;
  TTerminalQueueStatus * FQueueStatus;
  TCriticalSection FQueueStatusSection;
  TQueueEvent FQueueEvent;
  HANDLE FProgressSaveScreenHandle;
  HANDLE FSynchronizationSaveScreenHandle;
  HANDLE FAuthenticationSaveScreenHandle;
  TDateTime FSynchronizationStart;
  TStrings * FFileList;
  TList * FPanelItems;
  UnicodeString FSavedFindFolder;
  UnicodeString FOriginalEditFile;
  UnicodeString FLastEditFile;
  UnicodeString FLastMultipleEditFile;
  UnicodeString FLastMultipleEditFileTitle;
  UnicodeString FLastMultipleEditDirectory;
  intptr_t FLastEditorID;
  TGUICopyParamType FLastEditCopyParam;
  TKeepaliveThread * FKeepaliveThread;
  TSynchronizeController * FSynchronizeController;
  TStrings * FCapturedLog;
  TStrings * FAuthenticationLog;
  typedef std::map<intptr_t, TMultipleEdit> TMultipleEdits;
  TMultipleEdits FMultipleEdits;
  typedef std::vector<TEditHistory> TEditHistories;
  TEditHistories FEditHistories;
  UnicodeString FLastPath;
  TStrings * FPathHistory;
  UnicodeString FSessionsFolder;
  UnicodeString FNewSessionsFolder;
  UnicodeString FPrevSessionName;
  bool FQueueStatusInvalidated;
  bool FQueueItemInvalidated;
  bool FRefreshLocalDirectory;
  bool FRefreshRemoteDirectory;
  bool FQueueEventPending;
  bool FReloadDirectory;
  bool FLastMultipleEditReadOnly;
  bool FNoProgress;
  bool FSynchronizationCompare;
  bool FEditorPendingSave;
  bool FNoProgressFinish;
  bool FSynchronisingBrowse;
  bool FOutputLog;
  bool FLoadingSessionList;
  bool FCurrentDirectoryWasChanged;
};

class TSessionPanelItem : public TCustomFarPanelItem
{
NB_DISABLE_COPY(TSessionPanelItem)
public:
  explicit TSessionPanelItem(const UnicodeString & APath);
  explicit TSessionPanelItem(const TSessionData * ASessionData);
  static void SetPanelModes(TFarPanelModes * PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  UnicodeString FPath;
  const TSessionData * FSessionData;

  virtual void GetData(
    DWORD & Flags, UnicodeString & AFileName, int64_t & Size,
    DWORD & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    DWORD & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
};

class TSessionFolderPanelItem : public TCustomFarPanelItem
{
public:
  explicit TSessionFolderPanelItem(const UnicodeString & Folder);

protected:
  UnicodeString FFolder;

  virtual void GetData(
    DWORD & Flags, UnicodeString & AFileName, int64_t & Size,
    DWORD & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    DWORD & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
};

class TRemoteFilePanelItem : public TCustomFarPanelItem
{
NB_DISABLE_COPY(TRemoteFilePanelItem)
public:
  explicit TRemoteFilePanelItem(TRemoteFile * ARemoteFile);
  static void SetPanelModes(TFarPanelModes * PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  TRemoteFile * FRemoteFile;

  virtual void GetData(
    DWORD & Flags, UnicodeString & AFileName, int64_t & Size,
    DWORD & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    DWORD & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
  virtual UnicodeString GetCustomColumnData(size_t Column);
  static void TranslateColumnTypes(UnicodeString & AColumnTypes,
    TStrings * ColumnTitles);
};

