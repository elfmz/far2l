#include "SDLShared.h"

#include <atomic>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <cerrno>
#include <mutex>
#include <string>

namespace
{

static FontHintingMode ParseFontHintingModeInternal(const char *env_val)
{
	if (!env_val || !*env_val) {
		return FontHintingMode::Default;
	}
	std::string lower(env_val);
	for (auto &ch : lower) {
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	if (lower == "auto") {
		return FontHintingMode::Auto;
	}
	if (lower == "none" || lower == "off" || lower == "disable" || lower == "disabled") {
		return FontHintingMode::None;
	}
	return FontHintingMode::Default;
}

} // namespace

FontHintingMode ParseFontHintingMode(const char *env_val)
{
	return ParseFontHintingModeInternal(env_val);
}

const char *HintingModeName(FontHintingMode mode)
{
	switch (mode) {
	case FontHintingMode::Auto: return "auto";
	case FontHintingMode::None: return "none";
	default: return "default";
	}
}

bool g_sdl_debug_redraw = []() {
	const char *env = std::getenv("FAR2L_SDL_DEBUG_REDRAW");
	return env && *env && *env != '0';
}();

bool g_sdl_bitmap_fallback = []() {
	const char *env = std::getenv("FAR2L_SDL_BITMAP_FALLBACK");
	return env && *env && *env != '0';
}();

bool g_sdl_subpixel_aa = []() {
	const char *env = std::getenv("FAR2L_SDL_SUBPIXEL_AA");
	return env && *env && *env != '0';
}();

FontHintingMode g_sdl_font_hinting = []() {
	const char *env = std::getenv("FAR2L_SDL_FONT_HINTING");
	return ParseFontHintingModeInternal(env);
}();

void SDLDebugLog(const char *fmt, ...)
{
	if (!g_sdl_debug_redraw) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[SDL backend] ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

SDLTraceWriter &SDLTraceWriter::Instance()
{
	static SDLTraceWriter instance;
	return instance;
}

void SDLTraceWriter::Emit(const char *category, const char *name)
{
	Emit(category, name, nullptr);
}

void SDLTraceWriter::Emit(const char *category, const char *name, const char *fmt, ...)
{
	if (!_enabled || !category || !name) {
		return;
	}

	char payload[512];
	payload[0] = '\0';
	if (fmt && *fmt) {
		va_list args;
		va_start(args, fmt);
		vsnprintf(payload, sizeof(payload), fmt, args);
		va_end(args);
	}

	const auto now = std::chrono::steady_clock::now();
	const auto ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

	std::lock_guard<std::mutex> lock(_mutex);
	if (!_file) {
		return;
	}
	fprintf(_file, "%lld\t%s\t%s", static_cast<long long>(ts), category, name);
	if (payload[0] != '\0') {
		fprintf(_file, "\t%s", payload);
	}
	fputc('\n', _file);
	fflush(_file);
}

bool SDLTraceWriter::Enabled() const
{
	return _enabled;
}

SDLTraceWriter::SDLTraceWriter()
{
	const char *env = std::getenv("FAR2L_SDL_TRACE");
	if (!env) {
		_enabled = false;
		return;
	}
	const char *path = *env ? env : "far2l-sdl-trace.log";
	_file = std::fopen(path, "w");
	if (!_file) {
		_enabled = false;
		fprintf(stderr, "SDLTraceWriter: failed to open %s (%s)\n", path, std::strerror(errno));
		return;
	}
	_enabled = true;
	fprintf(stderr, "[SDL backend] SDL tracing enabled -> %s\n", path);
}

SDLTraceWriter::~SDLTraceWriter()
{
	if (_file) {
		std::fclose(_file);
		_file = nullptr;
	}
}
