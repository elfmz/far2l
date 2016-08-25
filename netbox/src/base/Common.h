#pragma once

#include <Global.h>
#include <Exceptions.h>

extern const wchar_t EngShortMonthNames[12][4];
#define CONST_BOM "\xEF\xBB\xBF"
extern const wchar_t TokenPrefix;
extern const wchar_t NoReplacement;
extern const wchar_t TokenReplacement;
#define LOCAL_INVALID_CHARS "/\\:*?\"<>|"
#define PASSWORD_MASK "***"
#define sLineBreak L"\n"

// Order of the values also define order of the buttons/answers on the prompts
// MessageDlg relies on these to be <= 0x0000FFFF
const uint32_t qaYes      = 0x00000001;
// MessageDlg relies that answer do not conflict with mrCancel (=0x2)
const uint32_t qaNo       = 0x00000004;
const uint32_t qaOK       = 0x00000008;
const uint32_t qaCancel   = 0x00000010;
const uint32_t qaYesToAll = 0x00000020;
const uint32_t qaNoToAll  = 0x00000040;
const uint32_t qaAbort    = 0x00000080;
const uint32_t qaRetry    = 0x00000100;
const uint32_t qaIgnore   = 0x00000200;
const uint32_t qaSkip     = 0x00000400;
const uint32_t qaAll      = 0x00000800;
const uint32_t qaHelp     = 0x00001000;
const uint32_t qaReport   = 0x00002000;

const uint32_t qaFirst = qaYes;
const uint32_t qaLast  = qaReport;

const uint32_t qaNeverAskAgain = 0x00010000;

const int qpFatalAbort           = 0x01;
const int qpNeverAskAgainCheck   = 0x02;
const int qpAllowContinueOnError = 0x04;
const int qpIgnoreAbort          = 0x08;
const int qpWaitInBatch          = 0x10;

inline void ThrowExtException() { throw ExtException(static_cast<Exception *>(nullptr), UnicodeString(L"")); }

extern const UnicodeString HttpProtocol;
extern const UnicodeString HttpsProtocol;
extern const UnicodeString ProtocolSeparator;

UnicodeString ReplaceChar(const UnicodeString & Str, wchar_t A, wchar_t B);
UnicodeString DeleteChar(const UnicodeString & Str, wchar_t C);
void PackStr(UnicodeString & Str);
void PackStr(RawByteString & Str);
void PackStr(AnsiString & Str);
void Shred(UnicodeString & Str);
void Shred(UTF8String & Str);
void Shred(AnsiString & Str);
UnicodeString AnsiToString(const RawByteString & S);
UnicodeString AnsiToString(const char * S, size_t Len);
UnicodeString MakeValidFileName(const UnicodeString & AFileName);
UnicodeString RootKeyToStr(HKEY RootKey);
UnicodeString BooleanToStr(bool B);
UnicodeString BooleanToEngStr(bool B);
UnicodeString DefaultStr(const UnicodeString & Str, const UnicodeString & Default);
UnicodeString CutToChar(UnicodeString & Str, wchar_t Ch, bool Trim);
UnicodeString CopyToChars(const UnicodeString & Str, intptr_t & From, const UnicodeString & Chs, bool Trim,
  wchar_t * Delimiter = nullptr, bool DoubleDelimiterEscapes = false);
UnicodeString CopyToChar(const UnicodeString & Str, wchar_t Ch, bool Trim);
UnicodeString DelimitStr(const UnicodeString & Str, const UnicodeString & Chars);
UnicodeString ShellDelimitStr(const UnicodeString & Str, wchar_t Quote);
UnicodeString ExceptionLogString(Exception * E);
UnicodeString MainInstructions(const UnicodeString & S);
bool HasParagraphs(const UnicodeString & S);
UnicodeString MainInstructionsFirstParagraph(const UnicodeString & S);
bool ExtractMainInstructions(UnicodeString & S, UnicodeString & MainInstructions);
UnicodeString RemoveMainInstructionsTag(const UnicodeString & S);
UnicodeString UnformatMessage(const UnicodeString & S);
UnicodeString RemoveInteractiveMsgTag(const UnicodeString & S);
UnicodeString RemoveEmptyLines(const UnicodeString & S);
bool IsNumber(const UnicodeString & Str);
UnicodeString GetSystemTemporaryDirectory();
UnicodeString GetShellFolderPath(int CSIdl);
UnicodeString StripPathQuotes(const UnicodeString & APath);
UnicodeString AddQuotes(const UnicodeString & Str);
UnicodeString AddPathQuotes(const UnicodeString & APath);
void SplitCommand(const UnicodeString & Command, UnicodeString & Program,
  UnicodeString & Params, UnicodeString & Dir);
UnicodeString ValidLocalFileName(const UnicodeString & AFileName);
UnicodeString ValidLocalFileName(
  const UnicodeString & AFileName, wchar_t AInvalidCharsReplacement,
  const UnicodeString & ATokenizibleChars, const UnicodeString & ALocalInvalidChars);
UnicodeString ExtractProgram(const UnicodeString & Command);
UnicodeString ExtractProgramName(const UnicodeString & Command);
UnicodeString FormatCommand(const UnicodeString & Program, const UnicodeString & AParams);
UnicodeString ExpandFileNameCommand(const UnicodeString & Command,
  const UnicodeString & AFileName);
void ReformatFileNameCommand(UnicodeString & Command);
UnicodeString EscapeParam(const UnicodeString & AParam);
UnicodeString EscapePuttyCommandParam(const UnicodeString & AParam);
UnicodeString ExpandEnvironmentVariables(const UnicodeString & Str);
bool ComparePaths(const UnicodeString & APath1, const UnicodeString & APath2);
bool CompareFileName(const UnicodeString & APath1, const UnicodeString & APath2);
intptr_t CompareLogicalText(const UnicodeString & S1, const UnicodeString & S2);
bool IsReservedName(const UnicodeString & AFileName);
UnicodeString ApiPath(const UnicodeString & APath);
UnicodeString DisplayableStr(const RawByteString & Str);
UnicodeString ByteToHex(uint8_t B, bool UpperCase = true);
UnicodeString BytesToHex(const uint8_t * B, uintptr_t Length, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString BytesToHex(const RawByteString & Str, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString CharToHex(wchar_t Ch, bool UpperCase = true);
RawByteString HexToBytes(const UnicodeString & Hex);
uint8_t HexToByte(const UnicodeString & Hex);
bool IsLowerCaseLetter(wchar_t Ch);
bool IsUpperCaseLetter(wchar_t Ch);
bool IsLetter(wchar_t Ch);
bool IsDigit(wchar_t Ch);
bool IsHex(wchar_t Ch);
UnicodeString DecodeUrlChars(const UnicodeString & S);
UnicodeString EncodeUrlString(const UnicodeString & S);
UnicodeString EncodeUrlPath(const UnicodeString & S);
UnicodeString AppendUrlParams(const UnicodeString & URL, const UnicodeString & Params);
UnicodeString ExtractFileNameFromUrl(const UnicodeString & Url);
bool RecursiveDeleteFile(const UnicodeString & AFileName, bool ToRecycleBin);
void RecursiveDeleteFileChecked(const UnicodeString & AFileName, bool ToRecycleBin);
void DeleteFileChecked(const UnicodeString & AFileName);
uintptr_t CancelAnswer(uintptr_t Answers);
uintptr_t AbortAnswer(uintptr_t Answers);
uintptr_t ContinueAnswer(uintptr_t Answers);
UnicodeString LoadStr(intptr_t Ident, uintptr_t MaxLength = 0);
UnicodeString LoadStrPart(intptr_t Ident, intptr_t Part);
UnicodeString EscapeHotkey(const UnicodeString & Caption);
bool CutToken(UnicodeString & AStr, UnicodeString & AToken,
  UnicodeString * ARawToken = nullptr, UnicodeString * ASeparator = nullptr);
void AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter);
bool IsWinVista();
bool IsWin7();
bool IsWin8();
bool IsWin10();
bool IsWine();
int64_t Round(double Number);
bool TryRelativeStrToDateTime(const UnicodeString & AStr, TDateTime & DateTime);
LCID GetDefaultLCID();
UnicodeString DefaultEncodingName();
UnicodeString WindowsProductName();
bool GetWindowsProductType(DWORD & Type);
bool IsDirectoryWriteable(const UnicodeString & APath);
UnicodeString FormatNumber(int64_t Size);
UnicodeString FormatSize(int64_t Size);
UnicodeString ExtractFileBaseName(const UnicodeString & APath);
TStringList * TextToStringList(const UnicodeString & Text);
UnicodeString StringsToText(TStrings * Strings);
TStrings * CloneStrings(TStrings * Strings);
UnicodeString TrimVersion(const UnicodeString & Version);
UnicodeString FormatVersion(int MajorVersion, int MinorVersion, int Patch);
TFormatSettings GetEngFormatSettings();
int ParseShortEngMonthName(const UnicodeString & MonthStr);
// The defaults are equal to defaults of TStringList class (except for Sorted)
TStringList * CreateSortedStringList(bool CaseSensitive = false, TDuplicatesEnum Duplicates = dupIgnore);
UnicodeString FindIdent(const UnicodeString & Ident, TStrings * Idents);
void CheckCertificate(const UnicodeString & Path);
typedef struct x509_st X509;
typedef struct evp_pkey_st EVP_PKEY;
void ParseCertificate(const UnicodeString & Path,
  const UnicodeString & Passphrase, X509 *& Certificate, EVP_PKEY *& PrivateKey,
  bool & WrongPassphrase);
bool IsHttpUrl(const UnicodeString & S);
bool IsHttpOrHttpsUrl(const UnicodeString & S);
UnicodeString ChangeUrlProtocol(const UnicodeString & S, const UnicodeString & Protocol);
void LoadScriptFromFile(const UnicodeString & FileName, TStrings * Lines);
UnicodeString StripEllipsis(const UnicodeString & S);

DEFINE_CALLBACK_TYPE3(TProcessLocalFileEvent, void,
  const UnicodeString & /*FileName*/, const TSearchRec & /*Rec*/, void * /*Param*/);
bool FileSearchRec(const UnicodeString & AFileName, TSearchRec & Rec);

struct TSearchRecChecked : public TSearchRec
{
  UnicodeString Path;
};

DWORD FindCheck(DWORD Result, const UnicodeString & APath);
DWORD FindFirstUnchecked(const UnicodeString & APath, DWORD LocalFileAttrs, TSearchRecChecked & F);
DWORD FindFirstChecked(const UnicodeString & APath, DWORD LocalFileAttrs, TSearchRecChecked & F);
DWORD FindNextChecked(TSearchRecChecked & F);
void ProcessLocalDirectory(const UnicodeString & ADirName,
  TProcessLocalFileEvent CallBackFunc, void * Param = nullptr, DWORD FindAttrs = INVALID_FILE_ATTRIBUTES);
DWORD FileGetAttrFix(const UnicodeString & AFileName);

extern const wchar_t * DSTModeNames;
enum TDSTMode
{
  dstmWin  = 0, //
  dstmUnix = 1, // adjust UTC time to Windows "bug"
  dstmKeep = 2,
};

bool UsesDaylightHack();
TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day);
TDateTime EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec);
double DSTDifferenceForTime(const TDateTime & DateTime);
TDateTime SystemTimeToDateTimeVerbose(const SYSTEMTIME & SystemTime);
TDateTime UnixToDateTime(int64_t TimeStamp, TDSTMode DSTMode);
TDateTime ConvertTimestampToUTC(const TDateTime & DateTime);
TDateTime ConvertTimestampFromUTC(const TDateTime & DateTime);
FILETIME DateTimeToFileTime(const TDateTime & DateTime, TDSTMode DSTMode);
TDateTime AdjustDateTimeFromUnix(const TDateTime & DateTime, TDSTMode DSTMode);
void UnifyDateTimePrecision(TDateTime & DateTime1, TDateTime & DateTime2);
TDateTime FileTimeToDateTime(const FILETIME & FileTime);
int64_t ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode);
int64_t ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode);
UnicodeString FixedLenDateTimeFormat(const UnicodeString & Format);
UnicodeString StandardTimestamp(const TDateTime & DateTime);
UnicodeString StandardTimestamp();
UnicodeString StandardDatestamp();
UnicodeString FormatTimeZone(intptr_t Sec);
UnicodeString GetTimeZoneLogString();
bool AdjustClockForDSTEnabled();
intptr_t CompareFileTime(const TDateTime & T1, const TDateTime & T2);
intptr_t TimeToMSec(const TDateTime & T);
intptr_t TimeToSeconds(const TDateTime & T);
intptr_t TimeToMinutes(const TDateTime & T);

#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated

template<class T>
class TValueRestorer : public TObject
{
public:
  inline explicit TValueRestorer(T & Target, const T & Value) :
    FTarget(Target),
    FValue(Value),
    FArmed(true)
  {
  }

  inline explicit TValueRestorer(T & Target) :
    FTarget(Target),
    FValue(Target),
    FArmed(true)
  {
  }

  void Release()
  {
    if (FArmed)
    {
      FTarget = FValue;
      FArmed = false;
    }
  }

  inline ~TValueRestorer()
  {
    Release();
  }

protected:
  T & FTarget;
  T FValue;
  bool FArmed;
};

class TAutoNestingCounter : TValueRestorer<int>
{
public:
  inline explicit TAutoNestingCounter(int & Target) :
    TValueRestorer<int>(Target)
  {
    DebugAssert(Target >= 0);
    ++Target;
  }

  inline ~TAutoNestingCounter()
  {
    DebugAssert(!FArmed || (FTarget == (FValue + 1)));
  }
};

class TAutoFlag : public TValueRestorer<bool>
{
public:
  TAutoFlag(bool & Target) :
    TValueRestorer<bool>(Target)
  {
    DebugAssert(!Target);
    Target = true;
  }

  ~TAutoFlag()
  {
    DebugAssert(!FArmed || FTarget);
  }
};
#pragma warning(pop)

#include <map>

template<class T1, class T2>
class BiDiMap
{
public:
  typedef std::map<T1, T2> TFirstToSecond;
  typedef typename TFirstToSecond::const_iterator const_iterator;

  void Add(const T1 & Value1, const T2 & Value2)
  {
    FFirstToSecond.insert(std::make_pair(Value1, Value2));
    FSecondToFirst.insert(std::make_pair(Value2, Value1));
  }

  T1 LookupFirst(const T2 & Value2) const
  {
    typename TSecondToFirst::const_iterator Iterator = FSecondToFirst.find(Value2);
    DebugAssert(Iterator != FSecondToFirst.end());
    return Iterator->second;
  }

  T2 LookupSecond(const T1 & Value1) const
  {
    const_iterator Iterator = FFirstToSecond.find(Value1);
    DebugAssert(Iterator != FFirstToSecond.end());
    return Iterator->second;
  }

  const_iterator begin()
  {
    return FFirstToSecond.begin();
  }

  const_iterator end()
  {
    return FFirstToSecond.end();
  }

private:
  TFirstToSecond FFirstToSecond;
  typedef std::map<T2, T1> TSecondToFirst;
  TSecondToFirst FSecondToFirst;
};

typedef std::vector<UnicodeString> TUnicodeStringVector;

UnicodeString FormatBytes(int64_t Bytes, bool UseOrders = true);

#ifdef __linux__
FILE *_wfopen(const wchar_t *filename, const wchar_t *mode);
#endif

namespace base {

UnicodeString UnixExtractFileExt(const UnicodeString & APath);
UnicodeString UnixExtractFileName(const UnicodeString & APath);
UnicodeString ExtractFileName(const UnicodeString & APath, bool Unix);
UnicodeString GetEnvironmentVariable(const UnicodeString & AEnvVarName);

} // namespace base
