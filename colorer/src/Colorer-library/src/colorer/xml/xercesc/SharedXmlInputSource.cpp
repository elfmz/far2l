#include "colorer/xml/xercesc/SharedXmlInputSource.h"
#include <xercesc/util/BinFileInputStream.hpp>
#include "colorer/Exception.h"

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

SharedXmlInputSource::SharedXmlInputSource(uXercesXmlInputSource source)
{
  ref_count = 1;
  input_source = std::move(source);
  auto pStream = input_source->makeStream();
  // don`t use dynamic_cast, see https://github.com/colorer/Colorer-library/issues/32
  std::unique_ptr<xercesc::BinFileInputStream> bfis(
      static_cast<xercesc::BinFileInputStream*>(pStream));
  if (bfis == nullptr) {
    throw InputSourceException("can`t read " + input_source->getPath());
  }
  mSize = static_cast<XMLSize_t>(bfis->getSize());
  mSrc.reset(new XMLByte[mSize]);
  bfis->readBytes(mSrc.get(), mSize);
}

SharedXmlInputSource::~SharedXmlInputSource()
{
  // не нужно удалять объект, удаляемый из массива. мы и так уже в деструкторе
  isHash->erase(input_source->getPath());
  if (isHash->empty()) {
    delete isHash;
    isHash = nullptr;
  }
}

SharedXmlInputSource* SharedXmlInputSource::getSharedInputSource(const XMLCh* path,
                                                                 const XMLCh* base)
{
  uXercesXmlInputSource tempis = XercesXmlInputSource::newInstance(path, base);

  if (isHash == nullptr) {
    isHash = new std::unordered_map<UnicodeString, SharedXmlInputSource*>();
  }

  UnicodeString d_id = UnicodeString(tempis->getInputSource()->getSystemId());
  auto s = isHash->find(d_id);
  if (s != isHash->end()) {
    SharedXmlInputSource* sis = s->second;
    sis->addref();
    return sis;
  }
  else {
    auto* sis = new SharedXmlInputSource(std::move(tempis));
    isHash->emplace(std::make_pair(d_id, sis));
    return sis;
  }
}

xercesc::InputSource* SharedXmlInputSource::getInputSource() const
{
  return input_source->getInputSource();
}
