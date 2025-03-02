#ifndef COLORER_TEXTPARSER_H
#define COLORER_TEXTPARSER_H

#include "colorer/FileType.h"
#include "colorer/LineSource.h"
#include "colorer/RegionHandler.h"
#include "colorer/common/spimpl.h"

/**
 * Basic lexical/syntax parser interface.
 * This class provides interface to lexical text parsing abilities of
 * the Colorer library.
 *
 * It uses LineSource interface as a source of input data, and
 * RegionHandler as interface to transfer results of text parse process.
 *
 * Process of syntax parsing supports internal caching algorithm,
 * which allows to store internal parser state and reparse text
 * only partially (on change, on request).
 *
 * @ingroup colorer
 */
class TextParser
{
 public:
  /**
   * List of available parse modes
   * @ingroup colorer
   */
  enum class TextParseMode {
    /**
     * Parser will start execution from the root
     * document's scheme, no cache information will be used
     */
    TPM_CACHE_OFF,
    /**
     * Parser will use internal cache information to make
     * initial text positioning and guarantee syntax structure validness.
     * The text structure will not be dropped and cache tree will remain the same.
     */
    TPM_CACHE_READ,
    /**
     * Allows parser not only read cache information, but also update
     * it during parse process.
     * Also causes all cached data from starting parse position to be dropped.
     */
    TPM_CACHE_UPDATE
  };

  TextParser();
  /**
   * Sets root scheme (filetype) of the text to parse.
   * @param type FileType, which contains reference to
   * it's baseScheme. If parameter is null, there will
   * be no any kind of parse over the text.
   */
  void setFileType(FileType* type);

  /**
   * Installs LineSource, used as an input of text to parse
   */
  void setLineSource(LineSource* lh);

  /**
   * RegionHandler, used as output stream for parsed tree.
   */
  void setRegionHandler(RegionHandler* rh);

  /**
   * Performs cachable text parse.
   * Can build internal structure of contexts,
   * allowing apprication to continue parse from any already
   * reached position of text. This guarantees the validness of
   * result parse information.
   * @param from  Line to start parsing
   * @param num   Number of lines to parse
   * @param mode  Parsing mode.
   */
  int parse(int from, int num, TextParseMode mode);

  /**
   * Performs break of parsing process from external thread.
   * It is used to stop parse from external source. This is required
   * in some editor system implementations, where editor
   * can detect background changes in highlighted text.
   */
  void breakParse();

  /**
   * Clears internal cached text tree stucture
   */
  void clearCache();
  void setMaxBlockSize(int max_block_size);

  ~TextParser() = default;

 private:
  class Impl;

  spimpl::unique_impl_ptr<Impl> pimpl;
};

#endif  // COLORER_TEXTPARSER_H