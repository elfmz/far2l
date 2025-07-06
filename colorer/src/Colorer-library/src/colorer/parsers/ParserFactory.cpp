#include "colorer/ParserFactory.h"
#include "colorer/parsers/ParserFactoryImpl.h"

ParserFactory::ParserFactory() : pimpl(spimpl::make_unique_impl<Impl>()) {}

void ParserFactory::loadCatalog(const UnicodeString* catalog_path)
{
  pimpl->loadCatalog(catalog_path);
}

HrcLibrary& ParserFactory::getHrcLibrary() const
{
  return pimpl->getHrcLibrary();
}

std::unique_ptr<TextParser> ParserFactory::createTextParser()
{
  return pimpl->createTextParser();
}

std::unique_ptr<StyledHRDMapper> ParserFactory::createStyledMapper(const UnicodeString* classID,
                                                                   const UnicodeString* nameID)
{
  return pimpl->createStyledMapper(classID, nameID);
}

std::unique_ptr<TextHRDMapper> ParserFactory::createTextMapper(const UnicodeString* nameID)
{
  return pimpl->createTextMapper(nameID);
}

std::vector<const HrdNode*> ParserFactory::enumHrdInstances(const UnicodeString& classID)
{
  return pimpl->enumHrdInstances(classID);
}

void ParserFactory::addHrd(std::unique_ptr<HrdNode> hrd)
{
  pimpl->addHrd(std::move(hrd));
}

const HrdNode& ParserFactory::getHrdNode(const UnicodeString& classID, const UnicodeString& nameID)
{
  return pimpl->getHrdNode(classID, nameID);
}

void ParserFactory::loadHrcPath(const UnicodeString* location)
{
  pimpl->loadHrcPath(location, nullptr);
}

void ParserFactory::loadHrcSettings(const UnicodeString* location, bool user_defined)
{
  pimpl->loadHrcSettings(location, user_defined);
}

void ParserFactory::loadHrdPath(const UnicodeString* location)
{
  pimpl->loadHrdPath(location);
}