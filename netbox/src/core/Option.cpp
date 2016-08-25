#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include "Option.h"
#include "TextsCore.h"

TOptions::TOptions() :
  FSwitchMarks(L"/-"),
  FSwitchValueDelimiters(L"=:"),
  FParamCount(0),
  FNoMoreSwitches(false)
{
}

void TOptions::Add(const UnicodeString & Value)
{
  if (!FNoMoreSwitches &&
      (Value.Length() == 2) &&
      (Value[1] == Value[2]) &&
      (FSwitchMarks.Pos(Value[1]) > 0))
  {
    FNoMoreSwitches = true;
  }
  else
  {
    bool Switch = false;
    intptr_t Index = 0; // shut up
    wchar_t SwitchMark = L'\0';
    if (!FNoMoreSwitches &&
        (Value.Length() >= 2) &&
        (FSwitchMarks.Pos(Value[1]) > 0))
    {
      Index = 2;
      Switch = true;
      SwitchMark = Value[1];
      while (Switch && (Index <= Value.Length()))
      {
        if (Value.IsDelimiter(FSwitchValueDelimiters, Index))
        {
          break;
        }
        // this is to treat /home/martin as parameter, not as switch
        else if ((Value[Index] != L'?') && !IsLetter(Value[Index]))
        {
          Switch = false;
          break;
        }
        ++Index;
      }
    }

    TOption Option;

    if (Switch)
    {
      Option.Type = otSwitch;
      Option.Name = Value.SubString(2, Index - 2);
      Option.Value = Value.SubString(Index + 1, Value.Length());
      Option.ValueSet = (Index <= Value.Length());
    }
    else
    {
      Option.Type = otParam;
      Option.Value = Value;
      Option.ValueSet = false; // unused
      ++FParamCount;
    }

    Option.Used = false;
    Option.SwitchMark = SwitchMark;

    FOptions.push_back(Option);
  }

  FOriginalOptions = FOptions;
}

UnicodeString TOptions::GetParam(intptr_t AIndex)
{
  DebugAssert((AIndex >= 1) && (AIndex <= FParamCount));

  UnicodeString Result;
  size_t Idx = 0;
  while ((Idx < FOptions.size()) && (AIndex > 0))
  {
    if (FOptions[Idx].Type == otParam)
    {
      --AIndex;
      if (AIndex == 0)
      {
        Result = FOptions[Idx].Value;
        FOptions[Idx].Used = true;
      }
    }
    ++Idx;
  }

  return Result;
}

bool TOptions::GetEmpty() const
{
  return FOptions.empty();
}

bool TOptions::FindSwitch(const UnicodeString & Switch,
  UnicodeString & Value, intptr_t & ParamsStart, intptr_t & ParamsCount, bool CaseSensitive, bool & ValueSet)
{
  ParamsStart = 0;
  ValueSet = false;
  intptr_t Index = 0;
  bool Found = false;
  while ((Index < static_cast<intptr_t>(FOptions.size())) && !Found)
  {
    if (FOptions[Index].Type == otParam)
    {
      ParamsStart++;
    }
    else if (FOptions[Index].Type == otSwitch)
    {
      if ((!CaseSensitive && ::AnsiSameText(FOptions[Index].Name, Switch)) ||
          (CaseSensitive && ::AnsiSameStr(FOptions[Index].Name, Switch)))
      {
        Found = true;
        Value = FOptions[Index].Value;
        ValueSet = FOptions[Index].ValueSet;
        FOptions[Index].Used = true;
      }
    }
    ++Index;
  }

  ParamsCount = 0;
  if (Found)
  {
    ParamsStart++;
    while ((Index + ParamsCount < static_cast<intptr_t>(FOptions.size())) &&
           (FOptions[Index + ParamsCount].Type == otParam))
    {
      ParamsCount++;
    }
  }
  else
  {
    ParamsStart = 0;
  }

  return Found;
}

bool TOptions::FindSwitch(const UnicodeString & Switch, UnicodeString & Value)
{
  bool ValueSet;
  return FindSwitch(Switch, Value, ValueSet);
}

bool TOptions::FindSwitch(const UnicodeString & Switch, UnicodeString & Value, bool & ValueSet)
{
  intptr_t ParamsStart;
  intptr_t ParamsCount;
  return FindSwitch(Switch, Value, ParamsStart, ParamsCount, false, ValueSet);
}

bool TOptions::FindSwitch(const UnicodeString & Switch)
{
  UnicodeString Value;
  intptr_t ParamsStart;
  intptr_t ParamsCount;
  bool ValueSet;
  return FindSwitch(Switch, Value, ParamsStart, ParamsCount, false, ValueSet);
}

bool TOptions::FindSwitchCaseSensitive(const UnicodeString & Switch)
{
  UnicodeString Value;
  intptr_t ParamsStart;
  intptr_t ParamsCount;
  bool ValueSet;
  return FindSwitch(Switch, Value, ParamsStart, ParamsCount, true, ValueSet);
}

bool TOptions::FindSwitch(const UnicodeString & Switch,
  TStrings * Params, intptr_t ParamsMax)
{
  return DoFindSwitch(Switch, Params, ParamsMax, false);
}
//---------------------------------------------------------------------------
bool TOptions::FindSwitchCaseSensitive(const UnicodeString & Switch,
  TStrings * Params, int ParamsMax)
{
  return DoFindSwitch(Switch, Params, ParamsMax, true);
}

bool TOptions::DoFindSwitch(const UnicodeString & Switch,
  TStrings * Params, intptr_t ParamsMax, bool CaseSensitive)
{
  UnicodeString Value;
  intptr_t ParamsStart;
  intptr_t ParamsCount;
  bool ValueSet;
  bool Result = FindSwitch(Switch, Value, ParamsStart, ParamsCount, CaseSensitive, ValueSet);
  if (Result)
  {
    if ((ParamsMax >= 0) && (ParamsCount > ParamsMax))
    {
      ParamsCount = ParamsMax;
    }

    intptr_t Index = 0;
    while (Index < ParamsCount)
    {
      Params->Add(GetParam(ParamsStart + Index));
      ++Index;
    }
    ParamsProcessed(ParamsStart, ParamsCount);
  }
  return Result;
}

UnicodeString TOptions::SwitchValue(const UnicodeString & Switch,
  const UnicodeString & Default)
{
  UnicodeString Value;
  FindSwitch(Switch, Value);
  if (Value.IsEmpty())
  {
    Value = Default;
  }
  return Value;
}

bool TOptions::SwitchValue(const UnicodeString & Switch, bool Default, bool DefaultOnNonExistence)
{
  bool Result = false;
  int64_t IntValue = 0;
  UnicodeString Value;
  if (!FindSwitch(Switch, Value))
  {
    Result = DefaultOnNonExistence;
  }
  else if (Value.IsEmpty())
  {
    Result = Default;
  }
  else if (::SameText(Value, L"on"))
  {
    Result = true;
  }
  else if (::SameText(Value, L"off"))
  {
    Result = false;
  }
  else if (::TryStrToInt(Value, IntValue))
  {
    Result = (IntValue != 0);
  }
  else
  {
    throw Exception(FMTLOAD(URL_OPTION_BOOL_VALUE_ERROR, Value.c_str()));
  }
  return Result;
}

bool TOptions::SwitchValue(const UnicodeString & Switch, bool Default)
{
  return SwitchValue(Switch, Default, Default);
}

bool TOptions::UnusedSwitch(UnicodeString & Switch) const
{
  bool Result = false;
  size_t Index = 0;
  while (!Result && (Index < FOptions.size()))
  {
    if ((FOptions[Index].Type == otSwitch) &&
        !FOptions[Index].Used)
    {
      Switch = FOptions[Index].Name;
      Result = true;
    }
    ++Index;
  }

  return Result;
}

bool TOptions::WasSwitchAdded(UnicodeString & Switch, wchar_t & SwitchMark) const
{
  bool Result =
    DebugAlwaysTrue(FOptions.size() > 0) &&
    (FOptions.back().Type == otSwitch);
  if (Result)
  {
    const TOption & Option = FOptions.back();
    Switch = Option.Name;
    SwitchMark = Option.SwitchMark;
  }
  return Result;
}

void TOptions::ParamsProcessed(intptr_t ParamsStart, intptr_t ParamsCount)
{
  if (ParamsCount > 0)
  {
    DebugAssert((ParamsStart >= 0) && ((ParamsStart - ParamsCount + 1) <= FParamCount));

    size_t Index = 0;
    while ((Index < FOptions.size()) && (ParamsStart > 0))
    {
      if (FOptions[Index].Type == otParam)
      {
        --ParamsStart;

        if (ParamsStart == 0)
        {
          while (ParamsCount > 0)
          {
            DebugAssert(Index < FOptions.size());
            DebugAssert(FOptions[Index].Type == otParam);
            FOptions.erase(FOptions.begin() + Index);
            --FParamCount;
            --ParamsCount;
          }
        }
      }
      ++Index;
    }
  }
}

void TOptions::LogOptions(TLogOptionEvent OnLogOption)
{
  for (size_t Index = 0; Index < FOriginalOptions.size(); ++Index)
  {
    const TOption & Option = FOriginalOptions[Index];
    UnicodeString LogStr;
    switch (Option.Type)
    {
      case otParam:
        LogStr = FORMAT(L"Parameter: %s", Option.Value.c_str());
        DebugAssert(Option.Name.IsEmpty());
        break;

      case otSwitch:
        LogStr =
          FORMAT(L"Switch:    %c%s%s%s",
            FSwitchMarks[1], Option.Name.c_str(), (Option.Value.IsEmpty() ? UnicodeString() : FSwitchValueDelimiters.SubString(1, 1)).c_str(), Option.Value.c_str());
        break;

      default:
        DebugFail();
        break;
    }
    OnLogOption(LogStr);
  }
}
