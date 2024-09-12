#include "colorer/parsers/ParserFactoryImpl.h"
#include "colorer/base/BaseNames.h"
#include "colorer/parsers/CatalogParser.h"
#include "colorer/parsers/HrcLibraryImpl.h"
#include "colorer/utils/Environment.h"

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

    auto env = colorer::Environment::getOSVariable("COLORER_CATALOG");
    if (!env || env->isEmpty()) {
      throw ParserFactoryException("Can't find suitable catalog.xml for parse.");
    }
    base_catalog_path = colorer::Environment::normalizePath(env.get());
  }
  else {
    COLORER_LOG_DEBUG("loadCatalog for %", *catalog_path);
    base_catalog_path = colorer::Environment::normalizePath(catalog_path);
  }

  parseCatalog(*base_catalog_path);
  COLORER_LOG_DEBUG("start load hrc files");
  for (const auto& location : hrc_locations) {
    loadHrcPath(location);
  }

  COLORER_LOG_DEBUG("end load hrc files");
}

void ParserFactory::Impl::loadHrcPath(const UnicodeString& location)
{
  try {
    COLORER_LOG_DEBUG("try load '%'", location);
    if (XmlInputSource::isFileURI(*base_catalog_path, &location)) {
      auto files = colorer::Environment::getFilesFromPath(base_catalog_path.get(), &location, ".hrc");
      for (auto& file : files) {
        loadHrc(file, nullptr);
      }
    }
    else {
      loadHrc(location, base_catalog_path.get());
    }
  } catch (const Exception& e) {
    COLORER_LOG_ERROR("%", e.what());
  }
}

void ParserFactory::Impl::loadHrc(const UnicodeString& hrc_path, const UnicodeString* base_path) const
{
  XmlInputSource dfis(hrc_path, base_path);
  try {
    hrc_library->loadSource(&dfis);
  } catch (Exception& e) {
    COLORER_LOG_ERROR("Can't load hrc: %", dfis.getPath());
    COLORER_LOG_ERROR("%", e.what());
  }
}

void ParserFactory::Impl::parseCatalog(const UnicodeString& catalog_path)
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

[[maybe_unused]]
std::vector<UnicodeString> ParserFactory::Impl::enumHrdClasses()
{
  std::vector<UnicodeString> result;
  result.reserve(hrd_nodes.size());
  for (auto& hrd_node : hrd_nodes) {
    result.push_back(hrd_node.first);
  }
  return result;
}

std::vector<const HrdNode*> ParserFactory::Impl::enumHrdInstances(const UnicodeString& classID)
{
  auto hash = hrd_nodes.find(classID);
  std::vector<const HrdNode*> result;
  result.reserve(hash->second->size());
  for (auto& p : *hash->second) {
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
  for (auto& p : *hash->second) {
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
  UnicodeString class_id = UnicodeString(HrdClassText);

  auto mapper = std::make_unique<TextHRDMapper>();
  fillMapper(class_id, nameID, *mapper);

  return mapper;
}

void ParserFactory::Impl::fillMapper(const UnicodeString& classID, const UnicodeString* nameID, RegionMapper& mapper)
{
  const UnicodeString* name_id;
  const UnicodeString name_default(HrdNameDefault);
  if (nameID == nullptr) {
    auto hrd = colorer::Environment::getOSVariable("COLORER_HRD");
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
    if (idx.length() != 0) {
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
    hrd_nodes.emplace(hrd->hrd_class, std::make_unique<std::vector<std::unique_ptr<HrdNode>>>());
  }
  hrd_nodes.at(hrd->hrd_class)->emplace_back(std::move(hrd));
}
