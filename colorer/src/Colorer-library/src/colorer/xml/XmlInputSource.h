#ifndef _COLORER_XMLINPUTSOURCE_H_
#define _COLORER_XMLINPUTSOURCE_H_

#include <vector>
#include <xercesc/sax/InputSource.hpp>
#include <colorer/Common.h>
#include <xercesc/util/XMLUniDefs.hpp>

const XMLCh kJar[] = {xercesc::chLatin_j, xercesc::chLatin_a, xercesc::chLatin_r, xercesc::chColon, xercesc::chNull};
const XMLCh kPercent[] = {xercesc::chPercent, xercesc::chNull};

class XmlInputSource;

typedef std::unique_ptr<XmlInputSource> uXmlInputSource;

/**
* @brief Class to creat xercesc::InputSource
*/
class XmlInputSource: public xercesc::InputSource
{
public:
  /**
  * @brief Statically creates instance of uXmlInputSource object,
  * possibly based on parent source stream.
  * @param base Base stream, used to resolve relative paths.
  * @param path Could be relative file location, absolute file
  */
  static uXmlInputSource newInstance(const XMLCh* path, XmlInputSource* base);

  /**
  * @brief Tries statically create instance of InputSource object,
  * according to passed path string.
  * @param path Could be relative file location, absolute file
  */
  static uXmlInputSource newInstance(const XMLCh* path, const XMLCh* base);

  /**
  * @brief Creates inherited InputSource with the same type
  * relatively to the current.
  * @param relPath Relative URI part.
  */
  virtual uXmlInputSource createRelative(const XMLCh* relPath) const = 0;

  virtual xercesc::InputSource* getInputSource()
  {
    return nullptr;
  }

  virtual ~XmlInputSource() {};

  static UString getAbsolutePath(const String* basePath, const String* relPath);
  static XMLCh* ExpandEnvironment(const XMLCh* path);
  static bool isRelative(const String* path);
  static UString getClearPath(const String* basePath, const String* relPath);
  static bool isDirectory(const String* path);
  static void getFileFromDir(const String* relPath, std::vector<SString>& files);
protected:
  XmlInputSource() {};

private:
  XmlInputSource(XmlInputSource const &) = delete;
  XmlInputSource &operator=(XmlInputSource const &) = delete;
  XmlInputSource(XmlInputSource &&) = delete;
  XmlInputSource &operator=(XmlInputSource &&) = delete;
};


#endif //_COLORER_XMLINPUTSOURCE_H_


