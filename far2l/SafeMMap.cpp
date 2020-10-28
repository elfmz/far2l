#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include "../WinPort/sudo.h"
#include <sys/mman.h>
#include <sys/stat.h>

#include <exception>
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

static void InvokePrevSigaction(int num, siginfo_t *info, void *ctx, struct sigaction &prev_sa)
{
	if (prev_sa.sa_flags & SA_SIGINFO) {
		prev_sa.sa_sigaction(num, info, ctx);

	} else {
		prev_sa.sa_handler(num);
	}
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
	FDScope fd(sdc_open(path, O_RDONLY));
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
	if (!_view)
		throw std::runtime_error(StrPrintf("Alloc error %u", errno));

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
