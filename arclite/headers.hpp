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

#ifndef Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN
//	#define Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN 1
#endif

#if 0
#include "7z/CPP/7zip/Archive/IArchive.h"
#include "7z/CPP/7zip/IPassword.h"
#include "7z/CPP/7zip/ICoder.h"
#else
//ddddddddddd
#include "7z/h/CPP/7zip/Archive/IArchive.h"
#include "7z/h/CPP/7zip/IPassword.h"
#include "7z/h/CPP/7zip/ICoder.h"
#include "7z/h/CPP/Common/MyCom.h"
//	#include "7z/h2/CPP/7zip/Archive/IArchive.h"
//	#include "7z/h2/CPP/7zip/IPassword.h"
//	#include "7z/h2/CPP/7zip/ICoder.h"
#endif

WARNING_POP()

extern GUID IID_IUnknown;
