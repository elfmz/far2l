#ifndef COLORER_INPUTSOURCE_H
#define COLORER_INPUTSOURCE_H

#include "colorer/Common.h"
#include "colorer/Exception.h"

namespace colorer {
/** Abstract byte input source.
Supports derivation of input source,
using specified relative of absolute paths.
@ingroup common_io
*/
class InputSource
{
 public:
  /** Current stream location
   */
  [[nodiscard]]
  virtual const UnicodeString* getLocation() const = 0;

  /** Opens stream and returns array of readed bytes.
  @throw InputSourceException If some IO-errors occurs.
  */
  virtual const byte* openStream() = 0;
  /** Explicitly closes stream and frees all resources.
  Stream could be reopened.
  @throw InputSourceException If stream is already closed.
  */
  virtual void closeStream() = 0;
  /** Return length of opened stream
  @throw InputSourceException If stream is closed.
  */
  [[nodiscard]]
  virtual int length() const = 0;

  /** Tries statically create instance of InputSource object,
  according to passed @c path string.
  @param path Could be relative file location, absolute
  file, http uri, jar uri.
  */
  static InputSource* newInstance(const UnicodeString* path);

  /** Statically creates instance of InputSource object,
  possibly based on parent source stream.
  @param base Base stream, used to resolve relative paths.
  @param path Could be relative file location, absolute
  file, http uri, jar uri.
  */
  static InputSource* newInstance(const UnicodeString* path, InputSource* base);

  /** Returns new String, created from linking of
  @c basePath and @c relPath parameters.
  @param basePath Base path. Can be relative or absolute.
  @param relPath Relative path, used to append to basePath
  and construct new path. Can be @b absolute
  */
  static UnicodeString* getAbsolutePath(const UnicodeString* basePath, const UnicodeString* relPath);

  /** Checks, if passed path relative or not.
   */
  static bool isRelative(const UnicodeString* path);

  /** Creates inherited InputSource with the same type
  relatively to the current.
  @param relPath Relative URI part.
  */
  virtual InputSource* createRelative([[maybe_unused]] const UnicodeString* relPath) { return nullptr; };

  virtual ~InputSource() = default;
  InputSource(InputSource&&) = delete;
  InputSource(const InputSource&) = delete;
  InputSource& operator=(const InputSource&) = delete;
  InputSource& operator=(InputSource&&) = delete;

 protected:
  InputSource() = default;
};

}  // namespace colorer
#endif  // COLORER_INPUTSOURCE_H
