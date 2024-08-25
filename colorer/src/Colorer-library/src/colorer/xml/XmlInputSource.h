#ifndef COLORER_XMLINPUTSOURCE_H
#define COLORER_XMLINPUTSOURCE_H

#include "colorer/Common.h"
#ifdef COLORER_FEATURE_LIBXML
#include "colorer/xml/libxml2/LibXmlInputSource.h"
#else
#include "colorer/xml/xercesc/XercesXmlInputSource.h"
#endif


class XmlInputSource;
using uXmlInputSource = std::unique_ptr<XmlInputSource>;

class XmlInputSource
{
 public:
  explicit XmlInputSource(const UnicodeString& source_path);
  ~XmlInputSource();

  XmlInputSource(const UnicodeString& source_path, const UnicodeString* source_base);

  [[nodiscard]]
  uXmlInputSource createRelative(const UnicodeString& relPath) const;

  [[nodiscard]]
  UnicodeString& getPath() const;

#ifndef COLORER_FEATURE_LIBXML
  [[nodiscard]]
  XercesXmlInputSource* getInputSource() const;
#endif

  static bool isFileURI(const UnicodeString& path, const UnicodeString* base);

 private:
#ifdef COLORER_FEATURE_LIBXML
  std::unique_ptr<LibXmlInputSource> xml_input_source;
#else
  uXercesXmlInputSource xml_input_source;
#endif
};

#endif  // COLORER_XMLINPUTSOURCE_H
