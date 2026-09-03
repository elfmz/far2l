#pragma once

#include <cmath>

#include "SDLShared.h"
#include "SDLBackendUtils.h"
#include "SDLImageManager.h"
#include "SDLFontManager.h"
#include "utils.h"

static constexpr DWORD64 kColorAttributesMask =
	(FOREGROUND_INTENSITY | BACKGROUND_INTENSITY |
	 FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
	 BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE |
	 COMMON_LVB_REVERSE_VIDEO | COMMON_LVB_UNDERSCORE);

class GlyphAtlas
{
public:
	GlyphAtlas(SDL_Renderer *renderer, int width, int height)
		: _renderer(renderer), _width(width), _height(height)
	{
		if (_renderer) {
			_texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, _width, _height);
			if (_texture) {
				SDL_SetTextureBlendMode(_texture, SDL_BLENDMODE_BLEND);
			}
		}
	}

	~GlyphAtlas()
	{
		if (_texture) {
			SDL_DestroyTexture(_texture);
			_texture = nullptr;
		}
	}

	bool Allocate(int width, int height, SDL_Rect &rect)
	{
		if (!_texture || width <= 0 || height <= 0) {
			return false;
		}

		const int padded_w = width + kGlyphAtlasPaddingPx;
		const int padded_h = height + kGlyphAtlasPaddingPx;

		if (padded_w > _width || padded_h > _height) {
			return false;
		}

		if (_pen_x + padded_w > _width) {
			_pen_x = 0;
			_pen_y += _row_height;
			_row_height = kGlyphAtlasPaddingPx;
		}

		if (_pen_y + padded_h > _height) {
			return false;
		}

		rect = SDL_Rect{_pen_x, _pen_y, width, height};
		_pen_x += padded_w;
		_row_height = std::max(_row_height, padded_h);
		return true;
	}

	bool Upload(const SDL_Rect &dst, SDL_Surface *surface)
	{
		if (!_texture || !surface) {
			return false;
		}
		return SDL_UpdateTexture(_texture, &dst, surface->pixels, surface->pitch) == 0;
	}

	SDL_Texture *Texture() const { return _texture; }

private:
	SDL_Renderer *_renderer{nullptr};
	SDL_Texture *_texture{nullptr};
	int _width{0};
	int _height{0};
	int _pen_x{0};
	int _pen_y{0};
	int _row_height{kGlyphAtlasPaddingPx};
};


class SDLConsoleRendererImpl
{
	struct Glyph
	{
		GlyphAtlas *atlas{nullptr};
		SDL_Rect atlas_rect{0, 0, 0, 0};
		SDL_Texture *texture{nullptr}; // legacy bitmap fallback
		int bearing_x{0};
		int bearing_y{0};
		int advance{0};
		int width{0};
		int height{0};
		bool is_color{false};
	};
	struct GlyphCacheEntry
	{
		Glyph glyph;
		std::list<std::u32string>::iterator lru_it;
	};
	struct RenderLayout
	{
		int cell_px_w{0};
		int cell_px_h{0};
		int ascent_px{0};
		unsigned int cols{0};
		unsigned int rows{0};
		int origin_x{0};
		int origin_y{0};
		int console_px_w{0};
		int console_px_h{0};
		int output_px_w{0};
		int output_px_h{0};
		float scale_x{1.f};
		float scale_y{1.f};
	};

	struct U32StringHash
	{
		size_t operator()(const std::u32string &s) const noexcept
		{
			const size_t prime = sizeof(size_t) == 8 ? 1099511628211ull : 16777619u;
			size_t hash = sizeof(size_t) == 8 ? 1469598103934665603ull : 2166136261u;
			for (auto ch : s) {
				hash ^= static_cast<size_t>(ch);
				hash *= prime;
			}
			return hash;
		}
	};

	struct CursorState
	{
		COORD pos{0, 0};
		bool visible{false};
		UCHAR height{100};
		DWORD blink_interval{500};
		bool blink_state{true};
		bool blink_dirty{false};
		std::chrono::steady_clock::time_point next_toggle{std::chrono::steady_clock::now()};
		std::chrono::steady_clock::time_point suppress_until{std::chrono::steady_clock::time_point::min()};
	};

public:
	SDLConsoleRendererImpl()
	{
		_bitmap_fallback = g_sdl_bitmap_fallback;
		_layout_dirty = true;
	}
	~SDLConsoleRendererImpl() { Shutdown(); }

	bool Initialize(SDL_Window *window)
	{
		_window = window;
		// VK: moved to SDLMain as we should setup hints prior to create first window
		// SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // keep glyphs pixel-crisp
		{
			SDL_version ver{};
			SDL_GetVersion(&ver);
			const char *video_driver = SDL_GetCurrentVideoDriver();
			fprintf(stderr,
				"SDLConsoleRenderer: SDL %u.%u.%u video_driver=%s env:SDL_VIDEODRIVER=%s SDL_RENDER_DRIVER=%s\n",
				static_cast<unsigned>(ver.major),
				static_cast<unsigned>(ver.minor),
				static_cast<unsigned>(ver.patch),
				video_driver ? video_driver : "unknown",
				getenv("SDL_VIDEODRIVER") ? getenv("SDL_VIDEODRIVER") : "",
				getenv("SDL_RENDER_DRIVER") ? getenv("SDL_RENDER_DRIVER") : "");
		}
		const bool force_software = false;
		const char *wsl_distro = getenv("WSL_DISTRO_NAME");
		const bool prefer_software = wsl_distro && *wsl_distro;
		bool is_software = false;
		if (force_software || prefer_software) {
			_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
			is_software = (_renderer != nullptr);
			if (!_renderer && !force_software) {
				_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
				is_software = false;
			}
		} else {
			_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (!_renderer) {
				_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
				is_software = (_renderer != nullptr);
			}
		}
		if (!_renderer) {
			fprintf(stderr, "SDLConsoleRenderer: failed to create renderer: %s\n", SDL_GetError());
			return false;
		}

		SDL_RendererInfo rinfo{};
		bool supports_target = false;
		const int info_rc = SDL_GetRendererInfo(_renderer, &rinfo);
		if (info_rc == 0) {
			supports_target = (rinfo.flags & SDL_RENDERER_TARGETTEXTURE) != 0;
			fprintf(stderr, "SDLConsoleRenderer: renderer_info name=%s flags=0x%x\n",
				rinfo.name ? rinfo.name : "unknown",
				static_cast<unsigned>(rinfo.flags));
		} else {
			fprintf(stderr, "SDLConsoleRenderer: SDL_GetRendererInfo failed: %s\n", SDL_GetError());
		}

		if (is_software && !force_software) {
			const bool looks_dummy = (info_rc != 0) || (!supports_target && (rinfo.flags & SDL_RENDERER_ACCELERATED) == 0);
			const bool selftest_ok = SelfTestRenderer();
			if (looks_dummy || !selftest_ok) {
				fprintf(stderr, "SDLConsoleRenderer: software renderer looks unusable, retrying accelerated\n");
				SDL_DestroyRenderer(_renderer);
				_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
				is_software = false;
				if (!_renderer) {
					fprintf(stderr, "SDLConsoleRenderer: accelerated retry failed: %s\n", SDL_GetError());
					return false;
				}
				SDL_RendererInfo retry_info{};
				if (SDL_GetRendererInfo(_renderer, &retry_info) == 0) {
					fprintf(stderr, "SDLConsoleRenderer: renderer_info name=%s flags=0x%x\n",
						retry_info.name ? retry_info.name : "unknown",
						static_cast<unsigned>(retry_info.flags));
					supports_target = (retry_info.flags & SDL_RENDERER_TARGETTEXTURE) != 0;
				}
			}
		}

		_use_backing_texture = supports_target;
		fprintf(stderr, "SDLConsoleRenderer: renderer=%s (render_target=%s)\n",
			is_software ? "software" : "accelerated+vsync",
			_use_backing_texture ? "yes" : "no");
		SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderSetIntegerScale(_renderer, SDL_FALSE);
		SDL_RenderSetViewport(_renderer, nullptr);
		SDL_RenderSetClipRect(_renderer, nullptr);
//SDL_RenderSetLogicalSize(renderer, pixel_w, pixel_h);
		const float font_scale = GetFontScale();
		if (!_font_manager.Initialize(font_scale, g_sdl_subpixel_aa, g_sdl_font_hinting)) {
			return false;
		}
		_font_scale = font_scale;
		RequestFullRedraw("Initialize");
		return true;
	}

	void Shutdown()
	{
		ClearFontFaces();
		_font_scale = 1.f;
		_image_manager.ClearImages();
		FlushPendingTextureDestroys();
		DestroyBackingTexture();
		if (_renderer) {
			SDL_DestroyRenderer(_renderer);
			_renderer = nullptr;
		}
		_window = nullptr;
	}

	bool SelfTestRenderer()
	{
		if (!_renderer || !_window) {
			return false;
		}
		// Render-target test to detect dummy software renderers.
		SDL_Texture *test_tex = SDL_CreateTexture(
			_renderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_TARGET,
			1,
			1);
		if (!test_tex) {
			fprintf(stderr, "SDLConsoleRenderer: selftest create texture failed: %s\n", SDL_GetError());
			return false;
		}
		SDL_SetTextureBlendMode(test_tex, SDL_BLENDMODE_NONE);
		if (SDL_SetRenderTarget(_renderer, test_tex) != 0) {
			fprintf(stderr, "SDLConsoleRenderer: selftest set target failed: %s\n", SDL_GetError());
			SDL_DestroyTexture(test_tex);
			return false;
		}
		SDL_RenderSetScale(_renderer, 1.f, 1.f);
		SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(_renderer, 1, 2, 3, 255);
		SDL_RenderClear(_renderer);

		Uint32 pixel = 0;
		SDL_Rect r{0, 0, 1, 1};
		if (SDL_RenderReadPixels(_renderer, &r, SDL_PIXELFORMAT_ARGB8888, &pixel, sizeof(pixel)) != 0) {
			fprintf(stderr, "SDLConsoleRenderer: selftest readpixels failed: %s\n", SDL_GetError());
			SDL_SetRenderTarget(_renderer, nullptr);
			SDL_DestroyTexture(test_tex);
			return false;
		}
		SDL_SetRenderTarget(_renderer, nullptr);
		SDL_DestroyTexture(test_tex);
		const Uint8 a = (pixel >> 24) & 0xFF;
		const Uint8 red = (pixel >> 16) & 0xFF;
		const Uint8 green = (pixel >> 8) & 0xFF;
		const Uint8 blue = pixel & 0xFF;
		const bool ok = (a == 0xFF) && (red == 1) && (green == 2) && (blue == 3);
		if (!ok) {
			fprintf(stderr, "SDLConsoleRenderer: selftest mismatch argb=%02x%02x%02x%02x\n",
				a, red, green, blue);
		} else {
			fprintf(stderr, "SDLConsoleRenderer: selftest ok\n");
		}
		return ok;
	}

    void SyncConsoleSize(bool resize_window = true)
    {
        if (!g_winport_con_out || !_renderer) {
            return;
        }
        if (SDL_TRACE_ENABLED()) {
            SDL_TRACE_EVENT("renderer", "sync_console_size");
        }

        unsigned int cols = 0, rows = 0;
        g_winport_con_out->GetSize(cols, rows);

        _cols = cols;
        _rows = rows;

		const int cell_w = std::max(1, _font_manager.CellWidth());
		const int cell_h = std::max(1, _font_manager.CellHeight());
		const int pixel_w = std::max(1u, cols) * cell_w;
		const int pixel_h = std::max(1u, rows) * cell_h;

        float sx = 1.f;
        float sy = 1.f;
        GetWindowScale(sx, sy);
        const int win_w = std::max(1, static_cast<int>(std::floor(pixel_w / std::max(1.f, sx))));
        const int win_h = std::max(1, static_cast<int>(std::floor(pixel_h / std::max(1.f, sy))));

        //if (resize_window && _window) {
            SDL_SetWindowMinimumSize(_window,
				std::max(1, static_cast<int>(std::floor(cell_w / std::max(1.f, sx)))),
				std::max(1, static_cast<int>(std::floor(cell_h / std::max(1.f, sy)))));
            int cur_w = 0;
            int cur_h = 0;
            SDL_GetWindowSize(_window, &cur_w, &cur_h);
            if (std::abs(cur_w - win_w) > 1 || std::abs(cur_h - win_h) > 1) {
                SDL_SetWindowSize(_window, win_w, win_h);
            }
        //}

        RequestFullRedraw("SyncConsoleSize");
        MarkLayoutDirty("SyncConsoleSize");
    }


	void DrawAreas(const std::vector<SMALL_RECT> &areas)
	{
		if (!g_winport_con_out || !_renderer) {
			return;
		}
        if (SDL_TRACE_ENABLED()) {
            SDL_TRACE_EVENT("renderer", "draw_areas", "count=%zu", areas.size());
        }

		BeginFrameIfNeeded();
		if (!_frame_active) {
			return;
		}

		const RenderLayout &layout = CurrentLayout();
		for (const auto &area_in : areas) {
			DrawArea(area_in);
			if (_present_full_copy) {
				continue;
			}
			const SHORT top = std::max<SHORT>(0, area_in.Top);
			const SHORT bottom = std::min<SHORT>((SHORT)_rows - 1, area_in.Bottom);
			const SHORT left = std::max<SHORT>(0, area_in.Left);
			const SHORT right = std::min<SHORT>((SHORT)_cols - 1, area_in.Right);
			if (top > bottom || left > right) {
				continue;
			}
			const int span_cols = right - left + 1;
			const int span_rows = bottom - top + 1;
			SDL_Rect rect = CellRectToPixelRect(layout, left, top, span_cols, span_rows);
			if (rect.w > 0 && rect.h > 0) {
				_present_dirty_rects.push_back(rect);
			}
		}
	}

	void DrawFullConsole()
	{
		if (!g_winport_con_out || !_renderer) {
			return;
		}
		if (SDL_TRACE_ENABLED()) {
			SDL_TRACE_EVENT("renderer", "draw_full_console", "cols=%u rows=%u", _cols, _rows);
		}
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleRenderer: DrawFullConsole");
		}

        unsigned int cols = 0, rows = 0;
        g_winport_con_out->GetSize(cols, rows);
        if (cols != _cols || rows != _rows) {
            _cols = cols;
            _rows = rows;
            MarkLayoutDirty("DrawFullConsole size sync");
        }

		BeginFrame(true);
		if (!_frame_active) {
			return;
		}
		_present_full_copy = true;
		_present_dirty_rects.clear();

		for (unsigned int row = 0; row < _rows; ++row) {
			IConsoleOutput::DirectLineAccess dla(g_winport_con_out, row);
			const CHAR_INFO *line = dla.Line();
			const unsigned int width = dla.Width();
			if (!line || !width) {
				continue;
			}
			unsigned int col = 0;
			while (col < width) {
				if (col > 0 && line[col].Char.UnicodeChar == 0 && CI_FULL_WIDTH_CHAR(line[col - 1])) {
					++col;
					continue;
				}
				DrawCell(col, row, line[col]);
				col += CI_FULL_WIDTH_CHAR(line[col]) ? 2 : 1;
			}
		}
	}

	void SetQuickEditRect(bool active, const SMALL_RECT &rect)
	{
		_qedit_active = active;
		_qedit_rect = rect;
	}

	void ClearQuickEditRect()
	{
		_qedit_active = false;
	}

	void Present()
	{
		static bool s_present_logged = false;
		if (!_renderer) {
			return;
		}
		const RenderLayout &layout = ResolveLayout();
		if (!_use_backing_texture) {
			if (!s_present_logged) {
				fprintf(stderr, "SDLConsoleRenderer: Present() direct path\n");
				s_present_logged = true;
			}
			SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
			SDL_RenderClear(_renderer);
			DrawFullConsole();
			if (_qedit_active) {
				SDL_SetRenderTarget(_renderer, nullptr);
				SDL_RenderSetScale(_renderer, 1.f, 1.f);
				SDL_RenderSetViewport(_renderer, nullptr);
				SDL_RenderSetClipRect(_renderer, nullptr);
				DrawQuickEditOverlay();
			}
			SDLImageManager::Layout img_layout{layout.origin_x, layout.origin_y, layout.cell_px_w, layout.cell_px_h};
			_image_manager.DrawImages(_renderer, img_layout);
			DrawCursorOverlay();
			SDL_RenderPresent(_renderer);
			SDL_RenderSetScale(_renderer, 1.f, 1.f);
			FlushPendingTextureDestroys();
			return;
		}
		if (!EnsureBackingTexture(layout)) {
			return;
		}
		if (!s_present_logged) {
			fprintf(stderr, "SDLConsoleRenderer: Present() backing texture path\n");
			s_present_logged = true;
		}

		if (SDL_TRACE_ENABLED()) {
			SDL_TRACE_EVENT("renderer", "present", "frame_active=%d redraw=%d",
				_frame_active ? 1 : 0, _needs_full_redraw ? 1 : 0);
		}

		if (_needs_full_redraw) {
			DrawFullConsole();
		}

		if (_qedit_active && _backing_texture) {
			SDL_SetRenderTarget(_renderer, _backing_texture);
			SDL_RenderSetScale(_renderer, 1.f, 1.f);
			SDL_RenderSetViewport(_renderer, nullptr);
			SDL_RenderSetClipRect(_renderer, nullptr);
			DrawQuickEditOverlay();
		}

		SDL_SetRenderTarget(_renderer, nullptr);
		SDL_RenderSetScale(_renderer, 1.f, 1.f);
		SDL_RenderSetViewport(_renderer, nullptr);
		SDL_RenderSetClipRect(_renderer, nullptr);
		if (!_present_full_copy) {
			const bool prev_drawn = _last_cursor_drawn_visible && _last_cursor_drawn_blink_state;
			bool next_blink_state = _cursor.blink_state;
			if (_cursor.blink_interval && _cursor.blink_dirty) {
				next_blink_state = !_cursor.blink_state;
			}
			const bool next_drawn = _cursor.visible && next_blink_state;
			auto push_cursor_rect = [&](const COORD &pos) {
				if (pos.X < 0 || pos.Y < 0 || pos.X >= (SHORT)_cols || pos.Y >= (SHORT)_rows) {
					return;
				}
				SDL_Rect rect = CellRectToPixelRect(layout, pos.X, pos.Y, 1, 1);
				if (rect.w > 0 && rect.h > 0) {
					_present_dirty_rects.push_back(rect);
				}
			};
			if (prev_drawn) {
				push_cursor_rect(_last_cursor_drawn_pos);
			}
			if (next_drawn) {
				push_cursor_rect(_cursor.pos);
			}
		}
		_present_full_copy = true;
		SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
		SDL_RenderClear(_renderer);
		if (_backing_texture) {
			SDL_Rect dst{layout.origin_x, layout.origin_y, layout.console_px_w, layout.console_px_h};
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleRenderer: present full copy (%dx%d) always", dst.w, dst.h);
			}
			SDL_RenderCopy(_renderer, _backing_texture, nullptr, &dst);
		}

		{
			SDLImageManager::Layout img_layout{layout.origin_x, layout.origin_y, layout.cell_px_w, layout.cell_px_h};
			_image_manager.DrawImages(_renderer, img_layout);
		}
		DrawCursorOverlay();

		SDL_RenderPresent(_renderer);
		SDL_RenderSetScale(_renderer, 1.f, 1.f);
		FlushPendingTextureDestroys();
		_present_dirty_rects.clear();
		_present_full_copy = false;
		_last_cursor_drawn_pos = _cursor.pos;
		_last_cursor_drawn_visible = _cursor.visible;
		_last_cursor_drawn_blink_state = _cursor.blink_state;
		_frame_active = false;
		_frame_layout_active = false;
	}

	int CellWidth() const { return _font_manager.CellWidth(); }
	int CellHeight() const { return _font_manager.CellHeight(); }
	unsigned int Columns() const { return _cols; }
	unsigned int Rows() const { return _rows; }
	void OutputPixelSize(int &out_w, int &out_h) const { GetOutputPixelSize(out_w, out_h); }
	void GetLayoutInfo(int &origin_x, int &origin_y, int &console_w, int &console_h,
		int &output_w, int &output_h) const
	{
		const RenderLayout &layout = ResolveLayout();
		origin_x = layout.origin_x;
		origin_y = layout.origin_y;
		console_w = layout.console_px_w;
		console_h = layout.console_px_h;
		output_w = layout.output_px_w;
		output_h = layout.output_px_h;
	}
	void NotifyOutputResize()
	{
		RequestFullRedraw("NotifyOutputResize");
		MarkLayoutDirty("NotifyOutputResize");
	}
	bool SetImage(const std::string &id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height,
		const uint8_t *buffer, size_t buffer_size);
	bool TransformImage(const std::string &id, const SMALL_RECT *area, uint16_t tf);
	bool DeleteImage(const std::string &id);
	void DescribeImageCaps(WinportGraphicsInfo &wgi) const;
	bool ReloadFontsFromConfig();
	bool ReloadFontsIfScaleChanged();
	bool CursorBlinkDue();
	void SuppressCursorBlinkFor(std::chrono::milliseconds duration);
	bool HasPendingFrame() const { return _frame_active || _needs_full_redraw || _image_manager.HasDirty(); }
	struct GlyphStats
	{
		size_t hits{0};
		size_t misses{0};
	};
	GlyphStats PopGlyphStats()
	{
		GlyphStats stats{_glyph_cache_hits, _glyph_cache_misses};
		_glyph_cache_hits = 0;
		_glyph_cache_misses = 0;
		return stats;
	}

	void SetCursorBlinkInterval(DWORD interval_ms)
	{
		if (interval_ms == 0) {
			interval_ms = 500;
		}
		_cursor.blink_interval = interval_ms;
		_cursor.next_toggle = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms);
		_cursor.blink_state = true;
		_cursor.blink_dirty = false;
	}
	void GetCursorState(COORD &pos, bool &visible) const
	{
		pos = _cursor.pos;
		visible = _cursor.visible;
	}

private:
	void RequestFullRedraw(const char *reason)
	{
		if (!_needs_full_redraw) {
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleRenderer: full redraw requested (%s)", reason ? reason : "unspecified");
			}
			_needs_full_redraw = true;
		} else if (g_sdl_debug_redraw && reason) {
			SDLDebugLog("SDLConsoleRenderer: redundant full redraw request (%s)", reason);
		}
	}

	void ClearFullRedrawFlag(const char *reason)
	{
		if (_needs_full_redraw && g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleRenderer: full redraw satisfied (%s)", reason ? reason : "unspecified");
		}
		_needs_full_redraw = false;
	}

	void MarkLayoutDirty(const char *reason = nullptr)
	{
		if (!_layout_dirty && g_sdl_debug_redraw && reason) {
			SDLDebugLog("SDLConsoleRenderer: layout dirty (%s)", reason);
		}
		_layout_dirty = true;
	}

	std::u32string BuildCluster(const CHAR_INFO &ci) const
	{
		std::u32string out;
		if (CI_USING_COMPOSITE_CHAR(ci)) {
			const WCHAR *seq = WINPORT(CompositeCharLookup)(ci.Char.UnicodeChar);
			if (seq) {
				for (const WCHAR *p = seq; *p; ++p) {
					out.push_back(static_cast<uint32_t>(*p));
				}
			}
		} else if (ci.Char.UnicodeChar) {
			out.push_back(static_cast<uint32_t>(ci.Char.UnicodeChar));
		}

		if (out.empty()) {
			out.push_back(U' ');
		}
		return out;
	}

	void RecomputeLayout(RenderLayout &layout)
	{
		layout = {};
		const int cell_w = std::max(1, _font_manager.CellWidth());
		const int cell_h = std::max(1, _font_manager.CellHeight());
		layout.cell_px_w = cell_w;
		layout.cell_px_h = cell_h;
		layout.ascent_px = _font_manager.Ascent();
		layout.cols = _cols;
		layout.rows = _rows;
		layout.console_px_w = static_cast<int>(std::max(1u, _cols)) * layout.cell_px_w;
		layout.console_px_h = static_cast<int>(std::max(1u, _rows)) * layout.cell_px_h;

		int output_w = layout.console_px_w;
		int output_h = layout.console_px_h;
		GetOutputPixelSize(output_w, output_h);
		if (output_w <= 0) output_w = layout.console_px_w;
		if (output_h <= 0) output_h = layout.console_px_h;

		layout.output_px_w = output_w;
		layout.output_px_h = output_h;
		layout.scale_x = 1.f;
		layout.scale_y = 1.f;

		layout.origin_x = 0;
		layout.origin_y = 0;
	}

	void GetOutputPixelSize(int &out_w, int &out_h) const
	{
		out_w = 0;
		out_h = 0;
		if ((out_w <= 0 || out_h <= 0) && _renderer) {
			SDL_GetRendererOutputSize(_renderer, &out_w, &out_h);
		}
		if ((out_w <= 0 || out_h <= 0) && _window) {
			SDL_GetWindowSize(_window, &out_w, &out_h);
		}
	}

	void GetWindowScale(float &out_x, float &out_y) const
	{
		out_x = 1.f;
		out_y = 1.f;
		if (!_window) {
			return;
		}
		int win_w = 0;
		int win_h = 0;
		SDL_GetWindowSize(_window, &win_w, &win_h);
		if (win_w <= 0 || win_h <= 0) {
			return;
		}
		int pix_w = 0;
		int pix_h = 0;
		GetOutputPixelSize(pix_w, pix_h);
		if (pix_w > 0) {
			out_x = static_cast<float>(pix_w) / static_cast<float>(win_w);
		}
		if (pix_h > 0) {
			out_y = static_cast<float>(pix_h) / static_cast<float>(win_h);
		}
	}

	float GetFontScale() const
	{
		float sx = 1.f, sy = 1.f;
		GetWindowScale(sx, sy);
		return (sy > 0.f) ? sy : 1.f;
	}

const RenderLayout &ResolveLayout() const
	{
		if (_layout_dirty) {
			auto *self = const_cast<SDLConsoleRendererImpl *>(this);
			self->RecomputeLayout(self->_layout);
			self->_layout_dirty = false;
		}
		return _layout;
	}

	const RenderLayout &CurrentLayout() const
	{
		if (_frame_layout_active) {
			return _frame_layout;
		}
		if (_layout_dirty) {
			return const_cast<SDLConsoleRendererImpl *>(this)->ResolveLayout();
		}
		return _layout;
	}

	int CellToPixelX(const RenderLayout &layout, int col) const
	{
		return layout.origin_x + col * layout.cell_px_w;
	}

	int CellToPixelY(const RenderLayout &layout, int row) const
	{
		return layout.origin_y + row * layout.cell_px_h;
	}

	SDL_Rect CellRectToPixelRect(const RenderLayout &layout, int col, int row, int span_cols, int span_rows) const
	{
		SDL_Rect rect;
		rect.x = CellToPixelX(layout, col);
		rect.y = CellToPixelY(layout, row);
		rect.w = span_cols * layout.cell_px_w;
		rect.h = span_rows * layout.cell_px_h;
		return rect;
	}

	void BeginFrame(bool force_clear = false)
	{
		const RenderLayout &layout = ResolveLayout();
		if (_use_backing_texture) {
			if (!EnsureBackingTexture(layout)) {
				return;
			}
			SDL_SetRenderTarget(_renderer, _backing_texture);
		} else {
			SDL_SetRenderTarget(_renderer, nullptr);
		}
		SDL_RenderSetScale(_renderer, 1.f, 1.f);
		SDL_RenderSetViewport(_renderer, nullptr);
		SDL_RenderSetClipRect(_renderer, nullptr);
		_frame_layout = layout;
		_frame_layout_active = true;

		if (force_clear) {
			if (SDL_TRACE_ENABLED()) {
				SDL_TRACE_EVENT("renderer", "begin_frame_clear", "state=start");
			}
			SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
			SDL_RenderClear(_renderer);
			ClearFullRedrawFlag("BeginFrame(start)");
		} else if (_needs_full_redraw && !_frame_active) {
			// If we deferred a full redraw, make sure the target is cleared once.
			SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
			SDL_RenderClear(_renderer);
			ClearFullRedrawFlag("BeginFrame(deferred)");
		}

		if (_frame_active) {
			return;
		}

		if (SDL_TRACE_ENABLED()) {
			SDL_TRACE_EVENT("renderer", "begin_frame", "force=%d", force_clear ? 1 : 0);
		}
		_frame_active = true;
	}

	void BeginFrameIfNeeded()
	{
		if (!_frame_active) {
			BeginFrame(_needs_full_redraw);
		} else if (_needs_full_redraw) {
			BeginFrame(true);
		}
	}

	void DrawArea(const SMALL_RECT &area_in)
    {
        const SHORT top = std::max<SHORT>(0, area_in.Top);
        const SHORT bottom = std::min<SHORT>((SHORT)_rows - 1, area_in.Bottom);
        const SHORT left = std::max<SHORT>(0, area_in.Left);
        const SHORT right = std::min<SHORT>((SHORT)_cols - 1, area_in.Right);

        if (top > bottom || left > right) {
            return;
        }

        for (SHORT row = top; row <= bottom; ++row) {
            IConsoleOutput::DirectLineAccess dla(g_winport_con_out, row);
            const CHAR_INFO *line = dla.Line();
            if (!line) {
                continue;
            }

            const unsigned int width = dla.Width();
            if (!width) {
                continue;
            }

            const SHORT max_col = static_cast<SHORT>(width - 1);
            const SHORT col_end = std::min(right, max_col);
            SHORT col = left;
            while (col <= col_end) {
                if (col > 0 && line[col].Char.UnicodeChar == 0 && CI_FULL_WIDTH_CHAR(line[col - 1])) {
                    ++col;
                    continue;
                }
                DrawCell(col, row, line[col]);
                col += CI_FULL_WIDTH_CHAR(line[col]) ? 2 : 1;
            }
        }
    }

	void DrawCell(int col, int row, const CHAR_INFO &ci)
    {
        const RenderLayout &layout = CurrentLayout();
		_last_draw_color_valid = false;
		_last_texture = nullptr;
		_last_tex_mod_valid = false;
		_last_tex_blend_valid = false;
        const DWORD64 attrs = ci.Attributes;
        const WinPortRGB fg = SDLConsoleForeground2RGB(attrs);
        const WinPortRGB bg = SDLConsoleBackground2RGB(attrs);
        const SDL_Color fg_color = ToSDLColor(fg);

        const bool full_width = CI_FULL_WIDTH_CHAR(ci);
        const int span = full_width ? 2 : 1;
        SDL_Rect rect = CellRectToPixelRect(layout, col, row, span, 1);
        const SDL_Color bgc = ToSDLColor(bg);

        SetDrawColorCached(bgc);
        SDL_RenderFillRect(_renderer, &rect);

		struct ScopedClip
		{
			SDL_Renderer *renderer;
			SDL_Rect prev{};
			bool had_clip{false};

			ScopedClip(SDL_Renderer *r, const SDL_Rect &rect) : renderer(r)
			{
				had_clip = SDL_RenderIsClipEnabled(renderer);
				if (had_clip) {
					SDL_RenderGetClipRect(renderer, &prev);
				}
				SDL_RenderSetClipRect(renderer, &rect);
			}

			~ScopedClip()
			{
				if (had_clip) {
					SDL_RenderSetClipRect(renderer, &prev);
				} else {
					SDL_RenderSetClipRect(renderer, nullptr);
				}
			}
		};

		ScopedClip clip(_renderer, rect);

        std::u32string cluster = BuildCluster(ci);
        if (cluster.empty()) {
            return;
        }

        if (cluster.size() == 1 && DrawBoxCharacter(cluster[0], fg_color, rect.x, rect.y)) {
            return;
        }

        Glyph *glyph = GetGlyph(cluster);
        if (!glyph) {
            return;
        }

        SDL_Texture *texture = glyph->texture;
        const SDL_Rect *src = nullptr;
        if (!texture && glyph->atlas) {
            texture = glyph->atlas->Texture();
            src = &glyph->atlas_rect;
        }

        if (!texture) {
            return;
        }

		const SDL_BlendMode blend_mode = ComposePremultBlendMode();
		SetTextureStateCached(texture, fg, glyph->is_color, blend_mode);

        SDL_Rect dst{
            rect.x + glyph->bearing_x,
            rect.y + layout.ascent_px - glyph->bearing_y,
            glyph->width,
            glyph->height
        };
        if (glyph->is_color && glyph->width > 0 && glyph->height > 0) {
            const float scale_w = static_cast<float>(rect.w) / static_cast<float>(glyph->width);
            const float scale_h = static_cast<float>(rect.h) / static_cast<float>(glyph->height);
            const float scale = std::min(1.0f, std::min(scale_w, scale_h));
            if (scale < 1.0f) {
                int dst_w = std::max(1, static_cast<int>(std::lround(glyph->width * scale)));
                int dst_h = std::max(1, static_cast<int>(std::lround(glyph->height * scale)));
                if (dst_w > rect.w) dst_w = rect.w;
                if (dst_h > rect.h) dst_h = rect.h;
                dst.x = rect.x + (rect.w - dst_w) / 2;
                dst.y = rect.y + (rect.h - dst_h) / 2;
                dst.w = dst_w;
                dst.h = dst_h;
            }
        }

        SDL_RenderCopy(_renderer, texture, src, &dst);
    }

	Glyph *GetGlyph(const std::u32string &cluster)
	{
		auto it = _glyph_cache.find(cluster);
		if (it != _glyph_cache.end()) {
			++_glyph_cache_hits;
			_glyph_lru.splice(_glyph_lru.begin(), _glyph_lru, it->second.lru_it);
			return &it->second.glyph;
		}

		++_glyph_cache_misses;
		if (!_font_manager.HasFaces()) {
			return nullptr;
		}

		SDLFontManager::GlyphBitmap bitmap;
		if (!_font_manager.RasterizeCluster(cluster, bitmap)) {
			return nullptr;
		}

		Glyph glyph;
		glyph.width = bitmap.width;
		glyph.height = bitmap.height;
		glyph.bearing_x = bitmap.bearing_x;
		glyph.bearing_y = bitmap.bearing_y;
		glyph.advance = bitmap.advance;
		glyph.is_color = bitmap.is_color;

		const bool use_bitmap_fallback = bitmap.is_color || _bitmap_fallback ||
			bitmap.width > kGlyphAtlasWidthPx || bitmap.height > kGlyphAtlasHeightPx;
		if (!use_bitmap_fallback) {
			if (!UploadGlyphToAtlas(bitmap, glyph)) {
				if (g_sdl_debug_redraw) {
					SDLDebugLog("SDLConsoleRenderer: atlas upload failed, falling back to standalone texture");
				}
			}
		}

		if (!glyph.atlas) {
			if (!bitmap.surface) {
				return nullptr;
			}
			SDL_Texture *texture = SDL_CreateTextureFromSurface(_renderer, bitmap.surface.get());
			if (!texture) {
				return nullptr;
			}
#if SDL_VERSION_ATLEAST(2, 0, 12)
			if (bitmap.is_color) {
				SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
			}
#endif
			SDL_SetTextureBlendMode(texture, ComposePremultBlendMode());
			glyph.texture = texture;
		}

		if (_glyph_cache.size() >= kMaxGlyphCacheEntries) {
			EvictGlyphFromCache();
		}
		_glyph_lru.emplace_front(cluster);
		GlyphCacheEntry entry;
		entry.glyph = std::move(glyph);
		entry.lru_it = _glyph_lru.begin();
		auto inserted = _glyph_cache.emplace(*entry.lru_it, std::move(entry));
		if (!inserted.second) {
			_glyph_lru.erase(_glyph_lru.begin());
			return &inserted.first->second.glyph;
		}

		return &inserted.first->second.glyph;
	}
	void ClearGlyphCache()
	{
		for (auto &kv : _glyph_cache) {
			if (kv.second.glyph.texture) {
				QueueTextureDestroy(kv.second.glyph.texture);
				kv.second.glyph.texture = nullptr;
			}
			kv.second.glyph.atlas = nullptr;
		}
		_glyph_cache.clear();
		_glyph_lru.clear();
		ClearGlyphAtlases();
	}

	bool UploadGlyphToAtlas(SDLFontManager::GlyphBitmap &bitmap, Glyph &glyph)
	{
		if (!bitmap.surface || bitmap.width <= 0 || bitmap.height <= 0) {
			return false;
		}

		SDL_Rect placement{0, 0, bitmap.width, bitmap.height};
		GlyphAtlas *target = nullptr;
		for (auto &atlas : _glyph_atlases) {
			if (atlas && atlas->Allocate(bitmap.width, bitmap.height, placement)) {
				target = atlas.get();
				break;
			}
		}

		if (!target) {
			target = CreateAtlas();
			if (!target) {
				return false;
			}
			if (!target->Allocate(bitmap.width, bitmap.height, placement)) {
				return false;
			}
		}

		if (!target->Upload(placement, bitmap.surface.get())) {
			return false;
		}
		glyph.atlas = target;
		glyph.atlas_rect = placement;
		return true;
	}

	GlyphAtlas *CreateAtlas()
	{
		if (!_renderer) {
			return nullptr;
		}
		auto atlas = std::make_unique<GlyphAtlas>(_renderer, kGlyphAtlasWidthPx, kGlyphAtlasHeightPx);
		if (!atlas || !atlas->Texture()) {
			return nullptr;
		}
		GlyphAtlas *ptr = atlas.get();
		_glyph_atlases.emplace_back(std::move(atlas));
		if (SDL_TRACE_ENABLED()) {
			SDL_TRACE_EVENT("renderer", "glyph_atlas_created", "pages=%zu", _glyph_atlases.size());
		}
		return ptr;
	}

	void ClearGlyphAtlases()
	{
		_glyph_atlases.clear();
	}

	void EvictGlyphFromCache()
	{
		if (_glyph_lru.empty()) {
			return;
		}
		auto tail_it = std::prev(_glyph_lru.end());
		const std::u32string &key = *tail_it;
		auto map_it = _glyph_cache.find(key);
		if (map_it != _glyph_cache.end()) {
			if (map_it->second.glyph.texture) {
				QueueTextureDestroy(map_it->second.glyph.texture);
				map_it->second.glyph.texture = nullptr;
			}
			_glyph_cache.erase(map_it);
		}
		_glyph_lru.erase(tail_it);
	}

	void QueueTextureDestroy(SDL_Texture *texture)
	{
		if (!texture) {
			return;
		}
		_textures_pending_destroy.push_back(texture);
		if (_textures_pending_destroy.size() >= kMaxPendingTextureDestroysBeforeFlush) {
			FlushPendingTextureDestroys(true);
		}
	}

	void FlushPendingTextureDestroys(bool force_renderer_flush = false)
	{
		if (_textures_pending_destroy.empty()) {
			return;
		}
#if SDL_VERSION_ATLEAST(2, 0, 10)
		if (force_renderer_flush && _renderer) {
			SDL_RenderFlush(_renderer);
		}
#else
		(void)force_renderer_flush;
#endif
		for (SDL_Texture *tex : _textures_pending_destroy) {
			if (tex) {
				SDL_DestroyTexture(tex);
			}
		}
		_textures_pending_destroy.clear();
	}

	void ClearFontFaces()
	{
		ClearGlyphCache();
		_font_manager.Shutdown();
	}

	SDL_Window *_window{nullptr};
	SDL_Renderer *_renderer{nullptr};
	SDLFontManager _font_manager;

    unsigned int _cols{0};
    unsigned int _rows{0};
	float _font_scale{1.f};

	bool _frame_active{false};
	bool _needs_full_redraw{true};

	std::unordered_map<std::u32string, GlyphCacheEntry, U32StringHash> _glyph_cache;
	std::list<std::u32string> _glyph_lru;
	std::vector<std::unique_ptr<GlyphAtlas>> _glyph_atlases;
	std::vector<SDL_Texture *> _textures_pending_destroy;
	CursorState _cursor;
	RenderLayout _layout{};
	RenderLayout _frame_layout{};
	bool _layout_dirty{true};
	bool _frame_layout_active{false};
	size_t _glyph_cache_hits{0};
	size_t _glyph_cache_misses{0};
	bool _bitmap_fallback{false};

	SDLImageManager _image_manager;
	SDL_Texture *_backing_texture{nullptr};
	int _backing_w{0};
	int _backing_h{0};
	bool _use_backing_texture{true};
	bool _present_full_copy{false};
	std::vector<SDL_Rect> _present_dirty_rects;
	COORD _last_cursor_drawn_pos{-1, -1};
	bool _last_cursor_drawn_visible{false};
	bool _last_cursor_drawn_blink_state{false};
	SDL_Color _last_draw_color{};
	bool _last_draw_color_valid{false};
	SDL_Texture *_last_texture{nullptr};
	SDL_Color _last_tex_mod{};
	bool _last_tex_mod_valid{false};
	SDL_BlendMode _last_tex_blend{SDL_BLENDMODE_NONE};
	bool _last_tex_blend_valid{false};
	bool _qedit_active{false};
	SMALL_RECT _qedit_rect{};

	void SetDrawColorCached(const SDL_Color &color)
	{
		if (!_last_draw_color_valid || _last_draw_color.r != color.r || _last_draw_color.g != color.g ||
				_last_draw_color.b != color.b || _last_draw_color.a != color.a) {
			SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, color.a);
			_last_draw_color = color;
			_last_draw_color_valid = true;
		}
	}

	void SetTextureStateCached(SDL_Texture *texture, const WinPortRGB &fg, bool is_color, SDL_BlendMode blend_mode)
	{
		if (texture != _last_texture) {
			_last_texture = texture;
			_last_tex_mod_valid = false;
			_last_tex_blend_valid = false;
		}
		SDL_Color mod_color = is_color ? SDL_Color{255, 255, 255, 255} : SDL_Color{fg.r, fg.g, fg.b, 255};
		if (!_last_tex_mod_valid || _last_tex_mod.r != mod_color.r || _last_tex_mod.g != mod_color.g ||
				_last_tex_mod.b != mod_color.b || _last_tex_mod.a != mod_color.a) {
			SDL_SetTextureColorMod(texture, mod_color.r, mod_color.g, mod_color.b);
			_last_tex_mod = mod_color;
			_last_tex_mod_valid = true;
		}
		if (!_last_tex_blend_valid || _last_tex_blend != blend_mode) {
			SDL_SetTextureBlendMode(texture, blend_mode);
			_last_tex_blend = blend_mode;
			_last_tex_blend_valid = true;
		}
	}

	void DrawCursorOverlay()
	{
		if (!g_winport_con_out || !_renderer) {
			return;
		}
		unsigned int cols = 0, rows = 0;
		g_winport_con_out->GetSize(cols, rows);
		if (cols != _cols || rows != _rows) {
			_cols = cols;
			_rows = rows;
			MarkLayoutDirty("DrawCursorOverlay size sync");
			if (_frame_layout_active) {
				RecomputeLayout(_frame_layout);
			}
		}
		const RenderLayout &layout = ResolveLayout();
		UCHAR height = 0;
		bool visible = false;
		COORD pos = g_winport_con_out->GetCursor(height, visible);
		_cursor.pos = pos;
		_cursor.visible = visible;
		_cursor.height = height ? height : 100;

		if (!visible || pos.X < 0 || pos.Y < 0 || pos.X >= (SHORT)_cols || pos.Y >= (SHORT)_rows) {
			_cursor.blink_dirty = false;
			return;
		}

		auto now = std::chrono::steady_clock::now();
		if (now < _cursor.suppress_until) {
			_cursor.blink_state = true;
			_cursor.blink_dirty = false;
			_cursor.next_toggle = now + std::chrono::milliseconds(_cursor.blink_interval ? _cursor.blink_interval : 500);
		} else if (_cursor.blink_interval) {
			if (_cursor.blink_dirty || now >= _cursor.next_toggle) {
				_cursor.blink_state = !_cursor.blink_state;
				_cursor.next_toggle = now + std::chrono::milliseconds(_cursor.blink_interval);
				_cursor.blink_dirty = false;
			}
		} else {
			_cursor.blink_state = true;
			_cursor.blink_dirty = false;
		}

		if (!_cursor.blink_state) {
			return;
		}

		CHAR_INFO ci{};
		if (!g_winport_con_out->Read(ci, pos)) {
			return;
		}

		const WinPortRGB bg = SDLConsoleBackground2RGB(ci.Attributes);
		SDL_Color cursor_color{Uint8(bg.r ^ 0xff), Uint8(bg.g ^ 0xff), Uint8(bg.b ^ 0xff), 255};

		const int px = CellToPixelX(layout, pos.X);
		const int py = CellToPixelY(layout, pos.Y);
		const int cell_h = layout.cell_px_h;

		int cursor_h = (cell_h * (_cursor.height ? _cursor.height : 100)) / 100;
		cursor_h = std::clamp(cursor_h, 1, cell_h);

		const int cursor_y = py + (cell_h - cursor_h);
		SDL_Rect rect{px, cursor_y, layout.cell_px_w, cursor_h};
		SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(_renderer, cursor_color.r, cursor_color.g, cursor_color.b, cursor_color.a);
		SDL_RenderFillRect(_renderer, &rect);
	}

	void DrawQuickEditOverlay()
	{
		if (!g_winport_con_out || !_renderer || !_qedit_active) {
			return;
		}
		SMALL_RECT area = _qedit_rect;
		if (area.Left > area.Right) std::swap(area.Left, area.Right);
		if (area.Top > area.Bottom) std::swap(area.Top, area.Bottom);
		if (area.Right < 0 || area.Bottom < 0 || area.Left >= (SHORT)_cols || area.Top >= (SHORT)_rows) {
			return;
		}
		const SHORT top = std::max<SHORT>(0, area.Top);
		const SHORT bottom = std::min<SHORT>((SHORT)_rows - 1, area.Bottom);
		const SHORT left = std::max<SHORT>(0, area.Left);
		const SHORT right = std::min<SHORT>((SHORT)_cols - 1, area.Right);
		if (top > bottom || left > right) {
			return;
		}

		for (SHORT row = top; row <= bottom; ++row) {
			IConsoleOutput::DirectLineAccess dla(g_winport_con_out, row);
			const CHAR_INFO *line = dla.Line();
			if (!line) {
				continue;
			}
			const unsigned int width = dla.Width();
			if (!width) {
				continue;
			}
			const SHORT max_col = static_cast<SHORT>(width - 1);
			const SHORT col_end = std::min(right, max_col);
			for (SHORT col = left; col <= col_end; ++col) {
				if (col > 0 && line[col].Char.UnicodeChar == 0 && CI_FULL_WIDTH_CHAR(line[col - 1])) {
					continue;
				}
				CHAR_INFO tmp = line[col];
				DWORD64 attributes = tmp.Attributes ^ kColorAttributesMask;
				if (attributes & FOREGROUND_TRUECOLOR) {
					attributes ^= 0x000000ffffff0000;
				}
				if (attributes & BACKGROUND_TRUECOLOR) {
					attributes ^= 0xffffff0000000000;
				}
				tmp.Attributes = attributes;
				DrawCell(col, row, tmp);
			}
		}
	}

	bool DrawBoxCharacter(uint32_t codepoint, const SDL_Color &fg, int px, int py);
	bool EnsureBackingTexture(const RenderLayout &layout);
	void DestroyBackingTexture();
};
bool SDLConsoleRendererImpl::CursorBlinkDue()
{
	if (!g_winport_con_out) {
		return false;
	}

	UCHAR height = 0;
	bool visible = false;
	COORD pos = g_winport_con_out->GetCursor(height, visible);
	_cursor.pos = pos;
	_cursor.visible = visible;
	_cursor.height = height ? height : 100;

	if (!visible || pos.X < 0 || pos.Y < 0 || pos.X >= (SHORT)_cols || pos.Y >= (SHORT)_rows) {
		_cursor.blink_dirty = false;
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	if (now < _cursor.suppress_until) {
		if (!_cursor.blink_state) {
			_cursor.blink_state = true;
			_cursor.blink_dirty = true;
		}
		return _cursor.blink_dirty;
	}
	if (_cursor.blink_interval == 0) {
		_cursor.blink_dirty = false;
		return false;
	}

	if (_cursor.blink_dirty) {
		return true;
	}

	if (now >= _cursor.next_toggle) {
		_cursor.blink_dirty = true;
		return true;
	}
	return false;
}

void SDLConsoleRendererImpl::SuppressCursorBlinkFor(std::chrono::milliseconds duration)
{
	auto now = std::chrono::steady_clock::now();
	auto until = now + duration;
	if (until > _cursor.suppress_until) {
		_cursor.suppress_until = until;
	}
	if (!_cursor.blink_state) {
		_cursor.blink_state = true;
		_cursor.blink_dirty = true;
	}
	_cursor.next_toggle = now + std::chrono::milliseconds(_cursor.blink_interval ? _cursor.blink_interval : 500);
}

void SDLConsoleRendererImpl::DestroyBackingTexture()
{
	if (_backing_texture) {
		SDL_DestroyTexture(_backing_texture);
		_backing_texture = nullptr;
	}
	_backing_w = 0;
	_backing_h = 0;
	_frame_active = false;
	_frame_layout_active = false;
}

bool SDLConsoleRendererImpl::EnsureBackingTexture(const RenderLayout &layout)
{
	if (!_renderer) {
		return false;
	}
	if (!_use_backing_texture) {
		return true;
	}
	const int needed_w = std::max(1, layout.console_px_w);
	const int needed_h = std::max(1, layout.console_px_h);
	if (needed_w <= 0 || needed_h <= 0) {
		return false;
	}

	if (_backing_texture && needed_w == _backing_w && needed_h == _backing_h) {
		return true;
	}

	DestroyBackingTexture();

	SDL_Texture *texture = SDL_CreateTexture(
		_renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		needed_w,
		needed_h);
	if (!texture) {
		fprintf(stderr, "SDLConsoleRenderer: failed to create backing texture %dx%d: %s\n",
			needed_w, needed_h, SDL_GetError());
		return false;
	}

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
	_backing_texture = texture;
	_backing_w = needed_w;
	_backing_h = needed_h;

	SDL_SetRenderTarget(_renderer, _backing_texture);
	SDL_RenderSetScale(_renderer, 1.f, 1.f);
	SDL_RenderSetViewport(_renderer, nullptr);
	SDL_RenderSetClipRect(_renderer, nullptr);
	SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
	SDL_RenderClear(_renderer);
	SDL_SetRenderTarget(_renderer, nullptr);
	SDL_RenderSetViewport(_renderer, nullptr);
	SDL_RenderSetClipRect(_renderer, nullptr);

	RequestFullRedraw("BackingTextureRecreated");
	return true;
}

bool SDLConsoleRendererImpl::SetImage(const std::string &id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height,
	const uint8_t *buffer, size_t buffer_size)
{
	const bool ok = _image_manager.SetImage(_renderer, id, flags, area, width, height, buffer, buffer_size,
		_font_manager.CellHeight());
	if (ok) {
		RequestFullRedraw("SetImage");
	}
	return ok;
}

bool SDLConsoleRendererImpl::TransformImage(const std::string &id, const SMALL_RECT *area, uint16_t tf)
{
	const bool ok = _image_manager.TransformImage(_renderer, id, area, tf);
	if (ok) {
		RequestFullRedraw("TransformImage");
	}
	return ok;
}

bool SDLConsoleRendererImpl::DeleteImage(const std::string &id)
{
	const bool ok = _image_manager.DeleteImage(id);
	if (ok) {
		RequestFullRedraw("DeleteImage");
	}
	return ok;
}

bool SDLConsoleRendererImpl::DrawBoxCharacter(uint32_t codepoint, const SDL_Color &fg, int px, int py)
{
	const int cell_w = _font_manager.CellWidth();
	const int cell_h = _font_manager.CellHeight();
	if (!_renderer || cell_w <= 0 || cell_h <= 0) {
		return false;
	}
	const auto draw = SDLBoxChar::Get(codepoint);
	if (!draw) {
		return false;
	}
	SDLBoxChar::Painter painter;
	painter.renderer = _renderer;
	painter.color = fg;
	painter.origin_x = px;
	painter.origin_y = py;
	painter.fw = cell_w;
	painter.fh = cell_h;
	painter.thickness = std::max(1, std::min(cell_w, cell_h) / 8);
	painter.wc = codepoint;
	draw(painter, 0, 0);
	return true;
}

void SDLConsoleRendererImpl::DescribeImageCaps(WinportGraphicsInfo &wgi) const
{
	memset(&wgi, 0, sizeof(wgi));
	wgi.PixPerCell.X = std::max(1, _font_manager.CellWidth());
	wgi.PixPerCell.Y = std::max(1, _font_manager.CellHeight());
	DWORD64 caps = WP_IMGCAP_RGBA | WP_IMGCAP_ATTACH | WP_IMGCAP_SCROLL | WP_IMGCAP_ROTMIR;
#if defined(__APPLE__)
	caps |= WP_IMGCAP_PNG | WP_IMGCAP_JPG;
#endif
	wgi.Caps = caps;
}

bool SDLConsoleRendererImpl::ReloadFontsFromConfig()
{
	ClearFontFaces();
	const float font_scale = GetFontScale();
	if (!_font_manager.Initialize(font_scale, g_sdl_subpixel_aa, g_sdl_font_hinting)) {
		fprintf(stderr, "SDLConsoleRenderer: ReloadFontsFromConfig failed\n");
		return false;
	}
	_font_scale = font_scale;
	RequestFullRedraw("ReloadFontsFromConfig");
	MarkLayoutDirty("ReloadFontsFromConfig");
	return true;
}

bool SDLConsoleRendererImpl::ReloadFontsIfScaleChanged()
{
	const float font_scale = GetFontScale();
	if (_font_manager.HasFaces() && std::abs(font_scale - _font_scale) < 0.01f) {
		return false;
	}

	ClearFontFaces();
	if (!_font_manager.Initialize(font_scale, g_sdl_subpixel_aa, g_sdl_font_hinting)) {
		fprintf(stderr, "SDLConsoleRenderer: ReloadFontsIfScaleChanged failed\n");
		return false;
	}

	_font_scale = font_scale;
	RequestFullRedraw("ReloadFontsIfScaleChanged");
	MarkLayoutDirty("ReloadFontsIfScaleChanged");
	return true;
}
