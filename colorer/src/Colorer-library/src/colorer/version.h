#ifndef _COLORER_VERSION_H_
#define _COLORER_VERSION_H_

#define COLORER_VER_MAJOR 1
#define COLORER_VER_MINOR 0
#define COLORER_VER_PATCH 5

#define COLORER_COPYRIGHT "(c) 1999-2009 Igor Russkih, (c) 2009-2020 Aleksey Dobrunov"

#ifdef _WIN64
#define CONF " x64"
#elif defined _WIN32
#define CONF " x86"
#else
#define CONF ""
#endif

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define COLORER_VERSION STRINGIZE(COLORER_VER_MAJOR) "." STRINGIZE(COLORER_VER_MINOR) "." STRINGIZE(COLORER_VER_PATCH) CONF

#endif  // _COLORER_VERSION_H_
