#ifndef COLORER_LIBXMLINPUTSOURCE_H
#define COLORER_LIBXMLINPUTSOURCE_H

#include "colorer/Common.h"
#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
#include "colorer/xml/libxml2/SharedXmlInputSource.h"
#endif

struct PathInJar
{
  UnicodeString full_path;
  UnicodeString path_to_jar;
  UnicodeString path_in_jar;
};

class LibXmlInputSource
{
 public:
  explicit LibXmlInputSource(const UnicodeString& path, const UnicodeString* base = nullptr);
  ~LibXmlInputSource();

  [[nodiscard]]
  LibXmlInputSource createRelative(const UnicodeString& relPath) const;

  [[nodiscard]]
  UnicodeString& getPath();

 private:
  UnicodeString sourcePath;

#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
 public:
  static PathInJar getFullPathsToZip(const UnicodeString& path, const UnicodeString* base = nullptr);

 private:
  SharedXmlInputSource* zip_source {nullptr};
  void initZipSource(const UnicodeString& path, const UnicodeString* base = nullptr);
#endif
};

#endif  // COLORER_LIBXMLINPUTSOURCE_H
