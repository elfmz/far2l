
#include <vcl.h>
#pragma hdrstop

#ifndef NO_FILEZILLA

#include <list>
#ifndef MPEXT
#define MPEXT
#endif
#include "FtpFileSystem.h"
#ifndef __linux__
#include "FileZillaIntf.h"
#else
#include "libfilezilla/libfilezilla.hpp"
#endif

#include <Common.h>
#include <Exceptions.h>
#include "Terminal.h"
#include "TextsCore.h"
#include "TextsFileZilla.h"
#include "HelpCore.h"
#include "WinSCPSecurity.h"
#include <StrUtils.hpp>
#include <DateUtils.hpp>
#include <openssl/x509_vfy.h>

const int DummyCodeClass = 8;
const int DummyTimeoutCode = 801;
const int DummyCancelCode = 802;
const int DummyDisconnectCode = 803;

class TFileZillaImpl : public TFileZillaIntf
{
public:
  explicit TFileZillaImpl(TFTPFileSystem * FileSystem);
  virtual ~TFileZillaImpl() {}

  virtual const wchar_t * Option(intptr_t OptionID) const;
  virtual intptr_t OptionVal(intptr_t OptionID) const;

protected:
  virtual bool DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam);

  virtual bool HandleStatus(const wchar_t * Status, int Type);
  virtual bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    int64_t Size1, int64_t Size2, time_t LocalTime,
    bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData,
    HANDLE & LocalFileHandle, int & RequestResult);
  virtual bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult);
  virtual bool HandleAsynchRequestNeedPass(
    struct TNeedPassRequestData & Data, int & RequestResult);
  virtual bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    uintptr_t Count);
  virtual bool HandleTransferStatus(bool Valid, int64_t TransferSize,
    int64_t Bytes, bool FileTransfer);
  virtual bool HandleReply(intptr_t Command, uintptr_t Reply);
  virtual bool HandleCapabilities(TFTPServerCapabilities * ServerCapabilities);
  virtual bool CheckError(intptr_t ReturnCode, const wchar_t * Context);

  virtual void PreserveDownloadFileTime(HANDLE AHandle, void * UserData);
  virtual bool GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time);
  virtual wchar_t * LastSysErrorMessage() const;
  virtual std::wstring GetClientString() const;

private:
  TFTPFileSystem * FFileSystem;
};

TFileZillaImpl::TFileZillaImpl(TFTPFileSystem * FileSystem) :
  TFileZillaIntf(),
  FFileSystem(FileSystem)
{
}

const wchar_t * TFileZillaImpl::Option(intptr_t OptionID) const
{
  return FFileSystem->GetOption(OptionID);
}

intptr_t TFileZillaImpl::OptionVal(intptr_t OptionID) const
{
  return FFileSystem->GetOptionVal(OptionID);
}

bool TFileZillaImpl::DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam)
{
  return FFileSystem->FTPPostMessage(Type, wParam, lParam);
}

bool TFileZillaImpl::HandleStatus(const wchar_t * Status, int Type)
{
  return FFileSystem->HandleStatus(Status, Type);
}

bool TFileZillaImpl::HandleAsynchRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
  const wchar_t * Path1, const wchar_t * Path2,
  int64_t Size1, int64_t Size2, time_t LocalTime,
  bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData,
  HANDLE & LocalFileHandle,
  int & RequestResult)
{
  return FFileSystem->HandleAsynchRequestOverwrite(
    FileName1, FileName1Len, FileName2, Path1, Path2, Size1, Size2, LocalTime,
    HasLocalTime, RemoteTime, UserData, LocalFileHandle, RequestResult);
}

bool TFileZillaImpl::HandleAsynchRequestVerifyCertificate(
  const TFtpsCertificateData & Data, int & RequestResult)
{
  return FFileSystem->HandleAsynchRequestVerifyCertificate(Data, RequestResult);
}

bool TFileZillaImpl::HandleAsynchRequestNeedPass(
  struct TNeedPassRequestData & Data, int & RequestResult)
{
  return FFileSystem->HandleAsynchRequestNeedPass(Data, RequestResult);
}

bool TFileZillaImpl::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, uintptr_t Count)
{
  return FFileSystem->HandleListData(Path, Entries, Count);
}

bool TFileZillaImpl::HandleTransferStatus(bool Valid, int64_t TransferSize,
  int64_t Bytes, bool FileTransfer)
{
  return FFileSystem->HandleTransferStatus(Valid, TransferSize, Bytes, FileTransfer);
}

bool TFileZillaImpl::HandleReply(intptr_t Command, uintptr_t Reply)
{
  return FFileSystem->HandleReply(Command, Reply);
}

bool TFileZillaImpl::HandleCapabilities(TFTPServerCapabilities * ServerCapabilities)
{
  return FFileSystem->HandleCapabilities(ServerCapabilities);
}

bool TFileZillaImpl::CheckError(intptr_t ReturnCode, const wchar_t * Context)
{
  return FFileSystem->CheckError(ReturnCode, Context);
}

void TFileZillaImpl::PreserveDownloadFileTime(HANDLE AHandle, void * UserData)
{
  return FFileSystem->PreserveDownloadFileTime(AHandle, UserData);
}

bool TFileZillaImpl::GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time)
{
  return FFileSystem->GetFileModificationTimeInUtc(FileName, Time);
}

wchar_t * TFileZillaImpl::LastSysErrorMessage() const
{
  return _wcsdup(::LastSysErrorMessage().c_str());
}

std::wstring TFileZillaImpl::GetClientString() const
{
  return std::wstring(GetSshVersionString().c_str());
}

static const wchar_t FtpsCertificateStorageKey[] = L"FtpsCertificates";
const UnicodeString SiteCommand(L"SITE");
const UnicodeString SymlinkSiteCommand(L"SYMLINK");
const UnicodeString CopySiteCommand(L"COPY");
const UnicodeString HashCommand(L"HASH"); // Cerberos + FileZilla servers
const UnicodeString AvblCommand(L"AVBL");
const UnicodeString XQuotaCommand(L"XQUOTA");
const UnicodeString DirectoryHasBytesPrefix(L"226-Directory has");

class TFTPFileListHelper : public TObject
{
NB_DISABLE_COPY(TFTPFileListHelper)
public:
  explicit TFTPFileListHelper(TFTPFileSystem * FileSystem, TRemoteFileList * FileList,
      bool IgnoreFileList) :
    FFileSystem(FileSystem),
    FFileList(FFileSystem->FFileList),
    FIgnoreFileList(FFileSystem->FIgnoreFileList)
  {
    FFileSystem->FFileList = FileList;
    FFileSystem->FIgnoreFileList = IgnoreFileList;
  }

  ~TFTPFileListHelper()
  {
    FFileSystem->FFileList = FFileList;
    FFileSystem->FIgnoreFileList = FIgnoreFileList;
  }

private:
  TFTPFileSystem * FFileSystem;
  TRemoteFileList * FFileList;
  bool FIgnoreFileList;
};

TFTPFileSystem::TFTPFileSystem(TTerminal * ATerminal) :
  TCustomFileSystem(ATerminal),
  FFileZillaIntf(nullptr),
  FQueueEvent(::CreateEvent(nullptr, true, false, nullptr)),
  FFileSystemInfoValid(false),
  FReply(0),
  FCommandReply(0),
  FLastCommand(CMD_UNKNOWN),
  FPasswordFailed(false),
  FStoredPasswordTried(false),
  FMultineResponse(false),
  FLastCode(0),
  FLastCodeClass(0),
  FLastReadDirectoryProgress(0),
  FLastResponse(new TStringList()),
  FLastErrorResponse(new TStringList()),
  FLastError(new TStringList()),
  FFeatures(new TStringList()),
  FFileList(nullptr),
  FFileListCache(nullptr),
  FActive(false),
  FOpening(false),
  FWaitingForReply(false),
  FFileTransferAbort(ftaNone),
  FIgnoreFileList(false),
  FFileTransferCancelled(false),
  FFileTransferResumed(0),
  FFileTransferPreserveTime(false),
  FFileTransferRemoveBOM(false),
  FFileTransferNoList(false),
  FFileTransferCPSLimit(0),
  FAwaitingProgress(false),
  FOnCaptureOutput(nullptr),
  FListAll(asOn),
  FDoListAll(false),
  FReadCurrentDirectory(false),
  FServerCapabilities(new TFTPServerCapabilities()),
  FDetectTimeDifference(false),
  FTimeDifference(0),
  FSupportsAnyChecksumFeature(false),
  FCertificate(nullptr),
  FPrivateKey(nullptr),
  FTransferActiveImmediately(false),
  FWindowsServer(false),
  FBytesAvailable(0),
  FBytesAvailableSupported(false)
{
}

void TFTPFileSystem::Init(void *)
{
  //FQueue.reserve(1000);
  ResetReply();

  FListAll = FTerminal->GetSessionData()->GetFtpListAll();
  FFileSystemInfo.ProtocolBaseName = L"FTP";
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
  FTimeoutStatus = LoadStr(IDS_ERRORMSG_TIMEOUT);
  FDisconnectStatus = LoadStr(IDS_STATUSMSG_DISCONNECTED);
  FServerCapabilities = new TFTPServerCapabilities();
  FHashAlgs.reset(new TStringList());
  FSupportedCommands.reset(CreateSortedStringList());
  FSupportedSiteCommands.reset(CreateSortedStringList());
  FCertificate = nullptr;
  FPrivateKey = nullptr;
  FBytesAvailable = -1;
  FBytesAvailableSupported = false;

  FChecksumAlgs.reset(new TStringList());
  FChecksumCommands.reset(new TStringList());
  RegisterChecksumAlgCommand(Sha1ChecksumAlg, L"XSHA1"); // e.g. Cerberos FTP
  RegisterChecksumAlgCommand(Sha256ChecksumAlg, L"XSHA256"); // e.g. Cerberos FTP
  RegisterChecksumAlgCommand(Sha512ChecksumAlg, L"XSHA512"); // e.g. Cerberos FTP
  RegisterChecksumAlgCommand(Md5ChecksumAlg, L"XMD5"); // e.g. Cerberos FTP
  RegisterChecksumAlgCommand(Md5ChecksumAlg, L"MD5"); // e.g. Apache FTP
  RegisterChecksumAlgCommand(Crc32ChecksumAlg, L"XCRC"); // e.g. Cerberos FTP
}

TFTPFileSystem::~TFTPFileSystem()
{
  DebugAssert(FFileList == nullptr);

  if (FFileZillaIntf)
  {
    FFileZillaIntf->Destroying();
  }

  // to release memory associated with the messages
  DiscardMessages();

  SAFE_DESTROY(FFileZillaIntf);

  ::CloseHandle(FQueueEvent);
  FQueueEvent = nullptr;

  SAFE_DESTROY(FLastResponse);
  SAFE_DESTROY(FLastErrorResponse);
  SAFE_DESTROY(FLastError);
  SAFE_DESTROY(FFeatures);
  SAFE_DESTROY(FServerCapabilities);
  SAFE_DESTROY(FLastError);
  SAFE_DESTROY(FFeatures);

  ResetCaches();
}

void TFTPFileSystem::Open()
{
  // on reconnect, typically there may be pending status messages from previous session
  DiscardMessages();

  ResetCaches();
  FReadCurrentDirectory = true;
  FCurrentDirectory.Clear();
  FHomeDirectory.Clear();

  TSessionData * Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();
  FSessionInfo.ProtocolBaseName = L"FTP";
  FSessionInfo.ProtocolName = FSessionInfo.ProtocolBaseName;

  switch (Data->GetFtps())
  {
    case ftpsNone:
      // noop;
      break;

    case ftpsImplicit:
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_IMPLICIT);
      break;

    case ftpsExplicitSsl:
    case ftpsExplicitTls:
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT);
      break;

    default:
      DebugFail();
      break;
  }

  FLastDataSent = Now();

  FMultineResponse = false;

  // initialize FZAPI on the first connect only
  if (FFileZillaIntf == nullptr)
  {
    std::unique_ptr<TFileZillaIntf> FileZillaImpl(new TFileZillaImpl(this));

    try__catch
    {
      TFileZillaIntf::TLogLevel LogLevel;
      switch (FTerminal->GetConfiguration()->GetActualLogProtocol())
      {
        default:
        case 0:
        case 1:
          LogLevel = TFileZillaIntf::LOG_PROGRESS;
          break;

        case 2:
          LogLevel = TFileZillaIntf::LOG_INFO;
          break;
      }
      FileZillaImpl->SetDebugLevel(LogLevel);
      FileZillaImpl->Init();
      FFileZillaIntf = FileZillaImpl.release();
    }
    /*catch (...)
    {
      delete FFileZillaIntf;
      FFileZillaIntf = NULL;
      throw;
    }*/
  }

  FWindowsServer = false;
  FTransferActiveImmediately = (Data->GetFtpTransferActiveImmediately() == asOn);

  FSessionInfo.LoginTime = Now();

  UnicodeString HostName = Data->GetHostNameExpanded();
  UnicodeString UserName = Data->GetUserNameExpanded();
  UnicodeString Password = Data->GetPassword();
  UnicodeString Account = Data->GetFtpAccount();
  UnicodeString Path = Data->GetRemoteDirectory();
  int ServerType = 0;
  switch (Data->GetFtps())
  {
    case ftpsNone:
      ServerType = TFileZillaIntf::SERVER_FTP;
      break;

    case ftpsImplicit:
      ServerType = TFileZillaIntf::SERVER_FTP_SSL_IMPLICIT;
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_IMPLICIT);
      break;

    case ftpsExplicitSsl:
      ServerType = TFileZillaIntf::SERVER_FTP_SSL_EXPLICIT;
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT);
      break;

    case ftpsExplicitTls:
      ServerType = TFileZillaIntf::SERVER_FTP_TLS_EXPLICIT;
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT);
      break;

    default:
      DebugFail();
      break;
  }

  intptr_t Pasv = (Data->GetFtpPasvMode() ? 1 : 2);

  intptr_t TimeZoneOffset = Data->GetTimeDifferenceAuto() ? 0 : TimeToMinutes(Data->GetTimeDifference());

  int UTF8 = 0;
  uintptr_t CodePage = Data->GetCodePageAsNumber();

  switch (CodePage)
  {
    case CP_ACP:
      UTF8 = 2; // no UTF8
      break;

    case CP_UTF8:
      UTF8 = 1; // always UTF8
      break;

    default:
      UTF8 = 0; // auto detect
      break;
  }

  FPasswordFailed = false;
  FStoredPasswordTried = false;
  bool PromptedForCredentials = false;

  do
  {
    FDetectTimeDifference = Data->GetTimeDifferenceAuto();
    FTimeDifference = 0;
    ResetFeatures();
    FSystem.Clear();
    FWelcomeMessage.Clear();
    FFileSystemInfoValid = false;

    TODO("the same for account? it ever used?");

    // ask for username if it was not specified in advance, even on retry,
    // but keep previous one as default,
    if (Data->GetUserNameExpanded().IsEmpty() && !FTerminal->GetSessionData()->GetFingerprintScan())
    {
      FTerminal->LogEvent("Username prompt (no username provided)");

      if (!FPasswordFailed && !PromptedForCredentials)
      {
        FTerminal->Information(LoadStr(FTP_CREDENTIAL_PROMPT), false);
        PromptedForCredentials = true;
      }

      if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(USERNAME_TITLE), L"",
            LoadStr(USERNAME_PROMPT2), true, 0, UserName))
      {
        FTerminal->FatalError(nullptr, LoadStr(AUTHENTICATION_FAILED));
      }
      else
      {
        FUserName = UserName;
      }
    }

    // On retry ask for password.
    // This is particularly important, when stored password is no longer valid,
    // so we do not blindly try keep trying it in a loop (possibly causing account lockout)
    if (FPasswordFailed)
    {
      FTerminal->LogEvent("Password prompt (last login attempt failed)");

      Password.Clear();
      if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(PASSWORD_TITLE), L"",
            LoadStr(PASSWORD_PROMPT), false, 0, Password))
      {
        FTerminal->FatalError(nullptr, LoadStr(AUTHENTICATION_FAILED));
      }
    }

    if ((Data->GetFtps() != ftpsNone) && (FCertificate == nullptr))
    {
      FTerminal->LoadTlsCertificate(FCertificate, FPrivateKey);
    }

    FPasswordFailed = false;
    TAutoFlag OpeningFlag(FOpening);

    FActive = FFileZillaIntf->Connect(
      HostName.c_str(), static_cast<int>(Data->GetPortNumber()), UserName.c_str(),
      Password.c_str(), Account.c_str(), Path.c_str(),
      ServerType, static_cast<int>(Pasv), static_cast<int>(TimeZoneOffset), UTF8,
      static_cast<int>(CodePage),
      static_cast<int>(Data->GetFtpForcePasvIp()),
      static_cast<int>(Data->GetFtpUseMlsd()),
      static_cast<int>(Data->GetFtpDupFF()),
      static_cast<int>(Data->GetFtpUndupFF()),
      FCertificate, FPrivateKey);

    DebugAssert(FActive);

    try
    {
      // do not wait for FTP response code as Connect is complex operation
      GotReply(WaitForCommandReply(false), REPLY_CONNECT, LoadStr(CONNECTION_FAILED));

      Shred(Password);

      // we have passed, even if we got 530 on the way (if it is possible at all),
      // ignore it
      DebugAssert(!FPasswordFailed);
      FPasswordFailed = false;
    }
    catch (...)
    {
      if (FPasswordFailed)
      {
        FTerminal->Information(LoadStr(Password.IsEmpty() ? FTP_ACCESS_DENIED_EMPTY_PASSWORD : FTP_ACCESS_DENIED), false);
      }
      else
      {
        // see handling of REPLY_CONNECT in GotReply
        FTerminal->Closed();
        throw;
      }
    }
  }
  while (FPasswordFailed);

  // see also TWebDAVFileSystem::CollectTLSSessionInfo()
  FSessionInfo.CSCipher = FFileZillaIntf->GetCipherName().c_str();
  FSessionInfo.SCCipher = FSessionInfo.CSCipher;
  UnicodeString TlsVersionStr = FFileZillaIntf->GetTlsVersionStr().c_str();
  AddToList(FSessionInfo.SecurityProtocolName, TlsVersionStr, L", ");
}

void TFTPFileSystem::Close()
{
  DebugAssert(FActive);
  bool Result;

  FFileZillaIntf->CustomCommand(L"QUIT");
  Result = true;

  /*if (FFileZillaIntf->Close(FOpening))
  {
    DebugCheck(FLAGSET(WaitForCommandReply(false), TFileZillaIntf::REPLY_DISCONNECTED));
    Result = true;
  }
  else
  {
    // See TFileZillaIntf::Close
    Result = FOpening;
  }*/

  if (DebugAlwaysTrue(Result))
  {
    DebugAssert(FActive);
    Discard();
    FTerminal->Closed();
  }
}

bool TFTPFileSystem::GetActive() const
{
  return FActive;
}

void TFTPFileSystem::CollectUsage()
{
  /*switch (FTerminal->SessionData->Ftps)
  {
    case ftpsNone:
      // noop
      break;

    case ftpsImplicit:
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPSImplicit");
      break;

    case ftpsExplicitSsl:
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPSExplicitSSL");
      break;

    case ftpsExplicitTls:
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPSExplicitTLS");
      break;

    default:
      DebugFail();
      break;
  }

  if (!FTerminal->SessionData->TlsCertificateFile.IsEmpty())
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPSCertificate");
  }

  if (FFileZillaIntf->UsingMlsd())
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPMLSD");
  }
  else
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPLIST");
  }

  if (FFileZillaIntf->UsingUtf8())
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPUTF8");
  }
  else
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPNonUTF8");
  }
  if (!GetCurrentDirectory().IsEmpty() && (GetCurrentDirectory()[1] != LOTHER_SLASH))
  {
    if (::IsUnixStyleWindowsPath(GetCurrentDirectory()))
    {
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPWindowsPath");
    }
    else if ((GetCurrentDirectory().Length() >= 3) && IsLetter(GetCurrentDirectory()[1]) && (GetCurrentDirectory()[2] == L':') && (CurrentDirectory[3] == LOTHER_SLASH))
    {
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPRealWindowsPath");
    }
    else
    {
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPOtherPath");
    }
  }

  UnicodeString TlsVersionStr = FFileZillaIntf->GetTlsVersionStr().c_str();
  if (!TlsVersionStr.IsEmpty())
  {
    FTerminal->CollectTlsUsage(TlsVersionStr);
  }

  // 220-FileZilla Server version 0.9.43 beta
  // 220-written by Tim Kosse (Tim.Kosse@gmx.de)
  // 220 Please visit http://sourceforge.net/projects/filezilla/
  // SYST
  // 215 UNIX emulated by FileZilla
  // (Welcome message is configurable)
  if (ContainsText(FSystem, L"FileZilla"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPFileZilla");
  }
  // 220 ProFTPD 1.3.4a Server (Debian) [::ffff:192.168.179.137]
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, L"ProFTPD"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPProFTPD");
  }
  // 220 Microsoft FTP Service
  // SYST
  // 215 Windows_NT
  else if (ContainsText(FWelcomeMessage, L"Microsoft FTP Service") ||
           ContainsText(FSystem, L"Windows_NT"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPIIS");
  }
  // 220 (vsFTPd 3.0.2)
  // SYST
  // 215 UNIX Type: L8
  // (Welcome message is configurable)
  else if (ContainsText(FWelcomeMessage, L"vsFTPd"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPvsFTPd");
  }
  // 220 Welcome to Pure-FTPd.
  // ...
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, L"Pure-FTPd"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPPureFTPd");
  }
  // 220 Titan FTP Server 10.47.1892 Ready.
  // ...
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, L"Titan FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPTitan");
  }
  // 220-Cerberus FTP Server - Home Edition
  // 220-This is the UNLICENSED Home Edition and may be used for home, personal use only
  // 220-Welcome to Cerberus FTP Server
  // 220 Created by Cerberus, LLC
  else if (ContainsText(FWelcomeMessage, L"Cerberus FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPCerberus");
  }
  // 220 Serv-U FTP Server v15.1 ready...
  else if (ContainsText(FWelcomeMessage, L"Serv-U FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPServU");
  }
  else if (ContainsText(FWelcomeMessage, L"WS_FTP"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPWSFTP");
  }
  // 220 Welcome to the most popular FTP hosting service! Save on hardware, software, hosting and admin. Share files/folders with read-write permission. Visit http://www.drivehq.com/ftp/;
  // ...
  // SYST
  // 215 UNIX emulated by DriveHQ FTP Server.
  else if (ContainsText(FSystem, L"DriveHQ"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPDriveHQ");
  }
  // 220 GlobalSCAPE EFT Server (v. 6.0) * UNREGISTERED COPY *
  // ...
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, L"GlobalSCAPE"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPGlobalScape");
  }
  // 220-<custom message>
  // 220 CompleteFTP v 8.1.3
  // ...
  // SYST
  // UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, L"CompleteFTP"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPComplete");
  }
  // 220 Core FTP Server Version 1.2, build 567, 64-bit, installed 8 days ago Unregistered
  // ...
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, L"Core FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPCore");
  }
  // 220 Service ready for new user.
  // ..
  // SYST
  // 215 UNIX Type: Apache FtpServer
  // (e.g. brickftp.com)
  else if (ContainsText(FSystem, L"Apache FtpServer"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPApache");
  }
  // 220 pos1 FTP server (GNU inetutils 1.3b) ready.
  // ...
  // SYST
  // 215 UNIX Type: L8 Version: Linux 2.6.15.7-ELinOS-314pm3
  // Displaying "(GNU inetutils 1.3b)" in a welcome message can be turned off (-q switch):
  // 220 pos1 FTP server ready.
  // (the same for "Version: Linux 2.6.15.7-ELinOS-314pm3" in SYST response)
  else if (ContainsText(FWelcomeMessage, L"GNU inetutils"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPInetutils");
  }
  // 220 Syncplify.me Server! FTP(S) Service Ready
  // Message is configurable
  else if (ContainsText(FWelcomeMessage, L"Syncplify"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPSyncplify");
  }
  // 220-Idea FTP Server v0.80 (xxx.home.pl) [xxx.xxx.xxx.xxx]
  // 220 Ready
  // ...
  // SYST
  // UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, L"Idea FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPIdea");
  }
  else
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPOther");
  }*/
}

void TFTPFileSystem::Idle()
{
  if (FActive && !FWaitingForReply)
  {
    PoolForFatalNonCommandReply();

    // Keep session alive
    if ((FTerminal->GetSessionData()->GetFtpPingType() != ptOff) &&
        ((Now() - FLastDataSent).GetValue() > FTerminal->GetSessionData()->GetFtpPingIntervalDT().GetValue() * 4))
    {
      FTerminal->LogEvent("Dummy directory read to keep session alive.");
      FLastDataSent = Now();

      std::unique_ptr<TRemoteDirectory> Files(new TRemoteDirectory(FTerminal));
      try__finally
      {
        try
        {
          Files->SetDirectory(GetCurrDirectory());
          DoReadDirectory(Files.get());
        }
        catch (...)
        {
          // ignore non-fatal errors
          // (i.e. current directory may not exist anymore)
          if (!FTerminal->GetActive())
          {
            throw;
          }
        }
      }
      __finally
      {
//        delete Files;
      };
    }
  }
}

void TFTPFileSystem::Discard()
{
  // remove all pending messages, to get complete log
  // note that we need to retry discard on reconnect, as there still may be another
  // "disconnect/timeout/..." status messages coming
  DiscardMessages();
  DebugAssert(FActive);
  FActive = false;

  // See neon's ne_ssl_clicert_free
  if (FPrivateKey != nullptr)
  {
    EVP_PKEY_free(FPrivateKey);
    FPrivateKey = nullptr;
  }
  if (FCertificate != nullptr)
  {
    X509_free(FCertificate);
    FCertificate = nullptr;
  }
}

UnicodeString TFTPFileSystem::GetAbsolutePath(const UnicodeString & APath, bool Local)
{
  return static_cast<const TFTPFileSystem *>(this)->GetAbsolutePath(APath, Local);
}

UnicodeString TFTPFileSystem::GetAbsolutePath(const UnicodeString & APath, bool /*Local*/) const
{
  TODO("improve (handle .. etc.)");
  if (core::UnixIsAbsolutePath(APath))
  {
    return APath;
  }
  else
  {
    return core::AbsolutePath(FCurrentDirectory, APath);
  }
}

UnicodeString TFTPFileSystem::ActualCurrentDirectory()
{
  UnicodeString CurrentPath(1024, 0);
  UnicodeString Result;
  if (FFileZillaIntf->GetCurrentPath(const_cast<wchar_t *>(CurrentPath.c_str()), CurrentPath.Length()))
  {
    Result = core::UnixExcludeTrailingBackslash(CurrentPath);
  }
  if (Result.IsEmpty())
  {
    Result = ROOTDIRECTORY;
  }
  return Result;
}

void TFTPFileSystem::EnsureLocation()
{
  // if we do not know what's the current directory, do nothing
  if (!FCurrentDirectory.IsEmpty())
  {
    // Make sure that the FZAPI current working directory,
    // is actually our working directory.
    // It may not be because:
    // 1) We did cached directory change
    // 2) Listing was requested for non-current directory, which
    // makes FZAPI change its current directory (and not restoring it back afterwards)
    if (!core::UnixSamePath(ActualCurrentDirectory(), FCurrentDirectory))
    {
      FTerminal->LogEvent(FORMAT(L"Synchronizing current directory \"%s\".",
        FCurrentDirectory.c_str()));
      DoChangeDirectory(FCurrentDirectory);
      // make sure FZAPI is aware that we changed current working directory
      FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
    }
  }
}

void TFTPFileSystem::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent OutputEvent)
{
  // end-user has right to expect that client current directory is really
  // current directory for the server
  EnsureLocation();

  DebugAssert(FOnCaptureOutput == nullptr);
  FOnCaptureOutput = OutputEvent;
  try__finally
  {
    SCOPE_EXIT
    {
      FOnCaptureOutput = nullptr;
    };
    SendCommand(Command);

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_3XX_CODE);
  }
  __finally
  {
    FOnCaptureOutput = nullptr;
  };
}

void TFTPFileSystem::ResetCaches()
{
  SAFE_DESTROY(FFileListCache);
}

void TFTPFileSystem::AnnounceFileListOperation()
{
  ResetCaches();
}

void TFTPFileSystem::DoChangeDirectory(const UnicodeString & Directory)
{
  UnicodeString Command = FORMAT(L"CWD %s", Directory.c_str());
  SendCommand(Command);

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
}

void TFTPFileSystem::ChangeDirectory(const UnicodeString & ADirectory)
{
  UnicodeString Directory = ADirectory;
  try
  {
    // For changing directory, we do not make paths absolute, instead we
    // delegate this to the server, hence we synchronize current working
    // directory with the server and only then we ask for the change with
    // relative path.
    // But if synchronization fails, typically because current working directory
    // no longer exists, we fall back to out own resolution, to give
    // user chance to leave the non-existing directory.
    EnsureLocation();
  }
  catch (...)
  {
    if (FTerminal->GetActive())
    {
      Directory = GetAbsolutePath(Directory, false);
    }
    else
    {
      throw;
    }
  }

  DoChangeDirectory(Directory);

  // make next ReadCurrentDirectory retrieve actual server-side current directory
  FReadCurrentDirectory = true;
}

void TFTPFileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FCurrentDirectory = core::UnixExcludeTrailingBackslash(Directory);
  if (FCurrentDirectory.IsEmpty())
  {
    FCurrentDirectory = ROOTDIRECTORY;
  }
  FReadCurrentDirectory = false;
}

void TFTPFileSystem::ChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  DebugAssert(Properties);
  DebugAssert(!Properties->Valid.Contains(vpGroup)); //-V595
  DebugAssert(!Properties->Valid.Contains(vpOwner)); //-V595
  DebugAssert(!Properties->Valid.Contains(vpLastAccess)); //-V595
  DebugAssert(!Properties->Valid.Contains(vpModification)); //-V595

  if (Properties && Properties->Valid.Contains(vpRights))
  {
    std::unique_ptr<TRemoteFile> OwnedFile;

    try__finally
    {
      UnicodeString FileName = GetAbsolutePath(AFileName, false);

      if (AFile == nullptr)
      {
        TRemoteFile * File = nullptr;
        ReadFile(FileName, File);
        OwnedFile.reset(File);
        AFile = File;
      }

      if ((AFile != nullptr) && AFile->GetIsDirectory() && FTerminal->CanRecurseToDirectory(AFile) && Properties->Recursive)
      {
        try
        {
          FTerminal->ProcessDirectory(AFileName, MAKE_CALLBACK(TTerminal::ChangeFileProperties, FTerminal),
            static_cast<void *>(const_cast<TRemoteProperties *>(Properties)));
        }
        catch (...)
        {
          Action.Cancel();
          throw;
        }
      }

      TRights Rights;
      if (AFile != nullptr)
      {
        Rights = *AFile->GetRights();
      }
      Rights |= Properties->Rights.GetNumberSet();
      Rights &= static_cast<uint16_t>(~Properties->Rights.GetNumberUnset());
      if ((AFile != nullptr) && AFile->GetIsDirectory() && Properties->AddXToDirectories)
      {
        Rights.AddExecute();
      }

      Action.Rights(Rights);

      UnicodeString FileNameOnly = base::UnixExtractFileName(FileName);
      UnicodeString FilePath = core::UnixExtractFilePath(FileName);
      // FZAPI wants octal number represented as decadic
      FFileZillaIntf->Chmod(Rights.GetNumberDecadic(), FileNameOnly.c_str(), FilePath.c_str());

      GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
    }
    __finally
    {
//      delete OwnedFile;
    };
  }
  else
  {
    Action.Cancel();
  }
}

bool TFTPFileSystem::LoadFilesProperties(TStrings * /*FileList*/)
{
  DebugFail();
  return false;
}

UnicodeString TFTPFileSystem::DoCalculateFileChecksum(
  bool UsingHashCommand, const UnicodeString & Alg, TRemoteFile * File)
{
  // Overview of server supporting various hash commands is at:
  // https://tools.ietf.org/html/draft-ietf-ftpext2-hash-03#appendix-B

  UnicodeString CommandName;

  if (UsingHashCommand)
  {
    CommandName = HashCommand;
  }
  else
  {
    intptr_t Index = FChecksumAlgs->IndexOf(Alg);
    if (Index < 0)
    {
      DebugFail();
      ThrowExtException();
    }
    else
    {
      CommandName = FChecksumCommands->GetString(Index);
    }
  }

  UnicodeString FileName = File->GetFullFileName();
  // FTP way is not to quote.
  // But as Serv-U, GlobalSCAPE and possibly others allow
  // additional parameters (SP ER range), they need to quote file name.
  // Cerberus and FileZilla Server on the other hand can do without quotes
  // (but they can handle them, not sure about other servers)

  // Quoting:
  // FileZilla Server simply checks if argument starts and ends with double-quote
  // and strips them, no double-quote escaping is possible.
  // That's for all commands, not just HASH
  // ProFTPD: TODO: Check how "SITE SYMLINK target link" is parsed

  // We can possibly autodetect this from announced command format:
  // XCRC filename;start;end
  // XMD5 filename;start;end
  // XSHA1 filename;start;end
  // XSHA256 filename;start;end
  // XSHA512 filename;start;end
  if (FileName.Pos(L" ") > 0)
  {
    FileName = FORMAT(L"\"%s\"", FileName.c_str());
  }

  UnicodeString Command = FORMAT(L"%s %s", CommandName.c_str(), FileName.c_str());
  SendCommand(Command);
  UnicodeString ResponseText = GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_SINGLE_LINE);

  UnicodeString Hash;
  if (UsingHashCommand)
  {
    // Code should be 213, but let's be tolerant and accept any 2xx

    // ("213" SP) hashname SP start-point "-" end-point SP filehash SP <pathname> (CRLF)
    UnicodeString Buf = ResponseText;
    // skip alg
    CutToChar(Buf, L' ', true);
    // skip range
    UnicodeString Range = CutToChar(Buf, L' ', true);
    // This should be range (SP-EP), but if it does not conform to the format,
    // it's likely because the server uses version of the HASH spec
    // before draft-ietf-ftpext2-hash-01
    // (including draft-bryan-ftp-hash-06 implemented by FileZilla server; or Cerberus),
    // that did not have the "range" part.
    // The FileZilla Server even omits the file name.
    // The latest draft as of implementing this is draft-bryan-ftpext-hash-02.
    if (Range.Pos(L"-") > 0)
    {
      Hash = CutToChar(Buf, L' ', true);
    }
    else
    {
      Hash = Range;
    }
  }
  else // All hash-specific commands
  {
    // Accepting any 2xx response. Most servers use 213,
    // but for example WS_FTP uses non-sense code 220 (Service ready for new user)

    // MD5 response according to a draft-twine-ftpmd5-00 includes a file name
    // (implemented by Apache FtpServer).
    // Other commands (X<hash>) return the hash only.
    ResponseText = ResponseText.Trim();
    intptr_t P = ResponseText.LastDelimiter(L" ");
    if (P > 0)
    {
      ResponseText.Delete(1, P);
    }

    Hash = ResponseText;
  }

  if (Hash.IsEmpty())
  {
    throw Exception(FMTLOAD(FTP_RESPONSE_ERROR, CommandName.c_str(), ResponseText.c_str()));
  }

  return LowerCase(Hash);
}

void TFTPFileSystem::DoCalculateFilesChecksum(bool UsingHashCommand,
  const UnicodeString & Alg, TStrings * FileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum,
  TFileOperationProgressType * OperationProgress, bool FirstLevel)
{
  TOnceDoneOperation OnceDoneOperation; // not used

  intptr_t Index1 = 0;
  while ((Index1 < FileList->GetCount()) && !OperationProgress->Cancel)
  {
    TRemoteFile * File = static_cast<TRemoteFile *>(FileList->GetObj(Index1));
    DebugAssert(File != nullptr);

    if (File->GetIsDirectory())
    {
      if (FTerminal->CanRecurseToDirectory(File) &&
          !File->GetIsParentDirectory() && !File->GetIsThisDirectory() &&
          // recurse into subdirectories only if we have callback function
          (OnCalculatedChecksum != nullptr))
      {
        OperationProgress->SetFile(File->GetFileName());
        TRemoteFileList * SubFiles =
          FTerminal->CustomReadDirectoryListing(File->GetFullFileName(), false);

        if (SubFiles != nullptr)
        {
          TStrings * SubFileList = new TStringList();
          bool Success = false;
          try__finally
          {
            SCOPE_EXIT
            {
              delete SubFiles;
              delete SubFileList;

              if (FirstLevel)
              {
                OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
              }
            };

            OperationProgress->SetFile(File->GetFileName());

            for (intptr_t Index2 = 0; Index2 < SubFiles->GetCount(); Index2++)
            {
              TRemoteFile * SubFile = SubFiles->GetFile(Index2);
              SubFileList->AddObject(SubFile->GetFullFileName(), SubFile);
            }

            // do not collect checksums for files in subdirectories,
            // only send back checksums via callback
            DoCalculateFilesChecksum(UsingHashCommand, Alg, SubFileList, nullptr,
              OnCalculatedChecksum, OperationProgress, false);

            Success = true;
          }
          __finally
          {
            delete SubFiles;
            delete SubFileList;

            if (FirstLevel)
            {
              OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
            }
          };
        }
      }
    }
    else
    {
      TChecksumSessionAction Action(FTerminal->GetActionLog());
      try
      {
        OperationProgress->SetFile(File->GetFileName());
        Action.SetFileName(FTerminal->GetAbsolutePath(File->GetFullFileName(), true));

        UnicodeString Checksum = DoCalculateFileChecksum(UsingHashCommand, Alg, File);

        if (OnCalculatedChecksum != nullptr)
        {
          OnCalculatedChecksum(File->GetFileName(), Alg, Checksum);
        }
        Action.Checksum(Alg, Checksum);
        if (Checksums != nullptr)
        {
          Checksums->Add(Checksum);
        }
      }
      catch (Exception & E)
      {
        FTerminal->RollbackAction(Action, OperationProgress, &E);

        // Error formatting expanded from inline to avoid strange exceptions
        UnicodeString Error =
          FMTLOAD(CHECKSUM_ERROR,
            (File != nullptr ? File->GetFullFileName().c_str() : L""));
        FTerminal->CommandError(&E, Error);
        // Abort loop.
        TODO("retries? resume?");
        Index1 = FileList->GetCount();
      }
    }
    Index1++;
  }
}

void TFTPFileSystem::CalculateFilesChecksum(const UnicodeString & Alg,
  TStrings * FileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  TFileOperationProgressType Progress(MAKE_CALLBACK(TTerminal::DoProgress, FTerminal), MAKE_CALLBACK(TTerminal::DoFinished, FTerminal));
  Progress.Start(foCalculateChecksum, osRemote, FileList->GetCount());

  FTerminal->SetOperationProgress(&Progress);

  try__finally
  {
    SCOPE_EXIT
    {
      FTerminal->SetOperationProgress(nullptr);
      Progress.Stop();
    };
    UnicodeString NormalizedAlg = FindIdent(FindIdent(Alg, FHashAlgs.get()), FChecksumAlgs.get());

    bool UsingHashCommand = (FHashAlgs->IndexOf(NormalizedAlg) >= 0);
    if (UsingHashCommand)
    {
      // The server should understand lowercase alg name by spec,
      // but we should use uppercase anyway
      SendCommand(FORMAT(L"OPTS %s %s", HashCommand.c_str(), UpperCase(NormalizedAlg).c_str()));
      GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
    }
    else if (FChecksumAlgs->IndexOf(NormalizedAlg) >= 0)
    {
      // will use algorithm-specific command
    }
    else
    {
      throw Exception(FMTLOAD(UNKNOWN_CHECKSUM, Alg.c_str()));
    }

    DoCalculateFilesChecksum(UsingHashCommand, NormalizedAlg, FileList, Checksums, OnCalculatedChecksum,
      &Progress, true);
  }
  __finally
  {
    FTerminal->SetOperationProgress(nullptr);
    Progress.Stop();
  };
}

bool TFTPFileSystem::ConfirmOverwrite(
  const UnicodeString & ASourceFullFileName,
  UnicodeString & ATargetFileName,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  bool AutoResume,
  const TOverwriteFileParams * FileParams,
  const TCopyParamType * CopyParam,
  OUT TOverwriteMode & OverwriteMode)
{
  bool Result;
  bool CanAutoResume = FLAGSET(Params, cpNoConfirmation) && AutoResume;
  bool DestIsSmaller = (FileParams != nullptr) && (FileParams->DestSize < FileParams->SourceSize);
  bool DestIsSame = (FileParams != nullptr) && (FileParams->DestSize == FileParams->SourceSize);
  bool CanResume =
    !OperationProgress->AsciiTransfer &&
    // when resuming transfer after interrupted connection,
    // do nothing (dummy resume) when the files has the same size.
    // this is workaround for servers that strangely fails just after successful
    // upload.
    (DestIsSmaller || (DestIsSame && CanAutoResume));

  uintptr_t Answer;
  if (CanAutoResume && CanResume)
  {
    if (DestIsSame)
    {
      DebugAssert(CanAutoResume);
      Answer = qaSkip;
    }
    else
    {
      Answer = qaRetry;
    }
  }
  else
  {
    // retry = "resume"
    // all = "yes to newer"
    // ignore = "rename"
    uintptr_t Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;
    if (CanResume)
    {
      Answers |= qaRetry;
    }
    TQueryButtonAlias Aliases[5];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(RESUME_BUTTON);
    Aliases[0].GroupWith = qaNo;
    Aliases[0].GrouppedShiftState = ssAlt;
    Aliases[1].Button = qaAll;
    Aliases[1].Alias = LoadStr(YES_TO_NEWER_BUTTON);
    Aliases[1].GroupWith = qaYes;
    Aliases[1].GrouppedShiftState = ssCtrl;
    Aliases[2].Button = qaIgnore;
    Aliases[2].Alias = LoadStr(RENAME_BUTTON);
    Aliases[2].GroupWith = qaNo;
    Aliases[2].GrouppedShiftState = ssCtrl;
    Aliases[3].Button = qaYesToAll;
    Aliases[3].GroupWith = qaYes;
    Aliases[3].GrouppedShiftState = ssShift;
    Aliases[4].Button = qaNoToAll;
    Aliases[4].GroupWith = qaNo;
    Aliases[4].GrouppedShiftState = ssShift;
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = _countof(Aliases);

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      Answer = FTerminal->ConfirmFileOverwrite(
        ASourceFullFileName, ATargetFileName, FileParams, Answers, &QueryParams,
        OperationProgress->Side == osLocal ? osRemote : osLocal,
        CopyParam, Params, OperationProgress);
    }
  }

  Result = true;

  switch (Answer)
  {
    // resume
    case qaRetry:
      OverwriteMode = omResume;
      DebugAssert(FileParams != nullptr);
      DebugAssert(CanResume);
      FFileTransferResumed = FileParams ? FileParams->DestSize : 0;
      break;

    // rename
    case qaIgnore:
      if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName,
            LoadStr(RENAME_TITLE), L"", LoadStr(RENAME_PROMPT2), true, 0, ATargetFileName))
      {
        OverwriteMode = omOverwrite;
      }
      else
      {
        if (!OperationProgress->Cancel)
        {
          OperationProgress->Cancel = csCancel;
        }
        FFileTransferAbort = ftaCancel;
        Result = false;
      }
      break;

    case qaYes:
      OverwriteMode = omOverwrite;
      break;

    case qaCancel:
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      FFileTransferAbort = ftaCancel;
      Result = false;
      break;

    case qaNo:
      FFileTransferAbort = ftaSkip;
      Result = false;
      break;

    case qaSkip:
      OverwriteMode = omComplete;
      break;

    default:
      DebugFail();
      Result = false;
      break;
  }
  return Result;
}

void TFTPFileSystem::ResetFileTransfer()
{
  FFileTransferAbort = ftaNone;
  FFileTransferCancelled = false;
  FFileTransferResumed = 0;
}

void TFTPFileSystem::ReadDirectoryProgress(int64_t Bytes)
{
  // with FTP we do not know exactly how many entries we have received,
  // instead we know number of bytes received only.
  // so we report approximation based on average size of entry.
  int Progress = static_cast<int>(Bytes / 80);
  if (Progress - FLastReadDirectoryProgress >= 10)
  {
    bool Cancel = false;
    FLastReadDirectoryProgress = Progress;
    FTerminal->DoReadDirectoryProgress(Progress, 0, Cancel);
    if (Cancel)
    {
      FTerminal->DoReadDirectoryProgress(-2, 0, Cancel);
      FFileZillaIntf->Cancel();
    }
  }
}

void TFTPFileSystem::DoFileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();

  OperationProgress->SetTransferSize(TransferSize);

  if (FFileTransferResumed > 0)
  {
    OperationProgress->AddResumed(FFileTransferResumed);
    FFileTransferResumed = 0;
  }

  int64_t Diff = Bytes - OperationProgress->TransferedSize;
  if (DebugAlwaysTrue(Diff >= 0))
  {
    OperationProgress->AddTransfered(Diff);
  }

  if (OperationProgress->Cancel == csCancel)
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    FFileZillaIntf->Cancel();
  }

  if (FFileTransferCPSLimit != OperationProgress->CPSLimit)
  {
    SetCPSLimit(OperationProgress);
  }
}

void TFTPFileSystem::SetCPSLimit(TFileOperationProgressType * OperationProgress)
{
  // Any reason we use separate field instead of directly using OperationProgress->CPSLimit?
  // Maybe thread-safety?
  FFileTransferCPSLimit = OperationProgress->CPSLimit;
  OperationProgress->SetSpeedCounters();
}

void TFTPFileSystem::FileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TGuard Guard(FTransferStatusCriticalSection);

  DoFileTransferProgress(TransferSize, Bytes);
}

void TFTPFileSystem::FileTransfer(const UnicodeString & AFileName,
  const UnicodeString & LocalFile, const UnicodeString & RemoteFile,
  const UnicodeString & RemotePath, bool Get, int64_t Size, intptr_t Type,
  TFileTransferData & UserData, TFileOperationProgressType * OperationProgress)
{
  FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(TRANSFER_ERROR, AFileName.c_str()), "",
  [&]()
  {
    FFileZillaIntf->FileTransfer(ApiPath(LocalFile).c_str(), RemoteFile.c_str(),
      RemotePath.c_str(), Get, Size, static_cast<int>(Type), &UserData);
    // we may actually catch response code of the listing
    // command (when checking for existence of the remote file)
    uintptr_t Reply = WaitForCommandReply();
    GotReply(Reply, FLAGMASK(FFileTransferCancelled, REPLY_ALLOW_CANCEL));
  });

  switch (FFileTransferAbort)
  {
    case ftaSkip:
      ThrowSkipFileNull();

    case ftaCancel:
      Abort();
      break;
  }

  if (!FFileTransferCancelled)
  {
    // show completion of transfer
    // call non-guarded variant to avoid deadlock with keepalives
    // (we are not waiting for reply anymore so keepalives are free to proceed)
    DoFileTransferProgress(OperationProgress->TransferSize, OperationProgress->TransferSize);
  }
}

void TFTPFileSystem::CopyToLocal(const TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  Params &= ~cpAppend;
  UnicodeString FullTargetDir = ::IncludeTrailingBackslash(TargetDir);

  intptr_t Index = 0;
  while (Index < AFilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    UnicodeString FileName = AFilesToCopy->GetString(Index);
    const TRemoteFile * File = NB_STATIC_DOWNCAST_CONST(TRemoteFile, AFilesToCopy->GetObj(Index));

    bool Success = false;
    try__finally
    {
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
      };
      UnicodeString AbsoluteFilePath = GetAbsolutePath(FileName, false);
      UnicodeString TargetDirectory = CreateTargetDirectory(File->GetFileName(), FullTargetDir, CopyParam);
      try
      {
        SinkRobust(AbsoluteFilePath, File, TargetDirectory, CopyParam, Params,
          OperationProgress, tfFirstLevel);
        Success = true;
        FLastDataSent = Now();
      }
      catch (ESkipFile & E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
    }
    __finally
    {
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    };
    ++Index;
  }
}

void TFTPFileSystem::SinkRobust(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  // the same in TSFTPFileSystem

  TDownloadSessionAction Action(FTerminal->GetActionLog());
  TRobustOperationLoop RobustLoop(FTerminal, OperationProgress);

  do
  {
    try
    {
      Sink(AFileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action);
    }
    catch (Exception & E)
    {
      if (!RobustLoop.TryReopen(E))
      {
        FTerminal->RollbackAction(Action, OperationProgress, &E);
        throw;
      }
    }

    if (RobustLoop.ShouldRetry())
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      DebugAssert(AFile != nullptr);
      if (!AFile->GetIsDirectory())
      {
        // prevent overwrite confirmations
        Params |= cpNoConfirmation;
        Flags |= tfAutoResume;
      }
    }
  }
  while (RobustLoop.Retry());
}

void TFTPFileSystem::Sink(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t AParams,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TDownloadSessionAction & Action)
{
  DebugAssert(AFile);
  UnicodeString OnlyFileName = base::UnixExtractFileName(AFileName);

  Action.SetFileName(AFileName);

  TFileMasks::TParams MaskParams;
  MaskParams.Size = AFile->GetSize();
  MaskParams.Modification = AFile->GetModification();

  UnicodeString BaseFileName = FTerminal->GetBaseFileName(AFileName);
  if (!CopyParam->AllowTransfer(BaseFileName, osRemote, AFile->GetIsDirectory(), MaskParams))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", AFileName.c_str()));
    ThrowSkipFileNull();
  }

  if (CopyParam->SkipTransfer(AFileName, AFile->GetIsDirectory()))
  {
    OperationProgress->AddSkippedFileSize(AFile->GetSize());
    ThrowSkipFileNull();
  }

  FTerminal->LogFileDetails(AFileName, AFile->GetModification(), AFile->GetSize());

  OperationProgress->SetFile(OnlyFileName);

  UnicodeString DestFileName =
    FTerminal->ChangeFileName(
      CopyParam, OnlyFileName, osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = TargetDir + DestFileName;

  if (AFile->GetIsDirectory())
  {
    Action.Cancel();
    if (FTerminal->CanRecurseToDirectory(AFile))
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName.c_str()), "",
      [&]()
      {
        DWORD LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
        if (FLAGCLEAR(LocalFileAttrs, faDirectory))
        {
          ThrowExtException();
        }
      });

      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CREATE_DIR_ERROR, DestFullName.c_str()), "",
      [&]()
      {
        THROWOSIFFALSE(::ForceDirectories(DestFullName));
      });

      TSinkFileParams SinkFileParams;
      SinkFileParams.TargetDir = ::IncludeTrailingBackslash(DestFullName);
      SinkFileParams.CopyParam = CopyParam;
      SinkFileParams.Params = AParams;
      SinkFileParams.OperationProgress = OperationProgress;
      SinkFileParams.Skipped = false;
      SinkFileParams.Flags = Flags & ~(tfFirstLevel | tfAutoResume);

      FTerminal->ProcessDirectory(AFileName, MAKE_CALLBACK(TFTPFileSystem::SinkFile, this), &SinkFileParams);

      // Do not delete directory if some of its files were skipped.
      // Throw "skip file" for the directory to avoid attempt to deletion
      // of any parent directory
      if (FLAGSET(AParams, cpDelete) && SinkFileParams.Skipped)
      {
        ThrowSkipFileNull();
      }
    }
    else
    {
      FTerminal->LogEvent(FORMAT(L"Skipping symlink to directory \"%s\".", AFileName.c_str()));
    }
  }
  else
  {
    FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to local directory started.", AFileName.c_str()));

    // Will we use ASCII of BINARY file transfer?
    OperationProgress->SetAsciiTransfer(
      CopyParam->UseAsciiTransfer(BaseFileName, osRemote, MaskParams));
    FTerminal->LogEvent(UnicodeString(OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary") +
      L" transfer mode selected.");

    // Suppose same data size to transfer as to write
    OperationProgress->SetTransferSize(AFile->GetSize());
    OperationProgress->SetLocalSize(OperationProgress->TransferSize);

    DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(NOT_FILE_ERROR, DestFullName.c_str()), "",
    [&]()
    {
      LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
      if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, faDirectory))
      {
        ThrowExtException();
      }
    });

    OperationProgress->TransferingFile = false; // not set with FTP protocol

    ResetFileTransfer();

    TFileTransferData UserData;

    UnicodeString FilePath = core::UnixExtractFilePath(AFileName);
    if (FilePath.IsEmpty())
    {
      FilePath = ROOTDIRECTORY;
    }
    uintptr_t TransferType = (OperationProgress->AsciiTransfer ? 1 : 2);

    {
      // ignore file list
      TFTPFileListHelper Helper(this, nullptr, true);

      SetCPSLimit(OperationProgress);
      FFileTransferPreserveTime = CopyParam->GetPreserveTime();
      // not used for downloads anyway
      FFileTransferRemoveBOM = CopyParam->GetRemoveBOM();
      FFileTransferNoList = CanTransferSkipList(AParams, Flags, CopyParam);
      UserData.FileName = DestFileName;
      UserData.Params = AParams;
      UserData.AutoResume = FLAGSET(Flags, tfAutoResume);
      UserData.CopyParam = CopyParam;
      UserData.Modification = AFile->GetModification();
      FileTransfer(AFileName, DestFullName, OnlyFileName,
        FilePath, true, AFile->GetSize(), TransferType, UserData, OperationProgress);
    }

    // in case dest filename is changed from overwrite dialog
    if (DestFileName != UserData.FileName)
    {
      DestFullName = TargetDir + UserData.FileName;
      LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
    }

    Action.Destination(::ExpandUNCFileName(DestFullName));

    if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
    {
      LocalFileAttrs = faArchive;
    }
    DWORD NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
    if ((NewAttrs & LocalFileAttrs) != NewAttrs)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DestFullName.c_str()), "",
      [&]()
      {
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DestFullName), (LocalFileAttrs | NewAttrs)) == 0);
      });
    }

    FTerminal->LogFileDone(OperationProgress);
  }

  if (FLAGSET(AParams, cpDelete))
  {
    // If file is directory, do not delete it recursively, because it should be
    // empty already. If not, it should not be deleted (some files were
    // skipped or some new files were copied to it, while we were downloading)
    intptr_t Params = dfNoRecursive;
    FTerminal->RemoteDeleteFile(AFileName, AFile, &Params);
  }
}

void TFTPFileSystem::SinkFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * Param)
{
  TSinkFileParams * Params = NB_STATIC_DOWNCAST(TSinkFileParams, Param);
  DebugAssert(Params->OperationProgress);
  try
  {
    SinkRobust(AFileName, AFile, Params->TargetDir, Params->CopyParam,
      Params->Params, Params->OperationProgress, Params->Flags);
  }
  catch (ESkipFile & E)
  {
    TFileOperationProgressType * OperationProgress = Params->OperationProgress;

    Params->Skipped = true;

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      if (!FTerminal->HandleException(&E))
      {
        throw;
      }
    }

    if (OperationProgress->Cancel)
    {
      Abort();
    }
  }
}

void TFTPFileSystem::CopyToRemote(const TStrings * AFilesToCopy,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  DebugAssert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));

  Params &= ~cpAppend;
  UnicodeString FileName, FileNameOnly;
  UnicodeString TargetDir = GetAbsolutePath(ATargetDir, false);
  UnicodeString FullTargetDir = core::UnixIncludeTrailingBackslash(TargetDir);
  intptr_t Index = 0;
  while ((Index < AFilesToCopy->GetCount()) && !OperationProgress->Cancel)
  {
    bool Success = false;
    FileName = AFilesToCopy->GetString(Index);
    TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, AFilesToCopy->GetObj(Index));
    UnicodeString RealFileName = File ? File->GetFileName() : FileName;
    FileNameOnly = base::ExtractFileName(RealFileName, false);

    try__finally
    {
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
      };
      try
      {
        if (FTerminal->GetSessionData()->GetCacheDirectories())
        {
          FTerminal->DirectoryModified(TargetDir, false);

          if (::DirectoryExists(ApiPath(::ExtractFilePath(FileName))))
          {
            FTerminal->DirectoryModified(FullTargetDir + FileNameOnly, true);
          }
        }
        SourceRobust(FileName, File, FullTargetDir, CopyParam, Params, OperationProgress,
          tfFirstLevel);
        Success = true;
        FLastDataSent = Now();
      }
      catch (ESkipFile & E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
    }
    __finally
    {
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    };
    ++Index;
  }
}

void TFTPFileSystem::SourceRobust(const UnicodeString & AFileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  // the same in TSFTPFileSystem

  TUploadSessionAction Action(FTerminal->GetActionLog());
  TRobustOperationLoop RobustLoop(FTerminal, OperationProgress);
  TOpenRemoteFileParams OpenParams;
  OpenParams.OverwriteMode = omOverwrite;
  TOverwriteFileParams FileParams;

  do
  {
    try
    {
      Source(AFileName, AFile, TargetDir, CopyParam, Params, &OpenParams, &FileParams, OperationProgress,
        Flags, Action);
    }
    catch (Exception & E)
    {
      if (!RobustLoop.TryReopen(E))
      {
        FTerminal->RollbackAction(Action, OperationProgress, &E);
        throw;
      }
    }

    if (RobustLoop.ShouldRetry())
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      // prevent overwrite confirmations
      // (should not be set for directories!)
      Params |= cpNoConfirmation;
      Flags |= tfAutoResume;
    }
  }
  while (RobustLoop.Retry());
}

bool TFTPFileSystem::CanTransferSkipList(intptr_t Params, uintptr_t Flags, const TCopyParamType * CopyParam) const
{
  bool Result =
    FLAGSET(Params, cpNoConfirmation) &&
    // cpAppend is not supported with FTP
    DebugAlwaysTrue(FLAGCLEAR(Params, cpAppend)) &&
    FLAGCLEAR(Params, cpResume) &&
    FLAGCLEAR(Flags, tfAutoResume) &&
    !CopyParam->GetNewerOnly();
  return Result;
}

// Copy file to remote host
void TFTPFileSystem::Source(const UnicodeString & AFileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TOpenRemoteFileParams * OpenParams,
  TOverwriteFileParams * FileParams,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TUploadSessionAction & Action)
{
  UnicodeString RealFileName = AFile ? AFile->GetFileName() : AFileName;
  Action.SetFileName(::ExpandUNCFileName(AFileName));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(AFileName, CopyParam, OperationProgress))
  {
    // FTerminal->LogEvent(FORMAT("File \"%s\" excluded from transfer", RealFileName.c_str()));
    ThrowSkipFileNull();
  }

  int64_t MTime = 0, ATime = 0;
  int64_t Size = 0;

  FTerminal->TerminalOpenLocalFile(AFileName, GENERIC_READ,
    nullptr, &OpenParams->LocalFileAttrs, nullptr, &MTime, &ATime, &Size);

  OperationProgress->SetFileInProgress();

  bool Dir = FLAGSET(OpenParams->LocalFileAttrs, faDirectory);
  if (Dir)
  {
    Action.Cancel();
    DirectorySource(::IncludeTrailingBackslash(RealFileName), TargetDir,
      OpenParams->LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
  }
  else
  {
    UnicodeString DestFileName =
      FTerminal->ChangeFileName(
        CopyParam, base::ExtractFileName(RealFileName, false), osLocal,
        FLAGSET(Flags, tfFirstLevel));

    FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", RealFileName.c_str()));

    OperationProgress->SetLocalSize(Size);

    // Suppose same data size to transfer as to read
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(OperationProgress->LocalSize);
    OperationProgress->TransferingFile = false;

    TDateTime Modification;
    // Inspired by Sysutils::FileAge
    WIN32_FIND_DATA FindData;
    HANDLE LocalFileHandle = ::FindFirstFile(ApiPath(AFileName).c_str(), &FindData);
    if (LocalFileHandle != INVALID_HANDLE_VALUE)
    {
      Modification =
        ::UnixToDateTime(
          ::ConvertTimestampToUnixSafe(FindData.ftLastWriteTime, dstmUnix),
          dstmUnix);
      ::FindClose(LocalFileHandle);
    }

    // Will we use ASCII of BINARY file transfer?
    TFileMasks::TParams MaskParams;
    MaskParams.Size = Size;
    MaskParams.Modification = Modification;
    UnicodeString BaseFileName = FTerminal->GetBaseFileName(RealFileName);
    OperationProgress->SetAsciiTransfer(
      CopyParam->UseAsciiTransfer(BaseFileName, osLocal, MaskParams));
    FTerminal->LogEvent(
      UnicodeString(OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary") +
        L" transfer mode selected.");

    ResetFileTransfer();

    TFileTransferData UserData;

    uintptr_t TransferType = (OperationProgress->AsciiTransfer ? 1 : 2);

    // should we check for interrupted transfer?
    bool ResumeAllowed = !OperationProgress->AsciiTransfer &&
                         CopyParam->AllowResume(OperationProgress->LocalSize) &&
                         IsCapable(fcRename);
//    OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

    FileParams->SourceSize = OperationProgress->LocalSize;
    FileParams->SourceTimestamp = ::UnixToDateTime(MTime,
                                  FTerminal->GetSessionData()->GetDSTMode());
    bool DoResume = (ResumeAllowed && (OpenParams->OverwriteMode == omOverwrite));
    {
      // ignore file list
      TFTPFileListHelper Helper(this, nullptr, true);

      SetCPSLimit(OperationProgress);
      // not used for uploads anyway
      FFileTransferPreserveTime = CopyParam->GetPreserveTime();
      FFileTransferRemoveBOM = CopyParam->GetRemoveBOM();
      FFileTransferNoList = CanTransferSkipList(Params, Flags, CopyParam);
      // not used for uploads, but we get new name (if any) back in this field
      UserData.FileName = DestFileName;
      UserData.Params = Params;
      UserData.AutoResume = FLAGSET(Flags, tfAutoResume) || DoResume;
      UserData.CopyParam = CopyParam;
      UserData.Modification = Modification;
      FileTransfer(RealFileName, AFileName, DestFileName,
        TargetDir, false, Size, TransferType, UserData, OperationProgress);
    }

    UnicodeString DestFullName = TargetDir + UserData.FileName;
    // only now, we know the final destination
    Action.Destination(DestFullName);

    // We are not able to tell if setting timestamp succeeded,
    // so we log it always (if supported).
    // Support for MDTM does not necessarily mean that the server supports
    // non-standard hack of setting timestamp using
    // MFMT-like (two argument) call to MDTM.
    // IIS definitely does.
    if (FFileTransferPreserveTime &&
        ((FServerCapabilities->GetCapability(mfmt_command) == yes) ||
         ((FServerCapabilities->GetCapability(mdtm_command) == yes))))
    {
      TTouchSessionAction TouchAction(FTerminal->GetActionLog(), DestFullName, Modification);

      if (!FFileZillaIntf->UsingMlsd())
      {
        FUploadedTimes[DestFullName] = Modification;
        if ((FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2))
        {
          FTerminal->LogEvent(
            FORMAT(L"Remembering modification time of \"%s\" as [%s]",
                   DestFullName.c_str(), StandardTimestamp(FUploadedTimes[DestFullName]).c_str()));
        }
      }
    }

    FTerminal->LogFileDone(OperationProgress);
  }

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, AFileName.c_str()), "",
      [&]()
      {
        THROWOSIFFALSE(::RemoveFile(AFileName));
      });
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(OpenParams->LocalFileAttrs, faArchive))
  {
    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, AFileName.c_str()), "",
    [&]()
    {
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(AFileName, OpenParams->LocalFileAttrs & ~faArchive) == 0);
    });
  }
}

void TFTPFileSystem::DirectorySource(const UnicodeString & DirectoryName,
  const UnicodeString & TargetDir, intptr_t Attrs, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  UnicodeString DestDirectoryName =
    FTerminal->ChangeFileName(
      CopyParam,
      base::ExtractFileName(::ExcludeTrailingBackslash(DirectoryName), false),
      osLocal, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = core::UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);

  OperationProgress->SetFile(DirectoryName);

  DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  TSearchRecChecked SearchRec;
  bool FindOK = false;

  FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()), "",
  [&]()
  {
    FindOK =
      ::FindFirstChecked((DirectoryName + L"*.*").c_str(),
        FindAttrs, SearchRec) == 0;
  });

  bool CreateDir = true;

  try__finally
  {
    SCOPE_EXIT
    {
      FindClose(SearchRec);
    };
    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = DirectoryName + SearchRec.Name;
      try
      {
        if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
        {
          SourceRobust(FileName, nullptr, DestFullName, CopyParam, Params, OperationProgress,
            Flags & ~(tfFirstLevel | tfAutoResume));
          // if any file got uploaded (i.e. there were any file in the
          // directory and at least one was not skipped),
          // do not try to create the directory,
          // as it should be already created by FZAPI during upload
          CreateDir = false;
        }
      }
      catch (ESkipFile & E)
      {
        // If ESkipFile occurs, just log it and continue with next file
        TSuspendFileOperationProgress Suspend(OperationProgress);
        // here a message to user was displayed, which was not appropriate
        // when user refused to overwrite the file in subdirectory.
        // hopefully it won't be missing in other situations.
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }

      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()), "",
      [&]()
      {
        FindOK = (::FindNextChecked(SearchRec) == 0);
      });
    }
  }
  __finally
  {
    FindClose(SearchRec);
  };

  if (CreateDir)
  {
    TRemoteProperties Properties;
    if (CopyParam->GetPreserveRights())
    {
      Properties.Valid = TValidProperties() << vpRights;
      Properties.Rights = CopyParam->RemoteFileRights(Attrs);
    }

    try
    {
      FTerminal->SetExceptionOnFail(true);
      try__finally
      {
        SCOPE_EXIT
        {
          FTerminal->SetExceptionOnFail(false);
        };
        FTerminal->RemoteCreateDirectory(DestFullName, &Properties);
      }
      __finally
      {
          FTerminal->SetExceptionOnFail(false);
      };
    }
    catch (...)
    {
      TRemoteFile * File = nullptr;
      // ignore non-fatal error when the directory already exists
      UnicodeString Fn = core::UnixExcludeTrailingBackslash(DestFullName);
      if (Fn.IsEmpty())
      {
        Fn = ROOTDIRECTORY;
      }
      bool Rethrow =
        !FTerminal->GetActive() ||
        !FTerminal->FileExists(Fn, &File) ||
        !File->GetIsDirectory();
      SAFE_DESTROY(File);
      if (Rethrow)
      {
        throw;
      }
    }
  }

  /* TODO : Delete also read-only directories. */
  /* TODO : Show error message on failure. */
  if (!OperationProgress->Cancel)
  {
    if (FLAGSET(Params, cpDelete))
    {
      FTerminal->RemoveLocalDirectory(ApiPath(DirectoryName));
    }
    else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()), "",
      [&]()
      {
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DirectoryName, Attrs & ~faArchive) == 0);
      });
    }
  }
}

void TFTPFileSystem::RemoteCreateDirectory(const UnicodeString & ADirName)
{
  UnicodeString DirName = GetAbsolutePath(ADirName, false);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true);

    FFileZillaIntf->MakeDir(DirName.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}

void TFTPFileSystem::CreateLink(const UnicodeString & AFileName,
  const UnicodeString & PointTo, bool Symbolic)
{
  DebugAssert(SupportsSiteCommand(SymlinkSiteCommand));
  if (DebugAlwaysTrue(Symbolic))
  {
    EnsureLocation();

    UnicodeString Command = FORMAT(L"%s %s %s %s", SiteCommand.c_str(), SymlinkSiteCommand.c_str(), PointTo.c_str(), AFileName.c_str());
    SendCommand(Command);
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}

void TFTPFileSystem::RemoteDeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action)
{
  UnicodeString FileName = GetAbsolutePath(AFileName, false);
  UnicodeString FileNameOnly = base::UnixExtractFileName(FileName);
  UnicodeString FilePath = core::UnixExtractFilePath(FileName);

  bool Dir = (AFile != nullptr) && AFile->GetIsDirectory() && FTerminal->CanRecurseToDirectory(AFile);

  if (Dir && FLAGCLEAR(Params, dfNoRecursive))
  {
    try
    {
      FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TTerminal::RemoteDeleteFile, FTerminal), &Params);
    }
    catch (...)
    {
      Action.Cancel();
      throw;
    }
  }

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true);

    if (Dir)
    {
      // If current remote directory is in the directory being removed,
      // some servers may refuse to delete it
      // This is common as ProcessDirectory above would CWD to
      // the directory to LIST it.
      // EnsureLocation should reset actual current directory to user's working directory.
      // If user's working directory is still below deleted directory, it is
      // perfectly correct to report an error.
      if (core::UnixIsChildPath(ActualCurrentDirectory(), FileName))
      {
        EnsureLocation();
      }
      FFileZillaIntf->RemoveDir(FileNameOnly.c_str(), FilePath.c_str());
    }
    else
    {
      FFileZillaIntf->Delete(FileNameOnly.c_str(), FilePath.c_str());
    }
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}

void TFTPFileSystem::CustomCommandOnFile(const UnicodeString & /*FileName*/,
  const TRemoteFile * /*File*/, const UnicodeString & /*Command*/, intptr_t /*Params*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  // if ever implemented, do not forget to add EnsureLocation,
  // see AnyCommand for a reason why
  DebugFail();
}

void TFTPFileSystem::DoStartup()
{
  std::unique_ptr<TStrings> PostLoginCommands(new TStringList());
  try__finally
  {
    PostLoginCommands->SetText(FTerminal->GetSessionData()->GetPostLoginCommands());
    for (intptr_t Index = 0; Index < PostLoginCommands->GetCount(); ++Index)
    {
      UnicodeString Command = PostLoginCommands->GetString(Index);
      if (!Command.IsEmpty())
      {
        SendCommand(Command);

        GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_3XX_CODE);
      }
    }
  }
  __finally
  {
//    delete PostLoginCommands;
  };

  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FHomeDirectory = FCurrentDirectory;
}

void TFTPFileSystem::HomeDirectory()
{
  // FHomeDirectory is an absolute path, so avoid unnecessary overhead
  // of ChangeDirectory, such as EnsureLocation
  DoChangeDirectory(FHomeDirectory);
  FCurrentDirectory = FHomeDirectory;
  FReadCurrentDirectory = false;
  // make sure FZAPI is aware that we changed current working directory
  FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
}

bool TFTPFileSystem::IsCapable(intptr_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability)
  {
    case fcResolveSymlink: // sic
    case fcTextMode:
    case fcModeChanging: // but not fcModeChangingUpload
    case fcNewerOnlyUpload:
    case fcAnyCommand: // but not fcShellAnyCommand
    case fcRename:
    case fcRemoteMove:
    case fcRemoveBOMUpload:
    case fcMoveToQueue:
      return true;

    case fcPreservingTimestampUpload:
      return (FServerCapabilities->GetCapability(mfmt_command) == yes);

    case fcRemoteCopy:
      return SupportsSiteCommand(CopySiteCommand);

    case fcSymbolicLink:
      return SupportsSiteCommand(SymlinkSiteCommand);

    case fcCalculatingChecksum:
      return FSupportsAnyChecksumFeature;

    case fcCheckingSpaceAvailable:
      return FBytesAvailableSupported || SupportsCommand(AvblCommand) || SupportsCommand(XQuotaCommand);

    case fcModeChangingUpload:
    case fcLoadingAdditionalProperties:
    case fcShellAnyCommand:
    case fcHardLink:
    case fcUserGroupListing:
    case fcGroupChanging:
    case fcOwnerChanging:
    case fcGroupOwnerChangingByID:
    case fcSecondaryShell:
    case fcNativeTextMode:
    case fcTimestampChanging:
    case fcIgnorePermErrors:
    case fcRemoveCtrlZUpload:
    case fcLocking:
    case fcPreservingTimestampDirs:
    case fcResumeSupport:
      return false;

    default:
      DebugFail();
      return false;
  }
}

void TFTPFileSystem::LookupUsersGroups()
{
  DebugFail();
}

void TFTPFileSystem::ReadCurrentDirectory()
{
  // ask the server for current directory on startup only
  // and immediately after call to CWD,
  // later our current directory may be not synchronized with FZAPI current
  // directory anyway, see comments in EnsureLocation
  if (FReadCurrentDirectory || DebugAlwaysFalse(FCurrentDirectory.IsEmpty()))
  {
    UnicodeString Command = L"PWD";
    SendCommand(Command);

    uintptr_t Code = 0;
    TStrings * Response = nullptr;
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE, L"", &Code, &Response);

    std::unique_ptr<TStrings> ResponsePtr(Response);

    try__finally
    {
      DebugAssert(ResponsePtr.get() != nullptr);
      bool Result = false;

      // the only allowed 2XX codes to "PWD"
      if (((Code == 257) && (ResponsePtr->GetCount() == 1)) ||
          (Code == 250)) // RTEMS FTP server sends 250 "/" is the current directory. http://bugs.farmanager.com/view.php?id=3090
      {
        UnicodeString Path = ResponsePtr->GetText();

        intptr_t P = Path.Pos(L"\"");
        if (P == 0)
        {
          // some systems use single quotes, be tolerant
          P = Path.Pos(L"'");
        }
        if (P != 0)
        {
          Path.Delete(1, P - 1);

          if (Unquote(Path))
          {
            Result = true;
          }
        }
        else
        {
          P = Path.Pos(L" ");
          Path.Delete(P, Path.Length() - P + 1);
          Result = true;
        }

        if (Result)
        {
          if ((Path.Length() > 0) && (Path[1] != LOTHER_SLASH))
          {
            Path = L"/" + Path;
          }
          FCurrentDirectory = core::AbsolutePath(ROOTDIRECTORY, core::UnixExcludeTrailingBackslash(Path));
          if (FCurrentDirectory.IsEmpty())
          {
            FCurrentDirectory = ROOTDIRECTORY;
          }
          FReadCurrentDirectory = false;
        }
      }

      if (Result)
      {
        FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
      }
      else
      {
        throw Exception(FMTLOAD(FTP_RESPONSE_ERROR, Command.c_str(), Response->GetText().c_str()));
      }
    }
    __finally
    {
//      delete Response;
    };
  }
}

void TFTPFileSystem::DoReadDirectory(TRemoteFileList * FileList)
{
  FBytesAvailable = -1;
  FileList->Reset();
  // FZAPI does not list parent directory, add it
  FileList->AddFile(new TRemoteParentDirectory(FTerminal));

  FLastReadDirectoryProgress = 0;

  TFTPFileListHelper Helper(this, FileList, false);

  // always specify path to list, do not attempt to list "current" dir as:
  // 1) List() lists again the last listed directory, not the current working directory
  // 2) we handle this way the cached directory change
  UnicodeString Directory = GetAbsolutePath(FileList->GetDirectory(), false);
  FFileZillaIntf->List(Directory.c_str());

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);

  AutoDetectTimeDifference(FileList);

  if ((FTimeDifference != 0) || !FUploadedTimes.empty())// optimization
  {
    for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
    {
      ApplyTimeDifference(FileList->GetFile(Index));
    }
  }

  FLastDataSent = Now();
}

void TFTPFileSystem::ApplyTimeDifference(TRemoteFile * File)
{
  if (!File)
    return;
  DebugAssert(File->GetModification() == File->GetLastAccess());
  File->ShiftTimeInSeconds(FTimeDifference);

  if (File->GetModificationFmt() != mfFull)
  {
    TUploadedTimes::iterator Iterator = FUploadedTimes.find(File->GetFullFileName());
    if (Iterator != FUploadedTimes.end())
    {
      TDateTime UploadModification = Iterator->second;
      TDateTime UploadModificationReduced = core::ReduceDateTimePrecision(UploadModification, File->GetModificationFmt());
      if (UploadModificationReduced == File->GetModification())
      {
        if ((FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2))
        {
          FTerminal->LogEvent(
            FORMAT(L"Enriching modification time of \"%s\" from [%s] to [%s]",
                   File->GetFullFileName().c_str(), StandardTimestamp(File->GetModification()).c_str(), StandardTimestamp(UploadModification).c_str()));
        }
        // implicitly sets ModificationFmt to mfFull
        File->SetModification(UploadModification);
      }
      else
      {
        if ((FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2))
        {
          FTerminal->LogEvent(
            FORMAT(L"Remembered modification time [%s]/[%s] of \"%s\" is obsolete, keeping [%s]",
                   StandardTimestamp(UploadModification).c_str(), StandardTimestamp(UploadModificationReduced).c_str(), File->GetFullFileName().c_str(), StandardTimestamp(File->GetModification()).c_str()));
        }
        FUploadedTimes.erase(File->GetFullFileName());
      }
    }
  }
}

void TFTPFileSystem::AutoDetectTimeDifference(TRemoteFileList * FileList)
{
  if (FDetectTimeDifference &&
      // Does not support MLST/MLSD, but supports MDTM at least
      !FFileZillaIntf->UsingMlsd() && SupportsReadingFile())
  {
    FTerminal->LogEvent(L"Detecting timezone difference...");

    for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
    {
      TRemoteFile * File = FileList->GetFile(Index);
      // For directories, we do not do MDTM in ReadFile
      // (it should not be problem to use them otherwise).
      // We are also not interested in files with day precision only.
      if (!File->GetIsDirectory() && !File->GetIsSymLink() &&
          File->GetIsTimeShiftingApplicable())
      {
        FDetectTimeDifference = false;

        std::unique_ptr<TRemoteFile> UtcFilePtr;
        try
        {
          TRemoteFile * UtcFile = nullptr;
          ReadFile(File->GetFullFileName(), UtcFile);
          UtcFilePtr.reset(UtcFile);
        }
        catch (Exception & /*E*/)
        {
          if (!FTerminal->GetActive())
          {
            throw;
          }
          FTerminal->LogEvent(FORMAT(L"Failed to retrieve file %s attributes to detect timezone difference", File->GetFullFileName().c_str()));
          break;
        }

        TDateTime UtcModification = UtcFilePtr->GetModification();
        UtcFilePtr.release();

        // MDTM returns seconds, trim those
        UtcModification = core::ReduceDateTimePrecision(UtcModification, File->GetModificationFmt());

        // Time difference between timestamp retrieved using MDTM (UTC converted to local timezone)
        // and using LIST (no conversion, expecting the server uses the same timezone as the client).
        // Note that FormatTimeZone reverses the value.
        FTimeDifference = static_cast<int64_t>(SecsPerDay * (UtcModification - File->GetModification()));

        UnicodeString LogMessage;
        if (FTimeDifference == 0)
        {
          LogMessage = FORMAT(L"No timezone difference detected using file %s", File->GetFullFileName().c_str());
        }
        else
        {
          LogMessage = FORMAT(L"Timezone difference of %s detected using file %s", FormatTimeZone(static_cast<intptr_t>(FTimeDifference)).c_str(), File->GetFullFileName().c_str());
        }
        FTerminal->LogEvent(LogMessage);

        break;
      }
    }

    if (FDetectTimeDifference)
    {
      FTerminal->LogEvent(L"Found no file to use for detecting timezone difference");
    }
  }
}

void TFTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  // whole below "-a" logic is for LIST,
  // if we know we are going to use MLSD, skip it
  if (FTerminal->GetSessionData()->GetFtpUseMlsd() == asOn)
  {
    DoReadDirectory(FileList);
  }
  else
  {
    bool GotNoFilesForAll = false;
    bool Repeat = false;

    do
    {
      Repeat = false;
      try
      {
        FDoListAll = (FListAll == asAuto) || (FListAll == asOn);
        DoReadDirectory(FileList);

        // We got no files with "-a", but again no files w/o "-a",
        // so it was not "-a"'s problem, revert to auto and let it decide the next time
        if (GotNoFilesForAll && (FileList->GetCount() == 0))
        {
          DebugAssert(FListAll == asOff);
          FListAll = asAuto;
        }
        else if (FListAll == asAuto)
        {
          // some servers take "-a" as a mask and return empty directory listing
          // (note that it's actually never empty here, there's always at least parent directory,
          // added explicitly by DoReadDirectory)
          if ((FileList->GetCount() == 0) ||
              ((FileList->GetCount() == 1) && FileList->GetFile(0)->GetIsParentDirectory()))
          {
            Repeat = true;
            FListAll = asOff;
            GotNoFilesForAll = true;
            FTerminal->LogEvent("LIST with -a switch returned empty directory listing, will try pure LIST");
          }
          else
          {
            // reading first directory has succeeded, always use "-a"
            FListAll = asOn;
          }
        }

        // use "-a" even for implicit directory reading by FZAPI?
        // (e.g. before file transfer)
        FDoListAll = (FListAll == asOn);
      }
      catch (Exception &)
      {
        FDoListAll = false;
        // reading the first directory has failed,
        // further try without "-a" only as the server may not support it
        if (FListAll == asAuto)
        {
          FTerminal->LogEvent("LIST with -a failed, will try pure LIST");
          if (!FTerminal->GetActive())
          {
            FTerminal->Reopen(ropNoReadDirectory);
          }

          FListAll = asOff;
          Repeat = true;
        }
        else
        {
          throw;
        }
      }
    }
    while (Repeat);
  }
}

void TFTPFileSystem::DoReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile)
{
  UnicodeString FileName = GetAbsolutePath(AFileName, false);
  UnicodeString FileNameOnly;
  UnicodeString FilePath;
  if (core::IsUnixRootPath(FileName))
  {
    FileNameOnly = FileName;
    FilePath = FileName;
  }
  else
  {
    FileNameOnly = base::UnixExtractFileName(FileName);
    FilePath = core::UnixExtractFilePath(FileName);
  }

  std::unique_ptr<TRemoteFileList> FileList(new TRemoteFileList());
  try__finally
  {
    // Duplicate() call below would use this to compose FullFileName
    FileList->SetDirectory(FilePath);
    TFTPFileListHelper Helper(this, FileList.get(), false);
    FFileZillaIntf->ListFile(FileNameOnly.c_str(), FilePath.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);
    TRemoteFile * File = FileList->FindFile(FileNameOnly.c_str());
    if (File != nullptr)
    {
      AFile = File->Duplicate();
      ApplyTimeDifference(AFile);
    }

    FLastDataSent = Now();
  }
  __finally
  {
//    delete FileList;
  };
}

bool TFTPFileSystem::SupportsReadingFile() const
{
  return
    FFileZillaIntf->UsingMlsd() ||
    ((FServerCapabilities->GetCapability(mdtm_command) == yes) &&
     (FServerCapabilities->GetCapability(size_command) == yes));
}

void TFTPFileSystem::ReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile)
{
  UnicodeString Path = core::UnixExtractFilePath(AFileName);
  UnicodeString NameOnly = base::UnixExtractFileName(AFileName);
  TRemoteFile * File = nullptr;
  bool Own = false;
  if (SupportsReadingFile())
  {
    DoReadFile(AFileName, File);
    Own = true;
  }
  else
  {
    if (core::IsUnixRootPath(AFileName))
    {
      FTerminal->LogEvent(FORMAT(L"%s is a root path", AFileName.c_str()));
      File = new TRemoteDirectoryFile();
      File->SetFullFileName(AFileName);
      File->SetFileName(L"");
    }
    else
    {
      // FZAPI does not have efficient way to read properties of one file.
      // In case we need properties of set of files from the same directory,
      // cache the file list for future
      if ((FFileListCache != nullptr) &&
          core::UnixSamePath(Path, FFileListCache->GetDirectory()) &&
          (core::UnixIsAbsolutePath(FFileListCache->GetDirectory()) ||
          (FFileListCachePath == GetCurrDirectory())))
      {
        File = FFileListCache->FindFile(NameOnly);
      }
      // if cache is invalid or file is not in cache, (re)read the directory
      if (File == nullptr)
      {
        std::unique_ptr<TRemoteFileList> FileListCache(new TRemoteFileList());
        FileListCache->SetDirectory(Path);
        try__catch
        {
          ReadDirectory(FileListCache.get());
        }
        /*catch (...)
        {
          delete FileListCache;
          throw;
        }*/
        // set only after we successfully read the directory,
        // otherwise, when we reconnect from ReadDirectory,
        // the FFileListCache is reset from ResetCache.
        SAFE_DESTROY(FFileListCache);
        FFileListCache = FileListCache.release();
        FFileListCachePath = GetCurrDirectory();

        File = FFileListCache->FindFile(NameOnly);
      }

      Own = false;
    }
  }

  if (File == nullptr)
  {
    AFile = nullptr;
    throw Exception(FMTLOAD(FILE_NOT_EXISTS, AFileName.c_str()));
  }

  DebugAssert(File != nullptr);
  AFile = Own ? File : File->Duplicate();
}

void TFTPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& AFile)
{
  // Resolving symlinks over FTP is big overhead
  // (involves opening TCPIP connection for retrieving "directory listing").
  // Moreover FZAPI does not support that anyway.
  // Though nowadays we could use MLST to read the symlink.
  std::unique_ptr<TRemoteFile> File(new TRemoteFile(SymlinkFile));
  try__catch
  {
    File->SetTerminal(FTerminal);
    File->SetFileName(base::UnixExtractFileName(SymlinkFile->GetLinkTo()));
    // FZAPI treats all symlink target as directories
    File->SetType(FILETYPE_SYMLINK);
    AFile = File.release();
  }
  /*catch (...)
  {
    delete File;
    File = NULL;
    throw;
  }*/
}

void TFTPFileSystem::RemoteRenameFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName)
{
  UnicodeString FileName = GetAbsolutePath(AFileName, false);
  UnicodeString NewName = GetAbsolutePath(ANewName, false);

  UnicodeString FileNameOnly = base::UnixExtractFileName(FileName);
  UnicodeString FilePathOnly = core::UnixExtractFilePath(FileName);
  UnicodeString NewNameOnly = base::UnixExtractFileName(NewName);
  UnicodeString NewPathOnly = core::UnixExtractFilePath(NewName);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true);

    FFileZillaIntf->Rename(FileNameOnly.c_str(), NewNameOnly.c_str(),
      FilePathOnly.c_str(), NewPathOnly.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}

void TFTPFileSystem::RemoteCopyFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName)
{
  DebugAssert(SupportsSiteCommand(CopySiteCommand));
  EnsureLocation();

  UnicodeString Command;

  Command = FORMAT(L"%s CPFR %s", SiteCommand.c_str(), AFileName.c_str());
  SendCommand(Command);
  GotReply(WaitForCommandReply(), REPLY_3XX_CODE);

  Command = FORMAT(L"%s CPTO %s", SiteCommand.c_str(), ANewName.c_str());
  SendCommand(Command);
  GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
}

TStrings * TFTPFileSystem::GetFixedPaths()
{
  return nullptr;
}

void TFTPFileSystem::SpaceAvailable(const UnicodeString & APath,
  TSpaceAvailable & ASpaceAvailable)
{
  if (FBytesAvailableSupported)
  {
    std::unique_ptr<TRemoteFileList> DummyFileList(new TRemoteFileList());
    DummyFileList->SetDirectory(APath);
    ReadDirectory(DummyFileList.get());
    ASpaceAvailable.UnusedBytesAvailableToUser = FBytesAvailable;
  }
  else if (SupportsCommand(XQuotaCommand))
  {
    // WS_FTP:
    // XQUOTA
    // 213-File and disk usage
    //     File count: 3
    //     File limit: 10000
    //     Disk usage: 1532791
    //     Disk limit: 2048000
    // 213 File and disk usage end

    // XQUOTA is global not path-specific
    UnicodeString Command = XQuotaCommand;
    SendCommand(Command);
    TStrings * Response = nullptr;
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE, L"", nullptr, &Response);
    std::unique_ptr<TStrings> ResponseOwner(Response);

    int64_t UsedBytes = -1;
    for (intptr_t Index = 0; Index < Response->GetCount(); Index++)
    {
      // trimming padding
      UnicodeString Line = Trim(Response->GetString(Index));
      UnicodeString Label = CutToChar(Line, L':', true);
      if (SameText(Label, L"Disk usage"))
      {
        UsedBytes = StrToInt64(Line);
      }
      else if (SameText(Label, L"Disk limit") && !SameText(Line, L"unlimited"))
      {
        ASpaceAvailable.BytesAvailableToUser = StrToInt64(Line);
      }
    }

    if ((UsedBytes >= 0) && (ASpaceAvailable.BytesAvailableToUser > 0))
    {
      ASpaceAvailable.UnusedBytesAvailableToUser = ASpaceAvailable.BytesAvailableToUser - UsedBytes;
    }
  }
  else if (SupportsCommand(AvblCommand))
  {
    // draft-peterson-streamlined-ftp-command-extensions-10
    // Implemented by Serv-U.
    UnicodeString Command = FORMAT(L"%s %s", AvblCommand.c_str(), APath.c_str());
    SendCommand(Command);
    UnicodeString Response = GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_SINGLE_LINE);
    ASpaceAvailable.UnusedBytesAvailableToUser = StrToInt64(Response);
  }
}

const TSessionInfo & TFTPFileSystem::GetSessionInfo() const
{
  return FSessionInfo;
}

const TFileSystemInfo & TFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  if (!FFileSystemInfoValid)
  {
    FFileSystemInfo.RemoteSystem = FSystem;
    FFileSystemInfo.RemoteSystem.Unique();

    if (FFeatures->GetCount() == 0)
    {
      FFileSystemInfo.AdditionalInfo = LoadStr(FTP_NO_FEATURE_INFO);
    }
    else
    {
      FFileSystemInfo.AdditionalInfo =
        FORMAT(L"%s\r\n", LoadStr(FTP_FEATURE_INFO).c_str());
      for (intptr_t Index = 0; Index < FFeatures->GetCount(); ++Index)
      {
        // For TrimLeft, refer to HandleFeatReply
        FFileSystemInfo.AdditionalInfo += FORMAT(L"  %s\r\n", TrimLeft(FFeatures->GetString(Index)).c_str());
      }
    }

    FTerminal->SaveCapabilities(FFileSystemInfo);

    FFileSystemInfoValid = true;
  }
  return FFileSystemInfo;
}

bool TFTPFileSystem::TemporaryTransferFile(const UnicodeString & /*FileName*/)
{
  return false;
}

bool TFTPFileSystem::GetStoredCredentialsTried() const
{
  return FStoredPasswordTried;
}

UnicodeString TFTPFileSystem::FSGetUserName() const
{
  return FUserName;
}

UnicodeString TFTPFileSystem::GetCurrDirectory() const
{
  return FCurrentDirectory;
}

const wchar_t * TFTPFileSystem::GetOption(intptr_t OptionID) const
{
  TSessionData * Data = FTerminal->GetSessionData();

  switch (OptionID)
  {
    case OPTION_PROXYHOST:
    case OPTION_FWHOST:
      FOptionScratch = Data->GetProxyHost();
      break;

    case OPTION_PROXYUSER:
    case OPTION_FWUSER:
      FOptionScratch = Data->GetProxyUsername();
      break;

    case OPTION_PROXYPASS:
    case OPTION_FWPASS:
      FOptionScratch = Data->GetProxyPassword();
      break;

    case OPTION_TRANSFERIP:
      FOptionScratch = FTerminal->GetConfiguration()->GetExternalIpAddress();
      break;

    case OPTION_ANONPWD:
    case OPTION_TRANSFERIP6:
      FOptionScratch.Clear();
      break;

    default:
      DebugFail();
      FOptionScratch.Clear();
  }

  return FOptionScratch.c_str();
}

intptr_t TFTPFileSystem::GetOptionVal(intptr_t OptionID) const
{
  TSessionData * Data = FTerminal->GetSessionData();
  intptr_t Result;

  switch (OptionID)
  {
    case OPTION_PROXYTYPE:
      switch (Data->GetActualProxyMethod())
      {
        case ::pmNone:
          Result = 0; // PROXYTYPE_NOPROXY;
          break;

        case pmSocks4:
          Result = 2; // PROXYTYPE_SOCKS4A
          break;

        case pmSocks5:
          Result = 3; // PROXYTYPE_SOCKS5
          break;

        case pmHTTP:
          Result = 4; // PROXYTYPE_HTTP11
          break;

        case pmTelnet:
        case pmCmd:
        default:
          DebugFail();
          Result = 0; // PROXYTYPE_NOPROXY;
          break;
      }
      break;

    case OPTION_PROXYPORT:
    case OPTION_FWPORT:
      Result = Data->GetProxyPort();
      break;

    case OPTION_PROXYUSELOGON:
      Result = !Data->GetProxyUsername().IsEmpty();
      break;

    case OPTION_LOGONTYPE:
      Result = Data->GetFtpProxyLogonType();
      break;

    case OPTION_TIMEOUTLENGTH:
      Result = Data->GetTimeout();
      break;

    case OPTION_DEBUGSHOWLISTING:
      Result = TRUE;
      break;

    case OPTION_PASV:
      // should never get here t_server.nPasv being nonzero
      DebugFail();
      Result = FALSE;
      break;

    case OPTION_PRESERVEDOWNLOADFILETIME:
    case OPTION_MPEXT_PRESERVEUPLOADFILETIME:
      Result = FFileTransferPreserveTime ? TRUE : FALSE;
      break;

    case OPTION_LIMITPORTRANGE:
      Result = FALSE;
      break;

    case OPTION_PORTRANGELOW:
    case OPTION_PORTRANGEHIGH:
      // should never get here OPTION_LIMITPORTRANGE being zero
      DebugFail();
      Result = 0;
      break;

    case OPTION_ENABLE_IPV6:
      Result = ((Data->GetAddressFamily() != afIPv4) ? TRUE : FALSE);
      break;

    case OPTION_KEEPALIVE:
      Result = ((Data->GetFtpPingType() != ptOff) ? TRUE : FALSE);
      break;

    case OPTION_INTERVALLOW:
    case OPTION_INTERVALHIGH:
      Result = Data->GetFtpPingInterval();
      break;

    case OPTION_VMSALLREVISIONS:
      Result = FALSE;
      break;

    case OPTION_SPEEDLIMIT_DOWNLOAD_TYPE:
    case OPTION_SPEEDLIMIT_UPLOAD_TYPE:
      Result = (FFileTransferCPSLimit == 0 ? 0 : 1);
      break;

    case OPTION_SPEEDLIMIT_DOWNLOAD_VALUE:
    case OPTION_SPEEDLIMIT_UPLOAD_VALUE:
      Result = static_cast<intptr_t>((FFileTransferCPSLimit / 1024)); // FZAPI expects KB/s
      break;

    case OPTION_MPEXT_SHOWHIDDEN:
      Result = (FDoListAll ? TRUE : FALSE);
      break;

    case OPTION_MPEXT_SSLSESSIONREUSE:
      Result = (Data->GetSslSessionReuse() ? TRUE : FALSE);
      break;

    case OPTION_MPEXT_MIN_TLS_VERSION:
      Result = Data->GetMinTlsVersion();
      break;

    case OPTION_MPEXT_MAX_TLS_VERSION:
      Result = Data->GetMaxTlsVersion();
      break;

    case OPTION_MPEXT_SNDBUF:
      Result = Data->GetSendBuf();
      break;

    case OPTION_MPEXT_TRANSFER_ACTIVE_IMMEDIATELY:
      Result = FTransferActiveImmediately;
      break;

    case OPTION_MPEXT_REMOVE_BOM:
      Result = FFileTransferRemoveBOM ? TRUE : FALSE;
      break;

    case OPTION_MPEXT_LOG_SENSITIVE:
      Result = FTerminal->GetConfiguration()->GetLogSensitive() ? TRUE : FALSE;
      break;

    case OPTION_MPEXT_HOST:
      Result = (Data->GetFtpHost() == asOn);
      break;

    case OPTION_MPEXT_NODELAY:
      Result = Data->GetTcpNoDelay();
      break;

    case OPTION_MPEXT_NOLIST:
      Result = FFileTransferNoList ? TRUE : FALSE;
      break;

    default:
      DebugFail();
      Result = FALSE;
      break;
  }

  return Result;
}

bool TFTPFileSystem::FTPPostMessage(uintptr_t Type, WPARAM wParam, LPARAM lParam)
{
  if (Type == TFileZillaIntf::MSG_TRANSFERSTATUS)
  {
    // Stop here if FileTransferProgress is proceeding,
    // it makes "pause" in queue work.
    // Paused queue item stops in some of the TFileOperationProgressType
    // methods called from FileTransferProgress
    TGuard Guard(FTransferStatusCriticalSection);
  }

  TGuard Guard(FQueueCriticalSection);

  FQueue.push_back(TMessageQueue::value_type(wParam, lParam));
  ::SetEvent(FQueueEvent);

  return true;
}

bool TFTPFileSystem::ProcessMessage()
{
  bool Result;
  TMessageQueue::value_type Message;

  {
    TGuard Guard(FQueueCriticalSection);

    Result = !FQueue.empty();
    if (Result)
    {
      Message = FQueue.front();
      FQueue.erase(FQueue.begin());
    }
    else
    {
      // now we are perfectly sure that the queue is empty as it is locked,
      // so reset the event
      ::ResetEvent(FQueueEvent);
    }
  }

  if (Result)
  {
    FFileZillaIntf->HandleMessage(Message.wparam, Message.lparam);
  }

  return Result;
}

void TFTPFileSystem::DiscardMessages()
{
  while (ProcessMessage());
}

void TFTPFileSystem::WaitForMessages()
{
  //if (FQueue.empty())
  //  return;
//  DWORD Result = ::WaitForSingleObject(FQueueEvent, INFINITE);
  DWORD Result = 0;
  do
  {
    Result = ::WaitForSingleObject(FQueueEvent, GUIUpdateInterval);
    FTerminal->ProcessGUI();
  }
  while (Result == WAIT_TIMEOUT);

  if (Result != WAIT_OBJECT_0)
  {
    FTerminal->FatalError(nullptr, FMTLOAD(INTERNAL_ERROR, L"ftp#1", ::IntToStr(Result).c_str()));
  }
}

void TFTPFileSystem::PoolForFatalNonCommandReply()
{
  DebugAssert(FReply == 0);
  DebugAssert(FCommandReply == 0);
  DebugAssert(!FWaitingForReply);

  FWaitingForReply = true;

  uintptr_t Reply = 0;

  try__finally
  {
    SCOPE_EXIT
    {
      FReply = 0;
      DebugAssert(FCommandReply == 0);
      FCommandReply = 0;
      DebugAssert(FWaitingForReply);
      FWaitingForReply = false;
    };
    // discard up to one reply
    // (it should not happen here that two replies are posted anyway)
    while (ProcessMessage() && (FReply == 0));
    Reply = FReply;
  }
  __finally
  {
    FReply = 0;
    DebugAssert(FCommandReply == 0);
    FCommandReply = 0;
    DebugAssert(FWaitingForReply);
    FWaitingForReply = false;
  };

  if (Reply != 0)
  {
    // throws
    GotNonCommandReply(Reply);
  }
}

bool TFTPFileSystem::NoFinalLastCode() const
{
  return (FLastCodeClass == 0) || (FLastCodeClass == 1);
}

bool TFTPFileSystem::KeepWaitingForReply(uintptr_t & ReplyToAwait, bool WantLastCode) const
{
  // to keep waiting,
  // non-command reply must be unset,
  // the reply we wait for must be unset or
  // last code must be unset (if we wait for it)
  return
     (FReply == 0) &&
     ((ReplyToAwait == 0) ||
      (WantLastCode && NoFinalLastCode()));
}

void TFTPFileSystem::DoWaitForReply(uintptr_t & ReplyToAwait, bool WantLastCode)
{
  try
  {
    while (FTerminal && (FTerminal->GetStatus() != ssClosed) && KeepWaitingForReply(ReplyToAwait, WantLastCode))
    {
      WaitForMessages();
      // wait for the first reply only,
      // i.e. in case two replies are posted get the first only.
      // e.g. when server closes the connection, but posts error message before,
      // sometime it happens that command (like download) fails because of the error
      // and does not catch the disconnection. then asynchronous "disconnect reply"
      // is posted immediately afterwards. leave detection of that to Idle()
      while (ProcessMessage() && KeepWaitingForReply(ReplyToAwait, WantLastCode));
    }

    if (FReply != 0)
    {
      // throws
      GotNonCommandReply(FReply);
    }
  }
  catch (...)
  {
    // even if non-fatal error happens, we must process pending message,
    // so that we "eat" the reply message, so that it gets not mistakenly
    // associated with future connect
    if (FTerminal->GetActive())
    {
      DoWaitForReply(ReplyToAwait, WantLastCode);
    }
    throw;
  }
}

uintptr_t TFTPFileSystem::WaitForReply(bool Command, bool WantLastCode)
{
  DebugAssert(FReply == 0);
  DebugAssert(FCommandReply == 0);
  DebugAssert(!FWaitingForReply);
  DebugAssert(!FTransferStatusCriticalSection.GetAcquired());

  ResetReply();
  FWaitingForReply = true;

  uintptr_t Reply = 0;

  try__finally
  {
    SCOPE_EXIT
    {
      FReply = 0;
      FCommandReply = 0;
      DebugAssert(FWaitingForReply);
      FWaitingForReply = false;
    };
    uintptr_t & ReplyToAwait = (Command ? FCommandReply : FReply);
    DoWaitForReply(ReplyToAwait, WantLastCode);

    Reply = ReplyToAwait;
  }
  __finally
  {
    FReply = 0;
    FCommandReply = 0;
    DebugAssert(FWaitingForReply);
    FWaitingForReply = false;
  };

  return Reply;
}

uintptr_t TFTPFileSystem::WaitForCommandReply(bool WantLastCode)
{
  return WaitForReply(true, WantLastCode);
}

void TFTPFileSystem::WaitForFatalNonCommandReply()
{
  WaitForReply(false, false);
  DebugFail();
}

void TFTPFileSystem::ResetReply()
{
  FLastCode = 0;
  FLastCodeClass = 0;
  DebugAssert(FLastResponse != nullptr);
  FLastResponse->Clear();
  DebugAssert(FLastErrorResponse != nullptr);
  FLastErrorResponse->Clear();
  DebugAssert(FLastError != nullptr);
  FLastError->Clear();
}

void TFTPFileSystem::GotNonCommandReply(uintptr_t Reply)
{
  DebugAssert(FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));
  GotReply(Reply);
  // should never get here as GotReply should raise fatal exception
  DebugFail();
}

UnicodeString TFTPFileSystem::GotReply(uintptr_t Reply, uintptr_t Flags,
  const UnicodeString & Error, uintptr_t * Code, TStrings ** Response)
{
  UnicodeString Result;
  try__finally
  {
    SCOPE_EXIT
    {
      ResetReply();
    };
    if (FLAGSET(Reply, TFileZillaIntf::REPLY_OK))
    {
      DebugAssert(Reply == TFileZillaIntf::REPLY_OK);

      // With REPLY_2XX_CODE treat "OK" non-2xx code like an error.
      // REPLY_3XX_CODE has to be always used along with REPLY_2XX_CODE.
      if ((FLAGSET(Flags, REPLY_2XX_CODE) && (FLastCodeClass != 2)) &&
          ((FLAGCLEAR(Flags, REPLY_3XX_CODE) || (FLastCodeClass != 3))))
      {
        GotReply(TFileZillaIntf::REPLY_ERROR, Flags, Error);
      }
    }
    else if (FLAGSET(Reply, TFileZillaIntf::REPLY_CANCEL) &&
        FLAGSET(Flags, REPLY_ALLOW_CANCEL))
    {
      DebugAssert(
        (Reply == (TFileZillaIntf::REPLY_CANCEL | TFileZillaIntf::REPLY_ERROR)) ||
        (Reply == (TFileZillaIntf::REPLY_ABORTED | TFileZillaIntf::REPLY_CANCEL | TFileZillaIntf::REPLY_ERROR)));
      // noop
    }
    // we do not expect these with our usage of FZ
    else if (Reply &
          (TFileZillaIntf::REPLY_WOULDBLOCK | TFileZillaIntf::REPLY_OWNERNOTSET |
           TFileZillaIntf::REPLY_INVALIDPARAM | TFileZillaIntf::REPLY_ALREADYCONNECTED |
           TFileZillaIntf::REPLY_IDLE | TFileZillaIntf::REPLY_NOTINITIALIZED |
           TFileZillaIntf::REPLY_ALREADYINIZIALIZED))
    {
      FTerminal->FatalError(nullptr, FMTLOAD(INTERNAL_ERROR, L"ftp#2", FORMAT(L"0x%x", static_cast<int>(Reply)).c_str()));
    }
    else
    {
      // everything else must be an error or disconnect notification
      DebugAssert(
        FLAGSET(Reply, TFileZillaIntf::REPLY_ERROR) ||
        FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));

      TODO("REPLY_CRITICALERROR ignored");

      // REPLY_NOTCONNECTED happens if connection is closed between moment
      // when FZAPI interface method dispatches the command to FZAPI thread
      // and moment when FZAPI thread receives the command
      bool Disconnected =
        FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED) ||
        FLAGSET(Reply, TFileZillaIntf::REPLY_NOTCONNECTED);

      UnicodeString HelpKeyword;
      std::unique_ptr<TStrings> MoreMessages(new TStringList());
      try__catch
      {
        if (Disconnected)
        {
          if (FLAGCLEAR(Flags, REPLY_CONNECT))
          {
            MoreMessages->Add(LoadStr(LOST_CONNECTION));
            Discard();
            FTerminal->Closed();
          }
          else
          {
            // For connection failure, do not report that connection was lost,
            // its obvious.
            // Also do not report to terminal that we are closed as
            // that turns terminal into closed mode, but we want to
            // pretend (at least with failed authentication) to retry
            // with the same connection (as with SSH), so we explicitly
            // close terminal in Open() only after we give up
            Discard();
          }
        }

        if (FLAGSET(Reply, TFileZillaIntf::REPLY_ABORTED))
        {
          MoreMessages->Add(LoadStr(USER_TERMINATED));
        }

        if (FLAGSET(Reply, TFileZillaIntf::REPLY_NOTSUPPORTED))
        {
          MoreMessages->Add(LoadStr(FZ_NOTSUPPORTED));
        }

        if (FLastCode == 530)
        {
          // Serv-U also uses this code in response to "SITE PSWD"
          MoreMessages->Add(LoadStr(AUTHENTICATION_FAILED));
        }

        if (FLastCode == 425)
        {
          if (!FTerminal->GetSessionData()->GetFtpPasvMode())
          {
            MoreMessages->Add(LoadStr(FTP_CANNOT_OPEN_ACTIVE_CONNECTION2));
            HelpKeyword = HELP_FTP_CANNOT_OPEN_ACTIVE_CONNECTION;
          }
        }
        if (FLastCode == 421)
        {
          Disconnected = true;
        }

        if (FLastCode == DummyTimeoutCode)
        {
          HelpKeyword = HELP_ERRORMSG_TIMEOUT;
        }

        if (FLastCode == DummyDisconnectCode)
        {
          HelpKeyword = HELP_STATUSMSG_DISCONNECTED;
        }

        MoreMessages->AddStrings(FLastError);
        // already cleared from WaitForReply, but GotReply can be also called
        // from Closed. then make sure that error from previous command not
        // associated with session closure is not reused
        FLastError->Clear();

        MoreMessages->AddStrings(FLastErrorResponse);
        // see comment for FLastError
        FLastResponse->Clear();
        FLastErrorResponse->Clear();

        if (MoreMessages->GetCount() == 0)
        {
          MoreMessages.reset();
        }
      }
      /*catch(...)
      {
        delete MoreMessages;
        throw;
      }*/

      UnicodeString ErrorStr = Error;
      if (ErrorStr.IsEmpty() && (MoreMessages.get() != nullptr))
      {
        DebugAssert(MoreMessages->GetCount() > 0);
        // bit too generic assigning of main instructions, let's see how it works
        ErrorStr = MainInstructions(MoreMessages->GetString(0));
        MoreMessages->Delete(0);
      }

      if (Disconnected)
      {
        // for fatal error, it is essential that there is some message
        DebugAssert(!ErrorStr.IsEmpty());
        std::unique_ptr<ExtException> E(new ExtException(ErrorStr, MoreMessages.release(), true));
        try__finally
        {
          FTerminal->FatalError(E.get(), L"");
        }
        __finally
        {
//          delete E;
        };
      }
      else
      {
        throw ExtException(ErrorStr, MoreMessages.release(), true, UnicodeString(HelpKeyword));
      }
    }

    if ((Code != nullptr) && (FLastCodeClass != DummyCodeClass))
    {
      *Code = static_cast<uintptr_t>(FLastCode);
    }

    if (FLAGSET(Flags, REPLY_SINGLE_LINE))
    {
      if (FLastResponse->GetCount() != 1)
      {
        throw Exception(FMTLOAD(FTP_RESPONSE_ERROR, FLastCommandSent.c_str(), FLastResponse->GetText().c_str()));
      }
      Result = FLastResponse->GetString(0);
    }

    if (Response != nullptr)
    {
      *Response = FLastResponse;
      FLastResponse = new TStringList();
      // just to be consistent
      SAFE_DESTROY(FLastErrorResponse);
      FLastErrorResponse = new TStringList();
    }
  }
  __finally
  {
    ResetReply();
  };
  return Result;
}

void TFTPFileSystem::SendCommand(const UnicodeString & Command)
{
  FFileZillaIntf->CustomCommand(Command.c_str());
  FLastCommandSent = CopyToChar(Command, L' ', false);
}

void TFTPFileSystem::SetLastCode(intptr_t Code)
{
  FLastCode = Code;
  FLastCodeClass = (Code / 100);
}

void TFTPFileSystem::StoreLastResponse(const UnicodeString & Text)
{
  FLastResponse->Add(Text);
  if (FLastCodeClass >= 4)
  {
    FLastErrorResponse->Add(Text);
  }
}

void TFTPFileSystem::HandleReplyStatus(const UnicodeString & Response)
{
  int64_t Code = 0;

  if (FOnCaptureOutput != nullptr)
  {
    FOnCaptureOutput(Response, cotOutput);
  }

  if (FWelcomeMessage.IsEmpty() && StartsStr(L"SSH", Response))
  {
    FLastErrorResponse->Add(LoadStr(SFTP_AS_FTP_ERROR));
  }

  // Two forms of multiline responses were observed
  // (the first is according to the RFC 959):

  // 211-Features:
  //  MDTM
  //  REST STREAM
  //  SIZE
  // 211 End

  // This format is according to RFC 2228.
  // Is used by ProFTPD when  MultilineRFC2228 is enabled
  // http://www.proftpd.org/docs/directives/linked/config_ref_MultilineRFC2228.html

  // 211-Features:
  // 211-MDTM
  // 211-REST STREAM
  // 211-SIZE
  // 211-AUTH TLS
  // 211-PBSZ
  // 211-PROT
  // 211 End

  // IIS 2003:

  // 211-FEAT
  //     SIZE
  //     MDTM
  // 211 END

  // Partially duplicated in CFtpControlSocket::OnReceive

  bool HasCodePrefix =
    (Response.Length() >= 3) &&
    ::TryStrToInt(Response.SubString(1, 3), Code) &&
    (Code >= 100) && (Code <= 599) &&
    ((Response.Length() == 3) || (Response[4] == L' ') || (Response[4] == L'-'));

  if (HasCodePrefix && !FMultineResponse)
  {
    FMultineResponse = (Response.Length() >= 4) && (Response[4] == L'-');
    FLastResponse->Clear();
    FLastErrorResponse->Clear();
    SetLastCode(static_cast<intptr_t>(Code));
    if (Response.Length() >= 5)
    {
      StoreLastResponse(Response.SubString(5, Response.Length() - 4));
    }
  }
  else
  {
    intptr_t Start = 0;
    // response with code prefix
    if (HasCodePrefix && (FLastCode == Code))
    {
      // End of multiline response?
      if ((Response.Length() <= 3) || (Response[4] == L' '))
      {
        FMultineResponse = false;
      }
      Start = 5;
    }
    else
    {
      Start = (((Response.Length() >= 1) && (Response[1] == L' ')) ? 2 : 1);
    }

    // Intermediate empty lines are being added
    if (FMultineResponse || (Response.Length() >= Start))
    {
      StoreLastResponse(Response.SubString(Start, Response.Length() - Start + 1));
    }
  }


  if (StartsStr(DirectoryHasBytesPrefix, Response))
  {
    UnicodeString Buf = Response;
    Buf.Delete(1, DirectoryHasBytesPrefix.Length());
    Buf = Buf.TrimLeft();
    UnicodeString BytesStr = CutToChar(Buf, L' ', true);
    BytesStr = ReplaceStr(BytesStr, L",", L"");
    FBytesAvailable = StrToInt64Def(BytesStr, -1);
    if (FBytesAvailable >= 0)
    {
      FBytesAvailableSupported = true;
    }
  }

  if (!FMultineResponse)
  {
    if (FLastCode == 220)
    {
      // HOST command also uses 220 response.
      // Neither our use of welcome message is prepared for changing it
      // during the session, so we keep the initial message only.
      // Theoretically the welcome message can be host-specific,
      // but IIS uses "220 Host accepted", and we are not interested in that anyway.
      // Serv-U repeats the initial welcome message.
      // WS_FTP uses "200 Command HOST succeed"
      if (FWelcomeMessage.IsEmpty())
      {
        FWelcomeMessage = FLastResponse->GetText();
        if (FTerminal->GetConfiguration()->GetShowFtpWelcomeMessage())
        {
          FTerminal->DisplayBanner(FWelcomeMessage);
        }
        // Idea FTP Server v0.80
        if ((FTerminal->GetSessionData()->GetFtpTransferActiveImmediately() == asAuto) &&
            FWelcomeMessage.Pos(L"Idea FTP Server") > 0)
        {
          FTerminal->LogEvent(L"The server requires TLS/SSL handshake on transfer connection before responding 1yz to STOR/APPE");
          FTransferActiveImmediately = true;
        }
      }
    }
    else if (FLastCommand == PASS)
    {
      FStoredPasswordTried = true;
      // 530 = "Not logged in."
      if (FLastCode == 530)
      {
        FPasswordFailed = true;
      }
    }
    else if (FLastCommand == SYST)
    {
      DebugAssert(FSystem.IsEmpty());
      // Positive reply to "SYST" must be 215, see RFC 959
      if (FLastCode == 215)
      {
        FSystem = FLastResponse->GetText().TrimRight();
        // full name is "Personal FTP Server PRO K6.0"
        if ((FListAll == asAuto) &&
            (FSystem.Pos(L"Personal FTP Server") > 0))
        {
          FTerminal->LogEvent("Server is known not to support LIST -a");
          FListAll = asOff;
        }
        // The FWelcomeMessage usually contains "Microsoft FTP Service" but can be empty
        if (ContainsText(FSystem, L"Windows_NT"))
        {
          FTerminal->LogEvent(L"The server is probably running Windows, assuming that directory listing timestamps are affected by DST.");
          FWindowsServer = true;
        }
      }
      else
      {
        FSystem.Clear();
      }
    }
    else if (FLastCommand == FEAT)
    {
      HandleFeatReply();
    }
  }
}

void TFTPFileSystem::ResetFeatures()
{
  FFeatures->Clear();
  FSupportedCommands->Clear();
  FSupportedSiteCommands->Clear();
  FHashAlgs->Clear();
  FSupportsAnyChecksumFeature = false;
}

void TFTPFileSystem::HandleFeatReply()
{
  ResetFeatures();
  // Response to FEAT must be multiline, where leading and trailing line
  // is "meaningless". See RFC 2389.
  if ((FLastCode == 211) && (FLastResponse->GetCount() > 2))
  {
    FLastResponse->Delete(0);
    FLastResponse->Delete(FLastResponse->GetCount() - 1);
    FFeatures->Assign(FLastResponse);
    for (intptr_t Index = 0; Index < FFeatures->GetCount(); Index++)
    {
      // IIS 2003 indents response by 4 spaces, instead of one,
      // see example in HandleReplyStatus
      UnicodeString Feature = TrimLeft(FFeatures->GetString(Index));

      UnicodeString Args = Feature;
      UnicodeString Command = CutToChar(Args, L' ', true);

      // Serv-U lists Xalg commands like:
      //  XSHA1 filename;start;end
      FSupportedCommands->Add(Command);

      if (SameText(Command, SiteCommand))
      {
        // Serv-U lists all SITE commands in one line like:
        //  SITE PSWD;SET;ZONE;CHMOD;MSG;EXEC;HELP
        // But ProFTPD lists them separately:
        //  SITE UTIME
        //  SITE RMDIR
        //  SITE COPY
        //  SITE MKDIR
        //  SITE SYMLINK
        while (!Args.IsEmpty())
        {
          UnicodeString Arg = CutToChar(Args, L';', true);
          FSupportedSiteCommands->Add(Arg);
        }
      }
      else if (SameText(Command, HashCommand))
      {
        while (!Args.IsEmpty())
        {
          UnicodeString Alg = CutToChar(Args, L';', true);
          if ((Alg.Length() > 0) && (Alg[Alg.Length()] == L'*'))
          {
            Alg.Delete(Alg.Length(), 1);
          }
          // FTP HASH alg names follow IANA as we do,
          // but using uppercase and we use lowercase
          FHashAlgs->Add(LowerCase(Alg));
          FSupportsAnyChecksumFeature = true;
        }
      }

      if (FChecksumCommands->IndexOf(Command) >= 0)
      {
        FSupportsAnyChecksumFeature = true;
      }
    }
  }
}

bool TFTPFileSystem::HandleStatus(const wchar_t * AStatus, int Type)
{
  TLogLineType LogType = static_cast<TLogLineType>(-1);
  UnicodeString Status(AStatus);

  switch (Type)
  {
    case TFileZillaIntf::LOG_STATUS:
      FTerminal->Information(Status, true);
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_COMMAND:
      if (Status == L"SYST")
      {
        // not to trigger the assert in HandleReplyStatus,
        // when SYST command is used by the user
        FSystem.Clear();
        FLastCommand = SYST;
      }
      else if (Status == L"FEAT")
      {
        FLastCommand = FEAT;
      }
      else if (Status.SubString(1, 5) == L"PASS ")
      {
        FLastCommand = PASS;
      }
      else
      {
        FLastCommand = CMD_UNKNOWN;
      }
      LogType = llInput;
      break;

    case TFileZillaIntf::LOG_ERROR:
    case TFileZillaIntf::LOG_APIERROR:
    case TFileZillaIntf::LOG_WARNING:
      // when timeout message occurs, break loop waiting for response code
      // by setting dummy one
      if (Type == TFileZillaIntf::LOG_ERROR)
      {
        if (StartsStr(FTimeoutStatus, Status))
        {
          if (NoFinalLastCode())
          {
            SetLastCode(DummyTimeoutCode);
          }
        }
        else if (Status == FDisconnectStatus)
        {
          if (NoFinalLastCode())
          {
            SetLastCode(DummyDisconnectCode);
          }
        }
      }
      // there can be multiple error messages associated with single failure
      // (such as "cannot open local file..." followed by "download failed")
      FLastError->Add(Status);
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_PROGRESS:
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_REPLY:
      HandleReplyStatus(AStatus);
      LogType = llOutput;
      break;

    case TFileZillaIntf::LOG_INFO:
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_DEBUG:
      LogType = llMessage;
      break;

    default:
      DebugFail();
      break;
  }

  if (FTerminal->GetLog()->GetLogging() && (LogType != static_cast<TLogLineType>(-1)))
  {
    FTerminal->GetLog()->Add(LogType, Status);
  }

  return true;
}

TDateTime TFTPFileSystem::ConvertLocalTimestamp(time_t Time)
{
  // This reverses how FZAPI converts FILETIME to time_t,
  // before passing it to FZ_ASYNCREQUEST_OVERWRITE.
  int64_t Timestamp;
  tm * Tm = gmtime(&Time); // localtime(&Time);
  if (Tm != nullptr)
  {
    SYSTEMTIME SystemTime;
    SystemTime.wYear = ToWord(Tm->tm_year + 1900);
    SystemTime.wMonth = ToWord(Tm->tm_mon + 1);
    SystemTime.wDayOfWeek = 0;
    SystemTime.wDay = ToWord(Tm->tm_mday);
    SystemTime.wHour = ToWord(Tm->tm_hour);
    SystemTime.wMinute = ToWord(Tm->tm_min);
    SystemTime.wSecond = ToWord(Tm->tm_sec);
    SystemTime.wMilliseconds = 0;

    FILETIME LocalTime;
    SystemTimeToFileTime(&SystemTime, &LocalTime);
    FILETIME FileTime;
    ::LocalFileTimeToFileTime(&LocalTime, &FileTime);
    Timestamp = ::ConvertTimestampToUnixSafe(FileTime, dstmUnix);
  }
  else
  {
    // incorrect, but at least something
    Timestamp = Time;
  }

  return ::UnixToDateTime(Timestamp, dstmUnix);
}

bool TFTPFileSystem::HandleAsynchRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
  const wchar_t * Path1, const wchar_t * Path2,
  int64_t Size1, int64_t Size2, time_t LocalTime,
  bool /*HasLocalTime*/, const TRemoteFileTime & RemoteTime, void * AUserData,
  HANDLE & LocalFileHandle,
  int & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    UnicodeString DestFullName = Path1;
    ::AppendPathDelimiterW(DestFullName);
    DestFullName += FileName1;
    TFileTransferData & UserData = *(NB_STATIC_DOWNCAST(TFileTransferData, AUserData));
    if (UserData.OverwriteResult >= 0)
    {
      // on retry, use the same answer as on the first attempt
      RequestResult = UserData.OverwriteResult;
    }
    else
    {
      TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();
      UnicodeString TargetFileName = FileName1;
      DebugAssert(UserData.FileName == TargetFileName);
      TOverwriteMode OverwriteMode = omOverwrite;
      TOverwriteFileParams FileParams;
      bool NoFileParams =
        (Size1 < 0) || (LocalTime == 0) ||
        (Size2 < 0) || !RemoteTime.HasDate;
      if (!NoFileParams)
      {
        FileParams.SourceSize = Size2;
        FileParams.DestSize = Size1;

        if (OperationProgress->Side == osLocal)
        {
          FileParams.SourceTimestamp = ConvertLocalTimestamp(LocalTime);
          RemoteFileTimeToDateTimeAndPrecision(RemoteTime, FileParams.DestTimestamp, FileParams.DestPrecision);
        }
        else
        {
          FileParams.DestTimestamp = ConvertLocalTimestamp(LocalTime);
          RemoteFileTimeToDateTimeAndPrecision(RemoteTime, FileParams.SourceTimestamp, FileParams.SourcePrecision);
        }
      }

      UnicodeString SourceFullFileName = Path2;
      if (OperationProgress->Side == osLocal)
      {
        SourceFullFileName = ::IncludeTrailingBackslash(SourceFullFileName);
      }
      else
      {
        SourceFullFileName = core::UnixIncludeTrailingBackslash(SourceFullFileName);
      }
      SourceFullFileName += FileName2;

      if (ConfirmOverwrite(SourceFullFileName, TargetFileName, UserData.Params, OperationProgress,
            UserData.AutoResume && UserData.CopyParam->AllowResume(FileParams.SourceSize),
            NoFileParams ? nullptr : &FileParams, UserData.CopyParam, OverwriteMode))
      {
        switch (OverwriteMode)
        {
          case omOverwrite:
            if ((OperationProgress->Side == osRemote) && !FTerminal->TerminalCreateLocalFile(DestFullName, OperationProgress,
              false, true,
              &LocalFileHandle))
            {
              RequestResult = TFileZillaIntf::FILEEXISTS_SKIP;
              break;
            }
            if (TargetFileName != FileName1)
            {
              wcsncpy_s(FileName1, FileName1Len, TargetFileName.c_str(), FileName1Len);
              FileName1[FileName1Len - 1] = L'\0';
              UserData.FileName = FileName1;
              RequestResult = TFileZillaIntf::FILEEXISTS_RENAME;
            }
            else
            {
              RequestResult = TFileZillaIntf::FILEEXISTS_OVERWRITE;
            }
            break;

          case omResume:
            if ((OperationProgress->Side == osRemote) && !FTerminal->TerminalCreateLocalFile(DestFullName, OperationProgress,
              true, true,
              &LocalFileHandle))
            {
  //            ThrowSkipFileNull();
              RequestResult = TFileZillaIntf::FILEEXISTS_SKIP;
            }
            else
              RequestResult = TFileZillaIntf::FILEEXISTS_RESUME;
            break;

          case omComplete:
            FTerminal->LogEvent("File transfer was completed before disconnect");
            RequestResult = TFileZillaIntf::FILEEXISTS_COMPLETE;
            break;

          default:
            DebugFail();
            RequestResult = TFileZillaIntf::FILEEXISTS_OVERWRITE;
            break;
        }
      }
      else
      {
        RequestResult = TFileZillaIntf::FILEEXISTS_SKIP;
      }
    }

    // remember the answer for the retries
    UserData.OverwriteResult = RequestResult;

    if (RequestResult == TFileZillaIntf::FILEEXISTS_SKIP)
    {
      // when user chooses not to overwrite, break loop waiting for response code
      // by setting dummy one, as FZAPI won't do anything then
      SetLastCode(DummyTimeoutCode);
    }

    return true;
  }
}

static UnicodeString FormatContactList(const UnicodeString & Entry1, const UnicodeString & Entry2)
{
  if (!Entry1.IsEmpty() && !Entry2.IsEmpty())
  {
    return FORMAT(L"%s, %s", Entry1.c_str(), Entry2.c_str());
  }
  else
  {
    return Entry1 + Entry2;
  }
}

UnicodeString FormatContact(const TFtpsCertificateData::TContact & Contact)
{
  UnicodeString Result =
    FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 1).c_str(),
       FormatContactList(FormatContactList(FormatContactList(
        Contact.Organization, Contact.Unit).c_str(), Contact.CommonName).c_str(), Contact.Mail).c_str());

  if ((wcslen(Contact.Country) > 0) ||
      (wcslen(Contact.StateProvince) > 0) ||
      (wcslen(Contact.Town) > 0))
  {
    Result +=
      FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 2).c_str(),
         FormatContactList(FormatContactList(
          Contact.Country, Contact.StateProvince).c_str(), Contact.Town).c_str());
  }

  if (wcslen(Contact.Other) > 0)
  {
    Result += FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 3).c_str(), Contact.Other);
  }

  return Result;
}

UnicodeString FormatValidityTime(const TFtpsCertificateData::TValidityTime & ValidityTime)
{
  /*
  return FormatDateTime(L"ddddd tt",
    EncodeDateVerbose(
      static_cast<uint16_t>(ValidityTime.Year), static_cast<uint16_t>(ValidityTime.Month),
      static_cast<uint16_t>(ValidityTime.Day)) +
    EncodeTimeVerbose(
      static_cast<uint16_t>(ValidityTime.Hour), static_cast<uint16_t>(ValidityTime.Min),
      static_cast<uint16_t>(ValidityTime.Sec), 0));
  */
  TODO("use Sysutils::FormatDateTime");
  uint16_t Y, M, D, H, Mm, S, MS;
  TDateTime DateTime =
    EncodeDateVerbose(
      static_cast<uint16_t>(ValidityTime.Year), static_cast<uint16_t>(ValidityTime.Month),
      static_cast<uint16_t>(ValidityTime.Day)) +
    EncodeTimeVerbose(
      static_cast<uint16_t>(ValidityTime.Hour), static_cast<uint16_t>(ValidityTime.Min),
      static_cast<uint16_t>(ValidityTime.Sec), 0);
  DateTime.DecodeDate(Y, M, D);
  DateTime.DecodeTime(H, Mm, S, MS);
  UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, Mm, S);
  return dt;
}

static bool VerifyNameMask(const UnicodeString & AName, const UnicodeString & AMask)
{
  bool Result = true;
  UnicodeString Name = AName;
  UnicodeString Mask = AMask;
  intptr_t Pos = 0;
  while (Result && (Pos = Mask.Pos(L"*")) > 0)
  {
    // Pos will typically be 1 here, so not actual comparison is done
    Result = ::SameText(Mask.SubString(1, Pos - 1), Name.SubString(1, Pos - 1));
    if (Result)
    {
      Mask.Delete(1, Pos); // including *
      Name.Delete(1, Pos - 1);
      // remove everything until the next dot
      Pos = Name.Pos(L".");
      if (Pos == 0)
      {
        Pos = Name.Length() + 1;
      }
      Name.Delete(1, Pos - 1);
    }
  }

  if (Result)
  {
    Result = ::SameText(Mask, Name);
  }

  return Result;
}

bool TFTPFileSystem::VerifyCertificateHostName(const TFtpsCertificateData & Data)
{
  UnicodeString HostName = FTerminal->GetSessionData()->GetHostNameExpanded();

  UnicodeString CommonName = Data.Subject.CommonName;
  bool NoMask = CommonName.IsEmpty();
  bool Result = !NoMask && VerifyNameMask(HostName, CommonName);
  if (Result)
  {
    FTerminal->LogEvent(FORMAT(L"Certificate common name \"%s\" matches hostname", CommonName.c_str()));
  }
  else
  {
    if (!NoMask && (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1))
    {
      FTerminal->LogEvent(FORMAT(L"Certificate common name \"%s\" does not match hostname", CommonName.c_str()));
    }
    UnicodeString SubjectAltName = Data.SubjectAltName;
    while (!Result && !SubjectAltName.IsEmpty())
    {
      UnicodeString Entry = CutToChar(SubjectAltName, L',', true);
      UnicodeString EntryName = CutToChar(Entry, L':', true);
      if (::SameText(EntryName, L"DNS"))
      {
        NoMask = false;
        Result = VerifyNameMask(HostName, Entry);
        if (Result)
        {
          FTerminal->LogEvent(FORMAT(L"Certificate subject alternative name \"%s\" matches hostname", Entry.c_str()));
        }
        else
        {
          if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
          {
            FTerminal->LogEvent(FORMAT(L"Certificate subject alternative name \"%s\" does not match hostname", Entry.c_str()));
          }
        }
      }
    }
  }
  if (!Result && NoMask)
  {
    FTerminal->LogEvent("Certificate has no common name nor subject alternative name, not verifying hostname");
    Result = true;
  }
  return Result;
}

static bool IsIPAddress(const UnicodeString & HostName)
{
  bool IPv4 = true;
  bool IPv6 = true;
  bool AnyColon = false;

  for (intptr_t Index = 1; Index <= HostName.Length(); Index++)
  {
    wchar_t C = HostName[Index];
    if (!IsDigit(C) && (C != L'.'))
    {
      IPv4 = false;
    }
    if (!IsHex(C) && (C != L':'))
    {
      IPv6 = false;
    }
    if (C == L':')
    {
      AnyColon = true;
    }
  }

  return IPv4 || (IPv6 && AnyColon);
}

bool TFTPFileSystem::HandleAsynchRequestVerifyCertificate(
  const TFtpsCertificateData & Data, int & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    FSessionInfo.CertificateFingerprint =
      BytesToHex(RawByteString(reinterpret_cast<const char *>(Data.Hash), Data.HashLen), false, L':');

    if (FTerminal->GetSessionData()->GetFingerprintScan())
    {
      RequestResult = 0;
    }
    else
    {
      UnicodeString CertificateSubject = Data.Subject.Organization;
      FTerminal->LogEvent(FORMAT(L"Verifying certificate for \"%s\" with fingerprint %s and %d failures", CertificateSubject.c_str(), FSessionInfo.CertificateFingerprint.c_str(), Data.VerificationResult));

      bool Trusted = false;
      bool TryWindowsSystemCertificateStore = false;
      UnicodeString VerificationResultStr;
      switch (Data.VerificationResult)
      {
        case X509_V_OK:
          Trusted = true;
          break;
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_GET_ISSUER_CERT);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE);
          break;
        case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY);
          break;
        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_SIGNATURE_FAILURE);
          break;
        case X509_V_ERR_CERT_NOT_YET_VALID:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_NOT_YET_VALID);
          break;
        case X509_V_ERR_CERT_HAS_EXPIRED:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_HAS_EXPIRED);
          break;
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
          VerificationResultStr = LoadStr(CERT_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD);
          break;
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
          VerificationResultStr = LoadStr(CERT_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD);
          break;
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
          VerificationResultStr = LoadStr(CERT_ERR_DEPTH_ZERO_SELF_SIGNED_CERT);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
          VerificationResultStr = LoadStr(CERT_ERR_SELF_SIGNED_CERT_IN_CHAIN);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_INVALID_CA:
          VerificationResultStr = LoadStr(CERT_ERR_INVALID_CA);
          break;
        case X509_V_ERR_PATH_LENGTH_EXCEEDED:
          VerificationResultStr = LoadStr(CERT_ERR_PATH_LENGTH_EXCEEDED);
          break;
        case X509_V_ERR_INVALID_PURPOSE:
          VerificationResultStr = LoadStr(CERT_ERR_INVALID_PURPOSE);
          break;
        case X509_V_ERR_CERT_UNTRUSTED:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_UNTRUSTED);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_CERT_REJECTED:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_REJECTED);
          break;
        case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
          VerificationResultStr = LoadStr(CERT_ERR_KEYUSAGE_NO_CERTSIGN);
          break;
        case X509_V_ERR_CERT_CHAIN_TOO_LONG:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_CHAIN_TOO_LONG);
          break;
        default:
          VerificationResultStr =
            FORMAT(L"%s (%s)",
              LoadStr(CERT_ERR_UNKNOWN).c_str(), UnicodeString(X509_verify_cert_error_string(Data.VerificationResult)).c_str());
          break;
      }

      bool IsHostNameIPAddress = IsIPAddress(FTerminal->GetSessionData()->GetHostNameExpanded());
      bool CertificateHostNameVerified = !IsHostNameIPAddress && VerifyCertificateHostName(Data);

      bool VerificationResult = Trusted;

      if (IsHostNameIPAddress || !CertificateHostNameVerified)
      {
        VerificationResult = false;
        TryWindowsSystemCertificateStore = false;
      }

      UnicodeString SiteKey = FTerminal->GetSessionData()->GetSiteKey();

      if (!VerificationResult)
      {
        if (FTerminal->VerifyCertificate(FtpsCertificateStorageKey, SiteKey,
              FSessionInfo.CertificateFingerprint, CertificateSubject, Data.VerificationResult))
        {
          // certificate is trusted, but for not purposes of info dialog
          VerificationResult = true;
        }
      }

      // TryWindowsSystemCertificateStore is set for the same set of failures
      // as trigger NE_SSL_UNTRUSTED flag in ne_openssl.c's verify_callback().
      // Use WindowsValidateCertificate only as a last resort (after checking the cached fiungerprint)
      // as it can take a very long time (up to 1 minute).
      if (!VerificationResult && TryWindowsSystemCertificateStore)
      {
        if (WindowsValidateCertificate(Data.Certificate, Data.CertificateLen))
        {
          FTerminal->LogEvent("Certificate verified against Windows certificate store");
          VerificationResult = true;
          // certificate is trusted for all purposes
          Trusted = true;
        }
      }

      const UnicodeString SummarySeparator = L"\n\n";
      UnicodeString Summary;
      // even if the fingerprint is cached, the certificate is still not trusted for a purposes of the info dialog.
      if (!Trusted)
      {
        AddToList(Summary, VerificationResultStr + L" " + FMTLOAD(CERT_ERRDEPTH, (Data.VerificationDepth + 1)), SummarySeparator);
      }

      if (IsHostNameIPAddress)
      {
        AddToList(Summary, FMTLOAD(CERT_IP_CANNOT_VERIFY, FTerminal->GetSessionData()->GetHostNameExpanded().c_str()), SummarySeparator);
      }
      else if (!CertificateHostNameVerified)
      {
        AddToList(Summary, FMTLOAD(CERT_NAME_MISMATCH, FTerminal->GetSessionData()->GetHostNameExpanded().c_str()), SummarySeparator);
      }

      if (Summary.IsEmpty())
      {
        Summary = LoadStr(CERT_OK);
      }

      FSessionInfo.Certificate =
        FMTLOAD(CERT_TEXT,
          FormatContact(Data.Issuer).c_str(),
          FormatContact(Data.Subject).c_str(),
          FormatValidityTime(Data.ValidFrom).c_str(),
          FormatValidityTime(Data.ValidUntil).c_str(),
          FSessionInfo.CertificateFingerprint.c_str(),
          Summary.c_str());

      RequestResult = VerificationResult ? 1 : 0;

      if (RequestResult == 0)
      {
        TClipboardHandler ClipboardHandler;
        ClipboardHandler.Text = FSessionInfo.CertificateFingerprint;

        TQueryButtonAlias Aliases[1];
        Aliases[0].Button = qaRetry;
        Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
        Aliases[0].OnClick = MAKE_CALLBACK(TClipboardHandler::Copy, &ClipboardHandler);

        TQueryParams Params(qpWaitInBatch);
        Params.HelpKeyword = HELP_VERIFY_CERTIFICATE;
        Params.NoBatchAnswers = qaYes | qaRetry;
        Params.Aliases = Aliases;
        Params.AliasesCount = _countof(Aliases);
        uintptr_t Answer = FTerminal->QueryUser(
          FMTLOAD(VERIFY_CERT_PROMPT3, FSessionInfo.Certificate.c_str()),
          nullptr, qaYes | qaNo | qaCancel | qaRetry, &Params, qtWarning);

        switch (Answer)
        {
          case qaYes:
            // 2 = always, as used by FZ's VerifyCertDlg.cpp,
            // however FZAPI takes all non-zero values equally
            RequestResult = 2;
            break;

          case qaNo:
            RequestResult = 1;
            break;

          case qaCancel:
            // FTerminal->Configuration->Usage->Inc(L"HostNotVerified");
            RequestResult = 0;
            break;

          default:
            DebugFail();
            RequestResult = 0;
            break;
        }

        if (RequestResult == 2)
        {
          FTerminal->CacheCertificate(
            FtpsCertificateStorageKey, SiteKey,
            FSessionInfo.CertificateFingerprint, Data.VerificationResult);
        }
      }

      // Cache only if the certificate was accepted manually
      if (!VerificationResult && (RequestResult != 0))
      {
        FTerminal->GetConfiguration()->RememberLastFingerprint(
          FTerminal->GetSessionData()->GetSiteKey(), TlsFingerprintType, FSessionInfo.CertificateFingerprint);
      }
    }

    return true;
  }
}

bool TFTPFileSystem::HandleAsynchRequestNeedPass(
  struct TNeedPassRequestData & Data, int & RequestResult) const
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    UnicodeString Password;
    if (FCertificate != nullptr)
    {
      FTerminal->LogEvent(L"Server asked for password, but we are using certificate, and no password was specified upfront, using fake password");
      Password = L"USINGCERT";
      RequestResult = TFileZillaIntf::REPLY_OK;
    }
    else
    {
      if (!FPasswordFailed && FTerminal->GetSessionData()->GetLoginType() == ltAnonymous)
      {
        RequestResult = TFileZillaIntf::REPLY_OK;
      }
      else if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkPassword, LoadStr(PASSWORD_TITLE), L"",
        LoadStr(PASSWORD_PROMPT), false, 0, Password))
      {
        RequestResult = TFileZillaIntf::REPLY_OK;
      }
      else
      {
        RequestResult = TFileZillaIntf::REPLY_ABORTED;
      }
    }

    Data.Password = nullptr;
    // When returning REPLY_OK, we need to return an allocated password,
    // even if we were returning and empty string we got on input.
    if (RequestResult == TFileZillaIntf::REPLY_OK)
    {
      Data.Password = _wcsdup(Password.c_str());
    }

    return true;
  }
}

void TFTPFileSystem::RemoteFileTimeToDateTimeAndPrecision(const TRemoteFileTime & Source, TDateTime & DateTime, TModificationFmt & ModificationFmt)
{
  // ModificationFmt must be set after Modification
  if (Source.HasDate)
  {
    DateTime =
      EncodeDateVerbose(Source.Year, Source.Month, Source.Day);
    if (Source.HasTime)
    {
      DateTime = DateTime +
        EncodeTimeVerbose(Source.Hour, Source.Minute, Source.Second, 0);
      // not exact as we got year as well, but it is most probably
      // guessed by FZAPI anyway
      ModificationFmt = Source.HasSeconds ? mfFull : mfMDHM;

      // With IIS, the Utc should be false only for MDTM
      if (FWindowsServer && !Source.Utc)
      {
        DateTime -= DSTDifferenceForTime(DateTime);
      }
    }
    else
    {
      ModificationFmt = mfMDY;
    }

    if (Source.Utc)
    {
      DateTime = ::ConvertTimestampFromUTC(DateTime);
    }

  }
  else
  {
    // With SCP we estimate date to be today, if we have at least time

    DateTime = double(0.0);
    ModificationFmt = mfNone;
  }
}

bool TFTPFileSystem::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, uintptr_t Count)
{
  if (!FActive)
  {
    return false;
  }
  else if (FIgnoreFileList)
  {
    // directory listing provided implicitly by FZAPI during certain operations is ignored
    DebugAssert(FFileList == nullptr);
    return false;
  }
  else
  {
    DebugAssert(FFileList != nullptr);
    // This can actually fail in real life,
    // when connected to server with case insensitive paths
    // Is empty when called from DoReadFile
    UnicodeString AbsPath = GetAbsolutePath(FFileList->GetDirectory(), false);
    DebugAssert(FFileList->GetDirectory().IsEmpty() || core::UnixSamePath(AbsPath, Path));
    DebugUsedParam(Path);

    for (uintptr_t Index = 0; Index < Count; ++Index)
    {
      const TListDataEntry * Entry = &Entries[Index];
      std::unique_ptr<TRemoteFile> File(new TRemoteFile());
      try
      {
        File->SetTerminal(FTerminal);

        File->SetFileName(Entry->Name);
        try
        {
          intptr_t PermissionsLen = wcslen(Entry->Permissions);
          if (PermissionsLen >= 10)
          {
            File->GetRights()->SetText(Entry->Permissions + 1);
          }
          else if ((PermissionsLen == 3) || (PermissionsLen == 4))
          {
            File->GetRights()->SetOctal(Entry->Permissions);
          }
        }
        catch (...)
        {
          // ignore permissions errors with FTP
        }

        File->SetHumanRights(Entry->HumanPerm);

        const wchar_t * Space = wcschr(Entry->OwnerGroup, L' ');
        if (Space != nullptr)
        {
          File->GetFileOwner().SetName(UnicodeString(Entry->OwnerGroup, Space - Entry->OwnerGroup));
          File->GetFileGroup().SetName(Space + 1);
        }
        else
        {
          File->GetFileOwner().SetName(Entry->OwnerGroup);
        }

        File->SetSize(Entry->Size);

        if (Entry->Link)
        {
          File->SetType(FILETYPE_SYMLINK);
        }
        else if (Entry->Dir)
        {
          File->SetType(FILETYPE_DIRECTORY);
        }
        else
        {
          File->SetType(FILETYPE_DEFAULT);
        }

        TDateTime Modification;
        TModificationFmt ModificationFmt;
        RemoteFileTimeToDateTimeAndPrecision(Entry->Time, Modification, ModificationFmt);
        File->SetModification(Modification);
        File->SetModificationFmt(ModificationFmt);
        File->SetLastAccess(File->GetModification());

        File->SetLinkTo(Entry->LinkTarget);

        File->Complete();
      }
      catch (Exception & E)
      {
//        delete File;
        UnicodeString EntryData =
          FORMAT(L"%s/%s/%s/%s/%s/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d",
             Entry->Name, Entry->Permissions, Entry->HumanPerm, Entry->OwnerGroup, ::Int64ToStr(Entry->Size).c_str(),
             int(Entry->Dir), int(Entry->Link), Entry->Time.Year, Entry->Time.Month, Entry->Time.Day,
             Entry->Time.Hour, Entry->Time.Minute, int(Entry->Time.HasTime),
             int(Entry->Time.HasSeconds), int(Entry->Time.HasDate));
        throw ETerminal(&E, FMTLOAD(LIST_LINE_ERROR, EntryData.c_str()), HELP_LIST_LINE_ERROR);
      }

      FFileList->AddFile(File.release());
    }
    return true;
  }
}

bool TFTPFileSystem::HandleTransferStatus(bool Valid, int64_t TransferSize,
  int64_t Bytes, bool FileTransfer)
{
  if (!FActive)
  {
    return false;
  }
  else if (!Valid)
  {
  }
  else if (FileTransfer)
  {
    FileTransferProgress(TransferSize, Bytes);
  }
  else
  {
    ReadDirectoryProgress(Bytes);
  }
  return true;
}

bool TFTPFileSystem::HandleReply(intptr_t Command, uintptr_t Reply)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
    {
      FTerminal->LogEvent(FORMAT(L"Got reply %x to the command %d", static_cast<int>(Reply), Command));
    }

    // reply with Command 0 is not associated with current operation
    // so do not treat is as a reply
    // (it is typically used asynchronously to notify about disconnects)
    if (Command != 0)
    {
      DebugAssert(FCommandReply == 0);
      FCommandReply = Reply;
    }
    else
    {
      DebugAssert(FReply == 0);
      FReply = Reply;
    }
    return true;
  }
}

bool TFTPFileSystem::HandleCapabilities(
  TFTPServerCapabilities * ServerCapabilities)
{
  FServerCapabilities->Assign(ServerCapabilities);
  FFileSystemInfoValid = false;
  return true;
}

bool TFTPFileSystem::CheckError(intptr_t ReturnCode, const wchar_t * Context)
{
  // we do not expect any FZAPI call to fail as it generally can fail only due to:
  // - invalid parameters
  // - busy FZAPI core
  // the only exception is REPLY_NOTCONNECTED that can happen if
  // connection is closed just between the last call to Idle()
  // and call to any FZAPI command
  // in such case reply without associated command is posted,
  // which we are going to wait for unless we are already waiting
  // on higher level (this typically happens if connection is lost while
  // waiting for user interaction and is detected within call to
  // SetAsyncRequestResult)
  if (FLAGSET(ReturnCode, TFileZillaIntf::REPLY_NOTCONNECTED))
  {
    if (!FWaitingForReply)
    {
      // throws
      WaitForFatalNonCommandReply();
    }
  }
  else
  {
    FTerminal->FatalError(nullptr,
      FMTLOAD(INTERNAL_ERROR, FORMAT(L"fz#%s", Context).c_str(), ::IntToHex(ReturnCode, 4).c_str()));
    DebugFail();
  }

  return false;
}

bool TFTPFileSystem::Unquote(UnicodeString & Str)
{
  enum
  {
    STATE_INIT,
    STATE_QUOTE,
    STATE_QUOTED,
    STATE_DONE
  } State;

  State = STATE_INIT;
  DebugAssert((Str.Length() > 0) && ((Str[1] == L'"') || (Str[1] == L'\'')));

  intptr_t Index = 1;
  wchar_t Quote = 0;
  while (Index <= Str.Length())
  {
    switch (State)
    {
      case STATE_INIT:
        if ((Str[Index] == L'"') || (Str[Index] == L'\''))
        {
          Quote = Str[Index];
          State = STATE_QUOTED;
          Str.Delete(Index, 1);
        }
        else
        {
          DebugFail();
          // no quoted string
          Str.SetLength(0);
        }
        break;

      case STATE_QUOTED:
        if (Str[Index] == Quote)
        {
          State = STATE_QUOTE;
          Str.Delete(Index, 1);
        }
        else
        {
          ++Index;
        }
        break;

      case STATE_QUOTE:
        if (Str[Index] == Quote)
        {
          ++Index;
        }
        else
        {
          // end of quoted string, trim the rest
          Str.SetLength(Index - 1);
          State = STATE_DONE;
        }
        break;
    }
  }

  return (State == STATE_DONE);
}

void TFTPFileSystem::PreserveDownloadFileTime(HANDLE AHandle, void * UserData)
{
  TFileTransferData * Data = NB_STATIC_DOWNCAST(TFileTransferData, UserData);
  FILETIME WrTime = ::DateTimeToFileTime(Data->Modification, dstmUnix);
  SetFileTime(AHandle, nullptr, nullptr, &WrTime);
}

bool TFTPFileSystem::GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time)
{
  bool Result;
  try
  {
    // error-handling-free and DST-mode-unaware copy of TTerminal::OpenLocalFile
    HANDLE LocalFileHandle = ::CreateFile(ApiPath(FileName).c_str(), GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0);
    if (LocalFileHandle == INVALID_HANDLE_VALUE)
    {
      Result = false;
    }
    else
    {
      FILETIME MTime;
      if (!GetFileTime(LocalFileHandle, nullptr, nullptr, &MTime))
      {
        Result = false;
      }
      else
      {
        TDateTime Modification = ::ConvertTimestampToUTC(::FileTimeToDateTime(MTime));

        uint16_t Year;
        uint16_t Month;
        uint16_t Day;
        Modification.DecodeDate(Year, Month, Day);
        Time.tm_year = Year - 1900;
        Time.tm_mon = Month - 1;
        Time.tm_mday = Day;

        uint16_t Hour;
        uint16_t Min;
        uint16_t Sec;
        uint16_t MSec;
        Modification.DecodeTime(Hour, Min, Sec, MSec);
        Time.tm_hour = Hour;
        Time.tm_min = Min;
        Time.tm_sec = Sec;

        Result = true;
      }

      ::CloseHandle(LocalFileHandle);
    }
  }
  catch (...)
  {
    Result = false;
  }
  return Result;
}

void TFTPFileSystem::RegisterChecksumAlgCommand(const UnicodeString & Alg, const UnicodeString & Command)
{
  FChecksumAlgs->Add(Alg);
  FChecksumCommands->Add(Command);
}

void TFTPFileSystem::GetSupportedChecksumAlgs(TStrings * Algs)
{
  for (intptr_t Index = 0; Index < FHashAlgs->GetCount(); Index++)
  {
    Algs->Add(FHashAlgs->GetString(Index));
  }

  for (intptr_t Index = 0; Index < FChecksumAlgs->GetCount(); Index++)
  {
    UnicodeString Alg = FChecksumAlgs->GetString(Index);
    UnicodeString Command = FChecksumCommands->GetString(Index);

    if (SupportsCommand(Command) && (Algs->IndexOf(Alg) < 0))
    {
      Algs->Add(Alg);
    }
  }
}

bool TFTPFileSystem::SupportsSiteCommand(const UnicodeString & Command) const
{
  return (FSupportedSiteCommands->IndexOf(Command) >= 0);
}

bool TFTPFileSystem::SupportsCommand(const UnicodeString & Command) const
{
  return (FSupportedCommands->IndexOf(Command) >= 0);
}

void TFTPFileSystem::LockFile(const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TFTPFileSystem::UnlockFile(const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TFTPFileSystem::UpdateFromMain(TCustomFileSystem * /*MainFileSystem*/)
{
  // noop
}

UnicodeString GetOpenSSLVersionText()
{
  return OPENSSL_VERSION_TEXT;
}

#endif // NO_FILEZILLA
