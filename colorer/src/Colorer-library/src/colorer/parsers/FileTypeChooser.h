#ifndef COLORER_FILETYPECHOOSER_H
#define COLORER_FILETYPECHOOSER_H

#include "colorer/cregexp/cregexp.h"

/** Stores regular expressions of filename and firstline
    elements and helps to detect file type.
    @ingroup colorer_parsers
*/
class FileTypeChooser
{
 public:
  enum class ChooserType { CT_FILENAME, CT_FIRSTLINE };

  /** Creates choose entry.
      @param type If 0 - filename RE, if 1 - firstline RE
      @param prior Priority of this rule
      @param re Associated regular expression
  */
  FileTypeChooser(ChooserType type, double priority, CRegExp* re);
  /** Returns type of chooser */
  [[nodiscard]] bool isFileName() const;
  /** Returns type of chooser */
  [[nodiscard]] bool isFileContent() const;
  /** Returns chooser priority */
  [[nodiscard]] double getPriority() const;
  /** Returns associated regular expression */
  [[nodiscard]] CRegExp* getRE() const;

 private:
  ChooserType m_type;
  double m_priority;
  std::unique_ptr<CRegExp> m_reg_matcher;
};

#endif  // COLORER_FILETYPECHOOSER_H
