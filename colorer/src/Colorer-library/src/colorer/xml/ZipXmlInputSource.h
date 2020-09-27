#ifndef _COLORER_ZIPINPUTSOURCE_H_
#define _COLORER_ZIPINPUTSOURCE_H_

#include <colorer/unicode/String.h>
#include <colorer/xml/XmlInputSource.h>
#include <xercesc/util/BinFileInputStream.hpp>
#include <colorer/xml/SharedXmlInputSource.h>

class ZipXmlInputSource : public XmlInputSource
{
public:
  ZipXmlInputSource(const XMLCh* path, const XMLCh* base);
  ZipXmlInputSource(const XMLCh* path, XmlInputSource* base);
  ~ZipXmlInputSource();
  xercesc::BinInputStream* makeStream() const override;
  uXmlInputSource createRelative(const XMLCh* relPath) const override;
  xercesc::InputSource* getInputSource() override;
private:
  void create(const XMLCh* path, const XMLCh* base);
  UString in_jar_location;
  SharedXmlInputSource* jar_input_source;

  ZipXmlInputSource(ZipXmlInputSource const &) = delete;
  ZipXmlInputSource &operator=(ZipXmlInputSource const &) = delete;
  ZipXmlInputSource(ZipXmlInputSource &&) = delete;
  ZipXmlInputSource &operator=(ZipXmlInputSource &&) = delete;
};


class UnZip : public xercesc::BinInputStream
{
public:
  UnZip(const XMLByte* src, XMLSize_t size, const String* path);
  ~UnZip();

  XMLFilePos curPos() const override;
  XMLSize_t readBytes(XMLByte* const toFill, const XMLSize_t maxToRead) override;
  const XMLCh* getContentType() const override;

private:

  XMLSize_t mPos;
  XMLSize_t mBoundary;
  std::unique_ptr<byte[]> stream;
  int len;

  UnZip(UnZip const &) = delete;
  UnZip &operator=(UnZip const &) = delete;
  UnZip(UnZip &&) = delete;
  UnZip &operator=(UnZip &&) = delete;
};

#endif //_COLORER_ZIPINPUTSOURCE_H_

