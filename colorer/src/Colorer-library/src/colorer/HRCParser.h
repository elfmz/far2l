#ifndef _COLORER_HRCPARSER_H_
#define _COLORER_HRCPARSER_H_

#include <colorer/FileType.h>
#include <colorer/Region.h>
#include <colorer/xml/XmlInputSource.h>


/** Informs application about internal HRC parsing problems.
    @ingroup colorer
*/
class HRCParserException : public Exception
{
public:
  HRCParserException() noexcept : Exception("[HRCParserException] ") {};
  HRCParserException(const String &msg) noexcept : HRCParserException()
  {
    what_str.append(msg);
  }
};


/** Abstract template of HRCParser class implementation.
    Defines basic operations of loading and accessing
    HRC information.
    @ingroup colorer
*/
class HRCParser
{
public:
  /** Loads HRC from specified InputSource stream.
      Referred HRC file can contain prototypes and
      real types definitions. If it contains just prototype definition,
      real type load must be performed before using with #loadType() method
      @param is InputSource stream of HRC file
  */
  virtual void loadSource(XmlInputSource *is) = 0;

  /** Enumerates sequentially all prototypes
      @param index index of type.
      @return Requested type, or null, if #index is too big
  */
  virtual FileType *enumerateFileTypes(int index) = 0;

  /** @param name Requested type name.
      @return File type, or null, there are no type with specified name.
  */
  virtual FileType *getFileType(const String *name) = 0;

  /** Searches and returns the best type for specified file.
      This method uses fileName and firstLine parameters
      to perform selection of the best HRC type from database.
      @param fileName Name of file
      @param firstLine First line of this file, could be null
      @param typeNo Sequential number of type, if more than one type
                    satisfy these input parameters.
  */
  virtual FileType *chooseFileType(const String *fileName, const String *firstLine, int typeNo = 0) = 0;

  virtual size_t getFileTypesCount() = 0;

  /** Total number of declared regions
  */
  virtual size_t getRegionCount() = 0;
  /** Returns region by internal id
  */
  virtual const Region *getRegion(int id) = 0;
  /** Returns region by name
      @note Also loads referred type, if it is not yet loaded.
  */
  virtual const Region *getRegion(const String *name) = 0;

  /** HRC base version.
      Usually this is the 'version' attribute of 'hrc' element
      of the first loaded HRC file.
  */
  virtual const String *getVersion() = 0;

  virtual ~HRCParser(){};
protected:
  HRCParser(){};
};


#endif

