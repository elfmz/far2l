#pragma once
#include <memory_resource>

template <class T>
	class EcoPool
{
	static constexpr size_t PoolBlockSize()
	{
		size_t size = 8;
		const size_t required = sizeof(T) > alignof(T) ? sizeof(T) : alignof(T);
		while (size < required)
			size <<= 1;
		return size;
	}

	static constexpr size_t BlockSize = PoolBlockSize();

	std::pmr::unsynchronized_pool_resource _pool {
		// libc++ otherwise routes this size class through its O(n) ad-hoc pool.
		{2 * 1024 * 1024 / BlockSize, 4 * BlockSize}
	};

public:
	template <typename... Args>
		T *Construct(Args... args)
	{
		void *p = _pool.allocate(sizeof(T), alignof(T));
		if (!p) {
			return nullptr;
		}
		return new (p) T(args...);
	}

	void Destruct(T *p)
	{
		p->~T();
		_pool.deallocate(p, sizeof(T), alignof(T));
	}

	void Purge()
	{
		_pool.release();
	}
};
