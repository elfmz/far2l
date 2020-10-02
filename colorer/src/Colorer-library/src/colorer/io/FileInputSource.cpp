#include<stdio.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/timeb.h>
#include<fcntl.h>
#include<time.h>

#if defined _WIN32
#include<io.h>
#include<windows.h>
#endif
#if defined __unix__ || defined __GNUC__
#include<unistd.h>
#endif
#ifndef O_BINARY
#define O_BINARY 0x0
#endif

#include<colorer/io/FileInputSource.h>

FileInputSource::FileInputSource(const String *basePath, FileInputSource *base){
  bool prefix = true;
  if (basePath->startsWith(CString("file://"))){
    baseLocation = new SString(basePath, 7, -1);
  }else if (basePath->startsWith(CString("file:/"))){
    baseLocation = new SString(basePath, 6, -1);
  }else if (basePath->startsWith(CString("file:"))){
    baseLocation = new SString(basePath, 5, -1);
  }else{
    if (isRelative(basePath) && base != nullptr)
      baseLocation = getAbsolutePath(base->getLocation(), basePath);
    else
      baseLocation = new SString(basePath);
    prefix = false;
  }
#if defined _WIN32
   // replace the environment variables to their values
  size_t i=ExpandEnvironmentStringsW(baseLocation->getWChars(),nullptr,0);
  wchar_t *temp = new wchar_t[i];
  ExpandEnvironmentStringsW(baseLocation->getWChars(),temp,static_cast<DWORD>(i));
  delete baseLocation;
  baseLocation = new SString(temp);
  delete[] temp;
#endif
  if(prefix && (baseLocation->indexOf(':') == String::npos || baseLocation->indexOf(':') > 10) && !baseLocation->startsWith(CString("/"))){
    SString *n_baseLocation = new SString();
    n_baseLocation->append(CString("/")).append(baseLocation);
    delete baseLocation;
    baseLocation = n_baseLocation;
  }
  stream = nullptr;
}

FileInputSource::~FileInputSource(){
  delete baseLocation;
  delete[] stream;
}
colorer::InputSource *FileInputSource::createRelative(const String *relPath){
  return new FileInputSource(relPath, this);
}

const String *FileInputSource::getLocation() const{
  return baseLocation;
}

const byte *FileInputSource::openStream()
{
  if (stream != nullptr) throw InputSourceException(SString("openStream(): source stream already opened: '")+baseLocation+"'");
#if defined _WIN32
  int source = _wopen(baseLocation->getWChars(), O_BINARY);
#else
  int source = open(baseLocation->getChars(), O_BINARY);
#endif
  if (source == -1)
    throw InputSourceException(SString("Can't open file '")+baseLocation+"'");
  struct stat st;
  fstat(source, &st);
  len = st.st_size;

  stream = new byte[len];
  memset(stream,0, sizeof(byte)*len);
  read(source, stream, len);
  close(source);
  return stream;
}

void FileInputSource::closeStream(){
  if (stream == nullptr) throw InputSourceException(SString("closeStream(): source stream is not yet opened"));
  delete[] stream;
  stream = nullptr;
}

int FileInputSource::length() const{
  if (stream == nullptr)
    throw InputSourceException(CString("length(): stream is not yet opened"));
  return len;
}


