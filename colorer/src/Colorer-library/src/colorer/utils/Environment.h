#ifndef COLORER_ENVIRONMENT_H
#define COLORER_ENVIRONMENT_H

#include "colorer/Common.h"
#include <filesystem>

class Environment
{
 public:
  static uUnicodeString getOSVariable(const UnicodeString& name);
  static uUnicodeString expandEnvironment(const UnicodeString* path);
  static uUnicodeString normalizePath(const UnicodeString* path);
  static std::filesystem::path normalizeFsPath(const UnicodeString* path);
};

#endif  // COLORER_ENVIRONMENT_H
