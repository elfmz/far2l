#include <colorer/xml/XmlInputSource.h>
#include <colorer/xml/LocalFileXmlInputSource.h>
#if COLORER_FEATURE_JARINPUTSOURCE
# include <colorer/xml/ZipXmlInputSource.h>
#endif
#include <colorer/Exception.h>
#include <xercesc/util/XMLString.hpp>
#ifdef __unix__
#include <dirent.h>
#include <sys/stat.h>
#endif
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif

uXmlInputSource XmlInputSource::newInstance(const XMLCh* path, XmlInputSource* base)
{
  if (xercesc::XMLString::startsWith(path, kJar)) {
#if COLORER_FEATURE_JARINPUTSOURCE
    return std::unique_ptr<ZipXmlInputSource>(new ZipXmlInputSource(path, base));
#else
    throw InputSourceException(CString("ZipXmlInputSource not supported"));
#endif
  }
  if (base) {
    return base->createRelative(path);
  }
  return std::unique_ptr<LocalFileXmlInputSource>(new LocalFileXmlInputSource(path, nullptr));
}

uXmlInputSource XmlInputSource::newInstance(const XMLCh* path, const XMLCh* base)
{
  if (!path || (*path == '\0')) {
    throw InputSourceException(CString("XmlInputSource::newInstance: path is nullptr"));
  }
  if (xercesc::XMLString::startsWith(path, kJar) || (base != nullptr && xercesc::XMLString::startsWith(base, kJar))) {
#if COLORER_FEATURE_JARINPUTSOURCE
    return std::unique_ptr<ZipXmlInputSource>(new ZipXmlInputSource(path, base));
#else
    throw InputSourceException(CString("ZipXmlInputSource not supported"));
#endif
  }
  return std::unique_ptr<LocalFileXmlInputSource>(new LocalFileXmlInputSource(path, base));
}

UString XmlInputSource::getAbsolutePath(const String* basePath, const String* relPath)
{
  int root_pos = basePath->lastIndexOf('/');
  int root_pos2 = basePath->lastIndexOf('\\');
  if (root_pos2 > root_pos) {
    root_pos = root_pos2;
  }
  if (root_pos == -1) {
    root_pos = 0;
  } else {
    root_pos++;
  }
  std::unique_ptr<SString> newPath(new SString());
  newPath->append(CString(basePath, 0, root_pos)).append(relPath);
  return std::move(newPath);
}

XMLCh* XmlInputSource::ExpandEnvironment(const XMLCh* path)
{
#ifdef _WIN32
  size_t i = ExpandEnvironmentStringsW(path, nullptr, 0);
  XMLCh* temp = new XMLCh[i];
  ExpandEnvironmentStringsW(path, temp, static_cast<DWORD>(i));
  return temp;
#else
  //TODO реализовать под nix
  XMLSize_t i = xercesc::XMLString::stringLen(path);
  XMLCh* temp = new XMLCh[i];
  xercesc::XMLString::copyString(temp, path);
  return temp;
#endif
}

bool XmlInputSource::isRelative(const String* path)
{
  if (path->indexOf(':') != String::npos && path->indexOf(':') < 10) return false;
  if (path->indexOf('/') == 0 || path->indexOf('\\') == 0) return false;
  return true;
}

UString XmlInputSource::getClearPath(const String* basePath, const String* relPath)
{
  UString clear_path(new SString(relPath));
  if (relPath->indexOf(CString("%")) != String::npos) {
    XMLCh* e_path = ExpandEnvironment(clear_path.get()->getW2Chars());
    clear_path.reset(new SString(CString(e_path)));
    delete[] e_path;
  }
  if (isRelative(clear_path.get())) {
    clear_path = std::move(getAbsolutePath(basePath, clear_path.get()));
    if (clear_path->startsWith(CString("file://"))) {
      clear_path.reset(new SString(clear_path.get(), 7, -1));
    }
  }
  return clear_path;
}

bool XmlInputSource::isDirectory(const String* path)
{
  bool is_dir = false;
#ifdef _WIN32
  // stat on win_xp and vc2015 have bug.
  DWORD dwAttrs = GetFileAttributesW(path->getWChars());
  if (dwAttrs == INVALID_FILE_ATTRIBUTES) {
    throw Exception(SString("Can't get info for file/path: ") + path);
  }
  else if (dwAttrs & FILE_ATTRIBUTE_DIRECTORY) {
    is_dir = true;
  }
#else

  struct stat st;
  int ret = stat(path->getChars(), &st);

  if (ret == -1) {
    throw Exception(SString("Can't get info for file/path: ") + path);
  }
  else if ((st.st_mode & S_IFDIR)) {
    is_dir = true;
  }
#endif

  return is_dir;
}

#ifdef _WIN32
void XmlInputSource::getFileFromDir(const String* relPath, std::vector<SString> &files)
{
  WIN32_FIND_DATAW ffd;
  HANDLE dir = FindFirstFileW((SString(relPath) + "\\*.*").getWChars(), &ffd);
  if (dir != INVALID_HANDLE_VALUE) {
    while (true) {
      if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        files.push_back(SString(relPath) + "\\" + SString(ffd.cFileName));
      }
      if (FindNextFileW(dir, &ffd) == FALSE) {
        break;
      }
    }
    FindClose(dir);
  }
}
#endif

#ifdef __unix__
void XmlInputSource::getFileFromDir(const String* relPath, std::vector<SString> &files)
{
  DIR* dir = opendir(relPath->getChars());
  if (dir != nullptr) {
    dirent* dire;
    while ((dire = readdir(dir)) != nullptr) {
      struct stat st;
      stat((SString(relPath) + "/" + dire->d_name).getChars(), &st);
      if (!(st.st_mode & S_IFDIR)) {
        files.push_back(SString(relPath) + "/" + dire->d_name);
      }
    }
  }
}
#endif

