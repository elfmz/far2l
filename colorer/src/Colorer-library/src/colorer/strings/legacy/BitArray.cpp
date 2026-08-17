#include <memory.h>
#include <colorer/strings/legacy/BitArray.h>

BitArray::BitArray()
{
}

BitArray::~BitArray()
{
  if (array && size_t(array) != 1) delete[] array;
}

void BitArray::setAll()
{
  if (array && size_t(array) != 1) {
    delete[] array;
  }
  array = (Element*)1;
}

void BitArray::clearAll()
{
  if (array && size_t(array) != 1) {
    delete[] array;
  }
  array = nullptr;
}

void BitArray::createArray(bool set)
{
  array = new Element[ELEMENTS];
  memset(array, set ? 0xFF : 0, ELEMENTS * sizeof(Element));
}

void BitArray::setBit(int pos)
{
  if (!array) createArray();
  if (size_t(array) == 1) return;
  array[pos >> SHIFT] |= 1 << (pos & MASK);
}

void BitArray::clearBit(int pos)
{
  if (!array) return;
  if (size_t(array) == 1) createArray(true);
  array[pos >> SHIFT] &= ~(1 << (pos & MASK));
}

void BitArray::addRange(int s, int e)
{
  if (size_t(array) == 1) return;
  if (!array) createArray();
  int cs = s >> SHIFT;
  if (s & MASK) {
    Element fillbytes = Element(-1) << (s & MASK);
    if ((e >> SHIFT) == (s >> SHIFT)) fillbytes &= Element(-1) >> (MASK - (e & MASK));
    array[cs] |= fillbytes;
    cs++;
  }
  int ce = e >> SHIFT;
  if (s >> SHIFT != ce && (e & MASK) != MASK) {
    array[ce] |= Element(-1) >> (MASK - (e & MASK));
    ce--;
  }
  for (int idx = cs; idx <= ce; idx++)
    array[idx] = Element(-1);
  if (cs == 0 && ce == ELEMENTS - 1) {
    delete[] array;
    array = (Element*)1;
  }
}

void BitArray::clearRange(int s, int e)
{
  if (!array) return;
  if (size_t(array) == 1) createArray(true);
  int cs = s >> SHIFT;
  if (s & MASK) {
    Element fillbytes = Element(-1) << (s & MASK);
    if ((e & MASK) == (s & MASK)) fillbytes &= Element(-1) >> (MASK - (e & MASK));
    array[cs] &= ~fillbytes;
    cs++;
  }
  int ce = e >> SHIFT;
  if (s >> SHIFT != ce && (e & MASK) != MASK) {
    array[ce] &= ~(Element(-1) >> (MASK - (e & MASK)));
    ce--;
  }
  for (int idx = cs; idx <= ce; idx++)
    array[idx] = 0x0;
  if (cs == 0 && ce == ELEMENTS - 1) {
    delete[] array;
    array = nullptr;
  }
}
void BitArray::addBitArray(const BitArray* ba)
{
  if (size_t(array) == 1) return;
  if (!ba || !ba->array) return;
  if (size_t(ba->array) == 1) {
    delete[] array;
    array = (Element*)1;
    return;
  }
  if (!array) createArray();
  for (int i = 0; i < ELEMENTS; i++)
    array[i] |= ba->array[i];
}

void BitArray::clearBitArray(const BitArray* ba)
{
  if (array == nullptr) return;
  if (ba == nullptr || ba->array == nullptr) return;
  if (size_t(array) == 1) createArray(true);
  if (size_t(ba->array) == 1) {
    delete[] array;
    array = nullptr;
    return;
  }
  for (int i = 0; i < ELEMENTS; i++)
    array[i] &= ~ba->array[i];
}

void BitArray::intersectBitArray(const BitArray* ba)
{
  if (array == nullptr) return;
  if (ba == nullptr || ba->array == nullptr) {
    delete[] array;
    array = nullptr;
    return;
  }
  if (size_t(ba->array) == 1) return;
  if (size_t(array) == 1) createArray(true);
  for (int i = 0; i < ELEMENTS; i++)
    array[i] &= ba->array[i];
}

void BitArray::addBitArray(char* bits, int _size)
{
  if (size_t(array) == 1) return;
  if (!array) createArray();
  for (int i = 0; i < _size && i < int(ELEMENTS * sizeof(Element)); i++)
    ((char*)array)[i] |= bits[i];
}

void BitArray::clearBitArray(char* bits, int _size)
{
  if (!array) return;
  if (size_t(array) == 1) createArray(true);
  for (int i = 0; i < _size && i < int(ELEMENTS * sizeof(Element)); i++)
    ((char*)array)[i] &= ~bits[i];
}
