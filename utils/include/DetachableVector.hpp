#pragma once

template <class T, class A = std::allocator<T> >
	class DetachableVector
{
	T *_data = nullptr;
	size_t _size = 0;
	size_t _capacity = 0;

	public:
	~DetachableVector()
	{
		if (_data != nullptr) {
			clear();
			A::deallocate(_data, _capacity);
			_data = nullptr;
			_capacity = 0xdeadbeef;
		}
	}

	T *detach()
	{
		T* out = _data;
		_data = nullptr;
		_size = _capacity = 0;
		return out;
	}

	template <typename... Ts>
		void emplace_back(Ts&&... args)
	{
		if (_size == _capacity) {
			size_t n_capacity = _capacity + 32 + _capacity / 4;
			if (n_capacity < _capacity)
				throw std::bad_alloc();
			T *n_data = A::allocate( n_capacity);
			for (size_t i = 0; i < _size; ++i) {
				A::construct(&n_data[i], _data[i]);
				A::destroy(&_data[i]);
			}
			if (_data != nullptr) {
				A::deallocate(_data, _capacity);
			}
			_capacity = n_capacity;
			_data = n_data;
		}
		A::construct(&_data[_size], std::forward<Ts>(args)... );
		++_size;
	}


	void resize(size_t size)
	{
		if (_size > size) {
			do {
				--_size;
				_data[_size]->~T();
			} while (_size > size);

		} else if (_size < size) {
			do {
				emplace_back();
			} while (_size < size);
		}
	}

	void clear() { resize(0); }
	T &operator[](size_t index) { return _data[index]; }
	const T &operator[](size_t index) const { return _data[index]; }
	size_t size() const {return _size; }

	typedef std::iterator<pointer, T> iterator;
	typedef std::iterator<pointer, const T> const_iterator;
	
	iterator begin() {return _base; }
	iterator end() {return _base + _size; }
	const_iterator begin() const {return _base; }
	const_iterator end() const {return _base + _size; }
};
