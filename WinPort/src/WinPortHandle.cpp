#include "WinPortHandle.h"
#include "WinCompat.h"
#include "WinPort.h"
#include <utils.h>

// #define DEBUG_HANDLE_LEAK

#ifdef DEBUG_HANDLE_LEAK
static std::atomic<long> s_handles_count{0};
static std::atomic<long> s_handles_count_max{0};
static void DebugHandleLeakIncrement()
{
	long v = ++s_handles_count;
	if (v > s_handles_count_max) {
		s_handles_count_max = v;
		fprintf(stderr, " !!! DebugHandleLeakIncrement: maximum updated to %ld\n", v);
	}
}

# define DEBUG_HANDLE_LEAK_INCREMENT DebugHandleLeakIncrement()
# define DEBUG_HANDLE_LEAK_DECREMENT --s_handles_count


#else
# define DEBUG_HANDLE_LEAK_INCREMENT
# define DEBUG_HANDLE_LEAK_DECREMENT
#endif

WinPortHandle::WinPortHandle(uint32_t type_magic)
	: _magic(type_magic | CommonMagicGood)
{
	DEBUG_HANDLE_LEAK_INCREMENT;
}

WinPortHandle::~WinPortHandle()
{
	DEBUG_HANDLE_LEAK_DECREMENT;
	ASSERT_MSG((_magic & CommonMagicMask) == CommonMagicGood, "bad magic=0x%x", _magic);
	_magic = CommonMagicBad | (_magic & TypeMagicMask);
}

HANDLE WinPortHandle::Register()
{
	return reinterpret_cast<HANDLE>(this);
}

bool WinPortHandle::Deregister(HANDLE h) noexcept
{
	WinPortHandle *wph = reinterpret_cast<WinPortHandle *>(h);
	bool out = wph->Cleanup();
	int saved_errno;
	if (!out) {
		saved_errno = errno;
	}
	delete wph;
	if (!out) {
		errno = saved_errno;
	}
	return out;
}

WinPortHandle *WinPortHandle::Access(HANDLE h, uint32_t type_magic) noexcept
{
	if (UNLIKELY(!h || h == INVALID_HANDLE_VALUE)) {
		return nullptr;
	}

	WinPortHandle *wph = reinterpret_cast<WinPortHandle *>(h);
	const uint32_t expected = CommonMagicGood | type_magic;
	ASSERT_MSG(wph->_magic == expected, "bad magic=0x%x expected=0x%x", wph->_magic, expected);
	return wph;
}
