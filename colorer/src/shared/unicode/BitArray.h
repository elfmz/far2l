#ifndef _COLORER_BITARRAY_H_
#define _COLORER_BITARRAY_H_

#include<common/Common.h>

/** Bit Array field.
    Creates and manages bit array objects.
    @ingroup unicode
*/
class BitArray{
public:
  /** Creates bit array with specified number of stored bitfields.
  */
  BitArray(int size = 256);
  ~BitArray();

  /** Sets bit at position @c pos */
  void setBit(int pos);
  /** Clears bit at position @c pos */
  void clearBit(int pos);
  /** Sets bit range */
  void addRange(int s, int e);
  /** Clears bit range */
  void clearRange(int s, int e);
  /** Sets bits to 1, whose corresponding values
      in passed bit array are also 1 (bitwize OR) */
  void addBitArray(BitArray*);
  /** Sets bits to 0, whose corresponding values
      in passed bit array are also 1 */
  void clearBitArray(BitArray*);
  /** Makes intersection of current and
      the passed bit array (bitwize AND) */
  void intersectBitArray(BitArray*);
  /** Adds bit array from the passed byte stream. */
  void addBitArray(char*, int);
  /** Clears bit array from the passed byte stream. */
  void clearBitArray(char*, int);
  /** Returns bit value at position @c pos. */
  bool getBit(int pos);

#define CNAME "BitArray"
#include<common/MemoryOperator.h>

private:
  int *array;
  int size;
  void createArray(bool set = false);
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
