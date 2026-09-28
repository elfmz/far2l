#ifndef COLORER_SCHEME_H
#define COLORER_SCHEME_H

#include "colorer/Common.h"
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
  [[nodiscard]] virtual const UnicodeString* getName() const = 0;
  /** Returns reference to FileType, this scheme belongs to.
   */
  [[nodiscard]] virtual FileType* getFileType() const = 0;

  virtual ~Scheme() = default;
  Scheme(Scheme&&) = delete;
  Scheme(const Scheme&) = delete;
  Scheme& operator=(const Scheme&) = delete;
  Scheme& operator=(Scheme&&) = delete;

 protected:
  Scheme() = default;
};

#endif  // COLORER_SCHEME_H
