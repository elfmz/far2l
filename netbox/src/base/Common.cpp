#define NO_WIN32_LEAN_AND_MEAN
#include <vcl.h>
#pragma hdrstop

#ifndef __linux__
#include <shellapi.h>
#endif
#include <Common.h>
#include <Global.h>
#include <StrUtils.hpp>
#include <Sysutils.hpp>
#include <DateUtils.hpp>
#include <math.h>
#ifndef __linux__
#include <shlobj.h>
#include <shlwapi.h>
#endif
#include <openssl/pkcs12.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <utils.h>
#include "TextsCore.h"

const wchar_t * DSTModeNames = L"Win;Unix;Keep";

const wchar_t EngShortMonthNames[12][4] =
{
  L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
  L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
};
const wchar_t TokenPrefix = L'%';
const wchar_t NoReplacement = wchar_t(0);
const wchar_t TokenReplacement = wchar_t(1);

UnicodeString ReplaceChar(const UnicodeString & Str, wchar_t A, wchar_t B)
{
  UnicodeString Result = Str;
  for (wchar_t * Ch = const_cast<wchar_t *>(Result.c_str()); Ch && *Ch; ++Ch)
    if (*Ch == A)
    {
      *Ch = B;
    }
  return Result;
}

UnicodeString DeleteChar(const UnicodeString & Str, wchar_t C)
{
  UnicodeString Result = Str;
  intptr_t P;
  while ((P = Result.Pos(C)) > 0)
  {
    Result.Delete(P, 1);
  }
  return Result;
}

template<typename T>
void DoPackStr(T & Str)
{
  // Following will free unnecessary bytes
  Str = Str.c_str();
}

void PackStr(UnicodeString & Str)
{
  DoPackStr(Str);
}

void PackStr(RawByteString & Str)
{
  DoPackStr(Str);
}

void PackStr(AnsiString & Str)
{
  DoPackStr(Str);
}

template<typename T>
void DoShred(T & Str)
{
  if (!Str.IsEmpty())
  {
    Str.Unique();
    ::memset((void*)Str.c_str(), 0, Str.Length() * sizeof(*Str.c_str()));
    Str = L"";
  }
}

void Shred(UnicodeString & Str)
{
  DoShred(Str);
}

void Shred(UTF8String & Str)
{
  DoShred(Str);
}

void Shred(AnsiString & Str)
{
  DoShred(Str);
}

UnicodeString AnsiToString(const RawByteString & S)
{
  return UnicodeString(AnsiString(S));
}

UnicodeString AnsiToString(const char * S, size_t Len)
{
  return UnicodeString(AnsiString(S, Len));
}

UnicodeString MakeValidFileName(const UnicodeString & AFileName)
{
  UnicodeString Result = AFileName;
  UnicodeString IllegalChars(L":;,=+<>|\"[] WGOOD_SLASH/?*");
  for (intptr_t Index = 0; Index < IllegalChars.Length(); ++Index)
  {
    Result = ReplaceChar(Result, IllegalChars[Index + 1], L'-');
  }
  return Result;
}

UnicodeString RootKeyToStr(HKEY RootKey)
{
  if (RootKey == HKEY_USERS)
    return "HKU";
  else if (RootKey == HKEY_LOCAL_MACHINE)
    return "HKLM";
  else if (RootKey == HKEY_CURRENT_USER)
    return "HKCU";
  else if (RootKey == HKEY_CLASSES_ROOT)
    return "HKCR";
#ifndef __linux__
  else if (RootKey == HKEY_CURRENT_CONFIG)
    return "HKCC";
  else if (RootKey == HKEY_DYN_DATA)
    return "HKDD";
#endif
  else
  {
    Abort();
    return "";
  }
}

UnicodeString BooleanToEngStr(bool B)
{
  if (B)
  {
    return "Yes";
  }
  else
  {
    return "No";
  }
}

UnicodeString BooleanToStr(bool B)
{
  if (B)
  {
    return LoadStr(YES_STR);
  }
  else
  {
    return LoadStr(NO_STR);
  }
}

UnicodeString DefaultStr(const UnicodeString & Str, const UnicodeString & Default)
{
  if (!Str.IsEmpty())
  {
    return Str;
  }
  else
  {
    return Default;
  }
}

UnicodeString CutToChar(UnicodeString & Str, wchar_t Ch, bool Trim)
{
  intptr_t P = Str.Pos(Ch);
  UnicodeString Result;
  if (P)
  {
    Result = Str.SubString(1, P - 1);
    Str.Delete(1, P);
  }
  else
  {
    Result = Str;
    Str.Clear();
  }
  if (Trim)
  {
    Result = Result.TrimRight();
    Str = Str.TrimLeft();
  }
  return Result;
}

UnicodeString CopyToChars(const UnicodeString & Str, intptr_t & From, const UnicodeString & Chs, bool Trim,
  wchar_t * Delimiter, bool DoubleDelimiterEscapes)
{
  UnicodeString Result;

  intptr_t P;
  for (P = From; P <= Str.Length(); P++)
  {
    if (::IsDelimiter(Chs, Str, P))
    {
      if (DoubleDelimiterEscapes &&
          (P < Str.Length()) &&
          ::IsDelimiter(Chs, Str, P + 1))
      {
        Result += Str[P];
        P++;
      }
      else
      {
        break;
      }
    }
    else
    {
      Result += Str[P];
    }
  }

  if (P <= Str.Length())
  {
    if (Delimiter != nullptr)
    {
      *Delimiter = Str[P];
    }
  }
  else
  {
    if (Delimiter != nullptr)
    {
      *Delimiter = L'\0';
    }
  }
  // even if we reached the end, return index, as if there were the delimiter,
  // so caller can easily find index of the end of the piece by subtracting
  // 2 from From (as long as he did not asked for trimming)
  From = P + 1;
  if (Trim)
  {
    Result = Result.TrimRight();
    while ((From <= Str.Length()) && (Str[From] == L' '))
    {
      From++;
    }
  }
  return Result;
}

UnicodeString CopyToChar(const UnicodeString & Str, wchar_t Ch, bool Trim)
{
  intptr_t From = 1;
  return CopyToChars(Str, From, UnicodeString(Ch), Trim);
}

UnicodeString DelimitStr(const UnicodeString & Str, const UnicodeString & Chars)
{
  UnicodeString Result = Str;

  for (intptr_t Index = 1; Index <= Result.Length(); ++Index)
  {
    if (Result.IsDelimiter(Chars, Index))
    {
      Result.Insert(WGOOD_SLASH, Index);
      ++Index;
    }
  }
  return Result;
}

UnicodeString ShellDelimitStr(const UnicodeString & Str, wchar_t Quote)
{
  UnicodeString Chars(L"$" WGOOD_SLASH);
  if (Quote == L'"')
  {
    Chars += L"`\"";
  }
  return DelimitStr(Str, Chars);
}

UnicodeString ExceptionLogString(Exception * E)
{
  DebugAssert(E);
  if (NB_STATIC_DOWNCAST(Exception, E) != nullptr)
  {
    UnicodeString Msg;
    Msg = FORMAT(L"%s", UnicodeString(E->what()).c_str());
    if (NB_STATIC_DOWNCAST(ExtException, E) != nullptr)
    {
      TStrings * MoreMessages = NB_STATIC_DOWNCAST(ExtException, E)->GetMoreMessages();
      if (MoreMessages)
      {
        Msg += L"\n" +
          ReplaceStr(MoreMessages->GetText(), L"\r", L"");
      }
    }
    return Msg;
  }
  else
  {
#if defined(__BORLANDC__)
    wchar_t Buffer[1024];
    ExceptionErrorMessage(ExceptObject(), ExceptAddr(), Buffer, _countof(Buffer));
    return UnicodeString(Buffer);
#else
    return UnicodeString(E->what());
#endif
  }
}

UnicodeString MainInstructions(const UnicodeString & S)
{
  UnicodeString MainMsgTag = LoadStr(MAIN_MSG_TAG);
  return MainMsgTag + S + MainMsgTag;
}

bool HasParagraphs(const UnicodeString & S)
{
  return (S.Pos(L"\n\n") > 0);
}

UnicodeString MainInstructionsFirstParagraph(const UnicodeString & S)
{
  // WORKAROUND, we consider it bad practice, the highlighting should better
  // be localized (but maybe we change our mind later)
  UnicodeString Result;
  intptr_t Pos = S.Pos(L"\n\n");
  // we would not be calling this on single paragraph message
  if (DebugAlwaysTrue(Pos > 0))
  {
    Result =
      MainInstructions(S.SubString(1, Pos - 1)) +
      S.SubString(Pos, S.Length() - Pos + 1);
  }
  else
  {
    Result = MainInstructions(S);
  }
  return Result;
}

bool ExtractMainInstructions(UnicodeString & S, UnicodeString & MainInstructions)
{
  bool Result = false;
  UnicodeString MainMsgTag = LoadStr(MAIN_MSG_TAG);
  if (StartsStr(MainMsgTag, S))
  {
    intptr_t EndTagPos =
      S.SubString(MainMsgTag.Length() + 1, S.Length() - MainMsgTag.Length()).Pos(MainMsgTag);
    if (EndTagPos > 0)
    {
      MainInstructions = S.SubString(MainMsgTag.Length() + 1, EndTagPos - 1);
      S.Delete(1, EndTagPos + (2 * MainMsgTag.Length()) - 1);
      Result = true;
    }
  }

  DebugAssert(MainInstructions.Pos(MainMsgTag) == 0);
  DebugAssert(S.Pos(MainMsgTag) == 0);

  return Result;
}

static intptr_t FindInteractiveMsgStart(const UnicodeString & S)
{
  intptr_t Result = 0;
  UnicodeString InteractiveMsgTag = LoadStr(INTERACTIVE_MSG_TAG);
  if (EndsStr(InteractiveMsgTag, S) &&
      (S.Length() >= 2 * InteractiveMsgTag.Length()))
  {
    Result = S.Length() - 2 * InteractiveMsgTag.Length() + 1;
    while ((Result > 0) && (S.SubString(Result, InteractiveMsgTag.Length()) != InteractiveMsgTag))
    {
      Result--;
    }
  }
  return Result;
}

UnicodeString RemoveMainInstructionsTag(const UnicodeString & S)
{
  UnicodeString Result = S;

  UnicodeString MainInstruction;
  if (ExtractMainInstructions(Result, MainInstruction))
  {
    Result = MainInstruction + Result;
  }
  return Result;
}

UnicodeString UnformatMessage(const UnicodeString & S)
{
  UnicodeString Result = RemoveMainInstructionsTag(S);

  intptr_t InteractiveMsgStart = FindInteractiveMsgStart(Result);
  if (InteractiveMsgStart > 0)
  {
    Result = Result.SubString(1, InteractiveMsgStart - 1);
  }
  return Result;
}

UnicodeString RemoveInteractiveMsgTag(const UnicodeString & S)
{
  UnicodeString Result = S;

  intptr_t InteractiveMsgStart = FindInteractiveMsgStart(Result);
  if (InteractiveMsgStart > 0)
  {
    UnicodeString InteractiveMsgTag = LoadStr(INTERACTIVE_MSG_TAG);
    Result.Delete(InteractiveMsgStart, InteractiveMsgTag.Length());
    Result.Delete(Result.Length() - InteractiveMsgTag.Length() + 1, InteractiveMsgTag.Length());
  }
  return Result;
}

UnicodeString RemoveEmptyLines(const UnicodeString & S)
{
  return
    ReplaceStr(
      ReplaceStr(S.TrimRight(), L"\n\n", L"\n"),
      L"\n \n", L"\n");
}

bool IsNumber(const UnicodeString & Str)
{
  int64_t Value = 0;
  if (Str == L"0")
    return true;
  return TryStrToInt(Str, Value);
}

UnicodeString GetSystemTemporaryDirectory()
{
#ifndef __linux__
  UnicodeString TempDir;
  TempDir.SetLength(NB_MAX_PATH);
  TempDir.SetLength(::GetTempPath(NB_MAX_PATH, const_cast<LPWSTR>(TempDir.c_str())));
  return TempDir;
#else
  return "/tmp";
#endif
}

UnicodeString GetShellFolderPath(int CSIdl)
{
  UnicodeString Result;
  wchar_t Path[2 * MAX_PATH + 10] = L"\0";
#ifndef __linux__
  if (SUCCEEDED(::SHGetFolderPath(nullptr, CSIdl, nullptr, SHGFP_TYPE_CURRENT, Path)))
  {
    Result = Path;
  }
#endif
  return Result;
}

// Particularly needed when using file name selected by TFilenameEdit,
// as it wraps a path to double-quotes, when there is a space in the path.
UnicodeString StripPathQuotes(const UnicodeString & APath)
{
  if ((APath.Length() >= 2) &&
      (APath[1] == L'\"') && (APath[APath.Length()] == L'\"'))
  {
    return APath.SubString(2, APath.Length() - 2);
  }
  else
  {
    return APath;
  }
}

UnicodeString AddQuotes(const UnicodeString & AStr)
{
  UnicodeString Result =  AStr;
  if (Result.Pos(L" ") > 0)
  {
    Result = L"\"" + Result + L"\"";
  }
  return Result;
}

UnicodeString AddPathQuotes(const UnicodeString & APath)
{
  UnicodeString Result = StripPathQuotes(APath);
  return AddQuotes(Result);
}

static wchar_t * ReplaceChar(
  UnicodeString & AFileName, wchar_t * InvalidChar, wchar_t InvalidCharsReplacement)
{
  intptr_t Index = InvalidChar - AFileName.c_str() + 1;
  if (InvalidCharsReplacement == TokenReplacement)
  {
    // currently we do not support unicode chars replacement
    if (AFileName[Index] > 0xFF)
    {
      ThrowExtException();
    }

    AFileName.Insert(ByteToHex(static_cast<uint8_t>(AFileName[Index])), Index + 1);
    AFileName[Index] = TokenPrefix;
    InvalidChar = const_cast<wchar_t *>(AFileName.c_str() + Index + 2);
  }
  else
  {
    AFileName[Index] = InvalidCharsReplacement;
    InvalidChar = const_cast<wchar_t *>(AFileName.c_str() + Index);
  }
  return InvalidChar;
}

UnicodeString ValidLocalFileName(const UnicodeString & AFileName)
{
  return ValidLocalFileName(AFileName, L'_', L"", LOCAL_INVALID_CHARS);
}

UnicodeString ValidLocalFileName(
  const UnicodeString & AFileName, wchar_t AInvalidCharsReplacement,
  const UnicodeString & ATokenizibleChars, const UnicodeString & ALocalInvalidChars)
{
  UnicodeString Result = AFileName;

  if (AInvalidCharsReplacement != NoReplacement)
  {
    bool ATokenReplacement = (AInvalidCharsReplacement == TokenReplacement);
    UnicodeString CharsStr = ATokenReplacement ? ATokenizibleChars : ALocalInvalidChars;
    const wchar_t * Chars =
      CharsStr.c_str();
    wchar_t * InvalidChar = const_cast<wchar_t *>(Result.c_str());
    while ((InvalidChar = wcspbrk(InvalidChar, Chars)) != nullptr)
    {
      intptr_t Pos = (InvalidChar - Result.c_str() + 1);
      wchar_t Char;
      if (ATokenReplacement &&
          (*InvalidChar == TokenPrefix) &&
          (((Result.Length() - Pos) <= 1) ||
           (((Char = static_cast<wchar_t>(HexToByte(Result.SubString(Pos + 1, 2)))) == L'\0') ||
            (ATokenizibleChars.Pos(Char) == 0))))
      {
        InvalidChar++;
      }
      else
      {
        InvalidChar = ReplaceChar(Result, InvalidChar, AInvalidCharsReplacement);
      }
    }

    // Windows trim trailing space or dot, hence we must encode it to preserve it
    if (!Result.IsEmpty() &&
        ((Result[Result.Length()] == L' ') ||
         (Result[Result.Length()] == L'.')))
    {
      ReplaceChar(Result, const_cast<wchar_t *>(Result.c_str() + Result.Length() - 1), AInvalidCharsReplacement);
    }

    if (IsReservedName(Result))
    {
      intptr_t P = Result.Pos(L".");
      if (P == 0)
      {
        P = Result.Length() + 1;
      }
      Result.Insert(L"%00", P);
    }
  }
  return Result;
}

void SplitCommand(const UnicodeString & Command, UnicodeString & Program,
  UnicodeString & Params, UnicodeString & Dir)
{
  UnicodeString Cmd = Command.Trim();
  Params.Clear();
  Dir.Clear();
  if (!Cmd.IsEmpty() && (Cmd[1] == L'\"'))
  {
    Cmd.Delete(1, 1);
    intptr_t P = Cmd.Pos(L'"');
    if (P)
    {
      Program = Cmd.SubString(1, P - 1).Trim();
      Params = Cmd.SubString(P + 1, Cmd.Length() - P).Trim();
    }
    else
    {
      throw Exception(FMTLOAD(INVALID_SHELL_COMMAND, UnicodeString(L"\"" + Cmd).c_str()));
    }
  }
  else
  {
    intptr_t P = Cmd.Pos(L" ");
    if (P)
    {
      Program = Cmd.SubString(1, P).Trim();
      Params = Cmd.SubString(P + 1, Cmd.Length() - P).Trim();
    }
    else
    {
      Program = Cmd;
    }
  }
  intptr_t B = Program.LastDelimiter(L"WGOOD_SLASH/");
  if (B)
  {
    Dir = Program.SubString(1, B).Trim();
  }
}

UnicodeString ExtractProgram(const UnicodeString & Command)
{
  UnicodeString Program;
  UnicodeString Params;
  UnicodeString Dir;

  SplitCommand(Command, Program, Params, Dir);

  return Program;
}

UnicodeString ExtractProgramName(const UnicodeString & Command)
{
  UnicodeString Name = base::ExtractFileName(ExtractProgram(Command), false);
  intptr_t Dot = Name.LastDelimiter(L".");
  if (Dot > 0)
  {
    Name = Name.SubString(1, Dot - 1);
  }
  return Name;
}

UnicodeString FormatCommand(const UnicodeString & Program, const UnicodeString & AParams)
{
  UnicodeString Result = Program.Trim();
  UnicodeString Params = AParams.Trim();
  if (!Params.IsEmpty())
    Params = L" " + Params;
  Result = AddQuotes(Result);
  return Result + Params;
}

const wchar_t ShellCommandFileNamePattern[] = L"!.!";

void ReformatFileNameCommand(UnicodeString & Command)
{
  if (!Command.IsEmpty())
  {
    UnicodeString Program, Params, Dir;
    SplitCommand(Command, Program, Params, Dir);
    if (Params.Pos(ShellCommandFileNamePattern) == 0)
    {
      Params = Params + (Params.IsEmpty() ? L"" : L" ") + ShellCommandFileNamePattern;
    }
    Command = FormatCommand(Program, Params);
  }
}

UnicodeString ExpandFileNameCommand(const UnicodeString & Command,
  const UnicodeString & AFileName)
{
  return AnsiReplaceStr(Command, ShellCommandFileNamePattern,
    AddPathQuotes(AFileName));
}

UnicodeString EscapeParam(const UnicodeString & AParam)
{
  // Make sure this won't break RTF syntax
  return ReplaceStr(AParam, L"\"", L"\"\"");
}

UnicodeString EscapePuttyCommandParam(const UnicodeString & Param)
{
  UnicodeString Result = Param;

  bool Space = false;

  for (intptr_t Index = 1; Index <= Result.Length(); ++Index)
  {
    switch (Result[Index])
    {
      case L'"':
        Result.Insert(L"\\", Index);
        ++Index;
        break;

      case L' ':
        Space = true;
        break;

      case LGOOD_SLASH:
        intptr_t I2 = Index;
        while ((I2 <= Result.Length()) && (Result[I2] == LGOOD_SLASH))
        {
          I2++;
        }
        if ((I2 <= Result.Length()) && (Result[I2] == L'"'))
        {
          while (Result[Index] == LGOOD_SLASH)
          {
            Result.Insert(L"\\", Index);
            Index += 2;
          }
          Index--;
        }
        break;
    }
  }

  if (Space)
  {
    Result = L"\"" + Result + L'"';
  }

  return Result;
}

UnicodeString ExpandEnvironmentVariables(const UnicodeString & Str)
{
#ifndef __linux__
  UnicodeString Buf;
  intptr_t Size = 1024;

  Buf.SetLength(Size);
  intptr_t Len = ::ExpandEnvironmentStringsW(Str.c_str(), const_cast<LPWSTR>(Buf.c_str()), static_cast<DWORD>(Size));

  if (Len > Size)
  {
    Buf.SetLength(Len);
    ::ExpandEnvironmentStringsW(Str.c_str(), const_cast<LPWSTR>(Buf.c_str()), static_cast<DWORD>(Len));
  }

  PackStr(Buf);

  return Buf;
#else
  return Str;
#endif
}

bool CompareFileName(const UnicodeString & APath1, const UnicodeString & APath2)
{
  UnicodeString ShortPath1 = ExtractShortPathName(APath1);
  UnicodeString ShortPath2 = ExtractShortPathName(APath2);

  bool Result;
  // ExtractShortPathName returns empty string if file does not exist
  if (ShortPath1.IsEmpty() || ShortPath2.IsEmpty())
  {
    Result = AnsiSameText(APath1, APath2);
  }
  else
  {
    Result = AnsiSameText(ShortPath1, ShortPath2);
  }
  return Result;
}

bool ComparePaths(const UnicodeString & APath1, const UnicodeString & APath2)
{
  TODO("ExpandUNCFileName");
  return AnsiSameText(::IncludeTrailingBackslash(APath1), ::IncludeTrailingBackslash(APath2));
}

intptr_t CompareLogicalText(const UnicodeString & S1, const UnicodeString & S2)
{
  if (S1.Length() > S2.Length())
  {
    return 1;
  }
  else if (S1.Length() < S2.Length())
  {
    return -1;
  }
  else
  {
#if defined(_MSC_VER)
    return ::StrCmpNCW(S1.c_str(), S2.c_str(), (int)S1.Length());
#else
    return S1.Compare(S2);
#endif
  }
}

bool IsReservedName(const UnicodeString & AFileName)
{
  UnicodeString FileName = AFileName;

  intptr_t P = FileName.Pos(L".");
  intptr_t Len = (P > 0) ? P - 1 : FileName.Length();
  if ((Len == 3) || (Len == 4))
  {
    if (P > 0)
    {
      FileName.SetLength(P - 1);
    }
    static UnicodeString Reserved[] =
    {
      "CON", "PRN", "AUX", "NUL",
      "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
      "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(_countof(Reserved)); ++Index)
    {
      if (SameText(FileName, Reserved[Index]))
      {
        return true;
      }
    }
  }
  return false;
}

// ApiPath support functions
// Inspired by
// http://stackoverflow.com/questions/18580945/need-clarification-for-converting-paths-into-long-unicode-paths-or-the-ones-star
// This can be reimplemented using PathCchCanonicalizeEx on Windows 8 and later
enum PATH_PREFIX_TYPE
{
  PPT_UNKNOWN,
  PPT_ABSOLUTE,           //Found absolute path that is none of the other types
  PPT_UNC,                //Found \\server\share\ prefix
  PPT_LONG_UNICODE,       //Found \\?\ prefix
  PPT_LONG_UNICODE_UNC,   //Found \\?\UNC\ prefix
};

static intptr_t PathRootLength(const UnicodeString & APath)
{
#ifndef __linux__
  // Correction for PathSkipRoot API

  // Replace all /'s with \'s because PathSkipRoot can't handle /'s
  UnicodeString Result = ReplaceChar(APath, LOTHER_SLASH, LGOOD_SLASH);

  // Now call the API
  LPCTSTR Buffer = ::PathSkipRoot(Result.c_str());

  return (Buffer != nullptr) ? (Buffer - Result.c_str()) : -1;
#else
  return 0;
#endif
}

static bool PathIsRelative_CorrectedForMicrosoftStupidity(const UnicodeString & APath)
{
#ifndef __linux__
  // Correction for PathIsRelative API

  // Replace all /'s with \'s because PathIsRelative can't handle /'s
  UnicodeString Result = ReplaceChar(APath, LOTHER_SLASH, LGOOD_SLASH);

  //Now call the API
  return ::PathIsRelative(Result.c_str()) != FALSE;
#else
  return false;
#endif
}

static intptr_t GetOffsetAfterPathRoot(const UnicodeString & APath, PATH_PREFIX_TYPE & PrefixType)
{
  // Checks if 'pPath' begins with the drive, share, prefix, etc
  // EXAMPLES:
  //    Path                          Return:   Points at:                 PrefixType:
  //   Relative\Folder\File.txt        0         Relative\Folder\File.txt   PPT_UNKNOWN
  //   \RelativeToRoot\Folder          1         RelativeToRoot\Folder      PPT_ABSOLUTE
  //   C:\Windows\Folder               3         Windows\Folder             PPT_ABSOLUTE
  //   \\server\share\Desktop          15        Desktop                    PPT_UNC
  //   \\?\C:\Windows\Folder           7         Windows\Folder             PPT_LONG_UNICODE
  //   \\?\UNC\server\share\Desktop    21        Desktop                    PPT_LONG_UNICODE_UNC
  // RETURN:
  //      = Index in 'pPath' after the root, or
  //      = 0 if no root was found
  intptr_t Result = 0;

  PrefixType = PPT_UNKNOWN;

  if (!APath.IsEmpty())
  {
    intptr_t Len = APath.Length();

    bool WinXPOnly = !IsWinVista();

    // The PathSkipRoot() API doesn't work correctly on Windows XP
    if (!WinXPOnly)
    {
      // Works since Vista and up, but still needs correction :)
      intptr_t RootLength = PathRootLength(APath);
      if (RootLength >= 0)
      {
        Result = RootLength + 1;
      }
    }

    // Now determine the type of prefix
    intptr_t IndCheckUNC = -1;

    if ((Len >= 8) &&
        (APath[1] == LGOOD_SLASH || APath[1] == LOTHER_SLASH) &&
        (APath[2] == LGOOD_SLASH || APath[2] == LOTHER_SLASH) &&
        (APath[3] == L'?') &&
        (APath[4] == LGOOD_SLASH || APath[4] == LOTHER_SLASH) &&
        (APath[5] == L'U' || APath[5] == L'u') &&
        (APath[6] == L'N' || APath[6] == L'n') &&
        (APath[7] == L'C' || APath[7] == L'c') &&
        (APath[8] == LGOOD_SLASH || APath[8] == LOTHER_SLASH))
    {
      // Found \\?\UNC\ prefix
      PrefixType = PPT_LONG_UNICODE_UNC;

      if (WinXPOnly)
      {
          //For older OS
          Result += 8;
      }

      //Check for UNC share later
      IndCheckUNC = 8;
    }
    else if ((Len >= 4) &&
        (APath[1] == LGOOD_SLASH || APath[1] == LOTHER_SLASH) &&
        (APath[2] == LGOOD_SLASH || APath[2] == LOTHER_SLASH) &&
        (APath[3] == L'?') &&
        (APath[4] == LGOOD_SLASH || APath[4] == LOTHER_SLASH))
    {
      // Found \\?\ prefix
      PrefixType = PPT_LONG_UNICODE;

      if (WinXPOnly)
      {
          //For older OS
          Result += 4;
      }
    }
    else if ((Len >= 2) &&
        (APath[1] == LGOOD_SLASH || APath[1] == LOTHER_SLASH) &&
        (APath[2] == LGOOD_SLASH || APath[2] == LOTHER_SLASH))
    {
      // Check for UNC share later
      IndCheckUNC = 2;
    }

    if (IndCheckUNC >= 0)
    {
      // Check for UNC, i.e. \\server\share\ part
      intptr_t Index = IndCheckUNC;
      for (int SkipSlashes = 2; SkipSlashes > 0; SkipSlashes--)
      {
        for(; Index <= Len; ++Index)
        {
          TCHAR z = APath[Index];
          if ((z == LGOOD_SLASH) || (z == LOTHER_SLASH) || (Index >= Len))
          {
            ++Index;
            if (SkipSlashes == 1)
            {
              if (PrefixType == PPT_UNKNOWN)
              {
                PrefixType = PPT_UNC;
              }

              if (WinXPOnly)
              {
                  //For older OS
                  Result = Index;
              }
            }

            break;
          }
        }
      }
    }

    if (WinXPOnly)
    {
      // Only if we didn't determine any other type
      if (PrefixType == PPT_UNKNOWN)
      {
        if (!PathIsRelative_CorrectedForMicrosoftStupidity(APath.SubString(Result, APath.Length() - Result + 1)))
        {
          PrefixType = PPT_ABSOLUTE;
        }
      }

      // For older OS only
      intptr_t RootLength = PathRootLength(APath.SubString(Result, APath.Length() - Result + 1));
      if (RootLength >= 0)
      {
        Result = RootLength + 1;
      }
    }
    else
    {
      // Only if we didn't determine any other type
      if (PrefixType == PPT_UNKNOWN)
      {
        if (!PathIsRelative_CorrectedForMicrosoftStupidity(APath))
        {
          PrefixType = PPT_ABSOLUTE;
        }
      }
    }
  }

  return Result;
}

static UnicodeString MakeUnicodeLargePath(const UnicodeString & APath)
{
  // Convert path from 'into a larger Unicode path, that allows up to 32,767 character length
  UnicodeString Result;

  if (!APath.IsEmpty())
  {
    // Determine the type of the existing prefix
    PATH_PREFIX_TYPE PrefixType;
    GetOffsetAfterPathRoot(APath, PrefixType);

    // Assume path to be without change
    Result = APath;

    switch (PrefixType)
    {
      case PPT_ABSOLUTE:
      {
        // First we need to check if its an absolute path relative to the root
        bool AddPrefix = true;
        if ((APath.Length() >= 1) &&
            ((APath[1] == LGOOD_SLASH) || (APath[1] == LOTHER_SLASH)))
        {
          AddPrefix = FALSE;

          // Get current root path
          UnicodeString CurrentDir = GetCurrentDir();
          PATH_PREFIX_TYPE PrefixType2; // unused
          intptr_t Following = GetOffsetAfterPathRoot(CurrentDir, PrefixType2);
          if (Following > 0)
          {
            AddPrefix = true;
            Result = CurrentDir.SubString(1, Following - 1) + Result.SubString(2, Result.Length() - 1);
          }
        }

        if (AddPrefix)
        {
          // Add \\?\ prefix
          Result = L"\\\\?\\" + Result;
        }
      }
      break;

      case PPT_UNC:
        // First we need to remove the opening slashes for UNC share
        if ((Result.Length() >= 2) &&
            ((Result[1] == LGOOD_SLASH) || (Result[1] == LOTHER_SLASH)) &&
            ((Result[2] == LGOOD_SLASH) || (Result[2] == LOTHER_SLASH)))
        {
          Result = Result.SubString(3, Result.Length() - 2);
        }

        // Add \\?\UNC\ prefix
        Result = L"\\\\?\\UNC\\" + Result;
        break;

      case PPT_LONG_UNICODE:
      case PPT_LONG_UNICODE_UNC:
        // nothing to do
        break;
    }

  }

  return Result;
}

UnicodeString ApiPath(const UnicodeString & APath)
{
  UnicodeString Result = APath;

  if (IsWin7() || (Result.Length() >= MAX_PATH))
  {
//    if (GetConfiguration() != nullptr)
//    {
//      GetConfiguration()->Usage->Inc(L"LongPath");
//    }
    Result = MakeUnicodeLargePath(Result);
  }
  return Result;
}

UnicodeString DisplayableStr(const RawByteString & Str)
{
  bool Displayable = true;
  intptr_t Index1 = 1;
  while ((Index1 <= Str.Length()) && Displayable)
  {
    if (((Str[Index1] < '\x20') || (static_cast<uint8_t>(Str[Index1]) >= static_cast<uint8_t >('\x80'))) &&
        (Str[Index1] != '\n') && (Str[Index1] != '\r') && (Str[Index1] != '\t') && (Str[Index1] != '\b'))
    {
      Displayable = false;
    }
    ++Index1;
  }

  UnicodeString Result;
  if (Displayable)
  {
    Result = L"\"";
    for (intptr_t Index2 = 1; Index2 <= Str.Length(); ++Index2)
    {
      switch (Str[Index2])
      {
        case '\n':
          Result += L"\\n";
          break;

        case '\r':
          Result += L"\\r";
          break;

        case '\t':
          Result += L"\\t";
          break;

        case '\b':
          Result += L"\\b";
          break;

        case '\\':
          Result += L"\\\\";
          break;

        case '"':
          Result += L"\\\"";
          break;

        default:
          Result += wchar_t(Str[Index2]);
          break;
      }
    }
    Result += L"\"";
  }
  else
  {
    Result = L"0x" + BytesToHex(Str);
  }
  return Result;
}

UnicodeString ByteToHex(uint8_t B, bool UpperCase)
{
  static wchar_t UpperDigits[] = L"0123456789ABCDEF";
  static wchar_t LowerDigits[] = L"0123456789abcdef";

  const wchar_t * Digits = (UpperCase ? UpperDigits : LowerDigits);
  UnicodeString Result;
  Result.SetLength(2);
  Result[1] = Digits[(B & 0xF0) >> 4];
  Result[2] = Digits[(B & 0x0F) >> 0];
  return Result;
}

UnicodeString BytesToHex(const uint8_t * B, uintptr_t Length, bool UpperCase, wchar_t Separator)
{
  UnicodeString Result;
  for (uintptr_t Index = 0; Index < Length; ++Index)
  {
    Result += ByteToHex(B[Index], UpperCase);
    if ((Separator != L'\0') && (Index < Length - 1))
    {
      Result += Separator;
    }
  }
  return Result;
}

UnicodeString BytesToHex(const RawByteString & Str, bool UpperCase, wchar_t Separator)
{
  return BytesToHex(reinterpret_cast<const uint8_t *>(Str.c_str()), Str.Length(), UpperCase, Separator);
}

UnicodeString CharToHex(wchar_t Ch, bool UpperCase)
{
  return BytesToHex(reinterpret_cast<const uint8_t *>(&Ch), sizeof(Ch), UpperCase);
}

RawByteString HexToBytes(const UnicodeString & Hex)
{
  UnicodeString Digits = "0123456789ABCDEF";
  RawByteString Result;
  intptr_t L = Hex.Length();
  if (L % 2 == 0)
  {
    for (intptr_t Index = 1; Index <= Hex.Length(); Index += 2)
    {
      intptr_t P1 = Digits.Pos(::UpCase(Hex[Index]));
      intptr_t P2 = Digits.Pos(::UpCase(Hex[Index + 1]));
      if (P1 <= 0 || P2 <= 0)
      {
        Result.Clear();
        break;
      }
      else
      {
        Result += static_cast<int8_t>((P1 - 1) * 16 + P2 - 1);
      }
    }
  }
  return Result;
}

uint8_t HexToByte(const UnicodeString & Hex)
{
  static UnicodeString Digits = "0123456789ABCDEF";
  DebugAssert(Hex.Length() == 2);
  intptr_t P1 = Digits.Pos(::UpCase(Hex[1]));
  intptr_t P2 = Digits.Pos(::UpCase(Hex[2]));

  return
    static_cast<uint8_t>(((P1 <= 0) || (P2 <= 0)) ? 0 : (((P1 - 1) << 4) + (P2 - 1)));
}

bool IsLowerCaseLetter(wchar_t Ch)
{
  return (Ch >= L'a') && (Ch <= L'z');
}

bool IsUpperCaseLetter(wchar_t Ch)
{
  return (Ch >= L'A') && (Ch <= L'Z');
}

bool IsLetter(wchar_t Ch)
{
  return IsLowerCaseLetter(Ch) || IsUpperCaseLetter(Ch);
}

bool IsDigit(wchar_t Ch)
{
  return (Ch >= L'0') && (Ch <= L'9');
}

bool IsHex(wchar_t Ch)
{
  return
    IsDigit(Ch) ||
    ((Ch >= L'A') && (Ch <= L'F')) ||
    ((Ch >= L'a') && (Ch <= L'f'));
}

DWORD FindCheck(DWORD Result, const UnicodeString & APath)
{
  if ((Result != ERROR_SUCCESS) &&
      (Result != ERROR_FILE_NOT_FOUND) &&
      (Result != ERROR_NO_MORE_FILES))
  {
    throw EOSExtException(FMTLOAD(FIND_FILE_ERROR, APath.c_str()), Result);
  }
  return Result;
}

DWORD FindFirstUnchecked(const UnicodeString & APath, DWORD Attr, TSearchRecChecked & F)
{
  F.Path = APath;
  return FindFirst(ApiPath(APath), Attr, F);
}

DWORD FindFirstChecked(const UnicodeString & APath, DWORD LocalFileAttrs, TSearchRecChecked & F)
{
  // return FindCheck(FindFirst(Path, LocalFileAttrs, F));
  DWORD Result = FindFirstUnchecked(APath, LocalFileAttrs, F);
  return FindCheck(Result, F.Path);
}

// Equivalent to FindNext, just to complement to FindFirstUnchecked
DWORD FindNextUnchecked(TSearchRecChecked & F)
{
  return FindNext(F);
}

// It can make sense to use FindNextChecked, even if unchecked FindFirst is used.
// I.e. even if we do not care that FindFirst failed, if FindNext
// fails after successful FindFirst, it mean some terrible problem
DWORD FindNextChecked(TSearchRecChecked & F)
{
  return FindCheck(FindNextUnchecked(F), F.Path);
}

bool FileSearchRec(const UnicodeString & AFileName, TSearchRec & Rec)
{
  DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  bool Result = (FindFirst(ApiPath(AFileName), FindAttrs, Rec) == 0);
  if (Result)
  {
    FindClose(Rec);
  }
  return Result;
}

void ProcessLocalDirectory(const UnicodeString & ADirName,
  TProcessLocalFileEvent CallBackFunc, void * Param,
  DWORD FindAttrs)
{
  DebugAssert(CallBackFunc);
  if (FindAttrs == INVALID_FILE_ATTRIBUTES)
  {
    FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  }
  TSearchRecChecked SearchRec;

  UnicodeString DirName = ApiPath(::IncludeTrailingBackslash(ADirName));
  if (FindFirstChecked(DirName + L"*.*", FindAttrs, SearchRec) == 0)
  {
    SCOPE_EXIT
    {
      FindClose(SearchRec);
    };
    do
    {
      if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
      {
        UnicodeString FileName = DirName + SearchRec.Name;
        CallBackFunc(FileName, SearchRec, Param);
      }

    }
    while (FindNextChecked(SearchRec) == 0);
  }
}

DWORD FileGetAttrFix(const UnicodeString & FileName)
{
  // The default for FileGetAttr is to follow links
  bool FollowLink = true;
  // But the FileGetAttr whe called for link with FollowLink set will always fail
  // as its calls InternalGetFileNameFromSymLink, which test for CheckWin32Version(6, 0)
  if (!IsWinVista())
  {
    FollowLink = false;
  }
  return ::FileGetAttr(FileName, FollowLink);
}

TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day)
{
  TDateTime Result;
  try
  {
    Result = EncodeDate(Year, Month, Day);
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%04u-%02u-%02u]", E.Message.c_str(), int(Year), int(Month), int(Day)));
  }
  return Result;
}

TDateTime EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec)
{
  TDateTime Result;
  try
  {
    Result = EncodeTime(Hour, Min, Sec, MSec);
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%02u:%02u:%02u.%04u]", E.Message.c_str(), int(Hour), int(Min), int(Sec), int(MSec)));
  }
  return Result;
}

TDateTime SystemTimeToDateTimeVerbose(const SYSTEMTIME & SystemTime)
{
  try
  {
    TDateTime DateTime = SystemTimeToDateTime(SystemTime);
    return DateTime;
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%3.3d]", E.Message.c_str(), int(SystemTime.wYear), int(SystemTime.wMonth), int(SystemTime.wDay), int(SystemTime.wHour), int(SystemTime.wMinute), int(SystemTime.wSecond), int(SystemTime.wMilliseconds)));
  }
}

struct TDateTimeParams : public TObject
{
  TDateTimeParams() :
    BaseDifference(0.0),
    BaseDifferenceSec(0),
    CurrentDaylightDifference(0.0),
    CurrentDaylightDifferenceSec(0),
    CurrentDifference(0.0),
    CurrentDifferenceSec(0),
    StandardDifference(0.0),
    StandardDifferenceSec(0),
    DaylightDifference(0.0),
    DaylightDifferenceSec(0),
    DaylightHack(false)
  {
    ClearStruct(SystemStandardDate);
    ClearStruct(SystemDaylightDate);
  }
  TDateTime UnixEpoch;
  double BaseDifference;
  intptr_t BaseDifferenceSec;
  // All Current* are actually global, not per-year
  // are valid for Year 0 (current) only
  double CurrentDaylightDifference;
  intptr_t CurrentDaylightDifferenceSec;
  double CurrentDifference;
  intptr_t CurrentDifferenceSec;
  double StandardDifference;
  intptr_t StandardDifferenceSec;
  double DaylightDifference;
  intptr_t DaylightDifferenceSec;
  SYSTEMTIME SystemStandardDate;
  SYSTEMTIME SystemDaylightDate;
  TDateTime StandardDate;
  TDateTime DaylightDate;
  UnicodeString StandardName;
  UnicodeString DaylightName;
  // This is actually global, not per-year
  bool DaylightHack;

  bool HasDST() const
  {
    // On some systems it occurs that StandardDate is unset, while
    // DaylightDate is set. MSDN states that this is invalid and
    // should be treated as if there is no daylight saving.
    // So check both.
    return
      (SystemStandardDate.wMonth != 0) &&
      (SystemDaylightDate.wMonth != 0);
  }

  bool SummerDST() const
  {
    return HasDST() && (DaylightDate < StandardDate);
  }
};

typedef std::map<int, TDateTimeParams> TYearlyDateTimeParams;
static TYearlyDateTimeParams YearlyDateTimeParams;
static TCriticalSection DateTimeParamsSection;
static void EncodeDSTMargin(const SYSTEMTIME & Date, uint16_t Year,
  TDateTime & Result);

static uint16_t DecodeYear(const TDateTime & DateTime)
{
  uint16_t Year, Month, Day;
  DecodeDate(DateTime, Year, Month, Day);
  return Year;
}

static const TDateTimeParams * GetDateTimeParams(uint16_t Year)
{
  TGuard Guard(DateTimeParamsSection);

  TDateTimeParams * Result;

  TYearlyDateTimeParams::iterator it = YearlyDateTimeParams.find(Year);
  if (it != YearlyDateTimeParams.end())
  {
    Result = &(*it).second;
  }
  else
  {
    // creates new entry as a side effect
    Result = &YearlyDateTimeParams[Year];
#ifndef __linux__
    TIME_ZONE_INFORMATION TZI;

    uint32_t GTZI;

    HINSTANCE Kernel32 = ::GetModuleHandle(L"kernel32.dll");
    typedef BOOL (WINAPI * TGetTimeZoneInformationForYear)(USHORT wYear, PDYNAMIC_TIME_ZONE_INFORMATION pdtzi, LPTIME_ZONE_INFORMATION ptzi);
    TGetTimeZoneInformationForYear GetTimeZoneInformationForYear =
      (TGetTimeZoneInformationForYear)::GetProcAddress(Kernel32, "GetTimeZoneInformationForYear");

    if ((Year == 0) || (GetTimeZoneInformationForYear == nullptr))
    {
      GTZI = GetTimeZoneInformation(&TZI);
    }
    else
    {
      GetTimeZoneInformationForYear(Year, nullptr, &TZI);
      GTZI = TIME_ZONE_ID_UNKNOWN;
    }

    switch (GTZI)
    {
      case TIME_ZONE_ID_UNKNOWN:
        Result->CurrentDaylightDifferenceSec = 0;
        break;

      case TIME_ZONE_ID_STANDARD:
        Result->CurrentDaylightDifferenceSec = TZI.StandardBias;
        break;

      case TIME_ZONE_ID_DAYLIGHT:
        Result->CurrentDaylightDifferenceSec = TZI.DaylightBias;
        break;

      case TIME_ZONE_ID_INVALID:
      default:
        throw Exception(FMTLOAD(TIMEZONE_ERROR));
    }

    Result->BaseDifferenceSec = TZI.Bias;
    Result->BaseDifference = (double)(TZI.Bias) / MinsPerDay;
    Result->BaseDifferenceSec *= SecsPerMin;

    Result->CurrentDifferenceSec = TZI.Bias +
      Result->CurrentDaylightDifferenceSec;
    Result->CurrentDifference =
      (double)(Result->CurrentDifferenceSec) / MinsPerDay;
    Result->CurrentDifferenceSec *= SecsPerMin;

    Result->CurrentDaylightDifference =
      (double)(Result->CurrentDaylightDifferenceSec) / MinsPerDay;
    Result->CurrentDaylightDifferenceSec *= SecsPerMin;

    Result->DaylightDifferenceSec = TZI.DaylightBias * SecsPerMin;
    Result->DaylightDifference = (double)(TZI.DaylightBias) / MinsPerDay;
    Result->StandardDifferenceSec = TZI.StandardBias * SecsPerMin;
    Result->StandardDifference = (double)(TZI.StandardBias) / MinsPerDay;

    Result->SystemStandardDate = TZI.StandardDate;
    Result->SystemDaylightDate = TZI.DaylightDate;

    uint16_t AYear = (Year != 0) ? Year : DecodeYear(Now());
    if (Result->HasDST())
    {
      EncodeDSTMargin(Result->SystemStandardDate, AYear, Result->StandardDate);
      EncodeDSTMargin(Result->SystemDaylightDate, AYear, Result->DaylightDate);
    }

    Result->StandardName = TZI.StandardName;
    Result->DaylightName = TZI.DaylightName;

    Result->DaylightHack = !IsWin7();
#endif
  }

  return Result;
}

static void EncodeDSTMargin(const SYSTEMTIME & Date, uint16_t Year,
  TDateTime & Result)
{
  if (Date.wYear == 0)
  {
    TDateTime Temp = EncodeDateVerbose(Year, Date.wMonth, 1);
    Result = Temp + ((Date.wDayOfWeek - DayOfWeek(Temp) + 8) % 7) +
      (7 * (Date.wDay - 1));
    // Day 5 means, the last occurrence of day-of-week in month
    if (Date.wDay == 5)
    {
      uint16_t Month = static_cast<uint16_t>(Date.wMonth + 1);
      if (Month > 12)
      {
        Month = static_cast<uint16_t>(Month - 12);
        Year++;
      }

      if (Result >= EncodeDateVerbose(Year, Month, 1))
      {
        Result -= 7;
      }
    }
    Result += EncodeTimeVerbose(Date.wHour, Date.wMinute, Date.wSecond,
      Date.wMilliseconds);
  }
  else
  {
    Result = EncodeDateVerbose(Year, Date.wMonth, Date.wDay) +
      EncodeTimeVerbose(Date.wHour, Date.wMinute, Date.wSecond, Date.wMilliseconds);
  }
}

static bool IsDateInDST(const TDateTime & DateTime)
{

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));

  bool Result;

  // On some systems it occurs that StandardDate is unset, while
  // DaylightDate is set. MSDN states that this is invalid and
  // should be treated as if there is no daylight saving.
  // So check both.
  if (!Params->HasDST())
  {
    Result = false;
  }
  else
  {

    if (Params->SummerDST())
    {
      Result =
        (DateTime >= Params->DaylightDate) &&
        (DateTime < Params->StandardDate);
    }
    else
    {
      Result =
        (DateTime < Params->StandardDate) ||
        (DateTime >= Params->DaylightDate);
    }
  }
  return Result;
}

bool UsesDaylightHack()
{
  return GetDateTimeParams(0)->DaylightHack;
}

TDateTime UnixToDateTime(int64_t TimeStamp, TDSTMode DSTMode)
{
  DebugAssert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  TDateTime Result = TDateTime(UnixDateDelta + ((double)(TimeStamp) / SecsPerDay));
  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
      Result -= CurrentParams->CurrentDifference;
    }
    else if (DSTMode == dstmKeep)
    {
      Result -= Params->BaseDifference;
    }
  }
  else
  {
    Result -= Params->BaseDifference;
  }

  if ((DSTMode == dstmUnix) || (DSTMode == dstmKeep))
  {
    Result -= DSTDifferenceForTime(Result);
  }

  return Result;
}

int64_t Round(double Number)
{
  double Floor = floor(Number);
  double Ceil = ceil(Number);
  return static_cast<int64_t>(((Number - Floor) > (Ceil - Number)) ? Ceil : Floor);
}

bool TryRelativeStrToDateTime(const UnicodeString & AStr, TDateTime & DateTime)
{
  UnicodeString S = AStr.Trim();
  intptr_t Index = 1;
  while ((Index <= S.Length()) && IsDigit(S[Index]))
  {
    ++Index;
  }
  UnicodeString NumberStr = S.SubString(1, Index - 1);
  int64_t Number = 0;
  bool Result = TryStrToInt(NumberStr, Number);
  if (Result)
  {
    S.Delete(1, Index - 1);
    S = S.Trim().UpperCase();
    DateTime = Now();
    // These may not overlap with ParseSize (K, M and G)
    if (S == "S")
    {
      DateTime = IncSecond(DateTime, -Number);
    }
    else if (S == "N")
    {
      DateTime = IncMinute(DateTime, -Number);
    }
    else if (S == "H")
    {
      DateTime = IncHour(DateTime, -Number);
    }
    else if (S == "D")
    {
      DateTime = IncDay(DateTime, -Number);
    }
    else if (S == "Y")
    {
      DateTime = IncYear(DateTime, -Number);
    }
    else
    {
      Result = false;
    }
  }
  return Result;
}

static int64_t DateTimeToUnix(const TDateTime & DateTime)
{
  const TDateTimeParams * CurrentParams = GetDateTimeParams(0);

  DebugAssert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  return Round((double)(DateTime - UnixDateDelta) * SecsPerDay) +
    CurrentParams->CurrentDifferenceSec;
}

FILETIME DateTimeToFileTime(const TDateTime & DateTime,
  TDSTMode /*DSTMode*/)
{
  int64_t UnixTimeStamp = ::DateTimeToUnix(DateTime);

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
  if (!Params->DaylightHack)
  {
    // We should probably use reversed code of FileTimeToDateTime here instead of custom implementation

    // We are incrementing and decrementing BaseDifferenceSec because it
    // can actually change between years
    // (as it did in Belarus from GMT+2 to GMT+3 between 2011 and 2012)

    UnixTimeStamp += (IsDateInDST(DateTime) ?
      Params->DaylightDifferenceSec : Params->StandardDifferenceSec) +
      Params->BaseDifferenceSec;

    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    UnixTimeStamp -=
      CurrentParams->CurrentDaylightDifferenceSec +
      CurrentParams->BaseDifferenceSec;

  }

  FILETIME Result;
  (*(int64_t*)&(Result) = ((int64_t)(UnixTimeStamp) + 11644473600LL) * 10000000LL);

  return Result;
}

TDateTime FileTimeToDateTime(const FILETIME & FileTime)
{
  // duplicated in DirView.pas
  TDateTime Result;
  // The 0xFFF... is sometime seen for invalid timestamps,
  // it would cause failure in SystemTimeToDateTime below
  if (FileTime.dwLowDateTime == DWORD(-1) / sizeof(DWORD)) //std::numeric_limits<DWORD>::max) //
  {
    Result = MinDateTime;
  }
  else
  {
    SYSTEMTIME SysTime;
#ifndef __linux__
    if (!UsesDaylightHack())
    {
      SYSTEMTIME UniversalSysTime;
      FileTimeToSystemTime(&FileTime, &UniversalSysTime);
      SystemTimeToTzSpecificLocalTime(nullptr, &UniversalSysTime, &SysTime);
    }
    else
#endif
    {
      FILETIME LocalFileTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SysTime);
    }
    Result = SystemTimeToDateTimeVerbose(SysTime);
  }
/*
  SYSTEMTIME SysTime;
  if (!UsesDaylightHack())
  {
    SYSTEMTIME UniverzalSysTime;
    FileTimeToSystemTime(&FileTime, &UniverzalSysTime);
    SystemTimeToTzSpecificLocalTime(nullptr, &UniverzalSysTime, &SysTime);
  }
  else
  {
    FILETIME LocalFileTime;
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SysTime);
  }
  TDateTime Result = SystemTimeToDateTime(SysTime);
*/
  return Result;
}

int64_t ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode)
{
  int64_t Result = ((*(int64_t *) & (FileTime)) / 10000000LL - 11644473600LL);
  if (UsesDaylightHack())
  {
    if ((DSTMode == dstmUnix) || (DSTMode == dstmKeep))
    {
      FILETIME LocalFileTime;
      SYSTEMTIME SystemTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SystemTime);
      TDateTime DateTime = SystemTimeToDateTimeVerbose(SystemTime);
      const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
      Result += (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);

      if (DSTMode == dstmKeep)
      {
        const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
        Result -= CurrentParams->CurrentDaylightDifferenceSec;
      }
    }
  }
  else
  {
    if (DSTMode == dstmWin)
    {
      FILETIME LocalFileTime;
      SYSTEMTIME SystemTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SystemTime);
      TDateTime DateTime = SystemTimeToDateTimeVerbose(SystemTime);
      const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
      Result -= (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);
    }
  }

  return Result;
}

TDateTime ConvertTimestampToUTC(const TDateTime & DateTime)
{
  TDateTime Result = DateTime;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
  Result += DSTDifferenceForTime(DateTime);
  Result += Params->BaseDifference;

  if (Params->DaylightHack)
  {
    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    Result += CurrentParams->CurrentDaylightDifference;
  }

  return Result;
}

TDateTime ConvertTimestampFromUTC(const TDateTime & DateTime)
{
  TDateTime Result = DateTime;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));
  Result -= DSTDifferenceForTime(DateTime);
  Result -= Params->BaseDifference;

  if (Params->DaylightHack)
  {
    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    Result -= CurrentParams->CurrentDaylightDifference;
  }

  return Result;
}

int64_t ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode)
{
  int64_t Result;
  if ((FileTime.dwLowDateTime == 0) &&
      (FileTime.dwHighDateTime == 0))
  {
    Result = ::DateTimeToUnix(Now());
  }
  else
  {
    Result = ::ConvertTimestampToUnix(FileTime, DSTMode);
  }
  return Result;
}

double DSTDifferenceForTime(const TDateTime & DateTime)
{
  double Result;
  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
  if (IsDateInDST(DateTime))
  {
    Result = Params->DaylightDifference;
  }
  else
  {
    Result = Params->StandardDifference;
  }
  return Result;
}

TDateTime AdjustDateTimeFromUnix(const TDateTime & DateTime, TDSTMode DSTMode)
{
  TDateTime Result = DateTime;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
      Result = Result - CurrentParams->CurrentDaylightDifference;
    }

    if (!IsDateInDST(Result))
    {
      if (DSTMode == dstmWin)
      {
        Result = Result - Params->DaylightDifference;
      }
    }
    else
    {
      Result = Result - Params->StandardDifference;
    }
  }
  else
  {
    if (DSTMode == dstmWin)
    {
      Result = Result + DSTDifferenceForTime(Result);
    }
  }

  return Result;
}

UnicodeString FixedLenDateTimeFormat(const UnicodeString & Format)
{
  UnicodeString Result = Format;
  bool AsIs = false;

  intptr_t Index = 1;
  while (Index <= Result.Length())
  {
    wchar_t F = Result[Index];
    if ((F == L'\'') || (F == L'\"'))
    {
      AsIs = !AsIs;
      ++Index;
    }
    else if (!AsIs && ((F == L'a') || (F == L'A')))
    {
      if (Result.SubString(Index, 5).LowerCase() == L"am/pm")
      {
        Index += 5;
      }
      else if (Result.SubString(Index, 3).LowerCase() == L"a/p")
      {
        Index += 3;
      }
      else if (Result.SubString(Index, 4).LowerCase() == L"ampm")
      {
        Index += 4;
      }
      else
      {
        ++Index;
      }
    }
    else
    {
      if (!AsIs && (wcschr(L"dDeEmMhHnNsS", F) != nullptr) &&
          ((Index == Result.Length()) || (Result[Index + 1] != F)))
      {
        Result.Insert(F, Index);
      }

      while ((Index <= Result.Length()) && (F == Result[Index]))
      {
        ++Index;
      }
    }
  }

  return Result;
}

UnicodeString FormatTimeZone(intptr_t Sec)
{
  UnicodeString Str;
  TODO("implement class TTimeSpan");
  /*
  TTimeSpan Span = TTimeSpan::FromSeconds(Sec);
  if ((Span.Seconds == 0) && (Span.Minutes == 0))
  {
    Str = FORMAT(L"%d", -Span.Hours);
  }
  else if (Span.Seconds == 0)
  {
    Str = FORMAT(L"%d:%2.2d", -Span.Hours, abs(Span.Minutes));
  }
  else
  {
    Str = FORMAT(L"%d:%2.2d:%2.2d", -Span.Hours, abs(Span.Minutes), abs(Span.Seconds));
  }
  Str = ((Span <= TTimeSpan::Zero) ? L"+" : L"") + Str;*/
  return Str;
}

UnicodeString GetTimeZoneLogString()
{
  const TDateTimeParams * CurrentParams = GetDateTimeParams(0);

  UnicodeString Result =
    FORMAT(L"Current: GMT%s", FormatTimeZone(CurrentParams->CurrentDifferenceSec).c_str());

  if (!CurrentParams->HasDST())
  {
    Result += FORMAT(L" (%s), No DST", CurrentParams->StandardName.c_str());
  }
  else
  {
    Result +=
      FORMAT(L", Standard: GMT%s (%s), DST: GMT%s (%s), DST Start: %s, DST End: %s",
        FormatTimeZone(CurrentParams->BaseDifferenceSec + CurrentParams->StandardDifferenceSec).c_str(),
         CurrentParams->StandardName.c_str(),
         FormatTimeZone(CurrentParams->BaseDifferenceSec + CurrentParams->DaylightDifferenceSec).c_str(),
         CurrentParams->DaylightName.c_str(),
         CurrentParams->DaylightDate.DateString().c_str(),
         CurrentParams->StandardDate.DateString().c_str());
  }

  return Result;
}

bool AdjustClockForDSTEnabled()
{
  // Windows XP deletes the DisableAutoDaylightTimeSet value when it is off
  // (the later versions set it to DynamicDaylightTimeDisabled to 0)
  bool DynamicDaylightTimeDisabled = false;
  std::unique_ptr<TRegistry> Registry(new TRegistry());
  Registry->SetAccess(KEY_READ);
  try
  {
    Registry->SetRootKey(HKEY_LOCAL_MACHINE);
    if (Registry->OpenKey("SYSTEM", false) &&
        Registry->OpenKey("CurrentControlSet", false) &&
        Registry->OpenKey("Control", false) &&
        Registry->OpenKey("TimeZoneInformation", false))
    {
      if (Registry->ValueExists("DynamicDaylightTimeDisabled"))
      {
        DynamicDaylightTimeDisabled = Registry->ReadBool("DynamicDaylightTimeDisabled");
      }
      // WORKAROUND
      // Windows XP equivalent
      else if (Registry->ValueExists("DisableAutoDaylightTimeSet"))
      {
        DynamicDaylightTimeDisabled = Registry->ReadBool("DisableAutoDaylightTimeSet");
      }
    }
  }
  catch (...)
  {
  }
  return !DynamicDaylightTimeDisabled;
}

UnicodeString StandardDatestamp()
{
#if defined(__BORLANDC__)
  return FormatDateTime(L"yyyy'-'mm'-'dd", ConvertTimestampToUTC(Now()));
#else
  TDateTime DT = ::ConvertTimestampToUTC(Now());
  uint16_t Y, M, D, H, N, S, MS;
  DT.DecodeDate(Y, M, D);
  DT.DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT(L"%04d-%02d-%02d", Y, M, D);
  return Result;
#endif
}

UnicodeString StandardTimestamp(const TDateTime & DateTime)
{
#if defined(__BORLANDC__)
  return FormatDateTime(L"yyyy'-'mm'-'dd'T'hh':'nn':'ss'.'zzz'Z'", ConvertTimestampToUTC(DateTime));
#else
  TDateTime DT = ::ConvertTimestampToUTC(DateTime);
  uint16_t Y, M, D, H, N, S, MS;
  DT.DecodeDate(Y, M, D);
  DT.DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT(L"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", Y, M, D, H, N, S, MS);
  return Result;
#endif
}

UnicodeString StandardTimestamp()
{
  return StandardTimestamp(Now());
}

intptr_t CompareFileTime(const TDateTime & T1, const TDateTime & T2)
{
  TDateTime TwoSeconds(0, 0, 2, 0);
  // "FAT" time precision
  // (when one time is seconds-precision and other is millisecond-precision,
  // we may have times like 12:00:00.000 and 12:00:01.999, which should
  // be treated the same)
  intptr_t Result;
  if (T1 == T2)
  {
    // just optimization
    Result = 0;
  }
  else if ((T1 < T2) && (T2 - T1 >= TwoSeconds))
  {
    Result = -1;
  }
  else if ((T1 > T2) && (T1 - T2 >= TwoSeconds))
  {
    Result = 1;
  }
  else
  {
    Result = 0;
  }
  return Result;
}

intptr_t TimeToMSec(const TDateTime & T)
{
  return int(Round(double(T) * double(MSecsPerDay)));
}

intptr_t TimeToSeconds(const TDateTime & T)
{
  return TimeToMSec(T) / MSecsPerSec;
}

intptr_t TimeToMinutes(const TDateTime & T)
{
  return TimeToSeconds(T) / SecsPerMin;
}

static bool DoRecursiveDeleteFile(const UnicodeString & AFileName, bool ToRecycleBin, UnicodeString & AErrorPath)
{
  bool Result;

  UnicodeString ErrorPath = AFileName;

#ifndef __linux__
  if (!ToRecycleBin)
#endif
  {
    TSearchRecChecked SearchRec;
    Result = FileSearchRec(AFileName, SearchRec);
    if (Result)
    {
      if (FLAGCLEAR(SearchRec.Attr, faDirectory))
      {
        Result = ::RemoveFile(AFileName);
      }
      else
      {
        Result = (FindFirstUnchecked(AFileName + L"\\*", faAnyFile, SearchRec) == 0);

        if (Result)
        {
          {
            SCOPE_EXIT
            {
              FindClose(SearchRec);
            };
            do
            {
              UnicodeString FileName2 = AFileName + L"\\" + SearchRec.Name;
              if (FLAGSET(SearchRec.Attr, faDirectory))
              {
                if ((SearchRec.Name != L".") && (SearchRec.Name != L".."))
                {
                  Result = DoRecursiveDeleteFile(FileName2, DebugAlwaysFalse(ToRecycleBin), ErrorPath);
                }
              }
              else
              {
                Result = ::RemoveFile(FileName2);
                if (!Result)
                {
                  ErrorPath = FileName2;
                }
              }
            }
            while (Result && (FindNextUnchecked(SearchRec) == 0));
          }

          if (Result)
          {
            Result = ::RemoveDir(AFileName);
          }
        }
      }
    }
  }
#ifndef __linux__
  else
  {
    SHFILEOPSTRUCT Data;

    ClearStruct(Data);
    Data.hwnd = nullptr;
    Data.wFunc = FO_DELETE;
    // SHFileOperation does not support long paths anyway
    UnicodeString FileList(ApiPath(AFileName));
    FileList.SetLength(FileList.Length() + 2);
    FileList[FileList.Length() - 1] = L'\0';
    FileList[FileList.Length()] = L'\0';
    Data.pFrom = FileList.c_str();
    Data.pTo = L"\0\0"; // this will actually give one null more than needed
    Data.fFlags = FOF_NOCONFIRMATION | FOF_RENAMEONCOLLISION | FOF_NOCONFIRMMKDIR |
      FOF_NOERRORUI | FOF_SILENT;
    if (DebugAlwaysTrue(ToRecycleBin))
    {
      Data.fFlags |= FOF_ALLOWUNDO;
    }
    int ErrorCode = ::SHFileOperation(&Data);
    Result = (ErrorCode == 0);
    if (!Result)
    {
      // according to MSDN, SHFileOperation may return following non-Win32
      // error codes
      if (((ErrorCode >= 0x71) && (ErrorCode <= 0x88)) ||
          (ErrorCode == 0xB7) || (ErrorCode == 0x402) || (ErrorCode == 0x10000) ||
          (ErrorCode == 0x10074))
      {
        ErrorCode = 0;
      }
      ::SetLastError(ErrorCode);
    }
  }
#endif

  if (!Result)
  {
    AErrorPath = ErrorPath;
  }

  return Result;
}

bool RecursiveDeleteFile(const UnicodeString & AFileName, bool ToRecycleBin)
{
  UnicodeString ErrorPath; // unused
  return DoRecursiveDeleteFile(AFileName, ToRecycleBin, ErrorPath);
}

void RecursiveDeleteFileChecked(const UnicodeString & AFileName, bool ToRecycleBin)
{
  UnicodeString ErrorPath;
  if (!DoRecursiveDeleteFile(AFileName, ToRecycleBin, ErrorPath))
  {
    throw EOSExtException(FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, ErrorPath.c_str()));
  }
}

void DeleteFileChecked(const UnicodeString & AFileName)
{
  if (!::RemoveFile(AFileName))
  {
    throw EOSExtException(FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, AFileName.c_str()));
  }
}

uintptr_t CancelAnswer(uintptr_t Answers)
{
  uintptr_t Result;
  if ((Answers & qaCancel) != 0)
  {
    Result = qaCancel;
  }
  else if ((Answers & qaNo) != 0)
  {
    Result = qaNo;
  }
  else if ((Answers & qaAbort) != 0)
  {
    Result = qaAbort;
  }
  else if ((Answers & qaOK) != 0)
  {
    Result = qaOK;
  }
  else
  {
    DebugFail();
    Result = qaCancel;
  }
  return Result;
}

uintptr_t AbortAnswer(uintptr_t Answers)
{
  uintptr_t Result;
  if (FLAGSET(Answers, qaAbort))
  {
    Result = qaAbort;
  }
  else
  {
    Result = CancelAnswer(Answers);
  }
  return Result;
}

uintptr_t ContinueAnswer(uintptr_t Answers)
{
  uintptr_t Result;
  if (FLAGSET(Answers, qaSkip))
  {
    Result = qaSkip;
  }
  else if (FLAGSET(Answers, qaIgnore))
  {
    Result = qaIgnore;
  }
  else if (FLAGSET(Answers, qaYes))
  {
    Result = qaYes;
  }
  else if (FLAGSET(Answers, qaOK))
  {
    Result = qaOK;
  }
  else if (FLAGSET(Answers, qaRetry))
  {
    Result = qaRetry;
  }
  else
  {
    Result = CancelAnswer(Answers);
  }
  return Result;
}

UnicodeString LoadStr(intptr_t Ident, uintptr_t /*MaxLength*/)
{
  UnicodeString Result = GetGlobalFunctions()->GetMsg(Ident);
  return Result;
}

UnicodeString LoadStrPart(intptr_t Ident, intptr_t Part)
{
  UnicodeString Result;
  UnicodeString Str = LoadStr(Ident);

  while (Part > 0)
  {
    Result = CutToChar(Str, L'|', false);
    Part--;
  }
  return Result;
}

UnicodeString DecodeUrlChars(const UnicodeString & S)
{
  UnicodeString Result = S;

  intptr_t Index = 1;
  while (Index <= Result.Length())
  {
    switch (Result[Index])
    {
      case L'+':
        Result[Index] = L' ';
        break;

      case L'%':
        UnicodeString Hex;
        while ((Index + 2 <= Result.Length()) && (Result[Index] == L'%') &&
               IsHex(Result[Index + 1]) && IsHex(Result[Index + 2]))
        {
          Hex += Result.SubString(Index + 1, 2);
          Result.Delete(Index, 3);
        }

        if (!Hex.IsEmpty())
        {
          RawByteString Bytes = HexToBytes(Hex);
          UTF8String UTF8(Bytes.c_str(), Bytes.Length());
          UnicodeString Chars(UTF8);
          Result.Insert(Chars, Index);
          Index += Chars.Length() - 1;
        }
        break;
    }
    ++Index;
  }
  return Result;
}

UnicodeString DoEncodeUrl(const UnicodeString & S, bool EncodeSlash)
{
  UnicodeString Result = S;

  intptr_t Index = 1;
  while (Index <= Result.Length())
  {
    wchar_t C = Result[Index];
    if (IsLetter(C) ||
        IsDigit(C) ||
        (C == L'_') || (C == L'-') || (C == L'.') ||
        ((C == LOTHER_SLASH) && !EncodeSlash))
    {
      ++Index;
    }
    else
    {
      UTF8String UtfS(Result.SubString(Index, 1));
      UnicodeString H;
      for (intptr_t Index2 = 1; Index2 <= UtfS.Length(); ++Index2)
      {
        H += L"%" + ByteToHex(static_cast<uint8_t>(UtfS[Index2]));
      }
      Result.Delete(Index, 1);
      Result.Insert(H, Index);
      Index += H.Length();
    }
#if 0
    if (Chars.Pos(Result[Index]) > 0)
    {
      UnicodeString H = ByteToHex(AnsiString(UnicodeString(Result[Index]))[1]);
      Result.Insert(H, Index + 1);
      Result[Index] = L'%';
      Index += H.Length();
    }
    ++Index;
#endif
  }
  return Result;
}

UnicodeString EncodeUrlString(const UnicodeString & S)
{
  return DoEncodeUrl(S, true);
}

UnicodeString EncodeUrlPath(const UnicodeString & S)
{
  return DoEncodeUrl(S, false);
}

UnicodeString AppendUrlParams(const UnicodeString & AURL, const UnicodeString & Params)
{
  UnicodeString URL = AURL;
  // see also TWebHelpSystem::ShowHelp
  const wchar_t FragmentSeparator = L'#';
  UnicodeString Result = ::CutToChar(URL, FragmentSeparator, false);

  if (Result.Pos(L"?") == 0)
  {
    Result += L"?";
  }
  else
  {
    Result += L"&";
  }

  Result += Params;

  AddToList(Result, URL, FragmentSeparator);

  return Result;
}

UnicodeString ExtractFileNameFromUrl(const UnicodeString & Url)
{
  UnicodeString Result = Url;
  intptr_t P = Result.Pos(L"?");
  if (P > 0)
  {
    Result.SetLength(P - 1);
  }
  P = Result.LastDelimiter("/");
  if (DebugAlwaysTrue(P > 0))
  {
    Result.Delete(1, P);
  }
  return Result;
}

UnicodeString EscapeHotkey(const UnicodeString & Caption)
{
  return ReplaceStr(Caption, L"&", L"&&");
}

// duplicated in console's Main.cpp
bool CutToken(UnicodeString & AStr, UnicodeString & AToken,
  UnicodeString * ARawToken, UnicodeString * ASeparator)
{
  bool Result;

  AToken.Clear();

  // inspired by Putty's sftp_getcmd() from PSFTP.C
  intptr_t Index = 1;
  while ((Index <= AStr.Length()) &&
    ((AStr[Index] == L' ') || (AStr[Index] == L'\t')))
  {
    ++Index;
  }

  if (Index <= AStr.Length())
  {
    bool Quoting = false;

    while (Index <= AStr.Length())
    {
      if (!Quoting && ((AStr[Index] == L' ') || (AStr[Index] == L'\t')))
      {
        break;
      }
      // We should escape quotes only within quotes
      // otherwise the "" means " (quote), but it should mean empty string.
      // Or have a special case for bare "".
      else if ((AStr[Index] == L'"') && (Index + 1 <= AStr.Length()) &&
        (AStr[Index + 1] == L'"'))
      {
        Index += 2;
        AToken += L'"';
      }
      else if (AStr[Index] == L'"')
      {
        ++Index;
        Quoting = !Quoting;
      }
      else
      {
        AToken += AStr[Index];
        ++Index;
      }
    }

    if (ARawToken != nullptr)
    {
      (*ARawToken) = AStr.SubString(1, Index - 1);
    }

    if (Index <= AStr.Length())
    {
      if (ASeparator != nullptr)
      {
        *ASeparator = AStr.SubString(Index, 1);
      }
      ++Index;
    }
    else
    {
      if (ASeparator != nullptr)
      {
        *ASeparator = UnicodeString();
      }
    }

    AStr = AStr.SubString(Index, AStr.Length());

    Result = true;
  }
  else
  {
    Result = false;
    AStr.Clear();
  }

  return Result;
}

void AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter)
{
  if (!Value.IsEmpty())
  {
    if (!List.IsEmpty() &&
        ((List.Length() < Delimiter.Length()) ||
         (List.SubString(List.Length() - Delimiter.Length() + 1, Delimiter.Length()) != Delimiter)))
    {
      List += Delimiter;
    }
    List += Value;
  }
}

static bool CheckWin32Version(int Major, int Minor)
{
  return (Win32MajorVersion >= Major) && (Win32MinorVersion >= Minor);
}

bool IsWinVista()
{
  // Vista is 6.0
  // Win XP is 5.1
  // There also 5.2, what is Windows 2003 or Windows XP 64bit
  // (we consider it WinXP for now)
  return CheckWin32Version(6, 0);
}

bool IsWin7()
{
  return CheckWin32Version(6, 1);
}
//---------------------------------------------------------------------------
bool IsWin8()
{
  return CheckWin32Version(6, 2);
}
//---------------------------------------------------------------------------
bool IsWin10()
{
  return CheckWin32Version(10, 0);
}

bool IsWine()
{
#ifndef __linux__
  HMODULE NtDll = ::GetModuleHandle(L"ntdll.dll");
  return
    DebugAlwaysTrue(NtDll != nullptr) &&
    (::GetProcAddress(NtDll, "wine_get_version") != nullptr);
#else
  return true;
#endif
}

LCID GetDefaultLCID()
{
#ifndef __linux__
  return GetUserDefaultLCID();
#else
  return 0;
#endif
}

UnicodeString DefaultEncodingName()
{
  static UnicodeString DefaultEncodingName;
  if (DefaultEncodingName.IsEmpty())
  {
    CPINFOEX Info;
    GetCPInfoEx(CP_ACP, 0, &Info);
    DefaultEncodingName = Info.CodePageName;
  }
  return DefaultEncodingName;
}

bool GetWindowsProductType(DWORD & Type)
{
  bool Result = false;
#ifndef __linux__
  HINSTANCE Kernel32 = ::GetModuleHandle(L"kernel32.dll");
  typedef BOOL (WINAPI * TGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);
  TGetProductInfo GetProductInfo =
      (TGetProductInfo)::GetProcAddress(Kernel32, "GetProductInfo");
  if (GetProductInfo != nullptr)
  {
    GetProductInfo(Win32MajorVersion, Win32MinorVersion, 0, 0, &Type);
    Result = true;
  }
#endif
  return Result;
}

UnicodeString WindowsProductName()
{
  UnicodeString Result;
  std::unique_ptr<TRegistry> Registry(new TRegistry());
  Registry->SetAccess(KEY_READ);
  try
  {
    Registry->SetRootKey(HKEY_LOCAL_MACHINE);
    if (Registry->OpenKey("SOFTWARE", false) &&
        Registry->OpenKey("Microsoft", false) &&
        Registry->OpenKey("Windows NT", false) &&
        Registry->OpenKey("CurrentVersion", false))
    {
      Result = Registry->ReadString("ProductName");
    }
  }
  catch (...)
  {
  }
  return Result;
}

bool IsDirectoryWriteable(const UnicodeString & APath)
{
  UnicodeString FileName =
    ::IncludeTrailingPathDelimiter(APath) +
    FORMAT(L"wscp_%s_%d.tmp", FormatDateTime(L"nnzzz", Now()).c_str(), int(GetCurrentProcessId()));
  HANDLE LocalFileHandle = ::CreateFile(ApiPath(FileName).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
    CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
  bool Result = (LocalFileHandle != INVALID_HANDLE_VALUE);
  if (Result)
  {
    ::CloseHandle(LocalFileHandle);
  }
  return Result;
}

UnicodeString FormatNumber(int64_t Number)
{
//  return FormatFloat(L"#,##0", Number);
  return FORMAT(L"%.0f", ToDouble(Number));
}

// simple alternative to FormatBytes
UnicodeString FormatSize(int64_t Size)
{
  return FormatNumber(Size);
}

UnicodeString ExtractFileBaseName(const UnicodeString & APath)
{
  return ChangeFileExt(base::ExtractFileName(APath, false), L"");
}

TStringList * TextToStringList(const UnicodeString & Text)
{
  std::unique_ptr<TStringList> List(new TStringList());
  List->SetText(Text);
  return List.release();
}

UnicodeString StringsToText(TStrings * Strings)
{
  UnicodeString Result;
  if (Strings->GetCount() == 1)
  {
    Result = Strings->GetString(0);
  }
  else
  {
    Result = Strings->GetText();
  }
  return Result;
}

TStrings * CloneStrings(TStrings * Strings)
{
  std::unique_ptr<TStringList> List(new TStringList());
  List->AddStrings(Strings);
  return List.release();
}

UnicodeString TrimVersion(const UnicodeString & Version)
{
  UnicodeString Result = Version;

  while ((Result.Pos(L".") != Result.LastDelimiter(L".")) &&
    (Result.SubString(Result.Length() - 1, 2) == L".0"))
  {
    Result.SetLength(Result.Length() - 2);
  }
  return Result;
}

UnicodeString FormatVersion(int MajorVersion, int MinorVersion, int Patch)
{
  return FORMAT(L"%d.%d.%d", MajorVersion, MinorVersion, Patch);
}

TFormatSettings GetEngFormatSettings()
{
  return TFormatSettings::Create(1033);
}

static int IndexStr(const UnicodeString & AStr)
{
  int Result = -1;
  for (int Index = 0; Index < 12; ++Index)
  {
    if (AStr.CompareIC(EngShortMonthNames[Index]) == 0)
    {
      Result = Index;
      break;
    }
  }
  return Result;
}

int ParseShortEngMonthName(const UnicodeString & MonthStr)
{
  // TFormatSettings FormatSettings = GetEngFormatSettings();
  // return IndexStr(MonthStr, FormatSettings.ShortMonthNames, FormatSettings.ShortMonthNames.size()) + 1;
  return IndexStr(MonthStr) + 1;
}

TStringList * CreateSortedStringList(bool CaseSensitive, TDuplicatesEnum Duplicates)
{
  TStringList * Result = new TStringList();
  Result->SetCaseSensitive(CaseSensitive);
  Result->SetSorted(true);
  Result->SetDuplicates(Duplicates);
  return Result;
}

static UnicodeString NormalizeIdent(const UnicodeString & Ident)
{
  UnicodeString Result = Ident;
  intptr_t Index = 1;
  while (Index <= Result.Length())
  {
    if (Result[Index] == L'-')
    {
      Result.Delete(Index, 1);
    }
    else
    {
      Index++;
    }
  }
  return Result;
}

UnicodeString FindIdent(const UnicodeString & Ident, TStrings * Idents)
{
  UnicodeString NormalizedIdent(NormalizeIdent(Ident));
  for (intptr_t Index = 0; Index < Idents->GetCount(); Index++)
  {
    if (SameText(NormalizedIdent, NormalizeIdent(Idents->GetString(Index))))
    {
      return Idents->GetString(Index);
    }
  }
  return Ident;
}

static UnicodeString GetTlsErrorStr(int Err)
{
  char * Buffer = new char[512];
  ERR_error_string(Err, Buffer);
  // not sure about the UTF8
  return UnicodeString(UTF8String(Buffer));
}
//---------------------------------------------------------------------------
#ifdef __linux__
FILE *_wfopen(const wchar_t *filename, const wchar_t *mode)
{
  return fopen(Wide2MB(filename).c_str(), Wide2MB(mode).c_str());
}
#endif
static FILE * OpenCertificate(const UnicodeString & Path)
{
  FILE * Result = _wfopen(ApiPath(Path).c_str(), L"rb");
  if (Result == nullptr)
  {
    int Error = errno;
    throw EOSExtException(MainInstructions(FMTLOAD(CERTIFICATE_OPEN_ERROR, Path.c_str())), Error);
  }

  return Result;
}
//---------------------------------------------------------------------------
struct TPemPasswordCallbackData
{
  UnicodeString * Passphrase;
};
//---------------------------------------------------------------------------
static int PemPasswordCallback(char * Buf, int Size, int /*RWFlag*/, void * UserData)
{
  TPemPasswordCallbackData & Data = *reinterpret_cast<TPemPasswordCallbackData *>(UserData);
  UTF8String UtfPassphrase = UTF8String(*Data.Passphrase);
  strncpy(Buf, UtfPassphrase.c_str(), Size);
  Shred(UtfPassphrase);
  Buf[Size - 1] = '\0';
  return static_cast<int>(strlen(Buf));
}
//---------------------------------------------------------------------------
static bool IsTlsPassphraseError(int Error, bool HasPassphrase)
{
  int ErrorLib = ERR_GET_LIB(Error);
  int ErrorReason = ERR_GET_REASON(Error);

  bool Result =
    ((ErrorLib == ERR_LIB_PKCS12) &&
     (ErrorReason == PKCS12_R_MAC_VERIFY_FAILURE)) ||
    ((ErrorLib == ERR_LIB_PEM) &&
     (ErrorReason == PEM_R_BAD_PASSWORD_READ)) ||
    (HasPassphrase && (ERR_LIB_EVP == ERR_LIB_EVP) &&
     ((ErrorReason == PEM_R_BAD_DECRYPT) || (ErrorReason == PEM_R_BAD_BASE64_DECODE)));

  return Result;
}
//---------------------------------------------------------------------------
static void ThrowTlsCertificateErrorIgnorePassphraseErrors(const UnicodeString & Path, bool HasPassphrase)
{
  int Error = ERR_get_error();
  if (!IsTlsPassphraseError(Error, HasPassphrase))
  {
    throw ExtException(MainInstructions(FMTLOAD(CERTIFICATE_READ_ERROR, Path.c_str())), GetTlsErrorStr(Error));
  }
}
//---------------------------------------------------------------------------
void ParseCertificate(const UnicodeString & Path,
  const UnicodeString & Passphrase, X509 *& Certificate, EVP_PKEY *& PrivateKey,
  bool & WrongPassphrase)
{
  Certificate = nullptr;
  PrivateKey = nullptr;
  bool HasPassphrase = !Passphrase.IsEmpty();

  FILE * File;

  // Inspired by neon's ne_ssl_clicert_read
  File = OpenCertificate(Path);
  // openssl pkcs12 -inkey cert.pem -in cert.crt -export -out cert.pfx
  // Binary file
  PKCS12 * Pkcs12 = d2i_PKCS12_fp(File, nullptr);
  fclose(File);

  if (Pkcs12 != nullptr)
  {
    // Not sure about the UTF-8 encoding, but there's no wchar_t API
    bool Result =
      (PKCS12_parse(Pkcs12, UTF8String(Passphrase).c_str(), &PrivateKey, &Certificate, nullptr) == 1);
    PKCS12_free(Pkcs12);

    if (!Result)
    {
      ThrowTlsCertificateErrorIgnorePassphraseErrors(Path, HasPassphrase);
      WrongPassphrase = true;
    }
  }
  else
  {
    ERR_clear_error();

    TPemPasswordCallbackData CallbackUserData;
    // PemPasswordCallback never writes to the .Passphrase
    CallbackUserData.Passphrase = const_cast<UnicodeString *>(&Passphrase);

    File = OpenCertificate(Path);
    // Encrypted:
    // openssl req -x509 -newkey rsa:2048 -keyout cert.pem -out cert.crt
    // -----BEGIN ENCRYPTED PRIVATE KEY-----
    // ...
    // -----END ENCRYPTED PRIVATE KEY-----

    // Not encrypted (add -nodes):
    // -----BEGIN PRIVATE KEY-----
    // ...
    // -----END PRIVATE KEY-----
    // Or (openssl genrsa -out client.key 1024   # used for certificate signing request)
    // -----BEGIN RSA PRIVATE KEY-----
    // ...
    // -----END RSA PRIVATE KEY-----
    PrivateKey = PEM_read_PrivateKey(File, nullptr, PemPasswordCallback, &CallbackUserData);
    fclose(File);

    try__finally
    {
      SCOPE_EXIT
      {
        // We loaded private key, but failed to load certificate, discard the certificate
        // (either exception was thrown or WrongPassphrase)
        if ((PrivateKey != nullptr) && (Certificate == nullptr))
        {
          EVP_PKEY_free(PrivateKey);
          PrivateKey = nullptr;
        }
        // Certificate was verified, but passphrase was wrong when loading private key,
        // so discard the certificate
        else if ((Certificate != nullptr) && (PrivateKey == nullptr))
        {
          X509_free(Certificate);
          Certificate = nullptr;
        }
      };
      if (PrivateKey == nullptr)
      {
        ThrowTlsCertificateErrorIgnorePassphraseErrors(Path, HasPassphrase);
        WrongPassphrase = true;
      }

      File = OpenCertificate(Path);
      // The file can contain both private and public key
      // (basically cert.pem and cert.crt appended one to each other)
      // -----BEGIN ENCRYPTED PRIVATE KEY-----
      // ...
      // -----END ENCRYPTED PRIVATE KEY-----
      // -----BEGIN CERTIFICATE-----
      // ...
      // -----END CERTIFICATE-----
      Certificate = PEM_read_X509(File, nullptr, PemPasswordCallback, &CallbackUserData);
      fclose(File);

      if (Certificate == nullptr)
      {
        int Error = ERR_get_error();
        // unlikely
        if (IsTlsPassphraseError(Error, HasPassphrase))
        {
          WrongPassphrase = true;
        }
        else
        {
          UnicodeString CertificatePath = ChangeFileExt(Path, L".cer");
          if (!FileExists(CertificatePath))
          {
            CertificatePath = ChangeFileExt(Path, L".crt");
          }

          if (!FileExists(CertificatePath))
          {
            throw Exception(MainInstructions(FMTLOAD(CERTIFICATE_PUBLIC_KEY_NOT_FOUND, Path.c_str())));
          }
          else
          {
            File = OpenCertificate(CertificatePath);
            // -----BEGIN CERTIFICATE-----
            // ...
            // -----END CERTIFICATE-----
            Certificate = PEM_read_X509(File, nullptr, PemPasswordCallback, &CallbackUserData);
            fclose(File);

            if (Certificate == nullptr)
            {
              int Base64Error = ERR_get_error();

              File = OpenCertificate(CertificatePath);
              // Binary DER-encoded certificate
              // (as above, with BEGIN/END removed, and decoded from Base64 to binary)
              // openssl x509 -in cert.crt -out client.der.crt -outform DER
              Certificate = d2i_X509_fp(File, nullptr);
              fclose(File);

              if (Certificate == nullptr)
              {
                int DERError = ERR_get_error();

                UnicodeString Message = MainInstructions(FMTLOAD(CERTIFICATE_READ_ERROR, CertificatePath.c_str()));
                UnicodeString MoreMessages =
                  FORMAT(L"Base64: %s\nDER: %s", GetTlsErrorStr(Base64Error).c_str(), GetTlsErrorStr(DERError).c_str());
                throw ExtException(Message, MoreMessages);
              }
            }
          }
        }
      }
    }
    __finally
    {
      // We loaded private key, but failed to load certificate, discard the certificate
      // (either exception was thrown or WrongPassphrase)
      if ((PrivateKey != nullptr) && (Certificate == nullptr))
      {
        EVP_PKEY_free(PrivateKey);
        PrivateKey = nullptr;
      }
      // Certificate was verified, but passphrase was wrong when loading private key,
      // so discard the certificate
      else if ((Certificate != nullptr) && (PrivateKey == nullptr))
      {
        X509_free(Certificate);
        Certificate = nullptr;
      }
    };
  }
}
//---------------------------------------------------------------------------
void CheckCertificate(const UnicodeString & Path)
{
  X509 * Certificate;
  EVP_PKEY * PrivateKey;
  bool WrongPassphrase;

  ParseCertificate(Path, L"", Certificate, PrivateKey, WrongPassphrase);

  if (PrivateKey != nullptr)
  {
    EVP_PKEY_free(PrivateKey);
  }
  if (Certificate != nullptr)
  {
    X509_free(Certificate);
  }
}
//---------------------------------------------------------------------------
const UnicodeString HttpProtocol(L"http");
const UnicodeString HttpsProtocol(L"https");
const UnicodeString ProtocolSeparator(L"://");
//---------------------------------------------------------------------------
bool IsHttpUrl(const UnicodeString & S)
{
  return StartsText(HttpProtocol + ProtocolSeparator, S);
}

bool IsHttpOrHttpsUrl(const UnicodeString & S)
{
  return
    IsHttpUrl(S) ||
    StartsText(HttpsProtocol + ProtocolSeparator, S);
}
//---------------------------------------------------------------------------
UnicodeString ChangeUrlProtocol(const UnicodeString & S, const UnicodeString & Protocol)
{
  intptr_t P = S.Pos(ProtocolSeparator);
  DebugAssert(P > 0);
  return Protocol + ProtocolSeparator + RightStr(S, S.Length() - P - ProtocolSeparator.Length() + 1);
}

const UnicodeString RtfPara = L"\\par\n";
const UnicodeString RtfHyperlinkField = L"HYPERLINK";
const UnicodeString RtfHyperlinkFieldPrefix = RtfHyperlinkField + L" \"";

UnicodeString FormatBytes(int64_t Bytes, bool UseOrders)
{
  UnicodeString Result;

  if (!UseOrders || (Bytes < static_cast<int64_t>(100 * 1024)))
  {
    // Result = FormatFloat(L"#,##0 \"B\"", Bytes);
    Result = FORMAT(L"%.0f B", ToDouble(Bytes));
  }
  else if (Bytes < static_cast<int64_t>(100 * 1024 * 1024))
  {
    // Result = FormatFloat(L"#,##0 \"KB\"", Bytes / 1024);
    Result = FORMAT(L"%.0f KB", ToDouble(Bytes / 1024.0));
  }
  else
  {
    // Result = FormatFloat(L"#,##0 \"MiB\"", Bytes / (1024*1024));
    Result = FORMAT(L"%.0f MiB", ToDouble(Bytes / (1024 * 1024.0)));
  }
  return Result;
}

namespace base {

UnicodeString UnixExtractFileName(const UnicodeString & APath)
{
  intptr_t Pos = APath.LastDelimiter(LOTHER_SLASH);
  UnicodeString Result;
  if (Pos > 0)
  {
    Result = APath.SubString(Pos + 1, APath.Length() - Pos);
  }
  else
  {
    Result = APath;
  }
  return Result;
}

UnicodeString UnixExtractFileExt(const UnicodeString & APath)
{
  UnicodeString FileName = base::UnixExtractFileName(APath);
  intptr_t Pos = FileName.LastDelimiter(L".");
  return (Pos > 0) ? APath.SubString(Pos, APath.Length() - Pos + 1) : UnicodeString();
}

UnicodeString ExtractFileName(const UnicodeString & APath, bool Unix)
{
  if (Unix)
  {
    return base::UnixExtractFileName(APath);
  }
  else
  {
    return ::ExtractFilename(APath, LGOOD_SLASH);
  }
}

UnicodeString GetEnvironmentVariable(const UnicodeString & AEnvVarName)
{
  UnicodeString Result;
  intptr_t Len = ::GetEnvironmentVariable(L"PATH", nullptr, 0);
  if (Len > 0)
  {
    Result.SetLength(Len - 1);
    ::GetEnvironmentVariable(AEnvVarName.c_str(), reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Result.c_str())), static_cast<DWORD>(Len));
  }
  return Result;
}

} // namespace base
