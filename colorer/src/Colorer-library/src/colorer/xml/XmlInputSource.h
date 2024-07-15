#ifndef COLORER_XMLINPUTSOURCE_H
#define COLORER_XMLINPUTSOURCE_H

#include <xercesc/sax/InputSource.hpp>
#include "colorer/Common.h"

XMLCH_LITERAL(kJar, u"jar:\0");
XMLCH_LITERAL(kPercent, u"%\0");

class XmlInputSource;

typedef std::unique_ptr<XmlInputSource> uXmlInputSource;

/**
 * @brief Class to creat xercesc::InputSource
 */
class XmlInputSource : public xercesc::InputSource
{
 public:
  /**
   * @brief Tries statically create instance of InputSource object,
   * according to passed path string.
   * @param path Could be relative file location, absolute file
   */
  static uXmlInputSource newInstance(const XMLCh* path, const XMLCh* base);
  static uXmlInputSource newInstance(const UnicodeString* path,
                                     const UnicodeString* base = nullptr);

  /**
   * @brief Creates inherited InputSource with the same type
   * relatively to the current.
   * @param relPath Relative URI part.
   */
  virtual uXmlInputSource createRelative(const XMLCh* relPath);

  [[nodiscard]] virtual xercesc::InputSource* getInputSource() const = 0;

  ~XmlInputSource() override = default;

  static bool isUriFile(const UnicodeString& path, const UnicodeString* base = nullptr);

  [[nodiscard]] UnicodeString& getPath() const;

  XmlInputSource(XmlInputSource const&) = delete;
  XmlInputSource& operator=(XmlInputSource const&) = delete;
  XmlInputSource(XmlInputSource&&) = delete;
  XmlInputSource& operator=(XmlInputSource&&) = delete;

 protected:
  XmlInputSource() = default;
  uUnicodeString source_path;
};

#endif  // COLORER_XMLINPUTSOURCE_H
