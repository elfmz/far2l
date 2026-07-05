#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <SDL.h>

#include "WinPortRGB.h"

#include "WinPort.h"
#include <Colorspace.h>
#include <BackendOptions.h>

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

void ComputeAccents(
		const WinPortRGB& c_text, const WinPortRGB& c_back,
		WinPortRGB& c_a_text, WinPortRGB& c_a_back) ;
RGB FarToRGB(const WinPortRGB& c) ;
WinPortRGB RGBtoFar(const RGB& rgb);
WinPortRGB GetSoftenColorIf(const WinPortRGB& _clr_text) ;
WinPortRGB GetEmbossColor(const WinPortRGB& _clr_text, const WinPortRGB& _clr_back) ;
WinPortRGB ComputeEmbossColor_HSL(const WinPortRGB& xbg, const WinPortRGB& xline);
WinPortRGB ComputeEmbossColor_LAB(const WinPortRGB& xbg, const WinPortRGB& xline);

namespace SDLBackend {
	extern BackendOptions* options;
}

RGB FarToRGB(const WinPortRGB& c) ;
WinPortRGB RGBtoFar(const RGB& rgb) ;
WinPortRGB RGBtoFar(const iRGB& rgb) ;

void DrawFilledTriangle(SDL_Renderer* r, int x1, int y1, int x2, int y2, int x3, int y3);

void drawHorizontalGradientBox(SDL_Renderer * renderer,
    const int x, const int y, const int w, const int h, const float steps,
    const SDL_Color c1, const SDL_Color c2, const int fill);
