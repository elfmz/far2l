#include <memory.h>
#include <colorer/strings/legacy/BitArray.h>

BitArray::BitArray(int _size)
{
  array = nullptr;
  this->size = _size / 8 / 4 + 1;
  if (_size % 8 == 0 && _size / 8 % 4 == 0) this->size--;
}

BitArray::~BitArray()
{
  if (array && size_t(array) != 1) delete[] array;
}

void BitArray::createArray(bool set)
{
  array = new int[size];
  memset(array, set ? 0xFF : 0, size * sizeof(int));
}

void BitArray::setBit(int pos)
{
  if (!array) createArray();
  if (size_t(array) == 1) return;
  array[pos >> 5] |= 1 << (pos & 0x1f);
}

void BitArray::clearBit(int pos)
{
  if (!array) return;
  if (size_t(array) == 1) createArray(true);
  array[pos >> 5] &= ~(1 << (pos & 0x1f));
}

void BitArray::addRange(int s, int e)
{
  if (size_t(array) == 1) return;
  if (!array) createArray();
  int cs = s >> 5;
  if (s & 0x1f) {
    int fillbytes = 0xFFFFFFFF << (s & 0x1f);
    if ((e >> 5) == (s >> 5)) fillbytes &= 0xFFFFFFFF >> (0x1F - (e & 0x1F));
    array[cs] |= fillbytes;
    cs++;
  }
  int ce = e >> 5;
  if (s >> 5 != ce && (e & 0x1f) != 0x1f) {
    array[ce] |= 0xFFFFFFFF >> (0x1F - (e & 0x1F));
    ce--;
  }
  for (int idx = cs; idx <= ce; idx++)
    array[idx] = 0xFFFFFFFF;
  if (cs == 0 && ce == size - 1) {
    delete[] array;
    array = (int*)1;
  }
}

void BitArray::clearRange(int s, int e)
{
  if (!array) return;
  if (size_t(array) == 1) createArray(true);
  int cs = s >> 5;
  if (s & 0x1f) {
    int fillbytes = 0xFFFFFFFF << (s & 0x1f);
    if ((e & 0x1F) == (s & 0x1F)) fillbytes &= 0xFFFFFFFF >> (0x1F - (e & 0x1F));
    array[cs] &= ~fillbytes;
    cs++;
  }
  int ce = e >> 5;
  if (s >> 5 != ce && (e & 0x1f) != 0x1f) {
    array[ce] &= ~(0xFFFFFFFF >> (0x1F - (e & 0x1F)));
    ce--;
  }
  for (int idx = cs; idx <= ce; idx++)
    array[idx] = 0x0;
  if (cs == 0 && ce == size - 1) {
    delete[] array;
    array = nullptr;
  }
}
void BitArray::addBitArray(BitArray* ba)
{
  if (size_t(array) == 1) return;
  if (!ba || !ba->array) return;
  if (size_t(ba->array) == 1) {
    delete[] array;
    array = (int*)1;
    return;
  }
  if (!array) createArray();
  for (int i = 0; i < size; i++)
    array[i] |= ba->array[i];
}

void BitArray::clearBitArray(BitArray* ba)
{
  if (array == nullptr) return;
  if (ba == nullptr || ba->array == nullptr) return;
  if (size_t(array) == 1) createArray(true);
  if (size_t(ba->array) == 1) {
    delete[] array;
    array = nullptr;
    return;
  }
  for (int i = 0; i < size; i++)
    array[i] &= ~ba->array[i];
}

void BitArray::intersectBitArray(BitArray* ba)
{
  if (array == nullptr) return;
  if (ba == nullptr || ba->array == nullptr) {
    delete[] array;
    array = nullptr;
    return;
  }
  if (size_t(ba->array) == 1) return;
  if (size_t(array) == 1) createArray(true);
  for (int i = 0; i < size; i++)
    array[i] &= ba->array[i];
}

void BitArray::addBitArray(char* bits, int _size)
{
  if (size_t(array) == 1) return;
  if (!array) createArray();
  for (int i = 0; i < _size && i < this->size * 4; i++)
    ((char*)array)[i] |= bits[i];
}

void BitArray::clearBitArray(char* bits, int _size)
{
  if (!array) return;
  if (size_t(array) == 1) createArray(true);
  for (int i = 0; i < _size && i < this->size * 4; i++)
    ((char*)array)[i] &= ~bits[i];
}

bool BitArray::getBit(int pos)
{
  if (!array) return false;
  if (size_t(array) == 1) return true;
  return (array[pos >> 5] & (1 << (pos & 0x1f))) != 0;
}


