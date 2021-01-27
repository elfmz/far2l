#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include "../WinPort/sudo.h"
#include <sys/mman.h>
#include <sys/stat.h>

#include <exception>
#include <stdexcept>
#include <functional>
#include <set>
#include <atomic>

#include <utils.h>
#include <ScopeHelpers.h>
#include "SafeMMap.hpp"

//#define PRINT_FAULTS

#ifdef PRINT_FAULTS
#include <sys/time.h>
#include <sys/resource.h>
static struct rusage s_usg{};
#endif


static std::set<SafeMMap *> s_safe_mmaps;
static std::atomic<int> s_safe_mmaps_sl;

// Cannot use mutexes etc from signal handler context
// (see https://man7.org/linux/man-pages/man7/signal-safety.7.html)
// so use simple spinlock instead to synchronize access to s_safe_mmaps
class SMM_Lock
{

public:
	SMM_Lock()
	{
		for (int z = 0; !s_safe_mmaps_sl.compare_exchange_strong(z, 1); z = 0) {
			usleep(1);
		}
	}

	~SMM_Lock()
	{
		s_safe_mmaps_sl = 0;
	}
};

static void SignalSafeLToA(long l, char *str, size_t last_char_ofs)
{
	while (l) {
		char x = (l % 16);
		str[last_char_ofs] = (x < 10) ? '0' + x : 'A' + x;
		l/= 16;
		if (!last_char_ofs) break;
		--last_char_ofs;
	}
}

static std::string s_crash_log;

static void InvokePrevSigaction(int num, siginfo_t *info, void *ctx, struct sigaction &prev_sa)
{

	char errmsg[] = "\nSignal 00h [0000000000000000h]\n";
	//                0123456789abcdef0123456789abcdef

	if (prev_sa.sa_flags & SA_SIGINFO) {
		if ((void *)prev_sa.sa_sigaction != (void *)SIG_IGN
				&& (void *)prev_sa.sa_sigaction != (void *)SIG_DFL
				&& (void *)prev_sa.sa_sigaction != (void *)SIG_ERR) {
			prev_sa.sa_sigaction(num, info, ctx);
		}

	} else if ((void *)prev_sa.sa_handler != (void *)SIG_IGN
			&& (void *)prev_sa.sa_handler != (void *)SIG_DFL
			&& (void *)prev_sa.sa_handler != (void *)SIG_ERR) {
		prev_sa.sa_handler(num);
	}

	SignalSafeLToA(num, errmsg, 0x09);
	SignalSafeLToA((long)info->si_addr, errmsg, 0x1c);

	int fd = open(s_crash_log.c_str(), O_APPEND | O_CREAT | O_WRONLY, 0600);
	if (fd != -1) {
		if (write(STDERR_FILENO, errmsg, strlen(errmsg)) == -1) {
			close(fd);
			fd = STDERR_FILENO;
		}
	} else {
		fd = STDERR_FILENO;
	}
	if (write(fd, errmsg, strlen(errmsg)) == -1) {
		perror("write(errmsg)");
	}
#ifndef __FreeBSD__ // todo: pass to linker -lexecinfo under BSD and then may remove this ifndef
	void *bt[16];
	size_t bt_count = sizeof(bt) / sizeof(bt[0]);
	bt_count = backtrace(bt, bt_count);
	backtrace_symbols_fd(bt, bt_count, fd);
	if (fd != STDERR_FILENO) {
		close(fd);
	}
#endif
	abort();
}

static struct sigaction s_prev_sa_bus {}, s_prev_sa_segv {};

void SafeMMap::sSigaction(int num, siginfo_t *info, void *ctx)
{
	{
		SMM_Lock sl;
		for (const auto &smm : s_safe_mmaps) {
			if ((uintptr_t)info->si_addr >= (uintptr_t)smm->_view && 
				(uintptr_t)info->si_addr - (uintptr_t)smm->_view < (uintptr_t)smm->_len) {
				if (!smm->_remapped) {
					smm->_remapped = true;
					mmap(smm->_view, smm->_len, smm->_prot,
						MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
					return;
				}
			}
		}
	}

	if (num == SIGSEGV) {
		InvokePrevSigaction(num, info, ctx, s_prev_sa_segv);

	} else if (num == SIGBUS) {
		InvokePrevSigaction(num, info, ctx, s_prev_sa_bus);

	} else {
		abort();
	}
}

void SafeMMap::sRegisterSignalHandler()
{
	if (s_crash_log.empty()) {
		s_crash_log = InMyConfig("crash.log");
	}

	struct sigaction sa{};
	sa.sa_sigaction = sSigaction;
	sa.sa_flags = SA_NODEFER | SA_RESTART | SA_SIGINFO;

	sigaction(SIGBUS, &sa, &s_prev_sa_bus);
	sigaction(SIGSEGV, &sa, &s_prev_sa_segv);
}

void SafeMMap::sUnregisterSignalHandler()
{
	struct sigaction tmp{};
	sigaction(SIGBUS, &s_prev_sa_bus, &tmp);
	sigaction(SIGSEGV, &s_prev_sa_segv, &tmp);
}


SafeMMap::SafeMMap(const char *path, enum Mode m, size_t len_limit)
	: _prot((m == M_READ) ? PROT_READ : PROT_WRITE)
{
#ifdef PRINT_FAULTS
	getrusage(RUSAGE_SELF, &s_usg);
#endif
	FDScope fd(sdc_open(path, (m == M_WRITE) ? O_RDWR : O_RDONLY));
	if (fd == -1) {
		throw std::runtime_error(StrPrintf("Open error %u", errno));
	}

	struct stat s{};
	if (sdc_fstat(fd, &s) != 0) {
		throw std::runtime_error(StrPrintf("Stat error %u", errno));
	}

	_len = std::min((size_t)s.st_size, len_limit);
	if (_len == 0) {
		return;
	}

	_view = mmap(NULL, _len, _prot, (m == M_WRITE) ? MAP_SHARED : MAP_PRIVATE, fd, 0);
	if (!_view || (_view == MAP_FAILED))
		throw std::runtime_error(StrPrintf("Map error %u", errno));

	SMM_Lock sl;
	s_safe_mmaps.insert(this);
}

SafeMMap::~SafeMMap()
{
	if (_view) {
		{
			SMM_Lock sl;
			s_safe_mmaps.erase(this);
		}
		munmap(_view, _len);
#ifdef PRINT_FAULTS
		struct rusage usg {};
		getrusage(RUSAGE_SELF, &usg);
		fprintf(stderr, "FAULTS: %u %u\n", usg.ru_majflt - s_usg.ru_majflt, usg.ru_minflt - s_usg.ru_minflt);
#endif
	}
}
