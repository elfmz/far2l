#ifndef COLORER_BITARRAY_H
#define COLORER_BITARRAY_H


/** Bit Array field.
    Creates and manages bit array objects.
    @ingroup unicode
*/
class BitArray
{
  typedef unsigned int Element;
  enum
  {
    ELEMENTS = (256 / 8 / 4),
    SHIFT = 5,
    MASK = 0x1f
  };

  Element* array{nullptr};
  void createArray(bool set = false);

  BitArray(const BitArray&) = delete;
  BitArray&operator =(const BitArray&) = delete;

public:
  /** Creates bit array with specified number of stored bitfields.
  */
  BitArray();
  ~BitArray();

  void setAll();
  void clearAll();
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
  void addBitArray(const BitArray*);
  /** Sets bits to 0, whose corresponding values
      in passed bit array are also 1 */
  void clearBitArray(const BitArray*);
  /** Makes intersection of current and
      the passed bit array (bitwize AND) */
  void intersectBitArray(const BitArray*);
  /** Adds bit array from the passed byte stream. */
  void addBitArray(char*, int);
  /** Clears bit array from the passed byte stream. */
  void clearBitArray(char*, int);
  /** Returns bit value at position @c pos. */
  inline bool getBit(int pos) const
  {
    if (!array) return false;
    if (size_t(array) == 1) return true;
    return (array[pos >> 5] & (1 << (pos & 0x1f))) != 0;
  }

#define CNAME "BitArray"
};

#endif
