#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#ifndef __linux__
#include <share.h>
#include <lmcons.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <secext.h>
#endif

#include <Common.h>
#include <Exceptions.h>

#include "SessionInfo.h"
#include "TextsCore.h"

static UnicodeString DoXmlEscape(const UnicodeString & AStr, bool NewLine)
{
  UnicodeString Str = AStr;
  for (intptr_t Index = 1; Index <= Str.Length(); ++Index)
  {
    const wchar_t * Repl = nullptr;
    switch (Str[Index])
    {
      case L'&':
        Repl = L"amp;";
        break;

      case L'>':
        Repl = L"gt;";
        break;

      case L'<':
        Repl = L"lt;";
        break;

      case L'"':
        Repl = L"quot;";
        break;

      case L'\n':
        if (NewLine)
        {
          Repl = L"#10;";
        }
        break;

      case L'\r':
        Str.Delete(Index, 1);
        Index--;
        break;
    }

    if (Repl != nullptr)
    {
      Str[Index] = L'&';
      Str.Insert(Repl, Index + 1);
      Index += wcslen(Repl);
    }
  }
  return Str;
}

static UnicodeString XmlEscape(const UnicodeString & Str)
{
  return DoXmlEscape(Str, false);
}

static UnicodeString XmlAttributeEscape(const UnicodeString & Str)
{
  return DoXmlEscape(Str, true);
}

class TSessionActionRecord : public TObject
{
NB_DECLARE_CLASS(TSessionActionRecord)
public:
  explicit TSessionActionRecord(TActionLog * Log, TLogAction Action) :
    FLog(Log),
    FAction(Action),
    FState(Opened),
    FRecursive(false),
    FErrorMessages(nullptr),
    FNames(new TStringList()),
    FValues(new TStringList()),
    FFileList(nullptr),
    FFile(nullptr)
  {
    FLog->AddPendingAction(this);
  }

  ~TSessionActionRecord()
  {
    SAFE_DESTROY(FErrorMessages);
    SAFE_DESTROY(FNames);
    SAFE_DESTROY(FValues);
    SAFE_DESTROY(FFileList);
    SAFE_DESTROY(FFile);
  }

  void Restart()
  {
    FState = Opened;
    FRecursive = false;
    SAFE_DESTROY(FErrorMessages);
    SAFE_DESTROY(FFileList);
    SAFE_DESTROY(FFile);
    FNames->Clear();
    FValues->Clear();
  }

  bool Record()
  {
    bool Result = (FState != Opened);
#if 0
    if (Result)
    {
      if (FLog->FLogging && (FState != Cancelled))
      {
        const wchar_t * Name = ActionName();
        UnicodeString Attrs;
        if (FRecursive)
        {
          Attrs = L" recursive=\"true\"";
        }
        FLog->AddIndented(FORMAT("<%s%s>", Name,  Attrs.c_str()));
        for (intptr_t Index = 0; Index < FNames->GetCount(); ++Index)
        {
          UnicodeString Value = FValues->GetString(Index);
          if (Value.IsEmpty())
          {
            FLog->AddIndented(FORMAT("  <%s />", FNames->GetString(Index).c_str()));
          }
          else
          {
            FLog->AddIndented(FORMAT("  <%s value=\"%s\" />",
              FNames->GetString(Index).c_str(), XmlAttributeEscape(Value).c_str()));
          }
        }
        if (FFileList != nullptr)
        {
          FLog->AddIndented(L"  <files>");
          for (intptr_t Index = 0; Index < FFileList->GetCount(); ++Index)
          {
            TRemoteFile * File = FFileList->GetFile(Index);

            FLog->AddIndented(L"    <file>");
            FLog->AddIndented(FORMAT("      <filename value=\"%s\" />", XmlAttributeEscape(File->GetFileName()).c_str()));
            FLog->AddIndented(FORMAT("      <type value=\"%s\" />", XmlAttributeEscape(File->GetType()).c_str()));
            if (!File->GetIsDirectory())
            {
              FLog->AddIndented(FORMAT("      <size value=\"%s\" />", ::Int64ToStr(File->GetSize()).c_str()));
            }
            FLog->AddIndented(FORMAT("      <modification value=\"%s\" />", StandardTimestamp(File->GetModification()).c_str()));
            FLog->AddIndented(FORMAT("      <permissions value=\"%s\" />", XmlAttributeEscape(File->GetRights()->GetText()).c_str()));
            FLog->AddIndented(L"    </file>");
          }
          FLog->AddIndented(L"  </files>");
          if (File->Owner.IsSet)
          {
            FLog->AddIndented(FORMAT(L"      <owner value=\"%s\" />", XmlAttributeEscape(File->Owner.DisplayText)));
          }
          if (File->Group.IsSet)
          {
            FLog->AddIndented(FORMAT(L"      <group value=\"%s\" />", XmlAttributeEscape(File->Group.DisplayText)));
          }
        }
        if (FFile != nullptr)
        {
          FLog->AddIndented(L"  <file>");
          FLog->AddIndented(FORMAT("    <type value=\"%s\" />", XmlAttributeEscape(FFile->GetType()).c_str()));
          if (!FFile->GetIsDirectory())
          {
            FLog->AddIndented(FORMAT("    <size value=\"%s\" />", ::Int64ToStr(FFile->GetSize()).c_str()));
          }
          FLog->AddIndented(FORMAT("    <modification value=\"%s\" />", StandardTimestamp(FFile->GetModification()).c_str()));
          FLog->AddIndented(FORMAT("    <permissions value=\"%s\" />", XmlAttributeEscape(FFile->GetRights()->GetText()).c_str()));
          FLog->AddIndented(L"  </file>");
        }
        if (FState == RolledBack)
        {
          if (FErrorMessages != nullptr)
          {
            FLog->AddIndented(L"  <result success=\"false\">");
            FLog->AddMessages(L"    ", FErrorMessages);
            FLog->AddIndented(L"  </result>");
          }
          else
          {
            FLog->AddIndented(L"  <result success=\"false\" />");
          }
        }
        else
        {
          FLog->AddIndented(L"  <result success=\"true\" />");
        }
        FLog->AddIndented(FORMAT("</%s>", Name));
      }
      delete this;
    }
#endif
    return Result;
  }

  void Commit()
  {
    Close(Committed);
  }

  void Rollback(Exception * E)
  {
    DebugAssert(FErrorMessages == nullptr);
    FErrorMessages = ExceptionToMoreMessages(E);
    Close(RolledBack);
  }

  void Cancel()
  {
    Close(Cancelled);
  }

  void SetFileName(const UnicodeString & AFileName)
  {
    Parameter(L"filename", AFileName);
  }

  void Destination(const UnicodeString & Destination)
  {
    Parameter(L"destination", Destination);
  }

  void Rights(const TRights & Rights)
  {
    Parameter(L"permissions", Rights.GetText());
  }

  void Modification(const TDateTime & DateTime)
  {
    Parameter(L"modification", StandardTimestamp(DateTime));
  }

  void Recursive()
  {
    FRecursive = true;
  }

  void Command(const UnicodeString & Command)
  {
    Parameter(L"command", Command);
  }

  void AddOutput(const UnicodeString & Output, bool StdError)
  {
    const wchar_t * Name = (StdError ? L"erroroutput" : L"output");
    intptr_t Index = FNames->IndexOf(Name);
    if (Index >= 0)
    {
      FValues->SetString(Index, FValues->GetString(Index) + L"\r\n" + Output);
    }
    else
    {
      Parameter(Name, Output);
    }
  }

  void ExitCode(int ExitCode)
  {
    Parameter(L"exitcode", ::IntToStr(ExitCode));
  }

  void Checksum(const UnicodeString & Alg, const UnicodeString & Checksum)
  {
    Parameter(L"algorithm", Alg);
    Parameter(L"checksum", Checksum);
  }

  void Cwd(const UnicodeString & Path)
  {
    Parameter(L"cwd", Path);
  }

  void FileList(TRemoteFileList * FileList)
  {
    if (FFileList == nullptr)
    {
      FFileList = new TRemoteFileList();
    }
    FileList->DuplicateTo(FFileList);
  }

  void File(TRemoteFile * AFile)
  {
    if (FFile != nullptr)
    {
      SAFE_DESTROY(FFile);
    }
    FFile = AFile->Duplicate(true);
  }

protected:
  enum TState
  {
    Opened,
    Committed,
    RolledBack,
    Cancelled,
  };

  inline void Close(TState State)
  {
    DebugAssert(FState == Opened);
    FState = State;
    FLog->RecordPendingActions();
  }

  const wchar_t * ActionName() const
  {
    switch (FAction)
    {
      case laUpload: return L"upload";
      case laDownload: return L"download";
      case laTouch: return L"touch";
      case laChmod: return L"chmod";
      case laMkdir: return L"mkdir";
      case laRm: return L"rm";
      case laMv: return L"mv";
      case laCall: return L"call";
      case laLs: return L"ls";
      case laStat: return L"stat";
      case laChecksum: return L"checksum";
      case laCwd: return L"cwd";
      default:
        DebugFail();
        return L"";
    }
  }

  void Parameter(const UnicodeString & Name, const UnicodeString & Value = L"")
  {
    FNames->Add(Name);
    FValues->Add(Value);
  }

private:
  TActionLog * FLog;
  TLogAction FAction;
  TState FState;
  bool FRecursive;
  TStrings * FErrorMessages;
  TStrings * FNames;
  TStrings * FValues;
  TRemoteFileList * FFileList;
  TRemoteFile * FFile;
};

TSessionAction::TSessionAction(TActionLog * Log, TLogAction Action)
{
  if (Log->FLogging)
  {
    FRecord = new TSessionActionRecord(Log, Action);
  }
  else
  {
    FRecord = nullptr;
  }
}

TSessionAction::~TSessionAction()
{
  if (FRecord != nullptr)
  {
    Commit();
  }
}

void TSessionAction::Restart()
{
  if (FRecord != nullptr)
  {
    FRecord->Restart();
  }
}

void TSessionAction::Commit()
{
  if (FRecord != nullptr)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = nullptr;
    Record->Commit();
  }
}

void TSessionAction::Rollback(Exception * E)
{
  if (FRecord != nullptr)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = nullptr;
    Record->Rollback(E);
  }
}

void TSessionAction::Cancel()
{
  if (FRecord != nullptr)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = nullptr;
    Record->Cancel();
  }
}


TFileSessionAction::TFileSessionAction(TActionLog * Log, TLogAction Action) :
  TSessionAction(Log, Action)
{
}

TFileSessionAction::TFileSessionAction(
  TActionLog * Log, TLogAction Action, const UnicodeString & AFileName) :
  TSessionAction(Log, Action)
{
  SetFileName(AFileName);
}

void TFileSessionAction::SetFileName(const UnicodeString & AFileName)
{
  if (FRecord != nullptr)
  {
    FRecord->SetFileName(AFileName);
  }
}


TFileLocationSessionAction::TFileLocationSessionAction(
  TActionLog * Log, TLogAction Action) :
  TFileSessionAction(Log, Action)
{
}

TFileLocationSessionAction::TFileLocationSessionAction(
  TActionLog * Log, TLogAction Action, const UnicodeString & AFileName) :
  TFileSessionAction(Log, Action, AFileName)
{
}

void TFileLocationSessionAction::Destination(const UnicodeString & Destination)
{
  if (FRecord != nullptr)
  {
    FRecord->Destination(Destination);
  }
}


TUploadSessionAction::TUploadSessionAction(TActionLog * Log) :
  TFileLocationSessionAction(Log, laUpload)
{
}


TDownloadSessionAction::TDownloadSessionAction(TActionLog * Log) :
  TFileLocationSessionAction(Log, laDownload)
{
}


TChmodSessionAction::TChmodSessionAction(
  TActionLog * Log, const UnicodeString & AFileName) :
  TFileSessionAction(Log, laChmod, AFileName)
{
}

void TChmodSessionAction::Recursive()
{
  if (FRecord != nullptr)
  {
    FRecord->Recursive();
  }
}

TChmodSessionAction::TChmodSessionAction(
  TActionLog * Log, const UnicodeString & AFileName, const TRights & ARights) :
  TFileSessionAction(Log, laChmod, AFileName)
{
  Rights(ARights);
}

void TChmodSessionAction::Rights(const TRights & Rights)
{
  if (FRecord != nullptr)
  {
    FRecord->Rights(Rights);
  }
}

TTouchSessionAction::TTouchSessionAction(
  TActionLog * Log, const UnicodeString & AFileName, const TDateTime & Modification) :
  TFileSessionAction(Log, laTouch, AFileName)
{
  if (FRecord != nullptr)
  {
    FRecord->Modification(Modification);
  }
}

TMkdirSessionAction::TMkdirSessionAction(
  TActionLog * Log, const UnicodeString & AFileName) :
  TFileSessionAction(Log, laMkdir, AFileName)
{
}

TRmSessionAction::TRmSessionAction(
  TActionLog * Log, const UnicodeString & AFileName) :
  TFileSessionAction(Log, laRm, AFileName)
{
}

void TRmSessionAction::Recursive()
{
  if (FRecord != nullptr)
  {
    FRecord->Recursive();
  }
}

TMvSessionAction::TMvSessionAction(TActionLog * Log,
    const UnicodeString & AFileName, const UnicodeString & ADestination) :
  TFileLocationSessionAction(Log, laMv, AFileName)
{
  Destination(ADestination);
}

TCallSessionAction::TCallSessionAction(TActionLog * Log,
    const UnicodeString & Command, const UnicodeString & Destination) :
  TSessionAction(Log, laCall)
{
  if (FRecord != nullptr)
  {
    FRecord->Command(Command);
    FRecord->Destination(Destination);
  }
}

void TCallSessionAction::AddOutput(const UnicodeString & Output, bool StdError)
{
  if (FRecord != nullptr)
  {
    FRecord->AddOutput(Output, StdError);
  }
}

void TCallSessionAction::ExitCode(int ExitCode)
{
  if (FRecord != nullptr)
  {
    FRecord->ExitCode(ExitCode);
  }
}

TLsSessionAction::TLsSessionAction(TActionLog * Log,
    const UnicodeString & Destination) :
  TSessionAction(Log, laLs)
{
  if (FRecord != nullptr)
  {
    FRecord->Destination(Destination);
  }
}

void TLsSessionAction::FileList(TRemoteFileList * FileList)
{
  if (FRecord != nullptr)
  {
    FRecord->FileList(FileList);
  }
}

TStatSessionAction::TStatSessionAction(TActionLog * Log, const UnicodeString & AFileName) :
  TFileSessionAction(Log, laStat, AFileName)
{
}

void TStatSessionAction::File(TRemoteFile * AFile)
{
  if (FRecord != nullptr)
  {
    FRecord->File(AFile);
  }
}


TChecksumSessionAction::TChecksumSessionAction(TActionLog * Log) :
  TFileSessionAction(Log, laChecksum)
{
}

void TChecksumSessionAction::Checksum(const UnicodeString & Alg, const UnicodeString & Checksum)
{
  if (FRecord != nullptr)
  {
    FRecord->Checksum(Alg, Checksum);
  }
}


TCwdSessionAction::TCwdSessionAction(TActionLog * Log, const UnicodeString & Path) :
  TSessionAction(Log, laCwd)
{
  if (FRecord != nullptr)
  {
    FRecord->Cwd(Path);
  }
}

TSessionInfo::TSessionInfo() :
  LoginTime(Now())
{
}

TFileSystemInfo::TFileSystemInfo()
{
  ::memset(&IsCapable, 0, sizeof(IsCapable));
}

static FILE * OpenFile(const UnicodeString & LogFileName, TSessionData * SessionData, bool Append, UnicodeString & ANewFileName)
{
  UnicodeString NewFileName = StripPathQuotes(GetExpandedLogFileName(LogFileName, SessionData));
  // Result = _wfopen(ANewFileName.c_str(), (Append ? L"a" : L"w"));
#ifndef __linux__
  FILE * Result = _fsopen(::W2MB(ApiPath(NewFileName).c_str()).c_str(),
    Append ? "a" : "w", SH_DENYWR); // _SH_DENYNO); //
#else
  FILE * Result = _wfopen(ANewFileName.c_str(), (Append ? L"a" : L"w"));
#endif
  if (Result != nullptr)
  {
    setvbuf(Result, nullptr, _IONBF, BUFSIZ);
    ANewFileName = NewFileName;
  }
  else
  {
    throw ECRTExtException(FMTLOAD(LOG_OPENERROR, NewFileName.c_str()));
  }
  return Result;
}

const wchar_t * LogLineMarks = L"<>!.*";

TSessionLog::TSessionLog(TSessionUI * UI, TSessionData * SessionData,
  TConfiguration * Configuration) :
  TStringList(),
  FConfiguration(Configuration),
  FParent(nullptr),
  FLogging(false),
  FFile(nullptr),
  FLoggedLines(0),
  FTopIndex(-1),
  FUI(UI),
  FSessionData(SessionData),
  FClosed(false)
{
}

TSessionLog::~TSessionLog()
{
  FClosed = true;
  ReflectSettings();
  DebugAssert(FFile == nullptr);
}

void TSessionLog::Lock()
{
  FCriticalSection.Enter();
}

void TSessionLog::Unlock()
{
  FCriticalSection.Leave();
}

UnicodeString TSessionLog::GetSessionName() const
{
  DebugAssert(FSessionData != nullptr);
  return FSessionData->GetSessionName();
}

UnicodeString TSessionLog::GetLine(intptr_t Index) const
{
  return GetString(Index - FTopIndex);
}

TLogLineType TSessionLog::GetType(intptr_t Index) const
{
  return static_cast<TLogLineType>(reinterpret_cast<size_t>(GetObj(Index - FTopIndex)));
}

void TSessionLog::DoAddToParent(TLogLineType AType, const UnicodeString & ALine)
{
  DebugAssert(FParent != nullptr);
  FParent->Add(AType, ALine);
}

void TSessionLog::DoAddToSelf(TLogLineType AType, const UnicodeString & ALine)
{
  if (FTopIndex < 0)
  {
    FTopIndex = 0;
  }

  TStringList::AddObject(ALine, static_cast<TObject *>(ToPtr(static_cast<size_t>(AType))));

  FLoggedLines++;

  if (LogToFile())
  {
    if (FFile == nullptr)
    {
      OpenLogFile();
    }

    if (FFile != nullptr)
    {
#if defined(__BORLANDC__)
      UnicodeString Timestamp = FormatDateTime(L" yyyy-mm-dd hh:nn:ss.zzz ", Now());
      UTF8String UtfLine = UTF8String(UnicodeString(LogLineMarks[Type]) + Timestamp + Line + "\n");
      fwrite(UtfLine.c_str(), UtfLine.Length(), 1, (FILE *)FFile);
#else
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = Now();
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT(L" %04d-%02d-%02d %02d:%02d:%02d.%03d ", Y, M, D, H, N, S, MS);
      UnicodeString Timestamp = dt;
      UTF8String UtfLine = UTF8String(UnicodeString(LogLineMarks[AType]) + Timestamp + ALine + "\n");
      fprintf(static_cast<FILE *>(FFile), "%s", const_cast<char *>(AnsiString(UtfLine).c_str()));
#endif
    }
  }
}

void TSessionLog::DoAdd(TLogLineType AType, const UnicodeString & ALine,
  TDoAddLogEvent Event)
{
  UnicodeString Prefix;

  if (!GetName().IsEmpty())
  {
    Prefix = L"[" + GetName() + L"] ";
  }

  UnicodeString Line = ALine;
  while (!Line.IsEmpty())
  {
    Event(AType, Prefix + CutToChar(Line, L'\n', false));
  }
}

void TSessionLog::Add(TLogLineType Type, const UnicodeString & Line)
{
  DebugAssert(FConfiguration);
  if (GetLogging())
  {
    try
    {
      if (FParent != nullptr)
      {
        DoAdd(Type, Line, MAKE_CALLBACK(TSessionLog::DoAddToParent, this));
      }
      else
      {
        TGuard Guard(FCriticalSection);

        BeginUpdate();

        try__finally
        {
          SCOPE_EXIT
          {
            DeleteUnnecessary();

            EndUpdate();
          };
          DoAdd(Type, Line, MAKE_CALLBACK(TSessionLog::DoAddToSelf, this));
        }
        __finally
        {
          DeleteUnnecessary();

          EndUpdate();
        };
      }
    }
    catch (Exception & E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->SetLogging(false);
      try
      {
        throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
      }
      catch (Exception & E)
      {
        AddException(&E);
        FUI->HandleExtendedException(&E);
      }
    }
  }
}

void TSessionLog::AddException(Exception * E)
{
  if (E != nullptr)
  {
    Add(llException, ExceptionLogString(E));
  }
}

void TSessionLog::ReflectSettings()
{
  TGuard Guard(FCriticalSection);

  bool ALogging =
    !FClosed &&
    ((FParent != nullptr) || FConfiguration->GetLogging());

  bool Changed = false;
  if (FLogging != ALogging)
  {
    FLogging = ALogging;
    Changed = true;
  }

  // if logging to file was turned off or log file was changed -> close current log file
  if ((FFile != nullptr) &&
      (!LogToFile() || (FCurrentLogFileName != FConfiguration->GetLogFileName())))
  {
    CloseLogFile();
  }

  DeleteUnnecessary();

  // trigger event only once we are in a consistent state
  if (Changed)
  {
    StateChange();
  }

}

bool TSessionLog::LogToFile() const
{
  return GetLogging() && FConfiguration->GetLogToFile() && (FParent == nullptr);
}

void TSessionLog::CloseLogFile()
{
  if (FFile != nullptr)
  {
    fclose(static_cast<FILE *>(FFile));
    FFile = nullptr;
  }
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
  StateChange();
}

void TSessionLog::OpenLogFile()
{
  try
  {
    DebugAssert(FFile == nullptr);
    DebugAssert(FConfiguration != nullptr);
    FCurrentLogFileName = FConfiguration->GetLogFileName();
    FFile = OpenFile(FCurrentLogFileName, FSessionData, FConfiguration->GetLogFileAppend(), FCurrentFileName);
  }
  catch (Exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName.Clear();
    FCurrentFileName.Clear();
    FConfiguration->SetLogFileName(UnicodeString());
    try
    {
      throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
    }
    catch (Exception & E)
    {
      AddException(&E);
      FUI->HandleExtendedException(&E);
    }
  }
  StateChange();
}

void TSessionLog::StateChange()
{
  if (FOnStateChange != nullptr)
  {
    FOnStateChange(this);
  }
}

void TSessionLog::DeleteUnnecessary()
{
  BeginUpdate();
  try__finally
  {
    SCOPE_EXIT
    {
      EndUpdate();
    };
    if (!GetLogging() || (FParent != nullptr))
    {
      Clear();
    }
    else
    {
      while (!FConfiguration->GetLogWindowComplete() && (GetCount() > FConfiguration->GetLogWindowLines()))
      {
        Delete(0);
        ++FTopIndex;
      }
    }
  }
  __finally
  {
    EndUpdate();
  };
}

void TSessionLog::AddSystemInfo()
{
  AddStartupInfo(true);
}

void TSessionLog::AddStartupInfo()
{
  AddStartupInfo(false);
}

void TSessionLog::AddStartupInfo(bool System)
{
  TSessionData * Data = (System ? nullptr : FSessionData);
  if (GetLogging())
  {
    if (FParent != nullptr)
    {
      // do not add session info for secondary session
      // (this should better be handled in the TSecondaryTerminal)
    }
    else
    {
      DoAddStartupInfo(Data);
    }
  }
}

UnicodeString TSessionLog::GetTlsVersionName(TTlsVersion TlsVersion)
{
  switch (TlsVersion)
  {
    default:
      DebugFail();
    case ssl2:
      return "SSLv2";
    case ssl3:
      return "SSLv3";
    case tls10:
      return "TLSv1.0";
    case tls11:
      return "TLSv1.1";
    case tls12:
      return "TLSv1.2";
  }
}

UnicodeString TSessionLog::LogSensitive(const UnicodeString & Str)
{
  if (FConfiguration->GetLogSensitive() && !Str.IsEmpty())
  {
    return Str;
  }
  else
  {
    return BooleanToEngStr(!Str.IsEmpty());
  }
}

UnicodeString TSessionLog::GetCmdLineLog()
{
  TODO("GetCmdLine()");
  UnicodeString Result = L"";

  if (!FConfiguration->GetLogSensitive())
  {
#if 0
    TManagementScript Script(StoredSessions, false);
    Script.MaskPasswordInCommandLine(Result, true);
#endif
  }

  return Result;
}

template <typename T>
UnicodeString EnumName(T Value, const UnicodeString & ANames)
{
  int N = int(Value);

  UnicodeString Names(ANames);

  do
  {
    UnicodeString Name = CutToChar(Names, L';', true);
    if (N == 0)
    {
      return Name;
    }
    N--;
  }
  while ((N >= 0) && !Names.IsEmpty());

  return L"(unknown)";
}

#define ADSTR(S) DoAdd(llMessage, S, MAKE_CALLBACK(TSessionLog::DoAddToSelf, this));
#define ADF(S, ...) DoAdd(llMessage, FORMAT(S, ##__VA_ARGS__), MAKE_CALLBACK(TSessionLog::DoAddToSelf, this));

void TSessionLog::DoAddStartupInfo(TSessionData * Data)
{
  TGuard Guard(FCriticalSection);

  BeginUpdate();
  try__finally
  {
    SCOPE_EXIT
    {
      DeleteUnnecessary();
      EndUpdate();
    };
    if (Data == nullptr)
    {
      AddSeparator();
      ADF(L"NetBox %s (OS %s)", FConfiguration->GetProductVersionStr().c_str(), FConfiguration->GetOSVersionStr().c_str());
      std::unique_ptr<THierarchicalStorage> Storage(FConfiguration->CreateConfigStorage());
       DebugAssert(Storage.get());
      ADF(L"Configuration: %s", Storage->GetSource().c_str());
#ifndef __linux__
      if (0)
      {
        typedef BOOL (WINAPI * TGetUserNameEx)(EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG nSize);
        HINSTANCE Secur32 = LoadLibrary(L"secur32.dll");
        TGetUserNameEx GetUserNameEx =
          (Secur32 != nullptr) ? reinterpret_cast<TGetUserNameEx>(::GetProcAddress(Secur32, "GetUserNameExW")) : nullptr;
        wchar_t UserName[UNLEN + 1];
        ULONG UserNameSize = _countof(UserName);
        if ((GetUserNameEx == nullptr) || DebugAlwaysFalse(!GetUserNameEx(NameSamCompatible, (LPWSTR)UserName, &UserNameSize)))
        {
          wcscpy_s(UserName, UNLEN, L"<Failed to retrieve username>");
        }
        UnicodeString LogStr;
        if (FConfiguration->GetLogProtocol() <= 0)
        {
          LogStr = L"Normal";
        }
        else if (FConfiguration->GetLogProtocol() == 1)
        {
          LogStr = L"Debug 1";
        }
        else if (FConfiguration->GetLogProtocol() >= 2)
        {
          LogStr = L"Debug 2";
        }
        if (FConfiguration->GetLogSensitive())
        {
          LogStr += L", Logging passwords";
        }
        ADF(L"Log level: %s", LogStr.c_str());
        ADF(L"Local account: %s", UserName);
      }
#endif
      uint16_t Y, M, D, H, N, S, MS;
      TDateTime DateTime = Now();
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d", D, M, Y, H, N, S);
      // ADF(L"Login time: %s", FormatDateTime(L"dddddd tt", Now()).c_str());
      ADF(L"Working directory: %s", ::GetCurrentDir().c_str());
      // ADF(L"Command-line: %s", GetCmdLineLog().c_str());
//      if (FConfiguration->LogProtocol >= 1)
//      {
//        AddOptions(GetGlobalOptions());
//      }
      // ADF(L"Time zone: %s", GetTimeZoneLogString().c_str());
      if (!AdjustClockForDSTEnabled())
      {
        ADF(L"Warning: System option \"Automatically adjust clock for Daylight Saving Time\" is disabled, timestamps will not be represented correctly");
      }
      ADF(L"Login time: %s", dt.c_str());
      AddSeparator();
    }
    else
    {
      if (0)
      {
        ADF(L"Session name: %s (%s)", Data->GetSessionName().c_str(), Data->GetSource().c_str());
      }
      ADF(L"Host name: %s (Port: %d)", Data->GetHostNameExpanded().c_str(), Data->GetPortNumber());
      if (0)
      {
        ADF(L"User name: %s (Password: %s, Key file: %s)",
          Data->GetUserNameExpanded().c_str(), BooleanToEngStr(!Data->GetPassword().IsEmpty()).c_str(),
           BooleanToEngStr(!Data->GetPublicKeyFile().IsEmpty()).c_str())
      }
      if (Data->GetUsesSsh())
      {
        ADF(L"Tunnel: %s", BooleanToEngStr(Data->GetTunnel()).c_str());
        if (Data->GetTunnel())
        {
          ADF(L"Tunnel: Host name: %s (Port: %d)", Data->GetTunnelHostName().c_str(), Data->GetTunnelPortNumber());
          if (0)
          {
            ADF(L"Tunnel: User name: %s (Password: %s, Key file: %s)",
              Data->GetTunnelUserName().c_str(), BooleanToEngStr(!Data->GetTunnelPassword().IsEmpty()).c_str(),
               BooleanToEngStr(!Data->GetTunnelPublicKeyFile().IsEmpty()).c_str());
              ADF(L"Tunnel: Local port number: %d", Data->GetTunnelLocalPortNumber());
          }
        }
      }
      ADF(L"Transfer Protocol: %s", Data->GetFSProtocolStr().c_str());
      ADF(L"Code Page: %d", Data->GetCodePageAsNumber());
      if (Data->GetUsesSsh() || (Data->GetFSProtocol() == fsFTP))
      {
        // wchar_t * PingTypes = (wchar_t *)L"-NC";
        TPingType PingType;
        intptr_t PingInterval;
        if (Data->GetFSProtocol() == fsFTP)
        {
          PingType = Data->GetFtpPingType();
          PingInterval = Data->GetFtpPingInterval();
        }
        else
        {
          PingType = Data->GetPingType();
          PingInterval = Data->GetPingInterval();
        }
        ADF(L"Ping type: %s, Ping interval: %d sec; Timeout: %d sec",
          EnumName(PingType, PingTypeNames).c_str(), PingInterval, Data->GetTimeout());
        ADF(L"Disable Nagle: %s",
          BooleanToEngStr(Data->GetTcpNoDelay()).c_str());
      }
      TProxyMethod ProxyMethod = Data->GetActualProxyMethod();
      {
        UnicodeString fp = FORMAT(L"FTP proxy %d", Data->GetFtpProxyLogonType());
        ADF(L"Proxy: %s",
          (Data->GetFtpProxyLogonType() != 0) ?
            fp.c_str() :
            EnumName(ProxyMethod, ProxyMethodNames).c_str());
      }
      if ((Data->GetFtpProxyLogonType() != 0) || (ProxyMethod != ::pmNone))
      {
        ADF(L"ProxyHostName: %s (Port: %d); ProxyUsername: %s; Passwd: %s",
          Data->GetProxyHost().c_str(), Data->GetProxyPort(),
          Data->GetProxyUsername().c_str(), BooleanToEngStr(!Data->GetProxyPassword().IsEmpty()).c_str());
        if (ProxyMethod == pmTelnet)
        {
          ADF(L"Telnet command: %s", Data->GetProxyTelnetCommand().c_str());
        }
        if (ProxyMethod == pmCmd)
        {
          ADF(L"Local command: %s", Data->GetProxyLocalCommand().c_str());
        }
      }
      if (Data->GetUsesSsh() || (Data->GetFSProtocol() == fsFTP))
      {
        ADF(L"Send buffer: %d", Data->GetSendBuf());
      }
      if (Data->GetUsesSsh())
      {
        ADF(L"SSH protocol version: %s; Compression: %s",
          Data->GetSshProtStr().c_str(), BooleanToEngStr(Data->GetCompression()).c_str());
        ADF(L"Bypass authentication: %s",
         BooleanToEngStr(Data->GetSshNoUserAuth()).c_str());
        ADF(L"Try agent: %s; Agent forwarding: %s; TIS/CryptoCard: %s; KI: %s; GSSAPI: %s",
           BooleanToEngStr(Data->GetTryAgent()).c_str(), BooleanToEngStr(Data->GetAgentFwd()).c_str(), BooleanToEngStr(Data->GetAuthTIS()).c_str(),
           BooleanToEngStr(Data->GetAuthKI()).c_str(), BooleanToEngStr(Data->GetAuthGSSAPI()).c_str());
        if (Data->GetAuthGSSAPI())
        {
          ADF(L"GSSAPI: Forwarding: %s; Server realm: %s",
            BooleanToEngStr(Data->GetGSSAPIFwdTGT()).c_str(), Data->GetGSSAPIServerRealm().c_str());
        }
        ADF(L"Ciphers: %s; Ssh2DES: %s",
          Data->GetCipherList().c_str(), BooleanToEngStr(Data->GetSsh2DES()).c_str());
//        ADF(L"KEX: %s", (Data->KexList));
        UnicodeString Bugs;
        for (intptr_t Index = 0; Index < BUG_COUNT; ++Index)
        {
          AddToList(Bugs, EnumName(Data->GetBug(static_cast<TSshBug>(Index)), AutoSwitchNames), L",");
        }
        ADF(L"SSH Bugs: %s", Bugs.c_str());
        ADF(L"Simple channel: %s", BooleanToEngStr(Data->GetSshSimple()).c_str());
        ADF(L"Return code variable: %s; Lookup user groups: %c",
           Data->GetDetectReturnVar() ? UnicodeString(L"Autodetect").c_str() : Data->GetReturnVar().c_str(),
           EnumName(Data->GetLookupUserGroups(), AutoSwitchNames).c_str());
        ADF(L"Shell: %s", Data->GetShell().IsEmpty() ? UnicodeString(L"default").c_str() : Data->GetShell().c_str());
        ADF(L"EOL: %s, UTF: %s", EnumName(Data->GetEOLType(), EOLTypeNames).c_str(), EnumName(Data->GetNotUtf(), NotAutoSwitchNames).c_str()); // NotUtf duplicated in FTP branch
        ADF(L"Clear aliases: %s, Unset nat.vars: %s, Resolve symlinks: %s; Follow directory symlinks: %s",
           BooleanToEngStr(Data->GetClearAliases()).c_str(), BooleanToEngStr(Data->GetUnsetNationalVars()).c_str(),
           BooleanToEngStr(Data->GetResolveSymlinks()).c_str(), BooleanToEngStr(Data->GetFollowDirectorySymlinks()).c_str());
        ADF(L"LS: %s, Ign LS warn: %s, Scp1 Comp: %s",
           Data->GetListingCommand().c_str(),
           BooleanToEngStr(Data->GetIgnoreLsWarnings()).c_str(),
           BooleanToEngStr(Data->GetScp1Compatibility()).c_str());
      }
      if ((Data->GetFSProtocol() == fsSFTP) || (Data->GetFSProtocol() == fsSFTPonly))
      {
        UnicodeString Bugs;
        for (intptr_t Index = 0; Index < SFTP_BUG_COUNT; ++Index)
        {
          AddToList(Bugs, EnumName(Data->GetSFTPBug(static_cast<TSftpBug>(Index)), AutoSwitchNames), L",");
        }
        ADF(L"SFTP Bugs: %s", Bugs.c_str());
        ADF(L"SFTP Server: %s", Data->GetSftpServer().IsEmpty()? UnicodeString(L"default").c_str() : Data->GetSftpServer().c_str());
      }
      bool FtpsOn = false;
      if (Data->GetFSProtocol() == fsFTP)
      {
        ADF(L"UTF: %s", EnumName(Data->GetNotUtf(), NotAutoSwitchNames).c_str()); // duplicated in UsesSsh branch
        UnicodeString Ftps;
        switch (Data->GetFtps())
        {
          case ftpsImplicit:
            Ftps = L"Implicit TLS/SSL";
            FtpsOn = true;
            break;

          case ftpsExplicitSsl:
            Ftps = L"Explicit SSL/TLS";
            FtpsOn = true;
            break;

          case ftpsExplicitTls:
            Ftps = L"Explicit TLS/SSL";
            FtpsOn = true;
            break;

          default:
            DebugAssert(Data->GetFtps() == ftpsNone);
            Ftps = L"None";
            break;
        }
        // kind of hidden option, so do not reveal it unless it is set
        if (Data->GetFtpTransferActiveImmediately() != asAuto)
        {
          ADF(L"Transfer active immediately: %s", EnumName(Data->GetFtpTransferActiveImmediately(), AutoSwitchNames).c_str());
        }
        ADF(L"FTPS: %s; [Client certificate: %s]",
           Ftps.c_str(), LogSensitive(Data->GetTlsCertificateFile()).c_str());
        ADF(L"FTP: Passive: %s [Force IP: %s]; MLSD: %s  [List all: %s]; HOST: %s",
           BooleanToEngStr(Data->GetFtpPasvMode()).c_str(),
           EnumName(Data->GetFtpForcePasvIp(), AutoSwitchNames).c_str(),
           EnumName(Data->GetFtpUseMlsd(), AutoSwitchNames).c_str(),
           EnumName(Data->GetFtpListAll(), AutoSwitchNames).c_str(),
           EnumName(Data->GetFtpHost(), AutoSwitchNames).c_str());
      }
      if (Data->GetFSProtocol() == fsWebDAV)
      {
        FtpsOn = (Data->GetFtps() != ftpsNone);
        ADF(L"HTTPS: %s [Client certificate: %s]",
          BooleanToEngStr(FtpsOn).c_str(), LogSensitive(Data->GetTlsCertificateFile()).c_str());
      }
      if (FtpsOn)
      {
        if (Data->GetFSProtocol() == fsFTP)
        {
          ADF(L"Session reuse: %s", BooleanToEngStr(Data->GetSslSessionReuse()).c_str());
        }
        ADF(L"TLS/SSL versions: %s-%s", GetTlsVersionName(Data->GetMinTlsVersion()).c_str(), GetTlsVersionName(Data->GetMaxTlsVersion()).c_str());
      }
      ADF(L"Local directory: %s, Remote directory: %s, Update: %s, Cache: %s",
         Data->GetLocalDirectory().IsEmpty() ? UnicodeString(L"default").c_str() : Data->GetLocalDirectory().c_str(),
         Data->GetRemoteDirectory().IsEmpty() ? UnicodeString(L"home").c_str() : Data->GetRemoteDirectory().c_str(),
         BooleanToEngStr(Data->GetUpdateDirectories()).c_str(),
         BooleanToEngStr(Data->GetCacheDirectories()).c_str());
      ADF(L"Cache directory changes: %s, Permanent: %s",
         BooleanToEngStr(Data->GetCacheDirectoryChanges()).c_str(),
         BooleanToEngStr(Data->GetPreserveDirectoryChanges()).c_str());
      ADF(L"Recycle bin: Delete to: %s, Overwritten to: %s, Bin path: %s",
         BooleanToEngStr(Data->GetDeleteToRecycleBin()).c_str(),
         BooleanToEngStr(Data->GetOverwrittenToRecycleBin()).c_str(),
         Data->GetRecycleBinPath().c_str());
      if (Data->GetTrimVMSVersions())
      {
        ADF(L"Trim VMS versions: %s",
          BooleanToEngStr(Data->GetTrimVMSVersions()).c_str());
      }
      UnicodeString TimeInfo;
      if ((Data->GetFSProtocol() == fsSFTP) || (Data->GetFSProtocol() == fsSFTPonly) || (Data->GetFSProtocol() == fsSCPonly) || (Data->GetFSProtocol() == fsWebDAV))
      {
        AddToList(TimeInfo, FORMAT(L"DST mode: %s", EnumName(static_cast<int>(Data->GetDSTMode()), DSTModeNames).c_str()), L";");
      }
      if ((Data->GetFSProtocol() == fsSCPonly) || (Data->GetFSProtocol() == fsFTP))
      {
        intptr_t TimeDifferenceMin = TimeToMinutes(Data->GetTimeDifference());
        AddToList(TimeInfo, FORMAT(L"Timezone offset: %dh %dm", TimeDifferenceMin / MinsPerHour, TimeDifferenceMin % MinsPerHour), L";");
      }
      ADSTR(TimeInfo);

      if (Data->GetFSProtocol() == fsWebDAV)
      {
        ADF(L"Compression: %s",
          BooleanToEngStr(Data->GetCompression()).c_str());
      }

      AddSeparator();
    }
  }
  __finally
  {
    DeleteUnnecessary();

    EndUpdate();
  };
}

void TSessionLog::AddOption(const UnicodeString & LogStr)
{
  ADSTR(LogStr);
}

void TSessionLog::AddOptions(TOptions * Options)
{
  Options->LogOptions(MAKE_CALLBACK(TSessionLog::AddOption, this));
}

#undef ADF
#undef ADSTR

void TSessionLog::AddSeparator()
{
//  Add(llMessage, L"--------------------------------------------------------------------------");
}

intptr_t TSessionLog::GetBottomIndex() const
{
  return (GetCount() > 0 ? (GetTopIndex() + GetCount() - 1) : -1);
}

bool TSessionLog::GetLoggingToFile() const
{
  DebugAssert((FFile == nullptr) || LogToFile());
  return (FFile != nullptr);
}

void TSessionLog::Clear()
{
  TGuard Guard(FCriticalSection);

  FTopIndex += GetCount();
  TStringList::Clear();
}

TSessionLog * TSessionLog::GetParent()
{
  return FParent;
}

void TSessionLog::SetParent(TSessionLog * Value)
{
  FParent = Value;
}

bool TSessionLog::GetLogging() const
{
  return FLogging;
}

TNotifyEvent & TSessionLog::GetOnChange()
{
  return TStringList::GetOnChange();
}

void TSessionLog::SetOnChange(TNotifyEvent Value)
{
  TStringList::SetOnChange(Value);
}

TNotifyEvent & TSessionLog::GetOnStateChange()
{
  return FOnStateChange;
}

void TSessionLog::SetOnStateChange(TNotifyEvent Value)
{
  FOnStateChange = Value;
}

UnicodeString TSessionLog::GetCurrentFileName() const
{
  return FCurrentFileName;
}

intptr_t TSessionLog::GetTopIndex() const
{
  return FTopIndex;
}

UnicodeString TSessionLog::GetName() const
{
  return FName;
}

void TSessionLog::SetName(const UnicodeString & Value)
{
  FName = Value;
}

intptr_t TSessionLog::GetCount() const
{
  return TStringList::GetCount();
}

TActionLog::TActionLog(TSessionUI * UI, TSessionData * SessionData,
  TConfiguration * Configuration) :
  FConfiguration(Configuration),
  FLogging(false),
  FFile(nullptr),
  FUI(UI),
  FSessionData(SessionData),
  FPendingActions(new TList()),
  FClosed(false),
  FInGroup(false),
  FEnabled(true),
  FIndent(L"  ")
{
  DebugAssert(UI != nullptr);
  DebugAssert(SessionData != nullptr);
  Init(UI, SessionData, Configuration);
}

TActionLog::TActionLog(TConfiguration * Configuration)
{
  Init(nullptr, nullptr, Configuration);
  // not associated with session, so no need to waiting for anything
  ReflectSettings();
}

void TActionLog::Init(TSessionUI * UI, TSessionData * SessionData,
  TConfiguration * Configuration)
{
//  FCriticalSection = new TCriticalSection;
  FConfiguration = Configuration;
  FUI = UI;
  FSessionData = SessionData;
  FFile = nullptr;
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
  FLogging = false;
  FClosed = false;
  FPendingActions = new TList();
  FIndent = L"  ";
  FInGroup = false;
  FEnabled = true;
}

TActionLog::~TActionLog()
{
  DebugAssert(FPendingActions->GetCount() == 0);
  SAFE_DESTROY(FPendingActions);
  FClosed = true;
  ReflectSettings();
  DebugAssert(FFile == nullptr);
}

void TActionLog::Add(const UnicodeString & Line)
{
  DebugAssert(FConfiguration);
  if (FLogging)
  {
    try
    {
      TGuard Guard(FCriticalSection);
      if (FFile == nullptr)
      {
        OpenLogFile();
      }

      if (FFile != nullptr)
      {
        UTF8String UtfLine = UTF8String(Line);
        fwrite(UtfLine.c_str(), 1, UtfLine.Length(), static_cast<FILE *>(FFile));
        fwrite("\n", 1, 1, static_cast<FILE *>(FFile));
      }
    }
    catch (Exception & E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->SetLogActions(false);
      try
      {
        throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
      }
      catch (Exception & E)
      {
        if (FUI != nullptr)
        {
          FUI->HandleExtendedException(&E);
        }
      }
    }
  }
}

void TActionLog::AddIndented(const UnicodeString & Line)
{
  Add(FIndent + Line);
}

void TActionLog::AddFailure(TStrings * Messages)
{
  AddIndented(L"<failure>");
  AddMessages(L"  ", Messages);
  AddIndented(L"</failure>");
}

void TActionLog::AddFailure(Exception * E)
{
  std::unique_ptr<TStrings> Messages(ExceptionToMoreMessages(E));
  if (Messages.get() != nullptr)
  {
    try__finally
    {
      AddFailure(Messages.get());
    }
    __finally
    {
//      delete Messages;
    };
  }
}

void TActionLog::AddMessages(const UnicodeString & Indent, TStrings * Messages)
{
  for (intptr_t Index = 0; Index < Messages->GetCount(); ++Index)
  {
    AddIndented(
      FORMAT((Indent + L"<message>%s</message>").c_str(), XmlEscape(Messages->GetString(Index)).c_str()));
  }
}

void TActionLog::ReflectSettings()
{
  TGuard Guard(FCriticalSection);

  bool ALogging =
    !FClosed && FConfiguration->GetLogActions() && GetEnabled();

  if (ALogging && !FLogging)
  {
    FLogging = true;
#if 0
    Add(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    UnicodeString SessionName =
      (FSessionData != NULL) ? XmlAttributeEscape(FSessionData->SessionName) : UnicodeString(L"nosession");
    Add(FORMAT(L"<session xmlns=\"http://winscp.net/schema/session/1.0\" name=\"%s\" start=\"%s\">",
      (SessionName, StandardTimestamp())));
#endif
  }
  else if (!ALogging && FLogging)
  {
    if (FInGroup)
    {
      EndGroup();
    }
    // do not try to close the file, if it has not been opened, to avoid recursion
    if (FFile != nullptr)
    {
//      Add(L"</session>");
    }
    CloseLogFile();
    FLogging = false;
  }

}

void TActionLog::CloseLogFile()
{
  if (FFile != nullptr)
  {
    fclose(static_cast<FILE *>(FFile));
    FFile = nullptr;
  }
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
}

void TActionLog::OpenLogFile()
{
  try
  {
    DebugAssert(FFile == nullptr);
    DebugAssert(FConfiguration != nullptr);
    FCurrentLogFileName = FConfiguration->GetActionsLogFileName();
    FFile = OpenFile(FCurrentLogFileName, FSessionData, false, FCurrentFileName);
  }
  catch (Exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName.Clear();
    FCurrentFileName.Clear();
    FConfiguration->SetLogActions(false);
    try
    {
      throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
    }
    catch (Exception & E)
    {
      if (FUI != nullptr)
      {
        FUI->HandleExtendedException(&E);
      }
    }
  }
}

void TActionLog::AddPendingAction(TSessionActionRecord * Action)
{
  FPendingActions->Add(Action);
}

void TActionLog::RecordPendingActions()
{
  while ((FPendingActions->GetCount() > 0) &&
         NB_STATIC_DOWNCAST(TSessionActionRecord, FPendingActions->GetItem(0))->Record())
  {
    FPendingActions->Delete(0);
  }
}

void TActionLog::BeginGroup(const UnicodeString & Name)
{
  DebugAssert(!FInGroup);
  FInGroup = true;
  DebugAssert(FIndent == L"  ");
  AddIndented(FORMAT(L"<group name=\"%s\" start=\"%s\">",
    XmlAttributeEscape(Name).c_str(), StandardTimestamp().c_str()));
  FIndent = L"    ";
}

void TActionLog::EndGroup()
{
  DebugAssert(FInGroup);
  FInGroup = false;
  DebugAssert(FIndent == L"    ");
  FIndent = L"  ";
  // this is called from ReflectSettings that in turn is called when logging fails,
  // so do not try to close the group, if it has not been opened, to avoid recursion
  if (FFile != nullptr)
  {
    AddIndented("</group>");
  }
}

void TActionLog::SetEnabled(bool Value)
{
  if (GetEnabled() != Value)
  {
    FEnabled = Value;
    ReflectSettings();
  }
}

NB_IMPLEMENT_CLASS(TSessionActionRecord, NB_GET_CLASS_INFO(TObject), nullptr)

