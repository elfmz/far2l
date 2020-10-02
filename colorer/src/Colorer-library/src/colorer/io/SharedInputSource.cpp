#include<colorer/io/SharedInputSource.h>

std::unordered_map<SString, SharedInputSource*>* SharedInputSource::isHash = nullptr;

SharedInputSource::SharedInputSource(InputSource* source)
{
  is = source;
  stream = nullptr;
  ref_count = 1;
}

SharedInputSource::~SharedInputSource()
{
  isHash->erase(is->getLocation());
  if (isHash->size() == 0) {
    delete isHash;
    isHash = nullptr;
  }
  delete is;
}


SharedInputSource* SharedInputSource::getInputSource(const String* path, InputSource* base)
{
  InputSource* tempis = InputSource::newInstance(path, base);

  if (isHash == nullptr) {
    isHash = new std::unordered_map<SString, SharedInputSource*>();
  }

  SharedInputSource* sis = nullptr;
  auto s = isHash->find(tempis->getLocation());
  if (s != isHash->end()) {
    sis = s->second;
  }

  if (sis == nullptr) {
    sis = new SharedInputSource(tempis);
    std::pair<SString, SharedInputSource*> pp(tempis->getLocation(), sis);
    isHash->emplace(pp);

    return sis;
  } else {
    delete tempis;
  }

  sis->addref();
  return sis;
}

