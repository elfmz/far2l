#ifndef _COLORER_BASE_ENTITY_RESOLVER_H_
#define _COLORER_BASE_ENTITY_RESOLVER_H_

#include <xercesc/util/XMLEntityResolver.hpp>
#include <colorer/xml/XmlInputSource.h>

class BaseEntityResolver : public xercesc::XMLEntityResolver
{
public:
    BaseEntityResolver(){};
    ~BaseEntityResolver() {};
    xercesc::InputSource* resolveEntity(xercesc::XMLResourceIdentifier* resourceIdentifier);
};

#endif


