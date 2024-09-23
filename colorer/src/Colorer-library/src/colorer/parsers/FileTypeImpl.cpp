#include "colorer/parsers/FileTypeImpl.h"

#include <memory>

FileType::Impl::Impl(UnicodeString name, UnicodeString group, UnicodeString description)
    : name(std::move(name)), group(std::move(group)), description(std::move(description))
{
}

const UnicodeString& FileType::Impl::getName() const
{
  return name;
}

const UnicodeString& FileType::Impl::getGroup() const
{
  return group;
}

const UnicodeString& FileType::Impl::getDescription() const
{
  return description;
}

void FileType::Impl::setName(const UnicodeString* param_name)
{
  name = *param_name;
}

void FileType::Impl::setGroup(const UnicodeString* group_name)
{
  group = *group_name;
}

void FileType::Impl::setDescription(const UnicodeString* description_)
{
  description = *description_;
}

Scheme* FileType::Impl::getBaseScheme() const
{
  return baseScheme;
}

std::vector<UnicodeString> FileType::Impl::enumParams() const
{
  std::vector<UnicodeString> r;
  r.reserve(paramsHash.size());
  for (const auto& p : paramsHash) {
    r.push_back(p.first);
  }
  return r;
}

const UnicodeString* FileType::Impl::getParamDescription(const UnicodeString& param_name) const
{
  auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end() && tp->second.description) {
    return tp->second.description.get();
  }
  return nullptr;
}

const UnicodeString* FileType::Impl::getParamValue(const UnicodeString& param_name) const
{
  auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    if (tp->second.user_value) {
      return tp->second.user_value.get();
    }
    return &tp->second.value;
  }
  return nullptr;
}

int FileType::Impl::getParamValueInt(const UnicodeString& param_name, int def) const
{
  int val = def;
  auto param_value = getParamValue(param_name);
  if (param_value && param_value->length() > 0) {
    auto param_str = UStr::to_stdstr(param_value);
    try {
      val = std::stoi(param_str, nullptr);
    } catch (std::exception&) {
      COLORER_LOG_ERROR("Error parse param % with value % to integer number", param_name,
                    param_str);
    }
  }
  return val;
}

const UnicodeString* FileType::Impl::getParamDefaultValue(const UnicodeString& param_name) const
{
  auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    return &tp->second.value;
  }
  return nullptr;
}

const UnicodeString* FileType::Impl::getParamUserValue(const UnicodeString& param_name) const
{
  auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end() && tp->second.user_value) {
    return tp->second.user_value.get();
  }
  return nullptr;
}

TypeParameter& FileType::Impl::addParam(const UnicodeString& param_name, const UnicodeString& value)
{
  auto it = paramsHash.emplace(param_name, TypeParameter(param_name, value));
  return it.first->second;
}

void FileType::Impl::setParamValue(const UnicodeString& param_name, const UnicodeString* value)
{
  auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    if (value) {
      tp->second.user_value = std::make_unique<UnicodeString>(*value);;
    }
    else {
      tp->second.user_value.reset();
    }
  }
  else {
    throw FileTypeException("Don`t set value " + *value + " for parameter \"" + param_name +
                            "\". Parameter not exists.");
  }
}

void FileType::Impl::setParamDefaultValue(const UnicodeString& param_name,
                                          const UnicodeString* value)
{
  auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    if (value) {
      tp->second.value = *value;
    }
    else {
      throw FileTypeException("Don`t set null value for parameter \"" + param_name + "\"");
    }
  }
  else {
    throw FileTypeException("Don`t set value " + *value + " for parameter \"" + param_name +
                            "\". Parameter not exists.");
  }
}

void FileType::Impl::setParamUserValue(const UnicodeString& param_name, const UnicodeString* value)
{
  setParamValue(param_name, value);
}

void FileType::Impl::setParamDescription(const UnicodeString& param_name,
                                         const UnicodeString* t_description)
{
  auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    tp->second.description = std::make_unique<UnicodeString>(*t_description);
  }
  else {
    throw FileTypeException("Don`t set value " + *t_description +
                            " for description of parameter \"" + param_name +
                            "\". Parameter not exists.");
  }
}

size_t FileType::Impl::getParamCount() const
{
  return paramsHash.size();
}

double FileType::Impl::getPriority(const UnicodeString* fileName,
                                   const UnicodeString* fileContent) const
{
  SMatches match {};
  double cur_prior {0};
  for (auto const& ftc : chooserVector) {
    if (ftc.isFileName() && fileName != nullptr && ftc.getRE()->parse(fileName, &match)) {
      cur_prior += ftc.getPriority();
    }
    else if (ftc.isFileContent() && fileContent != nullptr &&
             ftc.getRE()->parse(fileContent, &match)) {
      cur_prior += ftc.getPriority();
    }
  }
  return cur_prior;
}
