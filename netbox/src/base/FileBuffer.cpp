#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <FileBuffer.h>

const wchar_t * EOLTypeNames = L"LF;CRLF;CR";

char * EOLToStr(TEOLType EOLType)
{
  switch (EOLType)
  {
    case eolLF:
      return (char *)"\n";
    case eolCRLF:
      return (char *)"\r\n";
    case eolCR:
      return (char *)"\r";
    default:
      DebugFail();
      return (char *)"";
  }
}

TFileBuffer::TFileBuffer() :
  FMemory(new TMemoryStream())
{
}

TFileBuffer::~TFileBuffer()
{
  SAFE_DESTROY(FMemory);
}

void TFileBuffer::SetSize(int64_t Value)
{
  if (FMemory->GetSize() != Value)
  {
    FMemory->SetSize(Value);
  }
}

void TFileBuffer::SetPosition(int64_t Value)
{
  FMemory->SetPosition(Value);
}

int64_t TFileBuffer::GetPosition() const
{
  return FMemory->GetPosition();
}

void TFileBuffer::SetMemory(TMemoryStream * Value)
{
  if (FMemory != Value)
  {
    if (FMemory)
    {
      SAFE_DESTROY(FMemory);
    }
    FMemory = Value;
  }
}

int64_t TFileBuffer::ReadStream(TStream * Stream, const int64_t Len, bool ForceLen)
{
  int64_t Result = 0;
  try
  {
    SetSize(GetPosition() + Len);
    // C++5
    // FMemory->SetSize(FMemory->Position + Len);
    if (ForceLen)
    {
      Stream->ReadBuffer(GetData() + GetPosition(), Len);
      Result = Len;
    }
    else
    {
      Result = Stream->Read(GetData() + GetPosition(), Len);
    }
    if (Result != Len)
    {
      SetSize(GetSize() - Len + Result);
    }
    FMemory->Seek(Len, soFromCurrent);
  }
  catch (EReadError &)
  {
    ::RaiseLastOSError();
  }
  return Result;
}

int64_t TFileBuffer::LoadStream(TStream * Stream, const int64_t Len, bool ForceLen)
{
  FMemory->Seek(0, soFromBeginning);
  return ReadStream(Stream, Len, ForceLen);
}

void TFileBuffer::Convert(char * Source, char * Dest, intptr_t Params,
  bool & Token)
{
  DebugAssert(strlen(Source) <= 2);
  DebugAssert(strlen(Dest) <= 2);

  const std::string Bom(CONST_BOM);
  if (FLAGSET(Params, cpRemoveBOM) && (GetSize() >= 3) &&
      (memcmp(GetData(), Bom.c_str(), Bom.size()) == 0))
  {
    Delete(0, 3);
  }

  if (FLAGSET(Params, cpRemoveCtrlZ) && (GetSize() > 0) && ((*(GetData() + GetSize() - 1)) == '\x1A'))
  {
    Delete(GetSize() - 1, 1);
  }

  if (strcmp(Source, Dest) == 0)
  {
    return;
  }

  char * Ptr = GetData();

  // one character source EOL
  if (!Source[1])
  {
    bool PrevToken = Token;
    Token = false;

    for (intptr_t Index = 0; Index < GetSize(); ++Index)
    {
      // EOL already in destination format, make sure to pass it unmodified
      if ((Index < GetSize() - 1) && (*Ptr == Dest[0]) && (*(Ptr+1) == Dest[1]))
      {
        ++Index;
        Ptr++;
      }
      // last buffer ended with the first char of destination 2-char EOL format,
      // which got expanded to full destination format.
      // now we got the second char, so get rid of it.
      else if ((Index == 0) && PrevToken && (*Ptr == Dest[1]))
      {
        Delete(Index, 1);
      }
      // we are ending with the first char of destination 2-char EOL format,
      // append the second char and make sure we strip it from the next buffer, if any
      else if ((*Ptr == Dest[0]) && (Index == GetSize() - 1) && Dest[1])
      {
        Token = true;
        Insert(Index+1, Dest+1, 1);
        ++Index;
        Ptr = GetData() + Index;
      }
      else if (*Ptr == Source[0])
      {
        *Ptr = Dest[0];
        if (Dest[1])
        {
          Insert(Index+1, Dest+1, 1);
          ++Index;
          Ptr = GetData() + Index;
        }
      }
      Ptr++;
    }
  }
  // two character source EOL
  else
  {
    intptr_t Index;
    for (Index = 0; Index < GetSize() - 1; ++Index)
    {
      if ((*Ptr == Source[0]) && (*(Ptr+1) == Source[1]))
      {
        *Ptr = Dest[0];
        if (Dest[1])
        {
          *(Ptr+1) = Dest[1];
          ++Index; ++Ptr;
        }
        else
        {
          Delete(Index+1, 1);
          Ptr = GetData() + Index;
        }
      }
      Ptr++;
    }
    if ((Index < GetSize()) && (*Ptr == Source[0]))
    {
      Delete(Index, 1);
    }
  }
}

void TFileBuffer::Convert(TEOLType Source, TEOLType Dest, intptr_t Params,
  bool & Token)
{
  Convert(EOLToStr(Source), EOLToStr(Dest), Params, Token);
}

void TFileBuffer::Convert(char * Source, TEOLType Dest, intptr_t Params,
  bool & Token)
{
  Convert(Source, EOLToStr(Dest), Params, Token);
}

void TFileBuffer::Convert(TEOLType Source, char * Dest, intptr_t Params,
  bool & Token)
{
  Convert(EOLToStr(Source), Dest, Params, Token);
}

void TFileBuffer::Insert(int64_t Index, const char * Buf, int64_t Len)
{
  SetSize(GetSize() + Len);
  memmove(GetData() + Index + Len, GetData() + Index, static_cast<size_t>(GetSize() - Index - Len));
  memmove(GetData() + Index, Buf, Len);
}

void TFileBuffer::Delete(int64_t Index, int64_t Len)
{
  memmove(GetData() + Index, GetData() + Index + Len, static_cast<size_t>(GetSize() - Index - Len));
  SetSize(GetSize() - Len);
}

void TFileBuffer::WriteToStream(TStream * Stream, const int64_t Len)
{
  DebugAssert(Stream);
  try
  {
    Stream->WriteBuffer(GetData() + GetPosition(), Len);
    FMemory->Seek(Len, soFromCurrent);
  }
  catch (EWriteError &)
  {
    ::RaiseLastOSError();
  }
}

