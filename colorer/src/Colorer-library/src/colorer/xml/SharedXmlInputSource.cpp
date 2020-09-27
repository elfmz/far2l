#include <colorer/xml/SharedXmlInputSource.h>
#include <xercesc/util/BinFileInputStream.hpp>

std::unordered_map<SString, SharedXmlInputSource*>* SharedXmlInputSource::isHash = nullptr;

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

SharedXmlInputSource::SharedXmlInputSource(uXmlInputSource &source)
{
  ref_count = 1;
  input_source = std::move(source);
  auto stream = input_source->makeStream();
  std::unique_ptr<xercesc::BinFileInputStream> bfis(static_cast<xercesc::BinFileInputStream*>(stream));
  mSize = static_cast<XMLSize_t>(bfis->getSize());
  mSrc.reset(new XMLByte[mSize]);
  bfis->readBytes(mSrc.get(), mSize);
}

SharedXmlInputSource::~SharedXmlInputSource()
{
  CString d_id = CString(input_source->getInputSource()->getSystemId());
  //не нужно удалять объект, удаляемый из массива. мы и так уже в деструкторе
  isHash->erase(&d_id);
  if (isHash->size() == 0) {
    delete isHash;
    isHash = nullptr;
  }
}

SharedXmlInputSource* SharedXmlInputSource::getSharedInputSource(const XMLCh* path, const XMLCh* base)
{
  uXmlInputSource tempis = XmlInputSource::newInstance(path, base);

  if (isHash == nullptr) {
    isHash = new std::unordered_map<SString, SharedXmlInputSource*>();
  }

  CString d_id = CString(tempis->getInputSource()->getSystemId());
  auto s = isHash->find(d_id);
  if (s != isHash->end()) {
    SharedXmlInputSource* sis = s->second;
    sis->addref();
    return sis;
  } else {
    auto* sis = new SharedXmlInputSource(tempis);
    isHash->insert(std::make_pair(CString(sis->getInputSource()->getSystemId()), sis));
    return sis;
  }
}

xercesc::InputSource* SharedXmlInputSource::getInputSource() const
{
  return input_source->getInputSource();
}

