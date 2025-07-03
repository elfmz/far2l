#ifndef COLORER_HRCLIBRARY_H
#define COLORER_HRCLIBRARY_H

#include "colorer/Exception.h"
#include "colorer/FileType.h"
#include "colorer/Region.h"
#include "colorer/common/spimpl.h"
#include "colorer/xml/XmlInputSource.h"

/** Informs application about internal HRC parsing problems.
 */
class HrcLibraryException : public Exception
{
 public:
  explicit HrcLibraryException(const UnicodeString& msg) noexcept : Exception("[HrcLibraryException] " + msg) {}
};

/** HrcLibrary class.
    Defines basic operations of loading and accessing HRC information.
*/
class HrcLibrary
{
 public:
  HrcLibrary();

  /** Loads the contents of the HRC file from the passed XmlInputSource.
   * All the elements included in the file are fully loaded: prototype, package, type.
   * If the file does not contain a type definition, then before further use of this type,
   * it must be loaded using the #loadFileType() method.
   * @param is XmlInputSource stream of HRC file
   */
  void loadSource(XmlInputSource* is);

  /** Loads the contents of the HRC file from the passed XmlInputSource.
   * Only the prototype and common/external packages are loaded.
   * If the file does not contain a type definition, then before further use of this type,
   * it must be loaded using the #loadFileType() method.
   * @param is XmlInputSource stream of HRC file
   */
  void loadProtoTypes(XmlInputSource* is);

  void loadFileType(FileType* filetype);

  void loadHrcSettings(const XmlInputSource& is);

  /** Enumerates sequentially all prototypes
      @param index index of type.
      @return Requested type, or null, if #index is too big
  */
  FileType* enumerateFileTypes(unsigned int index);

  /** @param name Requested type name.
      @return File type, or null, there are no type with specified name.
  */
  FileType* getFileType(const UnicodeString* name);
  FileType* getFileType(const UnicodeString& name);

  /** Searches and returns the best type for specified file.
      This method uses fileName and firstLine parameters
      to perform selection of the best HRC type from database.
      @param fileName Name of file
      @param firstLine First line of this file, could be null
      @param typeNo Sequential number of type, if more than one type
                    satisfy these input parameters.
  */
  FileType* chooseFileType(const UnicodeString* fileName, const UnicodeString* firstLine, int typeNo = 0);

  size_t getFileTypesCount();

  /** Total number of declared regions
   */
  size_t getRegionCount();
  /** Returns region by internal id
   */
  const Region* getRegion(unsigned int id);
  /** Returns region by name
      @note Also loads referred type, if it is not yet loaded.
  */
  const Region* getRegion(const UnicodeString* name);

 private:
  class Impl;

  spimpl::unique_impl_ptr<Impl> pimpl;
};

#endif  // COLORER_HRCLIBRARY_H
