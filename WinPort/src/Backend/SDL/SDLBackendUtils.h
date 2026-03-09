#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <SDL.h>

#include "WinPortRGB.h"

#include "WinPort.h"

inline constexpr int kDefaultFontPointSize = 18;
inline constexpr int kMinFontPointSize = 6;
inline constexpr int kMaxFontPointSize = 96;
inline constexpr size_t kMaxGlyphCacheEntries = 512;
inline constexpr int kGlyphAtlasWidthPx = 2048;
inline constexpr int kGlyphAtlasHeightPx = 2048;
inline constexpr int kGlyphAtlasPaddingPx = 2;
inline constexpr size_t kMaxPendingTextureDestroysBeforeFlush = 128;

std::string TrimString(const std::string &value);
std::string ExpandUserPath(const std::string &path);
size_t ImagePayloadSize(DWORD64 flags, DWORD width, DWORD height);

SDL_BlendMode ComposePremultBlendMode();
WinPortRGB SDLConsoleForeground2RGB(DWORD64 attributes);
WinPortRGB SDLConsoleBackground2RGB(DWORD64 attributes);
SDL_Color ToSDLColor(const WinPortRGB &rgb);
