#ifndef COLORER_XMLREADER_H
#define COLORER_XMLREADER_H

#include "colorer/xml/XMLNode.h"
#include "colorer/xml/XmlInputSource.h"
#ifdef COLORER_FEATURE_LIBXML
#include "libxml2/LibXmlReader.h"
#else
#include "colorer/xml/xercesc/XercesXmlReader.h"
#endif

class XmlReader
{
 public:
  explicit XmlReader(const XmlInputSource& xml_input_source);
  ~XmlReader();
  bool parse();
  void getNodes(std::list<XMLNode>& nodes) const;

 private:
  const XmlInputSource* input_source;
#ifdef COLORER_FEATURE_LIBXML
  LibXmlReader* xml_reader = nullptr;
#else
  XercesXmlReader* xml_reader = nullptr;
#endif
};

#endif  // COLORER_XMLREADER_H
