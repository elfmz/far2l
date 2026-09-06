#include "colorer/parsers/FileTypeImpl.h"
#include <memory>

FileType::Impl::Impl(UnicodeString l_name, UnicodeString l_group, UnicodeString l_description)
    : name(std::move(l_name)), group(std::move(l_group)), description(std::move(l_description))
{
  if (name.isEmpty()) {
    throw FileTypeException("The file type name must not be empty");
  }
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

void FileType::Impl::setName(const UnicodeString& param_name)
{
  if (param_name.isEmpty()) {
    throw FileTypeException("The file type name must not be empty");
  }
  name = param_name;
}

void FileType::Impl::setGroup(const UnicodeString& group_name)
{
  group = group_name;
}

void FileType::Impl::setDescription(const UnicodeString& value)
{
  description = value;
}

Scheme* FileType::Impl::getBaseScheme() const
{
  return baseScheme;
}

std::vector<UnicodeString> FileType::Impl::enumParams() const
{
  std::vector<UnicodeString> r;
  r.reserve(paramsHash.size());
  for (const auto& [key, value] : paramsHash) {
    r.push_back(key);
  }
  return r;
}

const UnicodeString* FileType::Impl::getParamDescription(const UnicodeString& param_name) const
{
  const auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end() && tp->second.description) {
    return tp->second.description.get();
  }
  return nullptr;
}

const UnicodeString* FileType::Impl::getParamValue(const UnicodeString& param_name) const
{
  const auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    if (tp->second.user_value) {
      return tp->second.user_value.get();
    }
    return &tp->second.value;
  }
  return nullptr;
}

int FileType::Impl::getParamValueInt(const UnicodeString& param_name, const int def) const
{
  int val = def;
  const auto* param_value = getParamValue(param_name);
  if (param_value && param_value->length() > 0) {
    auto param_str = UStr::to_stdstr(param_value);
    try {
      val = std::stoi(param_str, nullptr);
    } catch (std::exception&) {
      COLORER_LOG_ERROR("Error parse param '%' with value '%' to integer number", param_name, param_str);
    }
  }
  return val;
}

int FileType::Impl::getParamValueHex(const UnicodeString& param_name, int def_value) const
{
  int val = def_value;
  const auto* param_value = getParamValue(param_name);
  if (param_value && param_value->length() > 0) {
    unsigned int converted = 0;
    bool result = UStr::HexToUInt(*param_value, &converted);
    if (result) {
      val = (int) converted;
    }
    else {
      COLORER_LOG_ERROR("Error convert param '%' with value '%' to hex number", param_name, param_value);
    }
  }
  return val;
}

const UnicodeString* FileType::Impl::getParamDefaultValue(const UnicodeString& param_name) const
{
  const auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    return &tp->second.value;
  }
  return nullptr;
}

const UnicodeString* FileType::Impl::getParamUserValue(const UnicodeString& param_name) const
{
  const auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end() && tp->second.user_value) {
    return tp->second.user_value.get();
  }
  return nullptr;
}

TypeParameter& FileType::Impl::addParam(const UnicodeString& param_name, const UnicodeString& value)
{
  auto [fst, snd] = paramsHash.try_emplace(param_name, param_name, value);
  return fst->second;
}

void FileType::Impl::setParamValue(const UnicodeString& param_name, const UnicodeString* value)
{
  const auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    if (value) {
      tp->second.user_value = std::make_unique<UnicodeString>(*value);
    }
    else {
      tp->second.user_value.reset();
    }
  }
  else {
    throw FileTypeException("Don`t set new value for parameter \"" + param_name + "\". Parameter not exists.");
  }
}

void FileType::Impl::setParamDefaultValue(const UnicodeString& param_name, const UnicodeString* value)
{
  if (!value) {
    throw FileTypeException("You can`t set the default value to null for the parameter \"" + param_name + "\"");
  }
  const auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    tp->second.value = *value;
  }
  else {
    throw FileTypeException("Don`t set new value for parameter \"" + param_name + "\". Parameter not exists.");
  }
}

void FileType::Impl::setParamUserValue(const UnicodeString& param_name, const UnicodeString* value)
{
  setParamValue(param_name, value);
}

void FileType::Impl::setParamDescription(const UnicodeString& param_name, const UnicodeString* value)
{
  const auto tp = paramsHash.find(param_name);
  if (tp != paramsHash.end()) {
    if (value) {
      tp->second.description = std::make_unique<UnicodeString>(*value);
    }
    else {
      tp->second.description.reset();
    }
  }
  else {
    throw FileTypeException("Don`t set value for description of parameter \"" + param_name +
                            "\". Parameter not exists.");
  }
}

size_t FileType::Impl::getParamCount() const
{
  return paramsHash.size();
}

double FileType::Impl::getPriority(const UnicodeString* fileName, const UnicodeString* fileContent) const
{
  double cur_prior {0};
  for (auto const& ftc : chooserVector) {
    if (ftc.isFileName()) {
      cur_prior += ftc.calcPriority(fileName);
    }
    else if (ftc.isFileContent()) {
      cur_prior += ftc.calcPriority(fileContent);
    }
  }
  return cur_prior;
}
