#include "colorer/HrcLibrary.h"
#include "colorer/parsers/HrcLibraryImpl.h"

#include <mutex>
#include <shared_mutex>

HrcLibrary::HrcLibrary() : pimpl(spimpl::make_unique_impl<Impl>()) {}

void HrcLibrary::loadSource(XmlInputSource* is)
{
  std::unique_lock<std::shared_mutex> lock(pimpl->access);
  pimpl->loadSource(is, Impl::LoadType::FULL);
}

void HrcLibrary::loadProtoTypes(XmlInputSource* is)
{
  std::unique_lock<std::shared_mutex> lock(pimpl->access);
  pimpl->loadSource(is, Impl::LoadType::PROTOTYPE);
}

FileType* HrcLibrary::enumerateFileTypes(unsigned int index)
{
  std::shared_lock<std::shared_mutex> lock(pimpl->access);
  return pimpl->enumerateFileTypes(index);
}

FileType* HrcLibrary::getFileType(const UnicodeString* name)
{
  std::shared_lock<std::shared_mutex> lock(pimpl->access);
  return pimpl->getFileType(name);
}

FileType* HrcLibrary::getFileType(const UnicodeString& name)
{
  std::shared_lock<std::shared_mutex> lock(pimpl->access);
  return pimpl->getFileType(&name);
}

FileType* HrcLibrary::chooseFileType(const UnicodeString* fileName, const UnicodeString* firstLine, int typeNo)
{
  std::shared_lock<std::shared_mutex> lock(pimpl->access);
  return pimpl->chooseFileType(fileName, firstLine, typeNo);
}

size_t HrcLibrary::getFileTypesCount()
{
  std::shared_lock<std::shared_mutex> lock(pimpl->access);
  return pimpl->getFileTypesCount();
}

size_t HrcLibrary::getRegionCount()
{
  std::shared_lock<std::shared_mutex> lock(pimpl->access);
  return pimpl->getRegionCount();
}

const Region* HrcLibrary::getRegion(unsigned int id)
{
  std::shared_lock<std::shared_mutex> lock(pimpl->access);
  return pimpl->getRegion(id);
}

const Region* HrcLibrary::getRegion(const UnicodeString* name)
{
  std::unique_lock<std::shared_mutex> lock(pimpl->access);
  return pimpl->getRegion(name);
}

void HrcLibrary::loadFileType(FileType* filetype)
{
  std::unique_lock<std::shared_mutex> lock(pimpl->access);
  pimpl->loadFileType(filetype);
}

void HrcLibrary::loadHrcSettings(const XmlInputSource& is)
{
  std::unique_lock<std::shared_mutex> lock(pimpl->access);
  pimpl->loadHrcSettings(is);
}
