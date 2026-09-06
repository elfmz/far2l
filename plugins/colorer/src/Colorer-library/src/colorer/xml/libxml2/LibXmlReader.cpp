#include "colorer/xml/libxml2/LibXmlReader.h"
#include <libxml/parserInternals.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <utility>
#include "colorer/Exception.h"
#include "colorer/base/BaseNames.h"
#include "colorer/utils/Environment.h"

#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
#include "colorer/zip/MemoryFile.h"
#endif

#ifdef _MSC_VER
#define strdup(p) _strdup(p)
#endif

namespace {

struct XmlLoadContext
{
  UnicodeString current_file;
  bool is_first_call = true;
};

XmlLoadContext* current_load = nullptr;

class XmlLoadCurrent
{
 public:
  explicit XmlLoadCurrent(XmlLoadContext* load) : previous(current_load)
  {
    current_load = load;
  }
  ~XmlLoadCurrent()
  {
    current_load = previous;
  }
  XmlLoadCurrent(const XmlLoadCurrent&) = delete;
  XmlLoadCurrent& operator=(const XmlLoadCurrent&) = delete;

 private:
  XmlLoadContext* previous;
};

XmlLoadContext* loadContext(xmlParserCtxtPtr ctxt)
{
  if (current_load != nullptr) {
    return current_load;
  }
  if (ctxt != nullptr && ctxt->_private != nullptr) {
    return static_cast<XmlLoadContext*>(ctxt->_private);
  }
  return nullptr;
}

char xml_err_buf[4096];
int xml_err_slen = 0;

}  // namespace

LibXmlReader::LibXmlReader(const UnicodeString& source_file)
{
  xmlSetExternalEntityLoader(xmlMyExternalEntityLoader);
  xmlSetGenericErrorFunc(nullptr, xml_error_func);

  xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
  if (ctxt == nullptr) {
    return;
  }

  XmlLoadContext load;
  load.current_file = source_file;
  ctxt->_private = &load;
  const XmlLoadCurrent current(&load);

  xmlDocPtr xmldoc =
      xmlCtxtReadFile(ctxt, UStr::to_stdstr(&source_file).c_str(), nullptr, XML_PARSE_NOENT | XML_PARSE_NONET);
  parsed = xmldoc != nullptr;
  if (xmldoc != nullptr) {
    xmlNode* current_node = xmlDocGetRootElement(xmldoc);
    size_t count = 0;
    for (xmlNode* node = current_node; node != nullptr; node = node->next) {
      ++count;
    }
    nodes.reserve(count);
    while (current_node != nullptr) {
      XMLNode result;
      populateNode(current_node, result);
      nodes.push_back(std::move(result));
      current_node = current_node->next;
    }
    xmlFreeDoc(xmldoc);
    ctxt->myDoc = nullptr;
  }
  ctxt->_private = nullptr;
  xmlFreeParserCtxt(ctxt);
}

LibXmlReader::LibXmlReader(const XmlInputSource& source) : LibXmlReader(source.getPath()) {}

LibXmlReader::~LibXmlReader() = default;

void LibXmlReader::parse(XMLNodeList& out_nodes)
{
  out_nodes = std::move(nodes);
}

bool LibXmlReader::populateNode(xmlNode* node, XMLNode& result)
{
  if (node->type == XML_ELEMENT_NODE) {
    result.name = UnicodeString(reinterpret_cast<const char*>(node->name));

    const auto text_string = getElementText(node);
    if (text_string && !text_string->isEmpty()) {
      result.text = UnicodeString(*text_string.get());
    }
    getChildren(node, result);
    getAttributes(node, result.attributes);

    return true;
  }
  return false;
}

uUnicodeString LibXmlReader::getElementText(const xmlNode* node)
{
  for (const xmlNode* child = node->children; child != nullptr; child = child->next) {
    if (child->type == XML_CDATA_SECTION_NODE) {
      return Encodings::fromUTF8(child->content);
    }
    if (child->type == XML_TEXT_NODE) {
      auto temp_string = Encodings::fromUTF8(child->content);
      temp_string->trim();
      if (temp_string->isEmpty()) {
        continue;
      }
      return temp_string;
    }
  }
  return nullptr;
}

void LibXmlReader::getChildren(xmlNode* node, XMLNode& result)
{
  if (node->children == nullptr) {
    return;
  }

  size_t count = 0;
  for (const xmlNode* child = node->children; child != nullptr; child = child->next) {
    if (child->type == XML_ELEMENT_NODE) {
      ++count;
    }
  }
  result.children.reserve(count);

  for (xmlNode* child = node->children; child != nullptr; child = child->next) {
    if (child->type != XML_ELEMENT_NODE) {
      continue;
    }
    XMLNode xml_child;
    if (populateNode(child, xml_child)) {
      result.children.push_back(std::move(xml_child));
    }
  }
}

void LibXmlReader::getAttributes(const xmlNode* node, std::unordered_map<UnicodeString, UnicodeString>& data)
{
  for (xmlAttrPtr attr = node->properties; attr != nullptr; attr = attr->next) {
    const auto content = xmlNodeGetContent(attr->children);
    auto decoded_string = Encodings::fromUTF8(content);
    data.try_emplace(reinterpret_cast<const char*>(attr->name), *decoded_string.get());
    xmlFree(content);
  }
}

#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
xmlParserInputPtr LibXmlReader::xmlZipEntityLoader(const PathInJar& paths, xmlParserCtxtPtr ctxt)
{
  const auto is = SharedXmlInputSource::getSharedInputSource(paths.path_to_jar);
  std::unique_ptr<std::vector<byte>> unzipped_stream;
  try {
    is->open();
    unzipped_stream = unzip(is->getSrc(), is->getSize(), paths.path_in_jar);
  } catch (...) {
    is->delref();
    throw;
  }
  is->delref();

  xmlParserInputBufferPtr buf =
      xmlParserInputBufferCreateMem(reinterpret_cast<const char*>(unzipped_stream->data()),
                                    static_cast<int>(unzipped_stream->size()), XML_CHAR_ENCODING_NONE);
  xmlParserInputPtr pInput = xmlNewIOInputStream(ctxt, buf, XML_CHAR_ENCODING_NONE);

  // filling in the filename for the external entity to work
  const auto root_pos = paths.path_in_jar.lastIndexOf('/') + 1;
  const auto file_name = UnicodeString(paths.path_in_jar, root_pos);
  pInput->filename = strdup(UStr::to_stdstr(&file_name).c_str());
  return pInput;
}
#endif

xmlParserInputPtr LibXmlReader::xmlMyExternalEntityLoader(const char* URL, const char* /*ID*/, xmlParserCtxtPtr ctxt)
{
  /*
   * The function is called before each opening of a file within libxml, whether it is an xmlReadFile,
   * or opening a file for an external entity.
   * I.e., the path to the file can be checked or modified here. If the external entity specifies a path similar
   * to the file path, then libxml itself forms the full path to the entity file by gluing the path from the current
   * file and the one specified in the external entity. But sometimes it fails, for example, on non-Latin letters in
   * the path to the main file.
   * At the same time, there is no path to the source file or to the file in the entity in the function parameters.
   */

  XmlLoadContext* load = loadContext(ctxt);
  if (load == nullptr) {
    return xmlNewInputFromFile(ctxt, URL);
  }

  auto filename = Encodings::fromUTF8(const_cast<char*>(URL), static_cast<int32_t>(strlen(URL)));
  UnicodeString string_url(*filename.get());

  // read entity string like "env:$FAR_HOME/hrd/catalog-console.xml"
  static const UnicodeString env(u"env:");
  if (!load->is_first_call && string_url.startsWith(env)) {
    const auto exp = colorer::Environment::expandSpecialEnvironment(string_url);
    string_url = UnicodeString(exp, env.length());
  }

#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
  if (string_url.startsWith(jar) || load->current_file.startsWith(jar)) {
    const auto paths =
        LibXmlInputSource::getFullPathsToZip(string_url, load->is_first_call ? nullptr : &load->current_file);
    load->is_first_call = false;
    xmlParserInputPtr ret = nullptr;
    try {
      ret = xmlZipEntityLoader(paths, ctxt);
    } catch (const Exception& e) {
      // Must not throw through libxml2's C entity-loader callback.
      COLORER_LOG_ERROR("zip entity load failed: %", e.what());
    } catch (...) {
      COLORER_LOG_ERROR("zip entity load failed");
    }
    return ret;
  }
#endif
  // We check if the file exists after all the conversions. If not, then we check the file relative to the one being processed.
  // This is relevant for the case of non-Latin letters in the path to the main file and entity on linux. There is no such problem on windows.
  if (!load->is_first_call && !colorer::Environment::isRegularFile(string_url)) {
    auto new_string_url = colorer::Environment::getAbsolutePath(load->current_file, string_url);
    if (colorer::Environment::isRegularFile(new_string_url)) {
      string_url = std::move(new_string_url);
    }
  }

  load->is_first_call = false;
  // read it as a regular file
  xmlParserInputPtr ret = xmlNewInputFromFile(ctxt, UStr::to_stdstr(&string_url).c_str());

  return ret;
}

void LibXmlReader::xml_error_func(void* /*ctx*/, const char* msg, ...)
{
  va_list args;

  /* libxml2 prints IO errors from bad includes paths by
   * calling the error function once per word. So we get to
   * re-assemble the message here and print it when we get
   * the line break. My enthusiasm about this is indescribable.
   */
  va_start(args, msg);
  const int rc = vsnprintf(&xml_err_buf[xml_err_slen], sizeof(xml_err_buf) - xml_err_slen, msg, args);
  va_end(args);

  /* This shouldn't really happen */
  if (rc < 0) {
    COLORER_LOG_ERROR("+++ out of cheese error. redo from start +++\n");
    xml_err_slen = 0;
    memset(xml_err_buf, 0, sizeof(xml_err_buf));
    return;
  }

  xml_err_slen += rc;
  if (xml_err_slen >= static_cast<int>(sizeof(xml_err_buf))) {
    /* truncated, let's flush this */
    xml_err_buf[sizeof(xml_err_buf) - 1] = '\n';
    xml_err_slen = sizeof(xml_err_buf);
  }

  /* We're assuming here that the last character is \n. */
  if (xml_err_buf[xml_err_slen - 1] == '\n') {
    xml_err_buf[xml_err_slen - 1] = '\0';
    COLORER_LOG_ERROR("%", xml_err_buf);
    memset(xml_err_buf, 0, sizeof(xml_err_buf));
    xml_err_slen = 0;
  }
}
