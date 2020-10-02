#ifndef _COLORER_HTTPINPUTSOURCE_H_
#define _COLORER_HTTPINPUTSOURCE_H_

#include<colorer/io/InputSource.h>

/** Implements reading from HTTP URLs.
    Implemented only for win32 systems using
    wininet API.
    @todo UNIX version...
    @ingroup common_io
*/
class HTTPInputSource : public colorer::InputSource
{
public:
  /** Creates stream from http protocol url.
      @param basePath URL to open (can be relative).
      @param base Parent input source, to use as base url for relative URLs.
  */
  HTTPInputSource(const String *basePath, HTTPInputSource *base);
  ~HTTPInputSource();

  const String *getLocation() const;

  const byte *openStream();
  void closeStream();
  int length() const;
protected:
  colorer::InputSource *createRelative(const String *relPath);
private:
  String *baseLocation;
  byte *stream;
  int len;
};

#endif

