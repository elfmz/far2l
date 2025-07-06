#include "colorer/parsers/HrcLibraryImpl.h"
#include <memory>
#include "colorer/base/XmlTagDefs.h"
#include "colorer/parsers/FileTypeImpl.h"
#include "colorer/xml/XmlReader.h"

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

  for (const auto* it : regionNamesVector) {
    delete it;
  }

  for (const auto& it : schemeEntitiesHash) {
    delete it.second;
  }
}

void HrcLibrary::Impl::loadSource(XmlInputSource* input_source, const LoadType load_type)
{
  if (!input_source) {
    throw HrcLibraryException("Can't open stream - 'null' is bad stream.");
  }

  // Сохраняем текущий контекст парсинга файла, т.к. у нас рекурсивное использование
  XmlInputSource* temp_is = current_input_source;
  LoadType temp_lt = current_load_type;
  current_input_source = input_source;
  current_load_type = load_type;

  try {
    parseHRC(*input_source);
  } catch (Exception&) {
    // восстанавливаем контекст
    current_input_source = temp_is;
    current_load_type = temp_lt;
    throw;
  }

  // восстанавливаем контекст
  current_input_source = temp_is;
  current_load_type = temp_lt;
}

void HrcLibrary::Impl::unloadFileType(const FileType* filetype)
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
  // проверяем статус загрузки указанного типа
  if (thisType == nullptr || filetype->pimpl->type_loading || thisType->pimpl->input_source_loading ||
      thisType->pimpl->load_broken)
  {
    // если он уже в процессе загрузки, то выходим
    // ошибку не возвращаем, почему ? TODO проверить
    return;
  }

  thisType->pimpl->input_source_loading = true;

  const auto& input_source = thisType->pimpl->inputSource;
  try {
    // загружаем только сам тип, все остальное уже у нас есть
    loadSource(input_source.get(), LoadType::TYPE);
  } catch (InputSourceException& e) {
    COLORER_LOG_ERROR("Can't open source stream: %", e.what());
    thisType->pimpl->load_broken = true;
  } catch (HrcLibraryException& e) {
    COLORER_LOG_ERROR("% [%]", e.what(), thisType->pimpl->inputSource ? input_source->getPath() : "");
    thisType->pimpl->load_broken = true;
  } catch (Exception& e) {
    COLORER_LOG_ERROR("% [%]", e.what(), thisType->pimpl->inputSource ? input_source->getPath() : "");
    thisType->pimpl->load_broken = true;
  } catch (...) {
    COLORER_LOG_ERROR("Unknown exception while loading %", input_source->getPath());
    thisType->pimpl->load_broken = true;
  }

  thisType->pimpl->input_source_loading = false;
}

void HrcLibrary::Impl::loadHrcSettings(const XmlInputSource& is)
{
  XmlReader xml_parser(is);
  if (!xml_parser.parse()) {
    throw HrcLibraryException("Error reading " + is.getPath());
  }
  std::list<XMLNode> nodes;
  xml_parser.getNodes(nodes);

  if (nodes.begin()->name != u"hrc-settings") {
    throw HrcLibraryException("main '<hrc-settings>' block not found");
  }
  for (const auto& node : nodes.begin()->children) {
    if (node.name == hrcTagPrototype) {
      updatePrototype(node);
    }
  }
}

FileType* HrcLibrary::Impl::chooseFileType(const UnicodeString* fileName, const UnicodeString* firstLine, int typeNo)
{
  FileType* best = nullptr;
  double max_prior = 0;
  const double DELTA = 1e-6;
  for (auto* ret : fileTypeVector) {
    double const prior = ret->pimpl->getPriority(fileName, firstLine);

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
  const auto filetype = fileTypeHash.find(*name);
  if (filetype != fileTypeHash.end()) {
    return filetype->second;
  }
  return nullptr;
}

FileType* HrcLibrary::Impl::enumerateFileTypes(const unsigned int index) const
{
  if (index < fileTypeVector.size()) {
    return fileTypeVector[index];
  }
  return nullptr;
}

size_t HrcLibrary::Impl::getFileTypesCount() const
{
  return fileTypeVector.size();
}

size_t HrcLibrary::Impl::getRegionCount() const
{
  return regionNamesVector.size();
}

const Region* HrcLibrary::Impl::getRegion(const unsigned int id) const
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
  return getNCRegion(name, false);
}

// protected methods

void HrcLibrary::Impl::parseHRC(const XmlInputSource& is)
{
  COLORER_LOG_DEBUG("begin parse '%'", is.getPath());

  XmlReader xml(is);
  if (!xml.parse()) {
    throw HrcLibraryException("Error reading hrc file '" + is.getPath() + "'");
  }
  std::list<XMLNode> nodes;
  xml.getNodes(nodes);

  if (nodes.begin()->name != hrcTagHrc) {
    throw HrcLibraryException("Incorrect hrc-file structure. Main '<hrc>' block not found. Current file " +
                              is.getPath());
  }

  bool globalUpdateStarted = false;
  if (!updateStarted) {
    globalUpdateStarted = true;
    updateStarted = true;
  }

  parseHrcBlock(*nodes.begin());

  structureChanged = true;
  if (globalUpdateStarted) {
    updateLinks();
    updateStarted = false;
  }

  COLORER_LOG_DEBUG("end parse '%'", is.getPath());
}

void HrcLibrary::Impl::parseHrcBlock(const XMLNode& elem)
{
  for (const auto& node : elem.children) {
    if (node.name == hrcTagPrototype) {
      addPrototype(node);
    }
    else if (node.name == hrcTagPackage) {
      addPrototype(node);
    }
    else if (node.name == hrcTagType) {
      addType(node);
    }
    else if (node.name == hrcTagAnnotation) {
      // do not read annotation
    }
    else {
      COLORER_LOG_WARN("Unused element '%'. Current file %.", node.name, current_input_source->getPath());
    }
  }
}

void HrcLibrary::Impl::addPrototype(const XMLNode& elem)
{
  const auto& global_package = elem.getAttrValue(hrcPackageAttrGlobal);
  const bool is_global_package = global_package.isEmpty() || global_package.compare(value_yes) == 0;

  if (elem.name == hrcTagPrototype && current_load_type == LoadType::TYPE) {
    // прототип не загружаем на стадии загрузки типов
    return;
  }
  if (elem.name == hrcTagPackage &&
      (
          // глобальный пакет не загружаем на стадии загрузки типов
          (is_global_package && current_load_type == LoadType::TYPE) ||
          // внутренний пакет не загружаем на стадии загрузки прототипов
          (!is_global_package && current_load_type == LoadType::PROTOTYPE)))
  {
    return;
  }

  auto typeName = elem.getAttrValue(hrcPrototypeAttrName);

  if (typeName.isEmpty()) {
    COLORER_LOG_ERROR("Found unnamed prototype/package. Skipped.");
    return;
  }

  const auto ft = fileTypeHash.find(typeName);
  if (ft != fileTypeHash.end()) {
    unloadFileType(ft->second);
    COLORER_LOG_WARN("Duplicate prototype '%'. First version unloaded, current is loading.", typeName);
  }

  const auto& typeGroup = elem.getAttrValue(hrcPrototypeAttrGroup);
  const auto& typeDescription = elem.getAttrValue(hrcPrototypeAttrDescription);
  auto* type = new FileType(typeName, typeGroup.isEmpty() ? typeName : typeGroup,
                            typeDescription.isEmpty() ? typeName : typeDescription);
  auto& ptype = type->pimpl;

  if (elem.name == hrcTagPackage) {
    ptype->isPackage = true;
  }

  parsePrototypeBlock(elem, type);

  fileTypeHash.try_emplace(typeName, type);
  if (!ptype->isPackage) {
    fileTypeVector.push_back(type);
  }
}

void HrcLibrary::Impl::parsePrototypeBlock(const XMLNode& elem, FileType* current_parse_prototype) const
{
  for (const auto& node : elem.children) {
    if (node.name == hrcTagLocation) {
      addPrototypeLocation(node, current_parse_prototype);
    }
    else if (!current_parse_prototype->pimpl->isPackage &&
             (node.name == hrcTagFilename || node.name == hrcTagFirstline))
    {
      addPrototypeDetectParam(node, current_parse_prototype);
    }
    else if (!current_parse_prototype->pimpl->isPackage && node.name == hrcTagParametrs) {
      addPrototypeParameters(node, current_parse_prototype);
    }
    else if (node.name == hrcTagAnnotation) {
      // not read annotation
    }
    else {
      COLORER_LOG_WARN("Unused element '%' in prototype '%'. Current file %.", node.name,
                       current_parse_prototype->pimpl->name, current_input_source->getPath());
    }
  }
  // Если после загрузки блока prototype у нас не заполнена ссылка на файл с определением типа
  // значит он должен быть описан в текущем файле. Но если это не так на самом деле, то это не критичная проблема.
  // Это позволяет в одном hrc файле хранить и prototype и type.
  if (current_parse_prototype->pimpl->inputSource == nullptr) {
    current_parse_prototype->pimpl->inputSource = std::make_unique<XmlInputSource>(current_input_source->getPath());
  }
}

void HrcLibrary::Impl::addPrototypeLocation(const XMLNode& elem, FileType* current_parse_prototype) const
{
  const auto& locationLink = elem.getAttrValue(hrcLocationAttrLink);
  if (locationLink.isEmpty()) {
    COLORER_LOG_ERROR("Bad 'location' link attribute in prototype '%'", current_parse_prototype->pimpl->name);
    return;
  }
  current_parse_prototype->pimpl->inputSource = current_input_source->createRelative(locationLink);
}

void HrcLibrary::Impl::addPrototypeDetectParam(const XMLNode& elem, FileType* current_parse_prototype) const
{
  if (elem.text.isEmpty()) {
    COLORER_LOG_WARN("Bad '%' element in prototype '%'", elem.name, current_parse_prototype->pimpl->name);
    return;
  }

  auto matchRE = std::make_unique<CRegExp>(&elem.text);
  matchRE->setPositionMoves(true);
  if (!matchRE->isOk()) {
    COLORER_LOG_WARN("Fault compiling chooser RE '%' in prototype '%'", elem.text,
                     current_parse_prototype->pimpl->name);
    return;
  }
  auto ctype = elem.name == hrcTagFilename ? FileTypeChooser::ChooserType::CT_FILENAME
                                           : FileTypeChooser::ChooserType::CT_FIRSTLINE;
  double prior = ctype == FileTypeChooser::ChooserType::CT_FILENAME ? 2 : 1;
  auto weight = elem.getAttrValue(hrcFilenameAttrWeight);
  if (!weight.isEmpty()) {
    try {
      double d = std::stod(UStr::to_stdstr(&weight));
      if (d < 0) {
        COLORER_LOG_WARN(
            "Weight must be greater than 0. Current value %. Default value will be used. Current "
            "file %.",
            d, current_input_source->getPath());
      }
      else {
        prior = d;
      }
    } catch (const std::exception& e) {
      COLORER_LOG_WARN(
          "Weight '%' is not valid for the prototype '%'. Message: %. Default value will be "
          "used. Current file %.",
          weight, current_parse_prototype->getName(), e.what(), current_input_source->getPath());
    }
  }
  current_parse_prototype->pimpl->chooserVector.emplace_back(ctype, prior, matchRE.release());
}

void HrcLibrary::Impl::addPrototypeParameters(const XMLNode& elem, FileType* current_parse_prototype) const
{
  for (const auto& node : elem.children) {
    if (node.name == hrcTagParam) {
      const auto& name = node.getAttrValue(hrcParamAttrName);
      const auto& value = node.getAttrValue(hrcParamAttrValue);
      if (name.isEmpty() || value.isEmpty()) {
        COLORER_LOG_WARN("Bad parameter in prototype '%'", current_parse_prototype->getName());
        continue;
      }
      auto& tp = current_parse_prototype->pimpl->addParam(name, value);
      const auto& descr = node.getAttrValue(hrcParamAttrDescription);
      if (!descr.isEmpty()) {
        tp.description = std::make_unique<UnicodeString>(descr);
      }
    }
    else {
      COLORER_LOG_WARN("Unused element '%' in prototype '%'. Current file %.", node.name,
                       current_parse_prototype->pimpl->name, current_input_source->getPath());
    }
  }
}

void HrcLibrary::Impl::addType(const XMLNode& elem)
{
  if (current_load_type == LoadType::PROTOTYPE) {
    // тип загружаем на всех стадиях, кроме стадии загрузки прототипа
    return;
  }
  const auto& typeName = elem.getAttrValue(hrcTypeAttrName);

  if (typeName.isEmpty()) {
    COLORER_LOG_ERROR("Unnamed type found");
    return;
  }

  const auto type_ref = fileTypeHash.find(typeName);
  if (type_ref == fileTypeHash.end()) {
    COLORER_LOG_ERROR("Type '%s' without prototype", typeName);
    return;
  }
  auto* const type = type_ref->second;
  if (type->pimpl->type_loading) {
    COLORER_LOG_WARN("Type '%' is loading already. Current file %", typeName, current_input_source->getPath());
    return;
  }

  type->pimpl->type_loading = true;

  FileType* o_parseType = current_parse_type;
  current_parse_type = type;

  parseTypeBlock(elem);

  const auto baseSchemeName = qualifyOwnName(type->getName());
  if (baseSchemeName != nullptr) {
    const auto sh = schemeHash.find(*baseSchemeName);
    type->pimpl->baseScheme = sh == schemeHash.end() ? nullptr : sh->second;
  }
  if (type->pimpl->baseScheme == nullptr && !type->pimpl->isPackage) {
    COLORER_LOG_WARN("Type '%' has no default scheme", typeName);
  }
  type->pimpl->loadDone = true;
  current_parse_type = o_parseType;
}

void HrcLibrary::Impl::parseTypeBlock(const XMLNode& elem)
{
  for (const auto& node : elem.children) {
    if (node.name == hrcTagRegion) {
      addTypeRegion(node);
    }
    else if (node.name == hrcTagEntity) {
      addTypeEntity(node);
    }
    else if (node.name == hrcTagImport) {
      addTypeImport(node);
    }
    else if (node.name == hrcTagScheme) {
      addScheme(node);
    }
    else if (node.name == hrcTagAnnotation) {
      // not read annotation
    }
  }
}

void HrcLibrary::Impl::addTypeRegion(const XMLNode& elem)
{
  const auto& regionName = elem.getAttrValue(hrcRegionAttrName);

  if (regionName.isEmpty()) {
    COLORER_LOG_ERROR("No 'name' attribute in <region> element");
    return;
  }
  const auto qname1 = qualifyOwnName(UnicodeString(regionName));
  if (qname1 == nullptr) {
    return;
  }
  if (regionNamesHash.find(*qname1) != regionNamesHash.end()) {
    COLORER_LOG_WARN("Duplicate region '%' definition in type '%'", *qname1, current_parse_type->getName());
    return;
  }

  const auto& regionParent = elem.getAttrValue(hrcRegionAttrParent);
  const auto& regionDescr = elem.getAttrValue(hrcRegionAttrDescription);
  const auto qname2 =
      qualifyForeignName(regionParent.isEmpty() ? nullptr : &regionParent, QualifyNameType::QNT_DEFINE, true);

  const Region* region = new Region(*qname1, &regionDescr, getRegion(qname2.get()), regionNamesVector.size());
  regionNamesVector.push_back(region);
  regionNamesHash.try_emplace(*qname1, region);
}

void HrcLibrary::Impl::addTypeEntity(const XMLNode& elem)
{
  const auto& entityName = elem.getAttrValue(hrcEntityAttrName);
  if (entityName.isEmpty() || !elem.isExist(hrcEntityAttrValue)) {
    COLORER_LOG_ERROR("Bad entity attributes");
    return;
  }
  const auto& entityValue = elem.getAttrValue(hrcEntityAttrValue);

  const auto qname1 = qualifyOwnName(entityName);
  uUnicodeString qname2 = useEntities(&entityValue);
  if (qname1 != nullptr && qname2 != nullptr) {
    schemeEntitiesHash.try_emplace(*qname1, qname2.release());
  }
}

void HrcLibrary::Impl::addTypeImport(const XMLNode& elem)
{
  const auto& typeParam = elem.getAttrValue(hrcImportAttrType);
  if (typeParam.isEmpty() || fileTypeHash.find(typeParam) == fileTypeHash.end()) {
    COLORER_LOG_ERROR("Import with bad '%' attribute in type '%'", typeParam, current_parse_type->pimpl->name);
    return;
  }
  current_parse_type->pimpl->importVector.emplace_back(typeParam);
}

void HrcLibrary::Impl::addScheme(const XMLNode& elem)
{
  const auto& schemeName = elem.getAttrValue(hrcSchemeAttrName);
  // todo check schemeName
  const auto qSchemeName = qualifyOwnName(schemeName);
  if (qSchemeName == nullptr) {
    COLORER_LOG_ERROR("bad scheme name in type '%'", current_parse_type->pimpl->name);
    return;
  }
  if (schemeHash.find(*qSchemeName) != schemeHash.end() || disabledSchemes.find(*qSchemeName) != disabledSchemes.end())
  {
    COLORER_LOG_ERROR("duplicate scheme name '%'", *qSchemeName);
    return;
  }

  auto* scheme = new SchemeImpl(qSchemeName.get());
  scheme->fileType = current_parse_type;

  schemeHash.try_emplace(*scheme->getName(), scheme);
  const auto& condIf = elem.getAttrValue(hrcSchemeAttrIf);
  const auto& condUnless = elem.getAttrValue(hrcSchemeAttrUnless);
  const UnicodeString* p1 = current_parse_type->getParamValue(condIf);
  const UnicodeString* p2 = current_parse_type->getParamValue(condUnless);
  if ((!condIf.isEmpty() && p1 && p1->compare("true") != 0) ||
      (!condUnless.isEmpty() && p2 && p2->compare("true") == 0))
  {
    // disabledSchemes.put(scheme->schemeName, 1);
    return;
  }
  parseSchemeBlock(scheme, elem);
}

void HrcLibrary::Impl::parseSchemeBlock(SchemeImpl* scheme, const XMLNode& elem)
{
  for (const auto& node : elem.children) {
    if (node.name == hrcTagInherit) {
      addSchemeInherit(scheme, node);
      continue;
    }
    if (node.name == hrcTagRegexp) {
      addSchemeRegexp(scheme, node);
      continue;
    }
    if (node.name == hrcTagBlock) {
      addSchemeBlock(scheme, node);
      continue;
    }
    if (node.name == hrcTagKeywords) {
      parseSchemeKeywords(scheme, node);
      continue;
    }
    if (node.name == hrcTagAnnotation) {
      // not read annotation
      continue;
    }
  }
}

void HrcLibrary::Impl::addSchemeInherit(SchemeImpl* scheme, const XMLNode& elem)
{
  auto nqSchemeName = elem.getAttrValue(hrcInheritAttrScheme);
  if (nqSchemeName.isEmpty()) {
    COLORER_LOG_ERROR("there is empty scheme name in inherit block of scheme '%', skip this inherit block.",
                      *scheme->schemeName);
    return;
  }
  auto scheme_node = std::make_unique<SchemeNodeInherit>();
  scheme_node->schemeName = std::make_unique<UnicodeString>(nqSchemeName);
  auto schemeName = qualifyForeignName(scheme_node->schemeName.get(), QualifyNameType::QNT_SCHEME, false);
  if (schemeName != nullptr) {
    scheme_node->scheme = schemeHash.find(*schemeName)->second;
    scheme_node->schemeName = std::move(schemeName);
  }

  for (const auto& node : elem.children) {
    if (node.name == hrcTagVirtual) {
      const auto& x_schemeName = node.getAttrValue(hrcVirtualAttrScheme);
      const auto& x_substName = node.getAttrValue(hrcVirtualAttrSubstScheme);
      if (x_schemeName.isEmpty() || x_substName.isEmpty()) {
        COLORER_LOG_ERROR("there is bad virtualize attributes of scheme '%', skip this virtual block.",
                          *scheme_node->schemeName);
        continue;
      }
      scheme_node->virtualEntryVector.emplace_back(new VirtualEntry(&x_schemeName, &x_substName));
    }
  }
  scheme->nodes.push_back(std::move(scheme_node));
}

void HrcLibrary::Impl::addSchemeRegexp(SchemeImpl* scheme, const XMLNode& elem)
{
  auto matchParam = elem.getAttrValue(hrcRegexpAttrMatch);

  if (matchParam.isEmpty()) {
    matchParam = elem.text;
  }

  if (matchParam.isEmpty()) {
    COLORER_LOG_ERROR("there is no 'match' attribute in regexp of scheme '%', skip this regexp block.",
                      *scheme->schemeName);
    return;
  }

  const auto entMatchParam = useEntities(&matchParam);
  auto regexp = std::make_unique<CRegExp>(entMatchParam.get());
  if (!regexp->isOk()) {
    COLORER_LOG_ERROR("fault compiling regexp '%' of scheme '%', skip this regexp block.", *entMatchParam,
                      *scheme->schemeName);
    return;
  }

  auto scheme_node = std::make_unique<SchemeNodeRegexp>();
  const auto& dhrcRegexpAttrPriority = elem.getAttrValue(hrcRegexpAttrPriority);
  scheme_node->lowPriority = UnicodeString(value_low).compare(dhrcRegexpAttrPriority) == 0;
  scheme_node->start = std::move(regexp);
  scheme_node->start->setPositionMoves(false);

  loadRegexpRegions(scheme_node.get(), elem);
  if (scheme_node->region) {
    scheme_node->regions[0] = scheme_node->region;
  }

  scheme->nodes.push_back(std::move(scheme_node));
}

void HrcLibrary::Impl::addSchemeBlock(SchemeImpl* scheme, const XMLNode& elem)
{
  auto start_param = elem.getAttrValue(hrcBlockAttrStart);
  auto end_param = elem.getAttrValue(hrcBlockAttrEnd);

  const XMLNode* element_start = nullptr;
  const XMLNode* element_end = nullptr;

  if (end_param.isEmpty() && start_param.isEmpty()) {
    // don`t use range-based loop , we need pointer to elements
    for (auto node = elem.children.begin(); node != elem.children.end(); ++node) {
      if (node->name == hrcBlockAttrStart) {
        element_start = &*node;
        if (node->isExist(hrcBlockAttrMatch)) {
          start_param = node->getAttrValue(hrcBlockAttrMatch);
        }
        else {
          start_param = node->text;
        }
      }
      else if (node->name == hrcBlockAttrEnd) {
        element_end = &*node;
        if (node->isExist(hrcBlockAttrMatch)) {
          end_param = node->getAttrValue(hrcBlockAttrMatch);
        }
        else {
          end_param = node->text;
        }
      }
    }
  }

  if (start_param.isEmpty()) {
    COLORER_LOG_ERROR("there is no 'start' attribute in block of scheme '%', skip this block.", *scheme->schemeName);
    return;
  }
  if (end_param.isEmpty()) {
    COLORER_LOG_ERROR("there is no 'end' attribute in block of scheme '%', skip this block.", *scheme->schemeName);
    return;
  }
  const auto& schemeName = elem.getAttrValue(hrcBlockAttrScheme);
  if (schemeName.isEmpty()) {
    COLORER_LOG_ERROR("there is no 'scheme' attribute in block of scheme '%', skip this block.", *scheme->schemeName);
    return;
  }

  const uUnicodeString startParam = useEntities(&start_param);
  auto start_regexp = std::make_unique<CRegExp>(startParam.get());
  start_regexp->setPositionMoves(false);
  if (!start_regexp->isOk()) {
    COLORER_LOG_ERROR("fault compiling start regexp '%' in block of scheme '%', skip this block.", *startParam,
                      *scheme->schemeName);
    return;
  }

  const uUnicodeString endParam = useEntities(&end_param);
  auto end_regexp = std::make_unique<CRegExp>();
  end_regexp->setPositionMoves(true);
  end_regexp->setBackRE(start_regexp.get());
  end_regexp->setRE(endParam.get());
  if (!end_regexp->isOk()) {
    COLORER_LOG_ERROR("fault compiling end regexp '%' in block of scheme '%', skip this block.", *startParam,
                      *scheme->schemeName);
    return;
  }

  const auto& attr_pr = elem.getAttrValue(hrcBlockAttrPriority);
  const auto& attr_cpr = elem.getAttrValue(hrcBlockAttrContentPriority);
  const auto& attr_ireg = elem.getAttrValue(hrcBlockAttrInnerRegion);

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

void HrcLibrary::Impl::parseSchemeKeywords(SchemeImpl* scheme, const XMLNode& elem)
{
  const auto rg_tmpl = UnicodeString(u"region");
  const Region* region = getNCRegion(&elem, rg_tmpl);
  if (region == nullptr) {
    COLORER_LOG_ERROR(
        "there is no 'region' attribute in keywords block of scheme '%', skip this keywords "
        "block.",
        *scheme->schemeName);
    return;
  }

  const auto& worddiv = elem.getAttrValue(hrcKeywordsAttrWorddiv);
  std::unique_ptr<CharacterClass> us_worddiv;
  if (!worddiv.isEmpty()) {
    const uUnicodeString entWordDiv = useEntities(&worddiv);
    us_worddiv = UStr::createCharClass(*entWordDiv.get(), 0, nullptr, false);
    if (us_worddiv == nullptr) {
      COLORER_LOG_ERROR(
          "fault compiling worddiv regexp '%' in keywords block of scheme '%'. skip this "
          "keywords block.",
          *entWordDiv, *scheme->schemeName);
    }
  }

  auto count = getSchemeKeywordsCount(elem);
  const auto& ignorecase_string = elem.getAttrValue(hrcKeywordsAttrIgnorecase);
  auto scheme_node = std::make_unique<SchemeNodeKeywords>();
  scheme_node->worddiv = std::move(us_worddiv);
  scheme_node->kwList = std::make_unique<KeywordList>(count);
  scheme_node->kwList->matchCase = UnicodeString(value_yes).compare(ignorecase_string) != 0;

  loopSchemeKeywords(elem, scheme, scheme_node.get(), region);
  scheme_node->kwList->firstChar->freeze();

  // TODO unique keywords in list
  scheme_node->kwList->sortList();
  scheme_node->kwList->substrIndex();
  scheme->nodes.push_back(std::move(scheme_node));
}

void HrcLibrary::Impl::loopSchemeKeywords(const XMLNode& elem, const SchemeImpl* scheme,
                                          const SchemeNodeKeywords* scheme_node, const Region* region)
{
  for (const auto& node : elem.children) {
    if (node.name == hrcTagWord) {
      addSchemeKeyword(node, scheme, scheme_node, region, KeywordInfo::KeywordType::KT_WORD);
    }
    else if (node.name == hrcTagSymb) {
      addSchemeKeyword(node, scheme, scheme_node, region, KeywordInfo::KeywordType::KT_SYMB);
    }
  }
}

void HrcLibrary::Impl::updatePrototype(const XMLNode& elem)
{
  const auto& typeName = elem.getAttrValue(hrcPrototypeAttrName);
  if (typeName.isEmpty()) {
    return;
  }

  const auto current_parse_prototype = getFileType(&typeName);
  if (!current_parse_prototype) {
    COLORER_LOG_WARN("Prototype '%' not found. Skipped.", typeName);
    return;
  }

  bool cleaned_detect_param = false;
  for (const auto& node : elem.children) {
    if (node.name == hrcTagParametrs) {
      for (const auto& node2 : node.children) {
        updatePrototypeParams(node2, current_parse_prototype);
      }
    }
    else if (node.name == hrcTagParam) {
      // for backward compatibility
      updatePrototypeParams(node, current_parse_prototype);
    }
    else if (node.name == hrcTagFilename || node.name == hrcTagFirstline) {
      if (!cleaned_detect_param) {
        // rewrite all detect params
        current_parse_prototype->pimpl->chooserVector.clear();
        cleaned_detect_param = true;
      }
      addPrototypeDetectParam(node, current_parse_prototype);
    }
  }
}

void HrcLibrary::Impl::updatePrototypeParams(const XMLNode& node, FileType* current_parse_prototype)
{
  if (node.name == hrcTagParam) {
    const auto& name = node.getAttrValue(hrcParamAttrName);

    if (name.isEmpty()) {
      return;
    }

    const auto& value = node.getAttrValue(hrcParamAttrValue);
    const auto& descr = node.getAttrValue(hrcParamAttrDescription);

    if (current_parse_prototype->getParamValue(name) == nullptr) {
      // параметр еще не задан для прототипа
      // загружаем параметры приложения, добавляем атрибут и значение
      current_parse_prototype->addParam(name, value);
    }
    else {
      // параметр уже задан для прототипа
      // сохраняем как значение по умолчанию
      current_parse_prototype->setParamDefaultValue(name, &value);
    }

    if (!descr.isEmpty()) {
      // обновляем описание параметра, если задано
      current_parse_prototype->setParamDescription(name, &descr);
    }
  }
}

void HrcLibrary::Impl::addSchemeKeyword(const XMLNode& elem, const SchemeImpl* scheme,
                                        const SchemeNodeKeywords* scheme_node, const Region* region,
                                        const KeywordInfo::KeywordType keyword_type)
{
  const auto& keyword_value = elem.getAttrValue(hrcWordAttrName);
  if (keyword_value.isEmpty()) {
    COLORER_LOG_WARN("the 'name' attribute in the '%' element of scheme '%' is empty or missing, skip it.",
                     *scheme->schemeName, keyword_type == KeywordInfo::KeywordType::KT_WORD ? "word" : "symb");
    return;
  }

  const Region* rgn = region;
  const auto& reg = elem.getAttrValue(hrcWordAttrRegion);
  if (!reg.isEmpty()) {
    rgn = getNCRegion(&elem, hrcWordAttrRegion);
  }

  KeywordInfo& list = scheme_node->kwList->kwList[scheme_node->kwList->count];
  list.keyword = std::make_unique<UnicodeString>(keyword_value);
  list.region = rgn;
  list.isSymbol = keyword_type == KeywordInfo::KeywordType::KT_SYMB;
  auto* first_char = scheme_node->kwList->firstChar.get();
  first_char->add(keyword_value[0]);
  if (!scheme_node->kwList->matchCase) {
    first_char->add(Character::toLowerCase(keyword_value[0]));
    first_char->add(Character::toUpperCase(keyword_value[0]));
    first_char->add(Character::toTitleCase(keyword_value[0]));
  }
  scheme_node->kwList->count++;
  scheme_node->kwList->minKeywordLength = std::min(scheme_node->kwList->minKeywordLength, list.keyword->length());
}

size_t HrcLibrary::Impl::getSchemeKeywordsCount(const XMLNode& elem)
{
  size_t result = 0;
  for (const auto& node : elem.children) {
    if (node.name == hrcTagWord || node.name == hrcTagSymb) {
      const auto& atr = node.getAttrValue(hrcWordAttrName);
      if (!atr.isEmpty()) {
        result++;
      }
    }
  }
  return result;
}

void HrcLibrary::Impl::loadRegexpRegions(SchemeNodeRegexp* node, const XMLNode& el)
{
  char16_t rg_tmpl[] = u"region\0";

  if (node->region == nullptr) {
    node->region = getNCRegion(&el, UnicodeString(rg_tmpl));
  }

  for (int i = 0; i < REGIONS_NUM; i++) {
    rg_tmpl[6] = static_cast<char16_t>((i < 0xA ? i : i + 39) + '0');
    node->regions[i] = getNCRegion(&el, UnicodeString(rg_tmpl));
  }

  for (int i = 0; i < NAMED_REGIONS_NUM; i++) {
    node->regionsn[i] = getNCRegion(node->start->getBracketName(i), false);
  }
}

void HrcLibrary::Impl::loadRegions(SchemeNodeBlock* node, const XMLNode* el, const bool start_element)
{
  char16_t rg_tmpl[] = u"region\0";
  if (el) {
    if (node->region == nullptr) {
      node->region = getNCRegion(el, UnicodeString(rg_tmpl));
    }

    for (int i = 0; i < REGIONS_NUM; i++) {
      rg_tmpl[6] = static_cast<char16_t>((i < 0xA ? i : i + 39) + '0');

      const auto* reg = getNCRegion(el, UnicodeString(rg_tmpl));
      if (start_element) {
        node->regions[i] = reg;
      }
      else {
        node->regione[i] = reg;
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

void HrcLibrary::Impl::loadBlockRegions(SchemeNodeBlock* node, const XMLNode& el)
{
  // regions as attributes in main block element

  char16_t rg_tmpl[] = u"region\0\0";

  node->region = getNCRegion(&el, UnicodeString(rg_tmpl));
  for (int i = 0; i < REGIONS_NUM; i++) {
    rg_tmpl[6] = '0';
    rg_tmpl[7] = static_cast<char16_t>((i < 0xA ? i : i + 39) + '0');
    node->regions[i] = getNCRegion(&el, UnicodeString(rg_tmpl));
    rg_tmpl[6] = '1';
    node->regione[i] = getNCRegion(&el, UnicodeString(rg_tmpl));
  }
}

void HrcLibrary::Impl::updateSchemeLink(uUnicodeString& scheme_name, SchemeImpl** scheme_impl, const byte scheme_type,
                                        const SchemeImpl* current_scheme)
{
  static const char* message[4] = {"cannot resolve scheme name '%' of block in scheme '%'",
                                   "cannot resolve scheme name '%' of inherit in scheme '%'",
                                   "cannot resolve scheme name '%' of virtual in scheme '%'",
                                   "cannot resolve subst-scheme name '%' of virtual in scheme '%'"};

  if (scheme_name != nullptr && *scheme_impl == nullptr) {
    const auto schemeName = qualifyForeignName(scheme_name.get(), QualifyNameType::QNT_SCHEME, true);
    if (schemeName != nullptr) {
      *scheme_impl = schemeHash.find(*schemeName)->second;
    }
    else {
      COLORER_LOG_ERROR(message[scheme_type], *scheme_name, *current_scheme->schemeName);
    }

    scheme_name.reset();
  }
}

void HrcLibrary::Impl::updateLinks()
{
  while (structureChanged) {
    structureChanged = false;
    for (auto const& [key, scheme] : schemeHash) {
      if (!scheme->fileType->pimpl->loadDone) {
        continue;
      }
      FileType* old_parseType = current_parse_type;
      current_parse_type = scheme->fileType;
      for (const auto& snode : scheme->nodes) {
        if (snode->type == SchemeNode::SchemeNodeType::SNT_BLOCK) {
          auto* snode_block = static_cast<SchemeNodeBlock*>(snode.get());

          updateSchemeLink(snode_block->schemeName, &snode_block->scheme, 0, scheme);
        }

        if (snode->type == SchemeNode::SchemeNodeType::SNT_INHERIT) {
          auto* snode_inherit = static_cast<SchemeNodeInherit*>(snode.get());

          updateSchemeLink(snode_inherit->schemeName, &snode_inherit->scheme, 1, scheme);
          for (auto* vt : snode_inherit->virtualEntryVector) {
            updateSchemeLink(vt->virtSchemeName, &vt->virtScheme, 2, scheme);
            updateSchemeLink(vt->substSchemeName, &vt->substScheme, 3, scheme);
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

uUnicodeString HrcLibrary::Impl::qualifyOwnName(const UnicodeString& name) const
{
  const auto colon = name.indexOf(':');
  if (colon != -1) {
    if (UnicodeString(name, 0, colon).compare(current_parse_type->pimpl->name) != 0) {
      COLORER_LOG_ERROR("type name qualifer in '%' doesn't match current type '%'", name,
                        current_parse_type->pimpl->name);
      return nullptr;
    }
    return std::make_unique<UnicodeString>(name);
  }

  auto sbuf = std::make_unique<UnicodeString>(current_parse_type->getName());
  sbuf->append(":").append(name);
  return sbuf;
}

bool HrcLibrary::Impl::checkNameExist(const UnicodeString* name, FileType* parseType, const QualifyNameType qntype,
                                      const bool logErrors)
{
  if (qntype == QualifyNameType::QNT_DEFINE && regionNamesHash.find(*name) == regionNamesHash.end()) {
    if (logErrors) {
      COLORER_LOG_ERROR("region '%', referenced in type '%', is not defined", *name, parseType->pimpl->name);
    }
    return false;
  }
  if (qntype == QualifyNameType::QNT_ENTITY && schemeEntitiesHash.find(*name) == schemeEntitiesHash.end()) {
    if (logErrors) {
      COLORER_LOG_ERROR("entity '%', referenced in type '%', is not defined", *name, parseType->pimpl->name);
    }
    return false;
  }
  if (qntype == QualifyNameType::QNT_SCHEME && schemeHash.find(*name) == schemeHash.end()) {
    if (logErrors) {
      COLORER_LOG_ERROR("scheme '%', referenced in type '%', is not defined", *name, parseType->pimpl->name);
    }
    return false;
  }
  return true;
}

uUnicodeString HrcLibrary::Impl::qualifyForeignName(const UnicodeString* name, const QualifyNameType qntype,
                                                    const bool logErrors)
{
  if (name == nullptr) {
    return nullptr;
  }
  const auto colon = name->indexOf(':');
  if (colon != -1) {  // qualified name
    const UnicodeString prefix(*name, 0, colon);
    const auto ft = fileTypeHash.find(prefix);
    FileType* prefType = nullptr;
    if (ft != fileTypeHash.end()) {
      prefType = ft->second;
    }

    if (prefType == nullptr) {
      if (logErrors) {
        COLORER_LOG_ERROR("type name qualifer in '%' doesn't match any type", *name);
      }
      return nullptr;
    }
    if (!prefType->pimpl->type_loading) {
      loadFileType(prefType);
    }
    if (prefType == current_parse_type || prefType->pimpl->type_loading) {
      return checkNameExist(name, prefType, qntype, logErrors) ? std::make_unique<UnicodeString>(*name) : nullptr;
    }
  }
  else {  // unqualified name
    for (int idx = -1;
         current_parse_type != nullptr && idx < static_cast<int>(current_parse_type->pimpl->importVector.size()); idx++)
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
      COLORER_LOG_ERROR("unqualified name '%' doesn't belong to any imported type [%]", *name,
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
    const auto elpos = name->indexOf(';', epos);
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

const Region* HrcLibrary::Impl::getNCRegion(const UnicodeString* name, const bool logErrors)
{
  if (name == nullptr || name->isEmpty()) {
    return nullptr;
  }
  const Region* reg = nullptr;
  const auto qname = qualifyForeignName(name, QualifyNameType::QNT_DEFINE, logErrors);
  if (qname == nullptr) {
    return nullptr;
  }
  const auto reg_ = regionNamesHash.find(*qname);
  if (reg_ != regionNamesHash.end()) {
    reg = reg_->second;
  }

  /** Check for 'default' region request.
      Regions with this name are always transparent
  */
  if (reg != nullptr) {
    const auto& s_name = reg->getName();
    const auto idx = s_name.indexOf(":default");
    if (idx != -1 && idx + 8 == s_name.length()) {
      return nullptr;
    }
  }
  return reg;
}

const Region* HrcLibrary::Impl::getNCRegion(const XMLNode* elem, const UnicodeString& tag)
{
  if (elem->isExist(tag)) {
    const auto& par = elem->getAttrValue(tag);
    return getNCRegion(&par, true);
  }
  return nullptr;
}
