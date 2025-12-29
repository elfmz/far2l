#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <time.h>
#include <errno.h>
#include "WinCompat.h"
#include "WinPort.h"

#ifdef __APPLE__
// # include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/vm_statistics.h>
#elif !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__DragonFly__) && !defined(__HAIKU__)
#include <sys/sysinfo.h>
#endif

WINPORT_DECL(GetDiskFreeSpaceW, BOOL, (LPCWSTR lpRootPathName,LPDWORD lpSectorsPerCluster,LPDWORD lpBytesPerSector,LPDWORD lpNumberOfFreeClusters,LPDWORD lpTotalNumberOfClusters))
{




	return TRUE;
}

static int ToPercent64(uint64_t N1, uint64_t N2)
{
	if (N1 > 10000) {
		N1/= 100;
		N2/= 100;
	}

	if (!N2)
		return 0;

	if (N2 < N1)
		return 100;

	return static_cast<int>(N1 * 100 / N2);
}

WINPORT_DECL(GlobalMemoryStatusEx, BOOL, (LPMEMORYSTATUSEX ms))
{
	ms->dwMemoryLoad = 50;
	ms->ullTotalPhys = 1024ull * 1024ull * 1024ull * 4ull;
	ms->ullAvailPhys = 1024ull * 1024ull * 1024ull * 2ull;

	ms->ullTotalPageFile = 0;
	ms->ullAvailPageFile = 0;

	ms->ullTotalVirtual = 0;
	ms->ullAvailVirtual = 0;
	ms->ullAvailExtendedVirtual = 0;

#ifdef __APPLE__
	unsigned long long totalram;
	vm_size_t page_size;
	unsigned long long freeram;
	int ret_sc;

	// ret_sc =  (sysctlbyname("hw.memsize", &totalram, &ulllen, NULL, 0) ? 1 : 0);
	ret_sc = (KERN_SUCCESS != _host_page_size(mach_host_self(), &page_size) ? 1 : 0);

	mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
	vm_statistics_data_t vmstat;

	ret_sc += (KERN_SUCCESS != host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count)
					? 1
					: 0);
	totalram =
			(vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count) * page_size;
	freeram = vmstat.free_count * page_size;

	// double total = vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count;
	// double wired = vmstat.wire_count;
	// double active = vmstat.active_count;
	// double inactive = vmstat.inactive_count;
	// double free = vmstat.free_count;

	if (!ret_sc) {
		ms->dwMemoryLoad = 100 - ToPercent64(freeram, totalram);

		ms->ullTotalPhys = totalram;
		ms->ullAvailPhys = freeram;
		return TRUE;
	}

#elif !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__DragonFly__) && !defined(__HAIKU__)
	struct sysinfo si = {};
	if (sysinfo(&si) == 0) {
		ms->dwMemoryLoad = 100 - ToPercent64(si.freeram + si.freeswap, si.totalram + si.totalswap);

		ms->ullTotalPhys = si.totalram;
		ms->ullAvailPhys = si.freeram;

		ms->ullTotalPageFile = si.totalswap;
		ms->ullAvailPageFile = si.freeswap;
		return TRUE;
	}
#endif

	return FALSE;
}

