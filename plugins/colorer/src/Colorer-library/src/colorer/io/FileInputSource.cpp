#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>
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
#ifndef O_RDONLY
#define O_RDONLY 0
#endif

#include "colorer/io/FileInputSource.h"

namespace {

struct FileDescriptor {
  int fd = -1;

  FileDescriptor() = default;
  ~FileDescriptor()
  {
    close();
  }
  FileDescriptor(const FileDescriptor&) = delete;
  FileDescriptor& operator=(const FileDescriptor&) = delete;

  void close()
  {
    if (fd == -1) {
      return;
    }
#ifdef _MSC_VER
    _close(fd);
#else
    ::close(fd);
#endif
    fd = -1;
  }
};

}  // namespace

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

  FileDescriptor source;
#ifdef _MSC_VER
  if (_sopen_s(&source.fd, UStr::to_stdstr(baseLocation).c_str(), _O_BINARY | O_RDONLY, _SH_DENYNO,
               _S_IREAD | _S_IWRITE) != 0 ||
      source.fd == -1)
#else
  source.fd = open(UStr::to_stdstr(baseLocation).c_str(), O_RDONLY | O_BINARY);
  if (source.fd == -1)
#endif
  {
    throw InputSourceException("Can't open file '" + *baseLocation + "'");
  }

  struct stat st {};
  if (fstat(source.fd, &st) != 0) {
    throw InputSourceException("Can't stat file '" + *baseLocation + "'");
  }
  len = static_cast<int>(st.st_size);
  if (len < 0) {
    throw InputSourceException("File is too large '" + *baseLocation + "'");
  }

  stream = new byte[len == 0 ? 1 : len];
  if (len == 0) {
    stream[0] = 0;
    return stream;
  }
  memset(stream, 0, sizeof(byte) * static_cast<size_t>(len));

  int got = 0;
  while (got < len) {
#ifdef _MSC_VER
    const int n = _read(source.fd, stream + got, static_cast<unsigned>(len - got));
#else
    const int n = static_cast<int>(read(source.fd, stream + got, static_cast<size_t>(len - got)));
#endif
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      delete[] stream;
      stream = nullptr;
      len = 0;
      throw InputSourceException("Error on read file '" + *baseLocation + "'");
    }
    if (n == 0) {
      delete[] stream;
      stream = nullptr;
      len = 0;
      throw InputSourceException("Error on read file '" + *baseLocation + "'");
    }
    got += n;
  }
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
