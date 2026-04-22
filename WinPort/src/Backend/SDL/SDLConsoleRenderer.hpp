#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "WinPort.h"

struct SDL_Window;

class SDLConsoleRenderer
{
public:
	struct GlyphStats
	{
		size_t hits{0};
		size_t misses{0};
	};

	SDLConsoleRenderer();
	~SDLConsoleRenderer();

	bool Initialize(SDL_Window *window);
	void Shutdown();

	void SyncConsoleSize(bool resize_window = true);
	void DrawFullConsole();
	void DrawAreas(const std::vector<SMALL_RECT> &areas);
	void SetQuickEditRect(bool active, const SMALL_RECT &rect);
	void ClearQuickEditRect();
	void Present();

	int CellWidth() const;
	int CellHeight() const;
	unsigned int Columns() const;
	unsigned int Rows() const;
	void OutputPixelSize(int &out_w, int &out_h) const;
	void GetLayoutInfo(int &origin_x, int &origin_y, int &console_w, int &console_h,
		int &out_w, int &out_h) const;
	void NotifyOutputResize();

	bool SetImage(const std::string &id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height,
		const uint8_t *buffer, size_t buffer_size);
	bool TransformImage(const std::string &id, const SMALL_RECT *area, uint16_t tf);
	bool DeleteImage(const std::string &id);
	void DescribeImageCaps(WinportGraphicsInfo &wgi) const;
	bool ReloadFontsFromConfig();
	bool ReloadFontsIfScaleChanged();
	bool CursorBlinkDue();
	void SuppressCursorBlinkFor(std::chrono::milliseconds duration);
	bool HasPendingFrame() const;
	GlyphStats PopGlyphStats();
	void SetCursorBlinkInterval(DWORD interval_ms);
	void GetCursorState(COORD &pos, bool &visible) const;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};
