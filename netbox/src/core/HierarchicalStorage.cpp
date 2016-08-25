#include <vcl.h>
#pragma hdrstop

#include <vector>
#include <Common.h>
#include <Exceptions.h>
#include <TextsCore.h>
#include <StrUtils.hpp>

#include "PuttyIntf.h"
#include "HierarchicalStorage.h"

#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch (...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch (...) { FFailed++; }

UnicodeString MungeStr(const UnicodeString & Str, bool ForceAnsi)
{
  RawByteString Source;
  if (ForceAnsi)
  {
    Source = RawByteString(AnsiString(Str));
  }
  else
  {
    Source = RawByteString(UTF8String(Str));
    if (Source.Length() > Str.Length())
    {
      Source.Insert(CONST_BOM, 1);
    }
  }
  // should contain ASCII characters only
  RawByteString Dest;
  Dest.SetLength(Source.Length() * 3 + 1);
  putty_mungestr(Source.c_str(), const_cast<char *>(Dest.c_str()));
  PackStr(Dest);
  return UnicodeString(Dest.c_str(), Dest.Length());
}

UnicodeString UnMungeStr(const UnicodeString & Str)
{
  // Str should contain ASCII characters only
  RawByteString Source = Str;
  RawByteString Dest;
  Dest.SetLength(Source.Length() + 1);
  putty_unmungestr(Source.c_str(), const_cast<char *>(Dest.c_str()), static_cast<int>(Dest.Length()));
  // Cut the string at null character
  PackStr(Dest);
  UnicodeString Result;
  const std::string Bom(CONST_BOM);
  if (Dest.Pos(Bom.c_str()) == 1)
  {
    Dest.Delete(1, Bom.size());
    Result = UTF8ToString(Dest);
  }
  else
  {
    Result = AnsiToString(Dest);
  }
  return Result;
}

UnicodeString PuttyMungeStr(const UnicodeString & Str)
{
  return MungeStr(Str, false);
}

UnicodeString PuttyUnMungeStr(const UnicodeString & Str)
{
  return UnMungeStr(Str);
}

UnicodeString MungeIniName(const UnicodeString & Str)
{
  intptr_t P = Str.Pos(L"=");
  // make this fast for now
  if (P > 0)
  {
    return ReplaceStr(Str, L"=", L"%3D");
  }
  else
  {
    return Str;
  }
}

UnicodeString UnMungeIniName(const UnicodeString & Str)
{
  intptr_t P = Str.Pos(L"%3D");
  // make this fast for now
  if (P > 0)
  {
    return ReplaceStr(Str, L"%3D", L"=");
  }
  else
  {
    return Str;
  }
}

THierarchicalStorage::THierarchicalStorage(const UnicodeString & AStorage) :
  FStorage(AStorage),
  FKeyHistory(new TStringList())
{
  SetAccessMode(smRead);
  SetExplicit(false);
  // While this was implemented in 5.0 already, for some reason
  // it was disabled (by mistake?). So although enabled for 5.6.1 only,
  // data written in Unicode/UTF8 can be read by all versions back to 5.0.
  SetForceAnsi(false);
  SetMungeStringValues(true);
}

THierarchicalStorage::~THierarchicalStorage()
{
  SAFE_DESTROY(FKeyHistory);
}

void THierarchicalStorage::Flush()
{
}

void THierarchicalStorage::SetAccessMode(TStorageAccessMode Value)
{
  FAccessMode = Value;
}

UnicodeString THierarchicalStorage::GetCurrentSubKeyMunged() const
{
  if (FKeyHistory->GetCount())
  {
    return FKeyHistory->GetString(FKeyHistory->GetCount() - 1);
  }
  else
  {
    return L"";
  }
}

UnicodeString THierarchicalStorage::GetCurrentSubKey() const
{
  return UnMungeStr(GetCurrentSubKeyMunged());
}

bool THierarchicalStorage::OpenRootKey(bool CanCreate)
{
  return OpenSubKey(L"", CanCreate);
}

UnicodeString THierarchicalStorage::MungeKeyName(const UnicodeString & Key)
{
  UnicodeString Result = MungeStr(Key, GetForceAnsi());
  // if there's already ANSI-munged subkey, keep ANSI munging
  if ((Result != Key) && !GetForceAnsi() && DoKeyExists(Key, true))
  {
    Result = MungeStr(Key, true);
  }
  return Result;
}

bool THierarchicalStorage::OpenSubKey(const UnicodeString & ASubKey, bool CanCreate, bool Path)
{
  bool Result;
  UnicodeString MungedKey;
  UnicodeString Key = ASubKey;
  if (Path)
  {
    DebugAssert(Key.IsEmpty() || (Key[Key.Length()] != L'\\'));
    Result = true;
    while (!Key.IsEmpty() && Result)
    {
      if (!MungedKey.IsEmpty())
      {
        MungedKey += L'\\';
      }
      MungedKey += MungeKeyName(CutToChar(Key, L'\\', false));
      Result = DoOpenSubKey(MungedKey, CanCreate);
    }

    // hack to restore last opened key for registry storage
    if (!Result)
    {
      FKeyHistory->Add(::IncludeTrailingBackslash(GetCurrentSubKey() + MungedKey));
      CloseSubKey();
    }
  }
  else
  {
    MungedKey = MungeKeyName(Key);
    Result = DoOpenSubKey(MungedKey, CanCreate);
  }

  if (Result)
  {
    FKeyHistory->Add(::IncludeTrailingBackslash(GetCurrentSubKey() + MungedKey));
  }

  return Result;
}

void THierarchicalStorage::CloseSubKey()
{
  if (FKeyHistory->GetCount() == 0)
    throw Exception(L"");
  else
    FKeyHistory->Delete(FKeyHistory->GetCount() - 1);
}

void THierarchicalStorage::CloseAll()
{
  while (!GetCurrentSubKey().IsEmpty())
  {
    CloseSubKey();
  }
}

void THierarchicalStorage::ClearSubKeys()
{
  std::unique_ptr<TStringList> SubKeys(new TStringList());
  try__finally
  {
    GetSubKeyNames(SubKeys.get());
    for (intptr_t Index = 0; Index < SubKeys->GetCount(); ++Index)
    {
      RecursiveDeleteSubKey(SubKeys->GetString(Index));
    }
  }
  __finally
  {
//    delete SubKeys;
  };
}

void THierarchicalStorage::RecursiveDeleteSubKey(const UnicodeString & Key)
{
  if (OpenSubKey(Key, false))
  {
    ClearSubKeys();
    CloseSubKey();
  }
  DeleteSubKey(Key);
}

bool THierarchicalStorage::HasSubKeys()
{
  bool Result = false;
  std::unique_ptr<TStrings> SubKeys(new TStringList());
  try__finally
  {
    GetSubKeyNames(SubKeys.get());
    Result = SubKeys->GetCount() > 0;
  }
  __finally
  {
//    delete SubKeys;
  };
  return Result;
}

bool THierarchicalStorage::HasSubKey(const UnicodeString & SubKey)
{
  bool Result = OpenSubKey(SubKey, false);
  if (Result)
  {
    CloseSubKey();
  }
  return Result;
}

bool THierarchicalStorage::KeyExists(const UnicodeString & SubKey)
{
  return DoKeyExists(SubKey, GetForceAnsi());
}

void THierarchicalStorage::ReadValues(TStrings * Strings,
  bool MaintainKeys)
{
  std::unique_ptr<TStrings> Names(new TStringList());
  try__finally
  {
    GetValueNames(Names.get());
    for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
    {
      if (MaintainKeys)
      {
        Strings->Add(FORMAT(L"%s=%s", Names->GetString(Index).c_str(),
          ReadString(Names->GetString(Index), L"").c_str()));
      }
      else
      {
        Strings->Add(ReadString(Names->GetString(Index), L""));
      }
    }
  }
  __finally
  {
//    delete Names;
  };
}

void THierarchicalStorage::ClearValues()
{
  std::unique_ptr<TStrings> Names(new TStringList());
  try__finally
  {
    GetValueNames(Names.get());
    for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
    {
      DeleteValue(Names->GetString(Index));
    }
  }
  __finally
  {
//    delete Names;
  };
}

void THierarchicalStorage::WriteValues(TStrings * Strings,
  bool MaintainKeys)
{
  ClearValues();

  if (Strings)
  {
    for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
    {
      if (MaintainKeys)
      {
        DebugAssert(Strings->GetString(Index).Pos(L"=") > 1);
        WriteString(Strings->GetName(Index), Strings->GetValue(Strings->GetName(Index)));
      }
      else
      {
        WriteString(::IntToStr(Index), Strings->GetString(Index));
      }
    }
  }
}

UnicodeString THierarchicalStorage::ReadString(const UnicodeString & Name, const UnicodeString & Default) const
{
  UnicodeString Result;
  if (GetMungeStringValues())
  {
    Result = UnMungeStr(ReadStringRaw(Name, MungeStr(Default, GetForceAnsi())));
  }
  else
  {
    Result = ReadStringRaw(Name, Default);
  }
  return Result;
}

RawByteString THierarchicalStorage::ReadBinaryData(const UnicodeString & Name) const
{
  size_t Size = BinaryDataSize(Name);
  RawByteString Value;
  Value.SetLength(Size);
  ReadBinaryData(Name, static_cast<void *>(const_cast<char *>(Value.c_str())), Size);
  return Value;
}

RawByteString THierarchicalStorage::ReadStringAsBinaryData(const UnicodeString & Name, const RawByteString & Default) const
{
  UnicodeString UnicodeDefault = AnsiToString(Default);
  // This should be exactly the same operation as calling ReadString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  UnicodeString String = ReadString(Name, UnicodeDefault);
  AnsiString Ansi = AnsiString(String);
  RawByteString Result = RawByteString(Ansi.c_str(), Ansi.Length());
  return Result;
}

void THierarchicalStorage::WriteString(const UnicodeString & Name, const UnicodeString & Value)
{
  if (GetMungeStringValues())
  {
    WriteStringRaw(Name, MungeStr(Value, GetForceAnsi()));
  }
  else
  {
    WriteStringRaw(Name, Value);
  }
}

void THierarchicalStorage::WriteBinaryData(const UnicodeString & Name,
  const RawByteString & Value)
{
  WriteBinaryData(Name, Value.c_str(), Value.Length());
}

void THierarchicalStorage::WriteBinaryDataAsString(const UnicodeString & Name, const RawByteString & Value)
{
  // This should be exactly the same operation as calling WriteString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  WriteString(Name, AnsiToString(Value));
}

UnicodeString THierarchicalStorage::IncludeTrailingBackslash(const UnicodeString & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.IsEmpty())
  {
    return S;
  }
  else
  {
    return ::IncludeTrailingBackslash(S);
  }
}

UnicodeString THierarchicalStorage::ExcludeTrailingBackslash(const UnicodeString & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.IsEmpty())
  {
    return S;
  }
  else
  {
    return ::ExcludeTrailingBackslash(S);
  }
}

bool THierarchicalStorage::GetTemporary() const
{
  return false;
}

TRegistryStorage::TRegistryStorage(const UnicodeString & AStorage) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(nullptr)
{
  Init();
}

TRegistryStorage::TRegistryStorage(const UnicodeString & AStorage, HKEY ARootKey) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(nullptr),
  FFailed(0)
{
  Init();
  FRegistry->SetRootKey(ARootKey);
}

void TRegistryStorage::Init()
{
  FFailed = 0;
  FRegistry = new TRegistry();
  FRegistry->SetAccess(KEY_READ);
}

TRegistryStorage::~TRegistryStorage()
{
  SAFE_DESTROY(FRegistry);
}

bool TRegistryStorage::Copy(TRegistryStorage * Storage)
{
  TRegistry * Registry = Storage->FRegistry;
  bool Result = true;
  std::unique_ptr<TStrings> Names(new TStringList());
  try__finally
  {
    std::vector<uint8_t> Buffer(1024);
    Registry->GetValueNames(Names.get());
    intptr_t Index = 0;
    while ((Index < Names->GetCount()) && Result)
    {
      UnicodeString Name = MungeStr(Names->GetString(Index), GetForceAnsi());
      DWORD Size = static_cast<DWORD>(Buffer.size());
      DWORD Type;
      int RegResult = 0;
      do
      {
        RegResult = ::RegQueryValueEx(Registry->GetCurrentKey(), Name.c_str(), nullptr,
          &Type, &Buffer[0], &Size);
        if (RegResult == ERROR_MORE_DATA)
        {
          Buffer.resize(Size);
        }
      }
      while (RegResult == ERROR_MORE_DATA);

      Result = (RegResult == ERROR_SUCCESS);
      if (Result)
      {
        RegResult = ::RegSetValueEx(FRegistry->GetCurrentKey(), Name.c_str(), 0, Type,
          &Buffer[0], Size);
        Result = (RegResult == ERROR_SUCCESS);
      }

      ++Index;
    }
  }
  __finally
  {
//    delete Names;
  };
  return Result;
}

UnicodeString TRegistryStorage::GetSource() const
{
  return RootKeyToStr(FRegistry->GetRootKey()) + L"\\" + GetStorage();
}

UnicodeString TRegistryStorage::GetSource()
{
  return RootKeyToStr(FRegistry->GetRootKey()) + L"\\" + GetStorage();
}

void TRegistryStorage::SetAccessMode(TStorageAccessMode Value)
{
  THierarchicalStorage::SetAccessMode(Value);
  if (FRegistry)
  {
    switch (GetAccessMode())
    {
      case smRead:
        FRegistry->SetAccess(KEY_READ);
        break;

      case smReadWrite:
      default:
        FRegistry->SetAccess(KEY_READ | KEY_WRITE);
        break;
    }
  }
}

bool TRegistryStorage::DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate)
{
  if (FKeyHistory->GetCount() > 0)
  {
    FRegistry->CloseKey();
  }
  UnicodeString Key = ::ExcludeTrailingBackslash(GetStorage() + GetCurrentSubKey() + SubKey);
  return FRegistry->OpenKey(Key, CanCreate);
}

void TRegistryStorage::CloseSubKey()
{
  FRegistry->CloseKey();
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount())
  {
    FRegistry->OpenKey(GetStorage() + GetCurrentSubKeyMunged(), True);
  }
}

bool TRegistryStorage::DeleteSubKey(const UnicodeString & SubKey)
{
  UnicodeString Key;
  if (FKeyHistory->GetCount() == 0)
  {
    Key = GetStorage() + GetCurrentSubKey();
  }
  Key += MungeKeyName(SubKey);
  return FRegistry->DeleteKey(Key);
}

void TRegistryStorage::GetSubKeyNames(TStrings * Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
  {
    Strings->SetString(Index, UnMungeStr(Strings->GetString(Index)));
  }
}

void TRegistryStorage::GetValueNames(TStrings * Strings) const
{
  FRegistry->GetValueNames(Strings);
}

bool TRegistryStorage::DeleteValue(const UnicodeString & Name)
{
  return FRegistry->DeleteValue(Name);
}

bool TRegistryStorage::DoKeyExists(const UnicodeString & SubKey, bool AForceAnsi)
{
  UnicodeString Key = MungeStr(SubKey, AForceAnsi);
  bool Result = FRegistry->KeyExists(Key);
  return Result;
}

bool TRegistryStorage::ValueExists(const UnicodeString & Value) const
{
  bool Result = FRegistry->ValueExists(Value);
  return Result;
}

size_t TRegistryStorage::BinaryDataSize(const UnicodeString & Name) const
{
  size_t Result = FRegistry->GetDataSize(Name);
  return Result;
}

bool TRegistryStorage::ReadBool(const UnicodeString & Name, bool Default) const
{
  READ_REGISTRY(ReadBool);
}

TDateTime TRegistryStorage::ReadDateTime(const UnicodeString & Name, const TDateTime & Default) const
{
  READ_REGISTRY(ReadDateTime);
}

double TRegistryStorage::ReadFloat(const UnicodeString & Name, double Default) const
{
  READ_REGISTRY(ReadFloat);
}

intptr_t TRegistryStorage::ReadInteger(const UnicodeString & Name, intptr_t Default) const
{
  READ_REGISTRY(ReadInteger);
}

int64_t TRegistryStorage::ReadInt64(const UnicodeString & Name, int64_t Default) const
{
  int64_t Result = Default;
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      FRegistry->ReadBinaryData(Name, &Result, sizeof(Result));
    }
    catch (...)
    {
      FFailed++;
    }
  }
  return Result;
}

UnicodeString TRegistryStorage::ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) const
{
  READ_REGISTRY(ReadString);
}

size_t TRegistryStorage::ReadBinaryData(const UnicodeString & Name,
  void * Buffer, size_t Size) const
{
  size_t Result;
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      Result = FRegistry->ReadBinaryData(Name, Buffer, Size);
    }
    catch (...)
    {
      Result = 0;
      FFailed++;
    }
  }
  else
  {
    Result = 0;
  }
  return Result;
}

void TRegistryStorage::WriteBool(const UnicodeString & Name, bool Value)
{
  WRITE_REGISTRY(WriteBool);
}

void TRegistryStorage::WriteDateTime(const UnicodeString & Name, const TDateTime & Value)
{
  WRITE_REGISTRY(WriteDateTime);
}

void TRegistryStorage::WriteFloat(const UnicodeString & Name, double Value)
{
  WRITE_REGISTRY(WriteFloat);
}

void TRegistryStorage::WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  WRITE_REGISTRY(WriteString);
}

void TRegistryStorage::WriteInteger(const UnicodeString & Name, intptr_t Value)
{
  WRITE_REGISTRY(WriteInteger);
}

void TRegistryStorage::WriteInt64(const UnicodeString & Name, int64_t Value)
{
  try
  {
    FRegistry->WriteBinaryData(Name, &Value, sizeof(Value));
  }
  catch (...)
  {
    FFailed++;
  }
}

void TRegistryStorage::WriteBinaryData(const UnicodeString & Name,
  const void * Buffer, size_t Size)
{
  try
  {
    FRegistry->WriteBinaryData(Name, const_cast<void *>(Buffer), Size);
  }
  catch (...)
  {
    FFailed++;
  }
}

intptr_t TRegistryStorage::GetFailed() const
{
  intptr_t Result = FFailed;
  FFailed = 0;
  return Result;
}

/*
__fastcall TCustomIniFileStorage::TCustomIniFileStorage(const UnicodeString Storage, TCustomIniFile * IniFile) :
  THierarchicalStorage(Storage),
  FIniFile(IniFile),
  FMasterStorageOpenFailures(0),
  FOpeningSubKey(false)
{
}
//---------------------------------------------------------------------------
__fastcall TCustomIniFileStorage::~TCustomIniFileStorage()
{
  delete FIniFile;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomIniFileStorage::GetSource()
{
  return Storage;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomIniFileStorage::GetCurrentSection()
{
  return ExcludeTrailingBackslash(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::CacheSections()
{
  if (FSections.get() == nullptr)
  {
    FSections.reset(new TStringList());
    FIniFile->ReadSections(FSections.get());
    FSections->Sorted = true; // has to set only after reading as ReadSections reset it to false
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::ResetCache()
{
  FSections.reset(nullptr);
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::SetAccessMode(TStorageAccessMode value)
{
  if (FMasterStorage.get() != nullptr)
  {
    FMasterStorage->AccessMode = value;
  }
  THierarchicalStorage::SetAccessMode(value);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DoOpenSubKey(const UnicodeString SubKey, bool CanCreate)
{
  bool Result = CanCreate;

  if (!Result)
  {
    CacheSections();
    UnicodeString NewKey = ExcludeTrailingBackslash(CurrentSubKey+SubKey);
    if (FSections->Count > 0)
    {
      int Index = -1;
      Result = FSections->Find(NewKey, Index);
      if (!Result &&
          (Index < FSections->Count) &&
          (FSections->Strings[Index].SubString(1, NewKey.Length()+1) == NewKey + L"\\"))
      {
        Result = true;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::OpenRootKey(bool CanCreate)
{
  // Not supported with master storage.
  // Actually currently, we use OpenRootKey with TRegistryStorage only.
  DebugAssert(FMasterStorage.get() == NULL);

  return THierarchicalStorage::OpenRootKey(CanCreate);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::OpenSubKey(UnicodeString Key, bool CanCreate, bool Path)
{
  bool Result;

  {
    TAutoFlag Flag(FOpeningSubKey);
    Result = THierarchicalStorage::OpenSubKey(Key, CanCreate, Path);
  }

  if (FMasterStorage.get() != nullptr)
  {
    if (FMasterStorageOpenFailures > 0)
    {
      FMasterStorageOpenFailures++;
    }
    else
    {
      bool MasterResult = FMasterStorage->OpenSubKey(Key, CanCreate, Path);
      if (!Result && MasterResult)
      {
        Result = THierarchicalStorage::OpenSubKey(Key, true, Path);
        DebugAssert(Result);
      }
      else if (Result && !MasterResult)
      {
        FMasterStorageOpenFailures++;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::CloseSubKey()
{
  THierarchicalStorage::CloseSubKey();

  // What we are called to restore previous key from OpenSubKey,
  // when opening path component fails, the master storage was not involved yet
  if (!FOpeningSubKey && (FMasterStorage.get() != nullptr))
  {
    if (FMasterStorageOpenFailures > 0)
    {
      FMasterStorageOpenFailures--;
    }
    else
    {
      FMasterStorage->CloseSubKey();
    }
  }
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DeleteSubKey(const UnicodeString SubKey)
{
  bool Result;
  try
  {
    ResetCache();
    FIniFile->EraseSection(CurrentSubKey + MungeKeyName(SubKey));
    Result = true;
  }
  catch (...)
  {
    Result = false;
  }
  if (HandleByMasterStorage())
  {
    Result = FMasterStorage->DeleteSubKey(SubKey);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::GetSubKeyNames(Classes::TStrings* Strings)
{
  Strings->Clear();
  if (HandleByMasterStorage())
  {
    FMasterStorage->GetSubKeyNames(Strings);
  }
  CacheSections();
  for (int i = 0; i < FSections->Count; i++)
  {
    UnicodeString Section = FSections->Strings[i];
    if (AnsiCompareText(CurrentSubKey,
        Section.SubString(1, CurrentSubKey.Length())) == 0)
    {
      UnicodeString SubSection = Section.SubString(CurrentSubKey.Length() + 1,
        Section.Length() - CurrentSubKey.Length());
      int P = SubSection.Pos(L"\\");
      if (P)
      {
        SubSection.SetLength(P - 1);
      }
      if (Strings->IndexOf(SubSection) < 0)
      {
        Strings->Add(UnMungeStr(SubSection));
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::GetValueNames(Classes::TStrings* Strings)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->GetValueNames(Strings);
  }
  FIniFile->ReadSection(CurrentSection, Strings);
  for (int Index = 0; Index < Strings->Count; Index++)
  {
    Strings->Strings[Index] = UnMungeIniName(Strings->Strings[Index]);
  }
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DoKeyExists(const UnicodeString SubKey, bool AForceAnsi)
{
  return
    (HandleByMasterStorage() && FMasterStorage->DoKeyExists(SubKey, AForceAnsi)) ||
    FIniFile->SectionExists(CurrentSubKey + MungeStr(SubKey, AForceAnsi));
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DoValueExists(const UnicodeString & Value)
{
  return FIniFile->ValueExists(CurrentSection, MungeIniName(Value));
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::ValueExists(const UnicodeString Value)
{
  return
    (HandleByMasterStorage() && FMasterStorage->ValueExists(Value)) ||
    DoValueExists(Value);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DeleteValue(const UnicodeString Name)
{
  bool Result = true;
  if (HandleByMasterStorage())
  {
    Result = FMasterStorage->DeleteValue(Name);
  }
  ResetCache();
  FIniFile->DeleteKey(CurrentSection, MungeIniName(Name));
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::HandleByMasterStorage()
{
  return
    (FMasterStorage.get() != nullptr) &&
    (FMasterStorageOpenFailures == 0);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::HandleReadByMasterStorage(const UnicodeString & Name)
{
  return HandleByMasterStorage() && !DoValueExists(Name);
}
//---------------------------------------------------------------------------
size_t __fastcall TCustomIniFileStorage::BinaryDataSize(const UnicodeString Name)
{
  if (HandleReadByMasterStorage(Name))
  {
    return FMasterStorage->BinaryDataSize(Name);
  }
  else
  {
    return ReadStringRaw(Name, L"").Length() / 2;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::ReadBool(const UnicodeString Name, bool Default)
{
  if (HandleReadByMasterStorage(Name))
  {
    return FMasterStorage->ReadBool(Name, Default);
  }
  else
  {
    return FIniFile->ReadBool(CurrentSection, MungeIniName(Name), Default);
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomIniFileStorage::ReadInteger(const UnicodeString Name, int Default)
{
  int Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadInteger(Name, Default);
  }
  else
  {
    Result = FIniFile->ReadInteger(CurrentSection, MungeIniName(Name), Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
__int64 __fastcall TCustomIniFileStorage::ReadInt64(const UnicodeString Name, __int64 Default)
{
  __int64 Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadInt64(Name, Default);
  }
  else
  {
    Result = Default;
    UnicodeString Str;
    Str = ReadStringRaw(Name, L"");
    if (!Str.IsEmpty())
    {
      Result = StrToInt64Def(Str, Default);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime __fastcall TCustomIniFileStorage::ReadDateTime(const UnicodeString Name, TDateTime Default)
{
  TDateTime Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadDateTime(Name, Default);
  }
  else
  {
    UnicodeString Value = FIniFile->ReadString(CurrentSection, MungeIniName(Name), L"");
    if (Value.IsEmpty())
    {
      Result = Default;
    }
    else
    {
      try
      {
        RawByteString Raw = HexToBytes(Value);
        if (static_cast<size_t>(Raw.Length()) == sizeof(Result))
        {
          memcpy(&Result, Raw.c_str(), sizeof(Result));
        }
        else
        {
          Result = StrToDateTime(Value);
        }
      }
      catch(...)
      {
        Result = Default;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
double __fastcall TCustomIniFileStorage::ReadFloat(const UnicodeString Name, double Default)
{
  double Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadFloat(Name, Default);
  }
  else
  {
    UnicodeString Value = FIniFile->ReadString(CurrentSection, MungeIniName(Name), L"");
    if (Value.IsEmpty())
    {
      Result = Default;
    }
    else
    {
      try
      {
        RawByteString Raw = HexToBytes(Value);
        if (static_cast<size_t>(Raw.Length()) == sizeof(Result))
        {
          memcpy(&Result, Raw.c_str(), sizeof(Result));
        }
        else
        {
          Result = static_cast<double>(StrToFloat(Value));
        }
      }
      catch(...)
      {
        Result = Default;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomIniFileStorage::ReadStringRaw(const UnicodeString Name, UnicodeString Default)
{
  UnicodeString Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadStringRaw(Name, Default);
  }
  else
  {
    Result = FIniFile->ReadString(CurrentSection, MungeIniName(Name), Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
size_t __fastcall TCustomIniFileStorage::ReadBinaryData(const UnicodeString Name,
  void * Buffer, size_t Size)
{
  size_t Len;
  if (HandleReadByMasterStorage(Name))
  {
    FMasterStorage->ReadBinaryData(Name, Buffer, Size);
  }
  else
  {
    RawByteString Value = HexToBytes(ReadStringRaw(Name, L""));
    Len = Value.Length();
    if (Size > Len)
    {
      Size = Len;
    }
    DebugAssert(Buffer);
    memcpy(Buffer, Value.c_str(), Size);
  }
  return Size;
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteBool(const UnicodeString Name, bool Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteBool(Name, Value);
  }
  ResetCache();
  FIniFile->WriteBool(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteInteger(const UnicodeString Name, int Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteInteger(Name, Value);
  }
  ResetCache();
  FIniFile->WriteInteger(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteInt64(const UnicodeString Name, __int64 Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteInt64(Name, Value);
  }
  DoWriteStringRaw(Name, IntToStr(Value));
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteDateTime(const UnicodeString Name, TDateTime Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteDateTime(Name, Value);
  }
  DoWriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteFloat(const UnicodeString Name, double Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteFloat(Name, Value);
  }
  DoWriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  ResetCache();
  FIniFile->WriteString(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteStringRaw(const UnicodeString Name, const UnicodeString Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteStringRaw(Name, Value);
  }
  DoWriteStringRaw(Name, Value);
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::DoWriteBinaryData(const UnicodeString & Name,
  const void * Buffer, int Size)
{
  DoWriteStringRaw(Name, BytesToHex(RawByteString(static_cast<const char*>(Buffer), Size)));
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteBinaryData(const UnicodeString Name,
  const void * Buffer, int Size)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteBinaryData(Name, Buffer, Size);
  }
  DoWriteBinaryData(Name, Buffer, Size);
}
//===========================================================================
__fastcall TIniFileStorage::TIniFileStorage(const UnicodeString AStorage):
  TCustomIniFileStorage(AStorage, new TMemIniFile(AStorage))
{
  FOriginal = new TStringList();
  dynamic_cast<TMemIniFile *>(FIniFile)->GetStrings(FOriginal);
  ApplyOverrides();
}
//---------------------------------------------------------------------------
void __fastcall TIniFileStorage::Flush()
{
  if (FMasterStorage.get() != nullptr)
  {
    FMasterStorage->Flush();
  }
  if (FOriginal != nullptr)
  {
    TStrings * Strings = new TStringList;
    try
    {
      dynamic_cast<TMemIniFile *>(FIniFile)->GetStrings(Strings);
      if (!Strings->Equals(FOriginal))
      {
        int Attr;
        // preserve attributes (especially hidden)
        bool Exists = FileExists(ApiPath(Storage));
        if (Exists)
        {
          Attr = GetFileAttributes(ApiPath(Storage).c_str());
        }
        else
        {
          Attr = FILE_ATTRIBUTE_NORMAL;
        }

        HANDLE Handle = CreateFile(ApiPath(Storage).c_str(), GENERIC_READ | GENERIC_WRITE,
          0, nullptr, CREATE_ALWAYS, Attr, 0);

        if (Handle == INVALID_HANDLE_VALUE)
        {
          // "access denied" errors upon implicit saves to existing file are ignored
          if (Explicit || !Exists || (GetLastError() != ERROR_ACCESS_DENIED))
          {
            throw EOSExtException(FMTLOAD((Exists ? WRITE_ERROR : CREATE_FILE_ERROR), (Storage)));
          }
        }
        else
        {
          TStream * Stream = new THandleStream(int(Handle));
          try
          {
            Strings->SaveToStream(Stream);
          }
          __finally
          {
            CloseHandle(Handle);
            delete Stream;
          }
        }
      }
    }
    __finally
    {
      delete FOriginal;
      FOriginal = nullptr;
      delete Strings;
    }
  }
}
//---------------------------------------------------------------------------
__fastcall TIniFileStorage::~TIniFileStorage()
{
  Flush();
}
//---------------------------------------------------------------------------
void __fastcall TIniFileStorage::ApplyOverrides()
{
  UnicodeString OverridesKey = IncludeTrailingBackslash(L"Override");

  CacheSections();
  for (int i = 0; i < FSections->Count; i++)
  {
    UnicodeString Section = FSections->Strings[i];

    if (AnsiSameText(OverridesKey,
          Section.SubString(1, OverridesKey.Length())))
    {
      UnicodeString SubKey = Section.SubString(OverridesKey.Length() + 1,
        Section.Length() - OverridesKey.Length());

      // this all uses raw names (munged)
      TStrings * Names = new TStringList;
      try
      {
        FIniFile->ReadSection(Section, Names);

        for (int ii = 0; ii < Names->Count; ii++)
        {
          UnicodeString Name = Names->Strings[ii];
          UnicodeString Value = FIniFile->ReadString(Section, Name, L"");
          FIniFile->WriteString(SubKey, Name, Value);
        }
      }
      __finally
      {
        delete Names;
      }

      FIniFile->EraseSection(Section);
      ResetCache();
    }
  }
}
//===========================================================================
enum TWriteMode { wmAllow, wmFail, wmIgnore };
//---------------------------------------------------------------------------
class TOptionsIniFile : public TCustomIniFile
{
public:
  __fastcall TOptionsIniFile(TStrings * Options, TWriteMode WriteMode, const UnicodeString & RootKey);

  virtual UnicodeString __fastcall ReadString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Default);
  virtual void __fastcall WriteString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Value);
  virtual void __fastcall ReadSection(const UnicodeString Section, TStrings * Strings);
  virtual void __fastcall ReadSections(TStrings* Strings);
  virtual void __fastcall ReadSectionValues(const UnicodeString Section, TStrings* Strings);
  virtual void __fastcall EraseSection(const UnicodeString Section);
  virtual void __fastcall DeleteKey(const UnicodeString Section, const UnicodeString Ident);
  virtual void __fastcall UpdateFile();
  // Hoisted overload
  void __fastcall ReadSections(const UnicodeString Section, TStrings* Strings);
  // Ntb, we can implement ValueExists more efficiently than the TCustomIniFile.ValueExists

private:
  TStrings * FOptions;
  TWriteMode FWriteMode;
  UnicodeString FRootKey;

  bool __fastcall AllowWrite();
  void __fastcall NotImplemented();
};
//---------------------------------------------------------------------------
__fastcall TOptionsIniFile::TOptionsIniFile(TStrings * Options, TWriteMode WriteMode, const UnicodeString & RootKey) :
  TCustomIniFile(UnicodeString())
{
  FOptions = Options;
  FWriteMode = WriteMode;
  FRootKey = RootKey;
  if (!FRootKey.IsEmpty())
  {
    FRootKey += PathDelim;
  }
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::NotImplemented()
{
  throw Exception(L"Not implemented");
}
//---------------------------------------------------------------------------
bool __fastcall TOptionsIniFile::AllowWrite()
{
  switch (FWriteMode)
  {
    case wmAllow:
      return true;

    case wmFail:
      NotImplemented();
      return false; // never gets here

    case wmIgnore:
      return false;

    default:
      DebugFail();
      return false;
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TOptionsIniFile::ReadString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Default)
{
  UnicodeString Name = Section;
  if (!Name.IsEmpty())
  {
    Name += PathDelim;
  }
  UnicodeString Value;
  if (!SameText(Name.SubString(1, FRootKey.Length()), FRootKey))
  {
    Value = Default;
  }
  else
  {
    Name.Delete(1, FRootKey.Length());
    Name += Ident;

    int Index = FOptions->IndexOfName(Name);
    if (Index >= 0)
    {
      Value = FOptions->ValueFromIndex[Index];
    }
    else
    {
      Value = Default;
    }
  }
  return Value;
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::WriteString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Value)
{
  if (AllowWrite() &&
      // Implemented for TSessionData.DoSave only
      DebugAlwaysTrue(Section.IsEmpty() && FRootKey.IsEmpty()))
  {
    FOptions->Values[Ident] = Value;
  }
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::ReadSection(const UnicodeString Section, TStrings * Strings)
{

  UnicodeString SectionPrefix = Section;
  if (!SectionPrefix.IsEmpty())
  {
    SectionPrefix += PathDelim;
  }

  if (SameText(SectionPrefix.SubString(1, FRootKey.Length()), FRootKey))
  {
    SectionPrefix.Delete(1, FRootKey.Length());

    Strings->BeginUpdate();
    try
    {
      for (int Index = 0; Index < FOptions->Count; Index++)
      {
        UnicodeString Name = FOptions->Names[Index];
        if (SameText(Name.SubString(1, SectionPrefix.Length()), SectionPrefix) &&
            (LastDelimiter(PathDelim, Name) <= SectionPrefix.Length()))
        {
          Strings->Add(Name.SubString(SectionPrefix.Length() + 1, Name.Length() - SectionPrefix.Length()));
        }
      }
    }
    __finally
    {
      Strings->EndUpdate();
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::ReadSections(TStrings * Strings)
{
  std::unique_ptr<TStringList> Sections(CreateSortedStringList());

  for (int Index = 0; Index < FOptions->Count; Index++)
  {
    UnicodeString Name = FOptions->Names[Index];
    int P = LastDelimiter(PathDelim, Name);
    if (P > 0)
    {
      UnicodeString Section = Name.SubString(1, P - 1);
      if (Sections->IndexOf(Section) < 0)
      {
        Sections->Add(Section);
      }
    }
  }

  for (int Index = 0; Index < Sections->Count; Index++)
  {
    Strings->Add(FRootKey + Sections->Strings[Index]);
  }
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::ReadSectionValues(const UnicodeString Section, TStrings * / *Strings* /)
{
  NotImplemented();
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::EraseSection(const UnicodeString Section)
{
  if (AllowWrite())
  {
    NotImplemented();
  }
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::DeleteKey(const UnicodeString Section, const UnicodeString Ident)
{
  if (AllowWrite() &&
      // Implemented for TSessionData.DoSave only
      DebugAlwaysTrue(Section.IsEmpty() && FRootKey.IsEmpty()))
  {
    int Index = FOptions->IndexOfName(Ident);
    if (Index >= 0)
    {
      FOptions->Delete(Index);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::UpdateFile()
{
  if (AllowWrite())
  {
    // noop
  }
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::ReadSections(const UnicodeString / *Section* /, TStrings * / * Strings* /)
{
  NotImplemented();
}
//===========================================================================
__fastcall TOptionsStorage::TOptionsStorage(TStrings * Options, bool AllowWrite):
  TCustomIniFileStorage(
    UnicodeString(L"Command-line options"),
    new TOptionsIniFile(Options, (AllowWrite ? wmAllow : wmFail), UnicodeString()))
{
}
//---------------------------------------------------------------------------
__fastcall TOptionsStorage::TOptionsStorage(TStrings * Options, const UnicodeString & RootKey, THierarchicalStorage * MasterStorage) :
  TCustomIniFileStorage(
    UnicodeString(L"Command-line options overriding " + MasterStorage->Source),
    new TOptionsIniFile(Options, wmIgnore, RootKey))
{
  FMasterStorage.reset(MasterStorage);
}
//---------------------------------------------------------------------------
bool __fastcall TOptionsStorage::GetTemporary()
{
  return true;
}
*/
