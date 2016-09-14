#include <vcl.h>


#include <Common.h>
#include <Exceptions.h>
#include <FileBuffer.h>
#include <Windows.hpp>

#include "FileInfo.h"

#define DWORD_ALIGN( base, ptr ) \
    ( (LPBYTE)(base) + ((((LPBYTE)(ptr) - (LPBYTE)(base)) + 3) & ~3) )
struct VS_VERSION_INFO_STRUCT32
{
  WORD  wLength;
  WORD  wValueLength;
  WORD  wType;
  WCHAR szKey[1];
};

static uintptr_t VERSION_GetFileVersionInfo_PE(const wchar_t * FileName, uintptr_t DataSize, void * Data)
{
  uintptr_t Len = 0;
#ifndef __linux__
  bool NeedFree = false;
  HMODULE Module = ::GetModuleHandle(FileName);
  if (Module == nullptr)
  {
    Module = ::LoadLibraryEx(FileName, 0, LOAD_LIBRARY_AS_DATAFILE);
    NeedFree = true;
  }
  if (Module == nullptr)
  {
    Len = 0;
  }
  else
  {
    try__finally
    {
      SCOPE_EXIT
      {
        if (NeedFree)
        {
          ::FreeLibrary(Module);
        }
      };
      HRSRC Rsrc = ::FindResource(Module, MAKEINTRESOURCE(VS_VERSION_INFO),
        MAKEINTRESOURCE(VS_FILE_INFO));
      if (Rsrc == nullptr)
      {
      }
      else
      {
        Len = ::SizeofResource(Module, static_cast<HRSRC>(Rsrc));
        HANDLE Mem = ::LoadResource(Module, static_cast<HRSRC>(Rsrc));
        if (Mem == nullptr)
        {
        }
        else
        {
          try__finally
          {
            SCOPE_EXIT
            {
              ::FreeResource(Mem);
            };
            VS_VERSION_INFO_STRUCT32 * VersionInfo = static_cast<VS_VERSION_INFO_STRUCT32 *>(LockResource(Mem));
            const VS_FIXEDFILEINFO * FixedInfo =
              (VS_FIXEDFILEINFO *)DWORD_ALIGN(VersionInfo, VersionInfo->szKey + wcslen(VersionInfo->szKey) + 1);

            if (FixedInfo->dwSignature != VS_FFI_SIGNATURE)
            {
              Len = 0;
            }
            else
            {
              if (Data != nullptr)
              {
                if (DataSize < Len)
                {
                  Len = DataSize;
                }
                if (Len > 0)
                {
                  memmove(Data, VersionInfo, Len);
                }
              }
            }
          }
          __finally
          {
            FreeResource(Mem);
          };
        }
      }
    }
    __finally
    {
      if (NeedFree)
      {
        FreeLibrary(Module);
      }
    };
  }
#endif
  return Len;
}

static uintptr_t GetFileVersionInfoSizeFix(const wchar_t * FileName, DWORD * AHandle)
{
  uintptr_t Len = 0;
#ifndef __linux__
  if (IsWin7())
  {
    *AHandle = 0;
    Len = VERSION_GetFileVersionInfo_PE(FileName, 0, nullptr);

    if (Len != 0)
    {
      Len = (Len * 2) + 4;
    }
  }
  else
  {
    Len = ::GetFileVersionInfoSize(const_cast<wchar_t *>(FileName), AHandle);
  }
#endif
  return Len;
}

bool GetFileVersionInfoFix(const wchar_t * FileName, uint32_t Handle,
  uintptr_t DataSize, void * Data)
{
  bool Result = false;
#ifndef __linux__
  if (IsWin7())
  {
    VS_VERSION_INFO_STRUCT32 * VersionInfo = static_cast<VS_VERSION_INFO_STRUCT32 *>(Data);

    uintptr_t Len = VERSION_GetFileVersionInfo_PE(FileName, DataSize, Data);

    Result = (Len != 0);
    if (Result)
    {
      static const char Signature[] = "FE2X";
      uintptr_t BufSize = static_cast<uintptr_t>(VersionInfo->wLength + strlen(Signature));

      if (DataSize >= BufSize)
      {
        uintptr_t ConvBuf = DataSize - VersionInfo->wLength;
        memmove((static_cast<char *>(Data)) + VersionInfo->wLength, Signature, ConvBuf > 4 ? 4 : ConvBuf );
      }
    }
  }
  else
  {
    Result = ::GetFileVersionInfo(FileName, Handle, static_cast<DWORD>(DataSize), Data) != 0;
  }
#endif
  return Result;
}

// Return pointer to file version info block
void * CreateFileInfo(const UnicodeString & AFileName)
{
  DWORD Handle;
  uintptr_t Size;
  void * Result = nullptr;

  // Get file version info block size
  Size = GetFileVersionInfoSizeFix(AFileName.c_str(), &Handle);
  // If size is valid
  if (Size > 0)
  {
    Result = nb_malloc(Size);
    // Get file version info block
    if (!GetFileVersionInfoFix(AFileName.c_str(), Handle, Size, Result))
    {
      nb_free(Result);
      Result = nullptr;
    }
  }
  else
  {
  }
  return Result;
}

// Free file version info block memory
void FreeFileInfo(void * FileInfo)
{
  if (FileInfo)
    nb_free(FileInfo);
}

typedef TTranslation TTranslations[65536];
typedef TTranslation *PTranslations;

// Return pointer to fixed file version info
TVSFixedFileInfo *GetFixedFileInfo(void * FileInfo)
{
  UINT Len;
  TVSFixedFileInfo *Result = nullptr;
#ifndef __linux__
  if (FileInfo && !::VerQueryValue(FileInfo, L"" WGOOD_SLASH "", reinterpret_cast<void **>(&Result), &Len))
  {
    throw Exception(L"Fixed file info not available");
  }
#endif
  return Result;
}

// Return number of available file version info translations
uint32_t GetTranslationCount(void * FileInfo)
{
  PTranslations P;
  UINT Len = 0;
#ifndef __linux__
  if (!::VerQueryValue(FileInfo, L"" WGOOD_SLASH "VarFileInfo" WGOOD_SLASH "Translation", reinterpret_cast<void **>(&P), &Len))
    throw Exception(L"File info translations not available");
#endif
  return Len / 4;
}

// Return i-th translation in the file version info translation list
TTranslation GetTranslation(void * FileInfo, intptr_t I)
{
  PTranslations P = nullptr;
  UINT Len = 0;
#ifndef __linux__
  if (!::VerQueryValue(FileInfo, L"" WGOOD_SLASH "VarFileInfo" WGOOD_SLASH "Translation", reinterpret_cast<void **>(&P), &Len))
    throw Exception(L"File info translations not available");
  if (I * sizeof(TTranslation) >= Len)
    throw Exception(L"Specified translation not available");
#endif
  return P[I];
}

// Return the name of the specified language
UnicodeString GetLanguage(Word Language)
{
#ifndef __linux__
  uintptr_t Len = 0;
  wchar_t P[256];
  Len = ::VerLanguageName(Language, P, _countof(P));
  if (Len > _countof(P))
    throw Exception(L"Language not available");
  return UnicodeString(P, Len);
#else
  return UnicodeString("RU");
#endif
}

// Return the value of the specified file version info string using the
// specified translation
UnicodeString GetFileInfoString(void * FileInfo,
  TTranslation Translation, const UnicodeString & StringName, bool AllowEmpty)
{
  UnicodeString Result;
#ifndef __linux__
  wchar_t * P;
  UINT Len;

  if (!::VerQueryValue(FileInfo, (UnicodeString(L"" WGOOD_SLASH "StringFileInfo" WGOOD_SLASH "") +
    ::IntToHex(Translation.Language, 4) +
    ::IntToHex(Translation.CharSet, 4) +
    L"" WGOOD_SLASH "" + StringName).c_str(), reinterpret_cast<void **>(&P), &Len))
  {
    if (!AllowEmpty)
    {
      throw Exception(L"Specified file info string not available");
    }
  }
  else
  {
    Result = UnicodeString(P, Len);
    PackStr(Result);
  }
#endif
  return Result;
}

intptr_t CalculateCompoundVersion(intptr_t MajorVer,
  intptr_t MinorVer, intptr_t Release, intptr_t Build)
{
  intptr_t CompoundVer = Build + 10000 * (Release + 100 * (MinorVer +
    100 * MajorVer));
  return CompoundVer;
}

intptr_t StrToCompoundVersion(const UnicodeString & AStr)
{
  UnicodeString S(AStr);
  int64_t MajorVer = StrToInt64(CutToChar(S, L'.', false));
  int64_t MinorVer = StrToInt64(CutToChar(S, L'.', false));
  int64_t Release = S.IsEmpty() ? 0 : StrToInt64(CutToChar(S, L'.', false));
  int64_t Build = S.IsEmpty() ? 0 : StrToInt64(CutToChar(S, L'.', false));
  return CalculateCompoundVersion(MajorVer, MinorVer, Release, Build);
}
