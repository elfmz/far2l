#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Exceptions.h>
#include "CopyParam.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"
#include "Interface.h"

const wchar_t * TransferModeNames[] = { L"binary", L"ascii", L"automatic" };
const int TransferModeNamesCount = _countof(TransferModeNames);
const wchar_t * ToggleNames[] = { L"off", L"on" };

TCopyParamType::TCopyParamType()
{
  Default();
}

TCopyParamType::TCopyParamType(const TCopyParamType & Source)
{
  Assign(&Source);
}

TCopyParamType::~TCopyParamType()
{
}

void TCopyParamType::Default()
{
  // when changing defaults, make sure GetInfoStr() can handle it
  SetFileNameCase(ncNoChange);
  SetPreserveReadOnly(false);
  SetPreserveTime(true);
  SetPreserveTimeDirs(false);
  FRights.SetNumber(TRights::rfDefault);
  SetPreserveRights(false); // Was True until #106
  SetIgnorePermErrors(false);
  FAsciiFileMask.SetMasks(UnicodeString(L"*.*html; *.htm; *.txt; *.php; *.php3; *.cgi; *.c; *.cpp; *.h; *.pas; "
    L"*.bas; *.tex; *.pl; *.js; .htaccess; *.xtml; *.css; *.cfg; *.ini; *.sh; *.xml"));
  SetTransferMode(tmBinary);
  SetAddXToDirectories(true);
  SetResumeSupport(rsSmart);
  SetResumeThreshold(100 * 1024); // (100 KB)
  SetInvalidCharsReplacement(TokenReplacement);
  SetLocalInvalidChars(LOCAL_INVALID_CHARS);
  SetCalculateSize(true);
  SetFileMask(L"*.*");
  GetIncludeFileMask().SetMasks(L"");
  SetTransferSkipList(nullptr);
  SetTransferResumeFile(L"");
  SetClearArchive(false);
  SetRemoveCtrlZ(false);
  SetRemoveBOM(false);
  SetCPSLimit(0);
  SetNewerOnly(false);
}

UnicodeString TCopyParamType::GetInfoStr(
  const UnicodeString & Separator, intptr_t Attrs) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  UnicodeString ScriptArgs;
  bool NoScriptArgs;
  UnicodeString AssemblyCode;
  bool NoCodeProperties;
  DoGetInfoStr(
    Separator, Attrs, Result, SomeAttrIncluded,
    UnicodeString(), ScriptArgs, NoScriptArgs, /*TAssemblyLanguage(0), AssemblyCode, */NoCodeProperties);
  return Result;
}

bool TCopyParamType::AnyUsableCopyParam(intptr_t Attrs) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  UnicodeString ScriptArgs;
  bool NoScriptArgs;
  UnicodeString AssemblyCode;
  bool NoCodeProperties;
  DoGetInfoStr(
    L";", Attrs, Result, SomeAttrIncluded,
    UnicodeString(), ScriptArgs, NoScriptArgs, /*TAssemblyLanguage(0), AssemblyCode, */NoCodeProperties);
  return SomeAttrIncluded;
}

UnicodeString TCopyParamType::GenerateTransferCommandArgs(int Attrs, const UnicodeString & Link, bool & NoScriptArgs) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  UnicodeString ScriptArgs;
  UnicodeString AssemblyCode;
  bool NoCodeProperties;
  DoGetInfoStr(
    L";", Attrs, Result, SomeAttrIncluded,
    Link, ScriptArgs, NoScriptArgs, /*TAssemblyLanguage(0), AssemblyCode, */NoCodeProperties);
  return ScriptArgs;
}

void TCopyParamType::DoGetInfoStr(
  const UnicodeString & Separator, intptr_t Options,
  UnicodeString & Result, bool & SomeAttrIncluded,
  const UnicodeString & Link, UnicodeString & ScriptArgs, bool & NoScriptArgs, /*TAssemblyLanguage Language, UnicodeString & AssemblyCode,*/
  bool & NoCodeProperties) const
{
  TCopyParamType Defaults;

  bool SomeAttrExcluded = false;
  NoScriptArgs = false;
  NoCodeProperties = false;
  SomeAttrIncluded = false;
  #define ADD(STR, EXCEPT) \
    if (FLAGCLEAR(Options, EXCEPT)) \
    { \
      AddToList(Result, (STR), Separator); \
      SomeAttrIncluded = true; \
    } \
    else \
    { \
      SomeAttrExcluded = true; \
    }

  bool AsciiFileMaskDiffers = (GetTransferMode() == tmAutomatic) && !(GetAsciiFileMask() == Defaults.GetAsciiFileMask());
  bool TransferModeDiffers = ((GetTransferMode() != Defaults.GetTransferMode()) || AsciiFileMaskDiffers);

  if (FLAGCLEAR(Options, cpaIncludeMaskOnly | cpaNoTransferMode))
  {
    // Adding Transfer type unconditionally
    bool FormatMask;
    int Ident;
    switch (GetTransferMode())
    {
      case tmBinary:
        FormatMask = false;
        Ident = 2;
        break;
      case tmAscii:
        FormatMask = false;
        Ident = 3;
        break;
      case tmAutomatic:
      default:
        FormatMask = !(GetAsciiFileMask() == Defaults.GetAsciiFileMask());
        Ident = FormatMask ? 4 : 5;
        break;
    }
    UnicodeString S = FORMAT(LoadStrPart(COPY_INFO_TRANSFER_TYPE2, 1).c_str(),
      LoadStrPart(COPY_INFO_TRANSFER_TYPE2, Ident).c_str());
    if (FormatMask)
    {
      S = FORMAT(S.c_str(), GetAsciiFileMask().GetMasks().c_str());
    }
    AddToList(Result, S, Separator);

    if (TransferModeDiffers)
    {
      ADD("", cpaIncludeMaskOnly | cpaNoTransferMode);

      /*ScriptArgs += RtfSwitchValue(TRANSFER_SWITCH, Link, TransferModeNames[TransferMode]);
      const wchar_t * TransferModeMembers[] = { L"Binary", L"Ascii", L"Automatic" };
      AssemblyCode += AssemblyProperty(
        Language, TransferOptionsClassName, L"TransferMode", L"TransferMode", TransferModeMembers[TransferMode], false);
      if (AsciiFileMaskDiffers)
      {
        NoScriptArgs = true;
        NoCodeProperties = true;
      }*/
    }
  }
  else
  {
    if (TransferModeDiffers)
    {
      SomeAttrExcluded = true;
      NoScriptArgs = true;
      NoCodeProperties = true;
    }
  }

  if (GetFileNameCase() != Defaults.GetFileNameCase())
  {
    ADD(FORMAT(LoadStrPart(COPY_INFO_FILENAME, 1).c_str(),
      LoadStrPart(COPY_INFO_FILENAME, GetFileNameCase() + 2).c_str()),
      cpaIncludeMaskOnly);

    NoScriptArgs = true;
    NoCodeProperties = true;
  }

  if ((GetInvalidCharsReplacement() == NoReplacement) !=
        (Defaults.GetInvalidCharsReplacement() == NoReplacement))
  {
    DebugAssert(GetInvalidCharsReplacement() == NoReplacement);
    if (GetInvalidCharsReplacement() == NoReplacement)
    {
      ADD(LoadStr(COPY_INFO_DONT_REPLACE_INV_CHARS).c_str(), cpaIncludeMaskOnly);
    }

    NoScriptArgs = true;
    NoCodeProperties = true;
  }

  if ((GetPreserveRights() != Defaults.GetPreserveRights()) ||
      (GetPreserveRights() &&
       ((GetRights() != Defaults.GetRights()) || (GetAddXToDirectories() != Defaults.GetAddXToDirectories()))))
  {
    const int Except = cpaIncludeMaskOnly | cpaNoRights;
    if (DebugAlwaysTrue(GetPreserveRights()))
    {
      UnicodeString RightsStr = GetRights().GetText();
      if (GetAddXToDirectories())
      {
        RightsStr += L", " + LoadStr(COPY_INFO_ADD_X_TO_DIRS);
      }
      ADD(FORMAT(LoadStr(COPY_INFO_PERMISSIONS).c_str(), RightsStr.c_str()),
        Except);
      if (FLAGCLEAR(Options, Except))
      {
//        ScriptArgs += RtfSwitchValue(PERMISSIONS_SWITCH, Link, Rights.Octal);

//        const UnicodeString FilePermissionsClassName = L"FilePermissions";
//        const bool Inline = true;
//        UnicodeString FilePermissions =
//          AssemblyNewClassInstanceStart(Language, FilePermissionsClassName, Inline) +
//          AssemblyProperty(Language, FilePermissionsClassName, L"Octal", Rights.Octal, Inline) +
//          AssemblyNewClassInstanceEnd(Language, Inline);

//        AssemblyCode += AssemblyPropertyRaw(Language, TransferOptionsClassName, L"FilePermissions", FilePermissions, false);
      }
    }

    if ((GetAddXToDirectories() != Defaults.GetAddXToDirectories()) && FLAGCLEAR(Options, Except))
    {
      NoScriptArgs = true;
      NoCodeProperties = true;
    }
  }

  bool APreserveTimeDirs = GetPreserveTime() && GetPreserveTimeDirs();
  if ((GetPreserveTime() != Defaults.GetPreserveTime()) || (APreserveTimeDirs != Defaults.GetPreserveTimeDirs()))
  {
    bool AddPreserveTime = false;
    UnicodeString Str = LoadStr(GetPreserveTime() ? COPY_INFO_TIMESTAMP : COPY_INFO_DONT_PRESERVE_TIME);

    const int ExceptDirs = cpaNoPreserveTimeDirs;
    if (APreserveTimeDirs != Defaults.GetPreserveTimeDirs())
    {
      if (DebugAlwaysTrue(GetPreserveTimeDirs()))
      {
        if (FLAGCLEAR(Options, ExceptDirs))
        {
          Str = FMTLOAD(COPY_INFO_PRESERVE_TIME_DIRS, (Str));
          AddPreserveTime = true;
        }
      }
      ADD("", ExceptDirs);
    }

    const int Except = cpaIncludeMaskOnly | cpaNoPreserveTime;
    if (GetPreserveTime() != Defaults.GetPreserveTime())
    {
      if (FLAGCLEAR(Options, Except))
      {
        AddPreserveTime = true;
      }
      ADD(L"", Except);
    }

    if (AddPreserveTime)
    {
      AddToList(Result, Str, Separator);
    }

    if (FLAGCLEAR(Options, Except))
    {
      if (GetPreserveTime())
      {
        if (GetPreserveTimeDirs() && FLAGCLEAR(Options, ExceptDirs))
        {
          //ScriptArgs += RtfSwitchValue(PRESERVETIME_SWITCH, Link, PRESERVETIMEDIRS_SWITCH_VALUE);
          NoCodeProperties = true;
        }
        else
        {
          DebugFail(); // should never get here
          //ScriptArgs += RtfSwitch(PRESERVETIME_SWITCH, Link);
        }
      }
      else
      {
//        ScriptArgs += RtfSwitch(NOPRESERVETIME_SWITCH, Link);
//        AssemblyCode += AssemblyProperty(Language, TransferOptionsClassName, L"PreserveTimestamp", false, false);
      }
    }
  }

  if ((GetPreserveRights() || GetPreserveTime()) &&
      (GetIgnorePermErrors() != Defaults.GetIgnorePermErrors()))
  {
    if (DebugAlwaysTrue(GetIgnorePermErrors()))
    {
      const int Except = cpaIncludeMaskOnly | cpaNoIgnorePermErrors;
      ADD(LoadStr(COPY_INFO_IGNORE_PERM_ERRORS), Except);
      if (FLAGCLEAR(Options, Except))
      {
        NoScriptArgs = true;
        NoCodeProperties = true;
      }
    }
  }

  if (GetPreserveReadOnly() != Defaults.GetPreserveReadOnly())
  {
    if (DebugAlwaysTrue(GetPreserveReadOnly()))
    {
      const int Except = cpaIncludeMaskOnly | cpaNoPreserveReadOnly;
      ADD(LoadStr(COPY_INFO_PRESERVE_READONLY), Except);
      if (FLAGCLEAR(Options, Except))
      {
        NoScriptArgs = true;
        NoCodeProperties = true;
      }
    }
  }

  if (GetCalculateSize() != Defaults.GetCalculateSize())
  {
    if (DebugAlwaysTrue(!GetCalculateSize()))
    {
      ADD(LoadStr(COPY_INFO_DONT_CALCULATE_SIZE), cpaIncludeMaskOnly);
      // Always false in scripting, in assembly controlled by use of FileTransferProgress
    }
  }

  if (GetClearArchive() != Defaults.GetClearArchive())
  {
    if (DebugAlwaysTrue(GetClearArchive()))
    {
      const int Except = cpaIncludeMaskOnly | cpaNoClearArchive;
      ADD(LoadStr(COPY_INFO_CLEAR_ARCHIVE), Except);
      if (FLAGCLEAR(Options, Except))
      {
        NoScriptArgs = true;
        NoCodeProperties = true;
      }
    }
  }

  if ((GetTransferMode() == tmAscii) || (GetTransferMode() == tmAutomatic))
  {
    if (GetRemoveBOM() != Defaults.GetRemoveBOM())
    {
      if (DebugAlwaysTrue(GetRemoveBOM()))
      {
        const int Except = cpaIncludeMaskOnly | cpaNoRemoveBOM | cpaNoTransferMode;
        ADD(LoadStr(COPY_INFO_REMOVE_BOM), Except);
        if (FLAGCLEAR(Options, Except))
        {
          NoScriptArgs = true;
          NoCodeProperties = true;
        }
      }
    }

    if (GetRemoveCtrlZ() != Defaults.GetRemoveCtrlZ())
    {
      if (DebugAlwaysTrue(GetRemoveCtrlZ()))
      {
        const int Except = cpaIncludeMaskOnly | cpaNoRemoveCtrlZ | cpaNoTransferMode;
        ADD(LoadStr(COPY_INFO_REMOVE_CTRLZ),Except);
        if (FLAGCLEAR(Options, Except))
        {
          NoScriptArgs = true;
          NoCodeProperties = true;
        }
      }
    }
  }

  if (!(GetIncludeFileMask() == Defaults.GetIncludeFileMask()))
  {
    ADD(FORMAT(LoadStr(COPY_INFO_FILE_MASK).c_str(), GetIncludeFileMask().GetMasks().c_str()),
      cpaNoIncludeMask);

//    ScriptArgs += RtfSwitch(FILEMASK_SWITCH, Link, IncludeFileMask.Masks);
//    AssemblyCode += AssemblyProperty(Language, TransferOptionsClassName, L"FileMask", IncludeFileMask.Masks, false);
  }

  DebugAssert(FTransferSkipList.get() == nullptr);
  DebugAssert(FTransferResumeFile.IsEmpty());

  if (GetCPSLimit() > 0)
  {
    intptr_t LimitKB = intptr_t(GetCPSLimit() / 1024);
    ADD(FMTLOAD(COPY_INFO_CPS_LIMIT2, (LimitKB)), cpaIncludeMaskOnly);

//    ScriptArgs += RtfSwitch(SPEED_SWITCH, Link, LimitKB);
//    AssemblyCode += AssemblyProperty(Language, TransferOptionsClassName, L"Speed", LimitKB, false);
  }

  if (GetNewerOnly() != Defaults.GetNewerOnly())
  {
    if (DebugAlwaysTrue(GetNewerOnly()))
    {
      const int Except = cpaIncludeMaskOnly | cpaNoNewerOnly;
      ADD(StripHotkey(LoadStr(COPY_PARAM_NEWER_ONLY)), Except);
      if (FLAGCLEAR(Options, Except))
      {
//        ScriptArgs += RtfSwitch(NEWERONLY_SWICH, Link);
        NoCodeProperties = true;
      }
    }
  }

  bool ResumeThresholdDiffers = ((GetResumeSupport() == rsSmart) && (GetResumeThreshold() != Defaults.GetResumeThreshold()));
  if (((GetResumeSupport() != Defaults.GetResumeSupport()) || ResumeThresholdDiffers) &&
      (GetTransferMode() != tmAscii) && FLAGCLEAR(Options, cpaNoResumeSupport))
  {
    UnicodeString Value;
    UnicodeString CodeState;
    intptr_t ResumeThresholdKB = (GetResumeThreshold() / 1024);
    switch (GetResumeSupport())
    {
      case rsOff:
        Value = ToggleNames[ToggleOff];
        CodeState = L"Off";
        break;

      case rsOn:
        Value = ToggleNames[ToggleOn];
        CodeState = L"On";
        break;

      case rsSmart:
        Value = IntToStr(ResumeThresholdKB);
        break;
    }
//    ScriptArgs += RtfSwitchValue(RESUMESUPPORT_SWITCH, Link, Value);

    const UnicodeString ResumeSupportClassName = L"TransferResumeSupport";
//    const bool Inline = true;
//    UnicodeString ResumeSupportCode =
//      AssemblyNewClassInstanceStart(Language, ResumeSupportClassName, Inline);
    if (GetResumeSupport() == rsSmart)
    {
//      ResumeSupportCode += AssemblyProperty(Language, ResumeSupportClassName, L"Threshold", ResumeThresholdKB, Inline);
    }
    else
    {
//      ResumeSupportCode += AssemblyProperty(Language, ResumeSupportClassName, L"State", L"TransferResumeSupportState", CodeState, Inline);
    }
//    ResumeSupportCode += AssemblyNewClassInstanceEnd(Language, Inline);

//    AssemblyCode += AssemblyPropertyRaw(Language, TransferOptionsClassName, L"ResumeSupport", ResumeSupportCode, false);
  }

  if (SomeAttrExcluded)
  {
    Result += (Result.IsEmpty() ? UnicodeString() : Separator) +
      FORMAT(LoadStrPart(COPY_INFO_NOT_USABLE, 1).c_str(),
        LoadStrPart(COPY_INFO_NOT_USABLE, (SomeAttrIncluded ? 2 : 3)).c_str());
  }
  else if (Result.IsEmpty())
  {
    Result = LoadStr(COPY_INFO_DEFAULT);
  }
  #undef ADD
}

void TCopyParamType::Assign(const TCopyParamType * Source)
{
  DebugAssert(Source != nullptr);
  #define COPY(Prop) Set ## Prop(Source->Get ## Prop())
  COPY(FileNameCase);
  COPY(PreserveReadOnly);
  COPY(PreserveTime);
  COPY(PreserveTimeDirs);
  COPY(Rights);
  COPY(AsciiFileMask);
  COPY(TransferMode);
  COPY(AddXToDirectories);
  COPY(PreserveRights);
  COPY(IgnorePermErrors);
  COPY(ResumeSupport);
  COPY(ResumeThreshold);
  COPY(InvalidCharsReplacement);
  COPY(LocalInvalidChars);
  COPY(CalculateSize);
  COPY(FileMask);
  COPY(IncludeFileMask);
  COPY(TransferSkipList);
  COPY(TransferResumeFile);
  COPY(ClearArchive);
  COPY(RemoveCtrlZ);
  COPY(RemoveBOM);
  COPY(CPSLimit);
  COPY(NewerOnly);
  #undef COPY
}

TCopyParamType & TCopyParamType::operator =(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}

void TCopyParamType::SetLocalInvalidChars(const UnicodeString & Value)
{
  if (Value != GetLocalInvalidChars())
  {
    FLocalInvalidChars = Value;
    FTokenizibleChars = FLocalInvalidChars; // + TokenPrefix;
  }
}

bool TCopyParamType::GetReplaceInvalidChars() const
{
  return (GetInvalidCharsReplacement() != NoReplacement);
}

void TCopyParamType::SetReplaceInvalidChars(bool Value)
{
  if (GetReplaceInvalidChars() != Value)
  {
    SetInvalidCharsReplacement(Value ? TokenReplacement : NoReplacement);
  }
}

UnicodeString TCopyParamType::ValidLocalFileName(const UnicodeString & AFileName) const
{
  return ::ValidLocalFileName(AFileName, GetInvalidCharsReplacement(), FTokenizibleChars, LOCAL_INVALID_CHARS);
}

UnicodeString TCopyParamType::RestoreChars(const UnicodeString & AFileName) const
{
  UnicodeString FileName = AFileName;
  if (GetInvalidCharsReplacement() == TokenReplacement)
  {
    wchar_t * InvalidChar = const_cast<wchar_t *>(FileName.c_str());
    while ((InvalidChar = wcschr(InvalidChar, TokenPrefix)) != nullptr)
    {
      intptr_t Index = InvalidChar - FileName.c_str() + 1;
      if (FileName.Length() >= Index + 2)
      {
        UnicodeString Hex = FileName.SubString(Index + 1, 2);
        wchar_t Char = static_cast<wchar_t>(HexToByte(Hex));
        if ((Char != L'\0') &&
            ((FTokenizibleChars.Pos(Char) > 0) ||
             (((Char == L' ') || (Char == L'.')) && (Index == FileName.Length() - 2))))
        {
          FileName[Index] = Char;
          FileName.Delete(Index + 1, 2);
          InvalidChar = const_cast<wchar_t *>(FileName.c_str() + Index);
        }
        else if ((Hex == L"00") &&
                 ((Index == FileName.Length() - 2) || (FileName[Index + 3] == L'.')) &&
                 IsReservedName(FileName.SubString(1, Index - 1) + FileName.SubString(Index + 3, FileName.Length() - Index - 3 + 1)))
        {
          FileName.Delete(Index, 3);
          InvalidChar = const_cast<wchar_t *>(FileName.c_str() + Index - 1);
        }
        else
        {
          InvalidChar++;
        }
      }
      else
      {
        InvalidChar++;
      }
    }
  }
  return FileName;
}

UnicodeString TCopyParamType::ValidLocalPath(const UnicodeString & APath) const
{
  UnicodeString Result;
  UnicodeString Path = APath;
  while (!Path.IsEmpty())
  {
    if (!Result.IsEmpty())
    {
      Result += L"\\";
    }
    Result += ValidLocalFileName(CutToChar(Path, L'\\', false));
  }
  return Result;
}

UnicodeString TCopyParamType::ChangeFileName(const UnicodeString & AFileName,
  TOperationSide Side, bool FirstLevel) const
{
  UnicodeString FileName = AFileName;
  if (FirstLevel)
  {
    FileName = MaskFileName(FileName, GetFileMask());
  }
  switch (GetFileNameCase())
  {
    case ncUpperCase:
      FileName = FileName.UpperCase();
      break;
    case ncLowerCase:
      FileName = FileName.LowerCase();
      break;
    case ncFirstUpperCase:
      FileName = FileName.SubString(1, 1).UpperCase() +
        FileName.SubString(2, FileName.Length() - 1).LowerCase();
      break;
    case ncLowerCaseShort:
      if ((FileName.Length() <= 12) && (FileName.Pos(L".") <= 9) &&
          (FileName == FileName.UpperCase()))
      {
        FileName = FileName.LowerCase();
      }
      break;
    case ncNoChange:
    default:
      /*nothing*/
      break;
  }
  if (Side == osRemote)
  {
    FileName = ValidLocalFileName(FileName);
  }
  else
  {
    FileName = RestoreChars(FileName);
  }
  return FileName;
}

bool TCopyParamType::UseAsciiTransfer(const UnicodeString & AFileName,
  TOperationSide Side, const TFileMasks::TParams & Params) const
{
  switch (GetTransferMode())
  {
    case tmBinary:
      return false;
    case tmAscii:
      return true;
    case tmAutomatic:
      return GetAsciiFileMask().Matches(AFileName, (Side == osLocal),
        false, &Params);
    default:
      DebugFail();
      return false;
  }
}

TRights TCopyParamType::RemoteFileRights(uintptr_t Attrs) const
{
  TRights R = GetRights();
  if ((Attrs & faDirectory) && GetAddXToDirectories())
    R.AddExecute();
  return R;
}

UnicodeString TCopyParamType::GetLogStr() const
{
  wchar_t CaseC[] = L"NULFS";
  wchar_t ModeC[] = L"BAM";
  wchar_t ResumeC[] = L"YSN";
  // OpenArray (ARRAYOFCONST) supports only up to 19 arguments, so we had to split it
  return
   FORMAT(
     L"  PrTime: %s%s; PrRO: %s; Rght: %s; PrR: %s (%s); FnCs: %c; RIC: %c; "
        L"Resume: %c (%d); CalcS: %s; Mask: %s\n",
      BooleanToEngStr(GetPreserveTime()).c_str(),
      UnicodeString(GetPreserveTime() && GetPreserveTimeDirs() ? L"+Dirs" : L"").c_str(),
      BooleanToEngStr(GetPreserveReadOnly()).c_str(),
      GetRights().GetText().c_str(),
      BooleanToEngStr(GetPreserveRights()).c_str(),
      BooleanToEngStr(GetIgnorePermErrors()).c_str(),
      CaseC[GetFileNameCase()],
      CharToHex(GetInvalidCharsReplacement()).c_str(),
      ResumeC[GetResumeSupport()],
      (int)GetResumeThreshold(),
      BooleanToEngStr(GetCalculateSize()).c_str(),
      GetFileMask().c_str()) +
    FORMAT(
      L"  TM: %c; ClAr: %s; RemEOF: %s; RemBOM: %s; CPS: %u; NewerOnly: %s; InclM: %s; ResumeL: %d\n"
      L"  AscM: %s\n",
       ModeC[GetTransferMode()],
       BooleanToEngStr(GetClearArchive()).c_str(),
       BooleanToEngStr(GetRemoveCtrlZ()).c_str(),
       BooleanToEngStr(GetRemoveBOM()).c_str(),
       int(GetCPSLimit()),
       BooleanToEngStr(GetNewerOnly()).c_str(),
       GetIncludeFileMask().GetMasks().c_str(),
       ((FTransferSkipList.get() != nullptr) ? FTransferSkipList->GetCount() : 0) + (!FTransferResumeFile.IsEmpty() ? 1 : 0),
       GetAsciiFileMask().GetMasks().c_str());
}

DWORD TCopyParamType::LocalFileAttrs(const TRights & Rights) const
{
  DWORD Result = 0;
  if (GetPreserveReadOnly() && !Rights.GetRight(TRights::rrUserWrite))
  {
    Result |= faReadOnly;
  }
  return Result;
}

bool TCopyParamType::AllowResume(int64_t Size) const
{
  switch (GetResumeSupport())
  {
    case rsOn:
      return true;
    case rsOff:
      return false;
    case rsSmart:
      return (Size >= GetResumeThreshold());
    default:
      DebugFail();
      return false;
  }
}

bool TCopyParamType::AllowAnyTransfer() const
{
  return
    GetIncludeFileMask().GetMasks().IsEmpty() &&
    ((FTransferSkipList.get() == nullptr) || (FTransferSkipList->GetCount() == 0)) &&
    FTransferResumeFile.IsEmpty();
}

bool TCopyParamType::AllowTransfer(const UnicodeString & AFileName,
  TOperationSide Side, bool Directory, const TFileMasks::TParams & Params) const
{
  bool Result = true;
  if (!GetIncludeFileMask().GetMasks().IsEmpty())
  {
    Result = GetIncludeFileMask().Matches(AFileName, (Side == osLocal),
      Directory, &Params);
  }
  return Result;
}

bool TCopyParamType::SkipTransfer(
  const UnicodeString & AFileName, bool Directory) const
{
  bool Result = false;
  // we deliberately do not filter directories, as path is added to resume list
  // when a transfer of file or directory is started,
  // so for directories we need to recurse and check every single file
  if (!Directory && FTransferSkipList.get() != nullptr)
  {
    Result = (FTransferSkipList->IndexOf(AFileName) >= 0);
  }
  return Result;
}

bool TCopyParamType::ResumeTransfer(const UnicodeString & AFileName) const
{
  // Returning true has the same effect as cpResume
  return
    (AFileName == FTransferResumeFile) &&
    DebugAlwaysTrue(!FTransferResumeFile.IsEmpty());
}

TStrings * TCopyParamType::GetTransferSkipList() const
{
  return FTransferSkipList.get();
}

void TCopyParamType::SetTransferSkipList(TStrings * Value)
{
  if ((Value == nullptr) || (Value->GetCount() == 0))
  {
    FTransferSkipList.reset(nullptr);
  }
  else
  {
    FTransferSkipList.reset(new TStringList());
    FTransferSkipList->AddStrings(Value);
    FTransferSkipList->SetSorted(true);
  }
}

void TCopyParamType::Load(THierarchicalStorage * Storage)
{
  SetAddXToDirectories(Storage->ReadBool("AddXToDirectories", GetAddXToDirectories()));
  GetAsciiFileMask().SetMasks(Storage->ReadString("Masks", GetAsciiFileMask().GetMasks()));
  SetFileNameCase(static_cast<TFileNameCase>(Storage->ReadInteger("FileNameCase", GetFileNameCase())));
  SetPreserveReadOnly(Storage->ReadBool("PreserveReadOnly", GetPreserveReadOnly()));
  SetPreserveTime(Storage->ReadBool("PreserveTime", GetPreserveTime()));
  SetPreserveTimeDirs(Storage->ReadBool("PreserveTimeDirs", GetPreserveTimeDirs()));
  SetPreserveRights(Storage->ReadBool("PreserveRights", GetPreserveRights()));
  SetIgnorePermErrors(Storage->ReadBool("IgnorePermErrors", GetIgnorePermErrors()));
  FRights.SetText(Storage->ReadString("Text", GetRights().GetText()));
  SetTransferMode(static_cast<TTransferMode>(Storage->ReadInteger("TransferMode", GetTransferMode())));
  SetResumeSupport(static_cast<TResumeSupport>(Storage->ReadInteger("ResumeSupport", GetResumeSupport())));
  SetResumeThreshold(Storage->ReadInt64("ResumeThreshold", GetResumeThreshold()));
  SetInvalidCharsReplacement(static_cast<wchar_t>(Storage->ReadInteger("ReplaceInvalidChars",
    static_cast<int>(GetInvalidCharsReplacement()))));
  SetLocalInvalidChars(Storage->ReadString("LocalInvalidChars", GetLocalInvalidChars()));
  SetCalculateSize(Storage->ReadBool("CalculateSize", GetCalculateSize()));
  if (Storage->ValueExists("IncludeFileMask"))
  {
    GetIncludeFileMask().SetMasks(Storage->ReadString("IncludeFileMask", GetIncludeFileMask().GetMasks()));
  }
  else if (Storage->ValueExists("ExcludeFileMask"))
  {
    UnicodeString ExcludeFileMask = Storage->ReadString("ExcludeFileMask", UnicodeString());
    if (!ExcludeFileMask.IsEmpty())
    {
      bool NegativeExclude = Storage->ReadBool("NegativeExclude", false);
      if (NegativeExclude)
      {
        GetIncludeFileMask().SetMasks(ExcludeFileMask);
      }
      // convert at least simple cases to new format
      else if (ExcludeFileMask.Pos(INCLUDE_EXCLUDE_FILE_MASKS_DELIMITER) == 0)
      {
        GetIncludeFileMask().SetMasks(UnicodeString(INCLUDE_EXCLUDE_FILE_MASKS_DELIMITER) + ExcludeFileMask);
      }
    }
  }
  SetTransferSkipList(nullptr);
  SetTransferResumeFile(L"");
  SetClearArchive(Storage->ReadBool("ClearArchive", GetClearArchive()));
  SetRemoveCtrlZ(Storage->ReadBool("RemoveCtrlZ", GetRemoveCtrlZ()));
  SetRemoveBOM(Storage->ReadBool("RemoveBOM", GetRemoveBOM()));
  SetCPSLimit(Storage->ReadInteger("CPSLimit", GetCPSLimit()));
  SetNewerOnly(Storage->ReadBool("NewerOnly", GetNewerOnly()));
}

void TCopyParamType::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteBool("AddXToDirectories", GetAddXToDirectories());
  Storage->WriteString("Masks", GetAsciiFileMask().GetMasks());
  Storage->WriteInteger("FileNameCase", GetFileNameCase());
  Storage->WriteBool("PreserveReadOnly", GetPreserveReadOnly());
  Storage->WriteBool("PreserveTime", GetPreserveTime());
  Storage->WriteBool("PreserveTimeDirs", GetPreserveTimeDirs());
  Storage->WriteBool("PreserveRights", GetPreserveRights());
  Storage->WriteBool("IgnorePermErrors", GetIgnorePermErrors());
  Storage->WriteString("Text", GetRights().GetText());
  Storage->WriteInteger("TransferMode", GetTransferMode());
  Storage->WriteInteger("ResumeSupport", GetResumeSupport());

  Storage->WriteInt64("ResumeThreshold", GetResumeThreshold());
  Storage->WriteInteger("ReplaceInvalidChars", static_cast<uint32_t>(GetInvalidCharsReplacement()));
  Storage->WriteString("LocalInvalidChars", GetLocalInvalidChars());
  Storage->WriteBool("CalculateSize", GetCalculateSize());
  Storage->WriteString("IncludeFileMask", GetIncludeFileMask().GetMasks());
  Storage->DeleteValue("ExcludeFileMask"); // obsolete
  Storage->DeleteValue("NegativeExclude"); // obsolete
  DebugAssert(FTransferSkipList.get() == nullptr);
  DebugAssert(FTransferResumeFile.IsEmpty());
  Storage->WriteBool("ClearArchive", GetClearArchive());
  Storage->WriteBool("RemoveCtrlZ", GetRemoveCtrlZ());
  Storage->WriteBool("RemoveBOM", GetRemoveBOM());
  Storage->WriteInteger("CPSLimit", GetCPSLimit());
  Storage->WriteBool("NewerOnly", GetNewerOnly());
}

#define C(Property) (Get ## Property() == rhp.Get ## Property())
bool TCopyParamType::operator==(const TCopyParamType & rhp) const
{
  DebugAssert(FTransferSkipList.get() == nullptr);
  DebugAssert(FTransferResumeFile.IsEmpty());
  DebugAssert(rhp.FTransferSkipList.get() == nullptr);
  DebugAssert(rhp.FTransferResumeFile.IsEmpty());
  return
    C(AddXToDirectories) &&
    C(AsciiFileMask) &&
    C(FileNameCase) &&
    C(PreserveReadOnly) &&
    C(PreserveTime) &&
    C(PreserveTimeDirs) &&
    C(PreserveRights) &&
    C(IgnorePermErrors) &&
    C(Rights) &&
    C(TransferMode) &&
    C(ResumeSupport) &&
    C(ResumeThreshold) &&
    C(InvalidCharsReplacement) &&
    C(LocalInvalidChars) &&
    C(CalculateSize) &&
    C(IncludeFileMask) &&
    C(ClearArchive) &&
    C(RemoveCtrlZ) &&
    C(RemoveBOM) &&
    C(CPSLimit) &&
    C(NewerOnly) &&
    true;
}
#undef C

static bool TryGetSpeedLimit(const UnicodeString & Text, uintptr_t & Speed)
{
  bool Result;
  if (AnsiSameText(Text, LoadStr(SPEED_UNLIMITED)))
  {
    Speed = 0;
    Result = true;
  }
  else
  {
    int64_t SSpeed;
    Result = TryStrToInt(Text, SSpeed) && (SSpeed >= 0);
    if (Result)
    {
      Speed = SSpeed * 1024;
    }
  }
  return Result;
}

uintptr_t GetSpeedLimit(const UnicodeString & Text)
{
  uintptr_t Speed = 0;
  if (!TryGetSpeedLimit(Text, Speed))
  {
    throw Exception(FMTLOAD(SPEED_INVALID, Text.c_str()));
  }
  return Speed;
}

UnicodeString SetSpeedLimit(uintptr_t Limit)
{
  UnicodeString Text;
  if (Limit == 0)
  {
    Text = LoadStr(SPEED_UNLIMITED);
  }
  else
  {
    Text = ::IntToStr(Limit / 1024);
  }
  return Text;
}

void CopySpeedLimits(TStrings * Source, TStrings * Dest)
{
  std::unique_ptr<TStringList> Temp(new TStringList());

  bool Unlimited = false;
  for (intptr_t Index = 0; Index < Source->GetCount(); ++Index)
  {
    UnicodeString Text = Source->GetString(Index);
    uintptr_t Speed;
    bool Valid = TryGetSpeedLimit(Text, Speed);
    if ((!Valid || (Speed == 0)) && !Unlimited)
    {
      Temp->Add(LoadStr(SPEED_UNLIMITED));
      Unlimited = true;
    }
    else if (Valid && (Speed > 0))
    {
      Temp->Add(Text);
    }
  }

  if (!Unlimited)
  {
    Temp->Insert(0, LoadStr(SPEED_UNLIMITED));
  }

  Dest->Assign(Temp.get());
}

NB_IMPLEMENT_CLASS(TCopyParamType, NB_GET_CLASS_INFO(TObject), nullptr)

