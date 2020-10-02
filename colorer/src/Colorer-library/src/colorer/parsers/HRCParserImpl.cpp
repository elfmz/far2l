#include <memory>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <cmath>
#include <cstdio>
#include <colorer/parsers/SchemeImpl.h>
#include <colorer/parsers/HRCParserImpl.h>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <colorer/xml/XmlInputSource.h>
#include <colorer/xml/BaseEntityResolver.h>
#include <colorer/xml/XmlTagDefs.h>
#include <colorer/xml/XStr.h>
#include <colorer/unicode/UnicodeTools.h>
#include <colorer/unicode/Character.h>

HRCParserImpl::HRCParserImpl():
  versionName(nullptr), parseProtoType(nullptr), parseType(nullptr), current_input_source(nullptr),
  structureChanged(false), updateStarted(false)
{
  fileTypeHash.reserve(200);
  fileTypeVector.reserve(150);
  regionNamesHash.reserve(1000);
  regionNamesVector.reserve(1000);
  schemeHash.reserve(4000);
}

HRCParserImpl::~HRCParserImpl()
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

  delete versionName;
}

void HRCParserImpl::loadSource(XmlInputSource* is)
{
  if (!is) {
    throw HRCParserException(CString("Can't open stream - 'null' is bad stream."));
  }

  XmlInputSource* istemp = current_input_source;
  current_input_source = is;
  try {
    parseHRC(is);
  } catch (Exception &) {
    current_input_source = istemp;
    throw;
  }
  current_input_source = istemp;
}

void HRCParserImpl::unloadFileType(FileTypeImpl* filetype)
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

void HRCParserImpl::loadFileType(FileType* filetype)
{
  auto* thisType = static_cast<FileTypeImpl*>(filetype);
  if (thisType == nullptr || thisType->type_loaded || thisType->input_source_loading || thisType->load_broken) {
    return;
  }

  thisType->input_source_loading = true;

  try {
    loadSource(thisType->inputSource.get());
  } catch (InputSourceException &e) {
    logger->error("Can't open source stream: {0}", e.what());
    thisType->load_broken = true;
  } catch (HRCParserException &e) {
    logger->error("{0} [{1}]", e.what(), thisType->inputSource ? XStr(thisType->inputSource->getInputSource()->getSystemId()).get_char():"");
    thisType->load_broken = true;
  } catch (Exception &e) {
    logger->error("{0} [{1}]", e.what(), thisType->inputSource ? XStr(thisType->inputSource->getInputSource()->getSystemId()).get_char() : "");
    thisType->load_broken = true;
  } catch (...) {
    logger->error("Unknown exception while loading {0}", XStr(thisType->inputSource->getInputSource()->getSystemId()).get_char());
    thisType->load_broken = true;
  }

  thisType->input_source_loading = false;
}

FileType* HRCParserImpl::chooseFileType(const String* fileName, const String* firstLine, int typeNo)
{
  FileTypeImpl* best = nullptr;
  double max_prior = 0;
  const double DELTA = 1e-6;
  for (auto ret : fileTypeVector) {
    double prior = ret->getPriority(fileName, firstLine);

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

FileType* HRCParserImpl::getFileType(const String* name)
{
  if (name == nullptr) {
    return nullptr;
  }
  auto filetype = fileTypeHash.find(name);
  if (filetype != fileTypeHash.end())
    return filetype->second;
  else
    return nullptr;
}

FileType* HRCParserImpl::enumerateFileTypes(int index)
{
  if (index < fileTypeVector.size()) {
    return fileTypeVector[index];
  }
  return nullptr;
}

size_t HRCParserImpl::getFileTypesCount()
{
  return fileTypeVector.size();
}

size_t HRCParserImpl::getRegionCount()
{
  return regionNamesVector.size();
}

const Region* HRCParserImpl::getRegion(int id)
{
  if (id < 0 || id >= regionNamesVector.size()) {
    return nullptr;
  }
  return regionNamesVector[id];
}

const Region* HRCParserImpl::getRegion(const String* name)
{
  if (name == nullptr) {
    return nullptr;
  }
  return getNCRegion(name, false); // regionNamesHash.get(name);
}

const String* HRCParserImpl::getVersion()
{
  return versionName;
}


// protected methods


void HRCParserImpl::parseHRC(XmlInputSource* is)
{
  logger->debug("begin parse '{0}'", *XStr(is->getInputSource()->getSystemId()).get_stdstr());
  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  BaseEntityResolver resolver;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setXMLEntityResolver(&resolver);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  xml_parser.parse(*is->getInputSource());
  if (error_handler.getSawErrors()) {
    throw HRCParserException(SString("Error reading hrc file '") + CString(is->getInputSource()->getSystemId()) + "'");
  }
  xercesc::DOMDocument* doc = xml_parser.getDocument();
  xercesc::DOMElement* root = doc->getDocumentElement();

  if (root && !xercesc::XMLString::equals(root->getNodeName(), hrcTagHrc)) {
    throw HRCParserException(SString("Incorrect hrc-file structure. Main '<hrc>' block not found. Current file ") + \
                             CString(is->getInputSource()->getSystemId()));
  }
  if (versionName == nullptr) {
    versionName = new CString(root->getAttribute(hrcHrcAttrVersion));
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

  logger->debug("end parse '{0}'", *XStr(is->getInputSource()->getSystemId()).get_stdstr());
}

void HRCParserImpl::parseHrcBlock(const xercesc::DOMElement* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* sub_elem = static_cast<xercesc::DOMElement*>(node);
      parseHrcBlockElements(sub_elem);
      continue;
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      for (xercesc::DOMNode* sub_node = node->getFirstChild(); sub_node != nullptr; sub_node = sub_node->getNextSibling()) {
        auto* sub_elem = static_cast<xercesc::DOMElement*>(sub_node);
        parseHrcBlockElements(sub_elem);
      }
    }
  }
}

void HRCParserImpl::parseHrcBlockElements(const xercesc::DOMElement* elem)
{
  if (elem->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
    if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagPrototype)) {
      addPrototype(elem);
      return;
    }
    if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagPackage)) {
      addPrototype(elem);
      return;
    }
    if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagType)) {
      addType(elem);
      return;
    }
    if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagAnnotation)) {
      // not read anotation
      return;
    }
    logger->warn("Unused element '{0}'. Current file {1}.", *XStr(elem->getNodeName()).get_stdstr() , *XStr(current_input_source->getInputSource()->getSystemId()).get_stdstr());
  }
}

void HRCParserImpl::addPrototype(const xercesc::DOMElement* elem)
{
  const XMLCh* typeName = elem->getAttribute(hrcPrototypeAttrName);
  const XMLCh* typeGroup = elem->getAttribute(hrcPrototypeAttrGroup);
  const XMLCh* typeDescription = elem->getAttribute(hrcPrototypeAttrDescription);
  if (*typeName == '\0') {
    logger->error("unnamed prototype");
    return;
  }
  if (typeDescription == nullptr) {
    typeDescription = typeName;
  }
  FileTypeImpl* f = nullptr;
  CString tname = CString(typeName);
  auto ft = fileTypeHash.find(&tname);
  if (ft != fileTypeHash.end()) {
    f = ft->second;
  }
  if (f != nullptr) {
    unloadFileType(f);
    logger->warn("Duplicate prototype '{0}'", tname.getChars());
    //  return;
  }
  auto* type = new FileTypeImpl(this);
  type->name.reset(new SString(CString(typeName)));
  type->description.reset(new SString(CString(typeDescription)));
  if (typeGroup != nullptr) {
    type->group.reset(new SString(CString(typeGroup)));
  }
  if (xercesc::XMLString::equals(elem->getNodeName(), hrcTagPackage)) {
    type->isPackage = true;
  }
  parseProtoType = type;
  parsePrototypeBlock(elem);

  type->protoLoaded = true;
  std::pair<SString, FileTypeImpl*> pp(type->getName(), type);
  fileTypeHash.emplace(pp);

  if (!type->isPackage) {
    fileTypeVector.push_back(type);
  }
}

void HRCParserImpl::parsePrototypeBlock(const xercesc::DOMElement* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagLocation)) {
        addPrototypeLocation(subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagFilename) ||
          xercesc::XMLString::equals(subelem->getNodeName(), hrcTagFirstline)) {
        addPrototypeDetectParam(subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagParametrs)) {
        addPrototypeParameters(subelem);
        continue;
      }
    }
  }
}

void HRCParserImpl::addPrototypeLocation(const xercesc::DOMElement* elem)
{
  const XMLCh* locationLink = elem->getAttribute(hrcLocationAttrLink);
  if (*locationLink == '\0') {
    logger->error("Bad 'location' link attribute in prototype '{0}'", parseProtoType->name->getChars());
    return;
  }
  parseProtoType->inputSource = XmlInputSource::newInstance(locationLink, current_input_source);
}

void HRCParserImpl::addPrototypeDetectParam(const xercesc::DOMElement* elem)
{
  if (elem->getFirstChild() == nullptr || elem->getFirstChild()->getNodeType() != xercesc::DOMNode::TEXT_NODE) {
    logger->warn("Bad '{0}' element in prototype '{1}'", XStr(elem->getNodeName()).get_char(), parseProtoType->name->getChars());
    return;
  }
  const XMLCh* match = ((xercesc::DOMText*)elem->getFirstChild())->getData();
  CString dmatch = CString(match);
  auto* matchRE = new CRegExp(&dmatch);
  matchRE->setPositionMoves(true);
  if (!matchRE->isOk()) {
    logger->warn("Fault compiling chooser RE '{0}' in prototype '{1}'", dmatch.getChars(), parseProtoType->name->getChars());
    delete matchRE;
    return;
  }
  FileTypeChooser::ChooserType ctype = xercesc::XMLString::equals(elem->getNodeName() , hrcTagFilename) ? FileTypeChooser::ChooserType::CT_FILENAME : FileTypeChooser::ChooserType::CT_FIRSTLINE;
  double prior = ctype ? 1 : 2;
  CString weight = CString(elem->getAttribute(hrcFilenameAttrWeight));
  UnicodeTools::getNumber(&weight, &prior);
  auto* ftc = new FileTypeChooser(ctype, prior, matchRE);
  parseProtoType->chooserVector.push_back(ftc);
}

void HRCParserImpl::addPrototypeParameters(const xercesc::DOMElement* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagParam)) {
        const XMLCh* name = subelem->getAttribute(hrcParamAttrName);
        const XMLCh* value = subelem->getAttribute(hrcParamAttrValue);
        const XMLCh* descr = subelem->getAttribute(hrcParamAttrDescription);
        if (*name == '\0' || *value == '\0') {
          logger->warn("Bad parameter in prototype '{0}'", parseProtoType->getName()->getChars());
          continue;
        }
        CString d_name = CString(name);
        TypeParameter* tp = parseProtoType->addParam(&d_name);
        tp->default_value.reset(new SString(CString(value)));
        if (*descr != '\0') {
          tp->description.reset(new SString(CString(descr)));
        }
      }
      continue;
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      addPrototypeParameters(static_cast<xercesc::DOMElement*>(node));
    }
  }
}

void HRCParserImpl::addType(const xercesc::DOMElement* elem)
{
  const XMLCh* typeName = elem->getAttribute(hrcTypeAttrName);

  if (*typeName == '\0') {
    logger->error("Unnamed type found");
    return;
  }
  CString d_name = CString(typeName);
  auto type_ref = fileTypeHash.find(&d_name);
  if (type_ref == fileTypeHash.end()) {
    logger->error("type '%s' without prototype", d_name.getChars());
    return;
  }
  FileTypeImpl* type = type_ref->second;
  if (type->type_loaded) {
    logger->warn("type '{0}' is already loaded", XStr(typeName).get_char());
    return;
  }
  type->type_loaded = true;

  FileTypeImpl* o_parseType = parseType;
  parseType = type;

  parseTypeBlock(elem);

  String* baseSchemeName = qualifyOwnName(type->getName());
  if (baseSchemeName != nullptr) {
    auto sh = schemeHash.find(baseSchemeName);
    type->baseScheme = sh == schemeHash.end() ? nullptr : sh->second;
  }
  delete baseSchemeName;
  if (type->baseScheme == nullptr && !type->isPackage) {
    logger->warn("type '{0}' has no default scheme", XStr(typeName).get_char());
  }
  type->loadDone = true;
  parseType = o_parseType;
}

void HRCParserImpl::parseTypeBlock(const xercesc::DOMElement* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagRegion)) {
        addTypeRegion(subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagEntity)) {
        addTypeEntity(subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagImport)) {
        addTypeImport(subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagScheme)) {
        addScheme(subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagAnnotation)) {
        // not read anotation
        continue;
      }
    }
    // случай entity ссылки на другой файл
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      parseTypeBlock(static_cast<xercesc::DOMElement*>(node));
    }
  }
}

void HRCParserImpl::addTypeRegion(const xercesc::DOMElement* elem)
{
  const XMLCh* regionName = elem->getAttribute(hrcRegionAttrName);
  const XMLCh* regionParent = elem->getAttribute(hrcRegionAttrParent);
  const XMLCh* regionDescr = elem->getAttribute(hrcRegionAttrDescription);
  if (*regionName == '\0') {
    logger->error("No 'name' attribute in <region> element");
    return;
  }
  CString d_region = CString(regionName);
  String* qname1 = qualifyOwnName(&d_region);
  if (qname1 == nullptr) {
    return;
  }
  CString d_regionparent = CString(regionParent);
  String* qname2 = qualifyForeignName(*regionParent != '\0' ? &d_regionparent : nullptr, QNT_DEFINE, true);
  if (regionNamesHash.find(qname1) != regionNamesHash.end()) {
    logger->warn("Duplicate region '{0}' definition in type '{1}'", qname1->getChars(), parseType->getName()->getChars());
    delete qname1;
    delete qname2;
    return;
  }

  CString regiondescr = CString(regionDescr);
  const Region* region = new Region(qname1, &regiondescr, getRegion(qname2), (int)regionNamesVector.size());
  regionNamesVector.push_back(region);
  std::pair<SString, const Region*> pp(qname1, region);
  regionNamesHash.emplace(pp);

  delete qname1;
  delete qname2;
}

void HRCParserImpl::addTypeEntity(const xercesc::DOMElement* elem)
{
  const XMLCh* entityName  = elem->getAttribute(hrcEntityAttrName);
  const XMLCh* entityValue = elem->getAttribute(hrcEntityAttrValue);
  if (*entityName == '\0' || elem->getAttributeNode(hrcEntityAttrValue) == nullptr) {
    logger->error("Bad entity attributes");
    return;
  }
  CString dentityName = CString(entityName);
  CString dentityValue = CString(entityValue);
  String* qname1 = qualifyOwnName(&dentityName);
  String* qname2 = useEntities(&dentityValue);
  if (qname1 != nullptr && qname2 != nullptr) {
    std::pair<SString, String*> pp(qname1, qname2);
    schemeEntitiesHash.emplace(pp);
    delete qname1;
  }
}

void HRCParserImpl::addTypeImport(const xercesc::DOMElement* elem)
{
  const XMLCh* typeParam = elem->getAttribute(hrcImportAttrType);
  CString typeparam = CString(typeParam);
  if (*typeParam == '\0' || fileTypeHash.find(&typeparam) == fileTypeHash.end()) {
    logger->error("Import with bad '{0}' attribute in type '{1}'", typeparam.getChars(), parseType->name->getChars());
    return;
  }
  parseType->importVector.emplace_back(new SString(CString(typeParam)));
}

void HRCParserImpl::addScheme(const xercesc::DOMElement* elem)
{
  const XMLCh* schemeName = elem->getAttribute(hrcSchemeAttrName);
  CString dschemeName = CString(schemeName);
  String* qSchemeName = qualifyOwnName(*schemeName != '\0' ? &dschemeName : nullptr);
  if (qSchemeName == nullptr) {
    logger->error("bad scheme name in type '{0}'", parseType->getName()->getChars());
    return;
  }
  if (schemeHash.find(qSchemeName) != schemeHash.end() ||
      disabledSchemes.find(qSchemeName) != disabledSchemes.end()) {
    logger->error("duplicate scheme name '{0}'", qSchemeName->getChars());
    delete qSchemeName;
    return;
  }

  auto* scheme = new SchemeImpl(qSchemeName);
  delete qSchemeName;
  scheme->fileType = parseType;

  std::pair<SString, SchemeImpl*> pp(scheme->getName(), scheme);
  schemeHash.emplace(pp);
  const XMLCh* condIf = elem->getAttribute(hrcSchemeAttrIf);
  const XMLCh* condUnless = elem->getAttribute(hrcSchemeAttrUnless);
  const String* p1 = parseType->getParamValue(CString(condIf));
  const String* p2 = parseType->getParamValue(CString(condUnless));
  if ((*condIf != '\0' && !CString("true").equals(p1)) ||
      (*condUnless != '\0' && CString("true").equals(p2))) {
    //disabledSchemes.put(scheme->schemeName, 1);
    return;
  }
  parseSchemeBlock(scheme, elem);
}

void HRCParserImpl::parseSchemeBlock(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
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
        addSchemeKeywords(scheme, subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagAnnotation)) {
        // not read anotation
        continue;
      }
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      parseSchemeBlock(scheme, static_cast<xercesc::DOMElement*>(node));
    }
  }
}

void HRCParserImpl::addSchemeInherit(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  const XMLCh* nqSchemeName = elem->getAttribute(hrcInheritAttrScheme);
  if (*nqSchemeName == '\0') {
    logger->error("empty scheme name in inheritance operator in scheme '{0}'", scheme->schemeName->getChars());
    return;
  }
  auto* scheme_node = new SchemeNode();
  scheme_node->type = SchemeNode::SNT_INHERIT;
  scheme_node->schemeName.reset(new SString(CString(nqSchemeName)));
  CString dnqSchemeName = CString(nqSchemeName);
  String* schemeName = qualifyForeignName(&dnqSchemeName, QNT_SCHEME, false);
  if (schemeName == nullptr) {
    //        if (errorHandler != null) errorHandler->warning(StringBuffer("forward inheritance of '")+nqSchemeName+"'. possible inherit loop with '"+scheme->schemeName+"'");
    //        delete next;
    //        continue;
  } else {
    scheme_node->scheme = schemeHash.find(schemeName)->second;
  }
  if (schemeName != nullptr) {
    scheme_node->schemeName.reset(schemeName);
  }

  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName() , hrcTagVirtual)) {
        const XMLCh* x_schemeName = subelem->getAttribute(hrcVirtualAttrScheme);
        const XMLCh* x_substName = subelem->getAttribute(hrcVirtualAttrSubstScheme);
        if (*x_schemeName == '\0' || *x_substName == '\0') {
          logger->error("bad virtualize attributes in scheme '{0}'", scheme->schemeName->getChars());
          continue;
        }
        CString d_schemeName = CString(x_schemeName);
        CString d_substName = CString(x_substName);
        scheme_node->virtualEntryVector.push_back(new VirtualEntry(&d_schemeName, &d_substName));
      }
    }
  }
  scheme->nodes.push_back(scheme_node);
}

void HRCParserImpl::addSchemeRegexp(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  const XMLCh* matchParam = elem->getAttribute(hrcRegexpAttrMatch);
  if (*matchParam == '\0') {
    for (xercesc::DOMNode* child = elem->getFirstChild(); child != nullptr; child = child->getNextSibling()) {
      if (child->getNodeType() == xercesc::DOMNode::CDATA_SECTION_NODE) {
        matchParam = ((xercesc::DOMCDATASection*)child)->getData();
        break;
      }
      if (child->getNodeType() == xercesc::DOMNode::TEXT_NODE) {
        const XMLCh* matchParam1;
        matchParam1 = ((xercesc::DOMText*)child)->getData();
        xercesc::XMLString::trim((XMLCh*)matchParam1);
        if (*matchParam1 != '\0') {
          matchParam = matchParam1;
          break;
        }
      }
    }
  }
  if (matchParam == nullptr) {
    logger->error("no 'match' in regexp in scheme '{0}'", scheme->schemeName->getChars());
    return;
  }
  CString dmatchParam = CString(matchParam);
  String* entMatchParam = useEntities(&dmatchParam);
  auto* scheme_node = new SchemeNode();
  CString dhrcRegexpAttrPriority = CString(elem->getAttribute(hrcRegexpAttrPriority));
  scheme_node->lowPriority = CString("low").equals(&dhrcRegexpAttrPriority);
  scheme_node->type = SchemeNode::SNT_RE;
  scheme_node->start.reset(new CRegExp(entMatchParam));
  if (!scheme_node->start || !scheme_node->start->isOk())
    logger->error("fault compiling regexp '{0}' in scheme '{1}'", entMatchParam->getChars(), scheme->schemeName->getChars());
  delete entMatchParam;
  scheme_node->start->setPositionMoves(false);
  scheme_node->end = nullptr;

  loadRegions(scheme_node, elem, true);
  if (scheme_node->region) {
    scheme_node->regions[0] = scheme_node->region;
  }

  scheme->nodes.push_back(scheme_node);
}

void HRCParserImpl::addSchemeBlock(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  const XMLCh* sParam = elem->getAttribute(hrcBlockAttrStart);
  const XMLCh* eParam = elem->getAttribute(hrcBlockAttrEnd);

  xercesc::DOMElement* eStart = nullptr, *eEnd = nullptr;

  for (xercesc::DOMNode* blkn = elem->getFirstChild(); blkn && !(*eParam != '\0' && *sParam != '\0'); blkn = blkn->getNextSibling()) {
    xercesc::DOMElement* blkel;
    if (blkn->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      blkel = static_cast<xercesc::DOMElement*>(blkn);
    } else {
      continue;
    }

    const XMLCh* p = nullptr;
    if (blkel->getAttributeNode(hrcBlockAttrMatch)) {
      p = blkel->getAttribute(hrcBlockAttrMatch);
    } else {
      for (xercesc::DOMNode* child = blkel->getFirstChild(); child != nullptr; child = child->getNextSibling()) {
        if (child->getNodeType() == xercesc::DOMNode::CDATA_SECTION_NODE) {
          p = ((xercesc::DOMCDATASection*)child)->getData();
          break;
        }
        if (child->getNodeType() == xercesc::DOMNode::TEXT_NODE) {
          const XMLCh* p1;
          p1 = ((xercesc::DOMText*)child)->getData();
          xercesc::XMLString::trim((XMLCh*)p1);
          if (*p1 != '\0') {
            p = p1;
            break;
          }
        }
      }
    }

    if (xercesc::XMLString::equals(blkel->getNodeName() , hrcBlockAttrStart)) {
      sParam = p;
      eStart = blkel;
    }
    if (xercesc::XMLString::equals(blkel->getNodeName() , hrcBlockAttrEnd)) {
      eParam = p;
      eEnd = blkel;
    }
  }

  String* startParam;
  String* endParam;
  CString dsParam = CString(sParam);
  if (!(startParam = useEntities(&dsParam))) {
    logger->error("'start' block attribute not found in scheme '{0}'", scheme->schemeName->getChars());
    delete startParam;
    return;
  }
  CString deParam = CString(eParam);
  if (!(endParam = useEntities(&deParam))) {
    logger->error("'end' block attribute not found in scheme '{0}'", scheme->schemeName->getChars());
    delete startParam;
    delete endParam;
    return;
  }
  const XMLCh* schemeName = elem->getAttribute(hrcBlockAttrScheme);
  if (*schemeName == '\0') {
    logger->error("block with bad scheme attribute in scheme '{0}'", scheme->schemeName->getChars());
    delete startParam;
    delete endParam;
    return;
  }
  auto* scheme_node = new SchemeNode();
  scheme_node->schemeName.reset(new SString(CString(schemeName)));
  CString attr_pr = CString(elem->getAttribute(hrcBlockAttrPriority));
  CString attr_cpr = CString(elem->getAttribute(hrcBlockAttrContentPriority));
  CString attr_ireg = CString(elem->getAttribute(hrcBlockAttrInnerRegion));
  scheme_node->lowPriority = CString("low").equals(&attr_pr);
  scheme_node->lowContentPriority = CString("low").equals(&attr_cpr);
  scheme_node->innerRegion = CString("yes").equals(&attr_ireg);
  scheme_node->type = SchemeNode::SNT_SCHEME;
  scheme_node->start.reset(new CRegExp(startParam));
  scheme_node->start->setPositionMoves(false);
  if (!scheme_node->start->isOk()) {
    logger->error("fault compiling regexp '{0}' in scheme '{1}'", startParam->getChars(), scheme->schemeName->getChars());
  }
  scheme_node->end .reset(new CRegExp());
  scheme_node->end->setPositionMoves(true);
  scheme_node->end->setBackRE(scheme_node->start.get());
  scheme_node->end->setRE(endParam);
  if (!scheme_node->end->isOk()) {
    logger->error("fault compiling regexp '{0}' in scheme '{1}'", endParam->getChars(), scheme->schemeName->getChars());
  }
  delete startParam;
  delete endParam;

  // !! EE
  loadBlockRegions(scheme_node, elem);
  loadRegions(scheme_node, eStart, true);
  loadRegions(scheme_node, eEnd, false);
  scheme->nodes.push_back(scheme_node);
}

void HRCParserImpl::addSchemeKeywords(SchemeImpl* scheme, const xercesc::DOMElement* elem)
{
  const Region* brgn = getNCRegion(elem, CString("region"));
  if (brgn == nullptr) {
    return;
  }

  auto* scheme_node = new SchemeNode();
  CString dhrcKeywordsAttrIgnorecase = CString(elem->getAttribute(hrcKeywordsAttrIgnorecase));
  CString dhrcKeywordsAttrPriority = CString(elem->getAttribute(hrcKeywordsAttrPriority));
  bool isCase = !CString("yes").equals(&dhrcKeywordsAttrIgnorecase);
  scheme_node->lowPriority = !CString("normal").equals(&dhrcKeywordsAttrPriority);

  const XMLCh* worddiv = elem->getAttribute(hrcKeywordsAttrWorddiv);

  scheme_node->worddiv = nullptr;
  if (*worddiv != '\0') {
    CString dworddiv = CString(worddiv);
    String* entWordDiv = useEntities(&dworddiv);
    scheme_node->worddiv.reset(CharacterClass::createCharClass(*entWordDiv, 0, nullptr, false));
    if (scheme_node->worddiv == nullptr) {
      logger->error("fault compiling worddiv regexp '{0}' in scheme '{1}'", entWordDiv->getChars(), scheme->schemeName->getChars());
    }
    delete entWordDiv;
  }

  scheme_node->kwList.reset(new KeywordList);
  scheme_node->kwList->num = getSchemeKeywordsCount(elem);

  scheme_node->kwList->kwList = new KeywordInfo[scheme_node->kwList->num];
  memset(scheme_node->kwList->kwList , 0, sizeof(KeywordInfo)*scheme_node->kwList->num);
  scheme_node->kwList->num = 0;
  scheme_node->kwList->matchCase = isCase;
  scheme_node->type = SchemeNode::SNT_KEYWORDS;

  for (xercesc::DOMNode* keywrd = elem->getFirstChild(); keywrd; keywrd = keywrd->getNextSibling()) {
    if (keywrd->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      addKeyword(scheme_node, brgn, static_cast<xercesc::DOMElement*>(keywrd));
      continue;
    }
    if (keywrd->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      for (xercesc::DOMNode* keywrd2 = keywrd->getFirstChild(); keywrd2; keywrd2 = keywrd2->getNextSibling()) {
        if (keywrd2->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
          addKeyword(scheme_node, brgn, static_cast<xercesc::DOMElement*>(keywrd2));
        }
      }
    }
  }
  scheme_node->kwList->sortList();
  scheme_node->kwList->substrIndex();
  scheme->nodes.push_back(scheme_node);
}

void HRCParserImpl::addKeyword(SchemeNode* scheme_node, const Region* brgn, const xercesc::DOMElement* elem)
{
  int type = 0;
  if (xercesc::XMLString::equals(elem->getNodeName() , hrcTagWord)) {
    type = 1;
  }
  if (xercesc::XMLString::equals(elem->getNodeName() , hrcTagSymb)) {
    type = 2;
  }
  if (!type) {
    return;
  }
  const XMLCh* param = elem->getAttribute(hrcWordAttrName);
  if (*param == '\0') {
    return;
  }

  const Region* rgn = brgn;
  const XMLCh* reg = elem->getAttribute(hrcWordAttrRegion);
  if (*reg != '\0') {
    rgn = getNCRegion(elem, CString(hrcWordAttrRegion));
  }

  int pos = scheme_node->kwList->num;
  scheme_node->kwList->kwList[pos].keyword.reset(new SString(CString(param)));
  scheme_node->kwList->kwList[pos].region = rgn;
  scheme_node->kwList->kwList[pos].isSymbol = (type == 2);
  scheme_node->kwList->kwList[pos].ssShorter = -1;
  scheme_node->kwList->firstChar->addChar(param[0]);
  if (!scheme_node->kwList->matchCase) {
    scheme_node->kwList->firstChar->addChar(Character::toLowerCase(param[0]));
    scheme_node->kwList->firstChar->addChar(Character::toUpperCase(param[0]));
    scheme_node->kwList->firstChar->addChar(Character::toTitleCase(param[0]));
  }
  scheme_node->kwList->num++;
  if (scheme_node->kwList->minKeywordLength > scheme_node->kwList->kwList[pos].keyword->length()) {
    scheme_node->kwList->minKeywordLength = scheme_node->kwList->kwList[pos].keyword->length();
  }
}

int HRCParserImpl::getSchemeKeywordsCount(const xercesc::DOMElement* elem)
{
  int result = 0;
  for (xercesc::DOMNode* keywrd_count = elem->getFirstChild(); keywrd_count; keywrd_count = keywrd_count->getNextSibling()) {
    if (keywrd_count->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      if (xercesc::XMLString::equals(keywrd_count->getNodeName() , hrcTagWord) ||
          xercesc::XMLString::equals(keywrd_count->getNodeName() , hrcTagSymb)) {
        result++;
      }
      continue;
    }
    if (keywrd_count->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      result += getSchemeKeywordsCount(static_cast<xercesc::DOMElement*>(keywrd_count));
    }
  }
  return result;
}

void HRCParserImpl::loadRegions(SchemeNode* node, const xercesc::DOMElement* el, bool st)
{
  static char rg_tmpl[8] = "region\0";

  if (el) {
    if (node->region == nullptr) {
      node->region = getNCRegion(el, CString("region"));
    }

    for (int i = 0; i < REGIONS_NUM; i++) {
      rg_tmpl[6] = (i < 0xA ? i : i + 7 + 32) + '0';

      if (st) {
        node->regions[i] = getNCRegion(el, CString(rg_tmpl));
      } else {
        node->regione[i] = getNCRegion(el, CString(rg_tmpl));
      }
    }
  }

  for (int i = 0; i < NAMED_REGIONS_NUM; i++) {
    if (st) {
      node->regionsn[i] = getNCRegion(node->start->getBracketName(i), false);
    } else {
      node->regionen[i] = getNCRegion(node->end->getBracketName(i), false);
    }
  }
}

void HRCParserImpl::loadBlockRegions(SchemeNode* node, const xercesc::DOMElement* el)
{
  int i;
  static char rg_tmpl[9] = "region\0\0";

  node->region = getNCRegion(el, CString("region"));
  for (i = 0; i < REGIONS_NUM; i++) {
    rg_tmpl[6] = '0';
    rg_tmpl[7] = (i < 0xA ? i : i + 7 + 32) + '0';
    node->regions[i] = getNCRegion(el, CString(rg_tmpl));
    rg_tmpl[6] = '1';
    node->regione[i] = getNCRegion(el, CString(rg_tmpl));
  }
}

void HRCParserImpl::updateLinks()
{
  while (structureChanged) {
    structureChanged = false;
    for (auto scheme_it = schemeHash.begin(); scheme_it != schemeHash.end(); ++scheme_it) {
      SchemeImpl* scheme = scheme_it->second;
      if (!scheme->fileType->loadDone) {
        continue;
      }
      FileTypeImpl* old_parseType = parseType;
      parseType = scheme->fileType;
      for (size_t sni = 0; sni < scheme->nodes.size(); sni++) {
        SchemeNode* snode = scheme->nodes.at(sni);
        if (snode->schemeName != nullptr && (snode->type == SchemeNode::SNT_SCHEME || snode->type == SchemeNode::SNT_INHERIT) && snode->scheme == nullptr) {
          String* schemeName = qualifyForeignName(snode->schemeName.get(), QNT_SCHEME, true);
          if (schemeName != nullptr) {
            snode->scheme = schemeHash.find(schemeName)->second;
          } else {
            logger->error("cannot resolve scheme name '{0}' in scheme '{1}'", snode->schemeName->getChars(), scheme->schemeName->getChars());
          }
          delete schemeName;
          snode->schemeName.reset();
        }
        if (snode->type == SchemeNode::SNT_INHERIT) {
          for (auto vt : snode->virtualEntryVector) {
            if (vt->virtScheme == nullptr && vt->virtSchemeName != nullptr) {
              String* vsn = qualifyForeignName(vt->virtSchemeName.get(), QNT_SCHEME, true);
              if (vsn) {
                vt->virtScheme = schemeHash.find(vsn)->second;
              } else {
                logger->error("cannot virtualize scheme '{0}' in scheme '{1}'", vt->virtSchemeName->getChars(), scheme->schemeName->getChars());
              }
              delete vsn;
              vt->virtSchemeName.reset();
            }
            if (vt->substScheme == nullptr && vt->substSchemeName != nullptr) {
              String* vsn = qualifyForeignName(vt->substSchemeName.get(), QNT_SCHEME, true);
              if (vsn) {
                vt->substScheme = schemeHash.find(vsn)->second;
              } else {
                logger->error("cannot virtualize using subst-scheme scheme '{0}' in scheme '{1}'", vt->substSchemeName->getChars(), scheme->schemeName->getChars());
              }
              delete vsn;
              vt->substSchemeName.reset();
            }
          }
        }
      }
      parseType = old_parseType;
      if (structureChanged) {
        break;
      }
    }
  }
}

String* HRCParserImpl::qualifyOwnName(const String* name)
{
  if (name == nullptr) {
    return nullptr;
  }
  size_t colon = name->indexOf(':');
  if (colon != String::npos) {
    if (parseType && CString(name, 0, colon) != *parseType->name) {
      logger->error("type name qualifer in '{0}' doesn't match type '{1}'", name->getChars(), parseType->name->getChars());
      return nullptr;
    } else {
      return new SString(name);
    }
  } else {
    if (parseType == nullptr) {
      return nullptr;
    }
    auto* sbuf = new SString(parseType->getName());
    sbuf->append(CString(":")).append(name);
    return sbuf;
  }
}

bool HRCParserImpl::checkNameExist(const String* name, FileTypeImpl* parseType, QualifyNameType qntype, bool logErrors)
{
  if (qntype == QNT_DEFINE && regionNamesHash.find(name) == regionNamesHash.end()) {
    if (logErrors)
      logger->error("region '{0}', referenced in type '{1}', is not defined", name->getChars(), parseType->name->getChars());
    return false;
  } else if (qntype == QNT_ENTITY && schemeEntitiesHash.find(name) == schemeEntitiesHash.end()) {
    if (logErrors)
      logger->error("entity '{0}', referenced in type '{1}', is not defined", name->getChars(), parseType->name->getChars());
    return false;
  } else if (qntype == QNT_SCHEME && schemeHash.find(name) == schemeHash.end()) {
    if (logErrors)
      logger->error("scheme '{0}', referenced in type '{1}', is not defined", name->getChars(), parseType->name->getChars());
    return false;
  }
  return true;
}

String* HRCParserImpl::qualifyForeignName(const String* name, QualifyNameType qntype, bool logErrors)
{
  if (name == nullptr) {
    return nullptr;
  }
  size_t colon = name->indexOf(':');
  if (colon != String::npos) { // qualified name
    CString prefix(name, 0, colon);
    auto ft = fileTypeHash.find(&prefix);
    FileTypeImpl* prefType = nullptr;
    if (ft != fileTypeHash.end()) {
      prefType = ft->second;
    }

    if (prefType == nullptr) {
      if (logErrors) {
        logger->error("type name qualifer in '{0}' doesn't match any type", name->getChars());
      }
      return nullptr;
    } else if (!prefType->type_loaded) {
      loadFileType(prefType);
    }
    if (prefType == parseType || prefType->type_loaded) {
      return checkNameExist(name, prefType, qntype, logErrors) ? (new SString(name)) : nullptr;
    }
  } else { // unqualified name
    for (int idx = -1; parseType != nullptr && idx < static_cast<int>(parseType->importVector.size()); idx++) {
      const String* tname = parseType->getName();
      if (idx > -1) {
        tname = parseType->importVector.at(idx).get();
      }
      FileTypeImpl* importer = fileTypeHash.find(tname)->second;
      if (!importer->type_loaded) {
        loadFileType(importer);
      }

      auto* qname = new SString(tname);
      qname->append(CString(":")).append(name);
      if (checkNameExist(qname, importer, qntype, false)) {
        return qname;
      }
      delete qname;
    }
    if (logErrors) {
      logger->error("unqualified name '{0}' doesn't belong to any imported type [{1}]", name->getChars(), XStr(current_input_source->getInputSource()->getSystemId()).get_char());
    }
  }
  return nullptr;
}

String* HRCParserImpl::useEntities(const String* name)
{
  int copypos = 0;
  size_t epos = 0;

  if (!name) {
    return nullptr;
  }
  auto* newname = new SString();

  while (true) {
    epos = name->indexOf('%', epos);
    if (epos == String::npos) {
      epos = name->length();
      break;
    }
    if (epos && (*name)[epos - 1] == '\\') {
      epos++;
      continue;
    }
    size_t elpos = name->indexOf(';', epos);
    if (elpos == String::npos) {
      epos = name->length();
      break;
    }
    CString enname(name, epos + 1, elpos - epos - 1);

    String* qEnName = qualifyForeignName(&enname, QNT_ENTITY, true);
    const String* enval = nullptr;
    if (qEnName != nullptr) {
      enval = schemeEntitiesHash.find(qEnName)->second;
      delete qEnName;
    }
    if (enval == nullptr) {
      epos++;
      continue;
    }
    newname->append(CString(name, copypos, epos - copypos));
    newname->append(enval);
    epos = elpos + 1;
    copypos = epos;
  }
  if (epos > copypos) {
    newname->append(CString(name, copypos, epos - copypos));
  }
  return newname;
}

const Region* HRCParserImpl::getNCRegion(const String* name, bool logErrors)
{
  if (name == nullptr) {
    return nullptr;
  }
  const Region* reg;
  String* qname = qualifyForeignName(name, QNT_DEFINE, logErrors);
  if (qname == nullptr) {
    return nullptr;
  }
  auto reg_ = regionNamesHash.find(qname);
  if (reg_ != regionNamesHash.end()) {
    reg = reg_->second;
  } else {
    reg = nullptr;
  }

  delete qname;
  /** Check for 'default' region request.
      Regions with this name are always transparent
  */
  if (reg != nullptr) {
    const String* s_name = reg->getName();
    size_t idx = s_name->indexOf(CString(":default"));
    if (idx != String::npos && idx + 8 == s_name->length()) {
      return nullptr;
    }
  }
  return reg;
}

const Region* HRCParserImpl::getNCRegion(const xercesc::DOMElement* el, const String &tag)
{
  const XMLCh* par = el->getAttribute(tag.getW2Chars());
  if (*par == '\0') {
    return nullptr;
  }
  CString dpar = CString(par);
  return getNCRegion(&dpar, true);
}



