#include "colorer/xml/libxml2/LibXmlReader.h"
#include <libxml/parserInternals.h>
#include <cstring>

LibXmlReader::LibXmlReader(const UnicodeString& source_file) : xmldoc(nullptr)
{
  xmlSetExternalEntityLoader(xmlMyExternalEntityLoader);
  xmlSetGenericErrorFunc(nullptr, xml_error_func);
  xmldoc = xmlReadFile(UStr::to_stdstr(&source_file).c_str(), nullptr, XML_PARSE_NOENT | XML_PARSE_NONET);
}

LibXmlReader::LibXmlReader(const XmlInputSource& source) : LibXmlReader(source.getPath()) {}

LibXmlReader::~LibXmlReader()
{
  if (xmldoc != nullptr) {
    xmlFreeDoc(xmldoc);
  }
}

void LibXmlReader::parse(std::list<XMLNode>& nodes)
{
  xmlNode* current = xmlDocGetRootElement(xmldoc);
  while (current != nullptr) {
    XMLNode result;
    populateNode(current, result);
    nodes.push_back(result);
    current = current->next;
  }
}

bool LibXmlReader::populateNode(xmlNode* node, XMLNode& result)
{
  if (node->type == XML_ELEMENT_NODE) {
    result.name = UnicodeString((const char*) node->name);

    const auto text_string = getElementText(node);
    if (!text_string.isEmpty()) {
      result.text = text_string;
    }
    getChildren(node, result);
    getAttributes(node, result.attributes);

    return true;
  }
  return false;
}

UnicodeString LibXmlReader::getElementText(xmlNode* node)
{
  for (const xmlNode* child = node->children; child != nullptr; child = child->next) {
    if (child->type == XML_CDATA_SECTION_NODE) {
      return UnicodeString((const char*) child->content);
    }
    if (child->type == XML_TEXT_NODE) {
      auto temp_string = UnicodeString((const char*) child->content);
      temp_string.trim();
      if (temp_string.isEmpty()) {
        continue;
      }
      return temp_string;
    }
  }
  return UnicodeString("");
}

void LibXmlReader::getChildren(xmlNode* node, XMLNode& result)
{
  if (node->children == nullptr) {
    return;
  }
  node = node->children;

  while (node != nullptr) {
    if (!xmlIsBlankNode(node)) {
      XMLNode child;
      if (populateNode(node, child)) {
        result.children.push_back(child);
      }
    }
    node = node->next;
  }
}

void LibXmlReader::getAttributes(const xmlNode* node, std::unordered_map<UnicodeString, UnicodeString>& data)
{
  for (xmlAttrPtr attr = node->properties; attr != nullptr; attr = attr->next) {
    data.emplace(std::pair((const char*) attr->name, (const char*) xmlNodeGetContent(attr->children)));
  }
}

xmlParserInputPtr LibXmlReader::xmlMyExternalEntityLoader(const char* URL, const char* /*ID*/, xmlParserCtxtPtr ctxt)
{
  xmlParserInputPtr ret = nullptr;
  // тут обработка имени файла для внешнего entity
  // при этом если в entity указан нормальный путь файловой системы, без всяких переменных окружения, архивов,
  // но можно с комбинацией ./ ../
  //  то в url будет указан полный путь, относительно текущего файла. libxml сама склеит путь.
  //  Иначе в url будет указан путь из самого entity, и дальше с ним над самому разбираться.
  //
  // в ctxt нет информации об обрабатываемом файле.
  ret = xmlNewInputFromFile(ctxt, URL);
  /*if (ret != nullptr) {
    return ret;
  }
  if (defaultLoader != nullptr) {
    ret = defaultLoader(URL, ID, ctxt);
  }*/
  return ret;
}

void LibXmlReader::xml_error_func(void* /*ctx*/, const char* msg, ...)
{
  static char buf[PATH_MAX];
  static int slen = 0;
  va_list args;

  /* libxml2 prints IO errors from bad includes paths by
   * calling the error function once per word. So we get to
   * re-assemble the message here and print it when we get
   * the line break. My enthusiasm about this is indescribable.
   */
  va_start(args, msg);
  int rc = vsnprintf(&buf[slen], sizeof(buf) - slen, msg, args);
  va_end(args);

  /* This shouldn't really happen */
  if (rc < 0) {
    COLORER_LOG_ERROR("+++ out of cheese error. redo from start +++\n");
    slen = 0;
    memset(buf, 0, sizeof(buf));
    return;
  }

  slen += rc;
  if (slen >= (int) sizeof(buf)) {
    /* truncated, let's flush this */
    buf[sizeof(buf) - 1] = '\n';
    slen = sizeof(buf);
  }

  /* We're assuming here that the last character is \n. */
  if (buf[slen - 1] == '\n') {
    buf[slen - 1] = '\0';
    COLORER_LOG_ERROR("%", buf);
    memset(buf, 0, sizeof(buf));
    slen = 0;
  }
}