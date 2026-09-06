#ifndef COLORER_PARSEDLINEWRITER_H
#define COLORER_PARSEDLINEWRITER_H

#include <unordered_map>
#include "colorer/handlers/LineRegion.h"
#include "colorer/io/Writer.h"
/**
    Static service methods of LineRegion output.
    @ingroup colorer_viewer
*/
class ParsedLineWriter
{
 public:
  /** Writes given line of text using list of passed line regions.
      Formats output with class of each token, enclosed in
      \<span class='regionName'>...\</span>
      @param markupWriter Writer, used for markup output
      @param textWriter Writer, used for text output
      @param line Line of text
      @param lineRegions Linked list of LineRegion structures.
             Only region references are used there.
  */
  static void tokenWrite(Writer* markupWriter, Writer* textWriter, std::unordered_map<UnicodeString, UnicodeString*>* /*docLinkHash*/,
                         const UnicodeString* line, LineRegion* lineRegions);

  /** Write specified line of text using list of LineRegion's.
      This method uses text fields of LineRegion class to enwrap each line
      region.
      It uses two Writers - @c markupWriter and @c textWriter.
      @c markupWriter is used to write markup elements of LineRegion,
      and @c textWriter is used to write line content.
      @param markupWriter Writer, used for markup output
      @param textWriter Writer, used for text output
      @param line Line of text
      @param lineRegions Linked list of LineRegion structures
  */
  static void markupWrite(Writer* markupWriter, Writer* textWriter, std::unordered_map<UnicodeString, UnicodeString*>* /*docLinkHash*/,
                          const UnicodeString* line, LineRegion* lineRegions);

  /** Write specified line of text using list of LineRegion's.
      This method uses integer fields of LineRegion class
      to enwrap each line region with generated HTML markup.
      Each region is
      @param markupWriter Writer, used for markup output
      @param textWriter Writer, used for text output
      @param line Line of text
      @param lineRegions Linked list of LineRegion structures
  */
  static void htmlRGBWrite(Writer* markupWriter, Writer* textWriter, std::unordered_map<UnicodeString, UnicodeString*>* docLinkHash,
                           const UnicodeString* line, LineRegion* lineRegions);

  /** Puts into stream style attributes from RegionDefine object.
   */
  static void writeStyle(Writer* writer, const StyledRegion* lr);

  /** Puts into stream starting HTML \<span> tag with requested style specification
   */
  static void writeStart(Writer* writer, const StyledRegion* lr);

  /** Puts into stream ending HTML \</span> tag
   */
  static void writeEnd(Writer* writer, const StyledRegion* lr);

  static void writeHref(Writer* writer, std::unordered_map<UnicodeString, UnicodeString*>* docLinkHash, const Scheme* scheme,
                        const UnicodeString& token, bool start);
};

#endif // COLORER_PARSEDLINEWRITER_H
