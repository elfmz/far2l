#ifndef _COLORER_HASHTABLECORE_H_
#define _COLORER_HASHTABLECORE_H_

#include<memory.h>
#include<unicode/String.h>

#define DEFAULT_LOAD_FACTOR 0.75
#define DEFAULT_CAPACITY    20

/**
    Hashtable Entry.
    @ingroup common
*/
template <class T>
class HashEntry{
public:
  int hash;
  SString *key;
  T value;
  HashEntry *next;

  HashEntry(int hash, const String *key, T value, HashEntry *next){
    this->hash = hash;
    this->key = new SString(key);
    this->value = value;
    this->next = next;
  };
  ~HashEntry(){ delete key; };
};

/** Common hashtable template.
    Has no suppositions about templated object structure.
    @ingroup common
*/
template <class T>
class HashtableCore{
public:
  /** Default constructor
  */
  HashtableCore();
  /** Constructor with explicit hashtable size declaration
      @param capacity Initial capacity of Hashtable
      @param loadFactor Initial loadFactor (0 < loadFactor < 1)
  */
  HashtableCore(int capacity, double loadFactor = DEFAULT_LOAD_FACTOR);
  /** Default Destructor
  */
  virtual ~HashtableCore();

  /** Puts object into hashtable.
      If such a key already exists, object is replaced.
      @return true, if object is replaced, false, if new object is inserted.
  */
  bool put(const String *key, T value);
  /** Removes object from hashtable.
      If there is no record, associated with @c key, method does nothing.
      @return true, if object was removed
  */
  bool remove(const String *key);

  /** Enumerates hashtable keys.
      @param index Internal index. Must be less, than #size()
  */
  const String *key(int index) const;

  /** Starts internal hashtable enumeration procedure on keys.
      @return First element's key value in a sequence, or null, if hashtable is empty.
  */
  String *enumerateKey() const;
  /** Returns the next key object with current enumeration procedure.
      If hashtable state is changed, and nextkey() call occurs, exception
      is thrown. Note, that internal enumeration counter is one for
      keys and values methods - so, each of them iterates one sequence.
  */
  String *nextkey() const;

  /** Clears hashtable.
      Deletes all objects, stored in it.
  */
  void clear();
  /** Number of objects, currently stored in hashtable.
  */
  int size() const;
protected:

  T *enumerate_int() const;
  T *next_int() const;

  int csize;
  int capacity;
  double loadFactor;
  HashEntry<T> **bucket;
  void rehash();
  HashtableCore &operator=(HashtableCore&);
  mutable HashEntry<T> *enum_he;
  mutable int enum_bno;
};

template<class T> HashtableCore<T>::HashtableCore(){
  csize = 0;
  enum_he = null;
  enum_bno = -1;
  capacity = DEFAULT_CAPACITY;
  loadFactor = DEFAULT_LOAD_FACTOR;
  bucket = new HashEntry<T>*[capacity];
  memset(bucket, 0, sizeof(HashEntry<T>*)*capacity);
};
template<class T> HashtableCore<T>::HashtableCore(int capacity, double loadFactor){
  csize = 0;
  if (capacity < 1) capacity = DEFAULT_CAPACITY;
  if (loadFactor < 0.01) loadFactor = DEFAULT_LOAD_FACTOR;
  this->capacity = capacity;
  this->loadFactor = loadFactor;
  bucket = new HashEntry<T>*[capacity];
  memset(bucket, 0, sizeof(HashEntry<T>*)*capacity);
};
template<class T> HashtableCore<T>::~HashtableCore(){
  clear();
  delete[] bucket;
};

template<class T> bool HashtableCore<T>::put(const String *key, T value){
  enum_bno = -1;
  int hash = key->hashCode();
  int bno = (hash&0x7FFFFFFF) % capacity;
  for(HashEntry<T> *he = bucket[bno]; he != null; he = he->next){
    if (he->hash == hash && *he->key == *key){
      he->value = value;
      return true;
    };
  };
  HashEntry<T> *he1 = bucket[bno];
  bucket[bno] = new HashEntry<T>(hash, key, value, he1);
  csize++;
  if (double(csize)/loadFactor > capacity) rehash();
  return false;
};

template<class T> bool HashtableCore<T>::remove(const String *key){
  int hash = key->hashCode();
  int bno = (hash&0x7FFFFFFF) % capacity;
  HashEntry<T> *he_prev = null;
  for(HashEntry<T> *he = bucket[bno]; he != null; he = he->next){
    if (he->hash == hash && *he->key == *key){
      if (he_prev == null){
        bucket[bno] = null;
      }else
        he_prev->next = he->next;
      delete he;
      csize--;
      enum_bno = -1;
      return true;
    };
    he_prev = he;
  };
  return false;
};

template<class T> void HashtableCore<T>::rehash(){
  int new_capacity = capacity*2+1;
  int new_csize = csize;
  HashEntry<T> **new_bucket = new HashEntry<T>*[new_capacity];
  memset(new_bucket, 0, sizeof(HashEntry<T>*)*new_capacity);

  for(int bno = 0; bno < capacity; bno++)
    for(HashEntry<T> *he = bucket[bno]; he != null; he = he->next){
      int new_bno = (he->hash&0x7FFFFFFF) % new_capacity;
      HashEntry<T> *new_he = new_bucket[new_bno];
      new_bucket[new_bno] = new HashEntry<T>(he->hash, he->key, he->value, new_he);
    };
  clear();
  delete[] bucket;
  csize = new_csize;
  capacity = new_capacity;
  bucket = new_bucket;
}

template<class T> const String* HashtableCore<T>::key(int index) const{
  if (index < 0 || index > csize) return null;
  for(int bno = 0; bno < capacity; bno++)
    for(HashEntry<T> *he = bucket[bno]; he != null; he = he->next){
      if (index == 0) return he->key;
      index--;
    };
  return null;
}


template<class T> T *HashtableCore<T>::enumerate_int() const{
  for(enum_bno = 0; enum_bno < capacity; enum_bno++){
    if(bucket[enum_bno] != null){
      enum_he = bucket[enum_bno];
      return &enum_he->value;
    };
  };
  enum_he = null;
  enum_bno = -2;
  return null;
}
template<class T> String *HashtableCore<T>::enumerateKey() const{
  for(enum_bno = 0; enum_bno < capacity; enum_bno++){
    if(bucket[enum_bno] != null){
      enum_he = bucket[enum_bno];
      return enum_he->key;
    };
  };
  enum_he = null;
  enum_bno = -2;
  return null;
}

template<class T> T *HashtableCore<T>::next_int() const{
  if (enum_bno == -1)
    throw Exception(DString("Hashtable enumeration fault - object change detected"));
  if (enum_bno < 0) // == -2
    throw Exception(DString("Hashtable enumeration fault - end was reached"));
  if (enum_he != null && enum_he->next != null){
    enum_he = enum_he->next;
    return &enum_he->value;
  };
  for(enum_bno = enum_bno+1; enum_bno < capacity; enum_bno++)
    if(bucket[enum_bno] != null){
      enum_he = bucket[enum_bno];
      return &enum_he->value;
    };
  enum_bno = -2;
  return null;
}
template<class T> String *HashtableCore<T>::nextkey() const{
  if (enum_bno == -1)
    throw Exception(DString("Hashtable enumeration fault - object change detected"));
  if (enum_bno < 0) // == -2
    throw Exception(DString("Hashtable enumeration fault - end was reached"));
  if (enum_he != null && enum_he->next != null){
    enum_he = enum_he->next;
    return enum_he->key;
  };
  for(enum_bno = enum_bno+1; enum_bno < capacity; enum_bno++){
    if(bucket[enum_bno] != null){
      enum_he = bucket[enum_bno];
      return enum_he->key;
    };
  };
  enum_bno = -2;
  return null;
}

template<class T> void HashtableCore<T>::clear(){
  for(int i = 0; i < capacity; i++){
    for (HashEntry<T> *ent = bucket[i]; ent != null;){
      HashEntry<T> *thisent = ent;
      ent = thisent->next;
      delete thisent;
    };
    bucket[i] = null;
    enum_bno = -1;
  };
  csize = 0;
};

template<class T> int HashtableCore<T>::size() const {
  return csize;
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
