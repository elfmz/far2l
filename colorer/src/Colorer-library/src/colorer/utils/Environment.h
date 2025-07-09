#ifndef COLORER_ENVIRONMENT_H
#define COLORER_ENVIRONMENT_H

#include <regex>
#include <vector>
#include "colorer/Common.h"
#include "colorer/utils/FileSystems.h"

namespace colorer {

class Environment
{
 public:
  static uUnicodeString getOSEnv(const UnicodeString& name);
  static void setOSEnv(const UnicodeString& name, const UnicodeString& value);
  static uUnicodeString normalizePath(const UnicodeString* path);
  static fs::path normalizeFsPath(const UnicodeString* path);
  static fs::path getClearFilePath(const UnicodeString* basePath, const UnicodeString* relPath);
  static fs::path to_filepath(const UnicodeString* str);

  static std::vector<UnicodeString> getFilesFromPath(const UnicodeString& path);
  static bool isRegularFile(const UnicodeString* basePath, const UnicodeString* relPath, UnicodeString& fullPath);
  static bool isRegularFile(const UnicodeString& path);
  static UnicodeString getAbsolutePath(const UnicodeString& basePath, const UnicodeString& relPath);

  static UnicodeString expandSpecialEnvironment(const UnicodeString& path);
  static UnicodeString expandEnvironment(const UnicodeString& path);

  static uintmax_t getFileSize(const UnicodeString& path);
  static UnicodeString getCurrentDir();
private:
  static std::string expandEnvByRegexp(const std::string& path, const std::regex& regex);
};

}  // namespace colorer
#endif  // COLORER_ENVIRONMENT_H
