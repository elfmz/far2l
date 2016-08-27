#include <vcl.h>
#pragma hdrstop
#include <LanguagesDEPfix.hpp>
#include "GUIConfiguration.h"
#include "GUITools.h"
#include <Common.h>
#include <FileInfo.h>
#include <TextsCore.h>
#include <Terminal.h>
#include <CoreMain.h>
#ifndef __linux__
#include <shlobj.h>
#endif

const intptr_t ccLocal = ccUser;
const intptr_t ccShowResults = ccUser << 1;
const intptr_t ccCopyResults = ccUser << 2;
const intptr_t ccSet = 0x80000000;

static const uintptr_t AdditionaLanguageMask = 0xFFFFFF00;
#define ADDITIONAL_LANGUAGE_PREFIX L"XX"

TGUICopyParamType::TGUICopyParamType()
  : TCopyParamType()
{
  GUIDefault();
}

TGUICopyParamType::TGUICopyParamType(const TCopyParamType & Source)
  : TCopyParamType(Source)
{
  GUIDefault();
}

TGUICopyParamType::TGUICopyParamType(const TGUICopyParamType & Source)
  : TCopyParamType(Source)
{
  GUIAssign(&Source);
}

void TGUICopyParamType::Assign(const TCopyParamType * Source)
{
  TCopyParamType::Assign(Source);

  const TGUICopyParamType * GUISource;
  GUISource = NB_STATIC_DOWNCAST_CONST(TGUICopyParamType, Source);
  if (GUISource != nullptr)
  {
    GUIAssign(GUISource);
  }
}

void TGUICopyParamType::GUIAssign(const TGUICopyParamType * Source)
{
  SetQueue(Source->GetQueue());
  SetQueueNoConfirmation(Source->GetQueueNoConfirmation());
  SetQueueIndividually(Source->GetQueueIndividually());
}

void TGUICopyParamType::Default()
{
  GUIDefault();
}

void TGUICopyParamType::GUIDefault()
{
  TCopyParamType::Default();

  SetQueue(false);
  SetQueueNoConfirmation(true);
  SetQueueIndividually(false);
}

void TGUICopyParamType::Load(THierarchicalStorage * Storage)
{
  TCopyParamType::Load(Storage);

  SetQueue(Storage->ReadBool("Queue", GetQueue()));
  SetQueueNoConfirmation(Storage->ReadBool("QueueNoConfirmation", GetQueueNoConfirmation()));
  SetQueueIndividually(Storage->ReadBool("QueueIndividually", GetQueueIndividually()));
}

void TGUICopyParamType::Save(THierarchicalStorage * Storage)
{
  TCopyParamType::Save(Storage);

  Storage->WriteBool("Queue", GetQueue());
  Storage->WriteBool("QueueNoConfirmation", GetQueueNoConfirmation());
  Storage->WriteBool("QueueIndividually", GetQueueIndividually());
}

TGUICopyParamType & TGUICopyParamType::operator =(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}

TGUICopyParamType & TGUICopyParamType::operator =(const TGUICopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}

void TCopyParamRuleData::Default()
{
  HostName.Clear();
  UserName.Clear();
  RemoteDirectory.Clear();
  LocalDirectory.Clear();
}

TCopyParamRule::TCopyParamRule()
{
}

TCopyParamRule::TCopyParamRule(const TCopyParamRuleData & Data) :
  FData(Data)
{
}

TCopyParamRule::TCopyParamRule(const TCopyParamRule & Source)
{
  FData.HostName = Source.FData.HostName;
  FData.UserName = Source.FData.UserName;
  FData.RemoteDirectory = Source.FData.RemoteDirectory;
  FData.LocalDirectory = Source.FData.LocalDirectory;
}

#define C(Property) (Property == rhp.Property)
bool TCopyParamRule::operator==(const TCopyParamRule & rhp) const
{
  return
    C(FData.HostName) &&
    C(FData.UserName) &&
    C(FData.RemoteDirectory) &&
    C(FData.LocalDirectory) &&
    true;
}
#undef C

bool TCopyParamRule::Match(const UnicodeString & Mask,
  const UnicodeString & Value, bool Path, bool Local) const
{
  bool Result;
  if (Mask.IsEmpty())
  {
    Result = true;
  }
  else
  {
    TFileMasks M(Mask);
    if (Path)
    {
      Result = M.Matches(Value, Local, true);
    }
    else
    {
      Result = M.Matches(Value, false);
    }
  }
  return Result;
}

bool TCopyParamRule::Matches(const TCopyParamRuleData & Value) const
{
  return
    Match(FData.HostName, Value.HostName, false) &&
    Match(FData.UserName, Value.UserName, false) &&
    Match(FData.RemoteDirectory, Value.RemoteDirectory, true, false) &&
    Match(FData.LocalDirectory, Value.LocalDirectory, true, true);
}

void TCopyParamRule::Load(THierarchicalStorage * Storage)
{
  FData.HostName = Storage->ReadString("HostName", FData.HostName);
  FData.UserName = Storage->ReadString("UserName", FData.UserName);
  FData.RemoteDirectory = Storage->ReadString("RemoteDirectory", FData.RemoteDirectory);
  FData.LocalDirectory = Storage->ReadString("LocalDirectory", FData.LocalDirectory);
}

void TCopyParamRule::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteString("HostName", FData.HostName);
  Storage->WriteString("UserName", FData.UserName);
  Storage->WriteString("RemoteDirectory", FData.RemoteDirectory);
  Storage->WriteString("LocalDirectory", FData.LocalDirectory);
}

bool TCopyParamRule::GetEmpty() const
{
  return
    FData.HostName.IsEmpty() &&
    FData.UserName.IsEmpty() &&
    FData.RemoteDirectory.IsEmpty() &&
    FData.LocalDirectory.IsEmpty();
}

TCopyParamRule & TCopyParamRule::operator=(const TCopyParamRule & other)
{
  SetData(other.FData);
  return *this;
}

UnicodeString TCopyParamRule::GetInfoStr(const UnicodeString & Separator) const
{
  UnicodeString Result;
  #define ADD(FMT, ELEM) \
    if (!FData.ELEM.IsEmpty()) \
      Result += (Result.IsEmpty() ? UnicodeString() : Separator) + FMTLOAD(FMT, FData.ELEM.c_str());
  ADD(COPY_RULE_HOSTNAME, HostName);
  ADD(COPY_RULE_USERNAME, UserName);
  ADD(COPY_RULE_REMOTE_DIR, RemoteDirectory);
  ADD(COPY_RULE_LOCAL_DIR, LocalDirectory);
  #undef ADD
  return Result;
}

void TCopyParamRule::SetData(const TCopyParamRuleData & Value)
{
  FData = Value;
}

TCopyParamList::TCopyParamList() :
  FRules(new TList()),
  FCopyParams(new TList()),
  FNames(new TStringList()),
  FNameList(nullptr),
  FModified(false)
{
}

TCopyParamList::TCopyParamList(const TCopyParamList & other) :
  FRules(new TList()),
  FCopyParams(new TList()),
  FNames(new TStringList()),
  FNameList(nullptr),
  FModified(false)
{
  this->operator=(other);
}

TCopyParamList::~TCopyParamList()
{
  Clear();
  SAFE_DESTROY(FCopyParams);
  SAFE_DESTROY(FRules);
  SAFE_DESTROY(FNames);
  SAFE_DESTROY(FNameList);
}

void TCopyParamList::Reset()
{
  SAFE_DESTROY(FNameList);
  FModified = false;
}

void TCopyParamList::Modify()
{
  SAFE_DESTROY(FNameList);
  FModified = true;
}

void TCopyParamList::ValidateName(const UnicodeString & Name)
{
  if (Name.LastDelimiter(CONST_INVALID_CHARS) > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), CONST_INVALID_CHARS));
  }
}

TCopyParamList & TCopyParamList::operator=(const TCopyParamList & rhl)
{
  Clear();

  for (intptr_t Index = 0; Index < rhl.GetCount(); ++Index)
  {
    TCopyParamType * CopyParam = new TCopyParamType(*rhl.GetCopyParam(Index));
    TCopyParamRule * Rule = nullptr;
    if (rhl.GetRule(Index) != nullptr)
    {
      Rule = new TCopyParamRule(*rhl.GetRule(Index));
    }
    Add(rhl.GetName(Index), CopyParam, Rule);
  }
  // there should be comparison of with the assigned list, but we rely on caller
  // to do it instead (TGUIConfiguration::SetCopyParamList)
  Modify();
  return *this;
}

bool TCopyParamList::operator==(const TCopyParamList & rhl) const
{
  bool Result = (GetCount() == rhl.GetCount());
  if (Result)
  {
    intptr_t Index = 0;
    while ((Index < GetCount()) && Result)
    {
      Result = (GetName(Index) == rhl.GetName(Index)) &&
        CompareItem(Index, rhl.GetCopyParam(Index), rhl.GetRule(Index));
      ++Index;
    }
  }
  return Result;
}

intptr_t TCopyParamList::IndexOfName(const UnicodeString & Name) const
{
  return FNames->IndexOf(Name.c_str());
}

bool TCopyParamList::CompareItem(intptr_t Index,
  const TCopyParamType * CopyParam, const TCopyParamRule * Rule) const
{
  return
    ((*GetCopyParam(Index)) == *CopyParam) &&
    ((GetRule(Index) == nullptr) ?
      (Rule == nullptr) :
      ((Rule != nullptr) && (*GetRule(Index)) == (*Rule)));
}

void TCopyParamList::Clear()
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    delete GetCopyParam(Index);
    delete GetRule(Index);
  }
  FCopyParams->Clear();
  FRules->Clear();
  FNames->Clear();
}

void TCopyParamList::Add(const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  Insert(GetCount(), Name, CopyParam, Rule);
}

void TCopyParamList::Insert(intptr_t Index, const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  DebugAssert(FNames->IndexOf(Name) < 0);
  FNames->Insert(Index, Name);
  DebugAssert(CopyParam != nullptr);
  FCopyParams->Insert(Index, CopyParam);
  FRules->Insert(Index, Rule);
  Modify();
}

void TCopyParamList::Change(intptr_t Index, const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  if ((Name != GetName(Index)) || !CompareItem(Index, CopyParam, Rule))
  {
    FNames->SetString(Index, Name);
    delete GetCopyParam(Index);
    FCopyParams->SetItem(Index, CopyParam);
    delete GetRule(Index);
    FRules->SetItem(Index, Rule);
    Modify();
  }
  else
  {
    SAFE_DESTROY(CopyParam);
    SAFE_DESTROY(Rule);
  }
}

void TCopyParamList::Move(intptr_t CurIndex, intptr_t NewIndex)
{
  if (CurIndex != NewIndex)
  {
    FNames->Move(CurIndex, NewIndex);
    FCopyParams->Move(CurIndex, NewIndex);
    FRules->Move(CurIndex, NewIndex);
    Modify();
  }
}

void TCopyParamList::Delete(intptr_t Index)
{
  DebugAssert((Index >= 0) && (Index < GetCount()));
  FNames->Delete(Index);
  delete GetCopyParam(Index);
  FCopyParams->Delete(Index);
  delete GetRule(Index);
  FRules->Delete(Index);
  Modify();
}

intptr_t TCopyParamList::Find(const TCopyParamRuleData & Value) const
{
  intptr_t Result = -1;
  intptr_t Index = 0;
  while ((Index < FRules->GetCount()) && (Result < 0))
  {
    if (FRules->GetItem(Index) != nullptr)
    {
      if (GetRule(Index)->Matches(Value))
      {
        Result = Index;
      }
    }
    ++Index;
  }
  return Result;
}

void TCopyParamList::Load(THierarchicalStorage * Storage, intptr_t ACount)
{
  for (intptr_t Index = 0; Index < ACount; ++Index)
  {
    UnicodeString Name = ::IntToStr(Index);
    std::unique_ptr<TCopyParamRule> Rule;
    std::unique_ptr<TCopyParamType> CopyParam(new TCopyParamType());
    if (Storage->OpenSubKey(Name, false))
    {
      SCOPE_EXIT
      {
        Storage->CloseSubKey();
      };
      Name = Storage->ReadString("Name", Name);
      CopyParam->Load(Storage);

      if (Storage->ReadBool("HasRule", false))
      {
        Rule.reset(new TCopyParamRule());
        Rule->Load(Storage);
      }
    }

    FCopyParams->Add(CopyParam.release());
    FRules->Add(Rule.release());
    FNames->Add(Name);
  }
  Reset();
}

void TCopyParamList::Save(THierarchicalStorage * Storage) const
{
  Storage->ClearSubKeys();
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Storage->OpenSubKey(::IntToStr(Index), true))
    {
      SCOPE_EXIT
      {
        Storage->CloseSubKey();
      };
      const TCopyParamType * CopyParam = GetCopyParam(Index);
      const TCopyParamRule * Rule = GetRule(Index);

      Storage->WriteString("Name", GetName(Index));
      CopyParam->Save(Storage);
      Storage->WriteBool("HasRule", (Rule != nullptr));
      if (Rule != nullptr)
      {
        Rule->Save(Storage);
      }
    }
  }
}

const TCopyParamRule * TCopyParamList::GetRule(intptr_t Index) const
{
  return NB_STATIC_DOWNCAST(TCopyParamRule, FRules->GetItem(Index));
}

const TCopyParamType * TCopyParamList::GetCopyParam(intptr_t Index) const
{
  return NB_STATIC_DOWNCAST(TCopyParamType, FCopyParams->GetItem(Index));
}

UnicodeString TCopyParamList::GetName(intptr_t Index) const
{
  return FNames->GetString(Index);
}

TStrings * TCopyParamList::GetNameList() const
{
  if (FNameList == nullptr)
  {
    FNameList = new TStringList();

    for (intptr_t Index = 0; Index < GetCount(); ++Index)
    {
      FNameList->Add(FNames->GetString(Index));
    }
  }
  return FNameList;
}

bool TCopyParamList::GetAnyRule() const
{
  bool Result = false;
  intptr_t Index = 0;
  while ((Index < GetCount()) && !Result)
  {
    Result = (GetRule(Index) != nullptr);
    ++Index;
  }
  return Result;
}

TGUIConfiguration::TGUIConfiguration() : TConfiguration(),
  FLocale(0),
  FLocales(CreateSortedStringList()),
  FLastLocalesExts(L"*"),
  FContinueOnError(false),
  FConfirmCommandSession(false),
  FPuttyPassword(false),
  FTelnetForFtpInPutty(false),
  FSynchronizeParams(0),
  FSynchronizeOptions(0),
  FSynchronizeModeAuto(0),
  FSynchronizeMode(0),
  FMaxWatchDirectories(0),
  FQueueAutoPopup(false),
  FSessionRememberPassword(false),
  FQueueTransfersLimit(0),
  FQueueKeepDoneItems(false),
  FQueueKeepDoneItemsFor(0),
  FBeepOnFinish(false),
  FCopyParamList(new TCopyParamList()),
  FCopyParamListDefaults(false),
  FKeepUpToDateChangeDelay(0),
  FSessionReopenAutoIdle(0)
{
  CoreSetResourceModule(0);
}

TGUIConfiguration::~TGUIConfiguration()
{
  SAFE_DESTROY(FLocales);
  SAFE_DESTROY(FCopyParamList);
}

void TGUIConfiguration::Default()
{
  TConfiguration::Default();

  // reset before call to DefaultLocalized()
  FDefaultCopyParam.Default();

  FCopyParamListDefaults = true;
  DefaultLocalized();

  FIgnoreCancelBeforeFinish = TDateTime(0, 0, 3, 0);
  FContinueOnError = false;
  FConfirmCommandSession = true;
  FSynchronizeParams = TTerminal::spNoConfirmation | TTerminal::spPreviewChanges;
  FSynchronizeModeAuto = -1;
  FSynchronizeMode = TTerminal::smRemote;
  FMaxWatchDirectories = 500;
  FSynchronizeOptions = soRecurse | soSynchronizeAsk;
  FQueueTransfersLimit = 2;
  FQueueKeepDoneItems = true;
  FQueueKeepDoneItemsFor = 15;
  FQueueAutoPopup = true;
  FSessionRememberPassword = false;
  UnicodeString ProgramsFolder;
  SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
  FDefaultPuttyPathOnly = ::IncludeTrailingBackslash(ProgramsFolder) + "PuTTY" WGOOD_SLASH "putty.exe";
  FDefaultPuttyPath = FormatCommand("%PROGRAMFILES%" WGOOD_SLASH "PuTTY" WGOOD_SLASH "putty.exe", L"");
  FPuttyPath = FDefaultPuttyPath;
  SetPSftpPath(FormatCommand("%PROGRAMFILES%" WGOOD_SLASH "PuTTY" WGOOD_SLASH "psftp.exe", L""));
  FPuttyPassword = false;
  FTelnetForFtpInPutty = true;
  FPuttySession = "WinSCP temporary session";
  FBeepOnFinish = false;
  FBeepOnFinishAfter = TDateTime(0, 0, 30, 0);
  FCopyParamCurrent.Clear();
  FKeepUpToDateChangeDelay = 500;
  FChecksumAlg = "md5";
  FSessionReopenAutoIdle = 9000;

  FNewDirectoryProperties.Default();
  FNewDirectoryProperties.Rights = TRights::rfDefault | TRights::rfExec;
}

void TGUIConfiguration::DefaultLocalized()
{
  if (FCopyParamListDefaults)
  {
    FCopyParamList->Clear();

    // guard against "empty resource string" from obsolete translations
    // (DefaultLocalized is called for the first time before detection of
    // obsolete translations)
    if (!LoadStr(COPY_PARAM_PRESET_ASCII).IsEmpty())
    {
      TCopyParamType * CopyParam;

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmAscii);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_ASCII), CopyParam, nullptr);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmBinary);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_BINARY), CopyParam, nullptr);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->GetIncludeFileMask().SetMasks("|*.bak; *.tmp; ~$*; *.wbk; *~; #*; .#*");
      CopyParam->SetNewerOnly(true);
      FCopyParamList->Add(LoadStr(COPY_PARAM_NEWER_ONLY), CopyParam, nullptr);
    }

    FCopyParamList->Reset();
  }
}

void TGUIConfiguration::UpdateStaticUsage()
{
  // TConfiguration::UpdateStaticUsage();
  // Usage->Set(L"CopyParamsCount", (FCopyParamListDefaults ? 0 : FCopyParamList->Count));
  // Usage->Set(L"CopyParamsCount", (FCopyParamListDefaults ? 0 : FCopyParamList->GetCount()));
}

UnicodeString TGUIConfiguration::PropertyToKey(const UnicodeString & Property)
{
  // no longer useful
  intptr_t P = Property.LastDelimiter(L".>");
  return Property.SubString(P + 1, Property.Length() - P);
}

// duplicated from core\configuration.cpp
#undef BLOCK
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
    { SCOPE_EXIT { Storage->CloseSubKey(); }; { BLOCK } }
#undef REGCONFIG
#define REGCONFIG(CANCREATE) \
  BLOCK(L"Interface", CANCREATE, \
    KEY(Bool,     ContinueOnError); \
    KEY(Bool,     ConfirmCommandSession); \
    KEY(Integer,  SynchronizeParams); \
    KEY(Integer,  SynchronizeOptions); \
    KEY(Integer,  SynchronizeModeAuto); \
    KEY(Integer,  SynchronizeMode); \
    KEY(Integer,  MaxWatchDirectories); \
    KEY(Integer,  QueueTransfersLimit); \
    KEY(Integer,  QueueKeepDoneItems); \
    KEY(Integer,  QueueKeepDoneItemsFor); \
    KEY(Bool,     QueueAutoPopup); \
    KEYEX(Bool,   QueueRememberPassword, SessionRememberPassword); \
    KEY(String,   PuttySession); \
    KEY(String,   PuttyPath); \
    KEY(Bool,     PuttyPassword); \
    KEY(Bool,     TelnetForFtpInPutty); \
    KEY(DateTime, IgnoreCancelBeforeFinish); \
    KEY(Bool,     BeepOnFinish); \
    KEY(DateTime, BeepOnFinishAfter); \
    KEY(Integer,  KeepUpToDateChangeDelay); \
    KEY(String,   ChecksumAlg); \
    KEY(Integer,  SessionReopenAutoIdle); \
  ); \

void TGUIConfiguration::SaveData(THierarchicalStorage * Storage, bool All)
{
  TConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
#ifndef LASTELEM
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>")+1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#endif
  #undef KEYEX
  #define KEYEX(TYPE, NAME, VAR) Storage->Write ## TYPE(LASTELEM(UnicodeString(MB_TEXT(#NAME))), Get ## VAR())
  #undef KEY
  #define KEY(TYPE, NAME) Storage->Write ## TYPE(PropertyToKey(MB_TEXT(#NAME)), Get ## NAME())
  REGCONFIG(true);
  #undef KEY
  #undef KEYEX

  if (Storage->OpenSubKey(L"Interface" WGOOD_SLASH "CopyParam", true, true))
  {
    SCOPE_EXIT
    {
      Storage->CloseSubKey();
    };
    FDefaultCopyParam.Save(Storage);

    if (FCopyParamListDefaults)
    {
      DebugAssert(!FCopyParamList->GetModified());
      Storage->WriteInteger("CopyParamList", -1);
    }
    else if (All || FCopyParamList->GetModified())
    {
      Storage->WriteInteger("CopyParamList", FCopyParamList->GetCount());
      FCopyParamList->Save(Storage);
    }
  }

  if (Storage->OpenSubKey(L"Interface" WGOOD_SLASH "NewDirectory", true, true))
  {
    SCOPE_EXIT
    {
      Storage->CloseSubKey();
    };
    FNewDirectoryProperties.Save(Storage);
  }
}

void TGUIConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
  #undef KEYEX
  #define KEYEX(TYPE, NAME, VAR) Set ## VAR(Storage->Read ## TYPE(LASTELEM(UnicodeString(MB_TEXT(#NAME))), Get ## VAR()))
  #undef KEY
  #define KEY(TYPE, NAME) Set ## NAME(Storage->Read ## TYPE(PropertyToKey(MB_TEXT(#NAME)), Get ## NAME()))
  REGCONFIG(false);
  #undef KEY
  #undef KEYEX

  if (Storage->OpenSubKey(L"Interface" WGOOD_SLASH "CopyParam", false, true))
  {
    SCOPE_EXIT
    {
      Storage->CloseSubKey();
    };
    // must be loaded before eventual setting defaults for CopyParamList
    FDefaultCopyParam.Load(Storage);

    intptr_t CopyParamListCount = Storage->ReadInteger("CopyParamList", -1);
    FCopyParamListDefaults = ((int)CopyParamListCount <= 0);
    if (!FCopyParamListDefaults)
    {
      FCopyParamList->Clear();
      FCopyParamList->Load(Storage, CopyParamListCount);
    }
    else if (FCopyParamList->GetModified())
    {
      FCopyParamList->Clear();
      FCopyParamListDefaults = false;
    }
    FCopyParamList->Reset();
  }

  // Make it compatible with versions prior to 3.7.1 that have not saved PuttyPath
  // with quotes. First check for absence of quotes.
  // Add quotes either if the path is set to default putty path (even if it does
  // not exists) or when the path points to existing file (so there are no parameters
  // yet in the string). Note that FileExists may display error dialog, but as
  // it should be called only for custom users path, let's expect that the user
  // can take care of it.
  if ((FPuttyPath.SubString(1, 1) != L"\"") &&
      (CompareFileName(::ExpandEnvironmentVariables(FPuttyPath), FDefaultPuttyPathOnly) ||
       ::FileExists(::ExpandEnvironmentVariables(FPuttyPath))))
  {
    FPuttyPath = FormatCommand(FPuttyPath, L"");
  }

  if (Storage->OpenSubKey(L"Interface" WGOOD_SLASH "NewDirectory", false, true))
  {
    SCOPE_EXIT
    {
      Storage->CloseSubKey();
    };
    FNewDirectoryProperties.Load(Storage);
  }
}

void TGUIConfiguration::Saved()
{
  TConfiguration::Saved();

  FCopyParamList->Reset();
}

HINSTANCE TGUIConfiguration::LoadNewResourceModule(LCID ALocale,
  UnicodeString * AFileName)
{
  HINSTANCE NewInstance = 0;
#ifndef __linux__
  UnicodeString LibraryFileName;
  bool Internal = (ALocale == InternalLocale());
  if (!Internal)
  {
    UnicodeString Module;
    UnicodeString LocaleName;

    Module = ModuleFileName();
    if ((ALocale & AdditionaLanguageMask) != AdditionaLanguageMask)
    {
      LOCALESIGNATURE LocSig;
      GetLocaleInfo(ALocale, LOCALE_SABBREVLANGNAME, reinterpret_cast<LPWSTR>(&LocSig), sizeof(LocSig) / sizeof(TCHAR));
      LocaleName = *reinterpret_cast<LPWSTR>(&LocSig);
      DebugAssert(!LocaleName.IsEmpty());
    }
    else
    {
      LocaleName = UnicodeString(ADDITIONAL_LANGUAGE_PREFIX) +
        static_cast<wchar_t>(ALocale & ~AdditionaLanguageMask);
    }

    Module = ::ChangeFileExt(Module, UnicodeString(L".") + LocaleName);
    // Look for a potential language/country translation
    NewInstance = ::LoadLibraryEx(Module.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
    if (!NewInstance)
    {
      // Finally look for a language only translation
      Module.SetLength(Module.Length() - 1);
      NewInstance = ::LoadLibraryEx(Module.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
      if (NewInstance)
      {
        LibraryFileName = Module;
      }
    }
    else
    {
      LibraryFileName = Module;
    }
  }

  if (!NewInstance && !Internal)
  {
    throw Exception(FMTLOAD(LOCALE_LOAD_ERROR, static_cast<int>(ALocale)));
  }
  else
  {
    if (Internal)
    {
      ThrowNotImplemented(90);
      NewInstance = 0; // FIXME  HInstance;
    }
  }

  if (AFileName != nullptr)
  {
    *AFileName = LibraryFileName;
  }
#endif
  return NewInstance;
}

LCID TGUIConfiguration::InternalLocale() const
{
  LCID Result;
  if (GetTranslationCount(GetApplicationInfo()) > 0)
  {
    TTranslation Translation;
    Translation = GetTranslation(GetApplicationInfo(), 0);
    Result = MAKELANGID(PRIMARYLANGID(Translation.Language), SUBLANG_DEFAULT);
  }
  else
  {
    DebugAssert(false);
    Result = 0;
  }
  return Result;
}

LCID TGUIConfiguration::GetLocale() const
{
  if (!FLocale)
  {
    FLocale = InternalLocale();
  }
  return FLocale;
}

void TGUIConfiguration::SetLocale(LCID Value)
{
  if (GetLocale() != Value)
  {
    HINSTANCE Module = LoadNewResourceModule(Value);
    if (Module != nullptr)
    {
      FLocale = Value;
      // SetResourceModule(Module);
    }
    else
    {
      DebugAssert(false);
    }
  }
}

void TGUIConfiguration::SetLocaleSafe(LCID Value)
{
  if (GetLocale() != Value)
  {
    HINSTANCE Module;

    try
    {
      Module = LoadNewResourceModule(Value);
    }
    catch (...)
    {
      // ignore any exception while loading locale
      Module = nullptr;
    }

    if (Module != nullptr)
    {
      FLocale = Value;
      // SetResourceModule(Module);
    }
  }
}

TStrings * TGUIConfiguration::GetLocales()
{
  ThrowNotImplemented(93);
  UnicodeString LocalesExts;
  std::unique_ptr<TStringList> Exts(CreateSortedStringList());

  DWORD FindAttrs = faReadOnly | faArchive;
  TSearchRecChecked SearchRec;
  bool Found;

  Found = (FindFirst(::ChangeFileExt(ModuleFileName(), L".*"),
    FindAttrs, SearchRec) == 0);
  {
    SCOPE_EXIT
    {
      FindClose(SearchRec);
    };
    UnicodeString Ext;
    while (Found)
    {
      Ext = ::ExtractFileExt(SearchRec.Name).UpperCase();
      if ((Ext.Length() >= 3) && (Ext != L".EXE") && (Ext != L".COM") &&
          (Ext != L".DLL") && (Ext != L".INI"))
      {
        Ext = Ext.SubString(2, Ext.Length() - 1);
        LocalesExts += Ext;
        Exts->Add(Ext);
      }
      Found = (FindNextChecked(SearchRec) == 0);
    }
  }

  if (FLastLocalesExts != LocalesExts)
  {
    FLastLocalesExts = LocalesExts;
    FLocales->Clear();

    /* // FIXME
    TLanguages * Langs = nullptr; // FIXME LanguagesDEPF();
    int Ext, Index, Count;
    wchar_t LocaleStr[255];
    LCID Locale;

    Count = Langs->GetCount();
    Index = -1;
    while (Index < Count)
    {
      if (Index >= 0)
      {
        Locale = Langs->LocaleID[Index];
        Ext = Exts->IndexOf(Langs->Ext[Index]);
        if (Ext < 0)
        {
          Ext = Exts->IndexOf(Langs->Ext[Index].SubString(1, 2));
          if (Ext >= 0)
          {
            Locale = MAKELANGID(PRIMARYLANGID(Locale), SUBLANG_DEFAULT);
          }
        }

        if (Ext >= 0)
        {
          Exts->SetObj(Ext, reinterpret_cast<TObject *>(Locale));
        }
        else
        {
          Locale = 0;
        }
      }
      else
      {
        Locale = InternalLocale();
      }

      if (Locale)
      {
        UnicodeString Name;
        GetLocaleInfo(Locale, LOCALE_SENGLANGUAGE,
          LocaleStr, sizeof(LocaleStr));
        Name = LocaleStr;
        Name += " - ";
        // LOCALE_SNATIVELANGNAME
        GetLocaleInfo(Locale, LOCALE_SLANGUAGE,
          LocaleStr, sizeof(LocaleStr));
        Name += LocaleStr;
        FLocales->AddObject(Name, reinterpret_cast<TObject *>(Locale));
      }
      ++Index;
    }
    */
    for (intptr_t Index = 0; Index < Exts->GetCount(); ++Index)
    {
      if ((Exts->GetObj(Index) == nullptr) &&
          (Exts->GetString(Index).Length() == 3) &&
          ::SameText(Exts->GetString(Index).SubString(1, 2), ADDITIONAL_LANGUAGE_PREFIX))
      {
        UnicodeString LangName = GetFileFileInfoString(L"LangName",
          ::ChangeFileExt(ModuleFileName(), UnicodeString(L".") + Exts->GetString(Index)));
        if (!LangName.IsEmpty())
        {
          FLocales->AddObject(LangName, reinterpret_cast<TObject *>(static_cast<size_t>(
            AdditionaLanguageMask + Exts->GetString(Index)[3])));
        }
      }
    }
  }

  return FLocales;
}

void TGUIConfiguration::SetDefaultCopyParam(const TGUICopyParamType & Value)
{
  FDefaultCopyParam.Assign(&Value);
  Changed();
}

bool TGUIConfiguration::GetRememberPassword() const
{
  return GetSessionRememberPassword() || GetPuttyPassword();
}

const TCopyParamList * TGUIConfiguration::GetCopyParamList()
{
  return FCopyParamList;
}

void TGUIConfiguration::SetCopyParamList(const TCopyParamList * Value)
{
  if (!(*FCopyParamList == *Value))
  {
    *FCopyParamList = *Value;
    FCopyParamListDefaults = false;
    Changed();
  }
}

intptr_t TGUIConfiguration::GetCopyParamIndex() const
{
  intptr_t Result;
  if (FCopyParamCurrent.IsEmpty())
  {
    Result = -1;
  }
  else
  {
    Result = FCopyParamList->IndexOfName(FCopyParamCurrent);
  }
  return Result;
}

void TGUIConfiguration::SetCopyParamIndex(intptr_t Value)
{
  UnicodeString Name;
  if (Value < 0)
  {
    Name.Clear();
  }
  else
  {
    Name = FCopyParamList->GetName(Value);
  }
  SetCopyParamCurrent(Name);
}

void TGUIConfiguration::SetCopyParamCurrent(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY(CopyParamCurrent);
}

const TGUICopyParamType TGUIConfiguration::GetCurrentCopyParam() const
{
  return GetCopyParamPreset(GetCopyParamCurrent());
}

const TGUICopyParamType TGUIConfiguration::GetCopyParamPreset(const UnicodeString & Name) const
{
  TGUICopyParamType Result = FDefaultCopyParam;
  if (!Name.IsEmpty())
  {
    intptr_t Index = FCopyParamList->IndexOfName(Name);
    DebugAssert(Index >= 0);
    if (Index >= 0)
    {
      const TCopyParamType * Preset = FCopyParamList->GetCopyParam(Index);
      DebugAssert(Preset != nullptr);
      Result.Assign(Preset); // overwrite all but GUI options
      // reset all options known not to be configurable per-preset
      // kind of hack
      Result.SetResumeSupport(FDefaultCopyParam.GetResumeSupport());
      Result.SetResumeThreshold(FDefaultCopyParam.GetResumeThreshold());
      Result.SetLocalInvalidChars(FDefaultCopyParam.GetLocalInvalidChars());
    }
  }
  return Result;
}

bool TGUIConfiguration::GetHasCopyParamPreset(const UnicodeString & Name) const
{
  return Name.IsEmpty() || (FCopyParamList->IndexOfName(Name) >= 0);
}

void TGUIConfiguration::SetNewDirectoryProperties(
  const TRemoteProperties & Value)
{
  SET_CONFIG_PROPERTY(NewDirectoryProperties);
}

void TGUIConfiguration::SetQueueTransfersLimit(intptr_t Value)
{
  SET_CONFIG_PROPERTY(QueueTransfersLimit);
}

void TGUIConfiguration::SetQueueKeepDoneItems(bool Value)
{
  SET_CONFIG_PROPERTY(QueueKeepDoneItems);
}

void TGUIConfiguration::SetQueueKeepDoneItemsFor(intptr_t Value)
{
  SET_CONFIG_PROPERTY(QueueKeepDoneItemsFor);
}

TStoredSessionList * TGUIConfiguration::SelectPuttySessionsForImport(
  TStoredSessionList * Sessions)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(new TStoredSessionList(true));
  ImportSessionList->SetDefaultSettings(Sessions->GetDefaultSettings());

  std::unique_ptr<TRegistryStorage> Storage(new TRegistryStorage(GetPuttySessionsKey()));
  Storage->SetForceAnsi(true);
  if (Storage->OpenRootKey(false))
  {
    ImportSessionList->Load(Storage.get(), false, true);
  }

  TSessionData * PuttySessionData =
    NB_STATIC_DOWNCAST(TSessionData, ImportSessionList->FindByName(GetPuttySession()));
  if (PuttySessionData != nullptr)
  {
    ImportSessionList->Remove(PuttySessionData);
  }
  ImportSessionList->SelectSessionsToImport(Sessions, true);

  return ImportSessionList.release();
}

bool TGUIConfiguration::AnyPuttySessionForImport(TStoredSessionList * Sessions)
{
  try
  {
    std::unique_ptr<TStoredSessionList> Sesssions(SelectPuttySessionsForImport(Sessions));
    return (Sesssions->GetCount() > 0);
  }
  catch (...)
  {
    return false;
  }
}

TStoredSessionList * TGUIConfiguration::SelectFilezillaSessionsForImport(
  TStoredSessionList * /*Sessions*/)
{
/*
  std::unique_ptr<TStoredSessionList> ImportSessionList(new TStoredSessionList(true));
  ImportSessionList->SetDefaultSettings(Sessions->GetDefaultSettings());

  UnicodeString AppDataPath = GetShellFolderPath(CSIDL_APPDATA);
  UnicodeString FilezillaSiteManagerFile =
    IncludeTrailingBackslash(AppDataPath) + L"FileZilla" WGOOD_SLASH "sitemanager.xml";

  if (::FileExists(FilezillaSiteManagerFile))
  {
    ImportSessionList->ImportFromFilezilla(FilezillaSiteManagerFile);

    ImportSessionList->SelectSessionsToImport(Sessions, true);
  }

  return ImportSessionList.release();
*/
  return nullptr;
}

bool TGUIConfiguration::AnyFilezillaSessionForImport(TStoredSessionList * Sessions)
{
  try
  {
    std::unique_ptr<TStoredSessionList> Sesssions(SelectFilezillaSessionsForImport(Sessions));
    return (Sesssions->GetCount() > 0);
  }
  catch (...)
  {
    return false;
  }
}

UnicodeString TGUIConfiguration::GetPuttyPath() const
{
  return FPuttyPath;
}

void TGUIConfiguration::SetPuttyPath(const UnicodeString & Value)
{
  FPuttyPath = Value;
}

UnicodeString TGUIConfiguration::GetDefaultPuttyPath() const
{
  return FDefaultPuttyPath;
}

UnicodeString TGUIConfiguration::GetPSftpPath() const
{
  return FPSftpPath;
}

void TGUIConfiguration::SetPSftpPath(const UnicodeString & Value)
{
  FPSftpPath = Value;
}

UnicodeString TGUIConfiguration::GetPuttySession() const
{
  return FPuttySession;
}

void TGUIConfiguration::SetPuttySession(const UnicodeString & Value)
{
  FPuttySession = Value;
}

UnicodeString TGUIConfiguration::GetCopyParamCurrent() const
{
  return FCopyParamCurrent;
}

UnicodeString TGUIConfiguration::GetChecksumAlg() const
{
  return FChecksumAlg;
}

void TGUIConfiguration::SetChecksumAlg(const UnicodeString & Value)
{
  FChecksumAlg = Value;
}

TGUIConfiguration * GetGUIConfiguration()
{
  return NB_STATIC_DOWNCAST(TGUIConfiguration, GetConfiguration());
}

NB_IMPLEMENT_CLASS(TGUICopyParamType, NB_GET_CLASS_INFO(TCopyParamType), nullptr)
NB_IMPLEMENT_CLASS(TGUIConfiguration, NB_GET_CLASS_INFO(TConfiguration), nullptr)
NB_IMPLEMENT_CLASS(TCopyParamRule, NB_GET_CLASS_INFO(TObject), nullptr)

