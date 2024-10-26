#include <limits.h>
#ifdef __linux__
# include <linux/limits.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 0x1000
#endif

#ifndef NAME_MAX
#define NAME_MAX 0xff
#endif

#if !defined(__LP64__) && defined(_LP64)
#define __LP64__
#endif

#define FAR_USE_INTERNALS
#define PROCPLUGINMACROFUNC
#define UNICODE
//#define WINPORT_DIRECT
//#define WINPORT_REGISTRY
#define _FAR_NO_NAMELESS_UNIONS
#define FAR_PYTHON_GEN

#define uid_t uint32_t
#define gid_t uint32_t
#include "farplug-wide.h"
#include "farcolor.h"
#include "farkeys.h"

// create typedef
#define WINPORT_DECL_DEF(NAME, RV, ARGS) typedef RV (*WINPORT_##NAME) ARGS;
#include "WinPortDecl.h"
// create structure
#undef WINPORT_DECL_DEF
#define WINPORT_DECL_DEF(NAME, RV, ARGS) WINPORT_##NAME NAME;
struct WINPORTDECL{
#include "WinPortDecl.h"
};
