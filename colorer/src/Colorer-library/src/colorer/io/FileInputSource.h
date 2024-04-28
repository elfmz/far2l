#ifndef _COLORER_FILEINPUTSOURCE_H_
#define _COLORER_FILEINPUTSOURCE_H_

#include "colorer/io/InputSource.h"

/** Reads data from file with OS services.
    @ingroup common_io
*/
class FileInputSource : public colorer::InputSource
{
 public:
  FileInputSource(const UnicodeString* basePath, FileInputSource* base);
  ~FileInputSource() override;

  [[nodiscard]] const UnicodeString* getLocation() const override;

  const byte* openStream() override;
  void closeStream() override;
  [[nodiscard]] int length() const override;

 protected:
  colorer::InputSource* createRelative(const UnicodeString* relPath) override;

  UnicodeString* baseLocation = nullptr;
  byte* stream = nullptr;
  int len = 0;
};

#endif
