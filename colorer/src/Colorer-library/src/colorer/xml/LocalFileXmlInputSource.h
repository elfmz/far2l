#ifndef _COLORER_LOCALFILEINPUTSOURCE_H_
#define _COLORER_LOCALFILEINPUTSOURCE_H_

#include <colorer/xml/XmlInputSource.h>
#include <xercesc/framework/LocalFileInputSource.hpp>

/**
* LocalFileXmlInputSource класс для работы с InputSource - локальными файлами
*/
class LocalFileXmlInputSource : public XmlInputSource
{
public:
  LocalFileXmlInputSource(const XMLCh* path, const XMLCh* base);
  ~LocalFileXmlInputSource();
  xercesc::BinInputStream* makeStream() const override;
  uXmlInputSource createRelative(const XMLCh* relPath) const override;
  xercesc::InputSource* getInputSource() override;
private:
  std::unique_ptr<xercesc::LocalFileInputSource> input_source;

  LocalFileXmlInputSource(LocalFileXmlInputSource const &) = delete;
  LocalFileXmlInputSource &operator=(LocalFileXmlInputSource const &) = delete;
  LocalFileXmlInputSource(LocalFileXmlInputSource &&) = delete;
  LocalFileXmlInputSource &operator=(LocalFileXmlInputSource &&) = delete;
};


#endif //_COLORER_LOCALFILEINPUTSOURCE_H_

