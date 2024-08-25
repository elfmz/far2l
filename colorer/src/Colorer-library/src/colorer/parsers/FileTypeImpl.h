#ifndef COLORER_FILETYPEIMPL_H
#define COLORER_FILETYPEIMPL_H

#include <unordered_map>
#include <vector>
#include "colorer/FileType.h"
#include "colorer/HrcLibrary.h"
#include "colorer/parsers/FileTypeChooser.h"
#include "colorer/parsers/SchemeImpl.h"
#include "colorer/xml/XmlInputSource.h"

/* structure for storing data of scheme parameter*/
class TypeParameter
{
 public:
  TypeParameter(UnicodeString t_name, UnicodeString t_value)
      : name(std::move(t_name)), value(std::move(t_value)) {};

  /* parameter name*/
  UnicodeString name;
  /* parameter description*/
  uUnicodeString description;
  /* default value*/
  UnicodeString value;
  /* user value*/
  uUnicodeString user_value;
};

/**
 * File Type storage implementation.
 * Contains different attributes of HRC file type.
 * @ingroup colorer_parsers
 */
class FileType::Impl
{
 public:
  Impl(UnicodeString name, UnicodeString group, UnicodeString description);

  [[nodiscard]] const UnicodeString& getName() const;
  [[nodiscard]] const UnicodeString& getGroup() const;
  [[nodiscard]] const UnicodeString& getDescription() const;

  void setName(const UnicodeString* param_name);
  void setGroup(const UnicodeString* group_name);
  void setDescription(const UnicodeString* description);

  [[nodiscard]] const UnicodeString* getParamValue(const UnicodeString& param_name) const;
  [[nodiscard]] const UnicodeString* getParamDefaultValue(const UnicodeString& param_name) const;
  [[nodiscard]] const UnicodeString* getParamUserValue(const UnicodeString& param_name) const;
  [[nodiscard]] const UnicodeString* getParamDescription(const UnicodeString& param_name) const;
  [[nodiscard]] int getParamValueInt(const UnicodeString& param_name, int def) const;

  void setParamValue(const UnicodeString& param_name, const UnicodeString* value);
  void setParamDefaultValue(const UnicodeString& param_name, const UnicodeString* value);
  void setParamUserValue(const UnicodeString& param_name, const UnicodeString* value);
  void setParamDescription(const UnicodeString& param_name, const UnicodeString* description);

  [[nodiscard]] std::vector<UnicodeString> enumParams() const;
  [[nodiscard]] size_t getParamCount() const;

  TypeParameter& addParam(const UnicodeString& param_name, const UnicodeString& value);

  [[nodiscard]] Scheme* getBaseScheme() const;
  /**
   * Returns total priority, accordingly to all it's
   * choosers (filename and firstline choosers).
   * All <code>fileContent</code> RE's are tested only if priority of previously
   * computed <code>fileName</code> RE's is more, than zero.
   * @param fileName String representation of file name (without path).
   *        If null, method skips filename matching, and starts directly
   *        with fileContent matching.
   * @param fileContent Some part of file's starting content (first line,
   *        for example). If null, skipped.
   * @return Computed total filetype priority.
   */
  double getPriority(const UnicodeString* fileName, const UnicodeString* fileContent) const;

  /// is type component loaded
  bool type_loading {false};
  /// is type references fully resolved
  bool loadDone {false};
  /// is initial type load failed
  bool load_broken {false};
  /// is this IS loading was started
  bool input_source_loading {false};

  UnicodeString name;
  UnicodeString group;
  UnicodeString description;
  bool isPackage {false};
  SchemeImpl* baseScheme {nullptr};

  std::vector<FileTypeChooser> chooserVector;
  std::unordered_map<UnicodeString, TypeParameter> paramsHash;
  std::vector<UnicodeString> importVector;
  uXmlInputSource inputSource;
};

#endif  // COLORER_FILETYPEIMPL_H
