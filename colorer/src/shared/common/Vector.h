#ifndef _COLORER_VECTOR_H_
#define _COLORER_VECTOR_H_

#include<common/Exception.h>

#define DEFAULT_VECTOR_CAPACITY 20

/** Ordered sequence of objects.
    Each object has it's ordinal position.
    @ingroup common
*/
template <class T>
class Vector{
public:
  /** Default Constructor
  */
  Vector();
  /** Constructor with explicit vector size specification
      @param initsize Initial vector size.
      @param incrementSize Positive number, which should be added to vector size
             each time it reaches it maximum capacity.
             If zero, internal array, each time filled, is double sized.
  */
  Vector(int initsize, int incrementSize = 0);
  /** Default Destructor
  */
  ~Vector();
  /** Clears vector.
  */
  void clear();
  /** Returns number of elements, stored in vector
  */
  int size() const;
  /** Changes vector size. if @c newSize is more, than current size,
      vector is expanded with requested number of elements, and set these
      elements to null. If @c newSize is less, than current size,
      number of top elements is deleted from it.
  */
  void setSize(int newSize);
  /** Ensures, that vector can store specified number of elements
      without resizing.
  */
  void ensureCapacity(int minCapacity);

  /** Adds element into tail of sequence.
  */
  void addElement(const T el);
  /** Inserts element at specified position and expand vector
      by one element.
  */
  void insertElementAt(const T el, int index);
  /** Replaces element at specified position with specified
      element. Vector's size is not changed.
  */
  void setElementAt(const T el, int index);
  /** Removes element at specified @c index and shift all
      elements.
  */
  void removeElementAt(int index);
  /** Removes first found element el from vector.
  */
  bool removeElement(const T el);
  /** Returns index of element, starting search from specified @c index
      parameter. Returns -1, if not found.
  */
  int indexOf(T el, int index) const;
  /** Returns index of element, starting search from start of vector.
      Returns -1, if not found.
  */
  int indexOf(T el) const;
  /** Returns element at specified position
      @throw OutOfBoundException If @c index is too big or less, than zero.
  */
  T elementAt(int index) const;
  /** Returns last element of vector.
      @throw OutOfBoundException If vector has no elements.
  */
  T lastElement() const;
private:
  int csize, asize;
  int incrementSize;
  T*  array;
  Vector &operator=(Vector&);
};

template<class T> Vector<T>::Vector(){
  csize = 0;
  incrementSize = 0;
  asize = DEFAULT_VECTOR_CAPACITY;
  array = new T[asize];
};
template<class T> Vector<T>::Vector(int initsize, int incrementSize){
  csize = 0;
  asize = initsize;
  this->incrementSize = incrementSize;
  array = new T[asize];
};
template<class T> Vector<T>::~Vector(){
  delete[] array;
};


template<class T> int Vector<T>::size() const {
  return csize;
};
template<class T> void Vector<T>::clear(){
  csize = 0;
};
template<class T> void Vector<T>::ensureCapacity(int minCapacity){
  if (asize >= minCapacity) return;
  T* newarray = new T[minCapacity];
  asize = minCapacity;
  for(int idx = 0; idx < csize; idx++)
    newarray[idx] = array[idx];
  delete[] array;
  array = newarray;
};
template<class T> void Vector<T>::setSize(int newSize){
  if (newSize < 0) throw OutOfBoundException();
  if (newSize <= csize){
    csize = newSize;
    return;
  };
  if (newSize > asize) ensureCapacity(newSize);
  if (newSize <= asize){
    for(int idx = csize; idx < newSize; idx++)
      array[idx] = null; //!!!
    csize = newSize;
  };
};

template<class T> void Vector<T>::addElement(const T el){
  insertElementAt(el, csize);
};
template<class T> void Vector<T>::insertElementAt(const T el, int index){
  if (index < 0 || index > csize) throw OutOfBoundException(SString(index));

  if (index == csize && asize > csize){
    array[index] = el;
    csize++;
    return;
  };
  if (asize > csize){
    for (int i = csize; i > index; i--)
      array[i] = array[i-1];
    array[index] = el;
    csize++;
    return;
  };
  if (incrementSize == 0) asize = asize*2;
  else asize += incrementSize;
  T *newarray = new T[asize];
  int nidx = 0;
  for (int oidx = 0; oidx < csize; oidx++, nidx++){
    if (oidx == index) newarray[nidx++] = el;
    newarray[nidx] = array[oidx];
  };
  if (index == csize) newarray[csize] = el;
  csize++;
  delete[] array;
  array = newarray;
};

template<class T> void Vector<T>::setElementAt(const T el, int index){
  if (index < 0 || index >= csize) throw OutOfBoundException(SString(index));
  array[index] = el;
};
template<class T> void Vector<T>::removeElementAt(int index){
  if (index < 0 || index >= csize) throw OutOfBoundException(SString(index));
  for(int idx = index; idx < csize-1; idx++)
    array[idx] = array[idx+1];
  csize--;
};
template<class T> bool Vector<T>::removeElement(const T el){
  for(int idx = 0; idx < csize; idx++)
    if (array[idx] == el){
      removeElementAt(idx);
      return true;
    };
  return false;
};

template<class T> T Vector<T>::elementAt(int index) const {
  if (index < 0 || index >= csize) throw OutOfBoundException(SString(index));
  return array[index];
};

template<class T> int Vector<T>::indexOf(const T el, int index) const {
  if (index < 0 || index >= csize) return -1;
  for(int i = index; i < csize; i++)
    if (array[i] == el) return i;
  return -1;
};
template<class T> int Vector<T>::indexOf(const T el) const {
  return indexOf(el, 0);
};
template<class T> T Vector<T>::lastElement() const{
  if (csize == 0) throw OutOfBoundException(DString("no lastElement in empty vector"));
  return array[csize-1];
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
