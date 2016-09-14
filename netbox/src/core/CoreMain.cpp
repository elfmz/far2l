#include <vcl.h>


#include <Common.h>

#include "CoreMain.h"
#include "Interface.h"
#include "Configuration.h"
#include "PuttyIntf.h"
#include "Cryptography.h"
#include <DateUtils.hpp>
#ifndef NO_FILEZILLA
#ifndef __linux__
#include "FileZillaIntf.h"
#else
#include "libfilezilla/libfilezilla.hpp"
#endif
#endif
#include "WebDAVFileSystem.h"

TStoredSessionList * StoredSessions = nullptr;

TQueryButtonAlias::TQueryButtonAlias() :
  Button(0),
  OnClick(nullptr),
  GroupWith(-1),
  Default(false),
  GrouppedShiftState(ssShift),
  ElevationRequired(false)
{
}

TQueryParams::TQueryParams(uintptr_t AParams, const UnicodeString & AHelpKeyword) :
  Aliases(nullptr),
  AliasesCount(0),
  Params(AParams),
  Timer(0),
  TimerEvent(nullptr),
  TimerMessage(L""),
  TimerAnswers(0),
  TimerQueryType(static_cast<TQueryType>(-1)),
  Timeout(0),
  TimeoutAnswer(0),
  NoBatchAnswers(0),
  HelpKeyword(AHelpKeyword)
{
}

TQueryParams::TQueryParams(const TQueryParams & Source)
{
  Assign(Source);
}

void TQueryParams::Assign(const TQueryParams & Source)
{
  Params = Source.Params;
  Aliases = Source.Aliases;
  AliasesCount = Source.AliasesCount;
  Timer = Source.Timer;
  TimerEvent = Source.TimerEvent;
  TimerMessage = Source.TimerMessage;
  TimerAnswers = Source.TimerAnswers;
  TimerQueryType = Source.TimerQueryType;
  Timeout = Source.Timeout;
  TimeoutAnswer = Source.TimeoutAnswer;
  NoBatchAnswers = Source.NoBatchAnswers;
  HelpKeyword = Source.HelpKeyword;
}

TQueryParams & TQueryParams::operator=(const TQueryParams & other)
{
  Assign(other);
  return *this;
}

bool IsAuthenticationPrompt(TPromptKind Kind)
{
  return
    (Kind == pkUserName) || (Kind == pkPassphrase) || (Kind == pkTIS) ||
    (Kind == pkCryptoCard) || (Kind == pkKeybInteractive) ||
    (Kind == pkPassword) || (Kind == pkNewPassword);
}

bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, TStrings * Prompts)
{
  return
    (Prompts->GetCount() == 1) && FLAGCLEAR(reinterpret_cast<intptr_t>(Prompts->GetObj(0)), pupEcho) &&
    ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
     (Kind == pkTIS) || (Kind == pkCryptoCard));
}

bool IsPasswordPrompt(TPromptKind Kind, TStrings * Prompts)
{
  return
    IsPasswordOrPassphrasePrompt(Kind, Prompts) &&
    (Kind != pkPassphrase);
}

TConfiguration * GetConfiguration()
{
  static TConfiguration * Configuration = nullptr;
  if (Configuration == nullptr)
  {
    Configuration = CreateConfiguration();
  }
  return Configuration;
}

void DeleteConfiguration()
{
  static bool ConfigurationDeleted = false;
  if (!ConfigurationDeleted)
  {
    TConfiguration * Conf = GetConfiguration();
    SAFE_DESTROY(Conf);
    ConfigurationDeleted = true;
  }
}

void CoreLoad()
{
  bool SessionList = false;
  std::unique_ptr<THierarchicalStorage> SessionsStorage(GetConfiguration()->CreateStorage(SessionList));
  THierarchicalStorage * ConfigStorage = nullptr;
  std::unique_ptr<THierarchicalStorage> ConfigStorageAuto;
  if (!SessionList)
  {
    // can reuse this for configuration
    ConfigStorage = SessionsStorage.get();
  }
  else
  {
    ConfigStorageAuto.reset(GetConfiguration()->CreateConfigStorage());
    ConfigStorage = ConfigStorageAuto.get();
  }

  DebugAssert(GetConfiguration() != nullptr);

  try
  {
    GetConfiguration()->Load(ConfigStorage);
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
  }

  // should be noop, unless exception occurred above
  ConfigStorage->CloseAll();

  StoredSessions = new TStoredSessionList();

  try
  {
    if (SessionsStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false))
    {
      StoredSessions->Load(SessionsStorage.get());
    }
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
  }
}

void CoreInitialize()
{
  WinInitialize();
  Randomize();
  CryptographyInitialize();

  // we do not expect configuration re-creation
  DebugAssert(GetConfiguration() != nullptr);
  // configuration needs to be created and loaded before putty is initialized,
  // so that random seed path is known
//  Configuration = CreateConfiguration();

  PuttyInitialize();
  #ifndef NO_FILEZILLA
  TFileZillaIntf::Initialize();
  #endif
  NeonInitialize();

  CoreLoad();
}

void CoreFinalize()
{
  try
  {
    GetConfiguration()->Save();
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
  }

  NeonFinalize();
  #ifndef NO_FILEZILLA
  TFileZillaIntf::Finalize();
  #endif
  PuttyFinalize();

  SAFE_DESTROY(StoredSessions);
  DeleteConfiguration();

  CryptographyFinalize();
  WinFinalize();
}

void CoreSetResourceModule(void * ResourceHandle)
{
  #ifndef NO_FILEZILLA
  TFileZillaIntf::SetResourceModule(ResourceHandle);
  #else
  DebugUsedParam(ResourceHandle);
  #endif
}

void CoreMaintenanceTask()
{
  DontSaveRandomSeed();
}

TOperationVisualizer::TOperationVisualizer(bool UseBusyCursor) :
  FUseBusyCursor(UseBusyCursor),
  FToken(nullptr)
{
  if (FUseBusyCursor)
  {
    FToken = BusyStart();
  }
}

TOperationVisualizer::~TOperationVisualizer()
{
  if (FUseBusyCursor)
  {
    BusyEnd(FToken);
  }
}

TInstantOperationVisualizer::TInstantOperationVisualizer() :
  TOperationVisualizer(true),
  FStart(Now())
{
}

TInstantOperationVisualizer::~TInstantOperationVisualizer()
{
  TDateTime Time = Now();
  int64_t Duration = MilliSecondsBetween(Time, FStart);
  const int64_t MinDuration = 250;
  if (Duration < MinDuration)
  {
    ::Sleep(static_cast<uint32_t>(MinDuration - Duration));
  }
}

