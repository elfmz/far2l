#define _XOPEN_SOURCE // macos wants it for ucontext

#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>

#if !defined(__FreeBSD__) && !defined(__DragonFly__) && !defined(__MUSL__) && !defined(__UCLIBC__) && !defined(__HAIKU__) && !defined(__ANDROID__) // todo: pass to linker -lexecinfo under BSD and then may remove this ifndef
# include <execinfo.h>
# define HAS_BACKTRACE
#endif

#include <fcntl.h>
#include <stdio.h>
#include "../WinPort/sudo.h"
#include <sys/mman.h>
#include <sys/stat.h>
#if !defined(__HAIKU__)
#include <ucontext.h>
#endif

#include <exception>
#include <stdexcept>
#include <functional>
#include <set>
#include <atomic>

#include <utils.h>
#include <windows.h>
#include "SafeMMap.hpp"
#include "farversion.h"

//#define PRINT_FAULTS

#ifdef PRINT_FAULTS
#include <sys/time.h>
#include <sys/resource.h>
static struct rusage s_usg{};
#endif

static std::set<SafeMMap *> s_safe_mmaps;
static std::atomic<int> s_safe_mmaps_sl{0};

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
		str[last_char_ofs] = (x < 10) ? '0' + x : 'A' + (x - 10);
		l/= 16;
		if (!last_char_ofs) break;
		--last_char_ofs;
	}
}

static std::string s_crash_log;

static void FDWriteStr(int fd, const char *str)
{
	if (write(fd, str, strlen(str)) == -1) {
		perror("FDWrite - write");
	}
}

static void FDWriteSignalInfo(int fd, int num, siginfo_t *info, void *ctx)
{
	FDWriteStr(fd, "\n🔥🔥🔥 ");
	FDWriteStr(fd, "far2l ");
	FDWriteStr(fd, FAR_BUILD);
	FDWriteStr(fd, " ");
	FDWriteStr(fd, FAR_PLATFORM);
	char errmsg[] = "\nSignal 00 [0000000000000000] on 0000000000000000\n";
	//                0123456789abcdef0123456789abcdef0123456789abcdef0
	//             0x0^            0x1^            0x2^            0x3^
	SignalSafeLToA(num, errmsg, 0x09);
	SignalSafeLToA((long)info->si_addr, errmsg, 0x1b);
	SignalSafeLToA((long)time(NULL), errmsg, 0x30);
	FDWriteStr(fd, errmsg);

	const ucontext_t *uctx = (const ucontext_t *)ctx;
	const long *mctx = (const long *)&uctx->uc_mcontext;
	const size_t mctx_count = sizeof(uctx->uc_mcontext) / sizeof(*mctx);

	for (size_t i = 0; i < mctx_count; ++i) {
		if (i == 0) {
			;
		} else if ((i & 3) == 0) {
			FDWriteStr(fd, "\n");
		} else {
			FDWriteStr(fd, " ");
		}

		char val[] = "0000000000000000";
		//            0123456789abcdef
		SignalSafeLToA(mctx[i], val, 0x0f);
		FDWriteStr(fd, val);
	}
	FDWriteStr(fd, "\n");
}

static inline void WriteCrashSigLog(int num, siginfo_t *info, void *ctx)
{
	FDScope fd(open(s_crash_log.c_str(), O_APPEND | O_CREAT | O_WRONLY, 0600));
	if (fd.Valid()) {
		FDWriteSignalInfo(fd, num, info, ctx);
#ifdef HAS_BACKTRACE
		// using backtrace/backtrace_symbols_fd is in general now allowed by signal safety rules
		// but in general it works and its enough cuz other important info already written in
		// signal-safe manner by FDWriteSignalInfo
		void *bt[16];
		size_t bt_count = sizeof(bt) / sizeof(bt[0]);
		bt_count = backtrace(bt, bt_count);
		backtrace_symbols_fd(bt, bt_count, fd);
#endif
		fsync(fd);
		void **stk = (void **)&ctx;
		for (unsigned int i = 0; i < 0x1000 && ((uintptr_t(&stk[i]) ^ uintptr_t(stk)) < 0x1000); ++i) {
			Dl_info dli = {0};
			if (dladdr(stk[i], &dli) && dli.dli_fname) {
				FDWriteStr(fd, dli.dli_fname);
				char tail[32];
				snprintf(tail, sizeof(tail), " + 0x%x\n", unsigned(uintptr_t(stk[i]) - uintptr_t(dli.dli_fbase)));
				FDWriteStr(fd, tail);
			}
		}
		fsync(fd);
	}

	FDWriteSignalInfo(STDERR_FILENO, num, info, ctx);
}

static void InvokePrevSigaction(int num, siginfo_t *info, void *ctx, struct sigaction &prev_sa)
{
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
}

static struct sigaction s_prev_sa_bus {}, s_prev_sa_segv {};

void SafeMMap::sSigaction(int num, siginfo_t *info, void *ctx)
{
	{
		SMM_Lock sl;
		for (const auto &smm : s_safe_mmaps) {
			if ((uintptr_t)info->si_addr >= (uintptr_t)smm->_view &&
				(uintptr_t)info->si_addr - (uintptr_t)smm->_view < (uintptr_t)smm->_len) {
				if (!smm->_dummy) {
					smm->_dummy = true;
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
	}

	WriteCrashSigLog(num, info, ctx);
	ABORT_MSG("see %s", s_crash_log.c_str());
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
	:
	_fd(sdc_open(path, (m == M_WRITE) ? O_RDWR : O_RDONLY)),
	_pg(getpagesize()),
	_prot((m == M_READ) ? PROT_READ : PROT_WRITE),
	_flags((m == M_WRITE) ? MAP_SHARED : MAP_PRIVATE)
{
	if (!_fd.Valid()) {
		ThrowPrintf("Open error %u", errno);
	}

#ifdef PRINT_FAULTS
	getrusage(RUSAGE_SELF, &s_usg);
#endif

	struct stat s{};
	if (sdc_fstat(_fd, &s) != 0) {
		ThrowPrintf("Stat error %u", errno);
	}

	_file_size = s.st_size;

	_len = (size_t)std::min(s.st_size, (off_t)len_limit);
	if (_len == 0) {
		return;
	}

	_view = mmap(NULL, _len, _prot, _flags, _fd, 0);
	if (!_view || (_view == MAP_FAILED))
		ThrowPrintf("SafeMMap::SafeMMap: mmap error %u", errno);

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
		if (munmap(_view, _len) == -1) {
			perror("~SafeMMap: munmap");
		}
#ifdef PRINT_FAULTS
		struct rusage usg {};
		getrusage(RUSAGE_SELF, &usg);
		fprintf(stderr, "FAULTS: %u %u\n", usg.ru_majflt - s_usg.ru_majflt, usg.ru_minflt - s_usg.ru_minflt);
#endif
	}
}

void SafeMMap::Slide(off_t file_offset)
{
	if (file_offset >= _file_size) {
		fprintf(stderr, "SafeMMap::Slide: file_offset[%llx] >= _file_size[%llx]\n",
			(unsigned long long)file_offset, (unsigned long long)_file_size);
	}

	const size_t new_len = (size_t)std::min((off_t)_len, _file_size - file_offset);
	// In documentation only BSD and Linux clearly stating that using MAP_FIXED
	// unmaps previous mapping(s) from affected address range.
	// So for that systems use approach looking most optimal: remap same pages to
	// different region of file. At least this should allow VMM to avoid searching
	// for free pages as well as reduce syscalls count by avoiding call to munmap().
#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
	void *new_view = mmap(_view, new_len, _prot, _flags | MAP_FIXED, _fd, file_offset);
#else
	void *new_view = mmap(nullptr, new_len, _prot, _flags, _fd, file_offset);
#endif
	if (!new_view || (new_view == MAP_FAILED)) {
		ThrowPrintf("SafeMMap::Slide: mmap error %u", errno);
	}

	if (_view != new_view) {
#if !defined(__linux__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
		fprintf(stderr, "SafeMMap::Slide: _view[%p] != new_view[%p]\n", _view, new_view);
#endif
		if (munmap(_view, _len) == -1) {
			perror("SafeMMap::Slide: munmap prev range");
		}
		_view = new_view;
		_len = new_len;

	} else if (new_len < _len) {
		const size_t al_new_len = AlignUp(new_len, _pg);
		const size_t al_len = AlignUp(_len, _pg);
		if (al_new_len < al_len) {
			//	From documentation:
			//		If the memory region specified by addr and len overlaps pages of any existing
			//		mapping(s), then the overlapped part of the existing mapping(s) will be discarded
			//	That means if not whole range remapped - then have to explicitly unmap remainder.
			if (munmap((unsigned char *)_view + al_new_len, al_len - al_new_len) == -1) {
				perror("SafeMMap::Slide: munmap remainder");
			}
		}
		_len = new_len;
	}

	if (_dummy) {
		_dummy = false;
		fprintf(stderr, "SafeMMap::Slide(0x%llx): dummy slided away\n", (unsigned long long)file_offset);
	}


//	fprintf(stderr, "SafeMMap::Slide: _view=%p len=%lx\n", _view, _len);
}
