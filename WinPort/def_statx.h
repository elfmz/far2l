#pragma once

#if defined(__linux__)
#	include <sys/syscall.h>
#	if defined(__has_include)
#		if __has_include(<linux/stat.h>)
#			include <linux/stat.h>
#		endif
#	endif
#endif

#if defined(__linux__)
#include <cstdint>

#define USE_STATX 1

#ifndef SYS_statx
#if defined(__x86_64__)
#define SYS_statx 332
#elif defined(__i386__)
#define SYS_statx 383
#elif defined(__aarch64__)
#define SYS_statx 291
#elif defined(__arm__)
#define SYS_statx 397
#else
#define SYS_statx -1
#undef USE_STATX
#define USE_STATX 0
#endif
#endif

#ifndef STATX_TYPE
#define STATX_TYPE					0x0001U
#define STATX_MODE					0x0002U
#define STATX_NLINK					0x0004U
#define STATX_UID					0x0008U
#define STATX_GID					0x0010U
#define STATX_ATIME					0x0020U
#define STATX_MTIME					0x0040U
#define STATX_CTIME					0x0080U
#define STATX_INO					0x0100U
#define STATX_SIZE					0x0200U
#define STATX_BLOCKS				0x0400U
#define STATX_BASIC_STATS			0x07ffU
#define STATX_ALL					0x0fffU
#define STATX_BTIME					0x0800U
#define STATX_MNT_ID				0x1000U
#define STATX_DIOALIGN				0x2000U
#define STATX_MNT_ID_UNIQUE			0x4000U
#define STATX_SUBVOL				0x8000U
#define STATX_WRITE_ATOMIC			0x00010000U
#define STATX__RESERVED				0x80000000U
#endif /* !STATX_TYPE */

#ifndef STATX_ATTR_COMPRESSED
	#define STATX_ATTR_COMPRESSED        0x00000004U
#endif
#ifndef STATX_ATTR_IMMUTABLE
	#define STATX_ATTR_IMMUTABLE         0x00000010U
#endif
#ifndef STATX_ATTR_APPEND
	#define STATX_ATTR_APPEND            0x00000020U
#endif
#ifndef STATX_ATTR_NODUMP
	#define STATX_ATTR_NODUMP            0x00000040U
#endif
#ifndef STATX_ATTR_ENCRYPTED
	#define STATX_ATTR_ENCRYPTED         0x00000080U
#endif
#ifndef STATX_ATTR_AUTOMOUNT
	#define STATX_ATTR_AUTOMOUNT         0x00000100U
#endif
#ifndef STATX_ATTR_SPARSE
	#define STATX_ATTR_SPARSE            0x00000200U
#endif
#ifndef STATX_ATTR_NOATIME
	#define STATX_ATTR_NOATIME           0x00000800U
#endif
#ifndef STATX_ATTR_SYNC
	#define STATX_ATTR_SYNC              0x00001000U
#endif
#ifndef STATX_ATTR_DAX
	#define STATX_ATTR_DAX               0x00002000U
#endif
#ifndef STATX_ATTR_VERITY
	#define STATX_ATTR_VERITY            0x00004000U
#endif
#ifndef STATX_ATTR_PROJINHERIT
	#define STATX_ATTR_PROJINHERIT       0x00008000U
#endif
#ifndef STATX_ATTR_CASEFOLD
	#define STATX_ATTR_CASEFOLD          0x00010000U
#endif
#ifndef STATX_ATTR_NOCOW
	#define STATX_ATTR_NOCOW             0x00020000U
#endif
#ifndef STATX_ATTR_ROOTID
	#define STATX_ATTR_ROOTID            0x00040000U
#endif
#ifndef STATX_ATTR_MOUNT_ROOT
	#define STATX_ATTR_MOUNT_ROOT        0x00100000U
#endif
#ifndef STATX_ATTR_VERITY_ENFORCE
	#define STATX_ATTR_VERITY_ENFORCE    0x00200000U
#endif
#ifndef STATX_ATTR_INODE_PINNED
	#define STATX_ATTR_INODE_PINNED      0x01000000U
#endif
#ifndef STATX_ATTR_INODE_UNPINNED
	#define STATX_ATTR_INODE_UNPINNED    0x02000000U
#endif

struct __statx_timestamp
{
	__int64_t tv_sec;
	__uint32_t tv_nsec;
	__int32_t __statx_timestamp_pad1[1];
};

struct __statx
{
	uint32_t stx_mask;
	uint32_t stx_blksize;
	uint64_t stx_attributes;
	uint32_t stx_nlink;
	uint32_t stx_uid;
	uint32_t stx_gid;
	uint16_t stx_mode;
	uint16_t __statx_pad1[1];
	uint64_t stx_ino;
	uint64_t stx_size;
	uint64_t stx_blocks;
	uint64_t stx_attributes_mask;
	struct __statx_timestamp stx_atime;
	struct __statx_timestamp stx_btime;
	struct __statx_timestamp stx_ctime;
	struct __statx_timestamp stx_mtime;
	uint32_t stx_rdev_major;
	uint32_t stx_rdev_minor;
	uint32_t stx_dev_major;
	uint32_t stx_dev_minor;
	uint64_t __statx_pad2[14];
};

#else

#define USE_STATX 0

#endif // defined(__linux__)

