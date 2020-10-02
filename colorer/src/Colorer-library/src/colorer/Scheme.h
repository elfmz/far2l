#ifndef _COLORER_SCHEME_H_
#define _COLORER_SCHEME_H_

#include <colorer/Common.h>
class FileType;

/** HRC Scheme instance information.
    Used in RegionHandler calls to pass curent region's scheme.
    @ingroup colorer
*/
class Scheme
{
public:
  /** Full qualified schema name.
  */
  virtual const String* getName() const = 0;
  /** Returns reference to FileType, this scheme belongs to.
  */
  virtual FileType* getFileType() const = 0;
protected:
  Scheme() {};
  virtual ~Scheme() {};
};

#endif


