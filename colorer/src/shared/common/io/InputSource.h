#ifndef _COLORER_INPUTSOURCE_H_
#define _COLORER_INPUTSOURCE_H_

#include<common/Common.h>

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
  virtual InputSource *createRelative(const String *relPath){ return null; };

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
