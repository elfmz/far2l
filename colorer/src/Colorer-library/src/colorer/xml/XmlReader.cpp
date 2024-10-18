#include "colorer/xml/XmlReader.h"

XmlReader::XmlReader(const XmlInputSource& xml_input_source)
{
  input_source = &xml_input_source;
}

XmlReader::~XmlReader()
{
  delete xml_reader;
}

bool XmlReader::parse()
{
  xml_reader = new LibXmlReader(*input_source);
  return xml_reader->isParsed();
}

void XmlReader::getNodes(std::list<XMLNode>& nodes) const
{
  xml_reader->parse(nodes);
}