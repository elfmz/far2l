#ifndef COLORER_XMLINPUTSOURCE_H
#define COLORER_XMLINPUTSOURCE_H

#include "colorer/Common.h"
#include "colorer/xml/libxml2/LibXmlInputSource.h"

class XmlInputSource;
using uXmlInputSource = std::unique_ptr<XmlInputSource>;

class XmlInputSource
{
 public:
  explicit XmlInputSource(const UnicodeString& source_path);
  ~XmlInputSource() = default;

  XmlInputSource(const UnicodeString& source_path, const UnicodeString* source_base);

  [[nodiscard]]
  uXmlInputSource createRelative(const UnicodeString& relPath) const;

  [[nodiscard]]
  UnicodeString& getPath() const;

  static bool isFileSystemURI(const UnicodeString& path, const UnicodeString* base);

 private:
  std::unique_ptr<LibXmlInputSource> xml_input_source;
};

#endif  // COLORER_XMLINPUTSOURCE_H
