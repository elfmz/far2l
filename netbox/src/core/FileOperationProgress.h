#pragma once

#include <Common.h>

#include "Configuration.h"
#include "CopyParam.h"

class TFileOperationProgressType;
enum TFileOperation
{
  foNone, foCopy, foMove, foDelete, foSetProperties,
  foRename, foCustomCommand, foCalculateSize, foRemoteMove, foRemoteCopy,
  foGetProperties, foCalculateChecksum, foLock, foUnlock
};

// csCancelTransfer and csRemoteAbort are used with SCP only
enum TCancelStatus
{
  csContinue = 0,
  csCancel,
  csCancelTransfer,
  csRemoteAbort,
};

enum TBatchOverwrite
{
  boNo,
  boAll,
  boNone,
  boOlder,
  boAlternateResume,
  boAppend,
  boResume,
};

/*typedef void __fastcall (__closure *TFileOperationProgressEvent)
  (TFileOperationProgressType & ProgressData);*/
DEFINE_CALLBACK_TYPE1(TFileOperationProgressEvent, void,
  TFileOperationProgressType & /*ProgressData*/);
/*typedef void __fastcall (__closure *TFileOperationFinished)
  (TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);*/
DEFINE_CALLBACK_TYPE6(TFileOperationFinishedEvent, void,
  TFileOperation /*Operation*/, TOperationSide /*Side*/, bool /*Temp*/,
  const UnicodeString & /*FileName*/, bool /*Success*/, TOnceDoneOperation & /*OnceDoneOperation*/);
//---------------------------------------------------------------------------
class TFileOperationProgressType : public TObject
{
private:
  // when it was last time suspended (to calculate suspend time in Resume())
  uintptr_t FSuspendTime;
  // when current file was started being transfered
  TDateTime FFileStartTime;
  intptr_t FFilesFinished;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinishedEvent FOnFinished;
  bool FReset;
  uintptr_t FLastSecond;
  uintptr_t FRemainingCPS;
  bool FCounterSet;
  std::vector<uint32_t> FTicks;
  std::vector<int64_t> FTotalTransferredThen;

protected:
  void ClearTransfer();
  inline void DoProgress();

public:
  // common data
  TFileOperation Operation;
  // on what side if operation being processed (local/remote), source of copy
  TOperationSide Side;
  UnicodeString FileName;
  UnicodeString FullFileName;
  UnicodeString Directory;
  bool AsciiTransfer;
  // Can be true with SCP protocol only
  bool TransferingFile;
  bool Temp;

  // file size to read/write
  int64_t LocalSize;
  int64_t LocallyUsed;
  int64_t TransferSize;
  int64_t TransferedSize;
  int64_t SkippedSize;
  bool InProgress;
  bool FileInProgress;
  TCancelStatus Cancel;
  intptr_t Count;
  // when operation started
  TDateTime StartTime;
  // bytes transfered
  int64_t TotalTransfered;
  int64_t TotalSkipped;
  int64_t TotalSize;

  TBatchOverwrite BatchOverwrite;
  bool SkipToAll;
  uintptr_t CPSLimit;

  bool TotalSizeSet;

  bool Suspended;

  explicit TFileOperationProgressType();
  explicit TFileOperationProgressType(
    TFileOperationProgressEvent AOnProgress, TFileOperationFinishedEvent AOnFinished);
  virtual ~TFileOperationProgressType();
  void AssignButKeepSuspendState(const TFileOperationProgressType & Other);
  void AddLocallyUsed(int64_t ASize);
  void AddTransfered(int64_t ASize, bool AddToTotals = true);
  void AddResumed(int64_t ASize);
  void AddSkippedFileSize(int64_t ASize);
  void Clear();
  uintptr_t CPS() const;
  void Finish(const UnicodeString & AFileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  void Progress();
  uintptr_t LocalBlockSize();
  bool IsLocallyDone() const;
  bool IsTransferDone() const;
  void SetFile(const UnicodeString & AFileName, bool AFileInProgress = true);
  void SetFileInProgress();
  intptr_t OperationProgress() const;
  uintptr_t TransferBlockSize();
  uintptr_t AdjustToCPSLimit(uintptr_t Size);
  void ThrottleToCPSLimit(uintptr_t Size);
  static uintptr_t StaticBlockSize();
  void Reset();
  void Resume();
  void SetLocalSize(int64_t ASize);
  void SetAsciiTransfer(bool AAsciiTransfer);
  void SetTransferSize(int64_t ASize);
  void ChangeTransferSize(int64_t ASize);
  void RollbackTransfer();
  void SetTotalSize(int64_t ASize);
  void Start(TFileOperation AOperation, TOperationSide ASide, intptr_t ACount);
  void Start(TFileOperation AOperation,
    TOperationSide ASide, intptr_t ACount, bool ATemp, const UnicodeString & ADirectory,
    uintptr_t ACPSLimit);
  void Stop();
  void Suspend();
  // whole operation
  TDateTime TimeElapsed() const;
  // only current file
  TDateTime TimeExpected() const;
  TDateTime TotalTimeExpected() const;
  TDateTime TotalTimeLeft() const;
  intptr_t TransferProgress() const;
  intptr_t OverallProgress() const;
  intptr_t TotalTransferProgress() const;
  void SetSpeedCounters();
};

class TSuspendFileOperationProgress : public TObject
{
NB_DISABLE_COPY(TSuspendFileOperationProgress)
public:
  explicit TSuspendFileOperationProgress(TFileOperationProgressType * OperationProgress) :
    FOperationProgress(OperationProgress)
  {
    if (FOperationProgress != nullptr)
    {
      FOperationProgress->Suspend();
    }
  }

  virtual ~TSuspendFileOperationProgress()
  {
    if (FOperationProgress != nullptr)
    {
      FOperationProgress->Resume();
    }
  }

private:
  TFileOperationProgressType * FOperationProgress;
};

