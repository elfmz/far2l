#pragma once
#include "WinCompat.h"
#include "WinPort.h"
#include <cctweaks.h>
#include <vector>
#include <atomic>

class WinPortHandle;

class WinPortHandle
{
	volatile uint32_t _magic;

protected:
	virtual ~WinPortHandle(); // use Deregister to delete

	virtual bool Cleanup() noexcept = 0;

public:
	static constexpr uint32_t TypeMagicMask   = 0x00000001; // one bit for type-specific magic (file/registry)
	static constexpr uint32_t CommonMagicMask = ~TypeMagicMask;
	static constexpr uint32_t CommonMagicGood = 0xcafebabe;
	static constexpr uint32_t CommonMagicBad  = CommonMagicGood ^ CommonMagicMask;

	WinPortHandle(uint32_t type_magic);

	HANDLE Register();
	static WinPortHandle *Access(HANDLE h, uint32_t type_magic) noexcept;
	static bool Deregister(HANDLE h) noexcept;
};

template <uint32_t MAGIC>
	struct MagicWinPortHandle : WinPortHandle
{
	static constexpr uint32_t TypeMagic = MAGIC;

	MagicWinPortHandle() : WinPortHandle(TypeMagic)
	{
		static_assert((TypeMagic & WinPortHandle::CommonMagicMask) == 0, "Bad MAGIC");
	}
};

template <class T>
	class AutoWinPortHandle
{
	T *_p;

public:
	AutoWinPortHandle(HANDLE h) : _p((T *)WinPortHandle::Access(h, T::TypeMagic))
	{
	}

	operator bool()
	{
		return _p != nullptr;
	}

	T * operator -> ()
	{
		return _p;
	}

	T * get()
	{
		return _p;
	}
};
