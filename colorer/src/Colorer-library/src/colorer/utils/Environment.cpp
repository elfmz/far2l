#include "colorer/utils/Environment.h"
#ifdef WIN32
#include <windows.h>
#include <cwchar>
#else
#include <regex>
#endif
namespace colorer {
fs::path Environment::to_filepath(const UnicodeString* str)
{
#ifdef _WINDOWS
  fs::path result = UStr::to_stdwstr(str);
#else
  fs::path result = UStr::to_stdstr(str);
#endif
  return result;
}

uUnicodeString Environment::getOSVariable(const UnicodeString& name)
{
#ifdef WIN32
  logger->debug("get system environment '{0}'", name);
  auto str_name = UStr::to_stdwstr(&name);
  size_t sz = 0;
  auto result_error = _wgetenv_s(&sz, nullptr, 0, str_name.c_str());
  if (result_error != 0 || sz == 0) {
    logger->debug("'{0}' not set", name);
    return nullptr;
  }
  std::vector<wchar_t> value(sz);
  result_error = _wgetenv_s(&sz, &value[0], sz, str_name.c_str());
  if (result_error != 0) {
    logger->debug("'{0}' not set", name);
    return nullptr;
  }
  auto result = std::make_unique<UnicodeString>(&value[0], int32_t(sz - 1));
  logger->debug("'{0}' = '{1}'", name, *result);
  return result;
#else
  logger->debug("get system environment '{0}'", name);
  auto str_name = UStr::to_stdstr(&name);
  const char* const value = std::getenv(str_name.c_str());
  if (!value) {
    logger->debug("'{0}' not set", name);
    return nullptr;
  }
  else {
    logger->debug("'{0}' = '{1}'", name, value);
    return std::make_unique<UnicodeString>(value);
  }
#endif
}

uUnicodeString Environment::expandEnvironment(const UnicodeString* path)
{
  logger->debug("expand system environment for '{0}'", *path);
#ifdef WIN32
  std::wstring path_ws = UStr::to_stdwstr(path);
  size_t i = ExpandEnvironmentStringsW(path_ws.c_str(), nullptr, 0);
  auto temp = std::make_unique<wchar_t[]>(i);
  ExpandEnvironmentStringsW(path_ws.c_str(), temp.get(), static_cast<DWORD>(i));
  return std::make_unique<UnicodeString>(temp.get());
#else
  std::smatch matcher;
  std::string result;
  auto text = UStr::to_stdstr(path);
  static const std::regex env_re {R"--(\$\{([^}]+)\})--"};
  while (std::regex_search(text, matcher, env_re)) {
    result += matcher.prefix().str();
    result += std::getenv(matcher[1].str().c_str());
    text = matcher.suffix().str();
  }
  result += text;

  logger->debug("result of expand '{0}'", result);
  return std::make_unique<UnicodeString>(result.c_str());
#endif
}

uUnicodeString Environment::normalizePath(const UnicodeString* path)
{
  return std::make_unique<UnicodeString>(normalizeFsPath(path).c_str());
}

fs::path Environment::normalizeFsPath(const UnicodeString* path)
{
  auto expanded_string = Environment::expandEnvironment(path);
  auto fpath = fs::path(Environment::to_filepath(expanded_string.get()));
  fpath = fpath.lexically_normal();
  if (fs::is_symlink(fpath)) {
    fpath = fs::read_symlink(fpath);
  }
  return fpath;
}

fs::path Environment::getClearFilePath(const UnicodeString* basePath, const UnicodeString* relPath)
{
  fs::path fs_basepath;
  if (basePath && !basePath->isEmpty()) {
    auto clear_basepath = normalizeFsPath(basePath);
    fs_basepath = fs::path(clear_basepath).parent_path();
  }
  auto clear_relpath = normalizeFsPath(relPath);

  fs::path full_path;
  if (fs_basepath.empty()) {
    full_path = clear_relpath;
  }
  else {
    full_path = fs_basepath / clear_relpath;
  }

  full_path = full_path.lexically_normal();

  return full_path;
}

std::vector<UnicodeString> Environment::getFilesFromPath(const UnicodeString* basePath, const UnicodeString* relPath,
                                                         const UnicodeString& extension)
{
  std::vector<UnicodeString> result;
  auto ext = Environment::to_filepath(&extension);
  auto clear_path = Environment::getClearFilePath(basePath, relPath);
  if (fs::is_directory(clear_path)) {
    for (auto& p : fs::directory_iterator(clear_path)) {
      if (fs::is_regular_file(p) && p.path().extension() == ext) {
        result.emplace_back(p.path().c_str());
      }
    }
  }
  else {
    result.emplace_back(clear_path.c_str());
  }

  return result;
}

bool Environment::isRegularFile(const UnicodeString* basePath, const UnicodeString* relPath, UnicodeString& fullPath)
{
  auto clear_path = Environment::getClearFilePath(basePath, relPath);
  fullPath = clear_path.u16string().c_str();
  return fs::is_regular_file(clear_path);
}

}  // namespace colorer