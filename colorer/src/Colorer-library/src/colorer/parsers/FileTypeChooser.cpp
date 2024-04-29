#include "colorer/parsers/FileTypeChooser.h"

FileTypeChooser::FileTypeChooser(ChooserType type, double priority, CRegExp* re)
    : m_type {type}, m_priority {priority}, m_reg_matcher {re}
{
}

bool FileTypeChooser::isFileName() const
{
  return m_type == ChooserType::CT_FILENAME;
}

bool FileTypeChooser::isFileContent() const
{
  return m_type == ChooserType::CT_FIRSTLINE;
}

double FileTypeChooser::getPriority() const
{
  return m_priority;
}

CRegExp* FileTypeChooser::getRE() const
{
  return m_reg_matcher.get();
}