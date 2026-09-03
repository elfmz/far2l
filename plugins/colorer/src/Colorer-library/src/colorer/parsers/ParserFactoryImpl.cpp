#include "colorer/parsers/ParserFactoryImpl.h"
#include "colorer/base/BaseNames.h"
#include "colorer/base/XmlTagDefs.h"
#include "colorer/parsers/CatalogParser.h"
#include "colorer/parsers/HrcLibraryImpl.h"
#include "colorer/utils/Environment.h"
#include "colorer/xml/XmlReader.h"

ParserFactory::Impl::Impl()
{
  hrc_library = new HrcLibrary();
}

ParserFactory::Impl::~Impl()
{
  delete hrc_library;
  CRegExp::clearRegExpStack();
}

void ParserFactory::Impl::loadCatalog(const UnicodeString* catalog_path)
{
  if (!catalog_path || catalog_path->isEmpty()) {
    COLORER_LOG_DEBUG("loadCatalog for empty path");

    auto env = colorer::Environment::getOSEnv("COLORER_CATALOG");
    if (!env || env->isEmpty()) {
      throw ParserFactoryException("Can't find suitable catalog.xml for parse.");
    }
    base_catalog_path = colorer::Environment::normalizePath(env.get());
  }
  else {
    COLORER_LOG_DEBUG("loadCatalog for %", *catalog_path);
    base_catalog_path = colorer::Environment::normalizePath(catalog_path);
  }

  readCatalog(*base_catalog_path);
  COLORER_LOG_DEBUG("start load hrc files");
  // загружаем hrc файлы, прописанные в hrc-sets
  // это могут быть: относительные пути, полные пути, пути до папки с файлами
  for (const auto& location : hrc_locations) {
    loadHrcPath(&location, base_catalog_path.get());
  }

  COLORER_LOG_DEBUG("end load hrc files");
}

void ParserFactory::Impl::loadHrcPath(const UnicodeString* location, const UnicodeString* base_path) const
{
  if (!location) {
    return;
  }

  try {
    COLORER_LOG_DEBUG("try load '%'", *location);
    if (XmlInputSource::isFileSystemURI(*location, base_path)) {
      // путь к обычному файлу/папке
      UnicodeString full_path;
      if (colorer::Environment::isRegularFile(base_path, location, full_path)) {
        // файл
        loadHrc(full_path, nullptr);
      }
      else {
        // папка с файлами
        auto files = colorer::Environment::getFilesFromPath(full_path);
        for (auto const& file : files) {
          // загружаем файлы только с расширением hrc, кроме ent.hrc
          if (file.endsWith(u".hrc") && !file.endsWith(u".ent.hrc")) {
            loadHrc(file, nullptr);
          }
        }
      }
    }
    else {
      // путь до специального файла, например архив
      loadHrc(*location, base_path);
    }
  } catch (const Exception& e) {
    COLORER_LOG_ERROR("%", e.what());
  }
}

void ParserFactory::Impl::loadHrcSettings(const UnicodeString* location, const bool user_defined) const
{
  uUnicodeString path;
  if (!user_defined && (!location || location->isEmpty())) {
    // hrcsetting уровня приложения загружается по фиксированному пути
    return;
  }
  if (!location || location->isEmpty()) {
    COLORER_LOG_DEBUG("loadHrcSettings for empty path");

    const auto env = colorer::Environment::getOSEnv("COLORER_HRC_SETTINGS");
    if (!env || env->isEmpty()) {
      COLORER_LOG_DEBUG("The path to hrcsettings config not specified, skipped.");
      return;
    }
    path = colorer::Environment::normalizePath(env.get());
  }
  else {
    COLORER_LOG_DEBUG("loadHrcSettings for %", *location);
    path = colorer::Environment::normalizePath(location);
  }

  const XmlInputSource config(*path.get());
  hrc_library->loadHrcSettings(config);
}

void ParserFactory::Impl::loadHrdPath(const UnicodeString* location)
{
  if (!location) {
    return;
  }

  COLORER_LOG_DEBUG("start load hrd files");
  try {
    COLORER_LOG_DEBUG("try load '%'", *location);
    if (XmlInputSource::isFileSystemURI(*location, nullptr)) {
      // путь к обычному файлу/папке
      UnicodeString full_path;
      if (colorer::Environment::isRegularFile(nullptr, location, full_path)) {
        // файл, обрабатываем как hrd-sets
        loadHrdSets(full_path);
      }
      else {
        // папка с файлами
        auto files = colorer::Environment::getFilesFromPath(*location);
        for (auto const& file : files) {
          // загружаем файлы только с расширением hrd
          if (file.endsWith(u".hrd")) {
            loadHrd(file);
          }
        }
      }
    }
  } catch (const Exception& e) {
    COLORER_LOG_ERROR("%", e.what());
  }
  COLORER_LOG_DEBUG("end load hrc files");
}

void ParserFactory::Impl::loadHrc(const UnicodeString& hrc_path, const UnicodeString* base_path) const
{
  XmlInputSource file_input_source(hrc_path, base_path);
  try {
    // Загружаем только описания прототипов
    hrc_library->loadProtoTypes(&file_input_source);
  } catch (Exception& e) {
    COLORER_LOG_ERROR("Can't load hrc: %", file_input_source.getPath());
    COLORER_LOG_ERROR("%", e.what());
  }
}

void ParserFactory::Impl::loadHrd(const UnicodeString& hrd_path)
{
  const XmlInputSource config(hrd_path);
  XmlReader xml_parser(config);
  if (!xml_parser.parse()) {
    throw ParserFactoryException(UnicodeString("Error reading ").append(hrd_path));
  }

  std::list<XMLNode> nodes;
  xml_parser.getNodes(nodes);

  auto elem = nodes.begin();
  if (elem->name != catTagHrd) {
    throw Exception("main '<hrd>' block not found");
  }
  const auto& hrd_class = elem->getAttrValue(catHrdAttrClass);
  const auto& hrd_name = elem->getAttrValue(catHrdAttrName);
  if (hrd_class.isEmpty() || hrd_name.isEmpty()) {
    COLORER_LOG_WARN("found HRD with empty class/name in '%'. skip this record.", hrd_path);
    return;
  }
  const auto& xhrd_desc = elem->getAttrValue(catHrdAttrDescription);

  auto hrd_node = std::make_unique<HrdNode>();
  hrd_node->hrd_class = UnicodeString(hrd_class);
  hrd_node->hrd_name = UnicodeString(hrd_name);
  hrd_node->hrd_description = UnicodeString(xhrd_desc);
  hrd_node->hrd_location.push_back(hrd_path);
  addHrd(std::move(hrd_node));
}

void ParserFactory::Impl::loadHrdSets(const UnicodeString& hrd_path)
{
  const XmlInputSource config(hrd_path);
  XmlReader xml_parser(config);
  if (!xml_parser.parse()) {
    throw ParserFactoryException(UnicodeString("Error reading ").append(hrd_path));
  }

  std::list<XMLNode> nodes;
  xml_parser.getNodes(nodes);

  if (nodes.begin()->name != catTagHrdSets) {
    throw Exception("main '<hrd-sets>' block not found");
  }
  for (const auto& node : nodes.begin()->children) {
    if (node.name == catTagHrd) {
      auto hrd = CatalogParser::parseHRDSetsChild(node);
      if (hrd)
        addHrd(std::move(hrd));
    }
  }
}

void ParserFactory::Impl::readCatalog(const UnicodeString& catalog_path)
{
  CatalogParser catalog_parser;
  catalog_parser.parse(&catalog_path);

  hrc_locations.clear();
  hrd_nodes.clear();
  std::copy(catalog_parser.hrc_locations.begin(), catalog_parser.hrc_locations.end(),
            std::back_inserter(hrc_locations));

  for (auto& item : catalog_parser.hrd_nodes) {
    addHrd(std::move(item));
  }
}

std::vector<UnicodeString> ParserFactory::Impl::enumHrdClasses() const
{
  std::vector<UnicodeString> result;
  result.reserve(hrd_nodes.size());
  for (const auto& hrd_node : hrd_nodes) {
    result.push_back(hrd_node.first);
  }
  return result;
}

std::vector<const HrdNode*> ParserFactory::Impl::enumHrdInstances(const UnicodeString& classID) const
{
  auto hash = hrd_nodes.find(classID);
  std::vector<const HrdNode*> result;
  result.reserve(hash->second->size());
  for (const auto& p : *hash->second) {
    result.push_back(p.get());
  }
  return result;
}

const HrdNode& ParserFactory::Impl::getHrdNode(const UnicodeString& classID, const UnicodeString& nameID)
{
  auto hash = hrd_nodes.find(classID);
  if (hash == hrd_nodes.end()) {
    throw ParserFactoryException("can't find HRDClass '" + classID + "'");
  }
  for (const auto& p : *hash->second) {
    if (nameID.compare(p->hrd_name) == 0) {
      return *p;
    }
  }
  throw ParserFactoryException("can't find HRDName '" + nameID + "'");
}

HrcLibrary& ParserFactory::Impl::getHrcLibrary() const
{
  return *hrc_library;
}

std::unique_ptr<TextParser> ParserFactory::Impl::createTextParser()
{
  return std::make_unique<TextParser>();
}

std::unique_ptr<StyledHRDMapper> ParserFactory::Impl::createStyledMapper(const UnicodeString* classID,
                                                                         const UnicodeString* nameID)
{
  const UnicodeString* class_id;
  const UnicodeString class_default(HrdClassRgb);
  if (classID == nullptr) {
    class_id = &class_default;
  }
  else {
    class_id = classID;
  }

  auto mapper = std::make_unique<StyledHRDMapper>();
  fillMapper(*class_id, nameID, *mapper);

  return mapper;
}

std::unique_ptr<TextHRDMapper> ParserFactory::Impl::createTextMapper(const UnicodeString* nameID)
{
  auto class_id = UnicodeString(HrdClassText);

  auto mapper = std::make_unique<TextHRDMapper>();
  fillMapper(class_id, nameID, *mapper);

  return mapper;
}

void ParserFactory::Impl::fillMapper(const UnicodeString& classID, const UnicodeString* nameID, RegionMapper& mapper)
{
  const UnicodeString* name_id;
  const UnicodeString name_default(HrdNameDefault);
  uUnicodeString hrd;
  if (nameID == nullptr) {
    hrd = colorer::Environment::getOSEnv("COLORER_HRD");
    if (hrd) {
      name_id = hrd.get();
    }
    else {
      name_id = &name_default;
    }
  }
  else {
    name_id = nameID;
  }

  auto hrd_node = getHrdNode(classID, *name_id);

  for (const auto& idx : hrd_node.hrd_location) {
    if (!idx.isEmpty()) {
      try {
        XmlInputSource dfis(idx, base_catalog_path.get());
        mapper.loadRegionMappings(dfis);
      } catch (Exception& e) {
        COLORER_LOG_ERROR("Can't load hrd: ");
        COLORER_LOG_ERROR("%", e.what());
        throw ParserFactoryException("Error load hrd");
      }
    }
  }
}

void ParserFactory::Impl::addHrd(std::unique_ptr<HrdNode> hrd)
{
  if (hrd_nodes.find(hrd->hrd_class) == hrd_nodes.end()) {
    hrd_nodes.try_emplace(hrd->hrd_class, std::make_unique<std::vector<std::unique_ptr<HrdNode>>>());
  }
  hrd_nodes.at(hrd->hrd_class)->emplace_back(std::move(hrd));
}
