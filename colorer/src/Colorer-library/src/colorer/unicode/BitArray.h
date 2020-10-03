#ifndef _COLORER_BITARRAY_H_
#define _COLORER_BITARRAY_H_

#include <colorer/Common.h>

/** Bit Array field.
    Creates and manages bit array objects.
    @ingroup unicode
*/
class BitArray
{
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

private:
  int* array;
  int size;
  void createArray(bool set = false);
};

#endif



