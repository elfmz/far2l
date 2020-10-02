#include <colorer/parsers/FileTypeImpl.h>
#include <colorer/unicode/UnicodeTools.h>

#include <memory>

FileTypeImpl::FileTypeImpl(HRCParserImpl* hrcParser): name(nullptr), group(nullptr), description(nullptr)
{
  this->hrcParser = hrcParser;
  protoLoaded = type_loaded = loadDone = load_broken = input_source_loading = false;
  isPackage = false;
  baseScheme = nullptr;
  inputSource = nullptr;
}

FileTypeImpl::~FileTypeImpl(){
  for(auto it : chooserVector){
    delete it;
  }
  chooserVector.clear();

  for(const auto& it: paramsHash){
    delete it.second;
  }
  paramsHash.clear();

  importVector.clear();
}

Scheme* FileTypeImpl::getBaseScheme() {
  if (!type_loaded) hrcParser->loadFileType(this);
  return baseScheme;
}

std::vector<SString> FileTypeImpl::enumParams() const {
  std::vector<SString> r;
  r.reserve(paramsHash.size());
  for (const auto & p : paramsHash)
  {
	  r.push_back(p.first);
  }
  return r;
}

const String* FileTypeImpl::getParamDescription(const String &name) const{
  auto tp = paramsHash.find(name);
  if (tp != paramsHash.end()) return tp->second->description.get();
  return nullptr;
}

const String *FileTypeImpl::getParamValue(const String &name) const{
  auto tp = paramsHash.find(name);
  if (tp != paramsHash.end()){
    if(tp->second->user_value) return tp->second->user_value.get();
    return tp->second->default_value.get();
  }
  return nullptr;
}

int FileTypeImpl::getParamValueInt(const String &name, int def) const{
  int val = def;
  UnicodeTools::getNumber(getParamValue(name), &val);
  return val;
}

const String* FileTypeImpl::getParamDefaultValue(const String &name) const{
  auto tp = paramsHash.find(name);
  if (tp !=paramsHash.end()) {
    return tp->second->default_value.get();
  }
  return nullptr;
}

const String* FileTypeImpl::getParamUserValue(const String &name) const{
  auto tp = paramsHash.find(name);
  if (tp !=paramsHash.end()) {
    return tp->second->user_value.get();
  }
  return nullptr;
}

TypeParameter* FileTypeImpl::addParam(const String *name){
  auto* tp = new TypeParameter;
  tp->name.reset(new SString(name));
  std::pair<SString, TypeParameter*> pp(name, tp);
  paramsHash.emplace(pp);
  return tp;
}

void FileTypeImpl::setParamValue(const String &name, const String *value){
  auto tp = paramsHash.find(name);
  if (tp != paramsHash.end()) {
    if (value) {
      tp->second->user_value.reset(new SString(value));
    }
    else{
      tp->second->user_value.reset();
    }
  }
}

void FileTypeImpl::setParamDefaultValue(const String &name, const String *value){
  auto tp = paramsHash.find(name);
  if (tp != paramsHash.end()) {
    tp->second->default_value.reset(new SString(value));
  }
}

void FileTypeImpl::setParamUserValue(const String &name, const String *value){
  setParamValue(name,value);
}

void FileTypeImpl::setParamDescription(const String &name, const String *value){
  auto tp = paramsHash.find(name);
  if (tp != paramsHash.end()) {
    tp->second->description.reset(new SString(value));
  }
}

void FileTypeImpl::removeParamValue(const String &name){
  paramsHash.erase(name);
}

size_t FileTypeImpl::getParamCount() const{
  return paramsHash.size();
}

size_t FileTypeImpl::getParamUserValueCount() const{
  size_t count=0;
  for (const auto & it : paramsHash){
    if (it.second->user_value) count++;
  }
  return count;
}

double FileTypeImpl::getPriority(const String *fileName, const String *fileContent) const{
  SMatches match;
  double cur_prior = 0;
  for(auto ftc : chooserVector){
    if (fileName != nullptr && ftc->isFileName() && ftc->getRE()->parse(fileName, &match))
      cur_prior += ftc->getPriority();
    if (fileContent != nullptr && ftc->isFileContent() && ftc->getRE()->parse(fileContent, &match))
      cur_prior += ftc->getPriority();
  }
  return cur_prior;
}

