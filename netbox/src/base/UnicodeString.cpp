#include <Sysutils.hpp>

#include "UnicodeString.hpp"
#pragma hdrstop

AnsiString::AnsiString(const AnsiString & rht)
{
  Init(rht.c_str(), rht.Length());
}

AnsiString::AnsiString(const wchar_t * Str)
{
  Init(Str, ::StrLength(Str));
}

AnsiString::AnsiString(const wchar_t * Str, intptr_t Size)
{
  Init(Str, Size);
}

AnsiString::AnsiString(const char* Str)
{
  Init(Str, Str ? strlen(Str) : 0);
}

AnsiString::AnsiString(const char * Str, intptr_t Size)
{
  Init(Str, Size);
}

AnsiString::AnsiString(const uint8_t * Str)
{
  Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0);
}

AnsiString::AnsiString(const uint8_t * Str, intptr_t Size)
{
  Init(Str, Size);
}

AnsiString::AnsiString(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

AnsiString::AnsiString(const UTF8String & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

AnsiString::AnsiString(const RawByteString & Str)
{
   Init(Str.c_str(), Str.GetLength());
}

void AnsiString::Init(const wchar_t * Str, intptr_t Length)
{
  intptr_t Size = ::WideCharToMultiByte(CP_UTF8, 0, Str, static_cast<int>(Length > 0 ? Length : -1), nullptr, 0, nullptr, nullptr);
  if (Length > 0)
  {
    Data.resize(Size + 1);
    ::WideCharToMultiByte(CP_UTF8, 0, Str, static_cast<int>(Length > 0 ? Length : -1),
      reinterpret_cast<LPSTR>(const_cast<char *>(Data.c_str())), static_cast<int>(Size), nullptr, nullptr);
    Data[Size] = 0;
    Data = Data.c_str();
  }
  else
  {
    Data.clear();
  }
}

void AnsiString::Init(const char * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<char *>(Data.c_str()), Str, Length);
  }
  Data = Data.c_str();
}

void AnsiString::Init(const uint8_t * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<char *>(Data.c_str()), Str, Length);
  }
  Data = Data.c_str();
}

intptr_t AnsiString::Pos(const AnsiString & Str) const
{
  return static_cast<intptr_t>(Data.find(Str.c_str(), 0, 1)) + 1;
}

intptr_t AnsiString::Pos(wchar_t Ch) const
{
  AnsiString Str(&Ch, 1);
  return static_cast<intptr_t>(Data.find(Str.c_str(), 0, 1)) + 1;
}

char AnsiString::operator [](intptr_t Idx) const
{
  ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
  return Data[Idx-1];
}

char &AnsiString::operator [](intptr_t Idx)
{
  ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
  return Data[Idx-1];
}

AnsiString & AnsiString::Insert(const char * Str, intptr_t Pos)
{
  Data.insert(Pos - 1, Str);
  return *this;
}

AnsiString AnsiString::SubString(intptr_t Pos, intptr_t Len) const
{
  // std::string S = std::string(Data.substr(Pos - 1, Len).c_str(), Len);
  // AnsiString Result(S.c_str(), S.size());
  AnsiString Result(Data.substr(Pos - 1, Len).c_str(), Len);
  return Result;
}

AnsiString & AnsiString::operator=(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator=(const AnsiString & StrCopy)
{
  if (*this != StrCopy)
  {
    Init(StrCopy.c_str(), StrCopy.Length());
  }
  return *this;
}

AnsiString & AnsiString::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator=(const char * lpszData)
{
  if (lpszData)
  {
    Init(lpszData, strlen(lpszData ? lpszData : ""));
  }
  return *this;
}

AnsiString & AnsiString::operator=(const wchar_t * lpwszData)
{
  Init(lpwszData, wcslen(NullToEmpty(lpwszData)));
  return *this;
}

AnsiString & AnsiString::operator +=(const AnsiString & rhs)
{
  Data.append(rhs.c_str(), rhs.Length());
  return *this;
}

AnsiString & AnsiString::operator +=(const char Ch)
{
  Data.append(1, Ch);
  return *this;
}

AnsiString & AnsiString::operator +=(const char * rhs)
{
  Data.append(rhs);
  return *this;
}

void AnsiString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw std::runtime_error("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

void RawByteString::Init(const wchar_t * Str, intptr_t Length)
{
  intptr_t Size = ::WideCharToMultiByte(CP_ACP, 0, Str, static_cast<int>(Length > 0 ? Length : -1), nullptr, 0, nullptr, nullptr);
  if (Length > 0)
  {
    Data.resize(Size + 1);
    ::WideCharToMultiByte(CP_ACP, 0, Str, static_cast<int>(Length > 0 ? Length : -1),
      reinterpret_cast<LPSTR>(const_cast<uint8_t *>(Data.c_str())), static_cast<int>(Size), nullptr, nullptr);
    Data[Size] = 0;
  }
  else
  {
    Data.clear();
  }
}

void RawByteString::Init(const char * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<uint8_t *>(Data.c_str()), Str, Length);
  }
}

void RawByteString::Init(const uint8_t * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<uint8_t *>(Data.c_str()), Str, Length);
  }
}

RawByteString::operator UnicodeString() const
{
  return UnicodeString(reinterpret_cast<const char *>(Data.c_str()), Data.size());
}

intptr_t RawByteString::Pos(wchar_t Ch) const
{
  return Data.find(static_cast<uint8_t>(Ch)) + 1;
}

intptr_t RawByteString::Pos(const char Ch) const
{
  return Data.find(static_cast<uint8_t>(Ch)) + 1;
}

intptr_t RawByteString::Pos(const char * Str) const
{
  return Data.find(reinterpret_cast<const uint8_t *>(Str)) + 1;
}

RawByteString::RawByteString(const wchar_t * Str)
{
  Init(Str, ::StrLength(Str));
}

RawByteString::RawByteString(const wchar_t * Str, intptr_t Size)
{
  Init(Str, Size);
}

RawByteString::RawByteString(const char * Str)
{
  Init(Str, Str ? strlen(Str) : 0);
}

RawByteString::RawByteString(const char * Str, intptr_t Size)
{
  Init(Str, Size);
}

RawByteString::RawByteString(const uint8_t * Str)
{
  Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0);
}

RawByteString::RawByteString(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

RawByteString::RawByteString(const RawByteString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

RawByteString::RawByteString(const AnsiString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

RawByteString::RawByteString(const UTF8String & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

RawByteString & RawByteString::Insert(const char * Str, intptr_t Pos)
{
  Data.insert(Pos - 1, reinterpret_cast<const uint8_t *>(Str));
  return *this;
}

RawByteString RawByteString::SubString(intptr_t Pos, intptr_t Len) const
{
  rawstring_t s = Data.substr(Pos - 1, Len);
  RawByteString Result(s.c_str(), s.size());
  return Result;
}

RawByteString & RawByteString::operator=(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator=(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator=(const AnsiString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator=(const char * lpszData)
{
  if (lpszData)
  {
    Init(lpszData, strlen(lpszData ? lpszData : ""));
  }
  return *this;
}

RawByteString & RawByteString::operator=(const wchar_t * lpwszData)
{
  Init(lpwszData, wcslen(NullToEmpty(lpwszData)));
  return *this;
}

RawByteString RawByteString::operator +(const RawByteString & rhs) const
{
  rawstring_t Result = Data + rhs.Data;
  return RawByteString(reinterpret_cast<const char *>(Result.c_str()), Result.size());
}

RawByteString & RawByteString::operator +=(const RawByteString & rhs)
{
  Data.append(reinterpret_cast<const uint8_t *>(rhs.c_str()), rhs.Length());
  return *this;
}

RawByteString & RawByteString::operator +=(const char Ch)
{
  uint8_t ch(static_cast<uint8_t>(Ch));
  Data.append(1, ch);
  return *this;
}

UTF8String::UTF8String(const UTF8String & rht)
{
  Init(rht.c_str(), rht.Length());
}

void UTF8String::Init(const wchar_t * Str, intptr_t Length)
{
//  Data.resize(Length);
//  if (Length > 0)
//  {
//    wmemmove(const_cast<wchar_t *>(Data.c_str()), Str, Length);
//  }
//  Data = Data.c_str();
  intptr_t Size = ::WideCharToMultiByte(CP_UTF8, 0, Str, static_cast<int>(Length > 0 ? Length : -1), nullptr, 0, nullptr, nullptr);
  Data.resize(Size + 1);
  if (Size > 0)
  {
    ::WideCharToMultiByte(CP_UTF8, 0, Str, -1, const_cast<char *>(Data.c_str()), static_cast<int>(Size), nullptr, nullptr);
    Data[Size] = 0;
  }
  Data = Data.c_str();
}

void UTF8String::Init(const char * Str, intptr_t Length)
{
//  intptr_t Size = ::MultiByteToWideChar(CP_UTF8, 0, Str, static_cast<int>(Length > 0 ? Length : -1), nullptr, 0);
//  Data.resize(Size + 1);
//  if (Size > 0)
//  {
//    ::MultiByteToWideChar(CP_UTF8, 0, Str, -1, const_cast<wchar_t *>(Data.c_str()), static_cast<int>(Size));
//    Data[Size] = 0;
//  }
//  Data = Data.c_str();
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<char *>(Data.c_str()), Str, Length);
  }
  Data = Data.c_str();
}

UTF8String::UTF8String(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

UTF8String::UTF8String(const wchar_t * Str)
{
  Init(Str, ::StrLength(Str));
}

UTF8String::UTF8String(const wchar_t * Str, intptr_t Size)
{
  Init(Str, Size);
}

UTF8String &UTF8String::Delete(intptr_t Index, intptr_t Count)
{
  Data.erase(Index - 1, Count);
  return *this;
}

intptr_t UTF8String::Pos(char Ch) const
{
  return Data.find(Ch) + 1;
}

int UTF8String::vprintf(const char * Format, va_list ArgList)
{
  SetLength(32 * 1024);
  return vsnprintf((char *)Data.c_str(), Data.size(), Format, ArgList);
}

UTF8String & UTF8String::Insert(const wchar_t * Str, intptr_t Pos)
{
  UTF8String UTF8(Str);
  Data.insert(Pos - 1, UTF8);
  return *this;
}

UTF8String UTF8String::SubString(intptr_t Pos, intptr_t Len) const
{
  return UTF8String(Data.substr(Pos - 1, Len).c_str());
}

UTF8String & UTF8String::operator=(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator=(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator=(const char * lpszData)
{
  if (lpszData)
  {
    Init(lpszData, strlen(lpszData ? lpszData : ""));
  }
  return *this;
}

UTF8String & UTF8String::operator=(const wchar_t * lpwszData)
{
  Init(lpwszData, wcslen(NullToEmpty(lpwszData)));
  return *this;
}

UTF8String UTF8String::operator +(const UTF8String & rhs) const
{
  string_t Result = Data + rhs.Data;
  return UTF8String(Result.c_str(), Result.size());
}

UTF8String & UTF8String::operator +=(const UTF8String & rhs)
{
  Data.append(rhs.Data.c_str(), rhs.Length());
  return *this;
}

UTF8String & UTF8String::operator +=(const RawByteString & rhs)
{
  UTF8String s(rhs.c_str(), rhs.Length());
  Data.append(s.Data.c_str(), s.Length());
  return *this;
}

UTF8String & UTF8String::operator +=(const char Ch)
{
  uint8_t ch(static_cast<uint8_t>(Ch));
  Data.append(1, ch);
  return *this;
}

bool operator ==(const UTF8String & lhs, const UTF8String & rhs)
{
  return lhs.Data == rhs.Data;
}

bool operator !=(const UTF8String & lhs, const UTF8String & rhs)
{
  return lhs.Data != rhs.Data;
}

void UnicodeString::Init(const wchar_t * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    wmemmove(const_cast<wchar_t *>(Data.c_str()), Str, Length);
  }
  Data = Data.c_str();
}

void UnicodeString::Init(const char * Str, intptr_t Length)
{
  intptr_t Size = ::MultiByteToWideChar(CP_UTF8, 0, Str, static_cast<int>(Length > 0 ? Length : -1), nullptr, 0);
  Data.resize(Size + 1);
  if (Size > 0)
  {
    ::MultiByteToWideChar(CP_UTF8, 0, Str, -1, const_cast<wchar_t *>(Data.c_str()), static_cast<int>(Size));
    Data[Size] = 0;
  }
  Data = Data.c_str();
}

UnicodeString::UnicodeString(const AnsiString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

UnicodeString & UnicodeString::Lower(intptr_t nStartPos, intptr_t nLength)
{
  Data = ::LowerCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

UnicodeString & UnicodeString::Upper(intptr_t nStartPos, intptr_t nLength)
{
  Data = ::UpperCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

intptr_t UnicodeString::Compare(const UnicodeString & Str) const
{
  return ::AnsiCompare(*this, Str);
}

intptr_t UnicodeString::CompareIC(const UnicodeString & Str) const
{
  return ::AnsiCompareIC(*this, Str);
}

intptr_t UnicodeString::ToInt() const
{
  return ::StrToIntDef(*this, 0);
}

UnicodeString & UnicodeString::Replace(intptr_t Pos, intptr_t Len, const wchar_t * Str, intptr_t DataLen)
{
  Data.replace(Pos - 1, Len, wstring_t(Str, DataLen));
  return *this;
}

UnicodeString & UnicodeString::Append(const char * lpszAdd, UINT CodePage)
{
  Data.append(::MB2W(lpszAdd, CodePage).c_str());
  return *this;
}

UnicodeString & UnicodeString::Insert(intptr_t Pos, const wchar_t * Str, intptr_t StrLen)
{
  Data.insert(Pos - 1, Str, StrLen);
  return *this;
}

intptr_t UnicodeString::Pos(wchar_t Ch) const
{
   return Data.find(Ch) + 1;
}

intptr_t UnicodeString::Pos(const UnicodeString & Str) const
{
   return Data.find(Str.Data) + 1;
}

bool UnicodeString::RPos(intptr_t & nPos, wchar_t Ch, intptr_t nStartPos) const
{
  size_t Pos = Data.find_last_of(Ch, Data.size() - nStartPos);
  nPos = Pos + 1;
  return Pos != std::wstring::npos;
}

UnicodeString UnicodeString::SubStr(intptr_t Pos, intptr_t Len) const
{
  wstring_t Str(Data.substr(Pos - 1, Len));
  return UnicodeString(Str.c_str(), Str.size());
}

UnicodeString UnicodeString::SubString(intptr_t Pos, intptr_t Len) const
{
  return SubStr(Pos, Len);
}

bool UnicodeString::IsDelimiter(const UnicodeString & Chars, intptr_t Pos) const
{
  return ::IsDelimiter(Chars, *this, Pos);
}

intptr_t UnicodeString::LastDelimiter(const UnicodeString & Delimiters) const
{
  return ::LastDelimiter(Delimiters, *this);
}

UnicodeString UnicodeString::Trim() const
{
  return ::Trim(*this);
}

UnicodeString UnicodeString::TrimLeft() const
{
  return ::TrimLeft(*this);
}

UnicodeString UnicodeString::TrimRight() const
{
  return ::TrimRight(*this);
}

void UnicodeString::sprintf(const wchar_t * fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  Data = ::FormatV(fmt, args).c_str();
  va_end(args);
}

UnicodeString & UnicodeString::operator=(const UnicodeString & StrCopy)
{
  if (*this != StrCopy)
  {
    Data = StrCopy.Data;
  }
  return *this;
}

UnicodeString & UnicodeString::operator=(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UnicodeString & UnicodeString::operator=(const AnsiString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  // Data = StrCopy.Data;
  return *this;
}

UnicodeString & UnicodeString::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UnicodeString & UnicodeString::operator=(const wchar_t * Str)
{
  Init(Str, wcslen(NullToEmpty(Str)));
  return *this;
}

UnicodeString & UnicodeString::operator=(const wchar_t Ch)
{
  Init(&Ch, 1);
  return *this;
}

UnicodeString & UnicodeString::operator=(const char * lpszData)
{
  if (lpszData)
  {
    Init(lpszData, strlen(lpszData ? lpszData : ""));
  }
  return *this;
}

UnicodeString UnicodeString::operator +(const UnicodeString & rhs) const
{
  wstring_t Result = Data + rhs.Data;
  return UnicodeString(Result.c_str(), Result.size());
}

UnicodeString & UnicodeString::operator +=(const UnicodeString & rhs)
{
  Data.append(rhs.Data.c_str(), rhs.Length());
  return *this;
}

UnicodeString & UnicodeString::operator +=(const wchar_t * rhs)
{
  Data.append(rhs);
  return *this;
}

UnicodeString & UnicodeString::operator +=(const RawByteString & rhs)
{
  UnicodeString s(rhs.c_str(), rhs.Length());
  Data.append(s.Data.c_str(), s.Length());
  return *this;
}

UnicodeString & UnicodeString::operator +=(const char Ch)
{
  Data.append(1, Ch);
  return *this;
}

UnicodeString & UnicodeString::operator +=(const wchar_t Ch)
{
  Data += Ch;
  return *this;
}

wchar_t UnicodeString::operator [](intptr_t Idx) const
{
  ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
  return Data[Idx-1];
}

wchar_t &UnicodeString::operator [](intptr_t Idx)
{
  ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
  return Data[Idx-1];
}

void UnicodeString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw std::runtime_error("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

UnicodeString operator +(const wchar_t lhs, const UnicodeString & rhs)
{
  return UnicodeString(&lhs, 1) + rhs;
}

UnicodeString operator +(const UnicodeString & lhs, const wchar_t rhs)
{
  return lhs + UnicodeString(rhs);
}

UnicodeString operator +(const wchar_t * lhs, const UnicodeString & rhs)
{
  return UnicodeString(lhs) + rhs;
}

UnicodeString operator +(const UnicodeString & lhs, const wchar_t * rhs)
{
  return lhs + UnicodeString(rhs);
}

UnicodeString operator +(const UnicodeString & lhs, const char * rhs)
{
  return lhs + UnicodeString(rhs);
}

bool operator ==(const UnicodeString & lhs, const wchar_t * rhs)
{
  return wcscmp(lhs.Data.c_str(), NullToEmpty(rhs)) == 0;
}

bool operator ==(const wchar_t * lhs, const UnicodeString & rhs)
{
  return wcscmp(NullToEmpty(lhs), rhs.Data.c_str()) == 0;
}

bool operator !=(const UnicodeString & lhs, const wchar_t * rhs)
{
  return wcscmp(lhs.Data.c_str(), NullToEmpty(rhs)) != 0;
}

bool operator !=(const wchar_t * lhs, const UnicodeString & rhs)
{
  return wcscmp(NullToEmpty(lhs), rhs.Data.c_str()) != 0;
}
