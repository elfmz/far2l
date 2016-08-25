#pragma once

#include <Common.h>
#include <FileBuffer.h>

#include "Option.h"
#include "NamedObjs.h"
#include "HierarchicalStorage.h"
#include "Configuration.h"

#define SET_SESSION_PROPERTY(Property) \
  if (F##Property != Value) { F##Property = Value; Modify(); }

enum TCipher
{
  cipWarn,
  cip3DES,
  cipBlowfish,
  cipAES,
  cipDES,
  cipArcfour,
  cipChaCha20,
};
#define CIPHER_COUNT (cipChaCha20 + 1)

// explicit values to skip obsoleted fsExternalSSH, fsExternalSFTP
enum TFSProtocol_219
{
  fsFTPS_219 = 6,
  fsHTTP_219 = 7,
  fsHTTPS_219 = 8,
};

enum TFSProtocol
{
  fsSCPonly = 0,
  fsSFTP = 1,
  fsSFTPonly = 2,
  fsFTP = 5,
  fsWebDAV = 6,
};
#define FSPROTOCOL_COUNT (fsWebDAV+1)

enum TLoginType
{
  ltAnonymous = 0,
  ltNormal = 1,
};

extern const wchar_t * ProxyMethodNames;

enum TProxyMethod
{
  pmNone, pmSocks4, pmSocks5, pmHTTP, pmTelnet, pmCmd, pmSystem,
};

enum TSshProt
{
  ssh1only, ssh1deprecated, ssh2deprecated, ssh2only,
};

enum TKex
{
  kexWarn, kexDHGroup1, kexDHGroup14, kexDHGEx, kexRSA, kexECDH,
};
#define KEX_COUNT (kexECDH + 1)

enum TSshBug
{
  sbIgnore1, sbPlainPW1, sbRSA1, sbHMAC2, sbDeriveKey2, sbRSAPad2,
  sbPKSessID2, sbRekey2, sbMaxPkt2, sbIgnore2, sbOldGex2, sbWinAdj,
};
#define BUG_COUNT (sbWinAdj + 1)

enum TSftpBug
{
  sbSymlink,
  sbSignedTS,
};
#define SFTP_BUG_COUNT (sbSignedTS + 1)

extern const wchar_t * PingTypeNames;

enum TPingType
{
  ptOff, ptNullPacket, ptDummyCommand,
};

enum TAddressFamily
{
  afAuto, afIPv4, afIPv6,
};

enum TFtps
{
  ftpsNone, ftpsImplicit, ftpsExplicitSsl, ftpsExplicitTls,
};

// has to match SSL_VERSION_XXX constants in AsyncSslSocketLayer.h
enum TTlsVersion { ssl2 = 2, ssl3 = 3, tls10 = 10, tls11 = 11, tls12 = 12 };
enum TSessionSource { ssNone, ssStored, ssStoredModified };
enum TSessionUrlFlags
{
  sufSpecific = 0x01,
  sufUserName = 0x02,
  sufPassword = 0x04,
  sufHostKey = 0x08,
  sufComplete = sufUserName | sufPassword | sufHostKey,
  sufOpen = sufUserName | sufPassword,
};

extern const UnicodeString CipherNames[CIPHER_COUNT];
extern const UnicodeString KexNames[KEX_COUNT];
extern const wchar_t SshProtList[][10];
extern const TCipher DefaultCipherList[CIPHER_COUNT];
extern const TKex DefaultKexList[KEX_COUNT];
extern const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][16];
extern const intptr_t DefaultSendBuf;
#define ANONYMOUS_USER_NAME L"anonymous"
#define ANONYMOUS_PASSWORD L""
extern const intptr_t SshPortNumber;
extern const intptr_t FtpPortNumber;
extern const intptr_t FtpsImplicitPortNumber;
extern const intptr_t HTTPPortNumber;
extern const intptr_t HTTPSPortNumber;
extern const intptr_t TelnetPortNumber;
extern const intptr_t ProxyPortNumber;
extern const UnicodeString PuttySshProtocol;
extern const UnicodeString PuttyTelnetProtocol;
extern const UnicodeString SftpProtocol;
extern const UnicodeString ScpProtocol;
extern const UnicodeString FtpProtocol;
extern const UnicodeString FtpsProtocol;
extern const UnicodeString FtpesProtocol;
#define WebDAVProtocol HttpProtocol
#define WebDAVSProtocol HttpsProtocol
extern const UnicodeString SshProtocol;
//#define ProtocolSeparator L"://"
//#define WinSCPProtocolPrefix L"winscp-"
extern const wchar_t UrlParamSeparator;
extern const wchar_t UrlParamValueSeparator;
//#define UrlHostKeyParamName L"fingerprint"
//#define UrlSaveParamName L"save"
//#define PassphraseOption L"passphrase"

extern const intptr_t SFTPMinVersion;
extern const intptr_t SFTPMaxVersion;

struct TIEProxyConfig : public TObject
{
  TIEProxyConfig() :
    AutoDetect(false),
    ProxyPort(0),
    ProxyMethod(pmNone)
  {}
  bool AutoDetect; // not used
  UnicodeString AutoConfigUrl; // not used
  UnicodeString Proxy; //< string in format "http=host:80;https=host:443;ftp=ftpproxy:20;socks=socksproxy:1080"
  UnicodeString ProxyBypass; //< string in format "*.local, foo.com, google.com"
  UnicodeString ProxyHost;
  intptr_t ProxyPort;
  TProxyMethod ProxyMethod;
};

class TStoredSessionList;

class TSessionData : public TNamedObject
{
friend class TStoredSessionList;
NB_DISABLE_COPY(TSessionData)
NB_DECLARE_CLASS(TSessionData)
public:
private:
  UnicodeString FHostName;
  intptr_t FPortNumber;
  UnicodeString FUserName;
  RawByteString FPassword;
  intptr_t FPingInterval;
  TPingType FPingType;
  bool FTryAgent;
  bool FAgentFwd;
  UnicodeString FListingCommand;
  bool FAuthTIS;
  bool FAuthKI;
  bool FAuthKIPassword;
  bool FAuthGSSAPI;
  bool FGSSAPIFwdTGT; // not supported anymore
  UnicodeString FGSSAPIServerRealm; // not supported anymore
  bool FChangeUsername;
  bool FCompression;
  TSshProt FSshProt;
  bool FSsh2DES;
  bool FSshNoUserAuth;
  TCipher FCiphers[CIPHER_COUNT];
  TKex FKex[KEX_COUNT];
  bool FClearAliases;
  TEOLType FEOLType;
  bool FTrimVMSVersions;
  UnicodeString FPublicKeyFile;
  UnicodeString FPassphrase;
  UnicodeString FPuttyProtocol;
  TFSProtocol FFSProtocol;
  bool FModified;
  UnicodeString FLocalDirectory;
  UnicodeString FRemoteDirectory;
  bool FLockInHome;
  bool FSpecial;
  bool FSynchronizeBrowsing;
  bool FUpdateDirectories;
  bool FCacheDirectories;
  bool FCacheDirectoryChanges;
  bool FPreserveDirectoryChanges;
  bool FSelected;
  TAutoSwitch FLookupUserGroups;
  UnicodeString FReturnVar;
  bool FScp1Compatibility;
  UnicodeString FShell;
  UnicodeString FSftpServer;
  intptr_t FTimeout;
  bool FUnsetNationalVars;
  bool FIgnoreLsWarnings;
  bool FTcpNoDelay;
  intptr_t FSendBuf;
  bool FSshSimple;
  TProxyMethod FProxyMethod;
  UnicodeString FProxyHost;
  intptr_t FProxyPort;
  UnicodeString FProxyUsername;
  RawByteString FProxyPassword;
  UnicodeString FProxyTelnetCommand;
  UnicodeString FProxyLocalCommand;
  TAutoSwitch FProxyDNS;
  bool FProxyLocalhost;
  intptr_t FFtpProxyLogonType;
  TAutoSwitch FBugs[BUG_COUNT];
  UnicodeString FCustomParam1;
  UnicodeString FCustomParam2;
  bool FResolveSymlinks;
  bool FFollowDirectorySymlinks;
  TDateTime FTimeDifference;
  bool FTimeDifferenceAuto;
  intptr_t FSFTPDownloadQueue;
  intptr_t FSFTPUploadQueue;
  intptr_t FSFTPListingQueue;
  intptr_t FSFTPMaxVersion;
  intptr_t FSFTPMinPacketSize;
  intptr_t FSFTPMaxPacketSize;
  TDSTMode FDSTMode;
  TAutoSwitch FSFTPBugs[SFTP_BUG_COUNT];
  bool FDeleteToRecycleBin;
  bool FOverwrittenToRecycleBin;
  UnicodeString FRecycleBinPath;
  UnicodeString FPostLoginCommands;
  TAutoSwitch FSCPLsFullTime;
  TAutoSwitch FFtpListAll;
  TAutoSwitch FFtpHost;
  bool FFtpDupFF;
  bool FFtpUndupFF;
  bool FSslSessionReuse;
  UnicodeString FTlsCertificateFile;
  TAddressFamily FAddressFamily;
  UnicodeString FRekeyData;
  uintptr_t FRekeyTime;
  intptr_t FColor;
  bool FTunnel;
  UnicodeString FTunnelHostName;
  intptr_t FTunnelPortNumber;
  UnicodeString FTunnelUserName;
  RawByteString FTunnelPassword;
  UnicodeString FTunnelPublicKeyFile;
  intptr_t FTunnelLocalPortNumber;
  UnicodeString FTunnelPortFwd;
  UnicodeString FTunnelHostKey;
  bool FFtpPasvMode;
  TAutoSwitch FFtpForcePasvIp;
  TAutoSwitch FFtpUseMlsd;
  UnicodeString FFtpAccount;
  intptr_t FFtpPingInterval;
  TPingType FFtpPingType;
  TAutoSwitch FFtpTransferActiveImmediately;
  TFtps FFtps;
  TTlsVersion FMinTlsVersion;
  TTlsVersion FMaxTlsVersion;
  TAutoSwitch FNotUtf;
  bool FIsWorkspace;
  UnicodeString FLink;
  UnicodeString FHostKey;
  bool FFingerprintScan;
  bool FOverrideCachedHostKey;
  UnicodeString FNote;

  UnicodeString FOrigHostName;
  intptr_t FOrigPortNumber;
  TProxyMethod FOrigProxyMethod;
  bool FTunnelConfigured;
  TSessionSource FSource;
  bool FSaveOnly;
  UnicodeString FCodePage;
  mutable uintptr_t FCodePageAsNumber;
  bool FFtpAllowEmptyPassword;
  TLoginType FLoginType;
  intptr_t FNumberOfRetries;
  uintptr_t FSessionVersion;

  mutable TIEProxyConfig * FIEProxyConfig;

public:
  void SetHostName(const UnicodeString & AValue);
  UnicodeString GetHostNameExpanded() const;
  void SetPortNumber(intptr_t Value);
  void SetUserName(const UnicodeString & Value);
  UnicodeString GetUserNameExpanded() const;
  void SetPassword(const UnicodeString & Value);
  UnicodeString GetPassword() const;
  void SetPingInterval(intptr_t Value);
  void SetTryAgent(bool Value);
  void SetAgentFwd(bool Value);
  void SetAuthTIS(bool Value);
  void SetAuthKI(bool Value);
  void SetAuthKIPassword(bool Value);
  void SetAuthGSSAPI(bool Value);
  void SetGSSAPIFwdTGT(bool Value);
  void SetGSSAPIServerRealm(const UnicodeString & Value);
  void SetChangeUsername(bool Value);
  void SetCompression(bool Value);
  void SetSshProt(TSshProt Value);
  void SetSsh2DES(bool Value);
  void SetSshNoUserAuth(bool Value);
  void SetCipher(intptr_t Index, TCipher Value);
  TCipher GetCipher(intptr_t Index) const;
  void SetKex(intptr_t Index, TKex Value);
  TKex GetKex(intptr_t Index) const;
  void SetPublicKeyFile(const UnicodeString & Value);
  UnicodeString GetPassphrase() const;
  void SetPassphrase(const UnicodeString & Value);

  void SetPuttyProtocol(const UnicodeString & Value);
  bool GetCanLogin() const;
  void SetPingIntervalDT(const TDateTime & Value);
  TDateTime GetPingIntervalDT() const;
  TDateTime GetFtpPingIntervalDT() const;
  void SetTimeDifference(const TDateTime & Value);
  void SetTimeDifferenceAuto(bool Value);
  void SetPingType(TPingType Value);
  UnicodeString GetSessionName() const;
  bool HasSessionName() const;
  UnicodeString GetDefaultSessionName() const;
  UnicodeString GetProtocolUrl() const;
  void SetFSProtocol(TFSProtocol Value);
  UnicodeString GetFSProtocolStr() const;
  void SetLocalDirectory(const UnicodeString & Value);
  void SetRemoteDirectory(const UnicodeString & Value);
  void SetSynchronizeBrowsing(bool Value);
  void SetUpdateDirectories(bool Value);
  void SetCacheDirectories(bool Value);
  void SetCacheDirectoryChanges(bool Value);
  void SetPreserveDirectoryChanges(bool Value);
  void SetLockInHome(bool Value);
  void SetSpecial(bool Value);
  UnicodeString GetInfoTip() const;
  bool GetDefaultShell() const;
  void SetDetectReturnVar(bool Value);
  bool GetDetectReturnVar() const;
  void SetListingCommand(const UnicodeString & Value);
  void SetClearAliases(bool Value);
  void SetDefaultShell(bool Value);
  void SetEOLType(TEOLType Value);
  void SetTrimVMSVersions(bool Value);
  void SetLookupUserGroups(TAutoSwitch Value);
  void SetReturnVar(const UnicodeString & Value);
  void SetScp1Compatibility(bool Value);
  void SetShell(const UnicodeString & Value);
  void SetSftpServer(const UnicodeString & Value);
  void SetTimeout(intptr_t Value);
  void SetUnsetNationalVars(bool Value);
  void SetIgnoreLsWarnings(bool Value);
  void SetTcpNoDelay(bool Value);
  void SetSendBuf(intptr_t Value);
  void SetSshSimple(bool Value);
  UnicodeString GetSshProtStr() const;
  bool GetUsesSsh() const;
  void SetCipherList(const UnicodeString & Value);
  UnicodeString GetCipherList() const;
  void SetKexList(const UnicodeString & Value);
  UnicodeString GetKexList() const;
  void SetProxyMethod(TProxyMethod Value);
  void SetProxyHost(const UnicodeString & Value);
  void SetProxyPort(intptr_t Value);
  void SetProxyUsername(const UnicodeString & Value);
  void SetProxyPassword(const UnicodeString & Value);
  void SetProxyTelnetCommand(const UnicodeString & Value);
  void SetProxyLocalCommand(const UnicodeString & Value);
  void SetProxyDNS(TAutoSwitch Value);
  void SetProxyLocalhost(bool Value);
  UnicodeString GetProxyPassword() const;
  void SetFtpProxyLogonType(intptr_t Value);
  void SetBug(TSshBug Bug, TAutoSwitch Value);
  TAutoSwitch GetBug(TSshBug Bug) const;
  UnicodeString GetSessionKey() const;
  void SetCustomParam1(const UnicodeString & Value);
  void SetCustomParam2(const UnicodeString & Value);
  void SetResolveSymlinks(bool Value);
  void SetFollowDirectorySymlinks(bool Value);
  void SetSFTPDownloadQueue(intptr_t Value);
  void SetSFTPUploadQueue(intptr_t Value);
  void SetSFTPListingQueue(intptr_t Value);
  void SetSFTPMaxVersion(intptr_t Value);
  void SetSFTPMaxPacketSize(intptr_t Value);
  void SetSFTPMinPacketSize(intptr_t Value);
  void SetSFTPBug(TSftpBug Bug, TAutoSwitch Value);
  TAutoSwitch GetSFTPBug(TSftpBug Bug) const;
  void SetSCPLsFullTime(TAutoSwitch Value);
  void SetFtpListAll(TAutoSwitch Value);
  void SetFtpHost(TAutoSwitch Value);
  void SetFtpDupFF(bool Value);
  void SetFtpUndupFF(bool Value);
  void SetSslSessionReuse(bool Value);
  void SetTlsCertificateFile(const UnicodeString & Value);
  UnicodeString GetStorageKey() const;
  UnicodeString GetInternalStorageKey() const;
  UnicodeString GetSiteKey() const;
  void SetDSTMode(TDSTMode Value);
  void SetDeleteToRecycleBin(bool Value);
  void SetOverwrittenToRecycleBin(bool Value);
  void SetRecycleBinPath(const UnicodeString & Value);
  void SetPostLoginCommands(const UnicodeString & Value);
  void SetAddressFamily(TAddressFamily Value);
  void SetRekeyData(const UnicodeString & Value);
  void SetRekeyTime(uintptr_t Value);
  void SetColor(intptr_t Value);
  void SetTunnel(bool Value);
  void SetTunnelHostName(const UnicodeString & Value);
  void SetTunnelPortNumber(intptr_t Value);
  void SetTunnelUserName(const UnicodeString & Value);
  void SetTunnelPassword(const UnicodeString & Value);
  UnicodeString GetTunnelPassword() const;
  void SetTunnelPublicKeyFile(const UnicodeString & Value);
  void SetTunnelPortFwd(const UnicodeString & Value);
  void SetTunnelLocalPortNumber(intptr_t Value);
  bool GetTunnelAutoassignLocalPortNumber();
  void SetTunnelHostKey(const UnicodeString & Value);
  void SetFtpPasvMode(bool Value);
  void SetFtpForcePasvIp(TAutoSwitch Value);
  void SetFtpUseMlsd(TAutoSwitch Value);
  void SetFtpAccount(const UnicodeString & Value);
  void SetFtpPingInterval(intptr_t Value);
  void SetFtpPingType(TPingType Value);
  void SetFtpTransferActiveImmediately(TAutoSwitch Value);
  void SetFtps(TFtps Value);
  void SetMinTlsVersion(TTlsVersion Value);
  void SetMaxTlsVersion(TTlsVersion Value);
  void SetNotUtf(TAutoSwitch Value);
  void SetIsWorkspace(bool Value);
  void SetLink(const UnicodeString & Value);
  void SetHostKey(const UnicodeString & Value);
  void SetFingerprintScan(bool Value) { FFingerprintScan = Value; }
  void SetNote(const UnicodeString & Value);
  TDateTime GetTimeoutDT();
  void SavePasswords(THierarchicalStorage * Storage, bool PuttyExport, bool DoNotEncryptPasswords);
  UnicodeString GetLocalName() const;
  UnicodeString GetFolderName() const;
  void Modify();
  UnicodeString GetSource() const;
  bool GetSaveOnly() const { return FSaveOnly; }
  void DoLoad(THierarchicalStorage * Storage, bool & RewritePassword);
  void DoSave(THierarchicalStorage * Storage,
    bool PuttyExport, const TSessionData * Default, bool DoNotEncryptPasswords);
  /*UnicodeString ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, const UnicodeString & Default);
  int ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, int Default);
  bool IsSame(const TSessionData * Default, bool AdvancedOnly, TStrings * DifferentProperties);*/
  UnicodeString GetNameWithoutHiddenPrefix() const;
  bool HasStateData() const;
  void CopyStateData(TSessionData * SourceData);
  void CopyNonCoreData(TSessionData * SourceData);
  UnicodeString GetNormalizedPuttyProtocol() const;
  static RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key);
  static UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  static RawByteString StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  static bool DoIsProtocolUrl(const UnicodeString & Url, const UnicodeString & Protocol, intptr_t & ProtocolLen);
  static bool IsProtocolUrl(const UnicodeString & Url, const UnicodeString & Protocol, intptr_t & ProtocolLen);
  static void AddSwitchValue(UnicodeString & Result, const UnicodeString & Name, const UnicodeString & Value);
  static void AddSwitch(UnicodeString & Result, const UnicodeString & Switch);
  static void AddSwitch(UnicodeString & Result, const UnicodeString & AName, const UnicodeString & Value);
  static void AddSwitch(UnicodeString & Result, const UnicodeString & AName, intptr_t Value);
  /*static UnicodeString AssemblyString(TAssemblyLanguage Language, const UnicodeString & S);
  static void AddAssemblyPropertyRaw(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & AName, const UnicodeString & Value);
  static void AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & AName, const UnicodeString & Value);
  static void AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & AName, const UnicodeString & Type,
    const UnicodeString & Member);
  static void AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & AName, intptr_t Value);
  void AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & AName, bool Value);*/
  TStrings * SaveToOptions(const TSessionData * Default);
  template<class AlgoT>
  void SetAlgoList(AlgoT * List, const AlgoT * DefaultList, const UnicodeString * Names,
    intptr_t Count, AlgoT WarnAlgo, const UnicodeString & AValue);

  //__property UnicodeString InternalStorageKey = { read = GetInternalStorageKey };

public:
  explicit TSessionData(const UnicodeString & AName);
  virtual ~TSessionData();
  TSessionData * Clone();
  void Default();
  void NonPersistant();
  void Load(THierarchicalStorage * Storage);
  void ApplyRawSettings(THierarchicalStorage * Storage);
  // void ImportFromFilezilla(_di_IXMLNode Node, const UnicodeString & APath);
  void Save(THierarchicalStorage * Storage, bool PuttyExport,
    const TSessionData * Default = nullptr);
  void SaveRecryptedPasswords(THierarchicalStorage * Storage);
  void RecryptPasswords();
  bool HasPassword() const;
  bool HasAnySessionPassword() const;
  bool HasAnyPassword() const;
  void ClearSessionPasswords();
  void Remove();
  void CacheHostKeyIfNotCached();
  virtual void Assign(const TPersistent * Source);
  virtual intptr_t Compare(const TNamedObject * Other) const;
  void CopyData(TSessionData * Source);
  void CopyDirectoriesStateData(TSessionData * SourceData);
  bool ParseUrl(const UnicodeString & AUrl, TOptions * Options,
    TStoredSessionList * AStoredSessions, bool & DefaultsOnly,
    UnicodeString * AFileName, bool * AProtocolDefined, UnicodeString * MaskedUrl);
  bool ParseOptions(TOptions * Options);
  void ConfigureTunnel(intptr_t PortNumber);
  void RollbackTunnel();
  void ExpandEnvironmentVariables();
  bool IsSame(const TSessionData * Default, bool AdvancedOnly) const;
  bool IsSame(const TSessionData * Default, bool AdvancedOnly, TStrings * DifferentProperties) const;
  bool IsSameSite(const TSessionData * Default) const;
  bool IsInFolderOrWorkspace(const UnicodeString & Name) const;
  UnicodeString GenerateSessionUrl(uintptr_t Flags);
  UnicodeString GenerateOpenCommandArgs();
//  UnicodeString GenerateAssemblyCode(TAssemblyLanguage Language);
  void LookupLastFingerprint();
  bool GetIsSecure() const;
  static void ValidatePath(const UnicodeString & APath);
  static void ValidateName(const UnicodeString & AName);
  static UnicodeString MakeValidName(const UnicodeString & Name);
  static UnicodeString ExtractLocalName(const UnicodeString & Name);
  static UnicodeString ExtractFolderName(const UnicodeString & Name);
  static UnicodeString ComposePath(const UnicodeString & APath, const UnicodeString & Name);
  static bool IsSensitiveOption(const UnicodeString & Option);
  static UnicodeString FormatSiteKey(const UnicodeString & HostName, intptr_t PortNumber);

  /*__property UnicodeString HostName  = { read=FHostName, write=SetHostName };
  __property UnicodeString HostNameExpanded  = { read=GetHostNameExpanded };
  __property int PortNumber  = { read=FPortNumber, write=SetPortNumber };
  __property UnicodeString UserName  = { read=FUserName, write=SetUserName };
  __property UnicodeString UserNameExpanded  = { read=GetUserNameExpanded };
  __property UnicodeString Password  = { read=GetPassword, write=SetPassword };
  __property int PingInterval  = { read=FPingInterval, write=SetPingInterval };
  __property bool TryAgent  = { read=FTryAgent, write=SetTryAgent };
  __property bool AgentFwd  = { read=FAgentFwd, write=SetAgentFwd };
  __property UnicodeString ListingCommand = { read = FListingCommand, write = SetListingCommand };
  __property bool AuthTIS  = { read=FAuthTIS, write=SetAuthTIS };
  __property bool AuthKI  = { read=FAuthKI, write=SetAuthKI };
  __property bool AuthKIPassword  = { read=FAuthKIPassword, write=SetAuthKIPassword };
  __property bool AuthGSSAPI  = { read=FAuthGSSAPI, write=SetAuthGSSAPI };
  __property bool GSSAPIFwdTGT = { read=FGSSAPIFwdTGT, write=SetGSSAPIFwdTGT };
  __property UnicodeString GSSAPIServerRealm = { read=FGSSAPIServerRealm, write=SetGSSAPIServerRealm };
  __property bool ChangeUsername  = { read=FChangeUsername, write=SetChangeUsername };
  __property bool Compression  = { read=FCompression, write=SetCompression };
  __property TSshProt SshProt  = { read=FSshProt, write=SetSshProt };
  __property bool UsesSsh = { read = GetUsesSsh };
  __property bool Ssh2DES  = { read=FSsh2DES, write=SetSsh2DES };
  __property bool SshNoUserAuth  = { read=FSshNoUserAuth, write=SetSshNoUserAuth };
  __property TCipher Cipher[int Index] = { read=GetCipher, write=SetCipher };
  __property TKex Kex[int Index] = { read=GetKex, write=SetKex };
  __property UnicodeString PublicKeyFile  = { read=FPublicKeyFile, write=SetPublicKeyFile };
  __property UnicodeString Passphrase  = { read=GetPassphrase, write=SetPassphrase };
  __property UnicodeString PuttyProtocol  = { read=FPuttyProtocol, write=SetPuttyProtocol };
  __property TFSProtocol FSProtocol  = { read=FFSProtocol, write=SetFSProtocol  };
  __property UnicodeString FSProtocolStr  = { read=GetFSProtocolStr };
  __property bool Modified  = { read=FModified, write=FModified };
  __property bool CanLogin  = { read=GetCanLogin };
  __property bool ClearAliases = { read = FClearAliases, write = SetClearAliases };
  __property TDateTime PingIntervalDT = { read = GetPingIntervalDT, write = SetPingIntervalDT };
  __property TDateTime TimeDifference = { read = FTimeDifference, write = SetTimeDifference };
  __property bool TimeDifferenceAuto = { read = FTimeDifferenceAuto, write = SetTimeDifferenceAuto };
  __property TPingType PingType = { read = FPingType, write = SetPingType };
  __property UnicodeString SessionName  = { read=GetSessionName };
  __property UnicodeString DefaultSessionName  = { read=GetDefaultSessionName };
  __property UnicodeString LocalDirectory  = { read=FLocalDirectory, write=SetLocalDirectory };
  __property UnicodeString RemoteDirectory  = { read=FRemoteDirectory, write=SetRemoteDirectory };
  __property bool SynchronizeBrowsing = { read=FSynchronizeBrowsing, write=SetSynchronizeBrowsing };
  __property bool UpdateDirectories = { read=FUpdateDirectories, write=SetUpdateDirectories };
  __property bool CacheDirectories = { read=FCacheDirectories, write=SetCacheDirectories };
  __property bool CacheDirectoryChanges = { read=FCacheDirectoryChanges, write=SetCacheDirectoryChanges };
  __property bool PreserveDirectoryChanges = { read=FPreserveDirectoryChanges, write=SetPreserveDirectoryChanges };
  __property bool LockInHome = { read=FLockInHome, write=SetLockInHome };
  __property bool Special = { read=FSpecial, write=SetSpecial };
  __property bool Selected  = { read=FSelected, write=FSelected };
  __property UnicodeString InfoTip  = { read=GetInfoTip };
  __property bool DefaultShell = { read = GetDefaultShell, write = SetDefaultShell };
  __property bool DetectReturnVar = { read = GetDetectReturnVar, write = SetDetectReturnVar };
  __property TEOLType EOLType = { read = FEOLType, write = SetEOLType };
  __property bool TrimVMSVersions = { read = FTrimVMSVersions, write = SetTrimVMSVersions };
  __property TAutoSwitch LookupUserGroups = { read = FLookupUserGroups, write = SetLookupUserGroups };
  __property UnicodeString ReturnVar = { read = FReturnVar, write = SetReturnVar };
  __property bool Scp1Compatibility = { read = FScp1Compatibility, write = SetScp1Compatibility };
  __property UnicodeString Shell = { read = FShell, write = SetShell };
  __property UnicodeString SftpServer = { read = FSftpServer, write = SetSftpServer };
  __property int Timeout = { read = FTimeout, write = SetTimeout };
  __property TDateTime TimeoutDT = { read = GetTimeoutDT };
  __property bool UnsetNationalVars = { read = FUnsetNationalVars, write = SetUnsetNationalVars };
  __property bool IgnoreLsWarnings  = { read=FIgnoreLsWarnings, write=SetIgnoreLsWarnings };
  __property bool TcpNoDelay  = { read=FTcpNoDelay, write=SetTcpNoDelay };
  __property int SendBuf  = { read=FSendBuf, write=SetSendBuf };
  __property bool SshSimple  = { read=FSshSimple, write=SetSshSimple };
  __property UnicodeString SshProtStr  = { read=GetSshProtStr };
  __property UnicodeString CipherList  = { read=GetCipherList, write=SetCipherList };
  __property UnicodeString KexList  = { read=GetKexList, write=SetKexList };
  __property TProxyMethod ProxyMethod  = { read=FProxyMethod, write=SetProxyMethod };
  __property UnicodeString ProxyHost  = { read=FProxyHost, write=SetProxyHost };
  __property int ProxyPort  = { read=FProxyPort, write=SetProxyPort };
  __property UnicodeString ProxyUsername  = { read=FProxyUsername, write=SetProxyUsername };
  __property UnicodeString ProxyPassword  = { read=GetProxyPassword, write=SetProxyPassword };
  __property UnicodeString ProxyTelnetCommand  = { read=FProxyTelnetCommand, write=SetProxyTelnetCommand };
  __property UnicodeString ProxyLocalCommand  = { read=FProxyLocalCommand, write=SetProxyLocalCommand };
  __property TAutoSwitch ProxyDNS  = { read=FProxyDNS, write=SetProxyDNS };
  __property bool ProxyLocalhost  = { read=FProxyLocalhost, write=SetProxyLocalhost };
  __property int FtpProxyLogonType  = { read=FFtpProxyLogonType, write=SetFtpProxyLogonType };
  __property TAutoSwitch Bug[TSshBug Bug]  = { read=GetBug, write=SetBug };
  __property UnicodeString CustomParam1 = { read = FCustomParam1, write = SetCustomParam1 };
  __property UnicodeString CustomParam2 = { read = FCustomParam2, write = SetCustomParam2 };
  __property UnicodeString SessionKey = { read = GetSessionKey };
  __property bool ResolveSymlinks = { read = FResolveSymlinks, write = SetResolveSymlinks };
  __property bool FollowDirectorySymlinks = { read = FFollowDirectorySymlinks, write = SetFollowDirectorySymlinks };
  __property int SFTPDownloadQueue = { read = FSFTPDownloadQueue, write = SetSFTPDownloadQueue };
  __property int SFTPUploadQueue = { read = FSFTPUploadQueue, write = SetSFTPUploadQueue };
  __property int SFTPListingQueue = { read = FSFTPListingQueue, write = SetSFTPListingQueue };
  __property int SFTPMaxVersion = { read = FSFTPMaxVersion, write = SetSFTPMaxVersion };
  __property unsigned long SFTPMaxPacketSize = { read = FSFTPMaxPacketSize, write = SetSFTPMaxPacketSize };
  __property TAutoSwitch SFTPBug[TSftpBug Bug]  = { read=GetSFTPBug, write=SetSFTPBug };
  __property TAutoSwitch SCPLsFullTime = { read = FSCPLsFullTime, write = SetSCPLsFullTime };
  __property TAutoSwitch FtpListAll = { read = FFtpListAll, write = SetFtpListAll };
  __property TAutoSwitch FtpHost = { read = FFtpHost, write = SetFtpHost };
  __property bool SslSessionReuse = { read = FSslSessionReuse, write = SetSslSessionReuse };
  __property UnicodeString TlsCertificateFile = { read=FTlsCertificateFile, write=SetTlsCertificateFile };
  __property TDSTMode DSTMode = { read = FDSTMode, write = SetDSTMode };
  __property bool DeleteToRecycleBin = { read = FDeleteToRecycleBin, write = SetDeleteToRecycleBin };
  __property bool OverwrittenToRecycleBin = { read = FOverwrittenToRecycleBin, write = SetOverwrittenToRecycleBin };
  __property UnicodeString RecycleBinPath = { read = FRecycleBinPath, write = SetRecycleBinPath };
  __property UnicodeString PostLoginCommands = { read = FPostLoginCommands, write = SetPostLoginCommands };
  __property TAddressFamily AddressFamily = { read = FAddressFamily, write = SetAddressFamily };
  __property UnicodeString RekeyData = { read = FRekeyData, write = SetRekeyData };
  __property unsigned int RekeyTime = { read = FRekeyTime, write = SetRekeyTime };
  __property int Color = { read = FColor, write = SetColor };
  __property bool Tunnel = { read = FTunnel, write = SetTunnel };
  __property UnicodeString TunnelHostName = { read = FTunnelHostName, write = SetTunnelHostName };
  __property int TunnelPortNumber = { read = FTunnelPortNumber, write = SetTunnelPortNumber };
  __property UnicodeString TunnelUserName = { read = FTunnelUserName, write = SetTunnelUserName };
  __property UnicodeString TunnelPassword = { read = GetTunnelPassword, write = SetTunnelPassword };
  __property UnicodeString TunnelPublicKeyFile = { read = FTunnelPublicKeyFile, write = SetTunnelPublicKeyFile };
  __property bool TunnelAutoassignLocalPortNumber = { read = GetTunnelAutoassignLocalPortNumber };
  __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber, write = SetTunnelLocalPortNumber };
  __property UnicodeString TunnelPortFwd = { read = FTunnelPortFwd, write = SetTunnelPortFwd };
  __property UnicodeString TunnelHostKey = { read = FTunnelHostKey, write = SetTunnelHostKey };
  __property bool FtpPasvMode = { read = FFtpPasvMode, write = SetFtpPasvMode };
  __property TAutoSwitch FtpForcePasvIp = { read = FFtpForcePasvIp, write = SetFtpForcePasvIp };
  __property TAutoSwitch FtpUseMlsd = { read = FFtpUseMlsd, write = SetFtpUseMlsd };
  __property UnicodeString FtpAccount = { read = FFtpAccount, write = SetFtpAccount };
  __property int FtpPingInterval  = { read=FFtpPingInterval, write=SetFtpPingInterval };
  __property TDateTime FtpPingIntervalDT  = { read=GetFtpPingIntervalDT };
  __property TPingType FtpPingType = { read = FFtpPingType, write = SetFtpPingType };
  __property TAutoSwitch FtpTransferActiveImmediately = { read = FFtpTransferActiveImmediately, write = SetFtpTransferActiveImmediately };
  __property TFtps Ftps = { read = FFtps, write = SetFtps };
  __property TTlsVersion MinTlsVersion = { read = FMinTlsVersion, write = SetMinTlsVersion };
  __property TTlsVersion MaxTlsVersion = { read = FMaxTlsVersion, write = SetMaxTlsVersion };
  __property TAutoSwitch NotUtf = { read = FNotUtf, write = SetNotUtf };
  __property bool IsWorkspace = { read = FIsWorkspace, write = SetIsWorkspace };
  __property UnicodeString Link = { read = FLink, write = SetLink };
  __property UnicodeString HostKey = { read = FHostKey, write = SetHostKey };
  __property bool FingerprintScan = { read = FFingerprintScan, write = FFingerprintScan };
  __property bool OverrideCachedHostKey = { read = FOverrideCachedHostKey };
  __property UnicodeString Note = { read = FNote, write = SetNote };
  __property UnicodeString StorageKey = { read = GetStorageKey };
  __property UnicodeString SiteKey = { read = GetSiteKey };
  __property UnicodeString OrigHostName = { read = FOrigHostName };
  __property int OrigPortNumber = { read = FOrigPortNumber };
  __property UnicodeString LocalName = { read = GetLocalName };
  __property UnicodeString FolderName = { read = GetFolderName };
  __property UnicodeString Source = { read = GetSource };
  __property bool SaveOnly = { read = FSaveOnly };*/

  bool GetTimeDifferenceAuto() const { return FTimeDifferenceAuto; }
  UnicodeString GetNote() const { return FNote; }
  UnicodeString GetProtocolStr() const;
  void SetProtocolStr(const UnicodeString & Value);

  UnicodeString GetHostName() const { return FHostName; }
  intptr_t GetPortNumber() const { return FPortNumber; }
  TLoginType GetLoginType() const;
  void SetLoginType(TLoginType Value);
  UnicodeString SessionGetUserName() const { return FUserName; }
  intptr_t GetPingInterval() const { return FPingInterval; }
  bool GetTryAgent() const { return FTryAgent; }
  bool GetAgentFwd() const { return FAgentFwd; }
  const UnicodeString GetListingCommand() const { return FListingCommand; }
  bool GetAuthTIS() const { return FAuthTIS; }
  bool GetAuthKI() const { return FAuthKI; }
  bool GetAuthKIPassword() const { return FAuthKIPassword; }
  bool GetAuthGSSAPI() const { return FAuthGSSAPI; }
  bool GetGSSAPIFwdTGT() const { return FGSSAPIFwdTGT; }
  const UnicodeString GetGSSAPIServerRealm() const { return FGSSAPIServerRealm; }
  bool GetChangeUsername() const { return FChangeUsername; }
  bool GetCompression() const { return FCompression; }
  TSshProt GetSshProt() const { return FSshProt; }
  bool GetSsh2DES() const { return FSsh2DES; }
  bool GetSshNoUserAuth() const { return FSshNoUserAuth; }
  const UnicodeString GetPublicKeyFile() const { return FPublicKeyFile; }
  UnicodeString GetPuttyProtocol() const { return FPuttyProtocol; }
  TFSProtocol GetFSProtocol() const { return FFSProtocol; }
  bool GetModified() const { return FModified; }
  void SetModified(bool Value) { FModified = Value; }
  bool GetClearAliases() const { return FClearAliases; }
  TDateTime GetTimeDifference() const { return FTimeDifference; }
  TPingType GetPingType() const { return FPingType; }
  UnicodeString GetLocalDirectory() const { return FLocalDirectory; }
  UnicodeString GetRemoteDirectory() const { return FRemoteDirectory; }
  bool GetSynchronizeBrowsing() const { return FSynchronizeBrowsing; }
  bool GetUpdateDirectories() const { return FUpdateDirectories; }
  bool GetCacheDirectories() const { return FCacheDirectories; }
  bool GetCacheDirectoryChanges() const { return FCacheDirectoryChanges; }
  bool GetPreserveDirectoryChanges() const { return FPreserveDirectoryChanges; }
  bool GetLockInHome() const { return FLockInHome; }
  bool GetSpecial() const { return FSpecial; }
  bool GetSelected() const { return FSelected; }
  void SetSelected(bool Value) { FSelected = Value; }
  TEOLType GetEOLType() const { return FEOLType; }
  bool GetTrimVMSVersions() const { return FTrimVMSVersions; }
  TAutoSwitch GetLookupUserGroups() const { return FLookupUserGroups; }
  UnicodeString GetReturnVar() const { return FReturnVar; }
  bool GetScp1Compatibility() const { return FScp1Compatibility; }
  UnicodeString GetShell() const { return FShell; }
  UnicodeString GetSftpServer() const { return FSftpServer; }
  intptr_t GetTimeout() const { return FTimeout; }
  bool GetUnsetNationalVars() const { return FUnsetNationalVars; }
  bool GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
  bool GetTcpNoDelay() const { return FTcpNoDelay; }
  intptr_t GetSendBuf() const { return FSendBuf; }
  bool GetSshSimple() const { return FSshSimple; }
  TProxyMethod GetProxyMethod() const { return FProxyMethod; }
  TProxyMethod GetActualProxyMethod() const
  {
    return GetProxyMethod() == pmSystem ? GetSystemProxyMethod() : GetProxyMethod();
  }
  UnicodeString GetProxyHost() const;
  intptr_t GetProxyPort() const;
  UnicodeString GetProxyUsername() const;
  UnicodeString GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
  UnicodeString GetProxyLocalCommand() const { return FProxyLocalCommand; }
  TAutoSwitch GetProxyDNS() const { return FProxyDNS; }
  bool GetProxyLocalhost() const { return FProxyLocalhost; }
  intptr_t GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
  UnicodeString GetCustomParam1() const { return FCustomParam1; }
  UnicodeString GetCustomParam2() const { return FCustomParam2; }
  bool GetResolveSymlinks() const { return FResolveSymlinks; }
  bool GetFollowDirectorySymlinks() const { return FFollowDirectorySymlinks; }
  intptr_t GetSFTPDownloadQueue() const { return FSFTPDownloadQueue; }
  intptr_t GetSFTPUploadQueue() const { return FSFTPUploadQueue; }
  intptr_t GetSFTPListingQueue() const { return FSFTPListingQueue; }
  intptr_t GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
  intptr_t GetSFTPMinPacketSize() const { return FSFTPMinPacketSize; }
  intptr_t GetSFTPMaxPacketSize() const { return FSFTPMaxPacketSize; }
  TAutoSwitch GetSCPLsFullTime() const { return FSCPLsFullTime; }
  TAutoSwitch GetFtpListAll() const { return FFtpListAll; }
  TAutoSwitch GetFtpHost() const { return FFtpHost; }
  bool GetFtpDupFF() const { return FFtpDupFF; }
  bool GetFtpUndupFF() const { return FFtpUndupFF; }
  bool GetSslSessionReuse() const { return FSslSessionReuse; }
  UnicodeString GetTlsCertificateFile() const { return FTlsCertificateFile; }
  TDSTMode GetDSTMode() const { return FDSTMode; }
  bool GetDeleteToRecycleBin() const { return FDeleteToRecycleBin; }
  bool GetOverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; }
  UnicodeString GetRecycleBinPath() const { return FRecycleBinPath; }
  UnicodeString GetPostLoginCommands() const { return FPostLoginCommands; }
  TAddressFamily GetAddressFamily() const { return FAddressFamily; }
  UnicodeString GetCodePage() const { return FCodePage; }
  void SetCodePage(const UnicodeString & Value);
  uintptr_t GetCodePageAsNumber() const;
  UnicodeString GetRekeyData() const { return FRekeyData; }
  uintptr_t GetRekeyTime() const { return FRekeyTime; }
  intptr_t GetColor() const { return FColor; }
  bool GetTunnel() const { return FTunnel; }
  UnicodeString GetTunnelHostName() const { return FTunnelHostName; }
  intptr_t GetTunnelPortNumber() const { return FTunnelPortNumber; }
  UnicodeString GetTunnelUserName() const { return FTunnelUserName; }
  UnicodeString GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
  intptr_t GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  UnicodeString GetTunnelPortFwd() const { return FTunnelPortFwd; }
  UnicodeString GetTunnelHostKey() const { return FTunnelHostKey; }
  bool GetFtpPasvMode() const { return FFtpPasvMode; }
  bool GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
  void SetFtpAllowEmptyPassword(bool Value);
  TAutoSwitch GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
  TAutoSwitch GetFtpUseMlsd() const { return FFtpUseMlsd; }
  UnicodeString GetFtpAccount() const { return FFtpAccount; }
  intptr_t GetFtpPingInterval() const { return FFtpPingInterval; }
  TPingType GetFtpPingType() const { return FFtpPingType; }
  TAutoSwitch GetFtpTransferActiveImmediately() const { return FFtpTransferActiveImmediately; }
  TFtps GetFtps() const { return FFtps; }
  TTlsVersion GetMinTlsVersion() const { return FMinTlsVersion; }
  TTlsVersion GetMaxTlsVersion() const { return FMaxTlsVersion; }
  TAutoSwitch GetNotUtf() const { return FNotUtf; }
  bool GetIsWorkspace() const { return FIsWorkspace; }
  UnicodeString GetLink() const { return FLink; }
  UnicodeString GetHostKey() const { return FHostKey; }
  bool GetFingerprintScan() const { return FFingerprintScan; }
  bool GetOverrideCachedHostKey() const { return FOverrideCachedHostKey; }
  UnicodeString GetOrigHostName() const { return FOrigHostName; }
  intptr_t GetOrigPortNumber() const { return FOrigPortNumber; }
  void SetPasswordless(bool Value);

  intptr_t GetNumberOfRetries() const { return FNumberOfRetries; }
  void SetNumberOfRetries(intptr_t Value) { FNumberOfRetries = Value; }
  uintptr_t GetSessionVersion() const { return FSessionVersion; }
  void SetSessionVersion(uintptr_t Value) { FSessionVersion = Value; }
  void RemoveProtocolPrefix(UnicodeString & HostName) const;

private:
  uintptr_t GetDefaultVersion() const { return ::GetCurrentVersionNumber(); }
  TFSProtocol TranslateFSProtocolNumber(intptr_t FSProtocol);
  TFSProtocol TranslateFSProtocol(const UnicodeString & ProtocolID) const;
  TFtps TranslateFtpEncryptionNumber(intptr_t FtpEncryption) const;

private:
  TProxyMethod GetSystemProxyMethod() const;
  void PrepareProxyData() const;
  void ParseIEProxyConfig() const;
  void FromURI(const UnicodeString & ProxyURI,
    UnicodeString & ProxyUrl, intptr_t & ProxyPort, TProxyMethod & ProxyMethod) const;
  void AdjustHostName(UnicodeString & HostName, const UnicodeString & Prefix) const;

};

class TStoredSessionList : public TNamedObjectList
{
NB_DISABLE_COPY(TStoredSessionList)
public:
  explicit TStoredSessionList(bool AReadOnly = false);
  void Load(const UnicodeString & AKey, bool UseDefaults);
  void Load();
  void Save(bool All, bool Explicit);
  void Saved();
  void ImportFromFilezilla(const UnicodeString & AFileName);
  void Export(const UnicodeString & AFileName);
  void Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false);
  void Save(THierarchicalStorage * Storage, bool All = false);
  void SelectAll(bool Select);
  void Import(TStoredSessionList * From, bool OnlySelected, TList * Imported);
  void RecryptPasswords(TStrings * RecryptPasswordErrors);
  TSessionData * AtSession(int Index)
    { return (TSessionData*)AtObject(Index); }
  void SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void Cleanup();
  void UpdateStaticUsage();
  intptr_t IndexOf(TSessionData * Data) const;
  const TSessionData * FindSame(TSessionData * Data) const;
  TSessionData * NewSession(const UnicodeString & SessionName, TSessionData * Session);
  void NewWorkspace(const UnicodeString & Name, TList * DataList);
  bool GetIsFolder(const UnicodeString & Name) const;
  bool GetIsWorkspace(const UnicodeString & Name) const;
  TSessionData * ParseUrl(const UnicodeString & Url, TOptions * Options, bool & DefaultsOnly,
    UnicodeString * AFileName = nullptr, bool * ProtocolDefined = nullptr, UnicodeString * MaskedUrl = nullptr);
  bool IsUrl(const UnicodeString & Url);
  bool CanLogin(TSessionData * Data);
  void GetFolderOrWorkspace(const UnicodeString & Name, TList * List);
  TStrings * GetFolderOrWorkspaceList(const UnicodeString & Name);
  TStrings * GetWorkspaces();
  bool HasAnyWorkspace();
  TSessionData * SaveWorkspaceData(TSessionData * Data);
  virtual ~TStoredSessionList();
/*
  __property TSessionData * Sessions[int Index]  = { read=AtSession };
  __property TSessionData * DefaultSettings  = { read=FDefaultSettings, write=SetDefaultSettings };
*/
  const TSessionData * GetSession(intptr_t Index) const { return NB_STATIC_DOWNCAST_CONST(TSessionData, AtObject(Index)); }
  TSessionData * GetSession(intptr_t Index) { return NB_STATIC_DOWNCAST(TSessionData, AtObject(Index)); }
  const TSessionData * GetDefaultSettings() const { return FDefaultSettings; }
  TSessionData * GetDefaultSettings() { return FDefaultSettings; }
  const TSessionData * GetSessionByName(const UnicodeString & SessionName) const;
  void SetDefaultSettings(const TSessionData * Value);

  static void ImportHostKeys(const UnicodeString & TargetKey,
    const UnicodeString & SourceKey, TStoredSessionList * Sessions,
    bool OnlySelected);

private:
  TSessionData * FDefaultSettings;
  bool FReadOnly;
  void DoSave(THierarchicalStorage * Storage, bool All,
    bool RecryptPasswordOnly, TStrings * RecryptPasswordErrors);
  void DoSave(bool All, bool Explicit, bool RecryptPasswordOnly,
    TStrings * RecryptPasswordErrors);
  void DoSave(THierarchicalStorage * Storage,
    TSessionData * Data, bool All, bool RecryptPasswordOnly,
    TSessionData * FactoryDefaults);
  TSessionData * ResolveWorkspaceData(TSessionData * Data);
  bool IsFolderOrWorkspace(const UnicodeString & Name, bool Workspace) const;
  TSessionData * CheckIsInFolderOrWorkspaceAndResolve(
    TSessionData * Data, const UnicodeString & Name);
//  void ImportLevelFromFilezilla(_di_IXMLNode Node, const UnicodeString & Path);
};

bool GetCodePageInfo(UINT CodePage, CPINFOEX & CodePageInfoEx);
uintptr_t GetCodePageAsNumber(const UnicodeString & CodePage);
UnicodeString GetCodePageAsString(uintptr_t CodePage);

UnicodeString GetExpandedLogFileName(const UnicodeString & LogFileName, TSessionData * SessionData);
bool GetIsSshProtocol(TFSProtocol FSProtocol);
intptr_t GetDefaultPort(TFSProtocol FSProtocol, TFtps Ftps);

