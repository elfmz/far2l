#include <iostream>
#include <iomanip>

#include <Classes.hpp>
#include <Common.h>
#include <rtlconsts.h>
#include <Sysutils.hpp>

intptr_t __cdecl debug_printf(const wchar_t * format, ...)
{
  (void)format;
  intptr_t len = 0;
#if !defined(NDEBUG) && !defined(__linux__)
  va_list args;
  va_start(args, format);
  len = _vscwprintf(format, args);
  std::wstring buf(len + 1, 0);
  vswprintf(const_cast<wchar_t *>(buf.c_str()), buf.size(), format, args);
  va_end(args);
  OutputDebugStringW(buf.c_str());
#endif
  return len;
}

intptr_t __cdecl debug_printf2(const char * format, ...)
{
  (void)format;
  intptr_t len = 0;
#if !defined(NDEBUG) && !defined(__linux__)
  va_list args;
  va_start(args, format);
  len = _vscprintf(format, args);
  std::string buf(len + sizeof(char), 0);
  vsprintf_s(&buf[0], buf.size(), format, args);
  va_end(args);
  OutputDebugStringA(buf.c_str());
#endif
  return len;
}

UnicodeString MB2W(const char * src, const UINT cp)
{
  if (!src || !*src)
  {
    return UnicodeString(L"");
  }

  intptr_t reqLength = ::MultiByteToWideChar(cp, 0, src, -1, nullptr, 0);
  UnicodeString Result;
  if (reqLength)
  {
    Result.SetLength(reqLength);
    ::MultiByteToWideChar(cp, 0, src, -1, const_cast<LPWSTR>(Result.c_str()), static_cast<int>(reqLength));
    Result.SetLength(Result.Length() - 1);  //remove NULL character
  }
  return Result;
}

AnsiString W2MB(const wchar_t * src, const UINT cp)
{
  if (!src || !*src)
  {
    return AnsiString("");
  }

  intptr_t reqLength = ::WideCharToMultiByte(cp, 0, src, -1, 0, 0, nullptr, nullptr);
  AnsiString Result;
  if (reqLength)
  {
    Result.SetLength(reqLength);
    ::WideCharToMultiByte(cp, 0, src, -1, const_cast<LPSTR>(Result.c_str()),
      static_cast<int>(reqLength), nullptr, nullptr);
    Result.SetLength(Result.Length() - 1);  //remove NULL character
  }
  return Result;
}

int RandSeed = 0;
const TDayTable MonthDays[] =
{
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

Exception::Exception(Exception * E) :
  std::runtime_error(E ? E->what() : ""),
  Message(E ? E->Message : L"")
{
}

Exception::Exception(const UnicodeString & Msg) :
  std::runtime_error(""),
  Message(Msg)
{
}

Exception::Exception(const wchar_t * Msg) :
  std::runtime_error(""),
  Message(Msg)
{
}

Exception::Exception(std::exception * E) :
  std::runtime_error(E ? E->what() : "")
{
}

Exception::Exception(const UnicodeString & Msg, int AHelpContext) :
  std::runtime_error(""),
  Message(Msg)
{
  TODO("FHelpContext = AHelpContext");
  (void)AHelpContext;
}

Exception::Exception(Exception * E, int Ident) :
  std::runtime_error(E ? E->what() : "")
{
  Message = FMTLOAD(Ident);
}

Exception::Exception(int Ident) :
  std::runtime_error("")
{
  Message = FMTLOAD(Ident);
}

UnicodeString IntToStr(intptr_t Value)
{
  UnicodeString Result;
  Result.sprintf(L"%d", Value);
  return Result;
}

UnicodeString Int64ToStr(int64_t Value)
{
  UnicodeString Result;
  Result.sprintf(L"%lld", Value);
  return Result;
}

intptr_t StrToInt(const UnicodeString & Value)
{
  int64_t Result = 0;
  if (TryStrToInt(Value, Result))
  {
    return static_cast<intptr_t>(Result);
  }
  else
  {
    return 0;
  }
}

int64_t ToInt(const UnicodeString & Value)
{
  int64_t Result = 0;
  if (TryStrToInt(Value, Result))
  {
    return Result;
  }
  else
  {
    return 0;
  }
}

intptr_t StrToIntDef(const UnicodeString & Value, intptr_t DefVal)
{
  int64_t Result = DefVal;
  if (TryStrToInt(Value, Result))
  {
    return static_cast<intptr_t>(Result);
  }
  else
  {
    return DefVal;
  }
}

int64_t StrToInt64(const UnicodeString & Value)
{
  return ToInt(Value);
}

int64_t StrToInt64Def(const UnicodeString & Value, int64_t DefVal)
{
  int64_t Result = DefVal;
  if (TryStrToInt(Value, Result))
  {
    return Result;
  }
  else
  {
    return DefVal;
  }
}

bool TryStrToInt(const UnicodeString & StrValue, int64_t & Value)
{
  bool Result = !StrValue.IsEmpty() && (StrValue.FindFirstNotOf(L"+-0123456789") == -1);
  if (Result)
  {
    errno = 0;
    Value = _wtoi64(StrValue.c_str());
    Result = (errno != EINVAL) && (errno != ERANGE);
  }
  return Result;
}

UnicodeString Trim(const UnicodeString & Str)
{
  UnicodeString Result = TrimRight(TrimLeft(Str));
  return Result;
}

UnicodeString TrimLeft(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  intptr_t Len = Result.Length();
  intptr_t Pos = 1;
  while ((Pos <= Len) && (Result[Pos] == L' '))
    Pos++;
  if (Pos > 1)
    return Result.SubString(Pos, Len - Pos + 1);
  else
    return Result;
}

UnicodeString TrimRight(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  intptr_t Len = Result.Length();
  while (Len > 0 &&
    ((Result[Len] == L' ') || (Result[Len] == L'\n')))
  {
    Len--;
  }
  Result.SetLength(Len);
  return Result;
}

UnicodeString UpperCase(const UnicodeString & Str)
{
  UnicodeString Result(Str);
  ::CharUpperBuff(const_cast<LPWSTR>(Result.c_str()), (DWORD)Result.Length());
  return Result;
}

UnicodeString LowerCase(const UnicodeString & Str)
{
  UnicodeString Result(Str);
  ::CharLowerBuff(const_cast<LPWSTR>(Result.c_str()), (DWORD)Result.Length());
  return Result;
}

wchar_t UpCase(const wchar_t Ch)
{
  return static_cast<wchar_t>(::towupper(Ch));
}

wchar_t LowCase(const wchar_t Ch)
{
  return static_cast<wchar_t>(::towlower(Ch));
}

UnicodeString AnsiReplaceStr(const UnicodeString & Str, const UnicodeString & From,
  const UnicodeString & To)
{
  UnicodeString Result = Str;
  intptr_t Pos = 0;
  while ((Pos = Result.Pos(From)) > 0)
  {
    Result.Replace(Pos, From.Length(), To);
  }
  return Result;
}

intptr_t AnsiPos(const UnicodeString & Str, wchar_t Ch)
{
  intptr_t Result = Str.Pos(Ch);
  return Result;
}

intptr_t Pos(const UnicodeString & Str, const UnicodeString & Substr)
{
  intptr_t Result = Str.Pos(Substr.c_str());
  return Result;
}

UnicodeString StringReplaceAll(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To)
{
  return AnsiReplaceStr(Str, From, To);
}

bool IsDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str, intptr_t AIndex)
{
  if (AIndex <= Str.Length())
  {
    wchar_t Ch = Str[AIndex];
    for (intptr_t Index = 1; Index <= Delimiters.Length(); ++Index)
    {
      if (Delimiters[Index] == Ch)
      {
        return true;
      }
    }
  }
  return false;
}

intptr_t FirstDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str)
{
  if (Str.Length())
  {
    for (intptr_t Index = 1; Index <= Str.Length(); ++Index)
    {
      if (Str.IsDelimiter(Delimiters, Index))
      {
        return Index;
      }
    }
  }
  return 0;
}

intptr_t LastDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str)
{
  if (Str.Length())
  {
    for (intptr_t Index = Str.Length(); Index >= 1; --Index)
    {
      if (Str.IsDelimiter(Delimiters, Index))
      {
        return Index;
      }
    }
  }
  return 0;
}

int StringCmp(const wchar_t * S1, const wchar_t * S2)
{
  return ::CompareString(0, SORT_STRINGSORT, S1, -1, S2, -1) - 2;
}

int StringCmpI(const wchar_t * S1, const wchar_t * S2)
{
  return ::CompareString(0, NORM_IGNORECASE | SORT_STRINGSORT, S1, -1, S2, -1) - 2;
}

intptr_t CompareText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

intptr_t AnsiCompare(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

// Case-sensitive compare
intptr_t AnsiCompareStr(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

bool AnsiSameText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmpI(Str1.c_str(), Str2.c_str()) == 0;
}

bool SameText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiSameText(Str1, Str2);
}

intptr_t AnsiCompareText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmpI(Str1.c_str(), Str2.c_str());
}

intptr_t AnsiCompareIC(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiCompareText(Str1, Str2);
}

bool AnsiSameStr(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiCompareIC(Str1, Str2) == 0;
}

bool AnsiContainsText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return ::Pos(Str1, Str2) > 0;
}

bool ContainsStr(const AnsiString & Str1, const AnsiString & Str2)
{
  return Str1.Pos(Str2) > 0;
}

bool ContainsText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiContainsText(Str1, Str2);
}

UnicodeString RightStr(const UnicodeString & Str, intptr_t ACount)
{
  UnicodeString Result = Str.SubString(Str.Length() - ACount, ACount);
  return Result;
}

intptr_t PosEx(const UnicodeString & SubStr, const UnicodeString & Str, intptr_t Offset)
{
  UnicodeString S = Str.SubString(Offset);
  intptr_t Result = S.Pos(SubStr);
  return Result;
}

UnicodeString UTF8ToString(const RawByteString & Str)
{
  return MB2W(Str.c_str(), CP_UTF8);
}

UnicodeString UTF8ToString(const char * Str, intptr_t Len)
{
  if (!Str || !*Str || !Len)
  {
    return UnicodeString(L"");
  }

  intptr_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, Str, static_cast<int>(Len), nullptr, 0);
  UnicodeString Result;
  if (reqLength)
  {
    Result.SetLength(reqLength);
    ::MultiByteToWideChar(CP_UTF8, 0, Str, static_cast<int>(Len), const_cast<LPWSTR>(Result.c_str()), static_cast<int>(reqLength));
    Result.SetLength(Result.Length() - 1);  //remove NULL character
  }
  return Result;
}

void RaiseLastOSError(DWORD LastError)
{
  if (LastError == 0)
    LastError = ::GetLastError();
  UnicodeString ErrorMsg;
  if (LastError != 0)
  {
    ErrorMsg = FMTLOAD(SOSError, LastError, ::SysErrorMessage(LastError).c_str());
  }
  else
  {
    ErrorMsg = FMTLOAD(SUnkOSError);
  }
  throw EOSError(ErrorMsg, LastError);
}

double StrToFloat(const UnicodeString & Value)
{
  return StrToFloatDef(Value, 0.0);
}

double StrToFloatDef(const UnicodeString & Value, double DefVal)
{
  double Result = 0.0;
  try
  {
    Result = atof(Wide2MB(Value.c_str()).c_str());
  }
  catch (...)
  {
    Result = DefVal;
  }
  return Result;
}

UnicodeString FormatFloat(const UnicodeString & /*Format*/, double Value)
{
  UnicodeString Result(20, L'\0');
#ifndef __linux__
  swprintf(&Result[1], L"%.2f", Value);
#else
  swprintf(&Result[1], 20, L"%.2f", Value);
#endif
  return Result.c_str();
}

bool IsZero(double Value)
{
  return fabs(Value) < std::numeric_limits<double>::epsilon();
}

TTimeStamp DateTimeToTimeStamp(const TDateTime & DateTime)
{
  TTimeStamp Result = {0, 0};
  double fractpart, intpart;
  fractpart = modf(DateTime, &intpart);
  Result.Time = static_cast<int>(fractpart * MSecsPerDay + 0.5);
  Result.Date = static_cast<int>(intpart + DateDelta);
  return Result;
}

int64_t FileRead(HANDLE AHandle, void * Buffer, int64_t Count)
{
  int64_t Result = -1;
  DWORD Res = 0;
  if (::ReadFile(AHandle, reinterpret_cast<LPVOID>(Buffer), static_cast<DWORD>(Count), &Res, nullptr))
  {
    Result = Res;
  }
  else
  {
    Result = -1;
  }
  return Result;
}

int64_t FileWrite(HANDLE AHandle, const void * Buffer, int64_t Count)
{
  int64_t Result = -1;
  DWORD Res = 0;
  if (::WriteFile(AHandle, Buffer, static_cast<DWORD>(Count), &Res, nullptr))
  {
    Result = Res;
  }
  else
  {
    Result = -1;
  }
  return Result;
}

int64_t FileSeek(HANDLE AHandle, int64_t Offset, DWORD Origin)
{
  LONG low = Offset & 0xFFFFFFFF;
  LONG high = Offset >> 32;
  low = ::SetFilePointer(AHandle, low, &high, Origin);
  return ((int64_t)high << 32) + low;
}

bool FileExists(const UnicodeString & AFileName)
{
  return FileGetAttr(AFileName) != INVALID_FILE_ATTRIBUTES;
}

bool RenameFile(const UnicodeString & From, const UnicodeString & To)
{
  bool Result = ::MoveFile(From.c_str(), To.c_str()) != 0;
  return Result;
}

bool DirectoryExists(const UnicodeString & ADir)
{
  if ((ADir == THISDIRECTORY) || (ADir == PARENTDIRECTORY))
  {
    return true;
  }

  DWORD LocalFileAttrs = FileGetAttr(ADir);

  if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, FILE_ATTRIBUTE_DIRECTORY))
  {
    return true;
  }
  return false;
}

UnicodeString FileSearch(const UnicodeString & AFileName, const UnicodeString & DirectoryList)
{
  UnicodeString Temp;
  UnicodeString Result;
  Temp = DirectoryList;
  UnicodeString PathSeparators = L"/\\";
  do
  {
    intptr_t Index = ::Pos(Temp, PathSeparators);
    while ((Temp.Length() > 0) && (Index == 0))
    {
      Temp.Delete(1, 1);
      Index = ::Pos(Temp, PathSeparators);
    }
    Index = ::Pos(Temp, PathSeparators);
    if (Index > 0)
    {
      Result = Temp.SubString(1, Index - 1);
      Temp.Delete(1, Index);
    }
    else
    {
      Result = Temp;
      Temp.Clear();
    }
    Result = ::IncludeTrailingBackslash(Result);
    Result = Result + AFileName;
    if (!::FileExists(Result))
    {
      Result.Clear();
    }
  }
  while (!(Temp.Length() == 0) || (Result.Length() != 0));
  return Result;
}

void FileAge(const UnicodeString & AFileName, TDateTime & ATimestamp)
{
  WIN32_FIND_DATA FindData;
  HANDLE LocalFileHandle = ::FindFirstFile(ApiPath(AFileName).c_str(), &FindData);
  if (LocalFileHandle != INVALID_HANDLE_VALUE)
  {
    ATimestamp =
      UnixToDateTime(
        ConvertTimestampToUnixSafe(FindData.ftLastWriteTime, dstmUnix),
        dstmUnix);
    ::FindClose(LocalFileHandle);
  }
}

DWORD FileGetAttr(const UnicodeString & AFileName, bool FollowLink)
{
  TODO("FollowLink");
  DWORD LocalFileAttrs = ::GetFileAttributes(ApiPath(AFileName).c_str());
  return LocalFileAttrs;
}

DWORD FileSetAttr(const UnicodeString & AFileName, DWORD LocalFileAttrs)
{
  DWORD Result = ::SetFileAttributes(ApiPath(AFileName).c_str(), LocalFileAttrs);
  return Result;
}

bool CreateDir(const UnicodeString & ADir, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  return ::CreateDirectory(ApiPath(ADir).c_str(), SecurityAttributes) != 0;
}

bool RemoveDir(const UnicodeString & ADir)
{
  return ::RemoveDirectory(ApiPath(ADir).c_str()) != 0;
}

bool ForceDirectories(const UnicodeString & ADir)
{
  bool Result = true;
  if (ADir.IsEmpty())
  {
    return false;
  }
  UnicodeString Dir2 = ::ExcludeTrailingBackslash(ADir);
  if ((Dir2.Length() < 3) || ::DirectoryExists(Dir2))
  {
    return Result;
  }
  if (::ExtractFilePath(Dir2).IsEmpty())
  {
    return ::CreateDir(Dir2);
  }
  Result = ::ForceDirectories(::ExtractFilePath(Dir2)) && CreateDir(Dir2);
  return Result;
}

bool RemoveFile(const UnicodeString & AFileName)
{
  ::DeleteFile(ApiPath(AFileName).c_str());
  return !::FileExists(AFileName);
}

UnicodeString Format(const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  UnicodeString Result = ::FormatV(Format, Args);
  va_end(Args);
  return Result.c_str();
}

UnicodeString FormatV(const wchar_t * Format, va_list Args)
{
  UnicodeString Result;
  if (Format && *Format)
  {
#ifndef __linux__
    intptr_t Len = _vscwprintf(Format, Args);
    Result.SetLength(Len + 1);
    vswprintf(const_cast<wchar_t *>(Result.c_str()), Len + 1, Format, Args);
#else
    Result.SetLength(1024);
    int Len = vswprintf(const_cast<wchar_t *>(Result.c_str()), 1024, Format, Args);
    Result.SetLength(Len + 1);
#endif
  }
  return Result.c_str();
}

AnsiString FormatA(const char * Format, ...)
{
  AnsiString Result(64, 0);
  va_list Args;
  va_start(Args, Format);
  Result = ::FormatA(Format, Args);
  va_end(Args);
  return Result;
}

AnsiString FormatA(const char * Format, va_list Args)
{
  AnsiString Result(64, 0);
  if (Format && *Format)
  {
#ifndef __linux__
    intptr_t Len = _vscprintf(Format, Args);
    Result.SetLength(Len + 1);
    vsprintf_s(&Result[1], Len + 1, Format, Args);
#else
    Result.SetLength(1024);
    int Len = vsnprintf(&Result[1], 1024, Format, Args);
    Result.SetLength(Len + 1);
#endif
  }
  return Result.c_str();
}

UnicodeString FmtLoadStr(intptr_t Id, ...)
{
  UnicodeString Result;
//  HINSTANCE hInstance = GetGlobalFunctions()->GetInstanceHandle();
//  intptr_t Length = ::LoadString(hInstance, static_cast<UINT>(Id),
//    const_cast<wchar_t *>(Fmt.c_str()), static_cast<int>(Fmt.GetLength()));
//  if (!Length)
//  {
//    DEBUG_PRINTF(L"Unknown resource string id: %d\n", Id);
//  }
//  else
  UnicodeString Fmt = GetGlobalFunctions()->GetMsg(Id);
  if (!Fmt.IsEmpty())
  {
    va_list Args;
    va_start(Args, Id);
#ifndef __linux__
    intptr_t Len = _vscwprintf(Fmt.c_str(), Args);
    Result.SetLength(Len + sizeof(wchar_t));
    vswprintf_s(&Result[1], Result.Length(), Fmt.c_str(), Args);
#else
    Result.SetLength(1024);
    int Len = vswprintf(&Result[1], Result.Length(), Fmt.c_str(), Args);
    Result.SetLength(Len + 1);
#endif
    va_end(Args);
  }
  else
  {
    DEBUG_PRINTF("Unknown resource string id: %d\n", Id);
  }
  return Result;
}

// Returns the next available word, ignoring whitespace
static const wchar_t *
NextWord(const wchar_t * Input)
{
  static wchar_t buffer[1024];
  static const wchar_t * text = nullptr;

  wchar_t * endOfBuffer = buffer + _countof(buffer) - 1;
  wchar_t * pBuffer = buffer;

  if (Input)
  {
    text = Input;
  }

  if (text)
  {
    /* add leading spaces */
    while (iswspace(*text))
    {
      *(pBuffer++) = *(text++);
    }

    /* copy the word to our static buffer */
    while (*text && !iswspace(*text) && pBuffer < endOfBuffer)
    {
      *(pBuffer++) = *(text++);
    }
  }

  *pBuffer = 0;

  return buffer;
}

UnicodeString WrapText(const UnicodeString & Line, intptr_t MaxWidth)
{
  UnicodeString Result;

  intptr_t LenBuffer = 0;
  intptr_t SpaceLeft = MaxWidth;

  if (MaxWidth == 0)
  {
    MaxWidth = 78;
  }
  if (MaxWidth < 5)
  {
    MaxWidth = 5;
  }

  /* two passes through the input. the first pass updates the buffer length.
   * the second pass creates and populates the buffer
   */
  while (Result.Length() == 0)
  {
    intptr_t LineCount = 0;

    if (LenBuffer)
    {
      /* second pass, so create the wrapped buffer */
      Result.SetLength(LenBuffer + 1);
      if (Result.Length() == 0)
      {
        break;
      }
    }
    wchar_t * w = const_cast<wchar_t *>(Result.c_str());

    /* for each Word in Text
         if Width(Word) > SpaceLeft
           insert line break before Word in Text
           SpaceLeft := LineWidth - Width(Word)
         else
           SpaceLeft := SpaceLeft - Width(Word) + SpaceWidth
    */
    const wchar_t * s = NextWord(Line.c_str());
    while (*s)
    {
      SpaceLeft = MaxWidth;

      /* force the first word to always be completely copied */
      while (*s)
      {
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        else
        {
          *(w++) = *s;
        }
        --SpaceLeft;
        ++s;
      }
      if (!*s)
      {
        s = NextWord(nullptr);
      }

      /* copy as many words as will fit onto the current line */
      while (*s && static_cast<intptr_t>(wcslen(s) + 1) <= SpaceLeft)
      {
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        --SpaceLeft;

        /* then copy the word */
        while (*s)
        {
          if (Result.Length() == 0)
          {
            ++LenBuffer;
          }
          else
          {
            *(w++) = *s;
          }
          --SpaceLeft;
          ++s;
        }
        if (!*s)
        {
          s = NextWord(nullptr);
        }
      }
      if (!*s)
      {
        s = NextWord(nullptr);
      }

      if (*s)
      {
        /* add a new line here */
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        else
        {
          *(w++) = L'\n';
        }
        // Skip whitespace before first word on new line
        while (iswspace(*s))
        {
          ++s;
        }
      }

      ++LineCount;
    }

    LenBuffer += 2;

    if (w)
    {
      *w = 0;
    }
  }

  return Result;
}

UnicodeString TranslateExceptionMessage(Exception * E)
{
  if (E)
  {
    if (NB_STATIC_DOWNCAST(Exception, E) != nullptr)
    {
      return NB_STATIC_DOWNCAST(Exception, E)->Message;
    }
    else
    {
      return E->what();
    }
  }
  else
  {
    return UnicodeString();
  }
}

void AppendWChar(UnicodeString & Str, const wchar_t Ch)
{
  if (!Str.IsEmpty() && Str[Str.Length()] != Ch)
  {
    Str += Ch;
  }
}

void AppendChar(std::string & Str, const char Ch)
{
  if (!Str.empty() && Str[Str.length() - 1] != Ch)
  {
    Str += Ch;
  }
}

void AppendPathDelimiterW(UnicodeString & Str)
{
  if (!Str.IsEmpty() && Str[Str.Length()] != L'/' && Str[Str.Length()] != L'\\')
  {
    Str += L"\\";
  }
}

UnicodeString ExpandEnvVars(const UnicodeString & Str)
{
#ifndef __linux__
  UnicodeString Buf(32 * 1024, 0);
  intptr_t size = ::ExpandEnvironmentStringsW(Str.c_str(), (wchar_t *)Buf.c_str(), static_cast<DWORD>(32 * 1024 - 1));
  UnicodeString Result = UnicodeString(Buf.c_str(), size - 1);
  return Result;
#else
  return Str;
#endif
}

UnicodeString StringOfChar(const wchar_t Ch, intptr_t Len)
{
  UnicodeString Result;
  if (Len < 0)
    Len = 0;
  Result.SetLength(Len);
  for (intptr_t Index = 1; Index <= Len; ++Index)
  {
    Result[Index] = Ch;
  }
  return Result;
}

UnicodeString ChangeFileExt(const UnicodeString & AFileName, const UnicodeString & AExt,
  wchar_t Delimiter)
{
  UnicodeString Result = ::ChangeFileExtension(AFileName, AExt, Delimiter);
  return Result;
}

UnicodeString ExtractFileExt(const UnicodeString & AFileName)
{
  UnicodeString Result = ::ExtractFileExtension(AFileName, L'.');
  return Result;
}

static UnicodeString ExpandFileName(const UnicodeString & AFileName)
{
#ifndef __linux__
  UnicodeString Result;
  UnicodeString Buf(NB_MAX_PATH, 0);
  intptr_t Size = ::GetFullPathNameW(AFileName.c_str(), static_cast<DWORD>(Buf.Length() - 1),
    reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Buf.c_str())), nullptr);
  if (Size > Buf.Length())
  {
    Buf.SetLength(Size);
    Size = ::GetFullPathNameW(AFileName.c_str(), static_cast<DWORD>(Buf.Length() - 1),
      reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Buf.c_str())), nullptr);
  }
  return UnicodeString(Buf.c_str(), Size);
#else
  return AFileName;
#endif
}

static UnicodeString GetUniversalName(UnicodeString & AFileName)
{
  UnicodeString Result = AFileName;
  return Result;
}

UnicodeString ExpandUNCFileName(const UnicodeString & AFileName)
{
  UnicodeString Result = ExpandFileName(AFileName);
  if ((Result.Length() >= 3) && (Result[1] == L':') && (::UpCase(Result[1]) >= L'A') &&
      (::UpCase(Result[1]) <= L'Z'))
  {
    Result = GetUniversalName(Result);
  }
  return Result;
}

static DWORD FindMatchingFile(TSearchRec & Rec)
{
  TFileTime LocalFileTime = {0};
  DWORD Result = ERROR_SUCCESS;
  while ((Rec.FindData.dwFileAttributes && Rec.ExcludeAttr) != 0)
  {
    if (!::FindNextFile(Rec.FindHandle, &Rec.FindData))
    {
      Result = ::GetLastError();
      return Result;
    }
  }
  FileTimeToLocalFileTime(&Rec.FindData.ftLastWriteTime, reinterpret_cast<LPFILETIME>(&LocalFileTime));
  WORD Hi = (Rec.Time & 0xFFFF0000) >> 16;
  WORD Lo = Rec.Time & 0xFFFF;
  FileTimeToDosDateTime(reinterpret_cast<LPFILETIME>(&LocalFileTime), &Hi, &Lo);
  Rec.Time = (Hi << 16) + Lo;
  Rec.Size = Rec.FindData.nFileSizeLow || static_cast<Int64>(Rec.FindData.nFileSizeHigh) << 32;
  Rec.Attr = Rec.FindData.dwFileAttributes;
  Rec.Name = Rec.FindData.cFileName;
  Result = ERROR_SUCCESS;
  return Result;
}

DWORD FindFirst(const UnicodeString & AFileName, DWORD LocalFileAttrs, TSearchRec & Rec)
{
  const DWORD faSpecial = faHidden | faSysFile | faDirectory;
  Rec.ExcludeAttr = (~LocalFileAttrs) & faSpecial;
  Rec.FindHandle = ::FindFirstFile(ApiPath(AFileName).c_str(), &Rec.FindData);
  DWORD Result = ERROR_SUCCESS;
  if (Rec.FindHandle != INVALID_HANDLE_VALUE)
  {
    Result = FindMatchingFile(Rec);
    if (Result != ERROR_SUCCESS)
    {
      FindClose(Rec);
    }
  }
  else
  {
    Result = ::GetLastError();
  }
  return Result;
}

DWORD FindNext(TSearchRec & Rec)
{
  DWORD Result = 0;
  if (::FindNextFile(Rec.FindHandle, &Rec.FindData))
    Result = FindMatchingFile(Rec);
  else
    Result = ::GetLastError();
  return Result;
}

DWORD FindClose(TSearchRec & Rec)
{
  DWORD Result = 0;
  if (Rec.FindHandle != INVALID_HANDLE_VALUE)
  {
    ::FindClose(Rec.FindHandle);
    Rec.FindHandle = INVALID_HANDLE_VALUE;
  }
  return Result;
}

void InitPlatformId()
{
#ifndef __linux__
  OSVERSIONINFO OSVersionInfo;
  OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
  if (::GetVersionEx(&OSVersionInfo) != 0)
  {
    Win32Platform = OSVersionInfo.dwPlatformId;
    Win32MajorVersion = OSVersionInfo.dwMajorVersion;
    Win32MinorVersion = OSVersionInfo.dwMinorVersion;
    Win32BuildNumber = OSVersionInfo.dwBuildNumber;
    // Win32CSDVersion = OSVersionInfo.szCSDVersion;
  }
#endif
}

bool Win32Check(bool RetVal)
{
  if (!RetVal)
  {
    RaiseLastOSError();
  }
  return RetVal;
}

UnicodeString SysErrorMessage(int ErrorCode)
{
  UnicodeString Result;
#ifndef __linux__
  wchar_t Buffer[255];
  intptr_t Len = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_ARGUMENT_ARRAY, nullptr, ErrorCode, 0,
    static_cast<LPTSTR>(Buffer),
    sizeof(Buffer), nullptr);
  while ((Len > 0) && ((Buffer[Len - 1] != 0) &&
    ((Buffer[Len - 1] <= 32) || (Buffer[Len - 1] == L'.'))))
  {
    Len--;
  }
  Result = UnicodeString(Buffer, Len);
#endif
  return Result;
}

UnicodeString ReplaceStrAll(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat)
{
  UnicodeString Result = Str;
  intptr_t Pos = Result.Pos(What);
  while (Pos > 0)
  {
    Result.Replace(Pos, What.Length(), ByWhat.c_str(), ByWhat.Length());
    Pos = Result.Pos(What);
  }
  return Result;
}

UnicodeString ExtractShortPathName(const UnicodeString & APath)
{
  // FIXME
  return APath;
}

//
// Returns everything, including the trailing Path separator, except the Filename
// part of the Path.
//
// "/foo/bar/baz.txt" --> "/foo/bar/"
UnicodeString ExtractDirectory(const UnicodeString & APath, wchar_t Delimiter)
{
  UnicodeString Result = APath.SubString(1, APath.RPos(Delimiter));
  return Result;
}

//
// Returns only the Filename part of the Path.
//
// "/foo/bar/baz.txt" --> "baz.txt"
UnicodeString ExtractFilename(const UnicodeString & APath, wchar_t Delimiter)
{
  return APath.SubString(APath.RPos(Delimiter) + 1);
}

//
// Returns the file's extension, if any. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt" --> ".txt"
// "/foo/bar/baz" --> ""
UnicodeString ExtractFileExtension(const UnicodeString & APath, wchar_t Delimiter)
{
  UnicodeString FileName = ::ExtractFilename(APath, Delimiter);
  intptr_t N = FileName.RPos(L'.');
  if (N > 0)
  {
    return FileName.SubString(N);
  }
  return UnicodeString();
}

//
// Modifies the Filename's extension. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt", ".dat" --> "/foo/bar/baz.dat"
// "/foo/bar/baz.txt", "" --> "/foo/bar/baz"
// "/foo/bar/baz", ".txt" --> "/foo/bar/baz.txt"
//
UnicodeString ChangeFileExtension(const UnicodeString & APath, const UnicodeString & Ext, wchar_t Delimiter)
{
  UnicodeString FileName = ::ExtractFilename(APath, Delimiter);
  if (FileName.RPos(L'.') > 1)
  {
    return ExtractDirectory(APath, Delimiter) +
           FileName.SubString(1, FileName.RPos(L'.') - 1) +
           Ext;
  }
  else
  {
    return ExtractDirectory(APath, Delimiter) +
           FileName +
           Ext;
  }
}

UnicodeString ExcludeTrailingBackslash(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  if ((Result.Length() > 0) && ((Result[Result.Length()] == L'/') ||
      (Result[Result.Length()] == L'\\')))
  {
    Result.SetLength(Result.Length() - 1);
  }
  return Result;
}

UnicodeString IncludeTrailingBackslash(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  intptr_t L = Result.Length();
  if ((L == 0) || ((Result[L] != L'/') && (Result[L] != L'\\')))
  {
    Result += L'\\';
  }
  return Result;
}

UnicodeString IncludeTrailingPathDelimiter(const UnicodeString & Str)
{
  return IncludeTrailingBackslash(Str);
}

UnicodeString ExtractFileDir(const UnicodeString & Str)
{
  UnicodeString Result;
  intptr_t Pos = Str.LastDelimiter(L"/\\");
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    Result = Str.SubString(1, Pos);
  }
  else
  {
    Result = (Pos == 1) ? UnicodeString(ROOTDIRECTORY) : UnicodeString();
  }
  return Result;
}

UnicodeString ExtractFilePath(const UnicodeString & Str)
{
  UnicodeString Result = ::ExtractFileDir(Str);
  return Result;
}

UnicodeString GetCurrentDir()
{
  UnicodeString Result = GetGlobalFunctions()->GetCurrDirectory();
  return Result;
}

UnicodeString StrToHex(const UnicodeString & Str, bool UpperCase, wchar_t Separator)
{
  UnicodeString Result;
  for (intptr_t Index = 1; Index <= Str.Length(); ++Index)
  {
    Result += CharToHex(Str[Index], UpperCase);
    if ((Separator != L'\0') && (Index <= Str.Length()))
    {
      Result += Separator;
    }
  }
  return Result;
}

UnicodeString HexToStr(const UnicodeString & Hex)
{
  UnicodeString Digits = "0123456789ABCDEF";
  UnicodeString Result;
  intptr_t L = Hex.Length() - 1;
  if (L % 2 == 0)
  {
    for (intptr_t Index = 1; Index <= Hex.Length(); Index += 2)
    {
      intptr_t P1 = Digits.FindFirstOf(::UpCase(Hex[Index]));
      intptr_t P2 = Digits.FindFirstOf(::UpCase(Hex[Index + 1]));
      if ((P1 == NPOS) || (P2 == NPOS))
      {
        Result.Clear();
        break;
      }
      else
      {
        Result += static_cast<wchar_t>((P1 - 1) * 16 + P2 - 1);
      }
    }
  }
  return Result;
}

uintptr_t HexToInt(const UnicodeString & Hex, uintptr_t MinChars)
{
  UnicodeString Digits = "0123456789ABCDEF";
  uintptr_t Result = 0;
  intptr_t Index = 1;
  while (Index <= Hex.Length())
  {
    intptr_t A = Digits.FindFirstOf(UpCase(Hex[Index]));
    if (A == NPOS)
    {
      if ((MinChars == NPOS) || (Index <= static_cast<intptr_t>(MinChars)))
      {
          Result = 0;
      }
      break;
    }

    Result = (Result * 16) + (static_cast<int>(A) - 1);

    ++Index;
  }
  return Result;
}

UnicodeString IntToHex(uintptr_t Int, uintptr_t MinChars)
{
  UnicodeString Result;
  Result.sprintf(L"%X", Int);
  intptr_t Pad = MinChars - Result.Length();
  if (Pad > 0)
  {
    for (intptr_t Index = 0; Index < Pad; ++Index)
    {
      Result.Insert(L'0', 1);
    }
  }
  return Result;
}

char HexToChar(const UnicodeString & Hex, uintptr_t MinChars)
{
  return static_cast<char>(HexToInt(Hex, MinChars));
}

static void ConvertError(intptr_t ErrorID)
{
  UnicodeString Msg = FMTLOAD(ErrorID, 0);
  throw EConvertError(Msg);
}

static void DivMod(const uintptr_t Dividend, uintptr_t Divisor,
  uintptr_t & Result, uintptr_t & Remainder)
{
  Result = Dividend / Divisor;
  Remainder = Dividend % Divisor;
}

static bool DecodeDateFully(const TDateTime & DateTime,
  uint16_t & Year, uint16_t & Month, uint16_t & Day,
  uint16_t & DOW)
{
  static const int D1 = 365;
  static const int D4 = D1 * 4 + 1;
  static const int D100 = D4 * 25 - 1;
  static const int D400 = D100 * 4 + 1;
  bool Result = false;
  int T = DateTimeToTimeStamp(DateTime).Date;
  if (T <= 0)
  {
    Year = 0;
    Month = 0;
    Day = 0;
    DOW = 0;
    return false;
  }
  else
  {
    DOW = T % 7 + 1;
    T--;
    uintptr_t Y = 1;
    while (T >= D400)
    {
      T -= D400;
      Y += 400;
    }
    uintptr_t D = 0;
    uintptr_t I = 0;
    DivMod(T, D100, I, D);
    if (I == 4)
    {
      I--;
      D += D100;
    }
    Y += I * 100;
    DivMod(D, D4, I, D);
    Y += I * 4;
    DivMod(D, D1, I, D);
    if (I == 4)
    {
      I--;
      D += ToWord(D1);
    }
    Y += I;
    Result = IsLeapYear(ToWord(Y));
    const TDayTable * DayTable = &MonthDays[Result];
    uintptr_t M = 1;
    while (true)
    {
      I = (*DayTable)[M - 1];
      if (D < I)
      {
        break;
      }
      D -= I;
      M++;
    }
    Year = static_cast<uint16_t>(Y);
    Month = static_cast<uint16_t>(M);
    Day = static_cast<uint16_t>(D + 1);
  }
  return Result;
}

void DecodeDate(const TDateTime & DateTime, uint16_t & Year,
  uint16_t & Month, uint16_t & Day)
{
  uint16_t Dummy = 0;
  DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}

void DecodeTime(const TDateTime & DateTime, uint16_t & Hour,
  uint16_t & Min, uint16_t & Sec, uint16_t & MSec)
{
  uintptr_t MinCount, MSecCount;
  DivMod(DateTimeToTimeStamp(DateTime).Time, 60000, MinCount, MSecCount);
  uintptr_t H, M, S, MS;
  DivMod(MinCount, 60, H, M);
  DivMod(MSecCount, 1000, S, MS);
  Hour = static_cast<uint16_t>(H);
  Min = static_cast<uint16_t>(M);
  Sec = static_cast<uint16_t>(S);
  MSec = static_cast<uint16_t>(MS);
}

static bool TryEncodeDate(int Year, int Month, int Day, TDateTime & Date)
{
  const TDayTable * DayTable = &MonthDays[IsLeapYear(ToWord(Year))];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
      (Day >= 1) && (Day <= (*DayTable)[Month - 1]))
  {
    for (int Index = 1; Index <= Month - 1; Index++)
    {
      Day += (*DayTable)[Index - 1];
    }
    int Idx = Year - 1;
    Date = TDateTime((double)(Idx * 365 + Idx / 4 - Idx / 100 + Idx / 400 + Day - DateDelta));
    return true;
  }
  return false;
}

TDateTime EncodeDate(int Year, int Month, int Day)
{
  TDateTime Result;
  if (!TryEncodeDate(Year, Month, Day, Result))
  {
    ::ConvertError(SDateEncodeError);
  }
  return Result;
}

static bool TryEncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec,
  TDateTime & Time)
{
  bool Result = false;
  if ((Hour < 24) && (Min < 60) && (Sec < 60) && (MSec < 1000))
  {
    Time = (Hour * 3600000 + Min * 60000 + Sec * 1000 + MSec) / ToDouble(MSecsPerDay);
    Result = true;
  }
  return Result;
}

TDateTime EncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec)
{
  TDateTime Result;
  if (!TryEncodeTime(Hour, Min, Sec, MSec, Result))
  {
    ::ConvertError(STimeEncodeError);
  }
  return Result;
}

TDateTime StrToDateTime(const UnicodeString & Value)
{
  (void)Value;
  ThrowNotImplemented(145);
  return TDateTime();
}

bool TryStrToDateTime(const UnicodeString & StrValue, TDateTime & Value,
  TFormatSettings & FormatSettings)
{
  (void)StrValue;
  (void)Value;
  (void)FormatSettings;
  ThrowNotImplemented(147);
  return false;
}

UnicodeString DateTimeToStr(UnicodeString & Result, const UnicodeString & Format,
  const TDateTime & DateTime)
{
  (void)Result;
  (void)Format;
  return DateTime.FormatString((wchar_t *)L"");
}

UnicodeString DateTimeToString(const TDateTime & DateTime)
{
  return DateTime.FormatString((wchar_t *)L"");
}

// DayOfWeek returns the day of the week of the given date. The Result is an
// integer between 1 and 7, corresponding to Sunday through Saturday.
// This function is not ISO 8601 compliant, for that see the DateUtils unit.
uint32_t DayOfWeek(const TDateTime & DateTime)
{
  return ::DateTimeToTimeStamp(DateTime).Date % 7 + 1;
}

TDateTime Date()
{
  SYSTEMTIME t;
  ::GetLocalTime(&t);
  TDateTime Result = ::EncodeDate(t.wYear, t.wMonth, t.wDay);
  return Result;
}

UnicodeString FormatDateTime(const UnicodeString & Fmt, const TDateTime & ADateTime)
{
  (void)Fmt;
  UnicodeString Result;
  if (Fmt == L"ddddd tt")
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
    uint16_t Year;
    uint16_t Month;
    uint16_t Day;
    uint16_t Hour;
    uint16_t Minutes;
    uint16_t Seconds;
    uint16_t Milliseconds;

    ADateTime.DecodeDate(Year, Month, Day);
    ADateTime.DecodeTime(Hour, Minutes, Seconds, Milliseconds);

    uint16_t Y, M, D, H, Mm, S, MS;
    TDateTime DateTime =
        EncodeDateVerbose(Year, Month, Day) +
        EncodeTimeVerbose(Hour, Minutes, Seconds, Milliseconds);
    DateTime.DecodeDate(Y, M, D);
    DateTime.DecodeTime(H, Mm, S, MS);
    Result = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, Mm, S);
  }
  else
  {
    ThrowNotImplemented(150);
  }
  return Result;
}

static TDateTime ComposeDateTime(const TDateTime & Date, const TDateTime & Time)
{
  TDateTime Result = TDateTime(Date);
  Result += Time;
  return Result;
}

TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime)
{
  TDateTime Result(0.0);
  Result = ComposeDateTime(EncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay),
    EncodeTime(SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds));
  return Result;
}

UnicodeString UnixExcludeLeadingBackslash(const UnicodeString & APath)
{
  UnicodeString Result = APath;
  while (!Result.IsEmpty() && Result[1] == L'/')
  {
    Result.Delete(1, 1);
  }
  return Result;
}

void Randomize()
{
  srand(static_cast<uint32_t>(time(nullptr)));
}

static void IncAMonth(Word & Year, Word & Month, Word & Day, Int64 NumberOfMonths = 1)
{
  Integer Sign;
  if (NumberOfMonths >= 0)
    Sign = 1;
  else
    Sign = -1;
  Year = Year + (NumberOfMonths % 12);
  NumberOfMonths = NumberOfMonths / 12;
  Month += ToWord(NumberOfMonths);
  if (ToWord(Month-1) > 11) // if Month <= 0, word(Month-1) > 11)
  {
    Year += ToWord(Sign);
    Month += -12 * ToWord(Sign);
  }
  const TDayTable * DayTable = &MonthDays[IsLeapYear(Year)];
  if (Day > (*DayTable)[Month])
    Day = ToWord(*DayTable[Month]);
}

static void ReplaceTime(TDateTime & DateTime, const TDateTime & NewTime)
{
  DateTime = Trunc(DateTime);
  if (DateTime >= 0)
    DateTime = DateTime + Abs(Frac(NewTime));
  else
    DateTime = DateTime - Abs(Frac(NewTime));
}

TDateTime IncYear(const TDateTime & AValue, const Int64 ANumberOfYears)
{
  TDateTime Result;
  Result = IncMonth(AValue, ANumberOfYears * MonthsPerYear);
  return Result;
}

TDateTime IncMonth(const TDateTime & AValue, const Int64 NumberOfMonths)
{
  TDateTime Result;
  Word Year, Month, Day;
  DecodeDate(AValue, Year, Month, Day);
  IncAMonth(Year, Month, Day, NumberOfMonths);
  Result = EncodeDate(Year, Month, Day);
  ReplaceTime(Result, AValue);
  return Result;
}

TDateTime IncWeek(const TDateTime & AValue, const Int64 ANumberOfWeeks)
{
  TDateTime Result;
  Result = AValue + ANumberOfWeeks * DaysPerWeek;
  return Result;
}

TDateTime IncDay(const TDateTime & AValue, const Int64 ANumberOfDays)
{
  TDateTime Result;
  Result = AValue + ANumberOfDays;
  return Result;
}

TDateTime IncHour(const TDateTime & AValue, const Int64 ANumberOfHours)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * HoursPerDay) + ANumberOfHours) / HoursPerDay;
  else
    Result = ((AValue * HoursPerDay) - ANumberOfHours) / HoursPerDay;
  return Result;
}

TDateTime IncMinute(const TDateTime & AValue, const Int64 ANumberOfMinutes)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * MinsPerDay) + ANumberOfMinutes) / MinsPerDay;
  else
    Result = ((AValue * MinsPerDay) - ANumberOfMinutes) / MinsPerDay;
  return Result;
}

TDateTime IncSecond(const TDateTime & AValue, const Int64 ANumberOfSeconds)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * SecsPerDay) + ANumberOfSeconds) / SecsPerDay;
  else
    Result = ((AValue * SecsPerDay) - ANumberOfSeconds) / SecsPerDay;
  return Result;
}

TDateTime IncMilliSecond(const TDateTime & AValue, const Int64 ANumberOfMilliSeconds)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * MSecsPerDay) + ANumberOfMilliSeconds) / MSecsPerDay;
  else
    Result = ((AValue * MSecsPerDay) - ANumberOfMilliSeconds) / MSecsPerDay;
  return Result;
}

Boolean IsLeapYear(Word Year)
{
  return (Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0));
}

// TCriticalSection

TCriticalSection::TCriticalSection() :
  FAcquired(0)
{
#ifndef __linux__
  InitializeCriticalSection(&FSection);
#endif
}

TCriticalSection::~TCriticalSection()
{
  DebugAssert(FAcquired == 0);
#ifndef __linux__
  DeleteCriticalSection(&FSection);
#endif
}

void TCriticalSection::Enter() const
{
#ifndef __linux__
  ::EnterCriticalSection(&FSection);
#else
  FSection.lock();
#endif
  FAcquired++;
}

void TCriticalSection::Leave() const
{
  FAcquired--;
#ifndef __linux__
  ::LeaveCriticalSection(&FSection);
#else
  FSection.unlock();
#endif
}

UnicodeString StripHotkey(const UnicodeString & AText)
{
  UnicodeString Result = AText;
  intptr_t Len = Result.Length();
  intptr_t Pos = 1;
  while (Pos <= Len)
  {
    if (Result[Pos] == L'&')
    {
      Result.Delete(Pos, 1);
      Len--;
    }
    else
    {
      Pos++;
    }
  }
  return Result;
}

bool StartsText(const UnicodeString & ASubText, const UnicodeString & AText)
{
  return AText.Pos(ASubText) == 1;
}

uintptr_t StrToVersionNumber(const UnicodeString & VersionMumberStr)
{
  uintptr_t Result = 0;
  UnicodeString Version = VersionMumberStr;
  int Shift = 16;
  while (!Version.IsEmpty())
  {
    UnicodeString Num = CutToChar(Version, L'.', true);
    Result += static_cast<uintptr_t>(Num.ToInt()) << Shift;
    if (Shift >= 8)
      Shift -= 8;
  }
  return Result;
}

UnicodeString VersionNumberToStr(uintptr_t VersionNumber)
{
  DWORD Major = (VersionNumber>>16) & 0xFF;
  DWORD Minor = (VersionNumber>>8) & 0xFF;
  DWORD Revision = (VersionNumber & 0xFF);
  UnicodeString Result = FORMAT(L"%d.%d.%d", Major, Minor, Revision);
  return Result;
}

TFormatSettings::TFormatSettings(int) :
  CurrencyFormat(0),
  NegCurrFormat(0),
  ThousandSeparator(0),
  DecimalSeparator(0),
  CurrencyDecimals(0),
  DateSeparator(0),
  TimeSeparator(0),
  ListSeparator(0),
  TwoDigitYearCenturyWindow(0)
{
}

NB_IMPLEMENT_CLASS(Exception, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(EAccessViolation, NB_GET_CLASS_INFO(Exception), nullptr)
NB_IMPLEMENT_CLASS(EAbort, NB_GET_CLASS_INFO(Exception), nullptr)
NB_IMPLEMENT_CLASS(EFileNotFoundError, NB_GET_CLASS_INFO(Exception), nullptr)
NB_IMPLEMENT_CLASS(EOSError, NB_GET_CLASS_INFO(Exception), nullptr)
