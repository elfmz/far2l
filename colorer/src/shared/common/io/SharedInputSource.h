#ifndef _COLORER_SHAREDINPUTSOURCE_H_
#define _COLORER_SHAREDINPUTSOURCE_H_

#include<common/Common.h>
#include<common/Hashtable.h>
#include<common/Logging.h>
#include<common/io/InputSource.h>

/**
 * InputSource class wrapper,
 * allows to manage class instances counter.
 * @ingroup common_io
 */
class SharedInputSource : InputSource
{

public:

  static SharedInputSource *getInputSource(const String *path, InputSource *base);

  /** Increments reference counter */
  int addref(){
    return ++ref_count;
  }

  /** Decrements reference counter */
  int delref(){
    if (ref_count == 0){
      CLR_ERROR("SharedInputSource", "delref: already zeroed references");
    }
    ref_count--;
    if (ref_count <= 0){
      delete this;
	  return -1;
    }
    return ref_count;
  }

  /**
   * Returns currently opened stream.
   * Opens it, if not yet opened.
   */
  const byte *getStream(){
    if (stream == null){
      stream = openStream();
    }
    return stream;
  }

  const String *getLocation() const{
    return is->getLocation();
  }

  const byte *openStream(){
    return is->openStream();
  }

  void closeStream(){ is->closeStream(); }

  int length() const{ return is->length(); }

private:

  SharedInputSource(InputSource *source);
  ~SharedInputSource();

  static Hashtable<SharedInputSource*> *isHash;

  InputSource *is;
  const byte *stream;
  int ref_count;
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
