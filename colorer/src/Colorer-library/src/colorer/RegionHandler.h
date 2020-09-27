#ifndef _COLORER_REGIONHANDLER_H_
#define _COLORER_REGIONHANDLER_H_

#include <colorer/Region.h>
#include <colorer/Scheme.h>

/** Handles parse information, passed from TextParser.
    TextParser class generates calls of this class methods
    sequentially while parsing the text from top to bottom.
    All enterScheme and leaveScheme calls are properly enclosed,
    addRegion calls can inform about regions, overlapped with each other.
    All handler methods are called sequentially. It means, that
    if one of methods is called with some line number, all other calls
    (before endParsing event comes) can inform about events in the same,
    or lower line's numbers. This makes sequential tokens processing.
    @ingroup colorer
*/
class RegionHandler
{
public:
  /** Start of text parsing.
      Called only once, when TextParser starts
      parsing of the specified block of text.
      All other event messages comes between this call and
      endParsing call.
      @param lno Start line number
  */
  virtual void startParsing(size_t lno) {};

  /** End of text parsing.
      Called only once, when TextParser stops
      parsing of the specified block of text.
      @param lno End line number
  */
  virtual void endParsing(size_t lno) {};

  /** Clear line event.
      Called once for each parsed text line, when TextParser starts to parse
      specified line of text. This method is called before any of the region
      information passed, and used often to clear internal handler
      structure of this line before adding new one.
      @param lno Line number
  */
  virtual void clearLine(size_t lno, String* line) {};

  /** Informs handler about lexical region in line.
      This is a basic method, wich transfer information from
      parser to application. Positions of different passed regions
      can be overlapped.
      @param lno Current line number
      @param sx Start X position of region in line
      @param ex End X position of region in line
      @param region Region information
  */
  virtual void addRegion(size_t lno, String* line, int sx, int ex, const Region* region) = 0;

  /** Informs handler about entering into specified scheme.
      Parameter <code>region</code> is used to specify
      scheme background region information.
      If text is parsed not from the first line, this method is called
      with fake parameters to compensate required scheme structure.
      @param lno Current line number
      @param sx Start X position of region in line
      @param ex End X position of region in line
      @param region Scheme Region information (background)
      @param scheme Additional Scheme information
  */
  virtual void enterScheme(size_t lno, String* line, int sx, int ex, const Region* region, const Scheme* scheme) = 0;

  /** Informs handler about leaveing specified scheme.
      Parameter <code>region</code> is used to specify
      scheme background region information.
      If text parse process ends, but current schemes stack is not balanced
      (this can happends because of bad balanced structure of source text,
      or partial text parse) this method is <b>not</b> called for unbalanced
      levels.
      @param lno Current line number
      @param sx Start X position of region in line
      @param ex End X position of region in line
      @param region Scheme Region information (background)
      @param scheme Additional Scheme information
  */
  virtual void leaveScheme(size_t lno, String* line, int sx, int ex, const Region* region, const Scheme* scheme) = 0;

protected:
  RegionHandler() {};
  virtual ~RegionHandler() {};
};

#endif


