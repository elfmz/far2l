#ifndef _COLORER_ZIPINPUTSOURCE_H_
#define _COLORER_ZIPINPUTSOURCE_H_

#include "colorer/xml/xercesc/SharedXmlInputSource.h"
#include "colorer/xml/xercesc/XercesXmlInputSource.h"
#include <xercesc/util/BinFileInputStream.hpp>

class ZipXmlInputSource : public XercesXmlInputSource
{
 public:
  ZipXmlInputSource(const XMLCh* path, const XMLCh* base);
  ~ZipXmlInputSource() override;
  [[nodiscard]] xercesc::BinInputStream* makeStream() const override;
  xercesc::InputSource* getInputSource() const override;

  static uUnicodeString getAbsolutePath(const UnicodeString* basePath, const UnicodeString* relPath);

  ZipXmlInputSource(ZipXmlInputSource const&) = delete;
  ZipXmlInputSource& operator=(ZipXmlInputSource const&) = delete;
  ZipXmlInputSource(ZipXmlInputSource&&) = delete;
  ZipXmlInputSource& operator=(ZipXmlInputSource&&) = delete;

 private:
  void create(const XMLCh* path, const XMLCh* base);
  uUnicodeString in_jar_location;
  SharedXmlInputSource* jar_input_source = nullptr;
};

class UnZip : public xercesc::BinInputStream
{
 public:
  UnZip(const XMLByte* src, XMLSize_t size, const UnicodeString* path);
  ~UnZip() override;

  [[nodiscard]] XMLFilePos curPos() const override;
  XMLSize_t readBytes(XMLByte* toFill, XMLSize_t maxToRead) override;
  [[nodiscard]] const XMLCh* getContentType() const override;

  UnZip(UnZip const&) = delete;
  UnZip& operator=(UnZip const&) = delete;
  UnZip(UnZip&&) = delete;
  UnZip& operator=(UnZip&&) = delete;

 private:
  XMLSize_t mPos;
  XMLSize_t mBoundary;
  std::unique_ptr<byte[]> stream;
  int len;
};

#endif  //_COLORER_ZIPINPUTSOURCE_H_
