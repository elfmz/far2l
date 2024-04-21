#ifndef _COLORER_TEXTLINESSTORE_H_
#define _COLORER_TEXTLINESSTORE_H_

#include <vector>
#include "colorer/LineSource.h"

/** Reads array of text lines and
    makes it accessible with LineSource interface.
    All lines should be separated with \\r\\n , \\n or \\r characters.

    @ingroup colorer_viewer
*/
class TextLinesStore : public LineSource
{
public:

  /** Empty constructor. Does nothing.
  */
  TextLinesStore()=default;
  ~TextLinesStore() override;

  /** Loads specified file into vector of strings.
      @param inFileName File to load.
      @param tab2spaces Points, if we have to convert all tabs in file into spaces.
  */
  void loadFile(const UnicodeString* inFileName, bool tab2spaces);
  /** Returns loaded file name.
  */
  const UnicodeString* getFileName();
  /** Returns total lines count in text. */
  size_t getLineCount();

  UnicodeString* getLine(size_t lno) override;
protected:
  /** Frees loaded file lines.
  */
  void freeFile();
private:
  std::vector<UnicodeString *> lines;
  uUnicodeString fileName;
  void replaceTabs(size_t lno);

};

#endif


