#include "colorer/xml/libxml2/SharedXmlInputSource.h"
#include <fstream>
#include "colorer/Exception.h"
#include "colorer/utils/Environment.h"

std::unordered_map<UnicodeString, SharedXmlInputSource*>* SharedXmlInputSource::isHash = nullptr;

int SharedXmlInputSource::addref()
{
  return ++ref_count;
}

int SharedXmlInputSource::delref()
{
  ref_count--;
  if (ref_count <= 0) {
    delete this;
    return -1;
  }
  return ref_count;
}

SharedXmlInputSource* SharedXmlInputSource::getSharedInputSource(const UnicodeString& path)
{
  if (isHash == nullptr) {
    isHash = new std::unordered_map<UnicodeString, SharedXmlInputSource*>();
  }

  const auto s = isHash->find(path);
  if (s != isHash->end()) {
    SharedXmlInputSource* sis = s->second;
    sis->addref();
    return sis;
  }

  auto* sis = new SharedXmlInputSource(path);
  isHash->try_emplace(path, sis);
  return sis;
}

SharedXmlInputSource::SharedXmlInputSource(const UnicodeString& path): source_path(path)
{

  is_open = false;
}

SharedXmlInputSource::~SharedXmlInputSource()
{
  // You don't need to delete an object that has been deleted from the array. We are already in the destructor.
  isHash->erase(source_path);
  if (isHash->empty()) {
    delete isHash;
    isHash = nullptr;
  }
}

int SharedXmlInputSource::getSize() const
{
  return mSize;
}

byte* SharedXmlInputSource::getSrc() const
{
  return mSrc.get();
}

void SharedXmlInputSource::open()
{
  if (!is_open) {
    std::ifstream f(colorer::Environment::to_filepath(&source_path), std::ios::in | std::ios::binary);
    if (!f.is_open()) {
      COLORER_LOG_ERROR("failed to open %", source_path);
      throw InputSourceException("failed to open " + source_path);
    }
    mSize = static_cast<int>(colorer::Environment::getFileSize(source_path));
    mSrc.reset(new byte[mSize]);
    f.read(reinterpret_cast<std::istream::char_type*>(mSrc.get()), mSize);
    f.close();
    is_open = true;
  }
}