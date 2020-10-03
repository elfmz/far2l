#ifndef _COLORER_TEXTPARSER_H_
#define _COLORER_TEXTPARSER_H_

#include <colorer/FileType.h>
#include <colorer/LineSource.h>
#include <colorer/RegionHandler.h>

/**
 * List of available parse modes
 * @ingroup colorer
 */
enum TextParseMode {
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

/**
 * Basic lexical/syntax parser interface.
 * This class provides interface to lexical text parsing abilities of
 * the Colorer library.
 *
 * It uses LineSource interface as a source of input data, and
 * RegionHandler as interface to transfer results of text parse process.
 *
 * Process of syntax parsing supports internal caching algorithim,
 * which allows to store internal parser state and reparse text
 * only partially (on change, on request).
 *
 * @ingroup colorer
 */
class TextParser
{
public:

  /**
   * Sets root scheme (filetype) of the text to parse.
   * @param type FileType, which contains reference to
   * it's baseScheme. If parameter is null, there will
   * be no any kind of parse over the text.
   */
  virtual void setFileType(FileType* type) = 0;

  /**
   * Installs LineSource, used as an input of text to parse
   */
  virtual void setLineSource(LineSource* lh) = 0;

  /**
   * RegionHandler, used as output stream for parsed tree.
   */
  virtual void setRegionHandler(RegionHandler* rh) = 0;

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
  virtual int parse(int from, int num, TextParseMode mode) = 0;

  /**
   * Performs break of parsing process from external thread.
   * It is used to stop parse from external source. This is required
   * in some editor system implementations, where editor
   * can detect background changes in highlighted text.
   */
  virtual void breakParse() = 0;

  /**
   * Clears internal cached text tree stucture
   */
  virtual void clearCache() = 0;

  virtual ~TextParser() {};
  
  virtual void setMaxBlockSize(int max_block_size){};
protected:
  TextParser() {};
};

#endif


