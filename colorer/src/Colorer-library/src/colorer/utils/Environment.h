#ifndef COLORER_ENVIRONMENT_H
#define COLORER_ENVIRONMENT_H

#include "colorer/Common.h"
#ifdef COLORER_FEATURE_OLD_COMPILERS
#include "colorer/platform/filesystem.hpp"
namespace fs = ghc::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#include <vector>
namespace colorer {

class Environment
{
 public:
  static uUnicodeString getOSVariable(const UnicodeString& name);
  static uUnicodeString expandEnvironment(const UnicodeString* path);
  static uUnicodeString normalizePath(const UnicodeString* path);
  static fs::path normalizeFsPath(const UnicodeString* path);
  static fs::path getClearFilePath(const UnicodeString* basePath, const UnicodeString* relPath);
  static fs::path to_filepath(const UnicodeString* str);

  static std::vector<UnicodeString> getFilesFromPath(const UnicodeString* basePath, const UnicodeString* relPath,
                                                     const UnicodeString& extension);
  static bool isRegularFile(const UnicodeString* basePath, const UnicodeString* relPath, UnicodeString& fullPath);
};

}  // namespace colorer
#endif  // COLORER_ENVIRONMENT_H
