#include "colorer/xml/libxml2/LibXmlInputSource.h"
#include "colorer/Exception.h"
#include "colorer/base/BaseNames.h"
#include "colorer/utils/Environment.h"

LibXmlInputSource::LibXmlInputSource(const UnicodeString& path, const UnicodeString* base)
{
  if (path.isEmpty()) {
    throw InputSourceException("LibXmlInputSource: path is empty");
  }
  UnicodeString full_path;
  if (path.startsWith(jar) || (base != nullptr && base->startsWith(jar))) {
#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
    initZipSource(path, base);
#else
    throw InputSourceException("zip input source not supported");
#endif
  }
  else if (colorer::Environment::isRegularFile(base, &path, full_path)) {
    sourcePath = full_path;
  }
  else {
    throw InputSourceException(full_path + " isn't regular file.");
  }
}

LibXmlInputSource::~LibXmlInputSource()
{
#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
  if (zip_source) {
    zip_source->delref();
  }
#endif
}

LibXmlInputSource LibXmlInputSource::createRelative(const UnicodeString& relPath) const
{
  return LibXmlInputSource(relPath, &sourcePath);
}

UnicodeString& LibXmlInputSource::getPath()
{
  return sourcePath;
}

#ifdef COLORER_FEATURE_ZIPINPUTSOURCE

void LibXmlInputSource::initZipSource(const UnicodeString& path, const UnicodeString* base)
{
  const auto paths = getFullPathsToZip(path, base);

  if (!colorer::Environment::isRegularFile(paths.path_to_jar)) {
    throw InputSourceException(paths.path_to_jar + " isn't regular file.");
  }

  sourcePath = paths.full_path;
  zip_source = SharedXmlInputSource::getSharedInputSource(paths.path_to_jar);
}

PathInJar LibXmlInputSource::getFullPathsToZip(const UnicodeString& path, const UnicodeString* base)
{
  if (path.startsWith(jar)) {
    auto local_path = colorer::Environment::expandSpecialEnvironment(path);
    const auto path_idx = local_path.lastIndexOf('!');
    if (path_idx == -1) {
      throw InputSourceException("Bad jar uri format: " + local_path);
    }

    UnicodeString path_to_jar;
    if (local_path.compare(path) == 0) {
      path_to_jar = colorer::Environment::getAbsolutePath(
          base ? *base : u"", UnicodeString(local_path, jar.length(), path_idx - jar.length()));
    }
    else {
      path_to_jar =
          colorer::Environment::getAbsolutePath(u"", UnicodeString(local_path, jar.length(), path_idx - jar.length()));
    }

    const UnicodeString path_in_jar(local_path, path_idx + 1);

    const UnicodeString full_path = jar + path_to_jar + u"!" + path_in_jar;
    return {full_path, path_to_jar, path_in_jar};
  }
  if (base != nullptr && base->startsWith(jar)) {
    const auto base_idx = base->lastIndexOf('!');
    if (base_idx == -1) {
      throw InputSourceException("Bad jar uri format: " + path);
    }
    const UnicodeString path_to_jar(*base, jar.length(), base_idx - jar.length());
    const UnicodeString path_in_jar = colorer::Environment::getAbsolutePath(UnicodeString(*base, base_idx + 1), path);

    const UnicodeString full_path = jar + path_to_jar + u"!" + path_in_jar;
    return {full_path, path_to_jar, path_in_jar};
  }
  throw InputSourceException("The path to the jar was not found");
}

#endif
