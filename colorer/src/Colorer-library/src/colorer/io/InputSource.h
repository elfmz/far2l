#ifndef _COLORER_INPUTSOURCE_H_
#define _COLORER_INPUTSOURCE_H_

#include<colorer/Common.h>
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
    virtual const String *getLocation() const = 0;

    /** Opens stream and returns array of readed bytes.
    @throw InputSourceException If some IO-errors occurs.
    */
    virtual const byte *openStream() = 0;
    /** Explicitly closes stream and frees all resources.
    Stream could be reopened.
    @throw InputSourceException If stream is already closed.
    */
    virtual void closeStream() = 0;
    /** Return length of opened stream
    @throw InputSourceException If stream is closed.
    */
    virtual int length() const = 0;

    /** Tries statically create instance of InputSource object,
    according to passed @c path string.
    @param path Could be relative file location, absolute
    file, http uri, jar uri.
    */
    static InputSource *newInstance(const String *path);

    /** Statically creates instance of InputSource object,
    possibly based on parent source stream.
    @param base Base stream, used to resolve relative paths.
    @param path Could be relative file location, absolute
    file, http uri, jar uri.
    */
    static InputSource *newInstance(const String *path, InputSource *base);

    /** Returns new String, created from linking of
    @c basePath and @c relPath parameters.
    @param basePath Base path. Can be relative or absolute.
    @param relPath Relative path, used to append to basePath
    and construct new path. Can be @b absolute
    */
    static String *getAbsolutePath(const String*basePath, const String*relPath);

    /** Checks, if passed path relative or not.
    */
    static bool isRelative(const String *path);

    /** Creates inherited InputSource with the same type
    relatively to the current.
    @param relPath Relative URI part.
    */
    virtual InputSource *createRelative(const String *relPath){ return nullptr;};

    virtual ~InputSource(){};
  protected:
    InputSource(){};
  };


  /** @deprecated I think deprecated class.
  @ingroup common_io
  */
  class MultipleInputSource{
  public:
    virtual bool hasMoreInput() const = 0;
    virtual InputSource *nextInput() const = 0;
    virtual const String *getLocation() const = 0;

    virtual ~MultipleInputSource(){};
  protected:
    MultipleInputSource(const String *basePath){};
  };
}
#endif

