#ifndef _COLORER_FILEINPUTSOURCE_H_
#define _COLORER_FILEINPUTSOURCE_H_

#include<colorer/io/InputSource.h>

/** Reads data from file with OS services.
    @ingroup common_io
*/
class FileInputSource : public colorer::InputSource
{
public:
  FileInputSource(const String *basePath, FileInputSource *base);
  ~FileInputSource();

  const String *getLocation() const;

  const byte *openStream();
  void closeStream();
  int length() const;
protected:
  colorer::InputSource *createRelative(const String *relPath);

  String *baseLocation;
  byte *stream;
  int len;
};

#endif

