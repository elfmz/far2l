#ifndef COLORER_BASE_ENTITY_RESOLVER_H
#define COLORER_BASE_ENTITY_RESOLVER_H

#include <xercesc/util/XMLEntityResolver.hpp>

class BaseEntityResolver : public xercesc::XMLEntityResolver
{
 public:
  xercesc::InputSource* resolveEntity(xercesc::XMLResourceIdentifier* resourceIdentifier) override;
};

#endif  // COLORER_BASE_ENTITY_RESOLVER_H
