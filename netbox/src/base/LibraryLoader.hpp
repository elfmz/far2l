#pragma once

#include <string>
#include <Classes.hpp>
#include <Global.h>

#ifndef __linux__

class TLibraryLoader : public TObject
{
public:
  explicit TLibraryLoader(const UnicodeString & libraryName, bool AllowFailure = false);
  explicit TLibraryLoader();
  ~TLibraryLoader();

  void Load(const UnicodeString & LibraryName, bool AllowFailure = false);
  void Unload();
  FARPROC GetProcAddress(const AnsiString & ProcedureName);
  FARPROC GetProcAddress(intptr_t ProcedureOrdinal);
  bool Loaded() const { return FHModule != nullptr; }

private:
  HMODULE FHModule;

private:
  TLibraryLoader(const TLibraryLoader &);
  TLibraryLoader & operator = (const TLibraryLoader &);
};

#endif
