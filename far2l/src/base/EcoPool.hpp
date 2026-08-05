#pragma once
#include <memory_resource>

template <class T>
	class EcoPool
{
	std::pmr::unsynchronized_pool_resource _pool {
		{2 * 1024 * 1024 / sizeof(T), sizeof(T) }
	};

public:
	template <typename... Args>
		T *Construct(Args... args)
	{
		void *p = _pool.allocate(sizeof(T), 2 * sizeof(void *));
		if (!p) {
			return nullptr;
		}
		return new (p) T(args...);
	}

	void Destruct(T *p)
	{
		p->~T();
		_pool.deallocate(p, sizeof(T), 2 * sizeof(void *));
	}

	void Purge()
	{
		_pool.release();
	}
};
