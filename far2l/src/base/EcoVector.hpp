#pragma once

// Memory-economic vector, used primarily for Edit.hpp/Edit.cpp
// Reasons:
// - sizeof(std::vector) == 3 * sizeof(void *)
// - sizeof(EcoVector) == sizeof(void *)
// - Strict shrink_to_fit() and guaranteed no allocation whenever its empty

template <class T, size_t FIRST_ENLARGE = 1>
	class EcoVector
{
	struct Content
	{
		size_t capacity, size;
		T data[1]; // ..or more
	};
	Content *_content{nullptr};

	EcoVector(const EcoVector &src) = delete;
	EcoVector &operator=(const EcoVector &src) = delete;

	bool reallocate(size_t new_capacity)
	{
		if (new_capacity != 0) {
			const size_t alloc_sz = sizeof(Content) + new_capacity * sizeof(T) - sizeof(T);
			if (UNLIKELY(alloc_sz < new_capacity)) {
				return false; // overflow
			}
			Content *new_content = (Content *)malloc(alloc_sz);
			if (UNLIKELY(!new_content)) {
				return false;
			}
			if (_content) {
				for (size_t i = _content->size; i--; ) {
					new (&new_content->data[i]) T(std::move(_content->data[i]));
				}
				new_content->size = _content->size;
				_content->size = _content->capacity = -1; // put a rake under dangling pointer
				free(_content);
			} else {
				new_content->size = 0;
			}
			new_content->capacity = new_capacity;
			_content = new_content;
		} else if (_content) {
			_content->size = _content->capacity = -1; // put a rake under dangling pointer
			free(_content);
			_content = nullptr;
		}
		return true;
	}

	bool enlarge()
	{
		const size_t old_capacity = capacity();
		size_t new_capacity = old_capacity ? (old_capacity + old_capacity / 4 + 7) : FIRST_ENLARGE;
		if (new_capacity > old_capacity && reallocate(new_capacity)) {
			return true;
		}
		new_capacity = old_capacity + 1;
		if (new_capacity > old_capacity && reallocate(new_capacity)) {
			return true;
		}
		return false;
	}

public:
    using iterator = T*;
    using const_iterator = const T*;

	EcoVector() = default;

	~EcoVector()
	{
		if (_content) {
			resize(0);
			ASSERT(!_content);
		}
	}

	EcoVector(EcoVector &&o) noexcept : _content(o._content)
	{
		o._content = nullptr;
	}

	EcoVector &operator=(EcoVector &&o) noexcept
	{
		std::swap(_content, o._content);
		return *this;
	}

	bool empty() const
	{
		return _content == nullptr; // size cannot zero otherwise
	}

	size_t size() const
	{
		return _content ? _content->size : 0;
	}

	size_t capacity() const
	{
		return _content ? _content->capacity : 0;
	}

	const T *data() const
	{
		return _content ? &_content->data[0] : nullptr;
	}
	T *data()
	{
		return _content ? &_content->data[0] : nullptr;
	}

	const T &at(size_t index) const
	{
		ASSERT_MSG(index < size(),  "EcoVector[]: bad %ld while size=%ld\n", (long)index, size());
		return _content->data[index];
	}

	T &at(size_t index)
	{
		ASSERT_MSG(index < size(),  "EcoVector[]: bad %ld while size=%ld\n", (long)index, size());
		return _content->data[index];
	}

	const T& operator[](size_t index) const { return at(index); }
	T& operator[](size_t index) { return at(index); }

	template <typename... Args>
		T *emplace_back(Args... args)
	{
		if (size() == capacity() && !enlarge()) {
			return nullptr;
		}
		T *p = new (&_content->data[size()]) T (args...);
		if (p) {
			_content->size++;
		}
		return p;
	}

	bool resize(size_t want_sz)
	{
		size_t sz = size();
		if (sz < want_sz) {
			if (want_sz > capacity() && !reallocate(want_sz)) {
				return false;
			}
			for (; sz < want_sz; ++sz) {
				new (&_content->data[sz]) T();
			}
		} else if (sz > want_sz) {
			do {
				--sz;
				_content->data[sz].~T();
			} while (sz > want_sz);
		} else {
			return true;
		}
		_content->size = want_sz;
		if (want_sz == 0) {
			reallocate(0); // ensure deallocated
		}
		return true;
	}

	void clear()
	{
		resize(0);
	}

	void shrink_to_fit()
	{
		if (capacity() != size()) {
			reallocate(size());
		}
	}

	void swap(EcoVector<T, FIRST_ENLARGE> &another) { std::swap(_content, another._content); }
    iterator begin() { return data(); }
    iterator end() { return data() + size(); }
    const_iterator begin() const { return data(); }
    const_iterator end() const { return data() + size(); }
};
