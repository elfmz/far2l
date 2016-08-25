#pragma once

#include <CopyParam.h>

struct TSynchronizeParamType : public TObject
{
  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  intptr_t Params;
  intptr_t Options;
};

class TSynchronizeController;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
DEFINE_CALLBACK_TYPE2(TSynchronizeAbortEvent, void,
  TObject * /*Sender*/, bool /*Close*/);
DEFINE_CALLBACK_TYPE2(TSynchronizeThreadsEvent, void,
  TObject * /*Sender*/, TThreadMethod /*Method*/);
enum TSynchronizeLogEntry
{
  slScan,
  slStart,
  slChange,
  slUpload,
  slDelete,
  slDirChange
};

DEFINE_CALLBACK_TYPE3(TSynchronizeLogEvent, void,
  TSynchronizeController * /*Controller*/, TSynchronizeLogEntry /*Entry*/,
  const UnicodeString & /*Message*/);
DEFINE_CALLBACK_TYPE8(TSynchronizeStartStopEvent, void,
  TObject * /*Sender*/, bool /*Start*/, const TSynchronizeParamType & /*Params*/,
  const TCopyParamType & /*CopyParam*/, TSynchronizeOptions * /*Options*/,
  TSynchronizeAbortEvent /*OnAbort*/, TSynchronizeThreadsEvent /*OnSynchronizeThreads*/,
  TSynchronizeLogEvent /*OnSynchronizeLog*/);
DEFINE_CALLBACK_TYPE8(TSynchronizeEvent, void,
  TSynchronizeController * /*Sender*/, const UnicodeString & /*LocalDirectory*/,
  const UnicodeString & /*RemoteDirectory*/, const TCopyParamType & /*CopyParam*/,
  const TSynchronizeParamType & /*Params*/, TSynchronizeChecklist ** /*Checklist*/,
  TSynchronizeOptions * /*Options*/, bool /*Full*/);
DEFINE_CALLBACK_TYPE3(TSynchronizeInvalidEvent, void,
  TSynchronizeController * /*Sender*/, const UnicodeString & /*Directory*/,
  const UnicodeString & /*ErrorStr*/);
DEFINE_CALLBACK_TYPE2(TSynchronizeTooManyDirectoriesEvent, void,
  TSynchronizeController * /*Sender*/, intptr_t & /*MaxDirectories*/);

namespace Discmon {
class TDiscMonitor;
}

enum TSynchronizeOperation
{
  soUpload,
  soDelete
};

class TSynchronizeController : public TObject
{
NB_DISABLE_COPY(TSynchronizeController)
public:
  explicit TSynchronizeController(TSynchronizeEvent AOnSynchronize,
    TSynchronizeInvalidEvent AOnSynchronizeInvalid,
    TSynchronizeTooManyDirectoriesEvent AOnTooManyDirectories);
  virtual ~TSynchronizeController();

  void StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
    TSynchronizeOptions * Options,
    TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
    TSynchronizeLogEvent OnSynchronizeLog);
  void LogOperation(TSynchronizeOperation Operation, const UnicodeString & AFileName);

private:
  TSynchronizeEvent FOnSynchronize;
  TSynchronizeParamType FSynchronizeParams;
  TSynchronizeOptions * FOptions;
  TSynchronizeThreadsEvent FOnSynchronizeThreads;
  Discmon::TDiscMonitor * FSynchronizeMonitor;
  TSynchronizeAbortEvent FSynchronizeAbort;
  TSynchronizeInvalidEvent FOnSynchronizeInvalid;
  TSynchronizeTooManyDirectoriesEvent FOnTooManyDirectories;
  TSynchronizeLogEvent FSynchronizeLog;
  TCopyParamType FCopyParam;

  void SynchronizeChange(TObject * Sender, const UnicodeString & Directory,
    bool & SubdirsChanged);
  void SynchronizeAbort(bool Close);
  void SynchronizeLog(TSynchronizeLogEntry Entry, const UnicodeString & Message);
  void SynchronizeInvalid(TObject * Sender, const UnicodeString & Directory,
    const UnicodeString & ErrorStr);
  void SynchronizeFilter(TObject * Sender, const UnicodeString & DirectoryName,
    bool & Add);
  void SynchronizeTooManyDirectories(TObject * Sender, intptr_t & MaxDirectories);
  void SynchronizeDirectoriesChange(TObject * Sender, intptr_t Directories);
};

