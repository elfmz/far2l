#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>

#if defined WIN32
#include <io.h>
#include <windows.h>
#endif
#if defined __unix__ || defined __GNUC__
#include <unistd.h>
#endif
#ifndef O_BINARY
#define O_BINARY 0x0
#endif

#include "colorer/io/FileInputSource.h"

FileInputSource::FileInputSource(const UnicodeString* basePath, FileInputSource* base)
{
  bool prefix = true;
  if (basePath->startsWith("file://")) {
    baseLocation = new UnicodeString(*basePath, 7);
  } else if (basePath->startsWith("file:/")) {
    baseLocation = new UnicodeString(*basePath, 6);
  } else if (basePath->startsWith("file:")) {
    baseLocation = new UnicodeString(*basePath, 5);
  } else {
    if (isRelative(basePath) && base != nullptr)
      baseLocation = getAbsolutePath(base->getLocation(), basePath);
    else
      baseLocation = new UnicodeString(*basePath);
    prefix = false;
  }
#if defined WIN32
  // replace the environment variables to their values
  size_t i = ExpandEnvironmentStrings(UStr::to_stdstr(baseLocation).c_str(), nullptr, 0);
  char* temp = new char[i];
  ExpandEnvironmentStrings(UStr::to_stdstr(baseLocation).c_str(), temp, static_cast<DWORD>(i));
  delete baseLocation;
  baseLocation = new UnicodeString(temp);
  delete[] temp;
#endif
  if (prefix && (baseLocation->indexOf(':') == -1 || baseLocation->indexOf(':') > 10) && !baseLocation->startsWith("/")) {
    auto* n_baseLocation = new UnicodeString();
    n_baseLocation->append("/").append(*baseLocation);
    delete baseLocation;
    baseLocation = n_baseLocation;
  }
}

FileInputSource::~FileInputSource()
{
  delete baseLocation;
  delete[] stream;
}
colorer::InputSource* FileInputSource::createRelative(const UnicodeString* relPath)
{
  return new FileInputSource(relPath, this);
}

const UnicodeString* FileInputSource::getLocation() const
{
  return baseLocation;
}

const byte* FileInputSource::openStream()
{
  if (stream != nullptr)
    throw InputSourceException("openStream(): source stream already opened: '" + *baseLocation + "'");
#ifdef _MSC_VER
  int source;
  _sopen_s(&source, UStr::to_stdstr(baseLocation).c_str(), _O_BINARY | O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
#else
  int source = open(UStr::to_stdstr(baseLocation).c_str(), O_BINARY);
#endif
  if (source == -1)
    throw InputSourceException("Can't open file '" + *baseLocation + "'");
  struct stat st
  {};
  fstat(source, &st);
  len = st.st_size;

  stream = new byte[len];
  memset(stream, 0, sizeof(byte) * len);
#ifdef _MSC_VER
  if (_read(source, stream, len) != len) {
    throw InputSourceException("Error on read file" + *baseLocation);
  }
  _close(source);
#else
  if (read(source, stream, len) != len) {
    throw InputSourceException("Error on read file" + *baseLocation);
  }
  close(source);
#endif
  return stream;
}

void FileInputSource::closeStream()
{
  if (stream == nullptr)
    throw InputSourceException("closeStream(): source stream is not yet opened");
  delete[] stream;
  stream = nullptr;
}

int FileInputSource::length() const
{
  if (stream == nullptr)
    throw InputSourceException("length(): stream is not yet opened");
  return len;
}
