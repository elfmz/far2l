#include <vcl.h>
#pragma hdrstop

#ifndef __linux__
#include <shlobj.h>
#include <shellapi.h>
#endif
#include <Common.h>

#include "GUITools.h"
#include "GUIConfiguration.h"
#include <TextsCore.h>
#include <CoreMain.h>
#include <SessionData.h>
#include <Interface.h>

#if 0
template<class TEditControl>
void ValidateMaskEditT(const UnicodeString & Mask, TEditControl * Edit, int ForceDirectoryMasks)
{
  DebugAssert(Edit != nullptr);
  TFileMasks Masks(ForceDirectoryMasks);
  try
  {
    Masks = Mask;
  }
  catch (EFileMasksException & E)
  {
    ShowExtendedException(&E);
    Edit->SetFocus();
    // This does not work for TEdit and TMemo (descendants of TCustomEdit) anymore,
    // as it re-selects whole text on exception in TCustomEdit.CMExit
//    Edit->SelStart = E.ErrorStart - 1;
//    Edit->SelLength = E.ErrorLen;
    Abort();
  }
}

void ValidateMaskEdit(TFarComboBox * Edit)
{
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}

void ValidateMaskEdit(TFarEdit * Edit)
{
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}
#endif

bool FindFile(UnicodeString & APath)
{
  bool Result = ::FileExists(APath);
  if (!Result)
  {
    intptr_t Len = ::GetEnvironmentVariable(L"PATH", nullptr, 0);
    if (Len > 0)
    {
      UnicodeString Paths;
      Paths.SetLength(Len - 1);
      ::GetEnvironmentVariable(L"PATH", reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Paths.c_str())), static_cast<DWORD>(Len));

      UnicodeString NewPath = ::FileSearch(base::ExtractFileName(APath, true), Paths);
      Result = !NewPath.IsEmpty();
      if (Result)
      {
        APath = NewPath;
      }
    }
  }
  return Result;
}

bool FileExistsEx(const UnicodeString & APath)
{
  UnicodeString LocalPath = APath;
  return FindFile(LocalPath);
}

void OpenSessionInPutty(const UnicodeString & PuttyPath,
  TSessionData * SessionData)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(PuttyPath, Program, Params, Dir);
  Program = ::ExpandEnvironmentVariables(Program);
  if (FindFile(Program))
  {
    Params = ::ExpandEnvironmentVariables(Params);
    UnicodeString Password = GetGUIConfiguration()->GetPuttyPassword() ? SessionData->GetPassword() : UnicodeString();
    UnicodeString Psw = Password;
    UnicodeString SessionName;
    std::unique_ptr<TRegistryStorage> Storage(new TRegistryStorage(GetConfiguration()->GetPuttySessionsKey()));
    Storage->SetAccessMode(smReadWrite);
    // make it compatible with putty
    Storage->SetMungeStringValues(false);
    Storage->SetForceAnsi(true);
    if (Storage->OpenRootKey(true))
    {
      if (Storage->KeyExists(SessionData->GetStorageKey()))
      {
        SessionName = SessionData->GetSessionName();
      }
      else
      {
        std::unique_ptr<TRegistryStorage> SourceStorage(new TRegistryStorage(GetConfiguration()->GetPuttySessionsKey()));
        SourceStorage->SetMungeStringValues(false);
        SourceStorage->SetForceAnsi(true);
        if (SourceStorage->OpenSubKey(StoredSessions->GetDefaultSettings()->GetName(), false) &&
            Storage->OpenSubKey(GetGUIConfiguration()->GetPuttySession(), true))
        {
          Storage->Copy(SourceStorage.get());
          Storage->CloseSubKey();
        }

        std::unique_ptr<TSessionData> ExportData(new TSessionData(L""));
        ExportData->Assign(SessionData);
        ExportData->SetModified(true);
        ExportData->SetName(GetGUIConfiguration()->GetPuttySession());
        ExportData->SetPassword(L"");

        if (SessionData->GetFSProtocol() == fsFTP)
        {
          if (GetGUIConfiguration()->GetTelnetForFtpInPutty())
          {
            ExportData->SetPuttyProtocol(PuttyTelnetProtocol);
            ExportData->SetPortNumber(TelnetPortNumber);
            // PuTTY  does not allow -pw for telnet
            Psw.Clear();
          }
          else
          {
            ExportData->SetPuttyProtocol(PuttySshProtocol);
            ExportData->SetPortNumber(SshPortNumber);
          }
        }

        ExportData->Save(Storage.get(), true);
        SessionName = GetGUIConfiguration()->GetPuttySession();
      }
    }

    if (!Params.IsEmpty())
    {
      Params += L" ";
    }
    if (!Psw.IsEmpty())
    {
      Params += FORMAT(L"-pw %s ", EscapePuttyCommandParam(Psw).c_str());
    }
    //Params += FORMAT(L"-load %s", EscapePuttyCommandParam(SessionName).c_str());
    Params += FORMAT(L"-l %s ", EscapePuttyCommandParam(SessionData->GetUserNameExpanded()).c_str());
    Params += FORMAT(L"-P %d ", SessionData->GetPortNumber());
    Params += FORMAT(L"%s ", EscapePuttyCommandParam(SessionData->GetHostNameExpanded()).c_str());

    if (!ExecuteShell(Program, Params))
    {
      throw Exception(FMTLOAD(EXECUTE_APP_ERROR, Program.c_str()));
    }
  }
  else
  {
    throw Exception(FMTLOAD(FILE_NOT_FOUND, Program.c_str()));
  }
}

bool FindTool(const UnicodeString & Name, UnicodeString & APath)
{
  UnicodeString AppPath = ::IncludeTrailingBackslash(::ExtractFilePath(GetConfiguration()->ModuleFileName()));
  APath = AppPath + Name;
  bool Result = true;
  if (!::FileExists(APath))
  {
    APath = AppPath + "PuTTY\\" + Name;
    if (!::FileExists(APath))
    {
      APath = Name;
      if (!FindFile(APath))
      {
        Result = false;
      }
    }
  }
  return Result;
}

bool ExecuteShell(const UnicodeString & APath, const UnicodeString & Params)
{
#ifndef __linux__
  return ((intptr_t)::ShellExecute(nullptr, L"open", const_cast<wchar_t *>(APath.data()),
    const_cast<wchar_t *>(Params.data()), nullptr, SW_SHOWNORMAL) > 32);
#else
  return false;
#endif
}

bool ExecuteShell(const UnicodeString & APath, const UnicodeString & Params,
  HANDLE & Handle)
{
#ifndef __linux__
  TShellExecuteInfoW ExecuteInfo;
  ClearStruct(ExecuteInfo);
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(APath.data());
  ExecuteInfo.lpParameters = const_cast<wchar_t *>(Params.data());
  ExecuteInfo.nShow = SW_SHOW;

  bool Result = (::ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    Handle = ExecuteInfo.hProcess;
  }
  return Result;
#else
  return false;
#endif
}

bool ExecuteShellAndWait(HINSTANCE /*Handle*/, const UnicodeString & APath,
  const UnicodeString & Params, TProcessMessagesEvent ProcessMessages)
{
#ifndef __linux__
  TShellExecuteInfoW ExecuteInfo;
  ClearStruct(ExecuteInfo);
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(APath.data());
  ExecuteInfo.lpParameters = const_cast<wchar_t *>(Params.data());
  ExecuteInfo.nShow = SW_SHOW;

  bool Result = (::ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    if (ProcessMessages != nullptr)
    {
      uint32_t WaitResult;
      do
      {
        WaitResult = ::WaitForSingleObject(ExecuteInfo.hProcess, 200);
        if (WaitResult == WAIT_FAILED)
        {
          throw Exception(LoadStr(DOCUMENT_WAIT_ERROR));
        }
        ProcessMessages();
      }
      while (WaitResult == WAIT_TIMEOUT);
    }
    else
    {
      ::WaitForSingleObject(ExecuteInfo.hProcess, INFINITE);
    }
  }
  return Result;
#else
  return false;
#endif
}

bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString & Command,
  TProcessMessagesEvent ProcessMessages)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  return ExecuteShellAndWait(Handle, Program, Params, ProcessMessages);
}

bool SpecialFolderLocation(int PathID, UnicodeString & APath)
{
#ifndef __linux__
  LPITEMIDLIST Pidl;
  wchar_t Buf[MAX_PATH];
  if (::SHGetSpecialFolderLocation(nullptr, PathID, &Pidl) == NO_ERROR &&
      ::SHGetPathFromIDList(Pidl, Buf))
  {
    APath = UnicodeString(Buf);
    return true;
  }
#endif
  return false;
}

UnicodeString GetPersonalFolder()
{
  UnicodeString Result;
  SpecialFolderLocation(CSIDL_PERSONAL, Result);

  if (IsWine())
  {
    UnicodeString WineHostHome;
    int Len1 = ::GetEnvironmentVariable(L"WINE_HOST_HOME", nullptr, 0);
    if (Len1 > 0)
    {
      WineHostHome.SetLength(Len1 - 1);
      ::GetEnvironmentVariable(L"WINE_HOST_HOME", const_cast<LPWSTR>(WineHostHome.c_str()), Len1);
    }
    if (!WineHostHome.IsEmpty())
    {
      UnicodeString WineHome = L"Z:" + core::ToUnixPath(WineHostHome);
      if (::DirectoryExists(WineHome))
      {
        Result = WineHome;
      }
    }
    else
    {
      // Should we use WinAPI GetUserName() instead?
      UnicodeString UserName;
      int Len2 = ::GetEnvironmentVariable(L"USERNAME", nullptr, 0);
      if (Len2 > 0)
      {
        UserName.SetLength(Len2 - 1);
        ::GetEnvironmentVariable(L"USERNAME", const_cast<LPWSTR>(UserName.c_str()), Len2);
      }
      if (!UserName.IsEmpty())
      {
        UnicodeString WineHome = L"Z:\\home\\" + UserName;
        if (::DirectoryExists(WineHome))
        {
          Result = WineHome;
        }
      }
    }
  }
  return Result;
}

UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, intptr_t Count, const UnicodeString & FirstItem)
{
  UnicodeString Result;
  if (Count == 1)
  {
    Result = FORMAT(SingleItemFormat.c_str(), FirstItem.c_str());
  }
  else
  {
    Result = FORMAT(MultiItemsFormat.c_str(), Count);
  }
  return Result;
}

UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, const TStrings * Items)
{
  return ItemsFormatString(SingleItemFormat, MultiItemsFormat,
    Items->GetCount(), (Items->GetCount() > 0 ? Items->GetString(0) : UnicodeString()));
}

UnicodeString FileNameFormatString(const UnicodeString & SingleFileFormat,
  const UnicodeString & MultiFilesFormat, const TStrings * AFiles, bool Remote)
{
  DebugAssert(AFiles != nullptr);
  UnicodeString Item;
  if (AFiles->GetCount() > 0)
  {
    Item = Remote ? base::UnixExtractFileName(AFiles->GetString(0)) :
      base::ExtractFileName(AFiles->GetString(0), true);
  }
  return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
    AFiles->GetCount(), Item);
}

UnicodeString UniqTempDir(const UnicodeString & BaseDir, const UnicodeString & Identity,
  bool Mask)
{
  UnicodeString TempDir;
  do
  {
    TempDir = BaseDir.IsEmpty() ? GetSystemTemporaryDirectory() : BaseDir;
    TempDir = ::IncludeTrailingBackslash(TempDir) + Identity;
    if (Mask)
    {
      TempDir += L"?????";
    }
    else
    {
#if defined(__BORLANDC__)
      TempDir += ::IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
#else
      TDateTime dt = Now();
      uint16_t H, M, S, MS;
      dt.DecodeTime(H, M, S, MS);
      TempDir += ::IncludeTrailingBackslash(FORMAT(L"%02d%03d", M, MS));
#endif
    }
  }
  while (!Mask && ::DirectoryExists(TempDir));

  return TempDir;
}

bool DeleteDirectory(const UnicodeString & ADirName)
{
  TSearchRecChecked SearchRec;
  bool retval = true;
  if (::FindFirst(ADirName + L"\\*", faAnyFile, SearchRec) == 0) // VCL Function
  {
    if (FLAGSET(SearchRec.Attr, faDirectory))
    {
      if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
        retval = DeleteDirectory(ADirName + L"\\" + SearchRec.Name);
    }
    else
    {
      retval = ::RemoveFile(ADirName + L"\\" + SearchRec.Name);
    }

    if (retval)
    {
      while (FindNextChecked(SearchRec) == 0)
      { // VCL Function
        if (FLAGSET(SearchRec.Attr, faDirectory))
        {
          if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
            retval = DeleteDirectory(ADirName + L"\\" + SearchRec.Name);
        }
        else
        {
          retval = ::RemoveFile(ADirName + L"\\" + SearchRec.Name);
        }

        if (!retval)
        {
          break;
        }
      }
    }
  }
  FindClose(SearchRec);
  if (retval)
  {
    retval = ::RemoveDir(ADirName); // VCL function
  }
  return retval;
}

UnicodeString FormatDateTimeSpan(const UnicodeString & /*TimeFormat*/, const TDateTime & DateTime)
{
  UnicodeString Result;
  if (static_cast<int>(DateTime) > 0)
  {
    Result = ::IntToStr(static_cast<intptr_t>((double)DateTime)) + L", ";
  }
  // days are decremented, because when there are to many of them,
  // "integer overflow" error occurs
#if defined(__BORLANDC__)
  Result += FormatDateTime(TimeFormat, DateTime - int(DateTime));
#else
  TDateTime dt(DateTime - static_cast<int>(DateTime));
  uint16_t H, M, S, MS;
  dt.DecodeTime(H, M, S, MS);
  Result += FORMAT(L"%02d:%02d:%02d", H, M, S);
#endif
  return Result;
}

TLocalCustomCommand::TLocalCustomCommand()
{
}

TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
  const UnicodeString & RemotePath, const UnicodeString & LocalPath) :
  TFileCustomCommand(Data, RemotePath)
{
  FLocalPath = LocalPath;
}

TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
  const UnicodeString & RemotePath, const UnicodeString & LocalPath,
  const UnicodeString & AFileName, const UnicodeString & LocalFileName, const UnicodeString & FileList) :
  TFileCustomCommand(Data, RemotePath, AFileName, FileList)
{
  FLocalPath = LocalPath;
  FLocalFileName = LocalFileName;
}

intptr_t TLocalCustomCommand::PatternLen(const UnicodeString & Command, intptr_t Index) const
{
  intptr_t Len = 0;
  if (Command[Index + 1] == L'^')
  {
    Len = 3;
  }
  else
  {
    Len = TFileCustomCommand::PatternLen(Command, Index);
  }
  return Len;
}

bool TLocalCustomCommand::PatternReplacement(
  intptr_t Index, const UnicodeString & Pattern, UnicodeString & Replacement, bool & Delimit) const
{
  bool Result = false;
  if (Pattern == L"!\\")
  {
    // When used as "!\" in an argument to PowerShell, the trailing \ would escpae the ",
    // so we exclude it
    Replacement = ::ExcludeTrailingBackslash(FLocalPath);
    Result = true;
  }
  else if (Pattern == L"!^!")
  {
    Replacement = FLocalFileName;
    Result = true;
  }
  else
  {
    Result = TFileCustomCommand::PatternReplacement(Index, Pattern, Replacement, Delimit);
  }
  return Result;
}

void TLocalCustomCommand::DelimitReplacement(
  UnicodeString & /*Replacement*/, wchar_t /*Quote*/)
{
  // never delimit local commands
}

bool TLocalCustomCommand::HasLocalFileName(const UnicodeString & Command) const
{
  return FindPattern(Command, L'^');
}

bool TLocalCustomCommand::IsFileCommand(const UnicodeString & Command) const
{
  return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}

