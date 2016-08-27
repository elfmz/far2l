#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <StrUtils.hpp>

#include "FileMasks.h"

#include "TextsCore.h"
#include "RemoteFiles.h"
#include "PuttyTools.h"
#include "Terminal.h"

#define FILE_MASKS_DELIMITERS L";,"
#define ALL_FILE_MASKS_DELIMITERS L";,|"
#define DIRECTORY_MASK_DELIMITERS L"/" WGOOD_SLASH ""
#define FILE_MASKS_DELIMITER_STR L"; "

EFileMasksException::EFileMasksException(
  const UnicodeString & AMessage, intptr_t AErrorStart, intptr_t AErrorLen) :
  Exception(AMessage), ErrorStart(AErrorStart), ErrorLen(AErrorLen)
{
}

static UnicodeString MaskFilePart(const UnicodeString & Part, const UnicodeString & Mask, bool & Masked)
{
  UnicodeString Result;
  intptr_t RestStart = 1;
  bool Delim = false;
  for (intptr_t Index = 1; Index <= Mask.Length(); ++Index)
  {
    switch (Mask[Index])
    {
      case LGOOD_SLASH:
        if (!Delim)
        {
          Delim = true;
          Masked = true;
        }
        break;

      case L'*':
        if (!Delim)
        {
          Result += Part.SubString(RestStart, Part.Length() - RestStart + 1);
          RestStart = Part.Length() + 1;
          Masked = true;
        }
        break;

      case L'?':
        if (!Delim)
        {
          if (RestStart <= Part.Length())
          {
            Result += Part[RestStart];
            RestStart++;
          }
          Masked = true;
        }
        break;

      default:
        Result += Mask[Index];
        RestStart++;
        Delim = false;
        break;
    }
  }
  return Result;
}

UnicodeString MaskFileName(const UnicodeString & AFileName, const UnicodeString & Mask)
{
  UnicodeString FileName = AFileName;
  if (IsEffectiveFileNameMask(Mask))
  {
    bool Masked = false;
    intptr_t P = Mask.LastDelimiter(L".");
    if (P > 0)
    {
      intptr_t P2 = FileName.LastDelimiter(L".");
      // only dot at beginning of file name is not considered as
      // name/ext separator
      UnicodeString FileExt = P2 > 1 ?
        FileName.SubString(P2 + 1, FileName.Length() - P2) : UnicodeString();
      FileExt = MaskFilePart(FileExt, Mask.SubString(P + 1, Mask.Length() - P), Masked);
      if (P2 > 1)
      {
        FileName.SetLength(P2 - 1);
      }
      FileName = MaskFilePart(FileName, Mask.SubString(1, P - 1), Masked);
      if (!FileExt.IsEmpty())
      {
        FileName += L"." + FileExt;
      }
    }
    else
    {
      FileName = MaskFilePart(FileName, Mask, Masked);
    }
  }
  return FileName;
}

bool IsFileNameMask(const UnicodeString & AMask)
{
  bool Result = AMask.IsEmpty(); // empty mask is the same as *
  if (!Result)
  {
    MaskFilePart(UnicodeString(), AMask, Result);
  }
  return Result;
}

bool IsEffectiveFileNameMask(const UnicodeString & AMask)
{
  return !AMask.IsEmpty() && (AMask != L"*") && (AMask != L"*.*");
}

UnicodeString DelimitFileNameMask(const UnicodeString & AMask)
{
  UnicodeString Mask = AMask;
  for (intptr_t Index = 1; Index <= Mask.Length(); ++Index)
  {
    if (wcschr(L"" WGOOD_SLASH "*?", Mask[Index]) != nullptr)
    {
      Mask.Insert(L"" WGOOD_SLASH "", Index);
      ++Index;
    }
  }
  return Mask;
}


TFileMasks::TParams::TParams() :
  Size(0)
{
}

UnicodeString TFileMasks::TParams::ToString() const
{
  return UnicodeString(L"[") + ::Int64ToStr(Size) + L"/" + ::DateTimeToString(Modification) + L"]";
}

bool TFileMasks::IsMask(const UnicodeString & Mask)
{
  return (Mask.LastDelimiter(L"?*[/") > 0);
}

bool TFileMasks::IsAnyMask(const UnicodeString & Mask)
{
  return Mask.IsEmpty() || (Mask == L"*.*") || (Mask == L"*");
}

UnicodeString TFileMasks::NormalizeMask(const UnicodeString & Mask, const UnicodeString & AnyMask)
{
  if (IsAnyMask(Mask))
  {
    return AnyMask;
  }
  else
  {
    return Mask;
  }
}

UnicodeString TFileMasks::ComposeMaskStr(
  TStrings * MasksStr, bool Directory)
{
  UnicodeString Result;
  UnicodeString ResultNoDirMask;
  for (intptr_t Index = 0; Index < MasksStr->GetCount(); ++Index)
  {
    UnicodeString Str = MasksStr->GetString(Index).Trim();
    if (!Str.IsEmpty())
    {
      for (intptr_t P = 1; P <= Str.Length(); P++)
      {
        if (Str.IsDelimiter(ALL_FILE_MASKS_DELIMITERS, P))
        {
          Str.Insert(Str[P], P);
          P++;
        }
      }

      UnicodeString StrNoDirMask;
      if (Directory)
      {
        StrNoDirMask = Str;
        Str = MakeDirectoryMask(Str);
      }
      else
      {
        while (Str.IsDelimiter(DIRECTORY_MASK_DELIMITERS, Str.Length()))
        {
          Str.SetLength(Str.Length() - 1);
        }
        StrNoDirMask = Str;
      }

      AddToList(Result, Str, FILE_MASKS_DELIMITER_STR);
      AddToList(ResultNoDirMask, StrNoDirMask, FILE_MASKS_DELIMITER_STR);
    }
  }

  // For directories, the above will add slash by the end of masks,
  // breaking size and time masks and thus circumventing their validation.
  // This performs as hoc validation to cover the scenario.
  // For files this makes no difference, but no harm either
  TFileMasks Temp(Directory ? 1 : 0);
  Temp = ResultNoDirMask;

  return Result;
}

UnicodeString TFileMasks::ComposeMaskStr(
  TStrings * IncludeFileMasksStr, TStrings * ExcludeFileMasksStr,
  TStrings * IncludeDirectoryMasksStr, TStrings * ExcludeDirectoryMasksStr)
{
  UnicodeString IncludeMasks = ComposeMaskStr(IncludeFileMasksStr, false);
  AddToList(IncludeMasks, ComposeMaskStr(IncludeDirectoryMasksStr, true), FILE_MASKS_DELIMITER_STR);
  UnicodeString ExcludeMasks = ComposeMaskStr(ExcludeFileMasksStr, false);
  AddToList(ExcludeMasks, ComposeMaskStr(ExcludeDirectoryMasksStr, true), FILE_MASKS_DELIMITER_STR);

  UnicodeString Result = IncludeMasks;
  if (!ExcludeMasks.IsEmpty())
  {
    if (!Result.IsEmpty())
    {
      Result += L' ';
    }
    Result += UnicodeString(INCLUDE_EXCLUDE_FILE_MASKS_DELIMITER) + L' ' + ExcludeMasks;
  }
  return Result;
}

TFileMasks::TFileMasks()
{
  Init();
}

TFileMasks::TFileMasks(intptr_t ForceDirectoryMasks) :
  FForceDirectoryMasks(ForceDirectoryMasks)
{
  Init();
}

TFileMasks::TFileMasks(const TFileMasks & Source)
{
  Init();
  FForceDirectoryMasks = Source.FForceDirectoryMasks;
  SetStr(Source.GetMasks(), false);
}

TFileMasks::TFileMasks(const UnicodeString & AMasks)
{
  Init();
  SetStr(AMasks, false);
}

TFileMasks::~TFileMasks()
{
  Clear();
}

void TFileMasks::Init()
{
  FForceDirectoryMasks = -1;
  for (intptr_t Index = 0; Index < 4; ++Index)
  {
    FMasksStr[Index] = nullptr;
  }

  DoInit(false);
}

void TFileMasks::DoInit(bool Delete)
{
  for (intptr_t Index = 0; Index < 4; ++Index)
  {
    if (Delete)
    {
      SAFE_DESTROY(FMasksStr[Index]);
    }
    FMasksStr[Index] = nullptr;
  }
}

void TFileMasks::Clear()
{
  DoInit(true);

  for (intptr_t Index = 0; Index < 4; ++Index)
  {
    Clear(FMasks[Index]);
  }
}

void TFileMasks::Clear(TMasks & Masks)
{
  TMasks::iterator it = Masks.begin();
  while (it != Masks.end())
  {
    ReleaseMaskMask(it->FileNameMask);
    ReleaseMaskMask(it->DirectoryMask);
    ++it;
  }
  Masks.clear();
}

bool TFileMasks::MatchesMasks(const UnicodeString & AFileName, bool Directory,
  const UnicodeString & APath, const TParams * Params, const TMasks & Masks, bool Recurse)
{
  bool Result = false;

  TMasks::const_iterator it = Masks.begin();
  while (!Result && (it != Masks.end()))
  {
    const TMask & Mask = *it;
    Result =
      MatchesMaskMask(Mask.DirectoryMask, APath) &&
      MatchesMaskMask(Mask.FileNameMask, AFileName);

    if (Result)
    {
      bool HasSize = (Params != nullptr);

      switch (Mask.HighSizeMask)
      {
        case TMask::None:
          Result = true;
          break;

        case TMask::Open:
          Result = HasSize && (Params->Size < Mask.HighSize);
          break;

        case TMask::Close:
          Result = HasSize && (Params->Size <= Mask.HighSize);
          break;
      }

      if (Result)
      {
        switch (Mask.LowSizeMask)
        {
          case TMask::None:
            Result = true;
            break;

          case TMask::Open:
            Result = HasSize && (Params->Size > Mask.LowSize);
            break;

          case TMask::Close:
            Result = HasSize && (Params->Size >= Mask.LowSize); //-V595
            break;
        }
      }

      bool HasModification = (Params != nullptr);

      if (Result)
      {
        switch (Mask.HighModificationMask)
        {
          case TMask::None:
            Result = true;
            break;

          case TMask::Open:
            Result = HasModification && (Params->Modification < Mask.HighModification);
            break;

          case TMask::Close:
            Result = HasModification && (Params->Modification <= Mask.HighModification);
            break;
        }
      }

      if (Result)
      {
        switch (Mask.LowModificationMask)
        {
          case TMask::None:
            Result = true;
            break;

          case TMask::Open:
            Result = HasModification && (Params->Modification > Mask.LowModification);
            break;

          case TMask::Close:
            Result = HasModification && (Params->Modification >= Mask.LowModification);
            break;
        }
      }
    }

    ++it;
  }

  if (!Result && Directory && !core::IsUnixRootPath(APath) && Recurse)
  {
    UnicodeString ParentFileName = base::UnixExtractFileName(APath);
    UnicodeString ParentPath = core::SimpleUnixExcludeTrailingBackslash(core::UnixExtractFilePath(APath));
    // Pass Params down or not?
    // Currently it includes Size/Time only, what is not used for directories.
    // So it depends on future use. Possibly we should make a copy
    // and pass on only relevant fields.
    Result = MatchesMasks(ParentFileName, true, ParentPath, Params, Masks, Recurse);
  }

  return Result;
}

bool TFileMasks::Matches(const UnicodeString & AFileName, bool Directory,
  const UnicodeString & APath, const TParams * Params) const
{
  bool ImplicitMatch;
  return Matches(AFileName, Directory, APath, Params, true, ImplicitMatch);
}

bool TFileMasks::Matches(const UnicodeString & AFileName, bool Directory,
  const UnicodeString & APath, const TParams * Params,
  bool RecurseInclude, bool & ImplicitMatch) const
{
  bool ImplicitIncludeMatch = FMasks[MASK_INDEX(Directory, true)].empty();
  bool ExplicitIncludeMatch = MatchesMasks(AFileName, Directory, APath, Params, FMasks[MASK_INDEX(Directory, true)], RecurseInclude);
  bool Result =
    (ImplicitIncludeMatch || ExplicitIncludeMatch) &&
    !MatchesMasks(AFileName, Directory, APath, Params, FMasks[MASK_INDEX(Directory, false)], false);
  ImplicitMatch =
    Result && ImplicitIncludeMatch && !ExplicitIncludeMatch &&
    FMasks[MASK_INDEX(Directory, false)].empty();
  return Result;
}

bool TFileMasks::Matches(const UnicodeString & AFileName, bool Local,
  bool Directory, const TParams * Params) const
{
  bool ImplicitMatch;
  return Matches(AFileName, Local, Directory, Params, true, ImplicitMatch);
}

bool TFileMasks::Matches(const UnicodeString & AFileName, bool Local,
  bool Directory, const TParams * Params, bool RecurseInclude, bool & ImplicitMatch) const
{
  bool Result;
  if (Local)
  {
    UnicodeString Path = ::ExtractFilePath(AFileName);
    if (!Path.IsEmpty())
    {
      Path = core::ToUnixPath(::ExcludeTrailingBackslash(Path));
    }
    Result = Matches(base::ExtractFileName(AFileName, false), Directory, Path, Params,
      RecurseInclude, ImplicitMatch);
  }
  else
  {
    Result = Matches(base::UnixExtractFileName(AFileName), Directory,
      core::SimpleUnixExcludeTrailingBackslash(core::UnixExtractFilePath(AFileName)), Params,
      RecurseInclude, ImplicitMatch);
  }
  return Result;
}

bool TFileMasks::operator ==(const TFileMasks & rhm) const
{
  return (GetMasks() == rhm.GetMasks());
}

TFileMasks & TFileMasks::operator =(const UnicodeString & rhs)
{
  SetMasks(rhs);
  return *this;
}

TFileMasks & TFileMasks::operator =(const TFileMasks & rhm)
{
  FForceDirectoryMasks = rhm.FForceDirectoryMasks;
  SetMasks(rhm.GetMasks());
  return *this;
}

bool TFileMasks::operator ==(const UnicodeString & rhs) const
{
  return (GetMasks() == rhs);
}

void TFileMasks::ThrowError(intptr_t Start, intptr_t End) const
{
  throw EFileMasksException(
    FMTLOAD(MASK_ERROR, GetMasks().SubString(Start, End - Start + 1).c_str()),
    Start, End - Start + 1);
}

void TFileMasks::CreateMaskMask(const UnicodeString & Mask, intptr_t Start, intptr_t End,
  bool Ex, TMaskMask & MaskMask) const
{
  try
  {
    DebugAssert(MaskMask.Mask == nullptr);
    if (Ex && IsAnyMask(Mask))
    {
      MaskMask.Kind = TMaskMask::Any;
      MaskMask.Mask = nullptr;
    }
    else
    {
      MaskMask.Kind = (Ex && (Mask == L"*.")) ? TMaskMask::NoExt : TMaskMask::Regular;
      MaskMask.Mask = new Masks::TMask(Mask);
    }
  }
  catch (...)
  {
    ThrowError(Start, End);
  }
}

UnicodeString TFileMasks::MakeDirectoryMask(const UnicodeString & AStr)
{
  DebugAssert(!AStr.IsEmpty());
  UnicodeString Str = AStr;
  if (Str.IsEmpty() || !Str.IsDelimiter(DIRECTORY_MASK_DELIMITERS, Str.Length()))
  {
    intptr_t D = Str.LastDelimiter(DIRECTORY_MASK_DELIMITERS);
    // if there's any [back]slash anywhere in str,
    // add the same [back]slash at the end, otherwise add slash
    wchar_t Delimiter = (D > 0) ? Str[D] : LOTHER_SLASH;
    Str += Delimiter;
  }
  return Str;
}

void TFileMasks::CreateMask(
  const UnicodeString & MaskStr, intptr_t MaskStart, intptr_t /*MaskEnd*/, bool Include)
{
  bool Directory = false; // shut up
  TMask Mask;

  Mask.MaskStr = MaskStr;
  Mask.UserStr = MaskStr;
  Mask.FileNameMask.Kind = TMaskMask::Any;
  Mask.FileNameMask.Mask = nullptr;
  Mask.DirectoryMask.Kind = TMaskMask::Any;
  Mask.DirectoryMask.Mask = nullptr;
  Mask.HighSizeMask = TMask::None;
  Mask.LowSizeMask = TMask::None;
  Mask.HighModificationMask = TMask::None;
  Mask.LowModificationMask = TMask::None;

  wchar_t NextPartDelimiter = L'\0';
  intptr_t NextPartFrom = 1;
  while (NextPartFrom <= MaskStr.Length())
  {
    wchar_t PartDelimiter = NextPartDelimiter;
    intptr_t PartFrom = NextPartFrom;
    UnicodeString PartStr = CopyToChars(MaskStr, NextPartFrom, L"<>", false, &NextPartDelimiter, true);

    intptr_t PartStart = MaskStart + PartFrom - 1;
    intptr_t PartEnd = MaskStart + NextPartFrom - 1 - 2;

    TrimEx(PartStr, PartStart, PartEnd);

    if (PartDelimiter != L'\0')
    {
      bool Low = (PartDelimiter == L'>');

      TMask::TMaskBoundary Boundary;
      if ((PartStr.Length() >= 1) && (PartStr[1] == L'='))
      {
        Boundary = TMask::Close;
        PartStr.Delete(1, 1);
      }
      else
      {
        Boundary = TMask::Open;
      }

      TFormatSettings FormatSettings = TFormatSettings::Create(GetDefaultLCID());
      FormatSettings.DateSeparator = L'-';
      FormatSettings.TimeSeparator = L':';
      FormatSettings.ShortDateFormat = "yyyy/mm/dd";
      FormatSettings.ShortTimeFormat = "hh:nn:ss";

      TDateTime Modification;
      if (TryStrToDateTime(PartStr, Modification, FormatSettings) ||
          TryRelativeStrToDateTime(PartStr, Modification))
      {
        TMask::TMaskBoundary & ModificationMask =
          (Low ? Mask.LowModificationMask : Mask.HighModificationMask);

        if ((ModificationMask != TMask::None) || Directory)
        {
          // include delimiter into size part
          ThrowError(PartStart - 1, PartEnd);
        }

        ModificationMask = Boundary;
        (Low ? Mask.LowModification : Mask.HighModification) = Modification;
      }
      else
      {
        TMask::TMaskBoundary & SizeMask = (Low ? Mask.LowSizeMask : Mask.HighSizeMask);
        int64_t & Size = (Low ? Mask.LowSize : Mask.HighSize);

        if ((SizeMask != TMask::None) || Directory)
        {
          // include delimiter into size part
          ThrowError(PartStart - 1, PartEnd);
        }

        SizeMask = Boundary;
        Size = ParseSize(PartStr);
      }
    }
    else if (!PartStr.IsEmpty())
    {
      intptr_t D = PartStr.LastDelimiter(DIRECTORY_MASK_DELIMITERS);

      Directory = (D > 0) && (D == PartStr.Length());

      if (Directory)
      {
        do
        {
          PartStr.SetLength(PartStr.Length() - 1);
          Mask.UserStr.Delete(PartStart - MaskStart + D, 1);
          D--;
        }
        while (PartStr.IsDelimiter(DIRECTORY_MASK_DELIMITERS, PartStr.Length()));

        D = PartStr.LastDelimiter(DIRECTORY_MASK_DELIMITERS);

        if (FForceDirectoryMasks == 0)
        {
          Directory = false;
          Mask.MaskStr = Mask.UserStr;
        }
      }
      else if (FForceDirectoryMasks > 0)
      {
        Directory = true;
        Mask.MaskStr.Insert(LOTHER_SLASH, PartStart - MaskStart + PartStr.Length());
      }

      if (D > 0)
      {
        // make sure sole "/" (root dir) is preserved as is
        CreateMaskMask(
          core::SimpleUnixExcludeTrailingBackslash(core::ToUnixPath(PartStr.SubString(1, D))),
          PartStart, PartStart + D - 1, false,
          Mask.DirectoryMask);
        CreateMaskMask(
          PartStr.SubString(D + 1, PartStr.Length() - D),
          PartStart + D, PartEnd, true,
          Mask.FileNameMask);
      }
      else
      {
        CreateMaskMask(PartStr, PartStart, PartEnd, true, Mask.FileNameMask);
      }
    }
  }

  FMasks[MASK_INDEX(Directory, Include)].push_back(Mask);
}

TStrings * TFileMasks::GetMasksStr(intptr_t Index) const
{
  if (FMasksStr[Index] == nullptr)
  {
    FMasksStr[Index] = new TStringList();
    TMasks::const_iterator it = FMasks[Index].begin();
    while (it != FMasks[Index].end())
    {
      FMasksStr[Index]->Add((*it).UserStr);
      ++it;
    }
  }

  return FMasksStr[Index];
}

void TFileMasks::ReleaseMaskMask(TMaskMask & MaskMask)
{
  SAFE_DESTROY(MaskMask.Mask);
}

void TFileMasks::TrimEx(UnicodeString & Str, intptr_t & Start, intptr_t & End)
{
  UnicodeString Buf = ::TrimLeft(Str);
  Start += Str.Length() - Buf.Length();
  Str = ::TrimRight(Buf);
  End -= Buf.Length() - Str.Length();
}

bool TFileMasks::MatchesMaskMask(const TMaskMask & MaskMask, const UnicodeString & Str)
{
  bool Result;
  if (MaskMask.Kind == TMaskMask::Any)
  {
    Result = true;
  }
  else if ((MaskMask.Kind == TMaskMask::NoExt) && (Str.Pos(L".") == 0))
  {
    Result = true;
  }
  else
  {
    Result = MaskMask.Mask->Matches(Str);
  }
  return Result;
}

void TFileMasks::SetMasks(const UnicodeString & Value)
{
  if (FStr != Value)
  {
    SetStr(Value, false);
  }
}

void TFileMasks::SetMask(const UnicodeString & Mask)
{
  SetStr(Mask, true);
}

void TFileMasks::SetStr(const UnicodeString & Str, bool SingleMask)
{
  UnicodeString Backup = FStr;
  try
  {
    FStr = Str;
    Clear();

    intptr_t NextMaskFrom = 1;
    bool Include = true;
    while (NextMaskFrom <= Str.Length())
    {
      intptr_t MaskStart = NextMaskFrom;
      wchar_t NextMaskDelimiter;
      UnicodeString MaskStr;
      if (SingleMask)
      {
        MaskStr = Str;
        NextMaskFrom = Str.Length() + 1;
        NextMaskDelimiter = L'\0';
      }
      else
      {
        MaskStr = CopyToChars(Str, NextMaskFrom, ALL_FILE_MASKS_DELIMITERS, false, &NextMaskDelimiter, true);
      }
      intptr_t MaskEnd = NextMaskFrom - 2;

      TrimEx(MaskStr, MaskStart, MaskEnd);

      if (!MaskStr.IsEmpty())
      {
        CreateMask(MaskStr, MaskStart, MaskEnd, Include);
      }

      if (NextMaskDelimiter == INCLUDE_EXCLUDE_FILE_MASKS_DELIMITER)
      {
        if (Include)
        {
          Include = false;
        }
        else
        {
          ThrowError(NextMaskFrom - 1, Str.Length());
        }
      }
    }
  }
  catch (...)
  {
    // this does not work correctly if previous mask was set using SetMask.
    // this should not fail (the mask was validated before),
    // otherwise we end in an infinite loop
    SetStr(Backup, false);
    throw;
  }
}


#define TEXT_TOKEN L'\255'

const wchar_t TCustomCommand::NoQuote = L'\0';
#define CONST_QUOTES L"\"'"

TCustomCommand::TCustomCommand()
{
}

void TCustomCommand::GetToken(
  const UnicodeString & Command, intptr_t Index, intptr_t & Len, wchar_t & PatternCmd) const
{
  DebugAssert(Index <= Command.Length());
  const wchar_t * Ptr = Command.c_str() + Index - 1;

  if (Ptr[0] == L'!')
  {
    PatternCmd = Ptr[1];
    if (PatternCmd == L'\0')
    {
      Len = 1;
    }
    else if (PatternCmd == L'!')
    {
      Len = 2;
    }
    else
    {
      Len = PatternLen(Command, Index);
    }

    if (Len <= 0)
    {
      throw Exception(FMTLOAD(CUSTOM_COMMAND_UNKNOWN, PatternCmd, Index));
    }
    else
    {
      if ((Command.Length() - Index + 1) < Len)
      {
        throw Exception(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, PatternCmd, Index));
      }
    }
  }
  else
  {
    PatternCmd = TEXT_TOKEN;
    const wchar_t * NextPattern = wcschr(Ptr, L'!');
    if (NextPattern == nullptr)
    {
      Len = Command.Length() - Index + 1;
    }
    else
    {
      Len = NextPattern - Ptr;
    }
  }
}

void TCustomCommand::PatternHint(intptr_t /*Index*/, const UnicodeString & /*Pattern*/)
{
  // noop
}

UnicodeString TCustomCommand::Complete(const UnicodeString & Command,
  bool LastPass)
{
  intptr_t Index = 1;
  intptr_t PatternIndex = 0;
  while (Index <= Command.Length())
  {
    intptr_t Len;
    wchar_t PatternCmd;
    GetToken(Command, Index, Len, PatternCmd);

    if (PatternCmd == TEXT_TOKEN)
    {
    }
    else if (PatternCmd == L'!')
    {
    }
    else
    {
      UnicodeString Pattern = Command.SubString(Index, Len);
      PatternHint(PatternIndex, Pattern);
      PatternIndex++;
    }

    Index += Len;
  }

  UnicodeString Result;
  Index = 1;
  PatternIndex = 0;
  while (Index <= Command.Length())
  {
    intptr_t Len;
    wchar_t PatternCmd;
    GetToken(Command, Index, Len, PatternCmd);

    if (PatternCmd == TEXT_TOKEN)
    {
      Result += Command.SubString(Index, Len);
    }
    else if (PatternCmd == L'!')
    {
      if (LastPass)
      {
        Result += L'!';
      }
      else
      {
        Result += Command.SubString(Index, Len);
      }
    }
    else
    {
      wchar_t Quote = NoQuote;
      UnicodeString Quotes(CONST_QUOTES);
      if ((Index > 1) && (Index + Len - 1 < Command.Length()) &&
          Command.IsDelimiter(Quotes, Index - 1) &&
          Command.IsDelimiter(Quotes, Index + Len) &&
          (Command[Index - 1] == Command[Index + Len]))
      {
        Quote = Command[Index - 1];
      }
      UnicodeString Pattern = Command.SubString(Index, Len);
      UnicodeString Replacement;
      bool Delimit = true;
      if (PatternReplacement(PatternIndex, Pattern, Replacement, Delimit))
      {
        if (!LastPass)
        {
          Replacement = ReplaceStr(Replacement, L"!", L"!!");
        }
        if (Delimit)
        {
          DelimitReplacement(Replacement, Quote);
        }
        Result += Replacement;
      }
      else
      {
        Result += Pattern;
      }

      PatternIndex++;
    }

    Index += Len;
  }

  return Result;
}

void TCustomCommand::DelimitReplacement(UnicodeString & Replacement, wchar_t Quote)
{
  Replacement = ShellDelimitStr(Replacement, Quote);
}

void TCustomCommand::Validate(const UnicodeString & Command)
{
  CustomValidate(Command, nullptr);
}

void TCustomCommand::CustomValidate(const UnicodeString & Command,
  void * Arg)
{
  intptr_t Index = 1;

  while (Index <= Command.Length())
  {
    intptr_t Len;
    wchar_t PatternCmd;
    GetToken(Command, Index, Len, PatternCmd);
    ValidatePattern(Command, Index, Len, PatternCmd, Arg);

    Index += Len;
  }
}

bool TCustomCommand::FindPattern(const UnicodeString & Command,
  wchar_t PatternCmd) const
{
  bool Result = false;
  intptr_t Index = 1;

  while (!Result && (Index <= Command.Length()))
  {
    intptr_t Len;
    wchar_t APatternCmd;
    GetToken(Command, Index, Len, APatternCmd);
    if (((PatternCmd != L'!') && (PatternCmd == APatternCmd)) ||
        ((PatternCmd == L'!') && (Len == 1) && (APatternCmd != TEXT_TOKEN)))
    {
      Result = true;
    }

    Index += Len;
  }

  return Result;
}

void TCustomCommand::ValidatePattern(const UnicodeString & /*Command*/,
  intptr_t /*Index*/, intptr_t /*Len*/, wchar_t /*PatternCmd*/, void * /*Arg*/)
{
}


TInteractiveCustomCommand::TInteractiveCustomCommand(
  TCustomCommand * ChildCustomCommand)
{
  FChildCustomCommand = ChildCustomCommand;
}

void TInteractiveCustomCommand::Prompt(
  intptr_t /*Index*/, const UnicodeString & /*Prompt*/, UnicodeString & Value) const
{
  Value.Clear();
}

void TInteractiveCustomCommand::Execute(
  const UnicodeString & /*Command*/, UnicodeString & Value) const
{
  Value.Clear();
}

intptr_t TInteractiveCustomCommand::PatternLen(const UnicodeString & Command, intptr_t Index) const
{
  intptr_t Len = 0;
  wchar_t PatternCmd = (Index < Command.Length()) ? Command[Index + 1] : L'\0';
  switch (PatternCmd)
  {
    case L'?':
    {
      const wchar_t * Ptr = Command.c_str() + Index - 1;
      const wchar_t * PatternEnd = wcschr(Ptr + 1, L'!');
      if (PatternEnd == nullptr)
      {
        throw Exception(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, Command[Index + 1], Index));
      }
      Len = PatternEnd - Ptr + 1;
    }
    break;

    case L'`':
    {
      const wchar_t * Ptr = Command.c_str() + Index - 1;
      const wchar_t * PatternEnd = wcschr(Ptr + 2, L'`');
      if (PatternEnd == nullptr)
      {
        throw Exception(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, Command[Index + 1], Index));
      }
      Len = PatternEnd - Ptr + 1;
    }
    break;

    default:
      Len = FChildCustomCommand->PatternLen(Command, Index);
      break;
  }
  return Len;
}

bool TInteractiveCustomCommand::IsPromptPattern(const UnicodeString & Pattern) const
{
  return (Pattern.Length() >= 3) && (Pattern[2] == L'?');
}

void TInteractiveCustomCommand::ParsePromptPattern(
  const UnicodeString & Pattern, UnicodeString & Prompt, UnicodeString & Default, bool & Delimit) const
{
  intptr_t Pos = Pattern.SubString(3, Pattern.Length() - 2).Pos(L"?");
  if (Pos > 0)
  {
    Default = Pattern.SubString(3 + Pos, Pattern.Length() - 3 - Pos);
    if ((Pos > 1) && (Pattern[3 + Pos - 2] == LGOOD_SLASH))
    {
      Delimit = false;
      Pos--;
    }
    Prompt = Pattern.SubString(3, Pos - 1);
  }
  else
  {
    Prompt = Pattern.SubString(3, Pattern.Length() - 3);
  }
}

bool TInteractiveCustomCommand::PatternReplacement(intptr_t Index, const UnicodeString & Pattern,
  UnicodeString & Replacement, bool & Delimit) const
{
  bool Result;
  if (IsPromptPattern(Pattern))
  {
    UnicodeString PromptStr;
    // The PromptStr and Replacement are actually never used
    // as the only implementation (TWinInteractiveCustomCommand) uses
    // prompts and defaults from PatternHint.
    ParsePromptPattern(Pattern, PromptStr, Replacement, Delimit);

    Prompt(Index, PromptStr, Replacement);

    Result = true;
  }
  else if ((Pattern.Length() >= 3) && (Pattern[2] == L'`'))
  {
    UnicodeString Command = Pattern.SubString(3, Pattern.Length() - 3);
    Command = FChildCustomCommand->Complete(Command, true);
    Execute(Command, Replacement);
    Delimit = false;
    Result = true;
  }
  else
  {
    Result = false;
  }

  return Result;
}


TCustomCommandData::TCustomCommandData()
{
}

TCustomCommandData::TCustomCommandData(const TCustomCommandData & Data)
{
  this->operator=(Data);
}

//---------------------------------------------------------------------------
TCustomCommandData::TCustomCommandData(TTerminal * Terminal)
{
  Init(Terminal->GetSessionData(), Terminal->TerminalGetUserName(), Terminal->GetPassword(),
    Terminal->GetSessionInfo().HostKeyFingerprint);
}

TCustomCommandData::TCustomCommandData(
  TSessionData * SessionData, const UnicodeString & AUserName, const UnicodeString & APassword)
{
  Init(SessionData, AUserName, APassword, UnicodeString());
}

void TCustomCommandData::Init(
  TSessionData * ASessionData, const UnicodeString & AUserName,
  const UnicodeString & APassword, const UnicodeString & AHostKey)
{
  FSessionData.reset(new TSessionData(L""));
  FSessionData->Assign(ASessionData);
  FSessionData->SetUserName(AUserName);
  FSessionData->SetPassword(APassword);
  FSessionData->SetHostKey(AHostKey);
}

TCustomCommandData & TCustomCommandData::operator=(const TCustomCommandData & Data)
{
  if (&Data != this)
  {
    DebugAssert(Data.GetSessionData() != nullptr);
    FSessionData.reset(new TSessionData(L""));
    FSessionData->Assign(Data.GetSessionData());
  }
  return *this;
}

TSessionData * TCustomCommandData::GetSessionData() const
{
  return FSessionData.get();
}

TFileCustomCommand::TFileCustomCommand()
{
}


TFileCustomCommand::TFileCustomCommand(const TCustomCommandData & Data,
  const UnicodeString & APath) :
  FData(Data),
  FPath(APath)
{
}

TFileCustomCommand::TFileCustomCommand(const TCustomCommandData & Data,
  const UnicodeString & APath, const UnicodeString & AFileName,
  const UnicodeString & FileList) :
  FData(Data),
  FPath(APath),
  FFileName(AFileName),
  FFileList(FileList)
{
}

intptr_t TFileCustomCommand::PatternLen(const UnicodeString & Command, intptr_t Index) const
{
  intptr_t Len;
  wchar_t PatternCmd = (Index < Command.Length()) ? Command[Index + 1] : L'\0';
  switch (PatternCmd)
  {
    case L'S':
    case L'@':
    case L'U':
    case L'P':
    case L'#':
    case LOTHER_SLASH:
    case L'&':
    case L'N':
      Len = 2;
      break;

    default:
      Len = 1;
      break;
  }
  return Len;
}

bool TFileCustomCommand::PatternReplacement(
  intptr_t /*Index*/, const UnicodeString & Pattern, UnicodeString & Replacement, bool & Delimit) const
{
  // keep consistent with TSessionLog::OpenLogFile

  TSessionData * SessionData = FData.GetSessionData();

  if (AnsiSameText(Pattern, L"!s"))
  {
    Replacement = SessionData->GenerateSessionUrl(sufComplete);
  }
  else if (Pattern == L"!@")
  {
    Replacement = SessionData->GetHostNameExpanded();
  }
  else if (::AnsiSameText(Pattern, L"!u"))
  {
    Replacement = SessionData->SessionGetUserName();
  }
  else if (::AnsiSameText(Pattern, L"!p"))
  {
    Replacement = SessionData->GetPassword();
  }
  else if (::AnsiSameText(Pattern, L"!#"))
  {
    Replacement = IntToStr(SessionData->GetPortNumber());
  }
  else if (Pattern == L"!/")
  {
    Replacement = core::UnixIncludeTrailingBackslash(FPath);
  }
  else if (Pattern == L"!&")
  {
    Replacement = FFileList;
    // already delimited
    Delimit = false;
  }
  else if (AnsiSameText(Pattern, L"!n"))
  {
    Replacement = SessionData->GetSessionName();
  }
  else
  {
    DebugAssert(Pattern.Length() == 1);
    Replacement = FFileName;
  }

  return true;
}

void TFileCustomCommand::Validate(const UnicodeString & Command)
{
  intptr_t Found[2] = { 0, 0 };
  CustomValidate(Command, &Found);
  if ((Found[0] > 0) && (Found[1] > 0))
  {
    throw Exception(FMTLOAD(CUSTOM_COMMAND_FILELIST_ERROR,
      Found[1], Found[0]));
  }
}

void TFileCustomCommand::ValidatePattern(const UnicodeString & Command,
  intptr_t Index, intptr_t /*Len*/, wchar_t PatternCmd, void * Arg)
{
  intptr_t * Found = static_cast<intptr_t *>(Arg);

  DebugAssert(Index > 0);

  if (PatternCmd == L'&')
  {
    Found[0] = static_cast<int>(Index);
  }
  else if ((PatternCmd != TEXT_TOKEN) && (PatternLen(Command, Index) == 1))
  {
    Found[1] = Index;
  }
}

bool TFileCustomCommand::IsFileListCommand(const UnicodeString & Command) const
{
  return FindPattern(Command, L'&');
}

bool TFileCustomCommand::IsRemoteFileCommand(const UnicodeString & Command) const
{
  return FindPattern(Command, L'!') || FindPattern(Command, L'&');
}

bool TFileCustomCommand::IsFileCommand(const UnicodeString & Command) const
{
  return IsRemoteFileCommand(Command);
}

bool TFileCustomCommand::IsSiteCommand(const UnicodeString & Command) const
{
  return FindPattern(Command, L'@');
}

bool TFileCustomCommand::IsPasswordCommand(const UnicodeString & Command) const
{
  return FindPattern(Command, L'p');
}

