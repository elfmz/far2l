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

#include <Colorspace.h>

void ComputeAccents(
		const WinPortRGB& c_text, const WinPortRGB& c_back,
		WinPortRGB& c_a_text, WinPortRGB& c_a_back) 
{
	HoverResult r = ComputeControlAccent(FarToRGB(c_text), FarToRGB(c_back));
	c_a_text = RGBtoFar(r.fg_hover);
	c_a_back = RGBtoFar(r.bg_hover);
}

RGB FarToRGB(const WinPortRGB& c) 
{
	RGB rgb{c.r/255.0, c.g/255.0, c.b/255.0};
	return rgb;
}

WinPortRGB RGBtoFar(const RGB& rgb) 
{
	return WinPortRGB((int)(rgb.r*255), (int)(rgb.g*255), (int)(rgb.b*255));
}

WinPortRGB RGBtoFar(const iRGB& rgb) 
{
	return WinPortRGB(rgb.r, rgb.g, rgb.b);
}

WinPortRGB GetSoftenColorIf(const WinPortRGB& _clr_text) 
{
	WinPortRGB clr{_clr_text.r, _clr_text.g, _clr_text.b};
	if (SDLBackend::options && SDLBackend::options->UseSoftenBevels) {
		if (IsNearBlack(_clr_text.r, _clr_text.g, _clr_text.b) || IsNearWhite(_clr_text.r, _clr_text.g, _clr_text.b)) {
			RGB clrR = FarToRGB(clr);
			iRGB clrT = SoftenBlackish_LAB(clrR);
			clr = RGBtoFar(clrT);
		}
	}
	return clr;
}

WinPortRGB GetEmbossColor(const WinPortRGB& _clr_text, const WinPortRGB& _clr_back) {
	WinPortRGB clr_fade;
	/* near to black / near to white means LAB */

	int blackb = _clr_back.r + _clr_back.g + _clr_back.b;
	int blackf = _clr_text.r + _clr_text.g + _clr_text.b;

	if (blackb < 0x5f || blackf < 0x5f || blackb > 700 || blackf > 700) 
		clr_fade = ComputeEmbossColor_LAB(_clr_back, GetSoftenColorIf(_clr_text));
	else
		clr_fade = ComputeEmbossColor_HSL(_clr_back, _clr_text);
	return clr_fade;
}

WinPortRGB ComputeEmbossColor_HSL(const WinPortRGB& xbg, const WinPortRGB& xline) 
{
	RGB bg = FarToRGB(xbg);
	RGB fg = FarToRGB(xline);
	RGB r = ComputeRaiseColor_HSL(bg, fg);

	return RGBtoFar(r);;
}

WinPortRGB ComputeEmbossColor_LAB(const WinPortRGB& xbg, const WinPortRGB& xline) 
{
	RGB bg = FarToRGB(xbg);
	RGB fg = FarToRGB(xline);
	RGB r = ComputeRaiseColor_LAB(bg, fg);

	return RGBtoFar(r);
}

namespace SDLBackend {
	BackendOptions* options = 0;
}

static inline void SDL_swap(int& a, int& b) {
	int x = a;
	a = b;
	b = x;
}

void DrawFilledTriangle(SDL_Renderer* r,
	int x1, int y1,
	int x2, int y2,
	int x3, int y3)
{
    // Sort vertices by Y (ascending)
    if (y2 < y1) { SDL_swap(x1, x2); SDL_swap(y1, y2); }
    if (y3 < y1) { SDL_swap(x1, x3); SDL_swap(y1, y3); }
    if (y3 < y2) { SDL_swap(x2, x3); SDL_swap(y2, y3); }

    // Helper lambda: interpolate X between two points
    auto interp = [](int y, int x0, int y0, int x1, int y1) {
        if (y1 == y0) return x0;
        return (int)(x0 + (x1 - x0) * (float)(y - y0) / (float)(y1 - y0));
    };

    // Draw upper part (y1 → y2)
    for (int y = y1; y <= y2; ++y) {
        int xa = interp(y, x1, y1, x3, y3);
        int xb = interp(y, x1, y1, x2, y2);
        if (xa > xb) SDL_swap(xa, xb);
        SDL_RenderDrawLine(r, xa, y, xb, y);
    }

    // Draw lower part (y2 → y3)
    for (int y = y2; y <= y3; ++y) {
        int xa = interp(y, x1, y1, x3, y3);
        int xb = interp(y, x2, y2, x3, y3);
        if (xa > xb) SDL_swap(xa, xb);
        SDL_RenderDrawLine(r, xa, y, xb, y);
    }
}

void drawHorizontalGradientBox(SDL_Renderer * renderer,
        const int x, const int y, const int w, const int h, const float steps,
        const SDL_Color c1, const SDL_Color c2, const int fill)
{

    /* Acumulator initial position */
    float yt = y;
    float rt = c1.r;
    float gt = c1.g;
    float bt = c1.b;
    float at = c1.a;
    
    /* Changes in each attribute */
    float ys = h/steps;
    float rs = (c2.r - c1.r)/steps;
    float gs = (c2.g - c1.g)/steps;
    float bs = (c2.b - c1.b)/steps;
    float as = (c2.a - c1.a)/steps;

    for(int i = 0; i < steps ; i++)
    {
        /* Create an horizontal rectangle sliced by the number of steps */
        SDL_Rect rect = { x, yt, w, ys+1 };

        /* Sets the rectangle color based on iteration */
        SDL_SetRenderDrawColor(renderer, rt, gt, bt, at);

        /* Paint it or coverit*/
        if(fill)
            SDL_RenderFillRect(renderer, &rect);
        else
            SDL_RenderDrawRect(renderer, &rect);

        /* Update colors and positions */
        yt += ys;
        rt += rs;
        gt += gs;
        bt += bs;
        at += as;
    }
}
