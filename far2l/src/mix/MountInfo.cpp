#include "headers.hpp"

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__CYGWIN__)
# if defined(__APPLE__) || defined(__FreeBSD__)  || defined(__DragonFly__)
#  include <sys/param.h>
#  include <sys/ucred.h>
# endif
# include <sys/mount.h>
#elif defined(__HAIKU__)
#  include <kernel/fs_info.h>
#else
# include <sys/statfs.h>
# include <linux/fs.h>
#endif
#include <errno.h>
#include <fstream>
#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "MountInfo.h"
#include <ScopeHelpers.h>
#include <Threaded.h>
#include <config.hpp>
#include <os_call.hpp>

#define DISK_SPACE_QUERY_TIMEOUT_MSEC 1000

static struct FSMagic {
	const char *name;
	unsigned int magic;
} s_fs_magics[] = {
{"ADFS",	0xadf5},
{"AFFS",	0xadff},
{"AFS",                0x5346414F},
{"AUTOFS",	0x0187},
{"CODA",	0x73757245},
{"CRAMFS",		0x28cd3d45},	/* some random number */
{"CRAMFS",	0x453dcd28},		/* magic number with the wrong endianness */
{"DEBUGFS",          0x64626720},
{"SECURITYFS",	0x73636673},
{"SELINUX",		0xf97cff8c},
{"SMACK",		0x43415d53},	/* "SMAC" */
{"RAMFS",		0x858458f6},	/* some random number */
{"TMPFS",		0x01021994},
{"HUGETLBFS", 	0x958458f6},	/* some random number */
{"SQUASHFS",		0x73717368},
{"ECRYPTFS",	0xf15f},
{"EFS",		0x414A53},
{"EXT2",	0xEF53},
{"EXT3",	0xEF53},
{"XENFS",	0xabba1974},
{"EXT4",	0xEF53},
{"BTRFS",	0x9123683E},
{"NILFS",	0x3434},
{"F2FS",	0xF2F52010},
{"HPFS",	0xf995e849},
{"ISOFS",	0x9660},
{"JFFS2",	0x72b6},
{"PSTOREFS",		0x6165676C},
{"EFIVARFS",		0xde5e81e4},
{"HOSTFS",	0x00c0ffee},

{"MINIX",	0x137F},		/* minix v1 fs, 14 char names */
{"MINIX",	0x138F},		/* minix v1 fs, 30 char names */
{"MINIX2",	0x2468},		/* minix v2 fs, 14 char names */
{"MINIX2",	0x2478},		/* minix v2 fs, 30 char names */
{"MINIX3",	0x4d5a},		/* minix v3 fs, 60 char names */

{"MSDOS",	0x4d44},		/* MD */
{"NCP",		0x564c},		/* Guess, what 0x564c is :-) */
{"NFS",		0x6969},
{"OPENPROM",	0x9fa1},
{"QNX4",	0x002f},		/* qnx4 fs detection */
{"QNX6",	0x68191122},	/* qnx6 fs detection */

{"REISERFS",	0x52654973},	/* used by gcc */
								/* used by file system utilities that
								look at the superblock, etc.  */
{"SMB",		0x517B},
{"CGROUP",	0x27e0eb},


{"STACK_END",		0x57AC6E9D},

{"TRACEFS",          0x74726163},

{"V9FS",		0x01021997},

{"BDEVFS",            0x62646576},
{"BINFMTFS",          0x42494e4d},
{"DEVPTS",	0x1cd1},
{"FUTEXFS",	0xBAD1DEA},
{"PIPEFS",            0x50495045},
{"PROC",	0x9fa0},
{"SOCKFS",		0x534F434B},
{"SYSFS",		0x62656572},
{"USBDEVICE",	0x9fa2},
{"MTD_INODE_FS",      0x11307854},
{"ANON_INODE_FS",	0x09041934},
{"BTRFS_TEST",	0x73727279},
{"NSFS",		0x6e736673},
{"BPF_FS",		0xcafe4a11}};


/////////////////////////////////////////////////////////////////////////////////////////////

struct Mountpoints : std::vector<Mountpoint>
{
	struct Pending
	{
		std::mutex mtx;
		std::condition_variable cond;
		int cnt{0};
	} pending;
};

static std::atomic<unsigned int> s_mount_info_threads{0};

class ThreadedStatFS : Threaded
{
	std::shared_ptr<Mountpoints> _mps;
	size_t _mpi;

	void *ThreadProc()
	{
		struct statfs s{};
		int r = statfs((*_mps)[_mpi].path.c_str(), &s);
		if (r == 0) {
			(*_mps)[_mpi].total = ((unsigned long long)s.f_blocks) * s.f_bsize; //f_frsize;
			(*_mps)[_mpi].avail = ((unsigned long long)s.f_bavail) * s.f_bsize; //f_frsize;
			(*_mps)[_mpi].free_ = ((unsigned long long)s.f_bfree) * s.f_bsize; //f_frsize;
			(*_mps)[_mpi].read_only = (s.f_flags & ST_RDONLY) != 0;
			(*_mps)[_mpi].bad = false;
		}
		return nullptr;
	}

public:
	ThreadedStatFS(std::shared_ptr<Mountpoints> &mps, size_t mpi)
		: _mps(mps), _mpi(mpi)
	{
		++s_mount_info_threads;
	}

	virtual ~ThreadedStatFS()
	{
		--s_mount_info_threads;
		std::unique_lock<std::mutex> lock(_mps->pending.mtx);
		_mps->pending.cnt--;
		if (_mps->pending.cnt == 0) {
			_mps->pending.cond.notify_all();
		}

	}

	void Start()
	{
		if (!StartThread(true)) {
			fprintf(stderr, "ThreadedStatFS: can't start thread\n");
			delete this;
		}
	}
};

class LocationsMenuExceptions
{
	std::vector<std::string> _exceptions;

public:
	LocationsMenuExceptions()
	{
		StrExplode(_exceptions, Opt.ChangeDriveExceptions.GetMB(), ";");
		for (auto &exc : _exceptions) {
			StrTrim(exc);
		}
	}

	bool Match(const char *path)
	{
		for (const auto &exc : _exceptions) {
			if (MatchWildcardICE(path, exc.c_str())) {
				return true;
			}
		}
		return false;
	}
};

MountInfo::MountInfo(bool for_location_menu)
{
	if (!for_location_menu) {
		// force-enable multi-threaded disk access: echo e > ~/.config/far2l/mtfs
		// force-disable multi-threaded disk access: echo d > ~/.config/far2l/mtfs
		FDScope fd(open(InMyConfig("mtfs").c_str(), O_RDONLY));
		if (fd.Valid()) {
			if (os_call_ssize(read, (int)fd, (void *)&_mtfs, sizeof(_mtfs)) == 0) {
				_mtfs = 'e';
			}
			fprintf(stderr, "%s: _mtfs='%c'\n", __FUNCTION__, _mtfs);
		}
	}

	_mountpoints = std::make_shared<Mountpoints>();
	LocationsMenuExceptions lme;

#ifdef __linux__
	// manual parsing mounts file instead of using setmntent cuz later doesnt return
	// mounted device path that is needed to determine if its 'rotational'
	std::ifstream is("/etc/mtab");
	if (is.is_open()) {
		std::string line, sys_path;
		std::vector<std::string> parts;
		while (std::getline(is, line)) {
			parts.clear();
			StrExplode(parts, line, " \t");
			for (auto &part : parts) {
				Environment::UnescapeCLikeSequences(part);
			}
			if (parts.size() > 1 && StrStartsFrom(parts[1], "/")
				&& (!for_location_menu || !lme.Match(parts[1].c_str())))
			{
				bool multi_thread_friendly;
				if (for_location_menu) {
					// Location menu doesn't care about this, so dont waist time
					multi_thread_friendly = false;

				} else if (StrStartsFrom(parts[0], "/dev/")) {
					sys_path = "/sys/block/";
					sys_path+= parts[0].substr(5);
					// strip device suffix, sda1 -> sda, mmcblk0p1 -> mmcblk0
					struct stat s;
					while (sys_path.size() > 12 && stat(sys_path.c_str(), &s) == -1) {
						sys_path.resize(sys_path.size() - 1);
					}
					sys_path+= "/queue/rotational";
					char c = '?';
					FDScope fd(open(sys_path.c_str(), O_RDONLY));
					if (fd.Valid()) {
						os_call_ssize(read, (int)fd, (void *)&c, sizeof(c));
					} else {
						fprintf(stderr, "%s: can't read '%s'\n", __FUNCTION__, sys_path.c_str());
					}
					multi_thread_friendly = (c == '0');

				} else {
					multi_thread_friendly = (parts[0] == "tmpfs" || parts[0] == "proc");
				}
				if (parts.size() == 1) {
					parts.emplace_back();
				}
				_mountpoints->emplace_back(Mountpoint{
					parts[1],
					parts[2],
					parts[0],
					multi_thread_friendly,
					false,
					false,
					0, 0, 0
				});
			}
		}
	}

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)

	int r = getfsstat(nullptr, 0, MNT_NOWAIT);
	if (r > 0) {
		std::vector<struct statfs> buf(r * 2 + 2);
		r = getfsstat(buf.data(), buf.size() * sizeof(*buf.data()), MNT_NOWAIT);
		if (r > 0) {
			buf.resize(r);
			for (const auto &fs : buf) {
				if (!for_location_menu || !lme.Match(fs.f_mntonname)) {
					_mountpoints->emplace_back(Mountpoint{
						fs.f_mntonname,
						fs.f_fstypename,
						fs.f_mntfromname,
						true,
						false,
						false,
						((unsigned long long)fs.f_blocks) * fs.f_bsize, // unreliable due to MNT_NOWAIT
						((unsigned long long)fs.f_bavail) * fs.f_bsize, // ThreadedStatFS will set true nums
						((unsigned long long)fs.f_bfree) * fs.f_bsize  // ...
					});
				}
			}
		}
	}
#endif

	if (_mountpoints->empty()) {
		fprintf(stderr, "%s: no mountpoints found\n", __FUNCTION__);

	} else if (for_location_menu) {
		// TODO: honor Opt.RememberLogicalDrives
		if (s_mount_info_threads != 0) {
			fprintf(stderr, "%s: still %u old threads hanging around\n",
				__FUNCTION__, (unsigned int)s_mount_info_threads);
		}
		for (size_t i = 0; i < _mountpoints->size(); ++i) {
			try {
				(*_mountpoints)[i].bad = true;
				(new ThreadedStatFS(_mountpoints, i))->Start();
				std::unique_lock<std::mutex> lock(_mountpoints->pending.mtx);
				_mountpoints->pending.cnt++;

			} catch (std::exception &e) {
				fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			}
		}
		for (DWORD ms = 0; ;) {
			std::chrono::milliseconds ms_before = std::chrono::duration_cast< std::chrono::milliseconds >
				(std::chrono::steady_clock::now().time_since_epoch());
			{
				std::unique_lock<std::mutex> lock(_mountpoints->pending.mtx);
				if (_mountpoints->pending.cnt == 0) {
					break;
				}
				_mountpoints->pending.cond.wait_for(lock,
					std::chrono::milliseconds(DISK_SPACE_QUERY_TIMEOUT_MSEC - ms));
			}
			ms+= (std::chrono::duration_cast< std::chrono::milliseconds >
				(std::chrono::steady_clock::now().time_since_epoch()) - ms_before).count();
			if (ms >= DISK_SPACE_QUERY_TIMEOUT_MSEC) {
				fprintf(stderr, "%s: timed out\n", __FUNCTION__);
				break;
			}
		}
	}

	for (const auto &it : *_mountpoints) {
		fprintf(stderr, "%s: mtf=%d spc=[%llu/%llu] fs='%s' at '%s'\n", __FUNCTION__,
			it.multi_thread_friendly, it.avail, it.total, it.filesystem.c_str(), it.path.c_str());
	}
}

const std::vector<Mountpoint> &MountInfo::Enum() const
{
	return *_mountpoints;
}

std::string MountInfo::GetFileSystem(const std::string &path) const
{
	std::string out;
#ifndef __HAIKU__
	size_t longest_match = 0;
	for (const auto &it : *_mountpoints) {
		if (it.path.size() > longest_match && StrStartsFrom(path, it.path.c_str())) {
			longest_match = it.path.size();
			out = it.filesystem;
		}
	}

	if (out.empty()) {
		struct statfs sfs{};
		if (sdc_statfs(path.c_str(), &sfs) == 0) {
#ifdef __APPLE__
		out = sfs.f_fstypename;
		if (out.empty())
#endif
			for (size_t i = 0; i < ARRAYSIZE(s_fs_magics); ++i) {
				if (sfs.f_type == s_fs_magics[i].magic) {
					out = s_fs_magics[i].name;
					break;
				}
			}
		}
	}
#else
	dev_t dev = dev_for_path(path.c_str());
	if (dev >= 0) {
		fs_info fsinfo;
		if (fs_stat_dev(dev, &fsinfo) == B_OK)
			out = fsinfo.fsh_name;
	}
#endif
	return out;
}

bool MountInfo::IsMultiThreadFriendly(const std::string &path) const
{
	if (_mtfs != 0) {
		return (_mtfs == 'e' || _mtfs == 'E');
	}

	bool out = true;
	size_t longest_match = 0;
	for (const auto &it : *_mountpoints) {
		if (it.path.size() > longest_match && StrStartsFrom(path, it.path.c_str())) {
			longest_match = it.path.size();
			out = it.multi_thread_friendly;
		}
	}
	return out;
}

