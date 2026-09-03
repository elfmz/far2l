#include "SDLBackendUtils.h"

#include "WinPortRGB.h"
#include "SDLShared.h"

#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

SDL_BlendMode ComposePremultBlendMode()
{
#if SDL_VERSION_ATLEAST(2, 0, 6)
	return SDL_ComposeCustomBlendMode(
		SDL_BLENDFACTOR_ONE,
		SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
		SDL_BLENDOPERATION_ADD,
		SDL_BLENDFACTOR_ONE,
		SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
		SDL_BLENDOPERATION_ADD);
#else
	return SDL_BLENDMODE_BLEND;
#endif
}

WinPortRGB SDLConsoleForeground2RGB(DWORD64 attributes)
{
	if (g_sdl_norgb) {
		attributes&= ~(DWORD64)(BACKGROUND_TRUECOLOR | FOREGROUND_TRUECOLOR);
	}
	return (attributes & COMMON_LVB_REVERSE_VIDEO)
		? ConsoleBackground2RGB(g_winport_palette, attributes)
		: ConsoleForeground2RGB(g_winport_palette, attributes);
}

WinPortRGB SDLConsoleBackground2RGB(DWORD64 attributes)
{
	if (g_sdl_norgb) {
		attributes&= ~(DWORD64)(BACKGROUND_TRUECOLOR | FOREGROUND_TRUECOLOR);
	}
	return (attributes & COMMON_LVB_REVERSE_VIDEO)
		? ConsoleForeground2RGB(g_winport_palette, attributes)
		: ConsoleBackground2RGB(g_winport_palette, attributes);
}

SDL_Color ToSDLColor(const WinPortRGB &rgb)
{
	return SDL_Color{(Uint8)rgb.r, (Uint8)rgb.g, (Uint8)rgb.b, 255};
}

std::string TrimString(const std::string &value)
{
	size_t start = 0;
	while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
		++start;
	}
	size_t end = value.size();
	while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
		--end;
	}
	return value.substr(start, end - start);
}

std::string ExpandUserPath(const std::string &path)
{
	if (path.empty() || path[0] != '~') {
		return path;
	}

	const char *home = getenv("HOME");
	if (!home || !*home) {
		return path;
	}

	if (path.size() == 1) {
		return std::string(home);
	}

	if (path[1] == '/' || path[1] == '\\') {
		return std::string(home) + path.substr(1);
	}

	return path;
}

size_t ImagePayloadSize(DWORD64 flags, DWORD width, DWORD height)
{
	switch (flags & WP_IMG_MASK_FMT) {
	case WP_IMG_PNG:
	case WP_IMG_JPG:
		return static_cast<size_t>(width);
	case WP_IMG_RGB:
		return static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
	case WP_IMG_RGBA:
		return static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
	default:
		return 0;
	}
}
