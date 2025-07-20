#include "colorer/HrcLibrary.h"
#include "colorer/parsers/HrcLibraryImpl.h"

HrcLibrary::HrcLibrary() : pimpl(spimpl::make_unique_impl<Impl>()) {}

void HrcLibrary::loadSource(XmlInputSource* is)
{
  pimpl->loadSource(is, Impl::LoadType::FULL);
}

void HrcLibrary::loadProtoTypes(XmlInputSource* is)
{
  pimpl->loadSource(is, Impl::LoadType::PROTOTYPE);
}

FileType* HrcLibrary::enumerateFileTypes(unsigned int index)
{
  return pimpl->enumerateFileTypes(index);
}

FileType* HrcLibrary::getFileType(const UnicodeString* name)
{
  return pimpl->getFileType(name);
}

FileType* HrcLibrary::getFileType(const UnicodeString& name)
{
  return pimpl->getFileType(&name);
}

FileType* HrcLibrary::chooseFileType(const UnicodeString* fileName, const UnicodeString* firstLine, int typeNo)
{
  return pimpl->chooseFileType(fileName, firstLine, typeNo);
}

size_t HrcLibrary::getFileTypesCount()
{
  return pimpl->getFileTypesCount();
}

size_t HrcLibrary::getRegionCount()
{
  return pimpl->getRegionCount();
}

const Region* HrcLibrary::getRegion(unsigned int id)
{
  return pimpl->getRegion(id);
}

const Region* HrcLibrary::getRegion(const UnicodeString* name)
{
  return pimpl->getRegion(name);
}

void HrcLibrary::loadFileType(FileType* filetype)
{
  pimpl->loadFileType(filetype);
}

void HrcLibrary::loadHrcSettings(const XmlInputSource& is)
{
  pimpl->loadHrcSettings(is);
}
