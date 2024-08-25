#ifndef _COLORER_HRCLIBRARYIMPL_H_
#define _COLORER_HRCLIBRARYIMPL_H_

#include <unordered_map>
#include "colorer/HrcLibrary.h"
#include "colorer/cregexp/cregexp.h"
#include "colorer/parsers/SchemeImpl.h"
#include "colorer/xml/XMLNode.h"
#include "colorer/xml/XmlInputSource.h"

class FileType;

/** Implementation of HrcLibrary.
    Reads and mantains HRC database of syntax rules,
    used by TextParser implementations to make
    realtime text syntax parsing.
    @ingroup colorer_parsers
*/
class HrcLibrary::Impl
{
 public:
  Impl();
  ~Impl();

  void loadSource(XmlInputSource* is);
  void loadFileType(FileType* filetype);
  FileType* getFileType(const UnicodeString* name);
  FileType* enumerateFileTypes(unsigned int index);
  FileType* chooseFileType(const UnicodeString* fileName, const UnicodeString* firstLine, int typeNo = 0);
  size_t getFileTypesCount();

  size_t getRegionCount();
  const Region* getRegion(unsigned int id);
  const Region* getRegion(const UnicodeString* name);

 protected:
  enum class QualifyNameType { QNT_DEFINE, QNT_SCHEME, QNT_ENTITY };

  // types and packages
  std::unordered_map<UnicodeString, FileType*> fileTypeHash;
  // only types
  std::vector<FileType*> fileTypeVector;

  std::unordered_map<UnicodeString, SchemeImpl*> schemeHash;
  std::unordered_map<UnicodeString, int> disabledSchemes;

  std::vector<const Region*> regionNamesVector;
  std::unordered_map<UnicodeString, const Region*> regionNamesHash;
  std::unordered_map<UnicodeString, UnicodeString*> schemeEntitiesHash;

  FileType* current_parse_type = nullptr;
  XmlInputSource* current_input_source = nullptr;
  bool structureChanged = false;
  bool updateStarted = false;

  void unloadFileType(const FileType* filetype);

  void parseHRC(const XmlInputSource& is);
  void parseHrcBlock(const XMLNode& elem);
  void addPrototype(const XMLNode& elem);
  void parsePrototypeBlock(const XMLNode& elem, FileType* current_parse_prototype);
  void addPrototypeLocation(const XMLNode& elem, FileType* current_parse_prototype);
  void addPrototypeDetectParam(const XMLNode& elem, FileType* current_parse_prototype);
  void addPrototypeParameters(const XMLNode& elem, FileType* current_parse_prototype);
  void addType(const XMLNode& elem);
  void parseTypeBlock(const XMLNode& elem);
  void addTypeRegion(const XMLNode& elem);
  void addTypeEntity(const XMLNode& elem);
  void addTypeImport(const XMLNode& elem);

  void addScheme(const XMLNode& elem);
  void parseSchemeBlock(SchemeImpl* scheme, const XMLNode& elem);
  void addSchemeInherit(SchemeImpl* scheme, const XMLNode& elem);
  void addSchemeRegexp(SchemeImpl* scheme, const XMLNode& elem);
  void addSchemeBlock(SchemeImpl* scheme, const XMLNode& elem);
  void parseSchemeKeywords(SchemeImpl* scheme, const XMLNode& elem);
  size_t getSchemeKeywordsCount(const XMLNode& elem);
  void addSchemeKeyword(const XMLNode& elem, const SchemeImpl* scheme, const SchemeNodeKeywords* scheme_node,
                        const Region* region, KeywordInfo::KeywordType keyword_type);
  void loadBlockRegions(SchemeNodeBlock* node, const XMLNode& el);
  void loadRegions(SchemeNodeBlock* node, const XMLNode* el, bool start_element);
  void loadRegexpRegions(SchemeNodeRegexp* node, const XMLNode& el);

  uUnicodeString qualifyOwnName(const UnicodeString& name);
  bool checkNameExist(const UnicodeString* name, FileType* parseType, QualifyNameType qntype, bool logErrors);
  uUnicodeString qualifyForeignName(const UnicodeString* name, QualifyNameType qntype, bool logErrors);

  void updateLinks();
  void updateSchemeLink(uUnicodeString& scheme_name, SchemeImpl** scheme_impl, byte scheme_type,
                        const SchemeImpl* current_scheme);
  uUnicodeString useEntities(const UnicodeString* name);
  const Region* getNCRegion(const XMLNode* elem, const UnicodeString& tag);
  const Region* getNCRegion(const UnicodeString* name, bool logErrors);
  void loopSchemeKeywords(const XMLNode& elem, const SchemeImpl* scheme,
                          const std::unique_ptr<SchemeNodeKeywords>& scheme_node, const Region* region);
};

#endif
