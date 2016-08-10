#ifndef _COLORER_HASHTABLE_H_
#define _COLORER_HASHTABLE_H_

#include<common/HashtableCore.h>

/**
    Hashtable template class, stores nullable objects.
    Assumes, that stored objects can be nullable (so, null object
    is returned, when no match is found).
    @ingroup common
*/
template <class T>
class Hashtable : public HashtableCore<T>
{
public:
  Hashtable():HashtableCore<T>(){};
  Hashtable(int capacity, double loadFactor = DEFAULT_LOAD_FACTOR):HashtableCore<T>(capacity, loadFactor){};
  ~Hashtable(){};

  T get(const String *key) const{
    int hash = key->hashCode();
    int bno = (hash&0x7FFFFFFF) % this->capacity;
    for(HashEntry<T> *he = this->bucket[bno]; he != null; he = he->next)
      if (he->hash == hash && *he->key == *key)
        return he->value;
    return null;
  };

  /** Starts internal hashtable enumeration procedure.
      Returns first element value in a sequence, or null, if hashtable is empty.
  */
  T enumerate() const{
    T * retval = this->enumerate_int();
    if (retval == null) return null;
    return *retval;
  };
  /** Returns the next value object with current enumeration procedure.
      If hashtable state is changed, and next() call occurs, exception
      is thrown. If end of hash is reached, exception is thrown.
  */
  T next() const{
    T *retval = this->next_int();
    if (retval == null) return null;
    return *retval;
  };
};


/**
    Hashtable template class, stores structured objects.
    Assumes, that stored objects can not be null (complex objects).
    @ingroup common
*/
template <class T>
class HashtableWOnulls : public HashtableCore<T>
{
public:
  HashtableWOnulls():HashtableCore<T>(){};
  HashtableWOnulls(int capacity, double loadFactor = DEFAULT_LOAD_FACTOR):HashtableCore<T>(capacity, loadFactor){};
  ~HashtableWOnulls(){};

  const T *get(const String *key) const{
    int hash = key->hashCode();
    int bno = (hash&0x7FFFFFFF) % this->capacity;
    for(HashEntry<T> *he = this->bucket[bno]; he != null; he = he->next)
      if (he->hash == hash && *he->key == *key)
        return &he->value;
    return null;
  };

  /** Starts internal hashtable enumeration procedure.
      Returns first element value in a sequence, or null, if hashtable is empty
  */
  T *enumerate() const{
    return this->enumerate_int();
  };
  /** Returns the next value object with current enumeration procedure.
      If hashtable state is changed, and next() call occurs, exception
      is thrown. If end of hash is reached, exception is thrown.
  */
  T *next() const{
    return this->next_int();
  };
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
