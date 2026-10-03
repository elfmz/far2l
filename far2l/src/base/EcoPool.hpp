#pragma once
#include <stdlib.h>
#include <stdio.h>

template <class T, size_t MAX_ARENA = (32 * 1024 * 1024) / sizeof(T)>
	class EcoPool
{
	union U
	{
		T placement;
		U *next_free;
	};
	U *_free_list{nullptr};
	std::vector<std::pair<U *, size_t>> _arenas;

	static size_t ArenaSize(size_t index)
	{
		const size_t out = size_t(1) << std::min(size_t(31), index * 2);
		return (out < MAX_ARENA) ? out : MAX_ARENA;
	}

public:
	~EcoPool()
	{
		Purge();
	}

	template <typename... Args>
		T *Construct(Args... args) noexcept
	{
		T *p;
		if (_free_list) {
			p = &_free_list->placement;
			_free_list = _free_list->next_free;
		} else {
			if (_arenas.empty() || _arenas.back().second >= ArenaSize(_arenas.size() - 1)) {
				U *new_arena = (U *)malloc(sizeof(U) * ArenaSize(_arenas.size()));
				if (!new_arena) {
					fprintf(stderr, "EcoPool<%lu>: failed to allocate arena %lu\n", (unsigned long)sizeof(T), (unsigned long)_arenas.size());
					return nullptr;
				}
				_arenas.emplace_back(std::make_pair(new_arena, (size_t)0));
			}
			auto &cur = _arenas.back();
			p = &cur.first[cur.second].placement;
			cur.second++;
		}
		try {
			return new (p) T(args...);
		} catch (std::exception &e) {
			fprintf(stderr, "EcoPool<%lu>: exception '%s'\n", (unsigned long)sizeof(T), e.what());
		} catch (...) {
			fprintf(stderr, "EcoPool<%lu>: exception\n", (unsigned long)sizeof(T));
		}
		Free(p);
		return nullptr;
	}

	void Destruct(T *p) noexcept
	{
		if (p) {
			p->~T();
			Free(p);
		}
	}

	void Free(T *p) noexcept
	{
		auto *prev_free_list = _free_list;
		_free_list = (U *)p;
		_free_list->next_free = prev_free_list;
	}

	void Purge() noexcept
	{
		for (auto &a : _arenas) {
			free(a.first);
		}
		_arenas.clear();
		_free_list = nullptr;
	}
};
