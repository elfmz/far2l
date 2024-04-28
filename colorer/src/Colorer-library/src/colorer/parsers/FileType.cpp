#include "colorer/FileType.h"
#include "colorer/parsers/FileTypeImpl.h"

FileType::FileType(UnicodeString name, UnicodeString group, UnicodeString description)
    : pimpl(
          spimpl::make_unique_impl<Impl>(std::move(name), std::move(group), std::move(description)))
{
}

const UnicodeString& FileType::getName() const
{
  return pimpl->getName();
}

const UnicodeString& FileType::getGroup() const
{
  return pimpl->getGroup();
}

const UnicodeString& FileType::getDescription() const
{
  return pimpl->getDescription();
}

Scheme* FileType::getBaseScheme()
{
  return pimpl->getBaseScheme();
}

std::vector<UnicodeString> FileType::enumParams() const
{
  return pimpl->enumParams();
}

const UnicodeString* FileType::getParamDescription(const UnicodeString& name) const
{
  return pimpl->getParamDescription(name);
}

const UnicodeString* FileType::getParamValue(const UnicodeString& name) const
{
  return pimpl->getParamValue(name);
}

const UnicodeString* FileType::getParamDefaultValue(const UnicodeString& name) const
{
  return pimpl->getParamDefaultValue(name);
}

int FileType::getParamValueInt(const UnicodeString& name, int def_value) const
{
  return pimpl->getParamValueInt(name, def_value);
}

void FileType::addParam(const UnicodeString* name, const UnicodeString* value)
{
  if (name == nullptr || value == nullptr) {
    throw FileTypeException("Parameter must have not empty name and value");
  }
  pimpl->addParam(*name, *value);
}

void FileType::addParam(const UnicodeString& name, const UnicodeString& value)
{
  pimpl->addParam(name, value);
}

size_t FileType::getParamCount() const
{
  return pimpl->getParamCount();
}

const UnicodeString* FileType::getParamUserValue(const UnicodeString& name_) const
{
  return pimpl->getParamUserValue(name_);
}

void FileType::setParamDescription(const UnicodeString& name_, const UnicodeString* value)
{
  pimpl->setParamDescription(name_, value);
}

void FileType::setParamDefaultValue(const UnicodeString& name_, const UnicodeString* value)
{
  pimpl->setParamDefaultValue(name_, value);
}

void FileType::setParamValue(const UnicodeString& name, const UnicodeString* value)
{
  pimpl->setParamValue(name, value);
}

void FileType::setName(const UnicodeString* name)
{
  pimpl->setName(name);
}

void FileType::setGroup(const UnicodeString* group)
{
  pimpl->setGroup(group);
}

void FileType::setDescription(const UnicodeString* description)
{
  pimpl->setDescription(description);
}

void FileType::setParamValue(const UnicodeString& name, const UnicodeString& value)
{
  pimpl->setParamValue(name, &value);
}
