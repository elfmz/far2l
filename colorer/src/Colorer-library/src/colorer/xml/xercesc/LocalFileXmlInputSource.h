#ifndef _COLORER_LOCALFILEINPUTSOURCE_H_
#define _COLORER_LOCALFILEINPUTSOURCE_H_

#include "colorer/xml/xercesc/XercesXmlInputSource.h"
#include <xercesc/framework/LocalFileInputSource.hpp>

/**
 * LocalFileXmlInputSource класс для работы с InputSource - локальными файлами
 */
class LocalFileXmlInputSource : public XercesXmlInputSource
{
 public:
  LocalFileXmlInputSource(const XMLCh* path, const XMLCh* base);
  ~LocalFileXmlInputSource() override = default;
  [[nodiscard]] xercesc::BinInputStream* makeStream() const override;
  xercesc::InputSource* getInputSource() const override;

  LocalFileXmlInputSource(LocalFileXmlInputSource const&) = delete;
  LocalFileXmlInputSource& operator=(LocalFileXmlInputSource const&) = delete;
  LocalFileXmlInputSource(LocalFileXmlInputSource&&) = delete;
  LocalFileXmlInputSource& operator=(LocalFileXmlInputSource&&) = delete;

};

#endif  //_COLORER_LOCALFILEINPUTSOURCE_H_
