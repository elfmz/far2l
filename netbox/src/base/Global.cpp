#include <vcl.h>
#pragma hdrstop

#ifdef _DEBUG
#include <stdio.h>
#include <vector>
#include "Interface.h"
#endif // ifdef _DEBUG

#include <Global.h>

// TGuard

TGuard::TGuard(const TCriticalSection & ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  FCriticalSection.Enter();
}

TGuard::~TGuard()
{
  FCriticalSection.Leave();
}

// TUnguard

TUnguard::TUnguard(TCriticalSection & ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  FCriticalSection.Leave();
}

TUnguard::~TUnguard()
{
  FCriticalSection.Enter();
}

#ifdef _DEBUG

static HANDLE TraceFile = nullptr;
BOOL IsTracing = false;
uintptr_t CallstackTls = CallstackTlsOff;
TCriticalSection * TracingCriticalSection = nullptr;

void SetTraceFile(HANDLE ATraceFile)
{
  TraceFile = ATraceFile;
  IsTracing = (TraceFile != 0);
  if (TracingCriticalSection == nullptr)
  {
    TracingCriticalSection = new TCriticalSection();
  }
}

void CleanupTracing()
{
  if (TracingCriticalSection != nullptr)
  {
    delete TracingCriticalSection;
    TracingCriticalSection = nullptr;
  }
}

#ifdef TRACE_IN_MEMORY

struct TTraceInMemory
{
#ifdef TRACE_IN_MEMORY_NO_FORMATTING
  DWORD Ticks;
  DWORD Thread;
  const wchar_t * SourceFile;
  const wchar_t * Func;
  int Line;
  const wchar_t * Message;
#else
  UTF8String Message;
#endif // TRACE_IN_MEMORY_NO_FORMATTING
};
typedef std::vector<TTraceInMemory> TTracesInMemory;
TTracesInMemory TracesInMemory;

int TraceThreadProc(void *)
{
  Trace(L">");
  try
  {
    do
    {
      Trace(L"2");
      TraceDumpToFile();
      Trace(L"3");
      ::Sleep(60000);
      Trace(L"4");
      // if resuming from sleep causes the previous Sleep to immediately break,
      // make sure we wait a little more before dumping
      ::Sleep(60000);
      Trace(L"5");
    }
    while (true);
  }
  catch (...)
  {
    Trace(L"E");
  }
  TraceExit();
  return 0;
}
#endif // TRACE_IN_MEMORY

#ifdef TRACE_IN_MEMORY_NO_FORMATTING

void DoTrace(const wchar_t * SourceFile, const wchar_t * Func,
  uintptr_t Line, const wchar_t * Message)
{
  if (TracingCriticalSection != nullptr)
  {
    TTraceInMemory TraceInMemory;
    TraceInMemory.Ticks = ::GetTickCount();
    TraceInMemory.Thread = ::GetCurrentThreadId();
    TraceInMemory.SourceFile = SourceFile;
    TraceInMemory.Func = Func;
    TraceInMemory.Line = Line;
    TraceInMemory.Message = Message;

    TGuard Guard(TracingCriticalSection);

    if (TracesInMemory.capacity() == 0)
    {
      TracesInMemory.reserve(100000);
      TThreadID ThreadID;
      StartThread(nullptr, 0, TraceThreadProc, nullptr, 0, ThreadID);
    }

    TracesInMemory.push_back(TraceInMemory);
  }
}

void DoTraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  uintptr_t Line, const wchar_t * AFormat, TVarRec * /*Args*/, const int /*Args_Size*/)
{
  DoTrace(SourceFile, Func, Line, AFormat);
}

#endif // TRACE_IN_MEMORY_NO_FORMATTING

#ifdef TRACE_IN_MEMORY

void TraceDumpToFile()
{
  if (TraceFile != nullptr)
  {
    TGuard Guard(TracingCriticalSection);

    DWORD Written;

    TDateTime N = Now();
    #ifdef TRACE_IN_MEMORY_NO_FORMATTING
    DWORD Ticks = GetTickCount();
    #endif

    const UnicodeString TimestampFormat = L"hh:mm:ss.zzz";
    UnicodeString TimeString = FormatDateTime(TimestampFormat, N);

    UTF8String Buffer = UTF8String(
      FORMAT("[%s] Dumping in-memory tracing =================================\n",
        (TimeString)));
    WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, nullptr);

    TTracesInMemory::const_iterator i = TracesInMemory.begin();
    while (i != TracesInMemory.end())
    {
      #ifdef TRACE_IN_MEMORY_NO_FORMATTING
      const wchar_t * SourceFile = i->SourceFile;
      const wchar_t * Slash = wcsrchr(SourceFile, L'\\');
      if (Slash != nullptr)
      {
        SourceFile = Slash + 1;
      }

      TimeString =
        FormatDateTime(TimestampFormat,
          IncMilliSecond(N, -static_cast<int>(Ticks - i->Ticks)));
      Buffer = UTF8String(FORMAT(L"[%s] [%.4X] [%s:%d:%s] %s\n",
        (TimeString, int(i->Thread), SourceFile,
         i->Line, i->Func, i->Message)));
      WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, nullptr);
      #else
      WriteFile(TraceFile, i->Message.c_str(), i->Message.Length(), &Written, nullptr);
      #endif
      ++i;
    }
    TracesInMemory.clear();

    TimeString = FormatDateTime(TimestampFormat, Now());
    Buffer = UTF8String(
      FORMAT("[%s] Done in-memory tracing =================================\n",
        (TimeString)));
    WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, nullptr);
  }
}

void TraceInMemoryCallback(const wchar_t * Msg)
{
  if (IsTracing)
  {
    DoTrace(L"PAS", L"unk", ::GetCurrentThreadId(), Msg);
  }
}
#endif // TRACE_IN_MEMORY

#ifndef TRACE_IN_MEMORY_NO_FORMATTING

void DoTrace(const wchar_t * SourceFile, const wchar_t * Func,
  uintptr_t Line, const wchar_t * Message)
{
  DebugAssert(IsTracing);

  UnicodeString TimeString;
  // DateTimeToString(TimeString, L"hh:mm:ss.zzz", Now());
  TODO("use Format");
  const wchar_t * Slash = wcsrchr(SourceFile, L'\\');
  if (Slash != nullptr)
  {
    SourceFile = Slash + 1;
  }
  UTF8String Buffer = UTF8String(FORMAT(L"[%s] [%.4X] [%s:%d:%s] %s\n",
    TimeString.c_str(), int(::GetCurrentThreadId()), SourceFile,
     Line, Func, Message));
#ifdef TRACE_IN_MEMORY
  if (TracingCriticalSection != nullptr)
  {
    TTraceInMemory TraceInMemory;
    TraceInMemory.Message = Buffer;

    TGuard Guard(TracingCriticalSection);

    if (TracesInMemory.capacity() == 0)
    {
      TracesInMemory.reserve(100000);
      TThreadID ThreadID;
      StartThread(nullptr, 0, TraceThreadProc, nullptr, 0, ThreadID);
    }

    TracesInMemory.push_back(TraceInMemory);
  }
#else
  DWORD Written;
  WriteFile(TraceFile, Buffer.c_str(), static_cast<DWORD>(Buffer.Length()), &Written, nullptr);
#endif TRACE_IN_MEMORY
}

void DoTraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  uintptr_t Line, const wchar_t * AFormat, va_list Args)
{
  DebugAssert(IsTracing);

  UnicodeString Message = FormatV(AFormat, Args);
  DoTrace(SourceFile, Func, Line, Message.c_str());
}

#endif // TRACE_IN_MEMORY_NO_FORMATTING

void DoAssert(const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber)
{
  if (IsTracing)
  {
    DoTrace(Filename, L"assert", LineNumber, Message);
  }
  _wassert(Message, Filename, (unsigned int)LineNumber);
}

#endif // _DEBUG
