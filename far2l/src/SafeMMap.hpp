#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>

/**
This class serves two purposes:
 1. Provides fail-safe memory mapped view of file, in case
of getting SIGBUS/SIGSEGV due to underlying FS failure it silently
substitudes file's view with a dummy zero-filled view.
The only way to be informed about IO error is to use IsDummy()
 2. For unrelevant to mappings SIGBUS/SIGSEGV it generates
crash reports writing them into ~/.config/far2l/crash.log
*/

class SafeMMap
{
	friend struct SignalHandlerRegistrar;

	void *_view = nullptr;
	size_t _len = 0;
	unsigned int _prot;
	volatile bool _remapped = false;

	static void sSigaction(int num, siginfo_t *info, void *ctx);
	static void sRegisterSignalHandler();
	static void sUnregisterSignalHandler();

	SafeMMap(const SafeMMap&) = delete;


public:

	struct SignalHandlerRegistrar
	{
		SignalHandlerRegistrar() { SafeMMap::sRegisterSignalHandler(); } 
		~SignalHandlerRegistrar() { SafeMMap::sUnregisterSignalHandler(); } 
	};

	enum Mode
	{
		M_READ,
		M_WRCOPY,
		M_WRITE
	};

	SafeMMap(const char *path, enum Mode m, size_t len_limit = (size_t)-1);
	~SafeMMap();

	inline bool IsDummy() const { return _remapped; }

	inline void *View() { return _view; }
	inline size_t Length() { return _len; }
};
