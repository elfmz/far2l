#ifndef _COLORER_FILETYPEIMPL_H_
#define _COLORER_FILETYPEIMPL_H_

#include <vector>
#include <unordered_map>
#include <colorer/parsers/HRCParserImpl.h>
#include <colorer/parsers/FileTypeChooser.h>

/* structure for storing data of scheme parameter*/
class TypeParameter {
public:
  TypeParameter(): name(nullptr), description(nullptr), default_value(nullptr), user_value(nullptr) {};
  ~TypeParameter() {}

  /* parameter name*/
  UString name;
  /* parameter description*/
  UString description;
  /* default value*/
  UString default_value;
  /* user value*/
  UString user_value;
};

/**
 * File Type storage implementation.
 * Contains different attributes of HRC file type.
 * @ingroup colorer_parsers
 */
class FileTypeImpl : public FileType
{
  friend class HRCParserImpl;
  friend class TextParserImpl;
public:
  const String *getName() const;
  const String *getGroup() const;
  const String *getDescription() const;

  void setName(const String *name_);
  void setGroup(const String *group_);
  void setDescription(const String *description_);

  const String *getParamValue(const String &name) const;
  const String *getParamDefaultValue(const String &name) const;
  const String *getParamUserValue(const String &name) const;
  const String *getParamDescription(const String &name) const;
  int getParamValueInt(const String &name, int def) const;

  void setParamValue(const String &name, const String *value);
  void setParamDefaultValue(const String &name, const String *value);
  void setParamUserValue(const String &name, const String *value);
  void setParamDescription(const String &name, const String *value);

  std::vector<SString> enumParams() const;
  size_t getParamCount() const;
  size_t getParamUserValueCount() const;

  TypeParameter* addParam(const String *name);
  void removeParamValue(const String &name);

  Scheme *getBaseScheme();
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
  double getPriority(const String *fileName, const String *fileContent) const;
protected:
  /// is prototype component loaded
  bool protoLoaded;
  /// is type component loaded
  bool type_loaded;
  /// is type references fully resolved
  bool loadDone;
  /// is initial type load failed
  bool load_broken;
  /// is this IS loading was started
  bool input_source_loading;

  UString name;
  UString group;
  UString description;
  bool isPackage;
  HRCParserImpl *hrcParser;
  SchemeImpl *baseScheme;

  std::vector<FileTypeChooser*> chooserVector;
  std::unordered_map<SString, TypeParameter*> paramsHash;
  std::vector<UString> importVector;
  uXmlInputSource inputSource;

  FileTypeImpl(HRCParserImpl *hrcParser);
  ~FileTypeImpl();
};

inline const String* FileTypeImpl::getName() const{
  return name.get();
}

inline const String* FileTypeImpl::getGroup() const{
  return group.get();
}

inline const String* FileTypeImpl::getDescription() const{
  return description.get();
}

inline void FileTypeImpl::setName(const String *name_) {
  name.reset(new SString(name_));
}

inline void FileTypeImpl::setGroup(const String *group_) {
  group.reset(new SString(group_));
}

inline void FileTypeImpl::setDescription(const String *description_) {
  description.reset(new SString(description_));
}
#endif

