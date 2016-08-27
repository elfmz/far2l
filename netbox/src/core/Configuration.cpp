#include <vcl.h>
#pragma hdrstop

#ifndef __linux__
#include <shlobj.h>
#endif
#include <FileInfo.h>

#include <Common.h>
#include <Exceptions.h>
#include "Configuration.h"
#include "PuttyIntf.h"
#include "TextsCore.h"
#include "Interface.h"
#include "CoreMain.h"
#include "WinSCPSecurity.h"
#include <algorithm>

#define BUILD_OFFICIAL

const wchar_t * AutoSwitchNames = L"On;Off;Auto";
const wchar_t * NotAutoSwitchNames = L"Off;On;Auto";

// See http://www.iana.org/assignments/hash-function-text-names/hash-function-text-names.xhtml
const UnicodeString Sha1ChecksumAlg(L"sha-1");
const UnicodeString Sha224ChecksumAlg(L"sha-224");
const UnicodeString Sha256ChecksumAlg(L"sha-256");
const UnicodeString Sha384ChecksumAlg(L"sha-384");
const UnicodeString Sha512ChecksumAlg(L"sha-512");
const UnicodeString Md5ChecksumAlg(L"md5");
// Not defined by IANA
const UnicodeString Crc32ChecksumAlg(L"crc32");

const UnicodeString SshFingerprintType(L"ssh");
const UnicodeString TlsFingerprintType(L"tls");

TConfiguration::TConfiguration() :
  FDontSave(false),
  FChanged(false),
  FUpdating(0),
  FApplicationInfo(nullptr),
  FLogging(false),
  FPermanentLogging(false),
  FLogWindowLines(0),
  FLogFileAppend(false),
  FLogSensitive(false),
  FPermanentLogSensitive(false),
  FLogProtocol(0),
  FPermanentLogProtocol(0),
  FActualLogProtocol(0),
  FLogActions(false),
  FPermanentLogActions(false),
  FConfirmOverwriting(false),
  FConfirmResume(false),
  FAutoReadDirectoryAfterOp(false),
  FSessionReopenAuto(0),
  FSessionReopenBackground(0),
  FSessionReopenTimeout(0),
  FSessionReopenAutoStall(0),
  FProgramIniPathWrittable(0),
  FTunnelLocalPortNumberLow(0),
  FTunnelLocalPortNumberHigh(0),
  FCacheDirectoryChangesMaxSize(0),
  FShowFtpWelcomeMessage(false),
  FTryFtpWhenSshFails(false),
  FScripting(false),
  FDisablePasswordStoring(false),
  FForceBanners(false),
  FDisableAcceptingHostKeys(false),
  FDefaultCollectUsage(false),
  FSessionReopenAutoMaximumNumberOfRetries(0)
{
  FUpdating = 0;
  FStorage = stRegistry;
  FDontSave = false;
  FApplicationInfo = nullptr;
  // FUsage = new TUsage(this);
  // FDefaultCollectUsage = false;
  FScripting = false;

  UnicodeString RandomSeedPath;
  if (!base::GetEnvironmentVariable("APPDATA").IsEmpty())
  {
    RandomSeedPath = "%APPDATA%";
  }
#ifndef __linux__
  else
  {
    RandomSeedPath = ::GetShellFolderPath(CSIDL_LOCAL_APPDATA);
    if (RandomSeedPath.IsEmpty())
    {
      RandomSeedPath = ::GetShellFolderPath(CSIDL_APPDATA);
    }
  }
#endif
  FDefaultRandomSeedFile = ::IncludeTrailingBackslash(RandomSeedPath) + "winscp.rnd";
}

void TConfiguration::Default()
{
  TGuard Guard(FCriticalSection);

  FDisablePasswordStoring = false;
  FForceBanners = false;
  FDisableAcceptingHostKeys = false;

  std::unique_ptr<TRegistryStorage> AdminStorage(new TRegistryStorage(GetRegistryStorageKey(), HKEY_LOCAL_MACHINE));
  try__finally
  {
    if (AdminStorage->OpenRootKey(false))
    {
      LoadAdmin(AdminStorage.get());
      AdminStorage->CloseSubKey();
    }
  }
  __finally
  {
//    delete AdminStorage;
  };

  SetRandomSeedFile(FDefaultRandomSeedFile);
  SetPuttyRegistryStorageKey(OriginalPuttyRegistryStorageKey);
  FConfirmOverwriting = true;
  FConfirmResume = true;
  FAutoReadDirectoryAfterOp = true;
  FSessionReopenAuto = 5000;
  FSessionReopenBackground = 2000;
  FSessionReopenTimeout = 0;
  FSessionReopenAutoStall = 60000;
  FTunnelLocalPortNumberLow = 50000;
  FTunnelLocalPortNumberHigh = 50099;
  FCacheDirectoryChangesMaxSize = 100;
  FShowFtpWelcomeMessage = false;
  FExternalIpAddress.Clear();
  FTryFtpWhenSshFails = true;
  SetCollectUsage(FDefaultCollectUsage);
  FSessionReopenAutoMaximumNumberOfRetries = CONST_DEFAULT_NUMBER_OF_RETRIES;
  FDefaultCollectUsage = false;

  FLogging = false;
  FPermanentLogging = false;
  FLogFileName = GetDefaultLogFileName();
  FPermanentLogFileName = FLogFileName;
  FLogFileAppend = true;
  FLogSensitive = false;
  FPermanentLogSensitive = FLogSensitive;
  FLogWindowLines = 100;
  FLogProtocol = 0;
  FPermanentLogProtocol = FLogProtocol;
  UpdateActualLogProtocol();
  FLogActions = false;
  FPermanentLogActions = false;
  FActionsLogFileName = "%TEMP%" WGOOD_SLASH "&S.xml";
  FPermanentActionsLogFileName = FActionsLogFileName;
  FProgramIniPathWrittable = -1;

  Changed();
}

TConfiguration::~TConfiguration()
{
  DebugAssert(!FUpdating);
  if (FApplicationInfo)
  {
    FreeFileInfo(FApplicationInfo);
  }
  // delete FCriticalSection;
  // delete FUsage;
}

void TConfiguration::UpdateStaticUsage()
{
  // Usage->Set(L"ConfigurationIniFile", (Storage == stIniFile));
  // Usage->Set("Unofficial", IsUnofficial);

  // this is called from here, because we are guarded from calling into
  // master password handler here, see TWinConfiguration::UpdateStaticUsage
  StoredSessions->UpdateStaticUsage();
}

THierarchicalStorage * TConfiguration::CreateConfigStorage()
{
  bool SessionList = false;
  return CreateStorage(SessionList);
}

THierarchicalStorage * TConfiguration::CreateStorage(bool & SessionList)
{
  THierarchicalStorage * Result = nullptr;
  if (GetStorage() == stRegistry)
  {
    Result = new TRegistryStorage(GetRegistryStorageKey());
  }
  else
  {
    ThrowNotImplemented(3005);
    DebugAssert(false);
  }

  if ((FOptionsStorage.get() != nullptr) && (FOptionsStorage->GetCount() > 0))
  {
    if (!SessionList)
    {
//      Result = new TOptionsStorage(FOptionsStorage.get(), ConfigurationSubKey, Result);
    }
    else
    {
      // cannot reuse session list storage for configuration as for it we need
      // the option-override storage above
    }
  }
  else
  {
    // All the above stores can be reused for configuration,
    // if no options-overrides are set
    SessionList = false;
  }

  return Result;
}

UnicodeString TConfiguration::PropertyToKey(const UnicodeString & Property)
{
  // no longer useful
  intptr_t P = Property.LastDelimiter(L".>");
  return Property.SubString(P + 1, Property.Length() - P);
}

#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>") + 1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
    { SCOPE_EXIT { Storage->CloseSubKey(); }; { BLOCK } }
#define KEY(TYPE, VAR) KEYEX(TYPE, VAR, VAR)
#undef REGCONFIG
#define REGCONFIG(CANCREATE) \
  BLOCK(L"Interface", CANCREATE, \
    KEY(String,   RandomSeedFile); \
    KEY(String,   PuttyRegistryStorageKey); \
    KEY(Bool,     ConfirmOverwriting); \
    KEY(Bool,     ConfirmResume); \
    KEY(Bool,     AutoReadDirectoryAfterOp); \
    KEY(Integer,  SessionReopenAuto); \
    KEY(Integer,  SessionReopenBackground); \
    KEY(Integer,  SessionReopenTimeout); \
    KEY(Integer,  SessionReopenAutoStall); \
    KEY(Integer,  TunnelLocalPortNumberLow); \
    KEY(Integer,  TunnelLocalPortNumberHigh); \
    KEY(Integer,  CacheDirectoryChangesMaxSize); \
    KEY(Bool,     ShowFtpWelcomeMessage); \
    KEY(String,   ExternalIpAddress); \
    KEY(Bool,     TryFtpWhenSshFails); \
    KEY(Bool,     CollectUsage); \
    KEY(Integer,  SessionReopenAutoMaximumNumberOfRetries); \
  ); \
  BLOCK(L"Logging", CANCREATE, \
    KEYEX(Bool,  PermanentLogging, Logging); \
    KEYEX(String,PermanentLogFileName, LogFileName); \
    KEY(Bool,    LogFileAppend); \
    KEYEX(Bool,  PermanentLogSensitive, LogSensitive); \
    KEY(Integer, LogWindowLines); \
    KEYEX(Integer,PermanentLogProtocol, LogProtocol); \
    KEYEX(Bool,  PermanentLogActions, LogActions); \
    KEYEX(String,PermanentActionsLogFileName, ActionsLogFileName); \
  );

void TConfiguration::SaveData(THierarchicalStorage * Storage, bool /*All*/)
{
#define KEYEX(TYPE, NAME, VAR) Storage->Write ## TYPE(LASTELEM(UnicodeString(MB_TEXT(#NAME))), Get ## VAR())
  REGCONFIG(true);
#undef KEYEX

  if (Storage->OpenSubKey("Usage", true))
  {
    // FUsage->Save(Storage);
    Storage->CloseSubKey();
  }
}

void TConfiguration::Save()
{
  // only modified, implicit
  DoSave(false, false);
}

void TConfiguration::SaveExplicit()
{
  // only modified, explicit
  DoSave(false, true);
}

void TConfiguration::DoSave(bool All, bool Explicit)
{
  if (FDontSave)
  {
    return;
  }

  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smReadWrite);
    Storage->SetExplicit(Explicit);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), true))
    {
      // if saving to TOptionsStorage, make sure we save everything so that
      // all configuration is properly transferred to the master storage
      bool ConfigAll = All || Storage->GetTemporary();
      SaveData(Storage.get(), ConfigAll);
    }
  }
  __finally
  {
//    delete AStorage;
  };

  Saved();

  if (All)
  {
    StoredSessions->Save(true, Explicit);
  }

  // clean up as last, so that if it fails (read only INI), the saving can proceed
  if (GetStorage() == stRegistry)
  {
    CleanupIniFile();
  }
}

void TConfiguration::Export(const UnicodeString & /*AFileName*/)
{
  ThrowNotImplemented(3004);
  /*
  // not to "append" the export to an existing file
  if (FileExists(FileName))
  {
    DeleteFileChecked(FileName);
  }

  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ExportStorage = NULL;
  try
  {
    ExportStorage = new TIniFileStorage(FileName);
    ExportStorage->AccessMode = smReadWrite;
    ExportStorage->Explicit = true;

    Storage = CreateConfigStorage();
    Storage->AccessMode = smRead;

    CopyData(Storage, ExportStorage);

    if (ExportStorage->OpenSubKey(ConfigurationSubKey, true))
    {
      SaveData(ExportStorage, true);
    }
  }
  __finally
  {
    delete ExportStorage;
    delete Storage;
  }

  StoredSessions->Export(FileName);
  */
}

void TConfiguration::Import(const UnicodeString & /*AFileName*/)
{
  ThrowNotImplemented(3005);

/*THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ImportStorage = NULL;
  try
  {
    ImportStorage = new TIniFileStorage(FileName);
    ImportStorage->AccessMode = smRead;

    Storage = CreateConfigStorage();
    Storage->AccessMode = smReadWrite;
    Storage->Explicit = true;

    CopyData(ImportStorage, Storage);

    Default();
    LoadFrom(ImportStorage);

    if (ImportStorage->OpenSubKey(Configuration->StoredSessionsSubKey, false))
    {
      StoredSessions->Clear();
      StoredSessions->DefaultSettings->Default();
      StoredSessions->Load(ImportStorage);
    }
  }
  __finally
  {
    delete ImportStorage;
    delete Storage;
  }

  // save all and explicit
  DoSave(true, true);*/
}

void TConfiguration::LoadData(THierarchicalStorage * Storage)
{
#define KEYEX(TYPE, NAME, VAR) Set ## VAR(Storage->Read ## TYPE(LASTELEM(UnicodeString(MB_TEXT(#NAME))), Get ## VAR()))
  REGCONFIG(false);
#undef KEYEX

  if (Storage->OpenSubKey("Usage", false))
  {
    // FUsage->Load(Storage);
    Storage->CloseSubKey();
  }

  if (FPermanentLogActions && FPermanentActionsLogFileName.IsEmpty() &&
      FPermanentLogging && !FPermanentLogFileName.IsEmpty())
  {
     FPermanentActionsLogFileName = FPermanentLogFileName;
     FPermanentLogging = false;
     FPermanentLogFileName.Clear();
  }
}

void TConfiguration::LoadAdmin(THierarchicalStorage * Storage)
{
  FDisablePasswordStoring = Storage->ReadBool("DisablePasswordStoring", FDisablePasswordStoring);
  FForceBanners = Storage->ReadBool("ForceBanners", FForceBanners);
  FDisableAcceptingHostKeys = Storage->ReadBool("DisableAcceptingHostKeys", FDisableAcceptingHostKeys);
  FDefaultCollectUsage = Storage->ReadBool("DefaultCollectUsage", FDefaultCollectUsage);
}

void TConfiguration::LoadFrom(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(GetConfigurationSubKey(), false))
  {
    LoadData(Storage);
    Storage->CloseSubKey();
  }
}

void TConfiguration::Load(THierarchicalStorage * Storage)
{
  TGuard Guard(FCriticalSection);
  TStorageAccessMode StorageAccessMode = Storage->GetAccessMode();
  try__finally
  {
    SCOPE_EXIT
    {
      Storage->SetAccessMode(StorageAccessMode);
    };
    Storage->SetAccessMode(smRead);
    LoadFrom(Storage);
  }
  __finally
  {
//    Storage->AccessMode = StorageAccessMode;
  };
}

void TConfiguration::CopyData(THierarchicalStorage * Source,
  THierarchicalStorage * Target)
{
  std::unique_ptr<TStrings > Names(new TStringList());
  try__finally
  {
    if (Source->OpenSubKey(GetConfigurationSubKey(), false))
    {
      if (Target->OpenSubKey(GetConfigurationSubKey(), true))
      {
        if (Source->OpenSubKey("CDCache", false))
        {
          if (Target->OpenSubKey("CDCache", true))
          {
            Names->Clear();
            Source->GetValueNames(Names.get());

            for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
            {
              Target->WriteBinaryData(Names->GetString(Index),
                Source->ReadBinaryData(Names->GetString(Index)));
            }

            Target->CloseSubKey();
          }
          Source->CloseSubKey();
        }

        if (Source->OpenSubKey("Banners", false))
        {
          if (Target->OpenSubKey("Banners", true))
          {
            Names->Clear();
            Source->GetValueNames(Names.get());

            for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
            {
              Target->WriteString(Names->GetString(Index),
                Source->ReadString(Names->GetString(Index), L""));
            }

            Target->CloseSubKey();
          }
          Source->CloseSubKey();
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }

    if (Source->OpenSubKey(GetSshHostKeysSubKey(), false))
    {
      if (Target->OpenSubKey(GetSshHostKeysSubKey(), true))
      {
        Names->Clear();
        Source->GetValueNames(Names.get());

        for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
        {
          Target->WriteStringRaw(Names->GetString(Index),
            Source->ReadStringRaw(Names->GetString(Index), L""));
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }
  }
  __finally
  {
//    delete Names;
  };
}

void TConfiguration::LoadDirectoryChangesCache(const UnicodeString & SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smRead);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
        Storage->OpenSubKey("CDCache", false) &&
        Storage->ValueExists(SessionKey))
    {
      DirectoryChangesCache->Deserialize(Storage->ReadBinaryData(SessionKey));
    }
  }
  __finally
  {
//    delete Storage;
  };
}

void TConfiguration::SaveDirectoryChangesCache(const UnicodeString & SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey("CDCache", true))
    {
      UnicodeString Data;
      DirectoryChangesCache->Serialize(Data);
      Storage->WriteBinaryData(SessionKey, Data);
    }
  }
  __finally
  {
//    delete Storage;
  };
}

UnicodeString TConfiguration::BannerHash(const UnicodeString & Banner) const
{
  RawByteString Result;
  Result.SetLength(16);
  md5checksum(
    reinterpret_cast<const char *>(Banner.c_str()), static_cast<int>(Banner.Length() * sizeof(wchar_t)),
    reinterpret_cast<uint8_t *>(const_cast<char *>(Result.c_str())));
  return BytesToHex(Result);
}

bool TConfiguration::ShowBanner(const UnicodeString & SessionKey,
  const UnicodeString & Banner)
{
  bool Result = false;
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smRead);
    Result =
      !Storage->OpenSubKey(GetConfigurationSubKey(), false) ||
      !Storage->OpenSubKey("Banners", false) ||
      !Storage->ValueExists(SessionKey) ||
      (Storage->ReadString(SessionKey, L"") != BannerHash(Banner));
    return Result;
  }
  __finally
  {
//    delete Storage;
  };

  return Result;
}

void TConfiguration::NeverShowBanner(const UnicodeString & SessionKey,
  const UnicodeString & Banner)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smReadWrite);

    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey("Banners", true))
    {
      Storage->WriteString(SessionKey, BannerHash(Banner));
    }
  }
  __finally
  {
//    delete Storage;
  };
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::FormatFingerprintKey(const UnicodeString & SiteKey, const UnicodeString & FingerprintType) const
{
  return FORMAT(L"%s:%s", SiteKey.c_str(), FingerprintType.c_str());
}

void TConfiguration::RememberLastFingerprint(const UnicodeString & SiteKey, const UnicodeString & FingerprintType, const UnicodeString & Fingerprint)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);

  if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
      Storage->OpenSubKey("LastFingerprints", true))
  {
    UnicodeString FingerprintKey = FormatFingerprintKey(SiteKey, FingerprintType);
    Storage->WriteString(FingerprintKey, Fingerprint);
  }
}

UnicodeString TConfiguration::GetLastFingerprint(const UnicodeString & SiteKey, const UnicodeString & FingerprintType)
{
  UnicodeString Result;

  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  Storage->SetAccessMode(smRead);

  if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
      Storage->OpenSubKey("LastFingerprints", false))
  {
    UnicodeString FingerprintKey = FormatFingerprintKey(SiteKey, FingerprintType);
    Result = Storage->ReadString(FingerprintKey, L"");
  }
  return Result;
}

void TConfiguration::Changed()
{
  if (FUpdating == 0)
  {
    if (GetOnChange())
    {
      GetOnChange()(this);
    }
  }
  else
  {
    FChanged = true;
  }
}

void TConfiguration::BeginUpdate()
{
  if (FUpdating == 0)
  {
    FChanged = false;
  }
  FUpdating++;
  // Greater value would probably indicate some nesting problem in code
  DebugAssert(FUpdating < 6);
}

void TConfiguration::EndUpdate()
{
  DebugAssert(FUpdating > 0);
  FUpdating--;
  if ((FUpdating == 0) && FChanged)
  {
    FChanged = false;
    Changed();
  }
}

void TConfiguration::CleanupConfiguration()
{
  try
  {
    CleanupRegistry(GetConfigurationSubKey());
    if (GetStorage() == stRegistry)
    {
      FDontSave = true;
    }
  }
  catch (Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_CONFIG_ERROR));
  }
}

void TConfiguration::CleanupRegistry(const UnicodeString & CleanupSubKey)
{
  std::unique_ptr<TRegistryStorage> Registry(new TRegistryStorage(GetRegistryStorageKey()));
  try__finally
  {
    Registry->RecursiveDeleteSubKey(CleanupSubKey);
  }
  __finally
  {
//    delete Registry;
  };
}

void TConfiguration::CleanupHostKeys()
{
  try
  {
    CleanupRegistry(GetSshHostKeysSubKey());
  }
  catch (Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_HOSTKEYS_ERROR));
  }
}

void TConfiguration::CleanupRandomSeedFile()
{
  try
  {
    DontSaveRandomSeed();
    if (::FileExists(ApiPath(GetRandomSeedFileName())))
    {
      DeleteFileChecked(GetRandomSeedFileName());
    }
  }
  catch (Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_SEEDFILE_ERROR));
  }
}

void TConfiguration::CleanupIniFile()
{
#if 0
  try
  {
    if (::FileExists(ApiPath(GetIniFileStorageNameForReading())))
    {
      DeleteFileChecked(GetIniFileStorageNameForReading());
    }
    if (GetStorage() == stIniFile)
    {
      FDontSave = true;
    }
  }
  catch (Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_INIFILE_ERROR));
  }
#endif
}

void TConfiguration::DontSave()
{
  FDontSave = true;
}

RawByteString TConfiguration::EncryptPassword(const UnicodeString & Password, const UnicodeString & Key)
{
  if (Password.IsEmpty())
  {
    return RawByteString();
  }
  else
  {
    return ::EncryptPassword(Password, Key);
  }
}

UnicodeString TConfiguration::DecryptPassword(const RawByteString & Password, const UnicodeString & Key)
{
  if (Password.IsEmpty())
  {
    return UnicodeString();
  }
  else
  {
    return ::DecryptPassword(Password, Key);
  }
}

RawByteString TConfiguration::StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & /*Key*/)
{
  return Password;
}

UnicodeString TConfiguration::GetOSVersionStr() const
{
  UnicodeString Result;
#ifndef __linux__
  OSVERSIONINFO OSVersionInfo;
  OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
  if (::GetVersionEx(&OSVersionInfo) != 0)
  {
    Result = FORMAT(L"%d.%d.%d", int(OSVersionInfo.dwMajorVersion),
      int(OSVersionInfo.dwMinorVersion), int(OSVersionInfo.dwBuildNumber));
    UnicodeString CSDVersion = OSVersionInfo.szCSDVersion;
    if (!CSDVersion.IsEmpty())
    {
      Result += L" " + CSDVersion;
    }
    UnicodeString ProductName = WindowsProductName();
    if (!ProductName.IsEmpty())
    {
      Result += L" - " + ProductName;
    }
  }
#endif
  return Result;
}

TVSFixedFileInfo * TConfiguration::GetFixedApplicationInfo() const
{
  return GetFixedFileInfo(GetApplicationInfo());
}

intptr_t TConfiguration::GetCompoundVersion() const
{
#ifndef __linux__
  TVSFixedFileInfo * FileInfo = GetFixedApplicationInfo();
  if (FileInfo)
  {
    return CalculateCompoundVersion(
      HIWORD(FileInfo->dwFileVersionMS), LOWORD(FileInfo->dwFileVersionMS),
      HIWORD(FileInfo->dwFileVersionLS), LOWORD(FileInfo->dwFileVersionLS));
  }
  else
#endif
    return 0;
}

UnicodeString TConfiguration::ModuleFileName() const
{
  ThrowNotImplemented(204);
  return L"";
}

void * TConfiguration::GetFileApplicationInfo(const UnicodeString & AFileName) const
{
  void * Result;
  if (AFileName.IsEmpty())
  {
    if (!FApplicationInfo)
    {
      FApplicationInfo = CreateFileInfo(ModuleFileName());
    }
    Result = FApplicationInfo;
  }
  else
  {
    Result = CreateFileInfo(AFileName);
  }
  return Result;
}

void * TConfiguration::GetApplicationInfo() const
{
  return GetFileApplicationInfo(L"");
}

UnicodeString TConfiguration::GetFileProductName(const UnicodeString & AFileName) const
{
  return GetFileFileInfoString(L"ProductName", AFileName);
}

UnicodeString TConfiguration::GetFileCompanyName(const UnicodeString & AFileName) const
{
  // particularly in IDE build, company name is empty
  return GetFileFileInfoString(L"CompanyName", AFileName, true);
}

UnicodeString TConfiguration::GetProductName() const
{
  return GetFileProductName(L"");
}

UnicodeString TConfiguration::GetCompanyName() const
{
  return GetFileCompanyName(L"");
}

UnicodeString TConfiguration::GetFileProductVersion(const UnicodeString & AFileName) const
{
  return TrimVersion(GetFileFileInfoString(L"ProductVersion", AFileName));
}

UnicodeString TConfiguration::GetFileDescription(const UnicodeString & AFileName)
{
  return GetFileFileInfoString(L"FileDescription", AFileName);
}

UnicodeString TConfiguration::GetFileProductVersion() const
{
  return GetFileProductVersion(L"");
}

UnicodeString TConfiguration::GetReleaseType() const
{
  return GetFileInfoString(L"ReleaseType");
}

bool TConfiguration::GetIsUnofficial() const
{
  #ifdef BUILD_OFFICIAL
  return false;
  #else
  return true;
  #endif
}

UnicodeString TConfiguration::GetProductVersionStr() const
{
  UnicodeString Result;
#ifndef __linux__
  TGuard Guard(FCriticalSection);
  try
  {
    TVSFixedFileInfo * Info = GetFixedApplicationInfo();
    /*return FMTLOAD(VERSION,
      HIWORD(Info->dwFileVersionMS),
      LOWORD(Info->dwFileVersionMS),
      HIWORD(Info->dwFileVersionLS),
      LOWORD(Info->dwFileVersionLS));*/
    UnicodeString BuildStr;
    if (!GetIsUnofficial())
    {
      BuildStr = LoadStr(VERSION_BUILD);
    }
    else
    {
      #ifdef _DEBUG
      BuildStr = LoadStr(VERSION_DEBUG_BUILD);
      #else
      BuildStr = LoadStr(VERSION_DEV_BUILD);
      #endif
    }

    int Build = LOWORD(Info->dwFileVersionLS);
    if (Build > 0)
    {
      BuildStr += L" " + ::IntToStr(Build);
    }

#if 0
    #ifndef BUILD_OFFICIAL
    UnicodeString BuildDate = __DATE__;
    UnicodeString MonthStr = CutToChar(BuildDate, L' ', true);
    int Month = ParseShortEngMonthName(MonthStr);
    int Day = StrToInt64(CutToChar(BuildDate, L' ', true));
    int Year = StrToInt64(Trim(BuildDate));
    UnicodeString DateStr = FORMAT("%d-%2.2d-%2.2d", Year, Month, Day);
    AddToList(BuildStr, DateStr, L" ");
    #endif
#endif

    UnicodeString FullVersion = GetProductVersion();

    UnicodeString AReleaseType = GetReleaseType();
    if (DebugAlwaysTrue(!AReleaseType.IsEmpty()) &&
        !SameText(AReleaseType, L"stable") &&
        !SameText(AReleaseType, L"development"))
    {
      FullVersion += L" " + AReleaseType;
    }

    Result = FMTLOAD(VERSION2, GetProductVersion().c_str(), Build);

#if 0
    #ifndef BUILD_OFFICIAL
    Result += L" " + LoadStr(VERSION_DONT_DISTRIBUTE);
    #endif
#endif
  }
  catch (Exception & E)
  {
    throw ExtException(&E, "Can't get application version");
  }
#endif
  return Result;
}

UnicodeString TConfiguration::GetFileVersion(const UnicodeString & FileName)
{
  UnicodeString Result;
  void * FileInfo = CreateFileInfo(FileName);
  try__finally
  {
    SCOPE_EXIT
    {
      FreeFileInfo(FileInfo);
    };
    Result = GetFileVersion(GetFixedFileInfo(FileInfo));
  }
  __finally
  {
    FreeFileInfo(FileInfo);
  };
  return Result;
}

UnicodeString TConfiguration::GetFileVersion(TVSFixedFileInfo * Info)
{
#ifndef __linux__
  TGuard Guard(FCriticalSection);
  try
  {
    UnicodeString Result =
      FormatVersion(
        HIWORD(Info->dwFileVersionMS),
        LOWORD(Info->dwFileVersionMS),
        HIWORD(Info->dwFileVersionLS));
    return Result;
  }
  catch (Exception &E)
  {
    throw ExtException(&E, L"Can't get file version");
  }
#else
  return UnicodeString(L"1.0");
#endif
}

UnicodeString TConfiguration::GetProductVersion() const
{
  TGuard Guard(FCriticalSection);
  UnicodeString Result;
#ifndef __linux__
  try
  {
    TVSFixedFileInfo * Info = GetFixedApplicationInfo();
    if (Info)
    {
      Result = FormatVersion(
        HIWORD(Info->dwFileVersionMS),
        LOWORD(Info->dwFileVersionMS),
        HIWORD(Info->dwFileVersionLS));
    }
  }
  catch (Exception & E)
  {
    throw ExtException(&E, "Can't get application version");
  }
#endif
  return Result;
}

UnicodeString TConfiguration::GetVersion()
{
  return GetFileVersion(GetFixedApplicationInfo());
}

UnicodeString TConfiguration::GetFileFileInfoString(const UnicodeString & AKey,
  const UnicodeString & AFileName, bool AllowEmpty) const
{
  TGuard Guard(FCriticalSection);

  UnicodeString Result;
  void * Info = GetFileApplicationInfo(AFileName);
  try__finally
  {
    SCOPE_EXIT
    {
      if (!AFileName.IsEmpty() && Info)
      {
        FreeFileInfo(Info);
      }
    };
    if ((Info != nullptr) && (GetTranslationCount(Info) > 0))
    {
      TTranslation Translation = GetTranslation(Info, 0);
      try
      {
        Result = ::GetFileInfoString(Info, Translation, AKey, AllowEmpty);
      }
      catch (const std::exception & e)
      {
        (void)e;
        DEBUG_PRINTF("Error: %s", ::MB2W(e.what()).c_str());
        Result.Clear();
      }
    }
    else
    {
      DebugAssert(!AFileName.IsEmpty());
    }
  }
  __finally
  {
    if (!AFileName.IsEmpty())
    {
      FreeFileInfo(Info);
    }
  };
  return Result;
}

UnicodeString TConfiguration::GetFileInfoString(const UnicodeString & Key) const
{
  return GetFileFileInfoString(Key, L"");
}

UnicodeString TConfiguration::GetRegistryStorageKey() const
{
  return GetRegistryKey();
}

void TConfiguration::SetNulStorage()
{
  FStorage = stNul;
}

void TConfiguration::SetDefaultStorage()
{
  FStorage = stDetect;
}

/*
void TConfiguration::SetIniFileStorageName(const UnicodeString & Value)
{
  FIniFileStorageName = Value;
  FStorage = stIniFile;
}

UnicodeString TConfiguration::GetIniFileStorageNameForReading()
{
  return GetIniFileStorageName(true);
}

UnicodeString TConfiguration::GetIniFileStorageNameForReadingWriting()
{
  return GetIniFileStorageName(false);
}

UnicodeString TConfiguration::GetIniFileStorageName(bool ReadingOnly)
{
  if (FIniFileStorageName.IsEmpty())
  {
    UnicodeString ProgramPath = ParamStr(0);

    UnicodeString ProgramIniPath = ChangeFileExt(ProgramPath, L".ini");

    UnicodeString IniPath;
    if (::FileExists(ApiPath(ProgramIniPath)))
    {
      IniPath = ProgramIniPath;
    }
    else
    {
      UnicodeString AppDataIniPath =
        IncludeTrailingBackslash(GetShellFolderPath(CSIDL_APPDATA)) +
        ::ExtractFileName(ProgramIniPath);
      if (::FileExists(ApiPath(AppDataIniPath)))
      {
        IniPath = AppDataIniPath;
      }
      else
      {
        // avoid expensive test if we are interested in existing files only
        if (!ReadingOnly && (FProgramIniPathWrittable < 0))
        {
          UnicodeString ProgramDir = ExtractFilePath(ProgramPath);
          FProgramIniPathWrittable = IsDirectoryWriteable(ProgramDir) ? 1 : 0;
        }

        // does not really matter what we return when < 0
        IniPath = (FProgramIniPathWrittable == 0) ? AppDataIniPath : ProgramIniPath;
      }
    }

    // BACKWARD COMPATIBILITY with 4.x
    if (FVirtualIniFileStorageName.IsEmpty() &&
        TPath::IsDriveRooted(IniPath))
    {
      UnicodeString LocalAppDataPath = GetShellFolderPath(CSIDL_LOCAL_APPDATA);
      // virtual store for non-system drives have a different virtual store,
      // do not bother about them
      if (TPath::IsDriveRooted(LocalAppDataPath) &&
          SameText(ExtractFileDrive(IniPath), ExtractFileDrive(LocalAppDataPath)))
      {
        FVirtualIniFileStorageName =
          IncludeTrailingBackslash(LocalAppDataPath) +
          L"VirtualStore" WGOOD_SLASH "" +
          IniPath.SubString(4, IniPath.Length() - 3);
      }
    }

    if (!FVirtualIniFileStorageName.IsEmpty() &&
        ::FileExists(ApiPath(FVirtualIniFileStorageName)))
    {
      return FVirtualIniFileStorageName;
    }
    else
    {
      return IniPath;
    }
  }
  else
  {
    return FIniFileStorageName;
  }
}
*/

void TConfiguration::SetOptionsStorage(TStrings * Value)
{
  if (FOptionsStorage.get() == nullptr)
  {
    FOptionsStorage.reset(new TStringList());
  }
  FOptionsStorage->AddStrings(Value);
}

TStrings * TConfiguration::GetOptionsStorage()
{
  return FOptionsStorage.get();
}

UnicodeString TConfiguration::GetPuttySessionsKey() const
{
  return GetPuttyRegistryStorageKey() + "" WGOOD_SLASH "Sessions";
}

UnicodeString TConfiguration::GetStoredSessionsSubKey() const
{
  return "Sessions";
}

UnicodeString TConfiguration::GetSshHostKeysSubKey() const
{
  return "SshHostKeys";
}

UnicodeString TConfiguration::GetConfigurationSubKey() const
{
  return "Configuration";
}

UnicodeString TConfiguration::GetRootKeyStr() const
{
  return RootKeyToStr(HKEY_CURRENT_USER);
}

void TConfiguration::SetStorage(TStorage Value)
{
  if (FStorage != Value)
  {
    TStorage StorageBak = FStorage;
    try
    {
      std::unique_ptr<THierarchicalStorage> SourceStorage(CreateConfigStorage());
      std::unique_ptr<THierarchicalStorage> TargetStorage(CreateConfigStorage());
      try__finally
      {
        SourceStorage->SetAccessMode(smRead);

        FStorage = Value;

        TargetStorage->SetAccessMode(smReadWrite);
        TargetStorage->SetExplicit(true);

        // copy before save as it removes the ini file,
        // when switching from ini to registry
        CopyData(SourceStorage.get(), TargetStorage.get());
      }
      __finally
      {
//        delete SourceStorage;
//        delete TargetStorage;
      };
      // save all and explicit,
      // this also removes an INI file, when switching to registry storage
      DoSave(true, true);
    }
    catch (...)
    {
      // If this fails, do not pretend that storage was switched.
      // For instance:
      // - When writing to an INI file fails (unlikely, as we fallback to user profile)
      // - When removing INI file fails, when switching to registry
      //   (possible, when the INI file is in Program Files folder)
      FStorage = StorageBak;
      throw;
    }
  }
}

void TConfiguration::Saved()
{
  // nothing
}

TStorage TConfiguration::GetStorage() const
{
  if (FStorage == stDetect)
  {
    /*if (::FileExists(ApiPath(IniFileStorageNameForReading)))
    {
      FStorage = stIniFile;
    }
    else*/
    {
      FStorage = stRegistry;
    }
  }
  return FStorage;
}

TStoredSessionList * TConfiguration::SelectFilezillaSessionsForImport(
  TStoredSessionList * Sessions, UnicodeString & Error)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(new TStoredSessionList(true));
  ImportSessionList->SetDefaultSettings(Sessions->GetDefaultSettings());

#ifndef __linux__
  UnicodeString AppDataPath = GetShellFolderPath(CSIDL_APPDATA);
#else
  UnicodeString AppDataPath = L"./";
#endif
  UnicodeString FilezillaSiteManagerFile =
    IncludeTrailingBackslash(AppDataPath) + L"FileZilla" WGOOD_SLASH "sitemanager.xml";

  if (FileExists(ApiPath(FilezillaSiteManagerFile)))
  {
    ImportSessionList->ImportFromFilezilla(FilezillaSiteManagerFile);

    if (ImportSessionList->GetCount() > 0)
    {
      ImportSessionList->SelectSessionsToImport(Sessions, true);
    }
    else
    {
      Error = FMTLOAD(FILEZILLA_NO_SITES, FilezillaSiteManagerFile.c_str());
    }
  }
  else
  {
    Error = FMTLOAD(FILEZILLA_SITE_MANAGER_NOT_FOUND, FilezillaSiteManagerFile.c_str());
  }

  return ImportSessionList.release();
}

bool TConfiguration::AnyFilezillaSessionForImport(TStoredSessionList * Sessions)
{
  try
  {
    UnicodeString Error;
    std::unique_ptr<TStoredSessionList> Sesssions(SelectFilezillaSessionsForImport(Sessions, Error));
    return (Sesssions->GetCount() > 0);
  }
  catch (...)
  {
    return false;
  }
}

void TConfiguration::SetRandomSeedFile(const UnicodeString & Value)
{
  if (GetRandomSeedFile() != Value)
  {
    UnicodeString PrevRandomSeedFileName = GetRandomSeedFileName();

    FRandomSeedFile = Value;

    // never allow empty seed file to avoid Putty trying to reinitialize the path
    if (GetRandomSeedFileName().IsEmpty())
    {
      FRandomSeedFile = FDefaultRandomSeedFile;
    }

    if (!PrevRandomSeedFileName.IsEmpty() &&
        (PrevRandomSeedFileName != GetRandomSeedFileName()) &&
        ::FileExists(ApiPath(PrevRandomSeedFileName)))
    {
      // ignore any error
      ::RemoveFile(ApiPath(PrevRandomSeedFileName));
    }
  }
}

UnicodeString TConfiguration::GetRandomSeedFileName() const
{
  // StripPathQuotes should not be needed as we do not feed quotes anymore
  return StripPathQuotes(::ExpandEnvironmentVariables(FRandomSeedFile)).Trim();
}

void TConfiguration::SetExternalIpAddress(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY(ExternalIpAddress);
}

void TConfiguration::SetTryFtpWhenSshFails(bool Value)
{
  SET_CONFIG_PROPERTY(TryFtpWhenSshFails);
}

void TConfiguration::SetPuttyRegistryStorageKey(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY(PuttyRegistryStorageKey);
}

TEOLType TConfiguration::GetLocalEOLType() const
{
  return eolCRLF;
}

bool TConfiguration::GetCollectUsage() const
{
  return false; // FUsage->Collect;
}

void TConfiguration::SetCollectUsage(bool /*Value*/)
{
  // FUsage->Collect = Value;
}

void TConfiguration::TemporaryLogging(const UnicodeString & ALogFileName)
{
  if (SameText(ExtractFileExt(ALogFileName), L".xml"))
  {
    TemporaryActionsLogging(ALogFileName);
  }
  else
  {
    FLogging = true;
    FLogFileName = ALogFileName;
    UpdateActualLogProtocol();
  }
}

void TConfiguration::TemporaryActionsLogging(const UnicodeString & ALogFileName)
{
  FLogActions = true;
  FActionsLogFileName = ALogFileName;
}

void TConfiguration::TemporaryLogProtocol(intptr_t ALogProtocol)
{
  FLogProtocol = ALogProtocol;
  UpdateActualLogProtocol();
}

void TConfiguration::TemporaryLogSensitive(bool ALogSensitive)
{
  FLogSensitive = ALogSensitive;
}

void TConfiguration::SetLogging(bool Value)
{
  if (GetLogging() != Value)
  {
    FPermanentLogging = Value;
    FLogging = Value;
    UpdateActualLogProtocol();
    Changed();
  }
}

void TConfiguration::SetLogFileName(const UnicodeString & Value)
{
  if (GetLogFileName() != Value)
  {
    FPermanentLogFileName = Value;
    FLogFileName = Value;
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetActionsLogFileName(const UnicodeString & Value)
{
  if (GetActionsLogFileName() != Value)
  {
    FPermanentActionsLogFileName = Value;
    FActionsLogFileName = Value;
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetLogToFile(bool Value)
{
  if (Value != GetLogToFile())
  {
    SetLogFileName(Value ? GetDefaultLogFileName() : UnicodeString(L""));
    Changed();
  }
}

bool TConfiguration::GetLogToFile() const
{
  return !GetLogFileName().IsEmpty();
}

void TConfiguration::UpdateActualLogProtocol()
{
  FActualLogProtocol = FLogging ? FLogProtocol : 0;
}

void TConfiguration::SetLogProtocol(intptr_t Value)
{
  if (GetLogProtocol() != Value)
  {
    FPermanentLogProtocol = Value;
    FLogProtocol = Value;
    Changed();
    UpdateActualLogProtocol();
  }
}

void TConfiguration::SetLogActions(bool Value)
{
  if (GetLogActions() != Value)
  {
    FPermanentLogActions = Value;
    FLogActions = Value;
    Changed();
  }
}

void TConfiguration::SetLogFileAppend(bool Value)
{
  SET_CONFIG_PROPERTY(LogFileAppend);
}

void TConfiguration::SetLogSensitive(bool Value)
{
  if (GetLogSensitive() != Value)
  {
    FPermanentLogSensitive = Value;
    FLogSensitive = Value;
    Changed();
  }
}

void TConfiguration::SetLogWindowLines(intptr_t Value)
{
  SET_CONFIG_PROPERTY(LogWindowLines);
}

void TConfiguration::SetLogWindowComplete(bool Value)
{
  if (Value != GetLogWindowComplete())
  {
    SetLogWindowLines(Value ? 0 : 50);
    Changed();
  }
}

bool TConfiguration::GetLogWindowComplete() const
{
  return static_cast<bool>(GetLogWindowLines() == 0);
}

UnicodeString TConfiguration::GetDefaultLogFileName() const
{
  return "%TEMP%" WGOOD_SLASH "&S.log";
}

void TConfiguration::SetConfirmOverwriting(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmOverwriting);
}

bool TConfiguration::GetConfirmOverwriting() const
{
  TGuard Guard(FCriticalSection);
  return FConfirmOverwriting;
}

void TConfiguration::SetConfirmResume(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmResume);
}

bool TConfiguration::GetConfirmResume() const
{
  TGuard Guard(FCriticalSection);
  return FConfirmResume;
}

void TConfiguration::SetAutoReadDirectoryAfterOp(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(AutoReadDirectoryAfterOp);
}

bool TConfiguration::GetAutoReadDirectoryAfterOp() const
{
  TGuard Guard(FCriticalSection);
  return FAutoReadDirectoryAfterOp;
}

UnicodeString TConfiguration::GetTimeFormat() const
{
  return "h:nn:ss";
}

UnicodeString TConfiguration::GetPartialExt() const
{
  return PARTIAL_EXT;
}

UnicodeString TConfiguration::GetDefaultKeyFile() const
{
  return L"";
}

bool TConfiguration::GetRememberPassword() const
{
  return false;
}

void TConfiguration::SetSessionReopenAuto(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAuto);
}

void TConfiguration::SetSessionReopenAutoMaximumNumberOfRetries(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoMaximumNumberOfRetries);
}

void TConfiguration::SetSessionReopenBackground(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenBackground);
}

void TConfiguration::SetSessionReopenTimeout(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenTimeout);
}

void TConfiguration::SetSessionReopenAutoStall(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoStall);
}

void TConfiguration::SetTunnelLocalPortNumberLow(intptr_t Value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberLow);
}

void TConfiguration::SetTunnelLocalPortNumberHigh(intptr_t Value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberHigh);
}

void TConfiguration::SetCacheDirectoryChangesMaxSize(intptr_t Value)
{
  SET_CONFIG_PROPERTY(CacheDirectoryChangesMaxSize);
}

void TConfiguration::SetShowFtpWelcomeMessage(bool Value)
{
  SET_CONFIG_PROPERTY(ShowFtpWelcomeMessage);
}

UnicodeString TConfiguration::GetPermanentLogFileName() const
{
  return FPermanentLogFileName;
}

void TConfiguration::SetPermanentLogFileName(const UnicodeString & Value)
{
  FPermanentLogFileName = Value;
}

UnicodeString TConfiguration::GetPermanentActionsLogFileName() const
{
  return FPermanentActionsLogFileName;
}

void TConfiguration::SetPermanentActionsLogFileName(const UnicodeString & Value)
{
  FPermanentActionsLogFileName = Value;
}

bool TConfiguration::GetPersistent() const
{
  return (GetStorage() != stNul);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TShortCuts::Add(const TShortCut & ShortCut)
{
  FShortCuts.push_back(ShortCut);
}

bool TShortCuts::Has(const TShortCut & ShortCut) const
{
  return std::find(FShortCuts.begin(), FShortCuts.end(), ShortCut) != FShortCuts.end();
}

NB_IMPLEMENT_CLASS(TConfiguration, NB_GET_CLASS_INFO(TObject), nullptr)
