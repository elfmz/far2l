#include "colorer/parsers/HrcLibraryImpl.h"
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/NumberFormatException.hpp>
#include <xercesc/util/XMLDouble.hpp>
#include "colorer/base/XmlTagDefs.h"
#include "colorer/parsers/FileTypeImpl.h"
#include "colorer/xml/BaseEntityResolver.h"
#include "colorer/xml/XmlParserErrorHandler.h"

HrcLibrary::Impl::Impl()
{
  fileTypeHash.reserve(200);
  fileTypeVector.reserve(150);
  regionNamesHash.reserve(1000);
  regionNamesVector.reserve(1000);
  schemeHash.reserve(4000);
}

HrcLibrary::Impl::~Impl()
{
  for (const auto& it : fileTypeHash) {
    delete it.second;
  }

  for (const auto& it : schemeHash) {
    delete it.second;
  }

  for (auto it : regionNamesVector) {
    delete it;
  }

  for (const auto& it : schemeEntitiesHash) {
    delete it.second;
  }
}

void HrcLibrary::Impl::loadSource(XmlInputSource* is)
{
  if (!is) {
    throw HrcLibraryException("Can't open stream - 'null' is bad stream.");
  }

  XmlInputSource* istemp = current_input_source;
  current_input_source = is;
  try {
    parseHRC(*is);
  } catch (Exception&) {
    current_input_source = istemp;
    throw;
  }
  current_input_source = istemp;
}

void HrcLibrary::Impl::unloadFileType(FileType* filetype)
{
  bool loop = true;
  while (loop) {
    loop = false;
    for (auto scheme = schemeHash.begin(); scheme != schemeHash.end(); ++scheme) {
      if (scheme->second->fileType == filetype) {
        schemeHash.erase(scheme);
        loop = true;
        break;
      }
    }
  }
  for (auto ft = fileTypeVector.begin(); ft != fileTypeVector.end(); ++ft) {
    if (*ft == filetype) {
      fileTypeVector.erase(ft);
      break;
    }
  }
  fileTypeHash.erase(filetype->getName());
  delete filetype;
}

void HrcLibrary::Impl::loadFileType(FileType* filetype)
{
  auto* thisType = filetype;
  if (thisType == nullptr || filetype->pimpl->type_loading ||
      thisType->pimpl->input_source_loading || thisType->pimpl->load_broken)
  {
    return;
  }

  thisType->pimpl->input_source_loading = true;

  auto& input_source = thisType->pimpl->inputSource;
  try {
    loadSource(input_source.get());
  } catch (InputSourceException& e) {
    logger->error("Can't open source stream: {0}", e.what());
    thisType->pimpl->load_broken = true;
  } catch (HrcLibraryException& e) {
    logger->error("{0} [{1}]", e.what(),
                  thisType->pimpl->inputSource ? input_source->getPath() : "");
    thisType->pimpl->load_broken = true;
  } catch (Exception& e) {
    logger->error("{0} [{1}]", e.what(),
                  thisType->pimpl->inputSource ? input_source->getPath() : "");
    thisType->pimpl->load_broken = true;
  } catch (...) {
    logger->error("Unknown exception while loading {0}", input_source->getPath());
    thisType->pimpl->load_broken = true;
  }

  thisType->pimpl->input_source_loading = false;
}

FileType* HrcLibrary::Impl::chooseFileType(const UnicodeString* fileName,
                                           const UnicodeString* firstLine, int typeNo)
{
  FileType* best = nullptr;
  double max_prior = 0;
  const double DELTA = 1e-6;
  for (auto ret : fileTypeVector) {
    double prior = ret->pimpl->getPriority(fileName, firstLine);

    if (typeNo > 0 && (prior - max_prior < DELTA)) {
      best = ret;
      typeNo--;
    }
    if (prior - max_prior > DELTA || best == nullptr) {
      best = ret;
      max_prior = prior;
    }
  }
  if (typeNo > 0) {
    return nullptr;
  }
  return best;
}

FileType* HrcLibrary::Impl::getFileType(const UnicodeString* name)
{
  if (name == nullptr) {
    return nullptr;
  }
  auto filetype = fileTypeHash.find(*name);
  if (filetype != fileTypeHash.end())
    return filetype->second;
  else
    return nullptr;
}

FileType* HrcLibrary::Impl::enumerateFileTypes(unsigned int index)
{
  if (index < fileTypeVector.size()) {
    return fileTypeVector[index];
  }
  return nullptr;
}

size_t HrcLibrary::Impl::getFileTypesCount()
{
  return fileTypeVector.size();
}

size_t HrcLibrary::Impl::getRegionCount()
{
  return regionNamesVector.size();
}

const Region* HrcLibrary::Impl::getRegion(unsigned int id)
{
  if (id >= regionNamesVector.size()) {
    return nullptr;
  }
  return regionNamesVector[id];
}

const Region* HrcLibrary::Impl::getRegion(const UnicodeString* name)
{
  if (name == nullptr) {
    return nullptr;
  }
  return getNCRegion(name, false);  // regionNamesHash.get(name);
}

// protected methods

void HrcLibrary::Impl::parseHRC(const XmlInputSource& is)
{
  logger->debug("begin parse '{0}'", is.getPath());
  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  BaseEntityResolver resolver;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setXMLEntityResolver(&resolver);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  xml_parser.setDisableDefaultEntityResolution(true);
  xml_parser.parse(*is.getInputSource());
  if (error_handler.getSawErrors()) {
    throw HrcLibraryException("Error reading hrc file '" + is.getPath() + "'");
  }
  xercesc::DOMDocument* doc = xml_parser.getDocument();
  xercesc::DOMElement* root = doc->getDocumentElement();

  if (!root || !xercesc::XMLString::equals(root->getNodeName(), hrcTagHrc)) {
    throw HrcLibraryException(
        "Incorrect hrc-file structure. Main '<hrc>' block not found. Current file " + is.getPath());
  }

  bool globalUpdateStarted = false;
  if (!updateStarted) {
    globalUpdateStarted = true;
    updateStarted = true;
  }

  parseHrcBlock(root);

  structureChanged = true;
  if (globalUpdateStarted) {
    updateLinks();
    updateStarted = false;
  }

  logger->debug("end parse '{0}'", is.getPath());
}

void HrcLibrary::Impl::parseHrcBlock(const xercesc::DOMElement* elem)
{
  for (auto node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      parseHrcBlockElements(node);
    }
    else if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      for (auto sub_node = node->getFirstChild(); sub_node != nullptr;
           sub_node = sub_node->getNextSibling())
      {
        parseHrcBlockElements(sub_node);
      }
    }
  }
}

void HrcLibrary::Impl::parseHrcBlockElements(const xercesc::DOMNode* elem)
{
  if (elem->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
    // don`t use dynamic_cast, see https://github.com/colorer/Colorer-library/issues/32
    auto sub_elem = static_cast<const xercesc::DOMElement*>(elem);
    if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagPrototype) ||
        xercesc::XMLString::equals(elem->getNodeName(), hrcTagPackage))
    {
      addPrototype(sub_elem);
    }
    else if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagType)) {
      addType(sub_elem);
    }
    else if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagAnnotation)) {
      // not read annotation
    }
    else {
      logger->warn("Unused element '{0}'. Current file {1}.", UStr::to_stdstr(elem->getNodeName()),
                   current_input_source->getPath());
    }
  }
}

void HrcLibrary::Impl::addPrototype(const xercesc::DOMElement* elem)
{
  const auto typeName = elem->getAttribute(hrcPrototypeAttrName);
  const auto typeGroup = elem->getAttribute(hrcPrototypeAttrGroup);
  const auto typeDescription = elem->getAttribute(hrcPrototypeAttrDescription);
  if (UStr::isEmpty(typeName)) {
    logger->error("Found unnamed prototype/package. Skipped.");
    return;
  }

  UnicodeString tname = UnicodeString(typeName);
  auto ft = fileTypeHash.find(tname);
  if (ft != fileTypeHash.end()) {
    unloadFileType(ft->second);
    logger->warn("Duplicate prototype '{0}'. First version unloaded, current is loading.", tname);
  }

  auto* type = new FileType(tname, UStr::isEmpty(typeGroup) ? tname : typeGroup,
                            UStr::isEmpty(typeDescription) ? tname : typeDescription);
  auto& ptype = type->pimpl;

  if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagPackage)) {
    ptype->isPackage = true;
  }

  parsePrototypeBlock(elem, type);

  fileTypeHash.emplace(tname, type);
  if (!ptype->isPackage) {
    fileTypeVector.push_back(type);
  }
}

void HrcLibrary::Impl::parsePrototypeBlock(const xercesc::DOMElement* elem,
                                           FileType* current_parse_prototype)
{
  for (auto node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* sub_elem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(sub_elem->getNodeName(), hrcTagLocation)) {
        addPrototypeLocation(sub_elem, current_parse_prototype);
      }
      else if (!current_parse_prototype->pimpl->isPackage &&
               (xercesc::XMLString::equals(sub_elem->getNodeName(), hrcTagFilename) ||
                xercesc::XMLString::equals(sub_elem->getNodeName(), hrcTagFirstline)))
      {
        addPrototypeDetectParam(sub_elem, current_parse_prototype);
      }
      else if (!current_parse_prototype->pimpl->isPackage &&
               (xercesc::XMLString::equals(sub_elem->getNodeName(), hrcTagParametrs)))
      {
        addPrototypeParameters(sub_elem, current_parse_prototype);
      }
      else if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagAnnotation)) {
        // not read annotation
      }
      else {
        logger->warn("Unused element '{0}' in prototype '{1}'. Current file {2}.",
                     UStr::to_stdstr(elem->getNodeName()), current_parse_prototype->pimpl->name,
                     current_input_source->getPath());
      }
    }
  }
}

void HrcLibrary::Impl::addPrototypeLocation(const xercesc::DOMElement* elem,
                                            FileType* current_parse_prototype)
{
  const XMLCh* locationLink = elem->getAttribute(hrcLocationAttrLink);
  if (UStr::isEmpty(locationLink)) {
    logger->error("Bad 'location' link attribute in prototype '{0}'",
                  current_parse_prototype->pimpl->name);
    return;
  }
  current_parse_prototype->pimpl->inputSource = current_input_source->createRelative(locationLink);
}

void HrcLibrary::Impl::addPrototypeDetectParam(const xercesc::DOMElement* elem,
                                               FileType* current_parse_prototype)
{
  if (elem->getFirstChild() == nullptr ||
      elem->getFirstChild()->getNodeType() != xercesc::DOMNode::TEXT_NODE)
  {
    logger->warn("Bad '{0}' element in prototype '{1}'", UStr::to_stdstr(elem->getNodeName()),
                 current_parse_prototype->pimpl->name);
    return;
  }
  auto elem_text = static_cast<xercesc::DOMText*>(elem->getFirstChild());

  UnicodeString dmatch = UnicodeString(elem_text->getData());
  auto matchRE = std::make_unique<CRegExp>(&dmatch);
  matchRE->setPositionMoves(true);
  if (!matchRE->isOk()) {
    logger->warn("Fault compiling chooser RE '{0}' in prototype '{1}'", dmatch,
                 current_parse_prototype->pimpl->name);
    return;
  }
  auto ctype = xercesc::XMLString::equals(elem->getNodeName(), hrcTagFilename)
      ? FileTypeChooser::ChooserType::CT_FILENAME
      : FileTypeChooser::ChooserType::CT_FIRSTLINE;
  double prior = ctype == FileTypeChooser::ChooserType::CT_FILENAME ? 2 : 1;
  auto weight = elem->getAttribute(hrcFilenameAttrWeight);
  if (!UStr::isEmpty(weight)) {
    try {
      auto w = xercesc::XMLDouble(weight);
      if (w.getValue() < 0) {
        logger->warn(
            "Weight must be greater than 0. Current value {0}. Default value will be used. Current "
            "file {1}.",
            w.getValue(), current_input_source->getPath());
      }
      else {
        prior = w.getValue();
      }
    } catch (xercesc::NumberFormatException& toCatch) {
      logger->warn(
          "Weight '{0}' is not valid for the prototype '{1}'. Message: {2}. Default value will be "
          "used. Current file {3}.",
          UStr::to_stdstr(weight), current_parse_prototype->getName(),
          UStr::to_stdstr(toCatch.getMessage()), current_input_source->getPath());
    }
  }
  current_parse_prototype->pimpl->chooserVector.emplace_back(ctype, prior, matchRE.release());
}

void HrcLibrary::Impl::addPrototypeParameters(const xercesc::DOMNode* elem,
                                              FileType* current_parse_prototype)
{
  for (auto node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagParam)) {
        auto name = subelem->getAttribute(hrcParamAttrName);
        auto value = subelem->getAttribute(hrcParamAttrValue);
        auto descr = subelem->getAttribute(hrcParamAttrDescription);
        if (UStr::isEmpty(name) || UStr::isEmpty(value)) {
          logger->warn("Bad parameter in prototype '{0}'", current_parse_prototype->getName());
          continue;
        }
        auto& tp =
            current_parse_prototype->pimpl->addParam(UnicodeString(name), UnicodeString(value));
        if (!UStr::isEmpty(descr)) {
          tp.description.emplace(descr);
        }
      }
      else {
        logger->warn("Unused element '{0}' in prototype '{1}'. Current file {2}.",
                     UStr::to_stdstr(elem->getNodeName()), current_parse_prototype->pimpl->name,
                     current_input_source->getPath());
      }
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      addPrototypeParameters(node, current_parse_prototype);
    }
  }
}

void HrcLibrary::Impl::addType(const xercesc::DOMElement* elem)
{
  auto typeName = elem->getAttribute(hrcTypeAttrName);

  if (UStr::isEmpty(typeName)) {
    logger->error("Unnamed type found");
    return;
  }
  auto d_name = UnicodeString(typeName);
  auto type_ref = fileTypeHash.find(d_name);
  if (type_ref == fileTypeHash.end()) {
    logger->error("Type '%s' without prototype", d_name);
    return;
  }
  auto type = type_ref->second;
  if (type->pimpl->type_loading) {
    logger->warn("Type '{0}' is loading already. Current file {1}", UStr::to_stdstr(typeName),
                 current_input_source->getPath());
    return;
  }

  type->pimpl->type_loading = true;

  FileType* o_parseType = current_parse_type;
  current_parse_type = type;

  parseTypeBlock(elem);

  auto baseSchemeName = qualifyOwnName(type->getName());
  if (baseSchemeName != nullptr) {
    auto sh = schemeHash.find(*baseSchemeName);
    type->pimpl->baseScheme = sh == schemeHash.end() ? nullptr : sh->second;
  }
  if (type->pimpl->baseScheme == nullptr && !type->pimpl->isPackage) {
    logger->warn("Type '{0}' has no default scheme", UStr::to_stdstr(typeName));
  }
  type->pimpl->loadDone = true;
  current_parse_type = o_parseType;
}

void HrcLibrary::Impl::parseTypeBlock(const xercesc::DOMNode* elem)
{
  for (auto node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagRegion)) {
        addTypeRegion(subelem);
      }
      else if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagEntity)) {
        addTypeEntity(subelem);
      }
      else if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagImport)) {
        addTypeImport(subelem);
      }
      else if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagScheme)) {
        addScheme(subelem);
      }
      else if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagAnnotation)) {
        // not read anotation
      }
    }
    // случай entity ссылки на другой файл
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      parseTypeBlock(node);
    }
  }
}

void HrcLibrary::Impl::addTypeRegion(const xercesc::DOMElement* elem)
{
  auto regionName = elem->getAttribute(hrcRegionAttrName);
  auto regionParent = elem->getAttribute(hrcRegionAttrParent);
  auto regionDescr = elem->getAttribute(hrcRegionAttrDescription);
  if (UStr::isEmpty(regionName)) {
    logger->error("No 'name' attribute in <region> element");
    return;
  }
  auto qname1 = qualifyOwnName(UnicodeString(regionName));
  if (qname1 == nullptr) {
    return;
  }
  if (regionNamesHash.find(*qname1) != regionNamesHash.end()) {
    logger->warn("Duplicate region '{0}' definition in type '{1}'", *qname1,
                 current_parse_type->getName());
    return;
  }

  UnicodeString d_regionparent = UnicodeString(regionParent);
  auto qname2 = qualifyForeignName(*regionParent != '\0' ? &d_regionparent : nullptr,
                                   QualifyNameType::QNT_DEFINE, true);

  UnicodeString regiondescr = UnicodeString(regionDescr);
  const Region* region =
      new Region(*qname1, &regiondescr, getRegion(qname2.get()), regionNamesVector.size());
  regionNamesVector.push_back(region);
  regionNamesHash.emplace(*qname1, region);
}

void HrcLibrary::Impl::addTypeEntity(const xercesc::DOMElement* elem)
{
  auto entityName = elem->getAttribute(hrcEntityAttrName);
  auto entityValue = elem->getAttribute(hrcEntityAttrValue);
  if (UStr::isEmpty(entityName) || entityValue == nullptr) {
    logger->error("Bad entity attributes");
    return;
  }
  auto dentityValue = UnicodeString(entityValue);
  auto qname1 = qualifyOwnName(UnicodeString(entityName));
  uUnicodeString qname2 = useEntities(&dentityValue);
  if (qname1 != nullptr && qname2 != nullptr) {
    schemeEntitiesHash.emplace(*qname1, qname2.release());
  }
}

void HrcLibrary::Impl::addTypeImport(const xercesc::DOMElement* elem)
{
  auto typeParam = elem->getAttribute(hrcImportAttrType);
  UnicodeString typeparam = UnicodeString(typeParam);
  if (UStr::isEmpty(typeParam) || fileTypeHash.find(typeparam) == fileTypeHash.end()) {
    logger->error("Import with bad '{0}' attribute in type '{1}'", typeparam,
                  current_parse_type->pimpl->name);
    return;
  }
  current_parse_type->pimpl->importVector.emplace_back(typeParam);
}

void HrcLibrary::Impl::addScheme(const xercesc::DOMElement* elem)
{
  auto schemeName = elem->getAttribute(hrcSchemeAttrName);
  UnicodeString dschemeName = UnicodeString(schemeName);
  // todo check schemeName
  auto qSchemeName = qualifyOwnName(dschemeName);
  if (qSchemeName == nullptr) {
    logger->error("bad scheme name in type '{0}'", current_parse_type->pimpl->name);
    return;
  }
  if (schemeHash.find(*qSchemeName) != schemeHash.end() ||
      disabledSchemes.find(*qSchemeName) != disabledSchemes.end())
  {
    logger->error("duplicate scheme name '{0}'", *qSchemeName);
    return;
  }

  auto* scheme = new SchemeImpl(qSchemeName.get());
  scheme->fileType = current_parse_type;

  schemeHash.emplace(*scheme->getName(), scheme);
  const XMLCh* condIf = elem->getAttribute(hrcSchemeAttrIf);
  const XMLCh* condUnless = elem->getAttribute(hrcSchemeAttrUnless);
  const UnicodeString* p1 = current_parse_type->getParamValue(UnicodeString(condIf));
  const UnicodeString* p2 = current_parse_type->getParamValue(UnicodeString(condUnless));
  if ((*condIf != '\0' && p1 && p1->compare("true") != 0) ||
      (*condUnless != '\0' && p2 && p2->compare("true") == 0))
  {
    // disabledSchemes.put(scheme->schemeName, 1);
    return;
  }
  parseSchemeBlock(scheme, elem);
}

void HrcLibrary::Impl::parseSchemeBlock(SchemeImpl* scheme, const xercesc::DOMNode* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagInherit)) {
        addSchemeInherit(scheme, subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagRegexp)) {
        addSchemeRegexp(scheme, subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagBlock)) {
        addSchemeBlock(scheme, subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagKeywords)) {
        parseSchemeKeywords(scheme, subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagAnnotation)) {
        // not read anotation
        continue;
      }
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      parseSchemeBlock(scheme, node);
    }
  }
}

void HrcLibrary::Impl::addSchemeInherit(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  const XMLCh* nqSchemeName = elem->getAttribute(hrcInheritAttrScheme);
  if (UStr::isEmpty(nqSchemeName)) {
    logger->error(
        "there is empty scheme name in inherit block of scheme '{0}', skip this inherit block.",
        *scheme->schemeName);
    return;
  }
  auto scheme_node = std::make_unique<SchemeNodeInherit>();
  scheme_node->schemeName = std::make_unique<UnicodeString>(nqSchemeName);
  auto schemeName =
      qualifyForeignName(scheme_node->schemeName.get(), QualifyNameType::QNT_SCHEME, false);
  if (schemeName != nullptr) {
    scheme_node->scheme = schemeHash.find(*schemeName)->second;
    scheme_node->schemeName = std::move(schemeName);
  }

  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagVirtual)) {
        const XMLCh* x_schemeName = subelem->getAttribute(hrcVirtualAttrScheme);
        const XMLCh* x_substName = subelem->getAttribute(hrcVirtualAttrSubstScheme);
        if (UStr::isEmpty(x_schemeName) || UStr::isEmpty(x_substName)) {
          logger->error(
              "there is bad virtualize attributes of scheme '{0}', skip this virtual block.",
              *scheme_node->schemeName);
          continue;
        }
        UnicodeString d_schemeName = UnicodeString(x_schemeName);
        UnicodeString d_substName = UnicodeString(x_substName);
        scheme_node->virtualEntryVector.emplace_back(new VirtualEntry(&d_schemeName, &d_substName));
      }
    }
  }
  scheme->nodes.push_back(std::move(scheme_node));
}

void HrcLibrary::Impl::addSchemeRegexp(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  const XMLCh* matchParam = elem->getAttribute(hrcRegexpAttrMatch);

  if (UStr::isEmpty(matchParam)) {
    matchParam = getElementText(elem);
  }

  if (UStr::isEmpty(matchParam)) {
    logger->error(
        "there is no 'match' attribute in regexp of scheme '{0}', skip this regexp block.",
        *scheme->schemeName);
    return;
  }

  auto dmatchParam = UnicodeString(matchParam);
  auto entMatchParam = useEntities(&dmatchParam);
  auto regexp = std::make_unique<CRegExp>(entMatchParam.get());
  if (!regexp->isOk()) {
    logger->error("fault compiling regexp '{0}' of scheme '{1}', skip this regexp block.",
                  *entMatchParam, *scheme->schemeName);
    return;
  }

  auto scheme_node = std::make_unique<SchemeNodeRegexp>();
  auto dhrcRegexpAttrPriority = UnicodeString(elem->getAttribute(hrcRegexpAttrPriority));
  scheme_node->lowPriority = UnicodeString(value_low).compare(dhrcRegexpAttrPriority) == 0;
  scheme_node->start = std::move(regexp);
  scheme_node->start->setPositionMoves(false);

  loadRegions(scheme_node.get(), elem);
  if (scheme_node->region) {
    scheme_node->regions[0] = scheme_node->region;
  }

  scheme->nodes.push_back(std::move(scheme_node));
}

void HrcLibrary::Impl::addSchemeBlock(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  const XMLCh* start_param = elem->getAttribute(hrcBlockAttrStart);
  const XMLCh* end_param = elem->getAttribute(hrcBlockAttrEnd);

  xercesc::DOMElement *element_start = nullptr, *element_end = nullptr;

  for (xercesc::DOMNode* blkn = elem->getFirstChild();
       blkn && !(!UStr::isEmpty(end_param) && !UStr::isEmpty(start_param));
       blkn = blkn->getNextSibling())
  {
    xercesc::DOMElement* blkel;
    if (blkn->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      blkel = static_cast<xercesc::DOMElement*>(blkn);
    }
    else {
      continue;
    }

    const XMLCh* p = nullptr;
    if (blkel->getAttributeNode(hrcBlockAttrMatch)) {
      p = blkel->getAttribute(hrcBlockAttrMatch);
    }
    else {
      p = getElementText(blkel);
    }

    if (xercesc::XMLString::equals(blkel->getNodeName(), hrcBlockAttrStart)) {
      start_param = p;
      element_start = blkel;
    }
    else if (xercesc::XMLString::equals(blkel->getNodeName(), hrcBlockAttrEnd)) {
      end_param = p;
      element_end = blkel;
    }
  }

  if (UStr::isEmpty(start_param)) {
    logger->error("there is no 'start' attribute in block of scheme '{0}', skip this block.",
                  *scheme->schemeName);
    return;
  }
  if (UStr::isEmpty(end_param)) {
    logger->error("there is no 'end' attribute in block of scheme '{0}', skip this block.",
                  *scheme->schemeName);
    return;
  }
  const XMLCh* schemeName = elem->getAttribute(hrcBlockAttrScheme);
  if (UStr::isEmpty(schemeName)) {
    logger->error("there is no 'scheme' attribute in block of scheme '{0}', skip this block.",
                  *scheme->schemeName);
    return;
  }

  auto dsParam = UnicodeString(start_param);
  uUnicodeString startParam = useEntities(&dsParam);
  auto start_regexp = std::make_unique<CRegExp>(startParam.get());
  start_regexp->setPositionMoves(false);
  if (!start_regexp->isOk()) {
    logger->error("fault compiling start regexp '{0}' in block of scheme '{1}', skip this block.",
                  *startParam, *scheme->schemeName);
    return;
  }

  auto deParam = UnicodeString(end_param);
  uUnicodeString endParam = useEntities(&deParam);
  auto end_regexp = std::make_unique<CRegExp>();
  end_regexp->setPositionMoves(true);
  end_regexp->setBackRE(start_regexp.get());
  end_regexp->setRE(endParam.get());
  if (!end_regexp->isOk()) {
    logger->error("fault compiling end regexp '{0}' in block of scheme '{1}', skip this block.",
                  *startParam, *scheme->schemeName);
    return;
  }

  UnicodeString attr_pr = UnicodeString(elem->getAttribute(hrcBlockAttrPriority));
  UnicodeString attr_cpr = UnicodeString(elem->getAttribute(hrcBlockAttrContentPriority));
  UnicodeString attr_ireg = UnicodeString(elem->getAttribute(hrcBlockAttrInnerRegion));

  auto scheme_node = std::make_unique<SchemeNodeBlock>();
  scheme_node->schemeName = std::make_unique<UnicodeString>(schemeName);
  scheme_node->lowPriority = UnicodeString(value_low).compare(attr_pr) == 0;
  scheme_node->lowContentPriority = UnicodeString(value_low).compare(attr_cpr) == 0;
  scheme_node->innerRegion = UnicodeString(value_yes).compare(attr_ireg) == 0;
  scheme_node->start = std::move(start_regexp);
  scheme_node->end = std::move(end_regexp);

  loadBlockRegions(scheme_node.get(), elem);
  loadRegions(scheme_node.get(), element_start, true);
  loadRegions(scheme_node.get(), element_end, false);
  scheme->nodes.push_back(std::move(scheme_node));
}

const XMLCh* HrcLibrary::Impl::getElementText(const xercesc::DOMElement* blkel) const
{
  const XMLCh* p = nullptr;
  // возможно текст указан как ...
  for (xercesc::DOMNode* child = blkel->getFirstChild(); child != nullptr;
       child = child->getNextSibling())
  {
    if (child->getNodeType() == xercesc::DOMNode::CDATA_SECTION_NODE) {
      // ... блок CDATA:  например <regexp>![CDATA[ match_regexp ]]</regexp>
      auto cdata = static_cast<xercesc::DOMCDATASection*>(child);
      p = cdata->getData();
      break;
    }
    if (child->getNodeType() == xercesc::DOMNode::TEXT_NODE) {
      // ... текстовый блок <regexp> match_regexp </regexp>
      auto text = static_cast<xercesc::DOMText*>(child);
      const XMLCh* p1 = text->getData();
      auto temp_string = xercesc::XMLString::replicate(p1);
      // перед блоком CDATA могут быть пустые строки, учитываем это
      xercesc::XMLString::trim((XMLCh*) temp_string);
      if (!UStr::isEmpty(temp_string)) {
        p = p1;
        xercesc::XMLString::release(&temp_string);
        break;
      }
      xercesc::XMLString::release(&temp_string);
    }
  }
  return p;
}

void HrcLibrary::Impl::parseSchemeKeywords(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  XMLCh rg_tmpl[] = u"region\0";
  const Region* region = getNCRegion(elem, rg_tmpl);
  if (region == nullptr) {
    logger->error(
        "there is no 'region' attribute in keywords block of scheme '{0}', skip this keywords "
        "block.",
        *scheme->schemeName);
    return;
  }

  const XMLCh* worddiv = elem->getAttribute(hrcKeywordsAttrWorddiv);
  std::unique_ptr<CharacterClass> us_worddiv;
  if (!UStr::isEmpty(worddiv)) {
    auto dworddiv = UnicodeString(worddiv);
    uUnicodeString entWordDiv = useEntities(&dworddiv);
    us_worddiv = UStr::createCharClass(*entWordDiv.get(), 0, nullptr, false);
    if (us_worddiv == nullptr) {
      logger->error(
          "fault compiling worddiv regexp '{0}' in keywords block of scheme '{1}'. skip this "
          "keywords block.",
          *entWordDiv, *scheme->schemeName);
    }
  }

  auto count = getSchemeKeywordsCount(elem);
  auto ignorecase_string = UnicodeString(elem->getAttribute(hrcKeywordsAttrIgnorecase));
  auto scheme_node = std::make_unique<SchemeNodeKeywords>();
  scheme_node->worddiv = std::move(us_worddiv);
  scheme_node->kwList = std::make_unique<KeywordList>(count);
  scheme_node->kwList->matchCase = UnicodeString(value_yes).compare(ignorecase_string) != 0;

  loopSchemeKeywords(elem, scheme, scheme_node, region);
  scheme_node->kwList->firstChar->freeze();

  // TODO unique keywords in list
  scheme_node->kwList->sortList();
  scheme_node->kwList->substrIndex();
  scheme->nodes.push_back(std::move(scheme_node));
}

void HrcLibrary::Impl::loopSchemeKeywords(const xercesc::DOMNode* elem, const SchemeImpl* scheme,
                                          const std::unique_ptr<SchemeNodeKeywords>& scheme_node,
                                          const Region* region)
{
  for (auto keyword = elem->getFirstChild(); keyword; keyword = keyword->getNextSibling()) {
    if (keyword->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* sub_elem = static_cast<xercesc::DOMElement*>(keyword);
      if (xercesc::XMLString::equals(sub_elem->getNodeName(), hrcTagWord)) {
        addSchemeKeyword(sub_elem, scheme, scheme_node.get(), region,
                         KeywordInfo::KeywordType::KT_WORD);
      }
      else if (xercesc::XMLString::equals(sub_elem->getNodeName(), hrcTagSymb)) {
        addSchemeKeyword(sub_elem, scheme, scheme_node.get(), region,
                         KeywordInfo::KeywordType::KT_SYMB);
      }
    }
    else if (keyword->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      loopSchemeKeywords(keyword, scheme, scheme_node, region);
    }
  }
}

void HrcLibrary::Impl::addSchemeKeyword(const xercesc::DOMElement* elem, const SchemeImpl* scheme,
                                        SchemeNodeKeywords* scheme_node, const Region* region,
                                        KeywordInfo::KeywordType keyword_type)
{
  const XMLCh* keyword_value = elem->getAttribute(hrcWordAttrName);
  if (UStr::isEmpty(keyword_value)) {
    logger->warn(
        "the 'name' attribute in the '{1}' element of scheme '{0}' is empty or missing, skip it.",
        *scheme->schemeName, keyword_type == KeywordInfo::KeywordType::KT_WORD ? "word" : "symb");
    return;
  }

  const Region* rgn = region;
  const XMLCh* reg = elem->getAttribute(hrcWordAttrRegion);
  if (!UStr::isEmpty(reg)) {
    rgn = getNCRegion(elem, hrcWordAttrRegion);
  }

  KeywordInfo& list = scheme_node->kwList->kwList[scheme_node->kwList->count];
  list.keyword = std::make_unique<UnicodeString>(keyword_value);
  list.region = rgn;
  list.isSymbol = (keyword_type == KeywordInfo::KeywordType::KT_SYMB);
  auto first_char = scheme_node->kwList->firstChar.get();
  first_char->add(keyword_value[0]);
  if (!scheme_node->kwList->matchCase) {
    first_char->add(Character::toLowerCase(keyword_value[0]));
    first_char->add(Character::toUpperCase(keyword_value[0]));
    first_char->add(Character::toTitleCase(keyword_value[0]));
  }
  scheme_node->kwList->count++;
  if (scheme_node->kwList->minKeywordLength > list.keyword->length()) {
    scheme_node->kwList->minKeywordLength = list.keyword->length();
  }
}

size_t HrcLibrary::Impl::getSchemeKeywordsCount(const xercesc::DOMNode* elem)
{
  size_t result = 0;
  for (xercesc::DOMNode* keyword = elem->getFirstChild(); keyword;
       keyword = keyword->getNextSibling())
  {
    if (keyword->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* sub_elem = static_cast<xercesc::DOMElement*>(keyword);
      if ((xercesc::XMLString::equals(sub_elem->getNodeName(), hrcTagWord) ||
           xercesc::XMLString::equals(sub_elem->getNodeName(), hrcTagSymb)) &&
          !UStr::isEmpty(sub_elem->getAttribute(hrcWordAttrName)))
      {
        result++;
      }
      continue;
    }
    if (keyword->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      result += getSchemeKeywordsCount(keyword);
    }
  }
  return result;
}

void HrcLibrary::Impl::loadRegions(SchemeNodeRegexp* node, const xercesc::DOMElement* el)
{
  XMLCh rg_tmpl[] = u"region\0\0";

  if (el) {
    if (node->region == nullptr) {
      node->region = getNCRegion(el, rg_tmpl);
    }

    for (int i = 0; i < REGIONS_NUM; i++) {
      rg_tmpl[6] = static_cast<XMLCh>((i < 0xA ? i : i + 39) + '0');
      node->regions[i] = getNCRegion(el, rg_tmpl);
    }
  }

  for (int i = 0; i < NAMED_REGIONS_NUM; i++) {
    node->regionsn[i] = getNCRegion(node->start->getBracketName(i), false);
  }
}

void HrcLibrary::Impl::loadRegions(SchemeNodeBlock* node, const xercesc::DOMElement* el,
                                   bool start_element)
{
  XMLCh rg_tmpl[] = u"region\0\0";

  if (el) {
    if (node->region == nullptr) {
      node->region = getNCRegion(el, rg_tmpl);
    }

    for (int i = 0; i < REGIONS_NUM; i++) {
      rg_tmpl[6] = static_cast<XMLCh>((i < 0xA ? i : i + 39) + '0');

      if (start_element) {
        node->regions[i] = getNCRegion(el, rg_tmpl);
      }
      else {
        node->regione[i] = getNCRegion(el, rg_tmpl);
      }
    }
  }

  for (int i = 0; i < NAMED_REGIONS_NUM; i++) {
    if (start_element) {
      node->regionsn[i] = getNCRegion(node->start->getBracketName(i), false);
    }
    else {
      node->regionen[i] = getNCRegion(node->end->getBracketName(i), false);
    }
  }
}

void HrcLibrary::Impl::loadBlockRegions(SchemeNodeBlock* node, const xercesc::DOMElement* el)
{
  // regions as attributes in main block element

  XMLCh rg_tmpl[] = u"region\0\0\0";

  node->region = getNCRegion(el, rg_tmpl);
  for (int i = 0; i < REGIONS_NUM; i++) {
    rg_tmpl[6] = '0';
    rg_tmpl[7] = static_cast<XMLCh>((i < 0xA ? i : i + 39) + '0');
    node->regions[i] = getNCRegion(el, rg_tmpl);
    rg_tmpl[6] = '1';
    node->regione[i] = getNCRegion(el, rg_tmpl);
  }
}

void HrcLibrary::Impl::updateSchemeLink(uUnicodeString& scheme_name, SchemeImpl** scheme_impl,
                                        byte scheme_type, SchemeImpl* current_scheme)
{
  static const char* message[4] = {
      "cannot resolve scheme name '{0}' of block in scheme '{1}'",
      "cannot resolve scheme name '{0}' of inherit in scheme '{1}'",
      "cannot resolve scheme name '{0}' of virtual in scheme '{1}'",
      "cannot resolve subst-scheme name '{0}' of virtual in scheme '{1}'"};

  if (scheme_name != nullptr && *scheme_impl == nullptr) {
    auto schemeName = qualifyForeignName(scheme_name.get(), QualifyNameType::QNT_SCHEME, true);
    if (schemeName != nullptr) {
      *scheme_impl = schemeHash.find(*schemeName)->second;
    }
    else {
      logger->error(message[scheme_type], *scheme_name, *current_scheme->schemeName);
    }

    scheme_name.reset();
  }
}

void HrcLibrary::Impl::updateLinks()
{
  while (structureChanged) {
    structureChanged = false;
    for (auto& scheme_it : schemeHash) {
      SchemeImpl* scheme = scheme_it.second;
      if (!scheme->fileType->pimpl->loadDone) {
        continue;
      }
      FileType* old_parseType = current_parse_type;
      current_parse_type = scheme->fileType;
      for (auto& snode : scheme->nodes) {
        if (snode->type == SchemeNode::SchemeNodeType::SNT_BLOCK) {
          auto snode_block = static_cast<SchemeNodeBlock*>(snode.get());

          updateSchemeLink(snode_block->schemeName, &snode_block->scheme, 1, scheme);
        }

        if (snode->type == SchemeNode::SchemeNodeType::SNT_INHERIT) {
          auto snode_inherit = static_cast<SchemeNodeInherit*>(snode.get());

          updateSchemeLink(snode_inherit->schemeName, &snode_inherit->scheme, 2, scheme);
          for (auto vt : snode_inherit->virtualEntryVector) {
            updateSchemeLink(vt->virtSchemeName, &vt->virtScheme, 3, scheme);
            updateSchemeLink(vt->substSchemeName, &vt->substScheme, 4, scheme);
          }
        }
      }
      current_parse_type = old_parseType;
      if (structureChanged) {
        break;
      }
    }
  }
}

uUnicodeString HrcLibrary::Impl::qualifyOwnName(const UnicodeString& name)
{
  auto colon = name.indexOf(':');
  if (colon != -1) {
    if (UnicodeString(name, 0, colon).compare(current_parse_type->pimpl->name) != 0) {
      logger->error("type name qualifer in '{0}' doesn't match current type '{1}'", name,
                    current_parse_type->pimpl->name);
      return nullptr;
    }
    else {
      return std::make_unique<UnicodeString>(name);
    }
  }
  else {
    auto sbuf = std::make_unique<UnicodeString>(current_parse_type->getName());
    sbuf->append(":").append(name);
    return sbuf;
  }
}

bool HrcLibrary::Impl::checkNameExist(const UnicodeString* name, FileType* parseType,
                                      QualifyNameType qntype, bool logErrors)
{
  if (qntype == QualifyNameType::QNT_DEFINE && regionNamesHash.find(*name) == regionNamesHash.end())
  {
    if (logErrors)
      logger->error("region '{0}', referenced in type '{1}', is not defined", *name,
                    parseType->pimpl->name);
    return false;
  }
  else if (qntype == QualifyNameType::QNT_ENTITY &&
           schemeEntitiesHash.find(*name) == schemeEntitiesHash.end())
  {
    if (logErrors)
      logger->error("entity '{0}', referenced in type '{1}', is not defined", *name,
                    parseType->pimpl->name);
    return false;
  }
  else if (qntype == QualifyNameType::QNT_SCHEME && schemeHash.find(*name) == schemeHash.end()) {
    if (logErrors)
      logger->error("scheme '{0}', referenced in type '{1}', is not defined", *name,
                    parseType->pimpl->name);
    return false;
  }
  return true;
}

uUnicodeString HrcLibrary::Impl::qualifyForeignName(const UnicodeString* name,
                                                    QualifyNameType qntype, bool logErrors)
{
  if (name == nullptr) {
    return nullptr;
  }
  auto colon = name->indexOf(':');
  if (colon != -1) {  // qualified name
    UnicodeString prefix(*name, 0, colon);
    auto ft = fileTypeHash.find(prefix);
    FileType* prefType = nullptr;
    if (ft != fileTypeHash.end()) {
      prefType = ft->second;
    }

    if (prefType == nullptr) {
      if (logErrors) {
        logger->error("type name qualifer in '{0}' doesn't match any type", *name);
      }
      return nullptr;
    }
    else if (!prefType->pimpl->type_loading) {
      loadFileType(prefType);
    }
    if (prefType == current_parse_type || prefType->pimpl->type_loading) {
      return checkNameExist(name, prefType, qntype, logErrors)
          ? std::make_unique<UnicodeString>(*name)
          : nullptr;
    }
  }
  else {  // unqualified name
    for (int idx = -1; current_parse_type != nullptr &&
         idx < static_cast<int>(current_parse_type->pimpl->importVector.size());
         idx++)
    {
      auto tname = current_parse_type->getName();
      if (idx > -1) {
        tname = current_parse_type->pimpl->importVector.at(idx);
      }
      FileType* importer = fileTypeHash.find(tname)->second;
      if (!importer->pimpl->type_loading) {
        loadFileType(importer);
      }

      auto qname = std::make_unique<UnicodeString>(tname);
      qname->append(":").append(*name);
      if (checkNameExist(qname.get(), importer, qntype, false)) {
        return qname;
      }
    }
    if (logErrors) {
      logger->error("unqualified name '{0}' doesn't belong to any imported type [{1}]", *name,
                    current_input_source->getPath());
    }
  }
  return nullptr;
}

uUnicodeString HrcLibrary::Impl::useEntities(const UnicodeString* name)
{
  int copypos = 0;
  int32_t epos = 0;

  if (!name) {
    return nullptr;
  }
  auto newname = std::make_unique<UnicodeString>();

  while (true) {
    epos = name->indexOf('%', epos);
    if (epos == -1) {
      epos = name->length();
      break;
    }
    if (epos && (*name)[epos - 1] == '\\') {
      epos++;
      continue;
    }
    auto elpos = name->indexOf(';', epos);
    if (elpos == -1) {
      epos = name->length();
      break;
    }
    UnicodeString enname(*name, epos + 1, elpos - epos - 1);

    auto qEnName = qualifyForeignName(&enname, QualifyNameType::QNT_ENTITY, true);
    const UnicodeString* enval = nullptr;
    if (qEnName != nullptr) {
      enval = schemeEntitiesHash.find(*qEnName)->second;
    }
    if (enval == nullptr) {
      epos++;
      continue;
    }
    newname->append(UnicodeString(*name, copypos, epos - copypos));
    newname->append(*enval);
    epos = elpos + 1;
    copypos = epos;
  }
  if (epos > copypos) {
    newname->append(UnicodeString(*name, copypos, epos - copypos));
  }
  return newname;
}

const Region* HrcLibrary::Impl::getNCRegion(const UnicodeString* name, bool logErrors)
{
  if (name == nullptr || name->isEmpty()) {
    return nullptr;
  }
  const Region* reg = nullptr;
  auto qname = qualifyForeignName(name, QualifyNameType::QNT_DEFINE, logErrors);
  if (qname == nullptr) {
    return nullptr;
  }
  auto reg_ = regionNamesHash.find(*qname);
  if (reg_ != regionNamesHash.end()) {
    reg = reg_->second;
  }

  /** Check for 'default' region request.
      Regions with this name are always transparent
  */
  if (reg != nullptr) {
    auto s_name = reg->getName();
    auto idx = s_name.indexOf(":default");
    if (idx != -1 && idx + 8 == s_name.length()) {
      return nullptr;
    }
  }
  return reg;
}

const Region* HrcLibrary::Impl::getNCRegion(const xercesc::DOMElement* el, const XMLCh* tag)
{
  const XMLCh* par = el->getAttribute(tag);
  if (UStr::isEmpty(par)) {
    return nullptr;
  }
  UnicodeString dpar = UnicodeString(par);
  return getNCRegion(&dpar, true);
}
