#include "colorer/xml/xercesc/BaseEntityResolver.h"
#include <xercesc/framework/MemBufInputSource.hpp>
#include "colorer/Exception.h"
#include "colorer/xml/xercesc/XercesXmlInputSource.h"

xercesc::InputSource* BaseEntityResolver::resolveEntity(xercesc::XMLResourceIdentifier* resourceIdentifier)
{
  try {
    auto input_source =
        XercesXmlInputSource::newInstance(resourceIdentifier->getSystemId(), resourceIdentifier->getBaseURI());
    return input_source.release();
  } catch (InputSourceException& e) {
    COLORER_LOG_WARN(e.what());
    // Если не можем открыть external entity, то отдаем пустой файл.
    // Тем самым гасим ошибку загрузки схемы. Работа продолжится, но раскраска будет не до конца верной.
    auto empty_buf = new xercesc::MemBufInputSource((const XMLByte*) "", 0, "dummy");
    return empty_buf;
  }
}
