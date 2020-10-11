
#include<colorer/io/InputSource.h>

#include<colorer/io/FileInputSource.h>

#if COLORER_FEATURE_JARINPUTSOURCE
#include<colorer/io/JARInputSource.h>
#endif

#if COLORER_FEATURE_HTTPINPUTSOURCE
#include<colorer/io/HTTPInputSource.h>
#endif

using namespace colorer;

String *InputSource::getAbsolutePath(const String*basePath, const String*relPath){
  int root_pos = basePath->lastIndexOf('/');
  int root_pos2 = basePath->lastIndexOf('\\');
  if (root_pos2 > root_pos) root_pos = root_pos2;
  if (root_pos == -1) root_pos = 0;
  else root_pos++;
  SString *newPath = new SString();
  newPath->append(CString(basePath, 0, root_pos)).append(relPath);
  return newPath;
}

InputSource *InputSource::newInstance(const String *path){
  return newInstance(path, nullptr);
}

InputSource *InputSource::newInstance(const String *path, InputSource *base){
  if (path == nullptr){
    throw InputSourceException(CString("InputSource::newInstance: path is nullptr"));
  }
  if (path->startsWith(CString("http://"))){
#if COLORER_FEATURE_HTTPINPUTSOURCE
    return new HTTPInputSource(path, nullptr);
#else
    throw InputSourceException(CString("InputSource::newInstance: http not supported"));
#endif
  }
  if (path->startsWith(CString("jar:"))){
#if COLORER_FEATURE_JARINPUTSOURCE
    return new JARInputSource(path, base);
#else
    throw InputSourceException(CString("InputSource::newInstance: jar not supported"));
#endif
  }
  if (base != nullptr){
    InputSource *is = base->createRelative(path);
    if (is != nullptr) return is;
    throw InputSourceException(CString("Unknown input source type"));
  }
  return new FileInputSource(path, nullptr);
}

bool InputSource::isRelative(const String *path){
  if (path->indexOf(':') != String::npos && path->indexOf(':') < 10) return false;
  if (path->indexOf('/') == 0 || path->indexOf('\\') == 0) return false;
  return true;
}

