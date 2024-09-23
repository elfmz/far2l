#pragma once

/*
array.hpp

Шаблон работы с массивом

	TArray<Object> Array;
	// Object должен иметь конструктор по умолчанию и следующие операторы
	//  bool operator==(const Object &) const
	//  bool operator<(const Object &) const
	//  const Object& operator=(const Object &)

	TPointerArray<Object> Array;
	Object должен иметь конструктор по умолчанию.
	Класс для тупой но прозрачной работы с массивом понтеров на класс Object
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <new>		// for std::nothrow
#include <limits>
#include <assert.h>
#include <WinCompat.h>
#include "farrtl.hpp"

#ifdef __GNUC__
typedef int __cdecl (*TARRAYCMPFUNC)(const void *el1, const void *el2);
#else
typedef int (*TARRAYCMPFUNC)(const void *el1, const void *el2);
#endif

template <class Object>
class TArray
{
private:
	size_t internalCount, Count, Delta;
	Object **items;

private:
	static int __cdecl CmpItems(const Object **el1, const Object **el2);
	bool deleteItem(size_t index);

public:
	TArray(size_t Delta = 8);
	TArray(const TArray<Object> &rhs);
	~TArray() { Free(); }

public:
	TArray &operator=(const TArray<Object> &rhs);

public:
	void Free();
	void setDelta(size_t newDelta);
	bool setSize(size_t newSize);

	Object *setItem(size_t index, const Object &newItem);
	Object *getItem(size_t index);
	const Object *getConstItem(size_t index) const;
	int getIndex(const Object &item, int start = -1);

	// сортировка массива. Offset - сколько первых пунктов пропустить
	void Sort(TARRAYCMPFUNC user_cmp_func = nullptr, size_t Offset = 0);

	// упаковать массив - вместо нескольких одинаковых элементов,
	/*
		идущих подряд, оставить только один. Возвращает, false,
		если изменений массива не производилось.
		Вызов Pack() после Sort(nullptr) приведет к устранению
		дубликатов
	*/
	bool Pack();

public:		// inline
	size_t getSize() const { return Count; }
	Object *addItem(const Object &newItem) { return setItem(Count, newItem); }
};

template <class Object>
TArray<Object>::TArray(size_t delta)
	:
	internalCount(0), Count(0), items(nullptr)
{
	setDelta(delta);
}

template <class Object>
void TArray<Object>::Free()
{
	if (items) {
		for (size_t i = 0; i < Count; ++i)
			delete items[i];

		free(items);
		items = nullptr;
	}

	Count = internalCount = 0;
}

template <class Object>
Object *TArray<Object>::setItem(size_t index, const Object &newItem)
{
	bool set = true;

	if (index < Count)
		deleteItem(index);
	else
		set = setSize(index + (index == Count));

	if (set) {
		items[index] = new (std::nothrow) Object;

		if (items[index])
			*items[index] = newItem;

		return items[index];
	}

	return nullptr;
}

template <class Object>
Object *TArray<Object>::getItem(size_t index)
{
	return (index < Count) ? items[index] : nullptr;
}

template <class Object>
const Object *TArray<Object>::getConstItem(size_t index) const
{
	return (index < Count) ? items[index] : nullptr;
}

template <class Object>
void TArray<Object>::Sort(TARRAYCMPFUNC user_cmp_func, size_t Offset)
{
	if (Count) {
		if (!user_cmp_func)
			user_cmp_func = reinterpret_cast<TARRAYCMPFUNC>(CmpItems);

		far_qsort(reinterpret_cast<char *>(items + Offset), Count - Offset, sizeof(Object *), user_cmp_func);
	}
}

template <class Object>
bool TArray<Object>::Pack()
{
	bool was_changed = false;

	for (size_t index = 1; index < Count; ++index) {
		if (*items[index - 1] == *items[index]) {
			deleteItem(index);
			was_changed = true;
			--Count;

			if (index < Count) {
				memmove(&items[index], &items[index + 1], sizeof(Object *) * (Count - index));
				--index;
			}
		}
	}

	return was_changed;
}

template <class Object>
bool TArray<Object>::deleteItem(size_t index)
{
	if (index < Count) {
		delete items[index];
		items[index] = nullptr;
		return true;
	}

	return false;
}

template <class Object>
bool TArray<Object>::setSize(size_t newSize)
{
	bool rc = false;

	if (newSize < Count)	// уменьшение размера
	{
		assert(Count < std::numeric_limits<size_t>::max() / sizeof(void *));
		for (size_t i = newSize; i < Count; ++i) {
			delete items[i];
			items[i] = nullptr;
		}

		Count = newSize;
		rc = true;
	} else if (newSize < internalCount)		// увеличение, но в рамках имеющегося
	{
		for (size_t i = Count; i < newSize; ++i)
			items[i] = nullptr;

		Count = newSize;
		rc = true;
	} else		// увеличение размера
	{
		size_t Remainder = newSize % Delta;
		size_t newCount = Remainder ? (newSize + Delta) - Remainder : newSize ? newSize : Delta;
		Object **newItems = static_cast<Object **>(malloc(newCount * sizeof(Object *)));

		if (newItems) {
			if (items) {
				memcpy(newItems, items, Count * sizeof(Object *));
				free(items);
			}

			items = newItems;
			internalCount = newCount;

			for (size_t i = Count; i < newSize; ++i)
				items[i] = nullptr;

			Count = newSize;
			rc = true;
		}
	}

	return rc;
}

template <class Object>
int __cdecl TArray<Object>::CmpItems(const Object **el1, const Object **el2)
{
	if (el1 == el2)
		return 0;
	else if (**el1 == **el2)
		return 0;
	else if (**el1 < **el2)
		return -1;
	else
		return 1;
}

template <class Object>
TArray<Object>::TArray(const TArray<Object> &rhs)
	:
	items(nullptr), Count(0), internalCount(0)
{
	*this = rhs;
}

template <class Object>
TArray<Object> &TArray<Object>::operator=(const TArray<Object> &rhs)
{
	if (this == &rhs)
		return *this;

	setDelta(rhs.Delta);

	if (setSize(rhs.Count)) {
		for (unsigned i = 0; i < Count; ++i) {
			if (rhs.items[i]) {
				if (!items[i])
					items[i] = new Object;

				if (items[i])
					*items[i] = *rhs.items[i];
				else {
					Free();
					break;
				}
			} else {
				delete items[i];
				items[i] = nullptr;
			}
		}
	}

	return *this;
}

template <class Object>
void TArray<Object>::setDelta(size_t newDelta)
{
	if (newDelta < 4)
		newDelta = 4;

	Delta = newDelta;
}

template <class Object>
int TArray<Object>::getIndex(const Object &item, int start)
{
	int rc = -1;

	if (start == -1)
		start = 0;

	for (size_t i = start; i < Count; ++i) {
		if (items[i] && item == *items[i]) {
			rc = i;
			break;
		}
	}

	return rc;
}

template <class Object>
class TPointerArray
{
private:
	size_t internalCount, Count, Delta;
	Object **items;

private:
	bool setSize(size_t newSize)
	{
		bool rc = false;

		if (newSize < Count)	// уменьшение размера
		{
			Count = newSize;
			rc = true;
		} else if (newSize < internalCount)		// увеличение, но в рамках имеющегося
		{
			for (size_t i = Count; i < newSize; i++)
				items[i] = nullptr;

			Count = newSize;
			rc = true;
		} else		// увеличение размера
		{
			size_t Remainder = newSize % Delta;
			size_t newCount = Remainder ? (newSize + Delta) - Remainder : (newSize ? newSize : Delta);
			Object **newItems = static_cast<Object **>(realloc(items, newCount * sizeof(Object *)));

			if (newItems) {
				items = newItems;
				internalCount = newCount;

				for (size_t i = Count; i < newSize; i++)
					items[i] = nullptr;

				Count = newSize;
				rc = true;
			}
		}

		return rc;
	}

public:
	TPointerArray(size_t delta = 1)
	{
		items = nullptr;
		Count = internalCount = 0;
		setDelta(delta);
	}
	~TPointerArray() { Free(); }

	void Free()
	{
		if (items) {
			for (size_t i = 0; i < Count; ++i)
				delete items[i];

			free(items);
			items = nullptr;
		}

		Count = internalCount = 0;
	}

	void setDelta(size_t newDelta)
	{
		if (newDelta < 1)
			newDelta = 1;
		Delta = newDelta;
	}

	Object *getItem(size_t index) { return (index < Count) ? items[index] : nullptr; }

	const Object *getConstItem(size_t index) const { return (index < Count) ? items[index] : nullptr; }

	Object *lastItem() { return Count ? items[Count - 1] : nullptr; }

	Object *addItem() { return insertItem(Count); }

	Object *insertItem(size_t index)
	{
		if (index > Count)
			return nullptr;

		assert(Count < std::numeric_limits<size_t>::max() / sizeof(void *));
		Object *newItem = new (std::nothrow) Object;
		if (newItem && setSize(Count + 1)) {
			for (size_t i = Count - 1; i > index; i--)
				items[i] = items[i - 1];

			items[index] = newItem;
			return items[index];
		}

		if (newItem)
			delete newItem;

		return nullptr;
	}

	bool deleteItem(size_t index)
	{
		if (index < Count) {
			delete items[index];

			for (size_t i = index + 1; i < Count; i++)
				items[i - 1] = items[i];

			setSize(Count - 1);
			return true;
		}

		return false;
	}

	bool swapItems(size_t index1, size_t index2)
	{
		if (index1 < Count && index2 < Count) {
			Object *temp = items[index1];
			items[index1] = items[index2];
			items[index2] = temp;
			return true;
		}

		return false;
	}

	size_t getCount() const { return Count; }
};
