#include "colorer/xml/BaseEntityResolver.h"
#include <xercesc/util/XMLString.hpp>
#include "colorer/xml/XmlInputSource.h"

xercesc::InputSource* BaseEntityResolver::resolveEntity(
    xercesc::XMLResourceIdentifier* resourceIdentifier)
{
    auto input_source = XmlInputSource::newInstance(resourceIdentifier->getSystemId(),
                                                    resourceIdentifier->getBaseURI());
    return input_source.release();
}
