#pragma once

#include <cstdio>
#include <mutex>

enum class FontHintingMode
{
	Default,
	Auto,
	None
};

FontHintingMode ParseFontHintingMode(const char *env_val);
const char *HintingModeName(FontHintingMode mode);

extern bool g_sdl_debug_redraw;
extern bool g_sdl_bitmap_fallback;
extern bool g_sdl_subpixel_aa;
extern bool g_sdl_norgb;
extern FontHintingMode g_sdl_font_hinting;

void SDLDebugLog(const char *fmt, ...);

class SDLTraceWriter
{
public:
	static SDLTraceWriter &Instance();
	void Emit(const char *category, const char *name);
	void Emit(const char *category, const char *name, const char *fmt, ...);
	bool Enabled() const;

private:
	SDLTraceWriter();
	~SDLTraceWriter();
	SDLTraceWriter(const SDLTraceWriter &) = delete;
	SDLTraceWriter &operator=(const SDLTraceWriter &) = delete;

	std::mutex _mutex;
	FILE *_file{nullptr};
	bool _enabled{false};
};

#define SDL_TRACE_EVENT(cat, name, ...) SDLTraceWriter::Instance().Emit((cat), (name), ##__VA_ARGS__)
#define SDL_TRACE_ENABLED() SDLTraceWriter::Instance().Enabled()
