#ifndef _COLORER_TEXTPARSER_H_
#define _COLORER_TEXTPARSER_H_

#include<colorer/FileType.h>
#include<colorer/LineSource.h>
#include<colorer/RegionHandler.h>

/**
 * List of available parse modes
 * @ingroup colorer
 */
enum TextParseMode{
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
  virtual void setFileType(FileType *type) = 0;

  /**
   * Installs LineSource, used as an input of text to parse
   */
  virtual void setLineSource(LineSource *lh) = 0;

  /**
   * RegionHandler, used as output stream for parsed tree.
   */
  virtual void setRegionHandler(RegionHandler *rh) = 0;

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

  virtual ~TextParser(){};
protected:
  TextParser(){};
};

#endif
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
