#ifndef COLORER_XERCESXMLINPUTSOURCE_H
#define COLORER_XERCESXMLINPUTSOURCE_H

#include <xercesc/sax/InputSource.hpp>
#include "colorer/Common.h"

inline const auto kJar = (const XMLCh *) u"jar:\0";

class XercesXmlInputSource;

typedef std::unique_ptr<XercesXmlInputSource> uXercesXmlInputSource;

/**
 * @brief Class to creat xercesc::InputSource
 */
class XercesXmlInputSource : public xercesc::InputSource
{
 public:
  /**
   * @brief Tries statically create instance of InputSource object,
   * according to passed path string.
   * @param path Could be relative file location, absolute file
   */
  static uXercesXmlInputSource newInstance(const XMLCh* path, const XMLCh* base);
  static uXercesXmlInputSource newInstance(const UnicodeString* path,
                                     const UnicodeString* base = nullptr);

  /**
   * @brief Creates inherited InputSource with the same type
   * relatively to the current.
   * @param relPath Relative URI part.
   */
  virtual uXercesXmlInputSource createRelative(const UnicodeString& relPath);

  [[nodiscard]] virtual xercesc::InputSource* getInputSource() const = 0;

  ~XercesXmlInputSource() override = default;

  static bool isUriFile(const UnicodeString& path, const UnicodeString* base = nullptr);

  [[nodiscard]] UnicodeString& getPath() const;

  XercesXmlInputSource(XercesXmlInputSource const&) = delete;
  XercesXmlInputSource& operator=(XercesXmlInputSource const&) = delete;
  XercesXmlInputSource(XercesXmlInputSource&&) = delete;
  XercesXmlInputSource& operator=(XercesXmlInputSource&&) = delete;

 protected:
  XercesXmlInputSource() = default;
  uUnicodeString source_path;
};

#endif  // COLORER_XERCESXMLINPUTSOURCE_H
