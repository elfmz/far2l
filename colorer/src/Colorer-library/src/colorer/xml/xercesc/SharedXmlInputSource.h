#ifndef _COLORER_SHAREDXMLINPUTSOURCE_H_
#define _COLORER_SHAREDXMLINPUTSOURCE_H_

#include <unordered_map>
#include "colorer/Common.h"
#include "colorer/xml/xercesc/XercesXmlInputSource.h"
#include <xercesc/sax/InputSource.hpp>

class SharedXmlInputSource
{
 public:
  static SharedXmlInputSource* getSharedInputSource(const XMLCh* path, const XMLCh* base);
  [[nodiscard]] xercesc::InputSource* getInputSource() const;

  /** Increments reference counter */
  int addref();
  /** Decrements reference counter */
  int delref();

  [[nodiscard]] XMLSize_t getSize() const;
  [[nodiscard]] XMLByte* getSrc() const;

  SharedXmlInputSource(SharedXmlInputSource const&) = delete;
  SharedXmlInputSource& operator=(SharedXmlInputSource const&) = delete;
  SharedXmlInputSource(SharedXmlInputSource&&) = delete;
  SharedXmlInputSource& operator=(SharedXmlInputSource&&) = delete;

 private:
  explicit SharedXmlInputSource(uXercesXmlInputSource source);
  ~SharedXmlInputSource();

  static std::unordered_map<UnicodeString, SharedXmlInputSource*>* isHash;

  uXercesXmlInputSource input_source;
  int ref_count;
  std::unique_ptr<XMLByte[]> mSrc;
  XMLSize_t mSize;
};

inline XMLSize_t SharedXmlInputSource::getSize() const
{
  return mSize;
}

inline XMLByte* SharedXmlInputSource::getSrc() const
{
  return mSrc.get();
}

#endif  //_COLORER_SHAREDXMLINPUTSOURCE_H_
