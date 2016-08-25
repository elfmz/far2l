#pragma once

#include <stdexcept>
#include <limits>
#include <stdarg.h>
#include <math.h>
#include <vector>
#include <BaseDefs.hpp>

#pragma warning(push, 1)

#include <rtlconsts.h>
#include <headers.hpp>
#include <rtti.hpp>

#pragma warning(pop)

#define NPOS static_cast<intptr_t>(-1)

typedef HANDLE THandle;
typedef DWORD TThreadID;

class Exception;

extern const intptr_t MonthsPerYear;
extern const intptr_t DaysPerWeek;
extern const intptr_t MinsPerHour;
extern const intptr_t MinsPerDay;
extern const intptr_t SecsPerMin;
extern const intptr_t SecsPerHour;
extern const intptr_t HoursPerDay;
extern const intptr_t SecsPerDay;
extern const intptr_t MSecsPerDay;
extern const intptr_t MSecsPerSec;
extern const intptr_t OneSecond;
extern const intptr_t DateDelta;
extern const intptr_t UnixDateDelta;

class TObject;
DEFINE_CALLBACK_TYPE0(TThreadMethod, void);

DEFINE_CALLBACK_TYPE1(TNotifyEvent, void, TObject * /*Sender*/);

void Abort();
void Error(intptr_t Id, intptr_t ErrorId);
void ThrowNotImplemented(intptr_t ErrorId);

class TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DECLARE_CLASS(TObject)
public:
  TObject() {}
  virtual ~TObject() {}
  virtual void Changed() {}

  bool IsKindOf(TObjectClassId ClassId) const;
};

struct TPoint
{
  int x;
  int y;
  TPoint() :
    x(0),
    y(0)
  {}
  TPoint(int ax, int ay) :
    x(ax),
    y(ay)
  {}
};

struct TRect
{
  int Left;
  int Top;
  int Right;
  int Bottom;
  int Width() const { return Right - Left; }
  int Height() const { return Bottom - Top; }
  TRect() :
    Left(0),
    Top(0),
    Right(0),
    Bottom(0)
  {}
  TRect(int left, int top, int right, int bottom) :
    Left(left),
    Top(top),
    Right(right),
    Bottom(bottom)
  {}
  bool operator == (const TRect & other) const
  {
    return
      Left == other.Left &&
      Top == other.Top &&
      Right == other.Right &&
      Bottom == other.Bottom;
  }
  bool operator != (const TRect & other) const
  {
    return !(operator == (other));
  }
  bool operator == (const RECT & other) const
  {
    return
      Left == other.left &&
      Top == other.top &&
      Right == other.right &&
      Bottom == other.bottom;
  }
  bool operator != (const RECT & other) const
  {
    return !(operator == (other));
  }
};

class TPersistent : public TObject
{
NB_DECLARE_CLASS(TPersistent)
public:
  TPersistent();
  virtual ~TPersistent();
  virtual void Assign(const TPersistent * Source);
  virtual TPersistent * GetOwner();
protected:
  virtual void AssignTo(TPersistent * Dest) const;
private:
  void AssignError(const TPersistent * Source);
};

enum TListNotification
{
  lnAdded,
  lnExtracted,
  lnDeleted,
};

typedef intptr_t (CompareFunc)(const void * Item1, const void * Item2);

class TList : public TObject
{
NB_DECLARE_CLASS(TList)
public:
  TList();
  virtual ~TList();
  void * operator [](intptr_t Index) const;
  virtual void * GetItem(intptr_t Index) const { return FList[Index]; }
  virtual void * GetItem(intptr_t Index) { return FList[Index]; }
  void SetItem(intptr_t Index, void * Item);
  intptr_t Add(void * Value);
  void * Extract(void * Item);
  intptr_t Remove(void * Item);
  virtual void Move(intptr_t CurIndex, intptr_t NewIndex);
  virtual void Delete(intptr_t Index);
  void Insert(intptr_t Index, void * Item);
  intptr_t IndexOf(const void * Value) const;
  virtual void Clear();
  virtual void Sort(CompareFunc Func);
  virtual void Notify(void * Ptr, TListNotification Action);
  virtual void Sort();

  intptr_t GetCount() const;
  void SetCount(intptr_t Value);

private:
  std::vector<void *> FList;
};

class TObjectList : public TList
{
NB_DECLARE_CLASS(TObjectList)
public:
  TObjectList();
  virtual ~TObjectList();

  TObject * operator [](intptr_t Index) const;
  TObject * GetObj(intptr_t Index) const;
  void SetItem(intptr_t Index, TObject * Value);
  intptr_t Add(TObject * Value);
  intptr_t Remove(TObject * Value);
  void Extract(TObject * Value);
  virtual void Move(intptr_t Index, intptr_t To);
  virtual void Delete(intptr_t Index);
  void Insert(intptr_t Index, TObject * Value);
  intptr_t IndexOf(const TObject * Value) const;
  virtual void Clear();
  bool GetOwnsObjects() const { return FOwnsObjects; }
  void SetOwnsObjects(bool Value) { FOwnsObjects = Value; }
  virtual void Sort(CompareFunc func);
  virtual void Notify(void * Ptr, TListNotification Action);

private:
  bool FOwnsObjects;
};

enum TDuplicatesEnum
{
  dupAccept,
  dupError,
  dupIgnore
};

class TStream;

class TStrings : public TPersistent
{
NB_DECLARE_CLASS(TStrings)
public:
  TStrings();
  virtual ~TStrings();
  intptr_t Add(const UnicodeString & S, TObject * AObject = nullptr);
  virtual void Delete(intptr_t Index) = 0;
  virtual UnicodeString GetTextStr() const;
  virtual void SetTextStr(const UnicodeString & Text);
  virtual void BeginUpdate();
  virtual void EndUpdate();
  virtual void SetUpdateState(bool Updating);
  virtual intptr_t AddObject(const UnicodeString & S, TObject * AObject);
  virtual void InsertObject(intptr_t Index, const UnicodeString & Key, TObject * AObject);
  bool Equals(const TStrings * Value) const;
  virtual void Clear() = 0;
  void Move(intptr_t CurIndex, intptr_t NewIndex);
  virtual intptr_t IndexOf(const UnicodeString & S) const;
  virtual intptr_t IndexOfName(const UnicodeString & Name) const;
  UnicodeString ExtractName(const UnicodeString & S) const;
  void AddStrings(const TStrings * Strings);
  void Append(const UnicodeString & Value);
  virtual void Insert(intptr_t Index, const UnicodeString & AString, TObject * AObject = nullptr) = 0;
  void SaveToStream(TStream * Stream) const;
  wchar_t GetDelimiter() const { return FDelimiter; }
  void SetDelimiter(wchar_t Value) { FDelimiter = Value; }
  wchar_t GetQuoteChar() const { return FQuoteChar; }
  void SetQuoteChar(wchar_t Value) { FQuoteChar = Value; }
  UnicodeString GetDelimitedText() const;
  void SetDelimitedText(const UnicodeString & Value);
  virtual intptr_t CompareStrings(const UnicodeString & S1, const UnicodeString & S2) const;
  intptr_t GetUpdateCount() const { return FUpdateCount; }
  virtual void Assign(const TPersistent * Source);
  virtual intptr_t GetCount() const = 0;

public:
  virtual TObject * GetObj(intptr_t Index) const = 0;
  virtual void SetObj(intptr_t Index, TObject * AObject) = 0;
  virtual bool GetSorted() const = 0;
  virtual void SetSorted(bool Value) = 0;
  virtual bool GetCaseSensitive() const = 0;
  virtual void SetCaseSensitive(bool Value) = 0;
  void SetDuplicates(TDuplicatesEnum Value);
  UnicodeString GetCommaText() const;
  void SetCommaText(const UnicodeString & Value);
  virtual UnicodeString GetText() const;
  virtual void SetText(const UnicodeString & Text);
  virtual const UnicodeString & GetString(intptr_t Index) const = 0;
  virtual void SetString(intptr_t Index, const UnicodeString & S) = 0;
  const UnicodeString GetName(intptr_t Index) const;
  void SetName(intptr_t Index, const UnicodeString & Value);
  const UnicodeString GetValue(const UnicodeString & Name) const;
  void SetValue(const UnicodeString & Name, const UnicodeString & Value);
  UnicodeString GetValueFromIndex(intptr_t Index) const;

protected:
  TDuplicatesEnum FDuplicates;
  mutable wchar_t FDelimiter;
  mutable wchar_t FQuoteChar;
  intptr_t FUpdateCount;
};

class TStringList;
typedef intptr_t (TStringListSortCompare)(TStringList * List, intptr_t Index1, intptr_t Index2);

class TStringList : public TStrings
{
friend intptr_t StringListCompareStrings(TStringList * List, intptr_t Index1, intptr_t Index2);
NB_DECLARE_CLASS(TStringList)
public:
  TStringList();
  virtual ~TStringList();

  intptr_t Add(const UnicodeString & S);
  virtual intptr_t AddObject(const UnicodeString & S, TObject * AObject);
  void LoadFromFile(const UnicodeString & AFileName);
  TNotifyEvent & GetOnChange() { return FOnChange; }
  void SetOnChange(TNotifyEvent OnChange) { FOnChange = OnChange; }
  TNotifyEvent & GetOnChanging() { return FOnChanging; }
  void SetOnChanging(TNotifyEvent OnChanging) { FOnChanging = OnChanging; }
  void InsertItem(intptr_t Index, const UnicodeString & S, TObject * AObject);
  void QuickSort(intptr_t L, intptr_t R, TStringListSortCompare SCompare);

  virtual void Assign(const TPersistent * Source);
  virtual void Clear();
  virtual bool Find(const UnicodeString & S, intptr_t & Index) const;
  virtual intptr_t IndexOf(const UnicodeString & S) const;
  virtual void Delete(intptr_t Index);
  virtual void InsertObject(intptr_t Index, const UnicodeString & Key, TObject * AObject);
  virtual void Sort();
  virtual void CustomSort(TStringListSortCompare ACompareFunc);

  virtual void SetUpdateState(bool Updating);
  virtual void Changing();
  virtual void Changed();
  virtual void Insert(intptr_t Index, const UnicodeString & S, TObject * AObject = nullptr);
  virtual intptr_t CompareStrings(const UnicodeString & S1, const UnicodeString & S2) const;
  virtual intptr_t GetCount() const;

public:
  virtual TObject * GetObj(intptr_t Index) const;
  virtual void SetObj(intptr_t Index, TObject * AObject);
  virtual bool GetSorted() const { return FSorted; }
  virtual void SetSorted(bool Value);
  virtual bool GetCaseSensitive() const { return FCaseSensitive; }
  virtual void SetCaseSensitive(bool Value);
  virtual const UnicodeString & GetString(intptr_t Index) const;
  virtual void SetString(intptr_t Index, const UnicodeString & S);

private:
  TNotifyEvent FOnChange;
  TNotifyEvent FOnChanging;
  std::vector<UnicodeString> FStrings;
  std::vector<TObject *> FObjects;
  bool FSorted;
  bool FCaseSensitive;

private:
  void ExchangeItems(intptr_t Index1, intptr_t Index2);

private:
  TStringList(const TStringList &);
  TStringList & operator=(const TStringList &);
};

/// TDateTime: number of days since 12/30/1899
class TDateTime : public TObject
{
public:
  TDateTime() :
    FValue(0.0)
  {}
  explicit TDateTime(double Value) :
    FValue(Value)
  {
  }
  explicit TDateTime(uint16_t Hour,
    uint16_t Min, uint16_t Sec, uint16_t MSec);
  TDateTime(const TDateTime & rhs) :
    FValue(rhs.FValue)
  {
  }
  double GetValue() const { return operator double(); }
  TDateTime & operator = (const TDateTime & rhs)
  {
    FValue = rhs.FValue;
    return *this;
  }
  operator double() const
  {
    return FValue;
  }
  TDateTime & operator + (const TDateTime & rhs)
  {
    FValue += rhs.FValue;
    return *this;
  }
  TDateTime & operator += (const TDateTime & rhs)
  {
    FValue += rhs.FValue;
    return *this;
  }
  TDateTime & operator += (double val)
  {
    FValue += val;
    return *this;
  }
  TDateTime & operator - (const TDateTime & rhs)
  {
    FValue -= rhs.FValue;
    return *this;
  }
  TDateTime & operator -= (const TDateTime & rhs)
  {
    FValue -= rhs.FValue;
    return *this;
  }
  TDateTime & operator -= (double val)
  {
    FValue -= val;
    return *this;
  }
  TDateTime & operator = (double Value)
  {
    FValue = Value;
    return *this;
  }
  bool operator == (const TDateTime & rhs);
  bool operator != (const TDateTime & rhs)
  {
    return !(operator == (rhs));
  }
  UnicodeString DateString() const;
  UnicodeString TimeString(bool Short) const;
  UnicodeString FormatString(wchar_t * fmt) const;
  void DecodeDate(uint16_t & Y,
    uint16_t & M, uint16_t & D) const;
  void DecodeTime(uint16_t & H,
    uint16_t & N, uint16_t & S, uint16_t & MS) const;
private:
  double FValue;
};

#define MinDateTime TDateTime(-657434.0)

TDateTime Now();
TDateTime SpanOfNowAndThen(const TDateTime & ANow, const TDateTime & AThen);
double MilliSecondSpan(const TDateTime & ANow, const TDateTime & AThen);
int64_t MilliSecondsBetween(const TDateTime & ANow, const TDateTime & AThen);
int64_t SecondsBetween(const TDateTime & ANow, const TDateTime & AThen);

#ifndef __linux__
class TSHFileInfo : public TObject
{
  typedef DWORD_PTR (WINAPI * TGetFileInfo)(
  _In_ LPCTSTR pszPath,
  DWORD dwFileAttributes,
  _Inout_ SHFILEINFO *psfi,
  UINT cbFileInfo,
  UINT uFlags);

public:
  TSHFileInfo();
  virtual ~TSHFileInfo();

  //get the image's index in the system's image list
  int GetFileIconIndex(const UnicodeString & StrFileName, BOOL bSmallIcon) const;
  int GetDirIconIndex(BOOL bSmallIcon);

  //get file type
  UnicodeString GetFileType(const UnicodeString & StrFileName);

private:
  TGetFileInfo FGetFileInfo;
};
#endif

enum TSeekOrigin
{
  soFromBeginning = 0,
  soFromCurrent = 1,
  soFromEnd = 2
};

class TStream : public TObject
{
public:
  TStream();
  virtual ~TStream();
  virtual int64_t Read(void * Buffer, int64_t Count) = 0;
  virtual int64_t Write(const void * Buffer, int64_t Count) = 0;
  virtual int64_t Seek(int64_t Offset, int Origin) = 0;
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin) = 0;
  void ReadBuffer(void * Buffer, int64_t Count);
  void WriteBuffer(const void * Buffer, int64_t Count);
  int64_t CopyFrom(TStream * Source, int64_t Count);

public:
  int64_t GetPosition()
  {
    return Seek(0, soFromCurrent);
  }
  int64_t GetSize()
  {
    int64_t Pos = Seek(0, soFromCurrent);
    int64_t Result = Seek(0, soFromEnd);
    Seek(Pos, soFromBeginning);
    return Result;
  }

public:
  virtual void SetSize(const int64_t NewSize) = 0;
  void SetPosition(const int64_t Pos)
  {
    Seek(Pos, soFromBeginning);
  }
};

class THandleStream : public TStream
{
NB_DISABLE_COPY(THandleStream)
public:
  explicit THandleStream(HANDLE AHandle);
  virtual ~THandleStream();
  virtual int64_t Read(void * Buffer, int64_t Count);
  virtual int64_t Write(const void * Buffer, int64_t Count);
  virtual int64_t Seek(int64_t Offset, int Origin);
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin);

  HANDLE GetHandle() { return FHandle; }
protected:
  virtual void SetSize(const int64_t NewSize);
protected:
  HANDLE FHandle;
};

class TSafeHandleStream : public THandleStream
{
public:
  explicit TSafeHandleStream(THandle AHandle);
  virtual ~TSafeHandleStream() {}
  virtual int64_t Read(void * Buffer, int64_t Count);
  virtual int64_t Write(const void * Buffer, int64_t Count);
};

class EReadError : public std::runtime_error
{
public:
  explicit EReadError(const char * Msg) :
    std::runtime_error(Msg)
  {}
};

class EWriteError : public std::runtime_error
{
public:
  explicit EWriteError(const char * Msg) :
    std::runtime_error(Msg)
  {}
};

class TMemoryStream : public TStream
{
NB_DISABLE_COPY(TMemoryStream)
public:
  TMemoryStream();
  virtual ~TMemoryStream();
  virtual int64_t Read(void * Buffer, int64_t Count);
  virtual int64_t Seek(int64_t Offset, int Origin);
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin);
  void SaveToStream(TStream * Stream);
  void SaveToFile(const UnicodeString & AFileName);

  void Clear();
  void LoadFromStream(TStream * Stream);
  //void LoadFromFile(const UnicodeString & AFileName);
  int64_t GetSize() const { return FSize; }
  virtual void SetSize(const int64_t NewSize);
  virtual int64_t Write(const void * Buffer, int64_t Count);

  void * GetMemory() const { return FMemory; }

protected:
  void SetPointer(void * Ptr, int64_t Size);
  virtual void * Realloc(int64_t & NewCapacity);
  int64_t GetCapacity() const { return FCapacity; }

private:
  void SetCapacity(int64_t NewCapacity);
private:
  void * FMemory;
  int64_t FSize;
  int64_t FPosition;
  int64_t FCapacity;
};

struct TRegKeyInfo
{
  DWORD NumSubKeys;
  DWORD MaxSubKeyLen;
  DWORD NumValues;
  DWORD MaxValueLen;
  DWORD MaxDataLen;
  FILETIME FileTime;
};

enum TRegDataType
{
  rdUnknown, rdString, rdExpandString, rdInteger, rdBinary
};

struct TRegDataInfo
{
  TRegDataType RegData;
  DWORD DataSize;
};

class TRegistry : public TObject
{
NB_DISABLE_COPY(TRegistry)
public:
  TRegistry();
  ~TRegistry();
  void GetValueNames(TStrings * Names) const;
  void GetKeyNames(TStrings * Names) const;
  void CloseKey();
  bool OpenKey(const UnicodeString & Key, bool CanCreate);
  bool DeleteKey(const UnicodeString & Key);
  bool DeleteValue(const UnicodeString & Value) const;
  bool KeyExists(const UnicodeString & SubKey) const;
  bool ValueExists(const UnicodeString & Value) const;
  bool GetDataInfo(const UnicodeString & ValueName, TRegDataInfo & Value) const;
  TRegDataType GetDataType(const UnicodeString & ValueName) const;
  DWORD GetDataSize(const UnicodeString & Name) const;
  bool ReadBool(const UnicodeString & Name);
  TDateTime ReadDateTime(const UnicodeString & Name);
  double ReadFloat(const UnicodeString & Name) const;
  intptr_t ReadInteger(const UnicodeString & Name) const;
  int64_t ReadInt64(const UnicodeString & Name);
  UnicodeString ReadString(const UnicodeString & Name);
  UnicodeString ReadStringRaw(const UnicodeString & Name);
  size_t ReadBinaryData(const UnicodeString & Name,
    void * Buffer, size_t Size) const;

  void WriteBool(const UnicodeString & Name, bool Value);
  void WriteDateTime(const UnicodeString & Name, const TDateTime & Value);
  void WriteFloat(const UnicodeString & Name, double Value);
  void WriteString(const UnicodeString & Name, const UnicodeString & Value);
  void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  void WriteInteger(const UnicodeString & Name, intptr_t Value);
  void WriteInt64(const UnicodeString & Name, int64_t Value);
  void WriteBinaryData(const UnicodeString & Name,
    const void * Buffer, size_t Size);
private:
  void ChangeKey(HKEY Value, const UnicodeString & APath);
  HKEY GetBaseKey(bool Relative) const;
  HKEY GetKey(const UnicodeString & Key) const;
  void SetCurrentKey(HKEY Value) { FCurrentKey = Value; }
  bool GetKeyInfo(TRegKeyInfo & Value) const;
  int GetData(const UnicodeString & Name, void * Buffer,
    intptr_t BufSize, TRegDataType & RegData) const;
  void PutData(const UnicodeString & Name, const void * Buffer,
    intptr_t BufSize, TRegDataType RegData);

public:
  void SetAccess(uint32_t Value);
  HKEY GetCurrentKey() const;
  HKEY GetRootKey() const;
  void SetRootKey(HKEY ARootKey);

private:
  HKEY FCurrentKey;
  HKEY FRootKey;
  // bool FLazyWrite;
  UnicodeString FCurrentPath;
  bool FCloseRootKey;
  mutable uint32_t FAccess;
};

struct TTimeStamp
{
  int Time; // Number of milliseconds since midnight
  int Date; // One plus number of days since 1/1/0001
};

// FIXME
class TShortCut : public TObject
{
public:
  explicit TShortCut();
  explicit TShortCut(intptr_t Value);
  operator intptr_t() const;
  bool operator < (const TShortCut & rhs) const;
  intptr_t Compare(const TShortCut & rhs) const { return FValue - rhs.FValue; }

private:
  intptr_t FValue;
};

enum TReplaceFlag
{
  rfReplaceAll,
  rfIgnoreCase
};

enum TShiftStateFlag
{
  ssShift, ssAlt, ssCtrl, ssLeft, ssRight, ssMiddle, ssDouble, ssTouch, ssPen
};

inline double Trunc(double Value) { double intpart; modf(Value, &intpart); return intpart; }
inline double Frac(double Value) { double intpart; return modf(Value, &intpart); }
inline double Abs(double Value) { return fabs(Value); }

// forms\InputDlg.cpp
struct TInputDialogData
{
//  TCustomEdit * Edit;
  void * Edit;
};

//typedef void (__closure *TInputDialogInitialize)
//  (TObject * Sender, TInputDialogData * Data);
DEFINE_CALLBACK_TYPE2(TInputDialogInitializeEvent, void,
  TObject * /*Sender*/, TInputDialogData * /*Data*/);

enum TQueryType
{
  qtConfirmation,
  qtWarning,
  qtError,
  qtInformation,
};

struct TMessageParams;

class TGlobalFunctionsIntf
{
public:
  virtual ~TGlobalFunctionsIntf() {}

  virtual HINSTANCE GetInstanceHandle() const = 0;
  virtual UnicodeString GetMsg(intptr_t Id) const = 0;
  virtual UnicodeString GetCurrDirectory() const = 0;
  virtual UnicodeString GetStrVersionNumber() const = 0;
  virtual bool InputDialog(const UnicodeString & ACaption,
    const UnicodeString & APrompt, UnicodeString & Value, const UnicodeString & HelpKeyword,
    TStrings * History, bool PathInput,
    TInputDialogInitializeEvent OnInitialize, bool Echo) = 0;
  virtual uintptr_t MoreMessageDialog(const UnicodeString & Message,
    TStrings * MoreMessages, TQueryType Type, uintptr_t Answers,
      const TMessageParams * Params) = 0;
};

TGlobalFunctionsIntf * GetGlobalFunctions();

