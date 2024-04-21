#ifndef COLORER_HRCLIBRARY_H
#define COLORER_HRCLIBRARY_H

#include "colorer/Exception.h"
#include "colorer/FileType.h"
#include "colorer/Region.h"
#include "colorer/common/spimpl.h"
#include "colorer/xml/XmlInputSource.h"

/** Informs application about internal HRC parsing problems.
    @ingroup colorer
*/
class HrcLibraryException : public Exception
{
 public:
  explicit HrcLibraryException(const UnicodeString& msg) noexcept
      : Exception("[HrcLibraryException] " + msg)
  {
  }
};

/** Abstract template of HrcLibrary class implementation.
    Defines basic operations of loading and accessing
    HRC information.
    @ingroup colorer
*/
class HrcLibrary
{
 public:
  HrcLibrary();

  /** Loads HRC from specified InputSource stream.
      Referred HRC file can contain prototypes and
      real types definitions. If it contains just prototype definition,
      real type load must be performed before using with #loadType() method
      @param is InputSource stream of HRC file
  */
  void loadSource(XmlInputSource* is);
  void loadFileType(FileType* filetype);

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
  FileType* chooseFileType(const UnicodeString* fileName, const UnicodeString* firstLine,
                           int typeNo = 0);

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
