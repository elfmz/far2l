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

#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

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

WARNING_POP()

extern GUID IID_IUnknown;
