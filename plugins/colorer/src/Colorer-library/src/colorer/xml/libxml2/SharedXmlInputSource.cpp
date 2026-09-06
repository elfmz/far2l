#include "colorer/xml/libxml2/SharedXmlInputSource.h"
#include <fstream>
#include "colorer/Exception.h"
#include "colorer/utils/Environment.h"

XmlJarCache* XmlJarCache::current_ = nullptr;

XmlJarCache::Current::Current(XmlJarCache& cache) : previous(current_)
{
  current_ = &cache;
}

XmlJarCache::Current::~Current()
{
  current_ = previous;
}

XmlJarCache& XmlJarCache::fallback()
{
  static XmlJarCache process_fallback;
  return process_fallback;
}

XmlJarCache& XmlJarCache::active()
{
  return current_ != nullptr ? *current_ : fallback();
}

XmlJarCache* XmlJarCache::bound()
{
  return current_;
}

SharedXmlInputSource* XmlJarCache::get(const UnicodeString& path)
{
  const auto existing = entries.find(path);
  if (existing != entries.end()) {
    existing->second->addref();
    return existing->second.get();
  }

  auto sis = std::unique_ptr<SharedXmlInputSource>(new SharedXmlInputSource(path));
  auto* const raw = sis.get();
  const auto [it, inserted] = entries.try_emplace(path, std::move(sis));
  if (!inserted) {
    it->second->addref();
    return it->second.get();
  }
  return raw;
}

int SharedXmlInputSource::addref()
{
  return ++ref_count;
}

int SharedXmlInputSource::delref()
{
  ref_count--;
  if (ref_count <= 0) {
    return -1;
  }
  return ref_count;
}

SharedXmlInputSource* SharedXmlInputSource::getSharedInputSource(const UnicodeString& path)
{
  return XmlJarCache::active().get(path);
}

SharedXmlInputSource::SharedXmlInputSource(const UnicodeString& path) : source_path(path)
{
  is_open = false;
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
