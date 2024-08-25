#ifndef COLORER_LIBXMLINPUTSOURCE_H
#define COLORER_LIBXMLINPUTSOURCE_H

#include "colorer/Common.h"
#include "colorer/Exception.h"
#include "colorer/utils/Environment.h"

class LibXmlInputSource
{
 public:
  explicit LibXmlInputSource(const UnicodeString* path, const UnicodeString* base = nullptr)
  {
    UnicodeString full_path;
    if (colorer::Environment::isRegularFile(base, path, full_path)) {
      sourcePath = full_path;
      // file is not open yet, only after makeStream
    }
    else {
      throw InputSourceException(full_path + " isn't regular file.");
    }
  }

  LibXmlInputSource createRelative(const UnicodeString& relPath) const
  {
    return LibXmlInputSource(&relPath, &sourcePath);
  }

  [[nodiscard]]
  UnicodeString& getPath()
  {
    return sourcePath;
  }

 private:
  UnicodeString sourcePath;
};

#endif  // COLORER_LIBXMLINPUTSOURCE_H
