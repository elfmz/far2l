#pragma once

#define APP_BASENAME "far2l"

#ifdef _WIN32
# define GOOD_SLASH	'\\'
# define LGOOD_SLASH	L'\\'
# define WGOOD_SLASH	L"\\"
# define NATIVE_EOL		"\r\n"
# define NATIVE_EOLW		L"\r\n"
# define NATIVE_EOL2		"\r\0\n\0"
#else
# define GOOD_SLASH	'/'
# define LGOOD_SLASH	L'/'
# define WGOOD_SLASH	L"/"
# define NATIVE_EOL		"\n"
# define NATIVE_EOLW		L"\n"
# define NATIVE_EOL2		"\n\0"
#endif

#define DEVNULL		"/dev/null"
#define DEVNULLW	L"/dev/null"

