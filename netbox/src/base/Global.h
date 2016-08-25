#pragma once

#include <tchar.h>
#include <assert.h>
#include <Classes.hpp>
#include <Sysutils.hpp>

#ifdef __linux__
typedef int64_t __int64;
#define HWND void *
#endif

#define FORMAT(S, ...) ::Format(S, ##__VA_ARGS__)
#define FMTLOAD(Id, ...) ::FmtLoadStr(Id, ##__VA_ARGS__)
#ifndef LENOF
#define LENOF(x) (_countof(X))
#endif
#define FLAGSET(SET, FLAG) (((SET) & (FLAG)) == (FLAG))
#define FLAGCLEAR(SET, FLAG) (((SET) & (FLAG)) == 0)
#define FLAGMASK(ENABLE, FLAG) ((ENABLE) ? (FLAG) : 0)

void ShowExtendedException(Exception * E);
bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages);

class TGuard : public TObject
{
NB_DISABLE_COPY(TGuard)
public:
  explicit TGuard(const TCriticalSection & ACriticalSection);
  ~TGuard();

private:
  const TCriticalSection & FCriticalSection;
};

class TUnguard : public TObject
{
NB_DISABLE_COPY(TUnguard)
public:
  explicit TUnguard(TCriticalSection & ACriticalSection);
  ~TUnguard();

private:
  TCriticalSection & FCriticalSection;
};

#if !defined(_DEBUG)

#define DebugAssert(p) (void)(p)
#define DebugCheck(p) (p)
#define DebugFail() (void)0

#define DebugAlwaysTrue(p) (p)
#define DebugAlwaysFalse(p) (p)
#define DebugNotNull(p) (p)

#else // _DEBUG

void SetTraceFile(HANDLE TraceFile);
void CleanupTracing();
#define TRACEENV "WINSCPTRACE"
extern BOOL IsTracing;
const uintptr_t CallstackTlsOff = (uintptr_t)-1;
extern uintptr_t CallstackTls;
extern "C" void DoTrace(const wchar_t * SourceFile, const wchar_t * Func,
  uintptr_t Line, const wchar_t * Message);
void DoTraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  uintptr_t Line, const wchar_t * Format, va_list Args);

#ifdef TRACE_IN_MEMORY
void TraceDumpToFile();
void TraceInMemoryCallback(const wchar_t * Msg);

#endif // TRACE_IN_MEMORY

#define ACCESS_VIOLATION_TEST { (*((int*)nullptr)) = 0; }

void DoAssert(const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber);

#define DebugAssert(p) ((p) ? (void)0 : DoAssert(TEXT(#p), TEXT(__FILE__), __LINE__))
#define DebugCheck(p) { bool __CHECK_RESULT__ = (p); DebugAssert(__CHECK_RESULT__); }
#define DebugFail() DebugAssert(false)

inline bool DoAlwaysTrue(bool Value, const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber)
{
  if (!Value)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return Value;
}

inline bool DoAlwaysFalse(bool Value, const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber)
{
  if (Value)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return Value;
}

template<typename T>
inline typename T * DoCheckNotNull(T * p, const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber)
{
  if (p == nullptr)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return p;
}

#define DebugAlwaysTrue(p) DoAlwaysTrue((p), TEXT(#p), TEXT(__FILE__), __LINE__)
#define DebugAlwaysFalse(p) DoAlwaysFalse((p), TEXT(#p), TEXT(__FILE__), __LINE__)
#define DebugNotNull(p) DoCheckNotNull((p), TEXT(#p), TEXT(__FILE__), __LINE__)

#endif // _DEBUG

#define DebugUsedParam(p) (void)(p)

#define MB_TEXT(x) const_cast<wchar_t *>(::MB2W(x).c_str())
