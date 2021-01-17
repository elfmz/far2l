#ifdef _WIN64
#define PLATFORM " x64"
#elif defined _WIN32
#define PLATFORM " x86"
#else
#define PLATFORM ""
#endif

#define PLUGIN_VER_MAJOR 3
#define PLUGIN_VER_MINOR 25
#define PLUGIN_VER_PATCH 0
#define PLUGIN_DESC L"Calculator plugin for FAR manager 3.0" PLATFORM
#define PLUGIN_NAME L"Calculator"
#define PLUGIN_FILENAME L"calc.dll"
#define PLUGIN_COPYRIGHT L"© Igor Russkih, 1999-2001. © 2009-2012, uncle-vunkis. © FarPlugins Team, 2020 -"

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define PLUGIN_VERSION STRINGIZE(PLUGIN_VER_MAJOR) "." STRINGIZE(PLUGIN_VER_MINOR) "." STRINGIZE(PLUGIN_VER_PATCH)
