#pragma once

#include <Classes.hpp>

extern const wchar_t * EOLTypeNames;
enum TEOLType
{
  eolLF,    // \n
  eolCRLF,  // \r\n
  eolCR     // \r
};

const int cpRemoveCtrlZ = 0x01;
const int cpRemoveBOM   = 0x02;

class TFileBuffer : public TObject
{
NB_DISABLE_COPY(TFileBuffer)
public:
  TFileBuffer();
  virtual ~TFileBuffer();
  void Convert(char * Source, char * Dest, intptr_t Params, bool & Token);
  void Convert(TEOLType Source, TEOLType Dest, intptr_t Params, bool & Token);
  void Convert(char * Source, TEOLType Dest, intptr_t Params, bool & Token);
  void Convert(TEOLType Source, char * Dest, intptr_t Params, bool & Token);
  void Insert(int64_t Index, const char * Buf, int64_t Len);
  void Delete(int64_t Index, int64_t Len);
  int64_t LoadStream(TStream * Stream, const int64_t Len, bool ForceLen);
  int64_t ReadStream(TStream * Stream, const int64_t Len, bool ForceLen);
  void WriteToStream(TStream * Stream, const int64_t Len);

public:
  TMemoryStream * GetMemory() const { return FMemory; }
  void SetMemory(TMemoryStream * Value);
  char * GetData() const { return static_cast<char *>(FMemory->GetMemory()); }
  int64_t GetSize() const { return FMemory->GetSize(); }
  void SetSize(int64_t Value);
  void SetPosition(int64_t Value);
  int64_t GetPosition() const;

private:
  TMemoryStream * FMemory;
};

char * EOLToStr(TEOLType EOLType);

