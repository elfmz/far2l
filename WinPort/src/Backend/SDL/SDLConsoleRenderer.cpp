#include "SDLConsoleRenderer.hpp"

#include "Backend.h"
#include "SDLBoxChars.h"
#include "SDLFontDialog.h"
#include "CharClasses.h"
#include "utils.h"
#include "WinPort.h"
#include "CharArray.hpp"
#include "WinPortRGB.h"
#include "WideMB.h"
#include "TestPath.h"
#include "SDLShared.h"

#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>
#include <future>
#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <list>
#include <array>
#include <iterator>

#if !defined(__APPLE__)
#include <fontconfig/fontconfig.h>
#endif

#include <chrono>
#include <limits>
#include <climits>
#include <cmath>
#include <fstream>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#if defined(__APPLE__)
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

#include "SDLConsoleRendererImpl.hpp"

struct SDLConsoleRenderer::Impl
{
	SDLConsoleRendererImpl inner;
};

SDLConsoleRenderer::SDLConsoleRenderer() : _impl(std::make_unique<Impl>()) {}

SDLConsoleRenderer::~SDLConsoleRenderer() = default;

bool SDLConsoleRenderer::Initialize(SDL_Window *window)
{
	return _impl->inner.Initialize(window);
}

void SDLConsoleRenderer::Shutdown()
{
	_impl->inner.Shutdown();
}

void SDLConsoleRenderer::SyncConsoleSize(bool resize_window)
{
	_impl->inner.SyncConsoleSize(resize_window);
}

void SDLConsoleRenderer::DrawFullConsole()
{
	_impl->inner.DrawFullConsole();
}

void SDLConsoleRenderer::DrawAreas(const std::vector<SMALL_RECT> &areas)
{
	_impl->inner.DrawAreas(areas);
}

void SDLConsoleRenderer::SetQuickEditRect(bool active, const SMALL_RECT &rect)
{
	_impl->inner.SetQuickEditRect(active, rect);
}

void SDLConsoleRenderer::ClearQuickEditRect()
{
	_impl->inner.ClearQuickEditRect();
}

void SDLConsoleRenderer::Present()
{
	_impl->inner.Present();
}

int SDLConsoleRenderer::CellWidth() const
{
	return _impl->inner.CellWidth();
}

int SDLConsoleRenderer::CellHeight() const
{
	return _impl->inner.CellHeight();
}

unsigned int SDLConsoleRenderer::Columns() const
{
	return _impl->inner.Columns();
}

unsigned int SDLConsoleRenderer::Rows() const
{
	return _impl->inner.Rows();
}

void SDLConsoleRenderer::OutputPixelSize(int &out_w, int &out_h) const
{
	_impl->inner.OutputPixelSize(out_w, out_h);
}

void SDLConsoleRenderer::GetLayoutInfo(int &origin_x, int &origin_y, int &console_w, int &console_h,
	int &out_w, int &out_h) const
{
	_impl->inner.GetLayoutInfo(origin_x, origin_y, console_w, console_h, out_w, out_h);
}

void SDLConsoleRenderer::NotifyOutputResize()
{
	_impl->inner.NotifyOutputResize();
}

bool SDLConsoleRenderer::SetImage(const std::string &id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height,
	const uint8_t *buffer, size_t buffer_size)
{
	return _impl->inner.SetImage(id, flags, area, width, height, buffer, buffer_size);
}

bool SDLConsoleRenderer::TransformImage(const std::string &id, const SMALL_RECT *area, uint16_t tf)
{
	return _impl->inner.TransformImage(id, area, tf);
}

bool SDLConsoleRenderer::DeleteImage(const std::string &id)
{
	return _impl->inner.DeleteImage(id);
}

void SDLConsoleRenderer::DescribeImageCaps(WinportGraphicsInfo &wgi) const
{
	_impl->inner.DescribeImageCaps(wgi);
}

bool SDLConsoleRenderer::ReloadFontsFromConfig()
{
	return _impl->inner.ReloadFontsFromConfig();
}

bool SDLConsoleRenderer::ReloadFontsIfScaleChanged()
{
	return _impl->inner.ReloadFontsIfScaleChanged();
}

bool SDLConsoleRenderer::CursorBlinkDue()
{
	return _impl->inner.CursorBlinkDue();
}

void SDLConsoleRenderer::SuppressCursorBlinkFor(std::chrono::milliseconds duration)
{
	_impl->inner.SuppressCursorBlinkFor(duration);
}

bool SDLConsoleRenderer::HasPendingFrame() const
{
	return _impl->inner.HasPendingFrame();
}

SDLConsoleRenderer::GlyphStats SDLConsoleRenderer::PopGlyphStats()
{
	const auto stats = _impl->inner.PopGlyphStats();
	return GlyphStats{stats.hits, stats.misses};
}

void SDLConsoleRenderer::SetCursorBlinkInterval(DWORD interval_ms)
{
	_impl->inner.SetCursorBlinkInterval(interval_ms);
}

void SDLConsoleRenderer::GetCursorState(COORD &pos, bool &visible) const
{
	_impl->inner.GetCursorState(pos, visible);
}
