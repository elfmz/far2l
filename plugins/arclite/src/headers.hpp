#pragma once

#include "PlatformConstants.h"
#include "compiler.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <winioctl.h>
#include <process.h>
#endif

#include <assert.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include <memory>
#include <string>
#include <string_view>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <cmath>
#include <cstring>
#include <queue>
#include <unordered_set>

#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
//#include <cinttypes>
#include <inttypes.h>

//#include <variant>
//#include <functional> // Для std::function (если используется)
//#include <visit>

#include <sys/resource.h>


/*
 */

WARNING_PUSH()
WARNING_DISABLE_MSC(
		5204)	 // 'type-name': class has virtual functions, but its trivial destructor is not virtual;
				 // instances of objects derived from this class may not be destructed correctly
WARNING_DISABLE_GCC("-Wsuggest-override")
WARNING_DISABLE_CLANG("-Weverything")

#ifdef _WIN32
#include <basetyps.h>
#endif

#define PROCPLUGINMACROFUNC 1

#include <farplug-wide.h>
#include <farcolor.h>
#include <farkeys.h>
#include <KeyFileHelper.h>
#include <utils.h>

#include <StringConfig.h>
#include <CriticalSections.hpp>
#include <InterThreadCall.hpp>
#include "sudo.h"

#include "locale.hpp"

#define HELLO_I_AM_FROM_ARCLITE 1
#define EXTERNAL_CODECS			1

#include "7z/h/CPP/7zip/Archive/IArchive.h"
#include "7z/h/CPP/7zip/IPassword.h"
#include "7z/h/CPP/7zip/ICoder.h"
#include "7z/h/CPP/Common/MyCom.h"

#ifndef Z7_ARRAY_SIZE
#define Z7_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifdef _MSC_VER
	#define	FAR_ALIGNED(x) __declspec( align(x) )
	#define	FN_STATIC_INLINE static __forceinline
	#define	FN_INLINE __forceinline
#else
	#define	FAR_ALIGNED(x) __attribute__ ((aligned (x)))
	#define	FN_STATIC_INLINE static __attribute__((always_inline)) inline
	#define	FN_INLINE __attribute__((always_inline)) inline
#endif

#ifndef IS_BIG_ENDIAN
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	#define IS_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
	#define IS_BIG_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
		defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || \
		defined(__MIPSEB__)
	#define IS_BIG_ENDIAN 1
#else
	#define IS_BIG_ENDIAN 0
#endif
#endif

#if IS_BIG_ENDIAN
#define le16_to_host(x) ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))
#define le32_to_host(x) ((((x) >> 24) & 0x000000FF) | \
						(((x) >> 8)  & 0x0000FF00) | \
						(((x) << 8)  & 0x00FF0000) | \
						(((x) << 24) & 0xFF000000))
#else
#define le16_to_host(x) (x)
#define le32_to_host(x) (x)
#endif

WARNING_POP()

extern GUID IID_IUnknown;
