#pragma once

#include "Terminal.h"
#include "FileOperationProgress.h"

class TTerminalItem;
class TSimpleThread : public TObject
{
NB_DISABLE_COPY(TSimpleThread)
NB_DECLARE_CLASS(TSimpleThread)
public:
  explicit TSimpleThread();
  virtual ~TSimpleThread();
  void Init();

  virtual void Start();
  void WaitFor(uint32_t Milliseconds = INFINITE);
  virtual void Terminate() {}
  void Close();
  bool IsFinished();

protected:
  HANDLE FThread;
  TThreadID FThreadId;
  bool FFinished;

  virtual void Execute() = 0;
  virtual void Finished();

public:
  static int ThreadProc(void * Thread);
};

class TSignalThread : public TSimpleThread
{
NB_DISABLE_COPY(TSignalThread)
NB_DECLARE_CLASS(TSignalThread)
public:
  void Init(bool LowPriority);
  virtual void Start();
  virtual void Terminate();
  void TriggerEvent();

protected:
  HANDLE FEvent;
  bool FTerminated;

  explicit TSignalThread();
  virtual ~TSignalThread();

  virtual bool WaitForEvent();
  int WaitForEvent(uint32_t Timeout);
  virtual void Execute();
  virtual void ProcessEvent() = 0;
};

class TTerminal;
class TQueueItem;
class TTerminalQueue;
class TQueueItemProxy;
class TTerminalQueueStatus;

//typedef void __fastcall (__closure * TQueueListUpdate)
//  (TTerminalQueue * Queue);
DEFINE_CALLBACK_TYPE1(TQueueListUpdateEvent, void,
  TTerminalQueue * /*Queue*/);
//typedef void __fastcall (__closure * TQueueItemUpdateEvent)
//  (TTerminalQueue * Queue, TQueueItem * Item);
DEFINE_CALLBACK_TYPE2(TQueueItemUpdateEvent, void,
  TTerminalQueue * /*Queue*/, TQueueItem * /*Item*/);

enum TQueueEvent
{
  qeEmpty,
  qeEmptyButMonitored,
  qePendingUserAction,
};

//typedef void __fastcall (__closure * TQueueEventEvent)
//  (TTerminalQueue * Queue, TQueueEvent Event);
DEFINE_CALLBACK_TYPE2(TQueueEventEvent, void,
  TTerminalQueue * /*Queue*/, TQueueEvent /*Event*/);
//---------------------------------------------------------------------------
class TTerminalQueue : public TSignalThread
{
friend class TQueueItem;
friend class TQueueItemProxy;
NB_DISABLE_COPY(TTerminalQueue)
public:
  explicit TTerminalQueue(TTerminal * ATerminal, TConfiguration * AConfiguration);
  virtual ~TTerminalQueue();

  void Init();
  void AddItem(TQueueItem * Item);
  TTerminalQueueStatus * CreateStatus(TTerminalQueueStatus * Current);
  void Idle();

  /*__property bool IsEmpty = { read = GetIsEmpty };
  __property int TransfersLimit = { read = FTransfersLimit, write = SetTransfersLimit };
  __property int KeepDoneItemsFor = { read = FKeepDoneItemsFor, write = SetKeepDoneItemsFor };
  __property bool Enabled = { read = FEnabled, write = SetEnabled };
  __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  __property TQueueListUpdate OnListUpdate = { read = FOnListUpdate, write = FOnListUpdate };
  __property TQueueItemUpdateEvent OnQueueItemUpdate = { read = FOnQueueItemUpdate, write = FOnQueueItemUpdate };
  __property TQueueEventEvent OnEvent = { read = FOnEvent, write = FOnEvent };*/

  bool GetIsEmpty() const;
  intptr_t GetTransfersLimit() const { return FTransfersLimit; }
  intptr_t GetKeepDoneItemsFor() const { return FKeepDoneItemsFor; }
  bool GetEnabled() const { return FEnabled; }
  TQueryUserEvent & GetOnQueryUser() { return FOnQueryUser; }
  void SetOnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; }
  TPromptUserEvent & GetOnPromptUser() { return FOnPromptUser; }
  void SetOnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; }
  TExtendedExceptionEvent & GetOnShowExtendedException() { return FOnShowExtendedException; }
  void SetOnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; }
  TQueueListUpdateEvent & GetOnListUpdate() { return FOnListUpdate; }
  void SetOnListUpdate(TQueueListUpdateEvent Value) { FOnListUpdate = Value; }
  TQueueItemUpdateEvent & GetOnQueueItemUpdate() { return FOnQueueItemUpdate; }
  void SetOnQueueItemUpdate(TQueueItemUpdateEvent Value) { FOnQueueItemUpdate = Value; }
  TQueueEventEvent & GetOnEvent() { return FOnEvent; }
  void SetOnEvent(TQueueEventEvent Value) { FOnEvent = Value; }

protected:
  friend class TTerminalItem;
  friend class TQueryUserAction;
  friend class TPromptUserAction;
  friend class TShowExtendedExceptionAction;

  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TQueueItemUpdateEvent FOnQueueItemUpdate;
  TQueueListUpdateEvent FOnListUpdate;
  TQueueEventEvent FOnEvent;
  TTerminal * FTerminal;
  TConfiguration * FConfiguration;
  TSessionData * FSessionData;
  TList * FItems;
  TList * FDoneItems;
  intptr_t FItemsInProcess;
  TCriticalSection FItemsSection;
  intptr_t FFreeTerminals;
  TList * FTerminals;
  TList * FForcedItems;
  intptr_t FTemporaryTerminals;
  intptr_t FOverallTerminals;
  intptr_t FTransfersLimit;
  intptr_t FKeepDoneItemsFor;
  bool FEnabled;
  TDateTime FIdleInterval;
  TDateTime FLastIdle;

public:
  inline static TQueueItem * GetItem(TList * List, intptr_t Index);
  inline TQueueItem * GetItem(intptr_t Index);
  void FreeItemsList(TList *& List);
  void UpdateStatusForList(
    TTerminalQueueStatus * Status, TList * List, TTerminalQueueStatus * Current);
  bool ItemGetData(TQueueItem * Item, TQueueItemProxy * Proxy);
  bool ItemProcessUserAction(TQueueItem * Item, void * Arg);
  bool ItemMove(TQueueItem * Item, TQueueItem * BeforeItem);
  bool ItemExecuteNow(TQueueItem * Item);
  bool ItemDelete(TQueueItem * Item);
  bool ItemPause(TQueueItem * Item, bool Pause);
  bool ItemSetCPSLimit(TQueueItem * Item, uintptr_t CPSLimit);
  bool ItemGetCPSLimit(TQueueItem * Item, uintptr_t & CPSLimit) const;

  void RetryItem(TQueueItem * Item);
  void DeleteItem(TQueueItem * Item, bool CanKeep);

  virtual bool WaitForEvent();
  virtual void ProcessEvent();
  void TerminalFinished(TTerminalItem * TerminalItem);
  bool TerminalFree(TTerminalItem * TerminalItem);

  void DoQueueItemUpdate(TQueueItem * Item);
  void DoListUpdate();
  void DoEvent(TQueueEvent Event);

public:
  void SetMasks(const UnicodeString & Value);
  void SetTransfersLimit(intptr_t Value);
  void SetKeepDoneItemsFor(intptr_t Value);
  void SetEnabled(bool Value);
  void SetIsEmpty(bool Value);
};

//---------------------------------------------------------------------------
class TQueueItem : public TObject
{
friend class TTerminalQueue;
friend class TTerminalItem;
NB_DISABLE_COPY(TQueueItem)
NB_DECLARE_CLASS(TQueueItem)
public:
  enum TStatus
  {
    qsPending, qsConnecting, qsProcessing, qsPrompt, qsQuery, qsError,
    qsPaused, qsDone,
  };
  struct TInfo : public TObject
  {
    TInfo() :
      Operation(foNone),
      Side(osLocal),
      SingleFile(false)
    {}
    TFileOperation Operation;
    TOperationSide Side;
    UnicodeString Source;
    UnicodeString Destination;
    UnicodeString ModifiedLocal;
    UnicodeString ModifiedRemote;
    bool SingleFile;
  };

  static bool IsUserActionStatus(TStatus Status);

/*
  __property TStatus Status = { read = GetStatus };
  __property HANDLE CompleteEvent = { read = FCompleteEvent, write = FCompleteEvent };
*/
  HANDLE GetCompleteEvent() const { return FCompleteEvent; }
  void SetCompleteEvent(HANDLE Value) { FCompleteEvent = Value; }

protected:
  TStatus FStatus;
  TCriticalSection FSection;
  TTerminalItem * FTerminalItem;
  TFileOperationProgressType * FProgressData;
  TQueueItem::TInfo * FInfo;
  TTerminalQueue * FQueue;
  HANDLE FCompleteEvent;
  uintptr_t FCPSLimit;
  TDateTime FDoneAt;

  explicit TQueueItem();
  virtual ~TQueueItem();

public:
  void SetMasks(const UnicodeString & Value);
  void SetStatus(TStatus Status);
  TStatus GetStatus() const;
  void SetProgress(TFileOperationProgressType & ProgressData);
  void GetData(TQueueItemProxy * Proxy) const;
  void SetCPSLimit(uintptr_t CPSLimit);
  bool GetCPSLimit(uintptr_t & CPSLimit) const;

private:
  void Execute(TTerminalItem * TerminalItem);
  virtual void DoExecute(TTerminal * Terminal) = 0;
  uintptr_t GetCPSLimit() const;
  virtual uintptr_t DefaultCPSLimit() const;
  virtual UnicodeString GetStartupDirectory() const = 0;
  void Complete();
};

class TQueueItemProxy : public TObject
{
friend class TQueueItem;
friend class TTerminalQueueStatus;
friend class TTerminalQueue;
NB_DISABLE_COPY(TQueueItemProxy)
NB_DECLARE_CLASS(TQueueItemProxy)
public:
  bool Update();
  bool ProcessUserAction();
  bool Move(bool Sooner);
  bool Move(TQueueItemProxy * BeforeItem);
  bool ExecuteNow();
  bool Delete();
  bool Pause();
  bool Resume();
  bool SetCPSLimit(uintptr_t CPSLimit);
  bool GetCPSLimit(uintptr_t & CPSLimit) const;

/*
  __property TFileOperationProgressType * ProgressData = { read = GetProgressData };
  __property __int64 TotalTransferred = { read = GetTotalTransferred };
  __property TQueueItem::TInfo * Info = { read = FInfo };
  __property TQueueItem::TStatus Status = { read = FStatus };
  __property bool ProcessingUserAction = { read = FProcessingUserAction };
  __property int Index = { read = GetIndex };
  __property void * UserData = { read = FUserData, write = FUserData };
*/
  TQueueItem::TInfo * GetInfo() const { return FInfo; }
  TQueueItem::TStatus GetStatus() const { return FStatus; }
  bool GetProcessingUserAction() const { return FProcessingUserAction; }
  void * GetUserData() const { return FUserData; }
  void * GetUserData() { return FUserData; }
  void SetUserData(void * Value) { FUserData = Value; }
  void SetMasks(const UnicodeString & Value);

private:
  TFileOperationProgressType * FProgressData;
  TQueueItem::TStatus FStatus;
  TTerminalQueue * FQueue;
  TQueueItem * FQueueItem;
  TTerminalQueueStatus * FQueueStatus;
  TQueueItem::TInfo * FInfo;
  bool FProcessingUserAction;
  void * FUserData;

  explicit TQueueItemProxy(TTerminalQueue * Queue, TQueueItem * QueueItem);
  virtual ~TQueueItemProxy();

public:
  intptr_t GetIndex() const;
  TFileOperationProgressType * GetProgressData();
  int64_t GetTotalTransferred();
};

class TTerminalQueueStatus : public TObject
{
friend class TTerminalQueue;
friend class TQueueItemProxy;
NB_DISABLE_COPY(TTerminalQueueStatus)
public:
  virtual ~TTerminalQueueStatus();

  TQueueItemProxy * FindByQueueItem(TQueueItem * QueueItem);

  /*__property int Count = { read = GetCount };
  __property int DoneCount = { read = FDoneCount };
  __property int ActiveCount = { read = GetActiveCount };
  __property int DoneAndActiveCount = { read = GetDoneAndActiveCount };
  __property int ActiveAndPendingCount = { read = GetActiveAndPendingCount };
  __property TQueueItemProxy * Items[int Index] = { read = GetItem };*/

protected:
  TTerminalQueueStatus();

  void Add(TQueueItemProxy * ItemProxy);
  void Delete(TQueueItemProxy * ItemProxy);
  void ResetStats();

private:
  TList * FList;
  intptr_t FDoneCount;
  mutable intptr_t FActiveCount;

public:
  intptr_t GetCount() const;
  intptr_t GetDoneCount() const { return FDoneCount; }
  intptr_t GetActiveCount() const;
  intptr_t GetDoneAndActiveCount() const;
  intptr_t GetActiveAndPendingCount() const;
  TQueueItemProxy * GetItem(intptr_t Index) const;
  TQueueItemProxy * GetItem(intptr_t Index);
  void SetMasks(const UnicodeString & Value);
  void SetDoneCount(intptr_t Value);
};

class TLocatedQueueItem : public TQueueItem
{
protected:
  explicit TLocatedQueueItem(TTerminal * Terminal);
  virtual ~TLocatedQueueItem() {}

  virtual void DoExecute(TTerminal * Terminal);
  virtual UnicodeString GetStartupDirectory() const;

private:
  UnicodeString FCurrentDir;
};

class TTransferQueueItem : public TLocatedQueueItem
{
NB_DISABLE_COPY(TTransferQueueItem)
public:
  explicit TTransferQueueItem(TTerminal * Terminal,
    const TStrings * AFilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params, TOperationSide Side,
    bool SingleFile);
  virtual ~TTransferQueueItem();

protected:
  TStrings * FFilesToCopy;
  UnicodeString FTargetDir;
  TCopyParamType * FCopyParam;
  intptr_t FParams;

  virtual uintptr_t DefaultCPSLimit() const;
};

class TUploadQueueItem : public TTransferQueueItem
{
public:
  explicit TUploadQueueItem(TTerminal * Terminal,
    const TStrings * AFilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params, bool SingleFile);
  virtual ~TUploadQueueItem() {}

protected:
  virtual void DoExecute(TTerminal * Terminal);
};

class TDownloadQueueItem : public TTransferQueueItem
{
public:
  explicit TDownloadQueueItem(TTerminal * Terminal,
    const TStrings * AFilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params, bool SingleFile);
  virtual ~TDownloadQueueItem() {}

protected:
  virtual void DoExecute(TTerminal * Terminal);
};

class TUserAction;
class TTerminalThread : public TSignalThread
{
NB_DISABLE_COPY(TTerminalThread)
public:
  explicit TTerminalThread(TTerminal * Terminal);
  void Init();
  virtual ~TTerminalThread();

  void TerminalOpen();
  void TerminalReopen();

  void Cancel();
  void Idle();

/*
  __property TNotifyEvent OnIdle = { read = FOnIdle, write = FOnIdle };
  __property bool Cancelling = { read = FCancel };
*/
  TNotifyEvent & GetOnIdle() { return FOnIdle; }
  void SetOnIdle(TNotifyEvent Value) { FOnIdle = Value; }
  bool GetCancelling() const { return FCancel; }

protected:
  virtual void ProcessEvent();

private:
  TTerminal * FTerminal;

  TInformationEvent FOnInformation;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TDisplayBannerEvent FOnDisplayBanner;
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TNotifyEvent FOnInitializeLog;

  TNotifyEvent FOnIdle;

  TNotifyEvent FAction;
  HANDLE FActionEvent;
  TUserAction * FUserAction;

  Exception * FException;
  Exception * FIdleException;
  bool FCancel;
  bool FCancelled;
  bool FPendingIdle;

  DWORD FMainThread;
  TCriticalSection FSection;

  void WaitForUserAction(TUserAction * UserAction);
  void RunAction(TNotifyEvent Action);

  static void SaveException(Exception & E, Exception *& Exception);
  static void Rethrow(Exception *& Exception);
  void FatalAbort();
  void CheckCancel();

  void TerminalOpenEvent(TObject * Sender);
  void TerminalReopenEvent(TObject * Sender);

  void TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, bool Status, intptr_t Phase);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString & AQuery, TStrings * MoreMessages, uintptr_t Answers,
    const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    const UnicodeString & SessionName, const UnicodeString & Banner,
    bool & NeverShowAgain, intptr_t Options);
  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly);
  void TerminalStartReadDirectory(TObject * Sender);
  void TerminalReadDirectoryProgress(TObject * Sender, intptr_t Progress, intptr_t ResolvedLinks, bool & Cancel);
  void TerminalInitializeLog(TObject * Sender);
};

