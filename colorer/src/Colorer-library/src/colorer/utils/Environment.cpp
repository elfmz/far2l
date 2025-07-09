#include "colorer/utils/Environment.h"
#ifdef WIN32
#include <windows.h>
#include <cstdlib>
#include <cwchar>
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

uUnicodeString Environment::getOSEnv(const UnicodeString& name)
{
#ifdef _WINDOWS
  COLORER_LOG_DEBUG("get system environment '%'", name);
  auto str_name = UStr::to_stdwstr(&name);
  size_t sz = 0;
  auto result_error = _wgetenv_s(&sz, nullptr, 0, str_name.c_str());
  if (result_error != 0 || sz == 0) {
    COLORER_LOG_DEBUG("'%' not set", name);
    return nullptr;
  }
  std::vector<wchar_t> value(sz);
  result_error = _wgetenv_s(&sz, &value[0], sz, str_name.c_str());
  if (result_error != 0) {
    COLORER_LOG_DEBUG("'%' not set", name);
    return nullptr;
  }
  auto result = std::make_unique<UnicodeString>(&value[0], int32_t(sz - 1));
  COLORER_LOG_DEBUG("'%' = '%'", name, *result);
  return result;
#else
  COLORER_LOG_DEBUG("get system environment '%'", name);
  auto str_name = UStr::to_stdstr(&name);
  const char* const value = std::getenv(str_name.c_str());
  if (!value) {
    COLORER_LOG_DEBUG("'%' not set", name);
    return nullptr;
  }

  COLORER_LOG_DEBUG("'%' = '%'", name, value);
  return std::make_unique<UnicodeString>(value);
#endif
}

void Environment::setOSEnv(const UnicodeString& name, const UnicodeString& value)
{
#ifdef _WINDOWS
  _putenv_s(UStr::to_stdstr(&name).c_str(), UStr::to_stdstr(&value).c_str());
#else
  setenv(UStr::to_stdstr(&name).c_str(), UStr::to_stdstr(&value).c_str(), 1);
#endif
}

uUnicodeString Environment::normalizePath(const UnicodeString* path)
{
  return std::make_unique<UnicodeString>(normalizeFsPath(path).c_str());
}

fs::path Environment::normalizeFsPath(const UnicodeString* path)
{
  auto expanded_string = expandEnvironment(*path);
  auto fpath = fs::path(to_filepath(&expanded_string));
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
    if (fs::is_regular_file(clear_basepath)) {
      fs_basepath = clear_basepath.parent_path();
    }else {
      fs_basepath = clear_basepath;
    }
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

std::vector<UnicodeString> Environment::getFilesFromPath(const UnicodeString& path)
{
  std::vector<UnicodeString> result;
  auto clear_path = to_filepath(&path);
  if (fs::is_directory(clear_path)) {
    for (auto& p : fs::directory_iterator(clear_path)) {
      if (fs::is_regular_file(p)) {
        result.emplace_back(p.path().c_str());
      }
    }
  }
  return result;
}

bool Environment::isRegularFile(const UnicodeString* basePath, const UnicodeString* relPath, UnicodeString& fullPath)
{
  auto clear_path = Environment::getClearFilePath(basePath, relPath);
  fullPath = clear_path.u16string().c_str();
  return isRegularFile(fullPath);
}

bool Environment::isRegularFile(const UnicodeString& path)
{
  std::error_code ec;
  auto result = fs::is_regular_file(to_filepath(&path), ec);
  if (ec) {
    COLORER_LOG_ERROR("Error on checking file status. File: % , Error: %.", path, ec.message());
  }
  return result;
}

UnicodeString Environment::getAbsolutePath(const UnicodeString& basePath, const UnicodeString& relPath)
{
  auto root_pos = basePath.lastIndexOf('/');
  const auto root_pos2 = basePath.lastIndexOf('\\');
  if (root_pos2 > root_pos) {
    root_pos = root_pos2;
  }
  if (root_pos == -1) {
    root_pos = 0;
  }
  else {
    root_pos++;
  }
  UnicodeString newPath(basePath, 0, root_pos);
  newPath.append(relPath);
  return newPath;
}

UnicodeString Environment::expandSpecialEnvironment(const UnicodeString& path)
{
  COLORER_LOG_DEBUG("expand system environment for '%'", path);

  const auto text = UStr::to_stdstr(&path);
  auto result = expandEnvByRegexp(text, std::regex(R"--(\$([[:alpha:]]\w*)\b)--"));

  COLORER_LOG_DEBUG("result of expand '%'", result);
  return UStr::to_unistr(result);
}

UnicodeString Environment::expandEnvironment(const UnicodeString& path)
{
  COLORER_LOG_DEBUG("expand system environment for '%'", path);
  if (path.isEmpty()) {
    COLORER_LOG_DEBUG("result of expand ''");
    return {};
  }

#ifdef _WINDOWS
  std::wstring path_ws = UStr::to_stdwstr(&path);
  size_t i = ExpandEnvironmentStringsW(path_ws.c_str(), nullptr, 0);
  auto temp = std::make_unique<wchar_t[]>(i);
  ExpandEnvironmentStringsW(path_ws.c_str(), temp.get(), static_cast<DWORD>(i));
  UnicodeString result(temp.get());
  COLORER_LOG_DEBUG("result of expand '%'", result);
  return result;
#else
  const auto text = UStr::to_stdstr(&path);
  auto res = expandEnvByRegexp(text, std::regex(R"--(\$\{([[:alpha:]]\w*)\})--"));
  res = expandEnvByRegexp(res, std::regex(R"--(\$([[:alpha:]]\w*)\b)--"));
  COLORER_LOG_DEBUG("result of expand '%'", res);
  return UStr::to_unistr(res);
#endif
}

uintmax_t Environment::getFileSize(const UnicodeString& path)
{
  return fs::file_size(to_filepath(&path));
}

UnicodeString Environment::getCurrentDir()
{
  auto path = fs::current_path();
  return {path.c_str()};
}

std::string Environment::expandEnvByRegexp(const std::string& path, const std::regex& regex)
{
  std::smatch matcher;
  std::string result;
  auto text = path;
  while (std::regex_search(text, matcher, regex)) {
    result += matcher.prefix().str();
    auto env_value = getOSEnv(matcher[1].str().c_str());
    if (env_value) {
      // add expanded value
      result += UStr::to_stdstr(env_value);
    }
    else {
      // add variable name
      result += matcher[0].str();
    }
    text = matcher.suffix().str();
  }
  result += text;

  return result;
}

}  // namespace colorer