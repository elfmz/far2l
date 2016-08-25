#pragma once

#include <Classes.hpp>

enum TOptionType
{
  otParam,
  otSwitch
};

// typedef void (__closure *TLogOptionEvent)(const UnicodeString & LogStr);
DEFINE_CALLBACK_TYPE1(TLogOptionEvent, void, const UnicodeString & /*LogStr*/);

class TOptions : public TObject
{
public:
  TOptions();

  void Add(const UnicodeString & Option);

  // void ParseParams(const UnicodeString & Params);

  bool FindSwitch(const UnicodeString & Switch);
  bool FindSwitch(const UnicodeString & Switch, UnicodeString & Value);
  bool FindSwitch(const UnicodeString & Switch, UnicodeString & Value, bool & ValueSet);
  bool FindSwitch(const UnicodeString & Switch, intptr_t & ParamsStart,
    intptr_t & ParamsCount);
  bool FindSwitch(const UnicodeString & Switch, TStrings * Params,
    intptr_t ParamsMax = -1);
  bool FindSwitchCaseSensitive(const UnicodeString & Switch);
  bool FindSwitchCaseSensitive(const UnicodeString & Switch, TStrings * Params,
    int ParamsMax = -1);
  void ParamsProcessed(intptr_t Position, intptr_t Count);
  UnicodeString SwitchValue(const UnicodeString & Switch, const UnicodeString & Default = L"");
  bool SwitchValue(const UnicodeString & Switch, bool Default);
  bool SwitchValue(const UnicodeString & Switch, bool Default, bool DefaultOnNonExistence);
  bool UnusedSwitch(UnicodeString & Switch) const;
  bool WasSwitchAdded(UnicodeString & Switch, wchar_t & SwitchMark) const;

  void LogOptions(TLogOptionEvent OnEnumOption);

  /*__property int ParamCount = { read = FParamCount };
  __property UnicodeString Param[int Index] = { read = GetParam };
  __property bool Empty = { read = GetEmpty };*/

  intptr_t GetParamCount() const { return FParamCount; }
  UnicodeString GetParam(intptr_t AIndex);
  void Clear() { FOptions.resize(0); FNoMoreSwitches = false; FParamCount = 0; }
  bool GetEmpty() const;
  UnicodeString GetSwitchMarks() const { return FSwitchMarks; }

protected:
  UnicodeString FSwitchMarks;
  UnicodeString FSwitchValueDelimiters;

  bool FindSwitch(const UnicodeString & Switch,
    UnicodeString & Value, intptr_t & ParamsStart, intptr_t & ParamsCount, bool CaseSensitive, bool & ValueSet);
  bool DoFindSwitch(const UnicodeString & Switch, TStrings * Params,
    intptr_t ParamsMax, bool CaseInsensitive);

private:
  struct TOption : public TObject
  {
    TOption() : Type(otParam), ValueSet(false), Used(false), SwitchMark(0) {}
    UnicodeString Name;
    UnicodeString Value;
    TOptionType Type;
    bool ValueSet;
    bool Used;
    wchar_t SwitchMark;
  };

  typedef std::vector<TOption> TOptionsVector;
  TOptionsVector FOptions;
  TOptionsVector FOriginalOptions;
  intptr_t FParamCount;
  bool FNoMoreSwitches;

  /*UnicodeString __fastcall GetParam(int Index);
  bool __fastcall GetEmpty();*/
};

