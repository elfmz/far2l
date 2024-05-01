#include "colorer/utils/Environment.h"
#include <filesystem>
#ifdef WIN32
#include <windows.h>
#include <cwchar>
#else
#include <regex>
#endif

uUnicodeString Environment::getOSVariable(const UnicodeString& name)
{
#ifdef _MSC_VER
  logger->debug("get system environment {0}", name);
  auto str_name = UStr::to_stdwstr(&name);
  size_t sz = 0;
  auto result_error = _wgetenv_s(&sz, nullptr, 0, str_name.c_str());
  if (result_error != 0 || sz == 0) {
    logger->debug("{0} not set", name);
    return nullptr;
  }
  std::vector<wchar_t> value(sz);
  result_error = _wgetenv_s(&sz, &value[0], sz, str_name.c_str());
  if (result_error != 0) {
    logger->debug("{0} not set", name);
    return nullptr;
  }
  auto result = std::make_unique<UnicodeString>(&value[0], int32_t(sz - 1));
  logger->debug("{0} = '{1}'", name, *result);
  return result;
#else
  logger->debug("get system environment {0}", name);
  auto str_name = UStr::to_stdstr(&name);
  const char* const value = std::getenv(str_name.c_str());
  if (!value) {
    logger->debug("{0} not set", name);
    return nullptr;
  } else {
    logger->debug("{0} = '{1}'", name, value);
    return std::make_unique<UnicodeString>(value);
  }
#endif
}

uUnicodeString Environment::expandEnvironment(const UnicodeString* path)
{
#ifdef WIN32
  std::wstring path_ws = UStr::to_stdwstr(path);
  size_t i = ExpandEnvironmentStringsW(path_ws.c_str(), nullptr, 0);
  auto temp = std::make_unique<wchar_t[]>(i);
  ExpandEnvironmentStringsW(path_ws.c_str(), temp.get(), static_cast<DWORD>(i));
  return std::make_unique<UnicodeString>(temp.get());
#else
  auto text = UStr::to_stdstr(path);
  static const std::regex env_re {R"--(\$\{([^}]+)\})--"};
  std::smatch match;
  while (std::regex_search(text, match, env_re)) {
    auto const from = match[0];
    text.replace(from.first, from.second, std::getenv(match[1].str().c_str()));
  }
  return std::make_unique<UnicodeString>(text.c_str());
#endif
}

uUnicodeString Environment::normalizePath(const UnicodeString* path)
{
  return std::make_unique<UnicodeString>(normalizeFsPath(path).c_str());
}

std::filesystem::path Environment::normalizeFsPath(const UnicodeString* path)
{
  auto expanded_string = Environment::expandEnvironment(path);
  auto fpath = std::filesystem::path(UStr::to_filepath(expanded_string));
  fpath = fpath.lexically_normal();
  if (std::filesystem::is_symlink(fpath)) {
    fpath = std::filesystem::read_symlink(fpath);
  }
  return fpath;
}
