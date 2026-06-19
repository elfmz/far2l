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
#include "SDLConsoleRenderer.hpp"
#include "SDLBackendUtils.h"
#include "SDLFontManager.h"
#include "SDLPrinterSupport.h"

#include <SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#ifndef FT_LOAD_COLOR
#define FT_LOAD_COLOR 0
#endif
#include <hb.h>
#include <hb-ft.h>
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

bool WinPortClipboard_IsBusy();

WINPORT_DECL_DEF(FreezeConsoleOutput, VOID, ());
WINPORT_DECL_DEF(UnfreezeConsoleOutput, VOID, ());

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

#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

IConsoleOutput *g_winport_con_out = nullptr;
IConsoleInput *g_winport_con_in = nullptr;

bool g_sdl_norgb = false;

namespace
{
static const int g_sdl_dirty_pad_cells = 1;

#ifdef __APPLE__
std::atomic<DWORD> g_sdl_keyboard_leds_state{0};
#endif

struct WinState
{
	bool valid{false};
	bool maximized{false};
	bool fullscreen{false};
	int width{0};
	int height{0};
	int pos_x{SDL_WINDOWPOS_UNDEFINED};
	int pos_y{SDL_WINDOWPOS_UNDEFINED};

	WinState()
	{
		const std::string sdl_path = InMyConfig("sdl_winstate");
		const std::string legacy_path = InMyConfig("winstate");
		std::ifstream is(sdl_path.c_str());
		if (!is.is_open()) {
			is.open(legacy_path.c_str());
		}
		if (!is.is_open()) {
			return;
		}
		std::string str;
		if (!std::getline(is, str)) {
			return;
		}
		int flags = std::atoi(str.c_str());
		if ((flags & 1) == 0) {
			return;
		}
		valid = true;
		maximized = (flags & 2) != 0;
		fullscreen = (flags & 4) != 0;

		if (!std::getline(is, str)) return;
		const int w = std::atoi(str.c_str());
		if (!std::getline(is, str)) return;
		const int h = std::atoi(str.c_str());
		if (w >= 100 && h >= 100) {
			width = w;
			height = h;
		}
		if (!std::getline(is, str)) return;
		pos_x = std::atoi(str.c_str());
		if (!std::getline(is, str)) return;
		pos_y = std::atoi(str.c_str());
	}

	void Save() const
	{
		std::ofstream os(InMyConfig("sdl_winstate").c_str());
		if (!os.is_open()) {
			fprintf(stderr, "WinState: can't create\n");
			return;
		}
		int flags = 1;
		if (maximized) flags |= 2;
		if (fullscreen) flags |= 4;
		os << flags << std::endl;
		os << width << std::endl;
		os << height << std::endl;
		os << pos_x << std::endl;
		os << pos_y << std::endl;
		fprintf(stderr, "WinState: saved flags=%d size={%d,%d} pos={%d,%d}\n",
			flags, width, height, pos_x, pos_y);
	}
};

std::atomic<bool> g_sigint_pending{false};

struct SDLDebugLoggerInit
{
	SDLDebugLoggerInit()
	{
		if (g_sdl_debug_redraw) {
			SDLDebugLog("FAR2L_SDL_DEBUG_REDRAW logging enabled");
		}
	}
} g_sdl_debug_logger_init;

void SDLSigintHandler(int)
{
	g_sigint_pending.store(true);
}

class SigIntGuard
{
public:
	SigIntGuard()
		: _prev(signal(SIGINT, SDLSigintHandler))
	{
	}
	~SigIntGuard()
	{
		signal(SIGINT, _prev);
	}

private:
	using Handler = void (*)(int);
	Handler _prev;
};

static constexpr size_t kCommandQueueSize = 1024;
static_assert((kCommandQueueSize & (kCommandQueueSize - 1)) == 0, "command queue must be power of two");
static constexpr size_t kCommandQueueMask = kCommandQueueSize - 1;
static constexpr int kFrameIntervalMs = 16;
static constexpr auto kFrameInterval = std::chrono::milliseconds(kFrameIntervalMs);
// When we defer SDL_DestroyTexture the texture stays resident in GPU memory until we flush.
// Keep the backlog tiny so we don't balloon Metal allocations during heavy redraws.

enum class BackendCommandType
{
	Refresh,
	FullRefresh,
	ConsoleResized,
	TitleChanged,
	Flush,
	SetMaximized,
	WindowMoved,
	DisplayNotification,
	ChangeFont,
	SetImage,
	TransformImage,
	DeleteImage
};

static const char *BackendCommandTypeToString(BackendCommandType type)
{
	switch (type) {
	case BackendCommandType::Refresh: return "Refresh";
	case BackendCommandType::FullRefresh: return "FullRefresh";
	case BackendCommandType::ConsoleResized: return "ConsoleResized";
	case BackendCommandType::TitleChanged: return "TitleChanged";
	case BackendCommandType::Flush: return "Flush";
	case BackendCommandType::SetMaximized: return "SetMaximized";
	case BackendCommandType::WindowMoved: return "WindowMoved";
	case BackendCommandType::DisplayNotification: return "DisplayNotification";
	case BackendCommandType::ChangeFont: return "ChangeFont";
	case BackendCommandType::SetImage: return "SetImage";
	case BackendCommandType::TransformImage: return "TransformImage";
	case BackendCommandType::DeleteImage: return "DeleteImage";
	}
	return "Unknown";
}

struct BackendCommand
{
	BackendCommandType type;
	std::vector<SMALL_RECT> areas;
	bool bool_value{false};
	COORD coord_value{};
	std::wstring wide_a;
	std::wstring wide_b;
	std::string str_value;
	DWORD64 flags_value{0};
	SMALL_RECT rect_value{-1, -1, -1, -1};
	bool has_rect{false};
	uint32_t width_value{0};
	uint32_t height_value{0};
	uint16_t tf_value{0};
	std::vector<uint8_t> binary;
	std::shared_ptr<std::promise<bool>> completion;
};

class ConsoleModel
{
public:
	IConsoleOutput *Output() const { return g_winport_con_out; }
};

class SDLClipboardBackend : public IClipboardBackend
{
	int is_primary {0};
public:
	SDLClipboardBackend() = default;
	~SDLClipboardBackend() override = default;

	bool OnClipboardOpen() override { return true; }
	void OnClipboardClose() override {}

	void OnClipboardEmpty() override
	{
		is_primary ? SDL_SetPrimarySelectionText("") : SDL_SetClipboardText("");
	}

	bool OnClipboardIsFormatAvailable(UINT format) override
	{
		if (format == CF_UNICODETEXT || format == CF_TEXT) {
			return (is_primary ? SDL_HasPrimarySelectionText() : SDL_HasClipboardText() ) ? true : false;
		}
		return false;
	}

	void *OnClipboardSetData(UINT format, void *data) override
	{
		if (!data) {
			return nullptr;
		}
		std::string utf8;
		if (format == CF_UNICODETEXT) {
			Wide2MB((const wchar_t *)data, utf8);
		} else if (format == CF_TEXT) {
			const size_t len = WINPORT(ClipboardSize)(data);
			const char *src = static_cast<const char *>(data);
			utf8.assign(src, src + len);
		} else {
			return nullptr;
		}
		if (is_primary)
			SDL_SetPrimarySelectionText(utf8.c_str());
		else
			SDL_SetClipboardText(utf8.c_str());
		return data;
	}

	void *OnClipboardGetData(UINT format) override
	{
		char *text = is_primary ? SDL_GetPrimarySelectionText() : SDL_GetClipboardText();
		if (!text) {
			return nullptr;
		}

		void *out = nullptr;
		if (format == CF_UNICODETEXT) {
			const std::wstring wide = MB2Wide(text);
			out = ClipboardAllocFromZeroTerminatedString(wide.c_str());
		} else if (format == CF_TEXT) {
			out = ClipboardAllocFromZeroTerminatedString(text);
		}
		SDL_free(text);
		return out;
	}

	UINT OnClipboardRegisterFormat(const wchar_t *) override
	{
		return 0;
	}

	INT ChooseClipboard(INT format) 
	{ 
		int old = is_primary;
		is_primary = format > 0;
		return old != is_primary;
	}
};

class DirtyTracker
{
public:
	struct FrameStats
	{
		size_t pending_regions{0};
		size_t dirty_rects{0};
		size_t dirty_cells{0};
		bool full_redraw{false};
	};

	void MarkAreas(const std::vector<SMALL_RECT> &areas)
	{
		for (const auto &area_in : areas) {
			SMALL_RECT area = area_in;
			NormalizeArea(area);
			if (area.Left > area.Right || area.Top > area.Bottom) {
				continue;
			}
			bool merged = true;
			while (merged) {
				merged = false;
				for (size_t idx = 0; idx < _pending.size(); ++idx) {
					if (OverlapOrAdjacent(_pending[idx], area)) {
						area = UnionRect(_pending[idx], area);
						_pending.erase(_pending.begin() + idx);
						merged = true;
						break;
					}
				}
			}
			_pending.push_back(area);
			if (_pending.size() > kMaxPendingRects) {
				RequestFullRedraw();
				return;
			}
		}
	}

	void RequestFullRedraw()
	{
		_full_redraw = true;
		_pending.clear();
	}
	void RequestScan()
	{
		_force_scan = true;
	}

	void Resize(IConsoleOutput *console, unsigned int cols, unsigned int rows)
	{
		_cols = cols;
		_rows = rows;
		_snapshot.assign(static_cast<size_t>(_cols) * _rows, {});
		CaptureFullSnapshot(console);
		_full_redraw = true;
		_pending.clear();
	}

	bool HasDirty() const
	{
		return _full_redraw || !_pending.empty();
	}

	const FrameStats &LastStats() const { return _last_stats; }

	bool Consume(ConsoleModel &model, unsigned int cols, unsigned int rows, bool &full_redraw, std::vector<SMALL_RECT> &areas, FrameStats &stats)
	{
		stats = {};
		stats.pending_regions = _pending.size();
		full_redraw = false;
		areas.clear();

		IConsoleOutput *console = model.Output();
		if (!console || cols == 0 || rows == 0) {
			_pending.clear();
			_last_stats = stats;
			return false;
		}

		if (_cols != cols || _rows != rows || _snapshot.size() != static_cast<size_t>(cols) * rows) {
			Resize(console, cols, rows);
		}

		const unsigned int capped_rows = std::min<unsigned int>(_rows, static_cast<unsigned int>(SHRT_MAX));
		const unsigned int capped_cols = std::min<unsigned int>(_cols, static_cast<unsigned int>(SHRT_MAX));
		const SHORT max_row = capped_rows ? static_cast<SHORT>(capped_rows - 1) : static_cast<SHORT>(-1);
		const SHORT max_col = capped_cols ? static_cast<SHORT>(capped_cols - 1) : static_cast<SHORT>(-1);
		const SHORT pad = static_cast<SHORT>(std::max(0, g_sdl_dirty_pad_cells));

		if (_full_redraw || _snapshot.empty()) {
			full_redraw = true;
			stats.full_redraw = true;
			stats.dirty_cells = static_cast<size_t>(_cols) * _rows;
			if (capped_cols > 0 && capped_rows > 0) {
				stats.dirty_rects = 1;
				areas.push_back(SMALL_RECT{
					0,
					0,
					static_cast<SHORT>(capped_cols - 1),
					static_cast<SHORT>(capped_rows - 1)});
			}
			CaptureFullSnapshot(console);
			_full_redraw = false;
			_pending.clear();
			_last_stats = stats;
			return true;
		}

		if (_force_scan && _pending.empty()) {
			if (capped_cols > 0 && capped_rows > 0) {
				_pending.push_back(SMALL_RECT{
					0,
					0,
					static_cast<SHORT>(capped_cols - 1),
					static_cast<SHORT>(capped_rows - 1)});
			}
			_force_scan = false;
		}
		if (_pending.empty()) {
			_last_stats = stats;
			return false;
		}

		_span_buffer.clear();

		for (const auto &rect : _pending) {
			const SHORT top = std::max<SHORT>(0, static_cast<SHORT>(rect.Top - pad));
			const SHORT bottom = max_row >= 0
				? std::min<SHORT>(max_row, static_cast<SHORT>(rect.Bottom + pad))
				: static_cast<SHORT>(-1);
			const SHORT left = std::max<SHORT>(0, static_cast<SHORT>(rect.Left - pad));
			const SHORT right = max_col >= 0
				? std::min<SHORT>(max_col, static_cast<SHORT>(rect.Right + pad))
				: static_cast<SHORT>(-1);
			if (top > bottom || left > right) {
				continue;
			}

			for (SHORT row = top; row <= bottom; ++row) {
				IConsoleOutput::DirectLineAccess dla(console, row);
				const CHAR_INFO *line = dla.Line();
				const unsigned int width = dla.Width();

				SHORT run_start = -1;
				SHORT run_end = -1;
				for (SHORT col = left; col <= right; ++col) {
					const size_t idx = static_cast<size_t>(row) * _cols + static_cast<unsigned int>(col);
					CellSignature current = {};
					if (line && static_cast<unsigned int>(col) < width) {
						current = MakeSignature(line[col]);
					}
					if (_snapshot[idx] == current) {
						if (run_start >= 0) {
							const SHORT span_left = std::max<SHORT>(left, static_cast<SHORT>(run_start - pad));
							const SHORT span_right = std::min<SHORT>(right, static_cast<SHORT>(run_end + pad));
							_span_buffer.push_back(SMALL_RECT{span_left, row, span_right, row});
							run_start = -1;
							run_end = -1;
						}
						continue;
					}

					_snapshot[idx] = current;
					++stats.dirty_cells;
					if (run_start < 0) {
						run_start = col;
					}
					run_end = col;
				}
				if (run_start >= 0) {
					const SHORT span_left = std::max<SHORT>(left, static_cast<SHORT>(run_start - pad));
					const SHORT span_right = std::min<SHORT>(right, static_cast<SHORT>(run_end + pad));
					_span_buffer.push_back(SMALL_RECT{span_left, row, span_right, row});
				}
			}
		}

		_pending.clear();

		if (_span_buffer.empty()) {
			_last_stats = stats;
			return false;
		}

		areas.reserve(_span_buffer.size());
		for (const auto &rect : _span_buffer) {
			if (!areas.empty()) {
				auto &prev = areas.back();
				if (rect.Left == prev.Left && rect.Right == prev.Right && rect.Top == prev.Bottom + 1) {
					prev.Bottom = rect.Bottom;
					continue;
				}
			}
			areas.push_back(rect);
		}

		stats.dirty_rects = areas.size();
		_last_stats = stats;
		return true;
	}

private:
	static constexpr size_t kMaxPendingRects = 256;

	static void NormalizeArea(SMALL_RECT &area)
	{
		if (area.Left > area.Right) {
			std::swap(area.Left, area.Right);
		}
		if (area.Top > area.Bottom) {
			std::swap(area.Top, area.Bottom);
		}
	}

	static bool OverlapOrAdjacent(const SMALL_RECT &a, const SMALL_RECT &b)
	{
		return a.Left <= static_cast<SHORT>(b.Right + 1) &&
			b.Left <= static_cast<SHORT>(a.Right + 1) &&
			a.Top <= static_cast<SHORT>(b.Bottom + 1) &&
			b.Top <= static_cast<SHORT>(a.Bottom + 1);
	}

	static SMALL_RECT UnionRect(const SMALL_RECT &a, const SMALL_RECT &b)
	{
		return SMALL_RECT{
			static_cast<SHORT>(std::min(a.Left, b.Left)),
			static_cast<SHORT>(std::min(a.Top, b.Top)),
			static_cast<SHORT>(std::max(a.Right, b.Right)),
			static_cast<SHORT>(std::max(a.Bottom, b.Bottom))};
	}

	struct CellSignature
	{
		DWORD64 attrs{0};
		uint64_t glyph{0};
		bool operator==(const CellSignature &other) const
		{
			return attrs == other.attrs && glyph == other.glyph;
		}
		bool operator!=(const CellSignature &other) const
		{
			return !(*this == other);
		}
	};

	static CellSignature MakeSignature(const CHAR_INFO &ci)
	{
		return CellSignature{ci.Attributes, static_cast<uint64_t>(ci.Char.UnicodeChar)};
	}

	void CaptureFullSnapshot(IConsoleOutput *console)
	{
		if (!console || _cols == 0 || _rows == 0) {
			std::fill(_snapshot.begin(), _snapshot.end(), CellSignature{});
			return;
		}
		for (unsigned int row = 0; row < _rows; ++row) {
			IConsoleOutput::DirectLineAccess dla(console, row);
			const CHAR_INFO *line = dla.Line();
			const unsigned int width = dla.Width();
			for (unsigned int col = 0; col < _cols; ++col) {
				const size_t idx = static_cast<size_t>(row) * _cols + col;
				if (line && col < width) {
					_snapshot[idx] = MakeSignature(line[col]);
				} else {
					_snapshot[idx] = CellSignature{};
				}
			}
		}
	}

		std::vector<SMALL_RECT> _pending;
		std::vector<SMALL_RECT> _span_buffer;
		std::vector<CellSignature> _snapshot;
		unsigned int _cols{0};
		unsigned int _rows{0};
		bool _full_redraw{false};
		bool _force_scan{false};
		FrameStats _last_stats{};
	};

class RendererWorker
{
public:
	void Attach(SDLConsoleRenderer *renderer) { _renderer = renderer; }

	bool Render(ConsoleModel &model, DirtyTracker &tracker, DirtyTracker::FrameStats &stats)
	{
		if (!_renderer) {
			return false;
		}
		bool full = false;
		if (!tracker.Consume(model, _renderer->Columns(), _renderer->Rows(), full, _rect_scratch, stats)) {
			return false;
		}
		if (full) {
			_renderer->DrawFullConsole();
		} else if (!_rect_scratch.empty()) {
			_renderer->DrawAreas(_rect_scratch);
		}
		return true;
	}

private:
	SDLConsoleRenderer *_renderer{nullptr};
	std::vector<SMALL_RECT> _rect_scratch;
};

class PresentController
{
public:
	void Attach(SDLConsoleRenderer *renderer) { _renderer = renderer; }

	void NotifyFrameDrawn()
	{
		_frame_pending = true;
		if (g_sdl_debug_redraw) {
			SDLDebugLog("PresentController: frame pending (waiters=%zu)", _flush_waiters.size());
		}
	}

	void NotifyPresented()
	{
		_frame_pending = false;
		if (g_sdl_debug_redraw) {
			SDLDebugLog("PresentController: frame presented (waiters=%zu)", _flush_waiters.size());
		}
		for (auto &waiter : _flush_waiters) {
			if (waiter) {
				waiter->set_value(true);
			}
		}
		_flush_waiters.clear();
	}

	void RegisterFlushWaiter(const std::shared_ptr<std::promise<bool>> &waiter, bool frame_pending)
	{
		if (!waiter) {
			return;
		}
		if (!frame_pending && !ShouldPresent()) {
			waiter->set_value(true);
			return;
		}
		_flush_waiters.push_back(waiter);
	}

	bool ShouldPresent() const
	{
		return _frame_pending || (_renderer && _renderer->HasPendingFrame());
	}

	bool FramePending() const { return _frame_pending; }
	size_t PendingWaiters() const { return _flush_waiters.size(); }

private:
	SDLConsoleRenderer *_renderer{nullptr};
	bool _frame_pending{false};
	std::vector<std::shared_ptr<std::promise<bool>>> _flush_waiters;
};

class SDLConsoleApp;
class SDLConsoleBackend : public IConsoleOutputBackend
{
	static constexpr Uint32 kBackendTickIntervalMs = 20;
	static constexpr DWORD kQeditCopyMinimalDelayMs = 150;
	static constexpr DWORD kQeditUnfreezeDelayMs = 1000;
	static constexpr DWORD kCursorBlinkSuppressMs = 500;
	static constexpr auto kDirtyCoalesceWindow = std::chrono::milliseconds(8);
public:
	explicit SDLConsoleBackend(SDLConsoleApp &owner);
	~SDLConsoleBackend() override;

	bool Initialize(SDL_Window *window);
	void Attach();
	void Detach();
	void RequestFullRefresh();

	void HandleSDLEvent(const SDL_Event &event);
	void HandleHostResize(int pixel_w, int pixel_h);
	bool SyncRendererFontScale();
	void RequestQuit();
	void StopRunning();
	void ProcessTick();
	static Uint32 TimerCallback(Uint32 interval, void *param);

	Uint32 EventType() const { return _event_id; }
	bool IsRunning() const { return _running.load(); }
	int CellWidth() const { return _renderer.CellWidth(); }
	int CellHeight() const { return _renderer.CellHeight(); }

	// IConsoleOutputBackend
	void OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count) override;
	void OnConsoleOutputResized() override;
	void OnConsoleOutputTitleChanged() override;
	void OnConsoleOutputWindowMoved(bool absolute, COORD pos) override;
	void OnConsoleSetMaximized(bool maximized) override;
	COORD OnConsoleGetLargestWindowSize() override;
	void OnConsoleAdhocQuickEdit() override;
	DWORD64 OnConsoleSetTweaks(DWORD64 tweaks) override
	{
		if (tweaks != TWEAKS_ONLY_QUERY_SUPPORTED) {
			_tweaks = tweaks;
		}
		return TWEAK_STATUS_SUPPORT_CHANGE_FONT | TWEAK_STATUS_SUPPORT_BLINK_RATE;
	}
	void OnConsoleChangeFont() override { RequestFontDialog(); }
	void OnConsoleSaveWindowState() override;
	void OnConsoleExit() override;
	bool OnConsoleIsActive() override { return _focused.load(); }
	void OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text) override;
	bool OnConsoleBackgroundMode(bool) override { return false; }
	bool OnConsoleSetFKeyTitles(const char **) override { return false; }
	BYTE OnConsoleGetColorPalette() override { return g_sdl_norgb ? 4 : 24; }
	void OnConsoleGetBasePalette(void *pbuff) override
	{
		if (pbuff) {
			memcpy(pbuff, &g_winport_palette, sizeof(g_winport_palette));
		}
	}
	bool OnConsoleSetBasePalette(void *) override { return false; }
	void OnConsoleOverrideColor(DWORD, DWORD *, DWORD *) override {}
	void OnConsoleSetCursorBlinkTime(DWORD interval) override
	{
		_renderer.SetCursorBlinkInterval(interval);
	}
	void OnConsoleOutputFlushDrawing() override;

	void OnGetConsoleImageCaps(WinportGraphicsInfo *wgi) override;
	bool OnSetConsoleImage(const char *, DWORD64, const SMALL_RECT *, DWORD, DWORD, const void *) override;
	bool OnTransformConsoleImage(const char *, const SMALL_RECT *, uint16_t) override;
	bool OnDeleteConsoleImage(const char *) override;
	const char *OnConsoleBackendInfo(int entity) override;

private:
	void PushCommand(BackendCommand &&cmd);
	void NotifyMainThread();
	void RequestProcessTick();
	void ProcessCommand(const BackendCommand &cmd);
	void UpdateTitle();
	void HandleKeyEvent(const SDL_KeyboardEvent &key);
	void HandleMouseMotion(const SDL_MouseMotionEvent &motion);
	void HandleMouseButton(const SDL_MouseButtonEvent &button);
	void HandleMouseWheel(const SDL_MouseWheelEvent &wheel);
	void HandleTextInput(const SDL_TextInputEvent &text);
	void RequestFontDialog();
	void ChangeFontInteractive();
	void EmitTextInputChar(wchar_t ch);
	void EmitMouseEvent(DWORD button_state, DWORD event_flags, SHORT x, SHORT y, SHORT wheel_delta = 0);
	SHORT ClampColumn(int pixel_x) const;
	SHORT ClampRow(int pixel_y) const;
	static WORD SDLKeycodeToVK(SDL_Keycode key);
	static int IsEnhancedKey(SDL_Keycode key, SDL_Scancode scancode);
	static DWORD ModifiersToControlState(Uint16 mod);
	void DamageAreaBetween(COORD c1, COORD c2);
	void CheckPutText2Clip();
	void CheckForUnfreeze(bool force);
	void StartMouseQEdit(COORD pos);
	void UpdateMouseQEdit(COORD pos, bool moving, bool button_up);
	void UpdateQuickEditOverlay(bool force);
	static void NormalizeArea(SMALL_RECT &area);
	void ExtendDirtyCoalesce();

	SDL_Window *_window{nullptr};
	SDLConsoleRenderer _renderer;
	ConsoleModel _console_model;
	DirtyTracker _dirty_tracker;
	RendererWorker _renderer_worker;
	PresentController _present_controller;
	SDL_TimerID _tick_timer{0};
	Uint32 _event_id{0};
	std::mutex _commands_mutex;
	std::array<BackendCommand, kCommandQueueSize> _command_ring;
	size_t _command_head{0};
	size_t _command_tail{0};
	bool _tick_requested{false};
	std::chrono::steady_clock::time_point _last_present_time{std::chrono::steady_clock::now()};
	std::chrono::steady_clock::time_point _dirty_coalesce_until{std::chrono::steady_clock::time_point::min()};
	std::vector<std::shared_ptr<std::promise<bool>>> _deferred_flush_waiters;
	std::mutex _flush_mutex;
	std::shared_future<bool> _flush_future;
	bool _pending_present{false};
	uint64_t _tick_count{0};
	uint64_t _tick_rendered_count{0};
	uint64_t _tick_present_count{0};
	long long _last_stats_log_ms{0};
	std::vector<std::string> _backend_info;
	std::string _info_buffer;
	std::atomic<bool> _running{true};
	std::atomic<bool> _quit_posted{false};
	std::atomic<bool> _focused{true};
	DWORD64 _tweaks{0};
	DWORD _mouse_button_state{0};
	COORD _last_mouse_pos{0, 0};
	int _last_host_px_w{-1};
	int _last_host_px_h{-1};
	bool _adhoc_quickedit{false};
	DWORD _qedit_unfreeze_start_ticks{0};
	DWORD _mouse_qedit_start_ticks{0};
	bool _mouse_qedit_moved{false};
	COORD _mouse_qedit_start{};
	COORD _mouse_qedit_last{};
	std::wstring _text2clip;
	COORD _last_cursor_pos{-1, -1};
	bool _last_cursor_visible{false};
};

#if defined(__APPLE__)
static void SDLMacFontMenuCallback(void *ctx)
{
	if (ctx) {
		static_cast<SDLConsoleBackend *>(ctx)->OnConsoleChangeFont();
	}
}
#endif

class SDLAppWorker
{
public:
	SDLAppWorker(int argc, char **argv, int (*entry)(int, char **));
	~SDLAppWorker();

	bool Start(IConsoleOutputBackend *backend);
	void Join();
	int Result() const { return _result.load(); }

private:
	void ThreadMain();

	int _argc;
	char **_argv;
	int (*_entry_point)(int, char **);
	IConsoleOutputBackend *_backend{nullptr};
	std::thread _thread;
	std::atomic<int> _result{0};
	std::atomic<bool> _started{false};
};

class SDLConsoleApp
{
public:
	explicit SDLConsoleApp(const WinPortMainBackendArg &arg);
	~SDLConsoleApp();

	bool Initialize();
	void Run();
	void RequestQuit();

	SDLConsoleBackend *Backend() { return _backend.get(); }

private:
	void Destroy();

	const WinPortMainBackendArg &_arg;
	SDL_Window *_window{nullptr};
	std::unique_ptr<SDLConsoleBackend> _backend;
	std::unique_ptr<SDLAppWorker> _worker;
	std::atomic<bool> _running{false};
};

SDLConsoleBackend::SDLConsoleBackend(SDLConsoleApp &owner)
{
	(void)owner;
	SDL_version v{};
	SDL_GetVersion(&v);
	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), "SDL %d.%d.%d", v.major, v.minor, v.patch);
	_backend_info.emplace_back(buffer);
	_backend_info.emplace_back(std::string("Platform: ") + SDL_GetPlatform());

	_event_id = SDL_RegisterEvents(1);
	if (_event_id == (Uint32)-1) {
		_event_id = SDL_USEREVENT;
	}

	_renderer_worker.Attach(&_renderer);
	_present_controller.Attach(&_renderer);
}

SDLConsoleBackend::~SDLConsoleBackend()
{
	StopRunning();
	if (_tick_timer) {
		SDL_RemoveTimer(_tick_timer);
		_tick_timer = 0;
	}
	SDL_StopTextInput();
}

bool SDLConsoleBackend::Initialize(SDL_Window *window)
{
	_window = window;
	if (!_renderer.Initialize(window)) {
		return false;
	}
	_renderer_worker.Attach(&_renderer);
	_present_controller.Attach(&_renderer);
	SDL_StartTextInput();
	_renderer.SyncConsoleSize();
	_dirty_tracker.Resize(_console_model.Output(), _renderer.Columns(), _renderer.Rows());
	UpdateTitle();
#if defined(__APPLE__)
	SDLInstallMacFontMenu(&SDLMacFontMenuCallback, this);
#endif
	if (_tick_timer == 0) {
		_tick_timer = SDL_AddTimer(kBackendTickIntervalMs, &SDLConsoleBackend::TimerCallback, this);
	}
	return true;
}

void SDLConsoleBackend::Attach()
{
	if (g_winport_con_out) {
		g_winport_con_out->SetBackend(this);
	}
	_renderer.SyncConsoleSize();
	_dirty_tracker.Resize(_console_model.Output(), _renderer.Columns(), _renderer.Rows());
	_renderer.DrawFullConsole();
	_present_controller.NotifyFrameDrawn();
	_renderer.Present();
	_present_controller.NotifyPresented();
}

void SDLConsoleBackend::Detach()
{
	if (g_winport_con_out) {
		g_winport_con_out->SetBackend(nullptr);
	}
}

void SDLConsoleBackend::HandleSDLEvent(const SDL_Event& event)
{
	if (event.type == _event_id) {
		ProcessTick();
		return;
	}

	switch (event.type) {
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		HandleKeyEvent(event.key);
		break;
	case SDL_TEXTINPUT:
		HandleTextInput(event.text);
		break;

	case SDL_MOUSEMOTION:
		HandleMouseMotion(event.motion);
		break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		HandleMouseButton(event.button);
		break;

	case SDL_MOUSEWHEEL:
		HandleMouseWheel(event.wheel);
		break;

	case SDL_WINDOWEVENT:
		switch (event.window.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			_focused.store(true);
			break;

		case SDL_WINDOWEVENT_FOCUS_LOST:
			_focused.store(false);
			break;

		case SDL_WINDOWEVENT_MOVED:
#if SDL_VERSION_ATLEAST(2, 0, 18)
		case SDL_WINDOWEVENT_DISPLAY_CHANGED:
#endif
			SyncRendererFontScale();
			break;

		case SDL_WINDOWEVENT_EXPOSED:
			if (SyncRendererFontScale()) {
				break;
			}
			_renderer.DrawFullConsole();
			_present_controller.NotifyFrameDrawn();
			_renderer.Present();
			_present_controller.NotifyPresented();
			break;

		case SDL_WINDOWEVENT_SIZE_CHANGED:
			if (SyncRendererFontScale()) {
				break;
			}
			HandleHostResize(event.window.data1, event.window.data2);
			_renderer.SyncConsoleSize();
			_renderer.DrawFullConsole();
			_present_controller.NotifyFrameDrawn();
			_renderer.Present();
			_present_controller.NotifyPresented();
			break;

		case SDL_WINDOWEVENT_RESIZED:
			if (!SyncRendererFontScale()) {
				HandleHostResize(event.window.data1, event.window.data2);
			}
			break;

		case SDL_WINDOWEVENT_CLOSE:
			if (WINPORT(GenerateConsoleCtrlEvent)(CTRL_CLOSE_EVENT, 0)) {
				break;
			}
			RequestQuit();
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
}


void SDLConsoleBackend::HandleHostResize(int pixel_w, int pixel_h)
{
	if (!g_winport_con_out || _renderer.CellWidth() <= 0 || _renderer.CellHeight() <= 0) {
		return;
	}
	int out_px_w = 0;
	int out_px_h = 0;
	_renderer.OutputPixelSize(out_px_w, out_px_h);
	const int tracked_px_w = out_px_w > 0 ? out_px_w : pixel_w;
	const int tracked_px_h = out_px_h > 0 ? out_px_h : pixel_h;
	const bool same_px = (tracked_px_w == _last_host_px_w && tracked_px_h == _last_host_px_h);
	_last_host_px_w = tracked_px_w;
	_last_host_px_h = tracked_px_h;
	if (SDL_TRACE_ENABLED()) {
		SDL_TRACE_EVENT("console", "host_resize", "px=%d,%d cell=%d,%d",
			tracked_px_w, tracked_px_h, _renderer.CellWidth(), _renderer.CellHeight());
	}
	const int size_w = tracked_px_w;
	const int size_h = tracked_px_h;
	const unsigned int cols = std::max(1, size_w / _renderer.CellWidth());
	const unsigned int rows = std::max(1, size_h / _renderer.CellHeight());

	unsigned int cur_cols = 0, cur_rows = 0;
	g_winport_con_out->GetSize(cur_cols, cur_rows);
	if (cur_cols == cols && cur_rows == rows) {
		if (!same_px) {
			_renderer.NotifyOutputResize();
		}
		return;
	}

	g_winport_con_out->SetSize(cols, rows);
	g_winport_con_out->GetSize(cur_cols, cur_rows);

	if (g_winport_con_in) {
		INPUT_RECORD ir{};
		ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
		ir.Event.WindowBufferSizeEvent.dwSize.X = static_cast<SHORT>(std::min<unsigned int>(cur_cols, SHRT_MAX));
		ir.Event.WindowBufferSizeEvent.dwSize.Y = static_cast<SHORT>(std::min<unsigned int>(cur_rows, SHRT_MAX));
		g_winport_con_in->Enqueue(&ir, 1);
	}
}

bool SDLConsoleBackend::SyncRendererFontScale()
{
	if (!_renderer.ReloadFontsIfScaleChanged()) {
		return false;
	}

	int win_w = 0;
	int win_h = 0;
	if (_window) {
		SDL_GetWindowSize(_window, &win_w, &win_h);
	}
	if (win_w > 0 && win_h > 0) {
		HandleHostResize(win_w, win_h);
	}

	_renderer.SyncConsoleSize(false);
	_dirty_tracker.Resize(_console_model.Output(), _renderer.Columns(), _renderer.Rows());
	_renderer.DrawFullConsole();
	_present_controller.NotifyFrameDrawn();
	_renderer.Present();
	_present_controller.NotifyPresented();
	RequestFullRefresh();
	_dirty_tracker.RequestFullRedraw();
	RequestProcessTick();
	return true;
}

void SDLConsoleBackend::RequestQuit()
{
	bool expected = false;
	if (_quit_posted.compare_exchange_strong(expected, true)) {
		SDL_Event quit{};
		quit.type = SDL_QUIT;
		SDL_PushEvent(&quit);
	}
}

void SDLConsoleBackend::StopRunning()
{
	bool expected = true;
	if (_running.compare_exchange_strong(expected, false)) {
		_quit_posted.store(false);
		RequestQuit();
	}
}

Uint32 SDLConsoleBackend::TimerCallback(Uint32 interval, void *param)
{
	auto *backend = static_cast<SDLConsoleBackend *>(param);
	if (!backend || !backend->IsRunning()) {
		return interval;
	}
	backend->RequestProcessTick();
	return interval;
}

void SDLConsoleBackend::ProcessTick()
{
	if (!_running.load()) {
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	std::vector<BackendCommand> commands;
	{
		std::lock_guard<std::mutex> lock(_commands_mutex);
		while (_command_tail != _command_head) {
			commands.emplace_back(std::move(_command_ring[_command_tail]));
			_command_ring[_command_tail] = BackendCommand{};
			_command_tail = (_command_tail + 1) & kCommandQueueMask;
		}
		_tick_requested = false;
	}

	for (auto &cmd : commands) {
		ProcessCommand(cmd);
	}

	if (_dirty_tracker.HasDirty() && now < _dirty_coalesce_until) {
		if (_present_controller.PendingWaiters() == 0 && _deferred_flush_waiters.empty()) {
			if (g_sdl_debug_redraw) {
				const auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(_dirty_coalesce_until - now).count();
				SDLDebugLog("SDLConsoleBackend: coalesce wait (%lld ms)", static_cast<long long>(remaining_ms));
			}
			return;
		}
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: coalesce bypassed (waiters=%zu deferred=%zu)",
				_present_controller.PendingWaiters(),
				_deferred_flush_waiters.size());
		}
	}
	if (!_deferred_flush_waiters.empty()) {
		const bool frame_pending = _dirty_tracker.HasDirty() || _present_controller.FramePending() || _renderer.HasPendingFrame();
		if (!frame_pending && !_present_controller.ShouldPresent()) {
			for (auto &waiter : _deferred_flush_waiters) {
				if (waiter) {
					waiter->set_value(true);
				}
			}
		} else {
			for (auto &waiter : _deferred_flush_waiters) {
				_present_controller.RegisterFlushWaiter(waiter, frame_pending);
			}
		}
		_deferred_flush_waiters.clear();
	}

	CheckForUnfreeze(false);
	CheckPutText2Clip();
	if (_mouse_qedit_start_ticks != 0) {
		UpdateQuickEditOverlay(false);
	}

	const bool cursor_due = _focused.load() ? _renderer.CursorBlinkDue() : false;
	COORD cursor_pos{};
	bool cursor_visible = false;
	_renderer.GetCursorState(cursor_pos, cursor_visible);
	if (cursor_visible != _last_cursor_visible || cursor_pos.X != _last_cursor_pos.X || cursor_pos.Y != _last_cursor_pos.Y) {
		const SHORT max_col = static_cast<SHORT>(_renderer.Columns());
		const SHORT max_row = static_cast<SHORT>(_renderer.Rows());
		auto in_bounds = [&](const COORD &pos) {
			return pos.X >= 0 && pos.Y >= 0 && pos.X < max_col && pos.Y < max_row;
		};
		if (_last_cursor_visible && in_bounds(_last_cursor_pos)) {
			SMALL_RECT old_area{_last_cursor_pos.X, _last_cursor_pos.Y, _last_cursor_pos.X, _last_cursor_pos.Y};
			_dirty_tracker.MarkAreas({old_area});
		}
		if (cursor_visible && in_bounds(cursor_pos)) {
			SMALL_RECT new_area{cursor_pos.X, cursor_pos.Y, cursor_pos.X, cursor_pos.Y};
			_dirty_tracker.MarkAreas({new_area});
		}
		if (g_sdl_debug_redraw) {
			SDLDebugLog(
				"SDLConsoleBackend: cursor move old=(%d,%d vis=%d) new=(%d,%d vis=%d)",
				_last_cursor_pos.X, _last_cursor_pos.Y, _last_cursor_visible ? 1 : 0,
				cursor_pos.X, cursor_pos.Y, cursor_visible ? 1 : 0);
		}
		_last_cursor_pos = cursor_pos;
		_last_cursor_visible = cursor_visible;
	}
	const bool has_pending_frame = _renderer.HasPendingFrame() || _present_controller.FramePending();
	if (!_dirty_tracker.HasDirty() && !cursor_due && !has_pending_frame && _present_controller.PendingWaiters() == 0) {
		_pending_present = false;
		_tick_rendered_count += 0;
		_tick_present_count += 0;
		_tick_count += 1;
		if (now_ms - _last_stats_log_ms >= 1000) {
			const DirtyTracker::FrameStats &stats = _dirty_tracker.LastStats();
			fprintf(stderr,
				"SDLConsoleBackend: stats tick=%llu rendered=%llu present=%llu dirty_rects=%zu dirty_cells=%zu full=%d pending_regions=%zu\n",
				static_cast<unsigned long long>(_tick_count),
				static_cast<unsigned long long>(_tick_rendered_count),
				static_cast<unsigned long long>(_tick_present_count),
				stats.dirty_rects,
				stats.dirty_cells,
				stats.full_redraw ? 1 : 0,
				stats.pending_regions);
			_last_stats_log_ms = now_ms;
			_tick_rendered_count = 0;
			_tick_present_count = 0;
			_tick_count = 0;
		}
		return;
	}
	if (now - _last_present_time < kFrameInterval && _present_controller.PendingWaiters() == 0) {
		return;
	}
	DirtyTracker::FrameStats dirty_stats{};
	bool rendered = _renderer_worker.Render(_console_model, _dirty_tracker, dirty_stats);
	if (rendered) {
		_present_controller.NotifyFrameDrawn();
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: rendered frame (dirty=%d)", _dirty_tracker.HasDirty() ? 1 : 0);
		}
	} else if (cursor_due && !_present_controller.FramePending()) {
		_dirty_tracker.RequestScan();
		rendered = _renderer_worker.Render(_console_model, _dirty_tracker, dirty_stats);
		if (rendered) {
			_present_controller.NotifyFrameDrawn();
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleBackend: cursor blink scan rendered");
			}
		} else {
			_present_controller.NotifyFrameDrawn();
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleBackend: cursor blink frame pending (no scan changes)");
			}
		}
	} else if (!_dirty_tracker.HasDirty() && !_present_controller.FramePending() && !_renderer.HasPendingFrame() &&
			   _present_controller.PendingWaiters() > 0) {
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: completing idle flush waiters");
		}
		_present_controller.NotifyPresented();
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog(
			"SDLConsoleBackend: tick render=%d dirty_pending=%d frame_pending=%d renderer_pending=%d waiters=%zu",
			rendered ? 1 : 0,
			_dirty_tracker.HasDirty() ? 1 : 0,
			_present_controller.FramePending() ? 1 : 0,
			_renderer.HasPendingFrame() ? 1 : 0,
			_present_controller.PendingWaiters());
	}

	if (!rendered && !cursor_due && !_renderer.HasPendingFrame()) {
		_pending_present = false;
		_tick_rendered_count += rendered ? 1 : 0;
		_tick_present_count += 0;
		_tick_count += 1;
		if (now_ms - _last_stats_log_ms >= 1000) {
			const DirtyTracker::FrameStats &stats = _dirty_tracker.LastStats();
			fprintf(stderr,
				"SDLConsoleBackend: stats tick=%llu rendered=%llu present=%llu dirty_rects=%zu dirty_cells=%zu full=%d pending_regions=%zu\n",
				static_cast<unsigned long long>(_tick_count),
				static_cast<unsigned long long>(_tick_rendered_count),
				static_cast<unsigned long long>(_tick_present_count),
				stats.dirty_rects,
				stats.dirty_cells,
				stats.full_redraw ? 1 : 0,
				stats.pending_regions);
			_last_stats_log_ms = now_ms;
			_tick_rendered_count = 0;
			_tick_present_count = 0;
			_tick_count = 0;
		}
		return;
	}

	const bool wants_present = _present_controller.ShouldPresent();
	if (g_sdl_debug_redraw) {
		SDLDebugLog(
			"SDLConsoleBackend: tick wants_present=%d pending_present=%d cursor_due=%d",
			wants_present ? 1 : 0,
			_pending_present ? 1 : 0,
			cursor_due ? 1 : 0);
	}
	if (wants_present) {
		const auto now = std::chrono::steady_clock::now();
		if (now - _last_present_time >= kFrameInterval || _present_controller.PendingWaiters() > 0) {
			_renderer.Present();
			_present_controller.NotifyPresented();
			_last_present_time = now;
			_pending_present = false;
			_tick_present_count += 1;
			const DirtyTracker::FrameStats &stats = rendered ? dirty_stats : _dirty_tracker.LastStats();
			const auto glyph_stats = _renderer.PopGlyphStats();
			if (SDL_TRACE_ENABLED()) {
				SDL_TRACE_EVENT(
					"renderer",
					"frame_stats",
					"rects=%zu cells=%zu glyph_hit=%zu glyph_miss=%zu full=%d pending=%zu",
					stats.dirty_rects,
					stats.dirty_cells,
					glyph_stats.hits,
					glyph_stats.misses,
					stats.full_redraw ? 1 : 0,
					stats.pending_regions);
			}
		} else {
			_pending_present = true;
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleBackend: present deferred (vsync gate)");
			}
		}
	} else {
		_pending_present = false;
	}

	_tick_rendered_count += rendered ? 1 : 0;
	_tick_count += 1;
	if (now_ms - _last_stats_log_ms >= 1000) {
		const DirtyTracker::FrameStats &stats = rendered ? dirty_stats : _dirty_tracker.LastStats();
		fprintf(stderr,
			"SDLConsoleBackend: stats tick=%llu rendered=%llu present=%llu dirty_rects=%zu dirty_cells=%zu full=%d pending_regions=%zu\n",
			static_cast<unsigned long long>(_tick_count),
			static_cast<unsigned long long>(_tick_rendered_count),
			static_cast<unsigned long long>(_tick_present_count),
			stats.dirty_rects,
			stats.dirty_cells,
			stats.full_redraw ? 1 : 0,
			stats.pending_regions);
		_last_stats_log_ms = now_ms;
		_tick_rendered_count = 0;
		_tick_present_count = 0;
		_tick_count = 0;
	}

	// When a present is deferred by the vsync gate we simply wait for the next
	// periodic tick (timer-driven). Forcing an immediate tick here spins the
	// event loop too quickly and we never sleep long enough to satisfy
	// kFrameIntervalMs, which previously caused permanent "present deferred"
	// logs and frozen frames.
}

void SDLConsoleBackend::OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count)
{
	if (!areas || !count) {
		return;
	}
	if (SDL_TRACE_ENABLED()) {
		SDL_TRACE_EVENT("console", "output_updated", "count=%zu", count);
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnConsoleOutputUpdated count=%zu", count);
	}
	BackendCommand cmd;
	cmd.type = BackendCommandType::Refresh;
	cmd.areas.assign(areas, areas + count);
	PushCommand(std::move(cmd));
}

void SDLConsoleBackend::OnConsoleOutputResized()
{
	BackendCommand cmd;
	cmd.type = BackendCommandType::ConsoleResized;
	if (SDL_TRACE_ENABLED()) {
		SDL_TRACE_EVENT("console", "resized", "cellW=%d cellH=%d", _renderer.CellWidth(), _renderer.CellHeight());
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnConsoleOutputResized");
	}
	PushCommand(std::move(cmd));
}

void SDLConsoleBackend::OnConsoleOutputTitleChanged()
{
	BackendCommand cmd;
	cmd.type = BackendCommandType::TitleChanged;
	if (SDL_TRACE_ENABLED()) {
		SDL_TRACE_EVENT("console", "title_changed");
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnConsoleOutputTitleChanged");
	}
	PushCommand(std::move(cmd));
}

void SDLConsoleBackend::OnConsoleOutputWindowMoved(bool absolute, COORD pos)
{
	if (!absolute) {
		return;
	}
	if (SDL_TRACE_ENABLED()) {
		SDL_TRACE_EVENT("console", "window_moved", "x=%d y=%d", pos.X, pos.Y);
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnConsoleOutputWindowMoved(%d,%d)", pos.X, pos.Y);
	}
	BackendCommand cmd;
	cmd.type = BackendCommandType::WindowMoved;
	cmd.coord_value = pos;
	PushCommand(std::move(cmd));
}

void SDLConsoleBackend::OnConsoleAdhocQuickEdit()
{
	if (_adhoc_quickedit) {
		return;
	}
	if ((_mouse_button_state & (FROM_LEFT_2ND_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED)) != 0) {
		return;
	}
	_adhoc_quickedit = true;
	if ((_mouse_button_state & FROM_LEFT_1ST_BUTTON_PRESSED) != 0) {
		_mouse_button_state &= ~FROM_LEFT_1ST_BUTTON_PRESSED;
		EmitMouseEvent(_mouse_button_state, 0, _last_mouse_pos.X, _last_mouse_pos.Y);
		StartMouseQEdit(_last_mouse_pos);
	}
}

void SDLConsoleBackend::OnConsoleSetMaximized(bool maximized)
{
	BackendCommand cmd;
	cmd.type = BackendCommandType::SetMaximized;
	cmd.bool_value = maximized;
	if (SDL_TRACE_ENABLED()) {
		SDL_TRACE_EVENT("console", "set_maximized", "value=%d", maximized ? 1 : 0);
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnConsoleSetMaximized(%d)", maximized ? 1 : 0);
	}
	PushCommand(std::move(cmd));
}

COORD SDLConsoleBackend::OnConsoleGetLargestWindowSize()
{
	COORD out{120, 50};
	SDL_Rect rect{};
	if (SDL_GetDisplayUsableBounds(0, &rect) == 0 && _renderer.CellWidth() > 0 && _renderer.CellHeight() > 0) {
		out.X = rect.w / _renderer.CellWidth();
		out.Y = rect.h / _renderer.CellHeight();
	}
	return out;
}

void SDLConsoleBackend::OnConsoleExit()
{
	StopRunning();
}

void SDLConsoleBackend::OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text)
{
	BackendCommand cmd;
	cmd.type = BackendCommandType::DisplayNotification;
	if (title) cmd.wide_a = title;
	if (text) cmd.wide_b = text;
	if (SDL_TRACE_ENABLED()) {
		SDL_TRACE_EVENT("console", "display_notification");
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnConsoleDisplayNotification");
	}
	PushCommand(std::move(cmd));
}

void SDLConsoleBackend::OnConsoleSaveWindowState()
{
	if (!_window) {
		return;
	}
	WinState ws;
	ws.valid = true;
	const Uint32 flags = SDL_GetWindowFlags(_window);
	ws.maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
	ws.fullscreen = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0 || (flags & SDL_WINDOW_FULLSCREEN) != 0;

	if (!ws.maximized && !ws.fullscreen) {
		SDL_GetWindowPosition(_window, &ws.pos_x, &ws.pos_y);
		SDL_GetWindowSize(_window, &ws.width, &ws.height);
	} else if (ws.width <= 0 || ws.height <= 0) {
		SDL_GetWindowSize(_window, &ws.width, &ws.height);
		if (ws.pos_x == SDL_WINDOWPOS_UNDEFINED || ws.pos_y == SDL_WINDOWPOS_UNDEFINED) {
			SDL_GetWindowPosition(_window, &ws.pos_x, &ws.pos_y);
		}
	}
	ws.Save();
}

void SDLConsoleBackend::OnConsoleOutputFlushDrawing()
{
	std::shared_future<bool> future;
	{
		std::lock_guard<std::mutex> lock(_flush_mutex);
		if (_flush_future.valid() && _flush_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
			_flush_future = std::shared_future<bool>{};
		}
		if (_flush_future.valid()) {
			future = _flush_future;
		} else {
			BackendCommand cmd;
			cmd.type = BackendCommandType::Flush;
			if (SDL_TRACE_ENABLED()) {
				SDL_TRACE_EVENT("console", "flush_requested");
			}
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleBackend: OnConsoleOutputFlushDrawing");
			}
			auto promise = std::make_shared<std::promise<bool>>();
			cmd.completion = promise;
			future = promise->get_future().share();
			_flush_future = future;
			PushCommand(std::move(cmd));
		}
	}
	future.wait();
}

void SDLConsoleBackend::OnGetConsoleImageCaps(WinportGraphicsInfo *wgi)
{
	if (!wgi) {
		return;
	}
	_renderer.DescribeImageCaps(*wgi);
}

bool SDLConsoleBackend::OnSetConsoleImage(const char *id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height, const void *buffer)
{
	if (!id) {
		return false;
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnSetConsoleImage(%s)", id);
	}

	BackendCommand cmd;
	cmd.type = BackendCommandType::SetImage;
	cmd.str_value = id;
	cmd.flags_value = flags;
	cmd.width_value = width;
	cmd.height_value = height;
	if (area) {
		cmd.rect_value = *area;
		cmd.has_rect = true;
	}

	const size_t payload = ImagePayloadSize(flags, width, height);
	if (payload) {
		if (!buffer) {
			return false;
		}
		const uint8_t *bytes = static_cast<const uint8_t *>(buffer);
		cmd.binary.assign(bytes, bytes + payload);
	}

	auto promise = std::make_shared<std::promise<bool>>();
	cmd.completion = promise;
	auto future = promise->get_future();
	PushCommand(std::move(cmd));
	return future.get();
}

bool SDLConsoleBackend::OnTransformConsoleImage(const char *id, const SMALL_RECT *area, uint16_t tf)
{
	if (!id) {
		return false;
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnTransformConsoleImage(%s)", id);
	}
	BackendCommand cmd;
	cmd.type = BackendCommandType::TransformImage;
	cmd.str_value = id;
	cmd.tf_value = tf;
	if (area) {
		cmd.rect_value = *area;
		cmd.has_rect = true;
	}

	auto promise = std::make_shared<std::promise<bool>>();
	cmd.completion = promise;
	auto future = promise->get_future();
	PushCommand(std::move(cmd));
	return future.get();
}

bool SDLConsoleBackend::OnDeleteConsoleImage(const char *id)
{
	if (!id) {
		return false;
	}
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: OnDeleteConsoleImage(%s)", id);
	}
	BackendCommand cmd;
	cmd.type = BackendCommandType::DeleteImage;
	cmd.str_value = id;

	auto promise = std::make_shared<std::promise<bool>>();
	cmd.completion = promise;
	auto future = promise->get_future();
	PushCommand(std::move(cmd));
	return future.get();
}

const char *SDLConsoleBackend::OnConsoleBackendInfo(int entity)
{
	if (entity == -1) {
		return "GUI|SDL";
	}
	/*if (entity < 0) {
		_info_buffer.clear();
		for (size_t i = 0; i < _backend_info.size(); ++i) {
			if (i) _info_buffer.append(", ");
			_info_buffer.append(_backend_info[i]);
		}
		return _info_buffer.c_str();
	}*/
	if (entity >= 0 && static_cast<size_t>(entity) < _backend_info.size()) {
		return _backend_info[entity].c_str();
	}
	return nullptr;
}

void SDLConsoleBackend::PushCommand(BackendCommand &&cmd)
{
	if (g_sdl_debug_redraw) {
		switch (cmd.type) {
		case BackendCommandType::Refresh:
			SDLDebugLog("SDLConsoleBackend: queue %s (areas=%zu)", BackendCommandTypeToString(cmd.type), cmd.areas.size());
			break;
		case BackendCommandType::SetImage:
			SDLDebugLog("SDLConsoleBackend: queue %s (id=%s, flags=0x%llx)", BackendCommandTypeToString(cmd.type), cmd.str_value.c_str(), static_cast<unsigned long long>(cmd.flags_value));
			break;
		case BackendCommandType::TransformImage:
		case BackendCommandType::DeleteImage:
			SDLDebugLog("SDLConsoleBackend: queue %s (id=%s)", BackendCommandTypeToString(cmd.type), cmd.str_value.c_str());
			break;
		default:
			SDLDebugLog("SDLConsoleBackend: queue %s", BackendCommandTypeToString(cmd.type));
			break;
		}
	}
	bool notify = false;
	{
		std::lock_guard<std::mutex> lock(_commands_mutex);
		const size_t next_head = (_command_head + 1) & kCommandQueueMask;
		if (next_head == _command_tail) {
			if (SDL_TRACE_ENABLED()) {
				SDL_TRACE_EVENT("backend", "queue_overflow", "type=%s", BackendCommandTypeToString(cmd.type));
			}
			_command_tail = (_command_tail + 1) & kCommandQueueMask;
		}
		_command_ring[_command_head] = std::move(cmd);
		_command_head = next_head;
		if (!_tick_requested) {
			_tick_requested = true;
			notify = true;
		}
	}
	if (notify) {
		NotifyMainThread();
	}
}

void SDLConsoleBackend::NotifyMainThread()
{
	SDL_Event ev{};
	ev.type = _event_id;
	SDL_PushEvent(&ev);
}

void SDLConsoleBackend::RequestProcessTick()
{
	bool notify = false;
	{
		std::lock_guard<std::mutex> lock(_commands_mutex);
		if (!_tick_requested) {
			_tick_requested = true;
			notify = true;
		}
	}
	if (notify) {
		NotifyMainThread();
	}
}

void SDLConsoleBackend::ExtendDirtyCoalesce()
{
	const auto now = std::chrono::steady_clock::now();
	const auto until = now + kDirtyCoalesceWindow;
	if (_dirty_coalesce_until < until) {
		_dirty_coalesce_until = until;
	}
}

void SDLConsoleBackend::ProcessCommand(const BackendCommand& cmd)
{
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: process %s", BackendCommandTypeToString(cmd.type));
	}
	if (SDL_TRACE_ENABLED()) {
		SDL_TRACE_EVENT("backend", "process_command", "type=%s", BackendCommandTypeToString(cmd.type));
	}
	switch (cmd.type) {

	case BackendCommandType::Refresh:
		_dirty_tracker.MarkAreas(cmd.areas);
		ExtendDirtyCoalesce();
		break;

	case BackendCommandType::FullRefresh:
		_dirty_tracker.RequestFullRedraw();
		ExtendDirtyCoalesce();
		break;

	case BackendCommandType::ConsoleResized:
		_renderer.SyncConsoleSize(false);
		_dirty_tracker.Resize(_console_model.Output(), _renderer.Columns(), _renderer.Rows());
		ExtendDirtyCoalesce();
		break;

	case BackendCommandType::Flush:
	{
		bool frame_pending = _dirty_tracker.HasDirty() || _present_controller.FramePending() || _renderer.HasPendingFrame();
		const auto now = std::chrono::steady_clock::now();
		if (!frame_pending) {
			_dirty_tracker.RequestScan();
			if (cmd.completion) {
				_present_controller.RegisterFlushWaiter(cmd.completion, true);
			}
				RequestProcessTick();
				if (g_sdl_debug_redraw) {
					SDLDebugLog("SDLConsoleBackend: Flush scan scheduled (dirty=0 frame=0 renderer=0, waiter=%d)",
						cmd.completion ? 1 : 0);
			}
			break;
		}
		if (now < _dirty_coalesce_until) {
			if (cmd.completion) {
				_deferred_flush_waiters.push_back(cmd.completion);
			}
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleBackend: Flush deferred for coalesce (waiters=%zu)", _deferred_flush_waiters.size());
			}
			RequestProcessTick();
			break;
		}
		if (g_sdl_debug_redraw) {
			SDLDebugLog(
				"SDLConsoleBackend: Flush wait (dirty=%d frame_pending=%d renderer_pending=%d waiters=%zu)",
				_dirty_tracker.HasDirty() ? 1 : 0,
				_present_controller.FramePending() ? 1 : 0,
				_renderer.HasPendingFrame() ? 1 : 0,
				_present_controller.PendingWaiters());
		}
		_present_controller.RegisterFlushWaiter(cmd.completion, frame_pending);
		RequestProcessTick();
		break;
	}

	case BackendCommandType::TitleChanged:
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: TitleChanged");
		}
		UpdateTitle();
		break;

	case BackendCommandType::SetMaximized:
		if (_window) {
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleBackend: SetMaximized(%d)", cmd.bool_value ? 1 : 0);
			}
			if (cmd.bool_value)
				SDL_MaximizeWindow(_window);
			else
				SDL_RestoreWindow(_window);
		}
		break;

	case BackendCommandType::WindowMoved:
		if (_window) {
			if (g_sdl_debug_redraw) {
				SDLDebugLog("SDLConsoleBackend: WindowMoved(%d,%d)", cmd.coord_value.X, cmd.coord_value.Y);
			}
			SDL_SetWindowPosition(
				_window,
				cmd.coord_value.X,
				cmd.coord_value.Y);
		}
		break;

	case BackendCommandType::DisplayNotification:
	{
		const std::string title = StrWide2MB(cmd.wide_a);
		const std::string text  = StrWide2MB(cmd.wide_b);
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: DisplayNotification title='%s'", title.c_str());
		}

		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_INFORMATION,
			title.empty() ? "far2l" : title.c_str(),
			text.c_str(),
			_window);
		break;
	}
	case BackendCommandType::ChangeFont:
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: ChangeFont command");
		}
		ChangeFontInteractive();
		_dirty_tracker.RequestFullRedraw();
		ExtendDirtyCoalesce();
		break;

	case BackendCommandType::SetImage:
	{
		const SMALL_RECT *rect = cmd.has_rect ? &cmd.rect_value : nullptr;
		const uint8_t *data = cmd.binary.empty() ? nullptr : cmd.binary.data();
		const size_t data_size = cmd.binary.size();
		const bool ok = _renderer.SetImage(cmd.str_value, cmd.flags_value, rect, cmd.width_value, cmd.height_value, data, data_size);
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: SetImage(%s) -> %s", cmd.str_value.c_str(), ok ? "ok" : "fail");
		}
		if (cmd.completion) {
			cmd.completion->set_value(ok);
		}
		if (ok) {
			_present_controller.NotifyFrameDrawn();
		}
		ExtendDirtyCoalesce();
		break;
	}

	case BackendCommandType::TransformImage:
	{
		const SMALL_RECT *rect = cmd.has_rect ? &cmd.rect_value : nullptr;
		const bool ok = _renderer.TransformImage(cmd.str_value, rect, cmd.tf_value);
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: TransformImage(%s) -> %s", cmd.str_value.c_str(), ok ? "ok" : "fail");
		}
		if (cmd.completion) {
			cmd.completion->set_value(ok);
		}
		if (ok) {
			_present_controller.NotifyFrameDrawn();
		}
		ExtendDirtyCoalesce();
		break;
	}

	case BackendCommandType::DeleteImage:
	{
		const bool ok = _renderer.DeleteImage(cmd.str_value);
		if (g_sdl_debug_redraw) {
			SDLDebugLog("SDLConsoleBackend: DeleteImage(%s) -> %s", cmd.str_value.c_str(), ok ? "ok" : "fail");
		}
		if (cmd.completion) {
			cmd.completion->set_value(ok);
		}
		if (ok) {
			_present_controller.NotifyFrameDrawn();
		}
		ExtendDirtyCoalesce();
		break;
	}

	default:
		break;
	}

}

void SDLConsoleBackend::UpdateTitle()
{
	if (!g_winport_con_out || !_window) {
		return;
	}
	const std::wstring title = g_winport_con_out->GetTitle();
	const std::string title_mb = StrWide2MB(title);
	SDL_SetWindowTitle(_window, title_mb.c_str());
}

WORD SDLConsoleBackend::SDLKeycodeToVK(SDL_Keycode key)
{
	if (key >= SDLK_a && key <= SDLK_z) {
		return static_cast<WORD>(std::toupper(static_cast<int>(key)));
	}
	if (key >= SDLK_0 && key <= SDLK_9) {
		return static_cast<WORD>(key);
	}
	switch (key) {
	case SDLK_RETURN: return VK_RETURN;
	case SDLK_ESCAPE: return VK_ESCAPE;
	case SDLK_TAB: return VK_TAB;
	case SDLK_BACKSPACE: return VK_BACK;
	case SDLK_SPACE: return VK_SPACE;
	case SDLK_UP: return VK_UP;
	case SDLK_DOWN: return VK_DOWN;
	case SDLK_LEFT: return VK_LEFT;
	case SDLK_RIGHT: return VK_RIGHT;
	case SDLK_HOME: return VK_HOME;
	case SDLK_END: return VK_END;
	case SDLK_PAGEUP: return VK_PRIOR;
	case SDLK_PAGEDOWN: return VK_NEXT;
	case SDLK_INSERT: return VK_INSERT;
	case SDLK_DELETE: return VK_DELETE;
	case SDLK_PERIOD: return VK_OEM_PERIOD;
	case SDLK_COMMA: return VK_OEM_COMMA;
	case SDLK_MINUS: return VK_OEM_MINUS;
	case SDLK_EQUALS: return VK_OEM_PLUS;
	case SDLK_SEMICOLON: return VK_OEM_1;
	case SDLK_SLASH: return VK_OEM_2;
	case SDLK_BACKQUOTE: return VK_OEM_3;
	case SDLK_LEFTBRACKET: return VK_OEM_4;
	case SDLK_BACKSLASH: return VK_OEM_5;
	case SDLK_RIGHTBRACKET: return VK_OEM_6;
	case SDLK_QUOTE: return VK_OEM_7;
	case SDLK_F1: return VK_F1;
	case SDLK_F2: return VK_F2;
	case SDLK_F3: return VK_F3;
	case SDLK_F4: return VK_F4;
	case SDLK_F5: return VK_F5;
	case SDLK_F6: return VK_F6;
	case SDLK_F7: return VK_F7;
	case SDLK_F8: return VK_F8;
	case SDLK_F9: return VK_F9;
	case SDLK_F10: return VK_F10;
	case SDLK_F11: return VK_F11;
	case SDLK_F12: return VK_F12;
	default:
		return 0;
	}
}

DWORD SDLConsoleBackend::ModifiersToControlState(Uint16 mod)
{
	DWORD state = 0;
	if (mod & KMOD_SHIFT) state |= SHIFT_PRESSED;
	if (mod & KMOD_CTRL) state |= LEFT_CTRL_PRESSED;
	if (mod & KMOD_ALT) state |= LEFT_ALT_PRESSED;
	if (mod & KMOD_CAPS) state |= CAPSLOCK_ON;
	if (mod & KMOD_LGUI) state |= LEFT_CTRL_PRESSED;
	if (mod & KMOD_RGUI) state |= RIGHT_CTRL_PRESSED;
	if (mod & KMOD_NUM) state |= NUMLOCK_ON;
#ifdef __APPLE__
	state |= g_sdl_keyboard_leds_state.load();
#endif
	return state;
}

int SDLConsoleBackend::IsEnhancedKey(SDL_Keycode key, SDL_Scancode scancode)
{
	(void)scancode;
	switch (key) {
	case SDLK_LEFT:
	case SDLK_RIGHT:
	case SDLK_UP:
	case SDLK_DOWN:
	case SDLK_HOME:
	case SDLK_END:
	case SDLK_PAGEDOWN:
	case SDLK_PAGEUP:
	case SDLK_INSERT:
	case SDLK_DELETE:
	case SDLK_PRINTSCREEN:
	case SDLK_KP_ENTER:
	case SDLK_KP_DIVIDE:
	case SDLK_NUMLOCKCLEAR:
	case SDLK_LGUI:
	case SDLK_RGUI:
	case SDLK_APPLICATION:
	case SDLK_RCTRL:
		return true;
	default:
		return false;
	}
}

void SDLConsoleBackend::HandleKeyEvent(const SDL_KeyboardEvent &key)
{
	if (!g_winport_con_in) {
		return;
	}
	if (key.type == SDL_KEYDOWN) {
		_renderer.SuppressCursorBlinkFor(std::chrono::milliseconds(kCursorBlinkSuppressMs));
	}
	if (SDL_IsTextInputActive()
		&& (key.keysym.mod & (KMOD_CTRL | KMOD_ALT)) == 0
		&& key.keysym.sym == SDLK_SPACE) {
		return;
	}
#ifdef __APPLE__
	if (key.type == SDL_KEYDOWN && key.keysym.sym == SDLK_CLEAR) {
		DWORD cur = g_sdl_keyboard_leds_state.load();
		g_sdl_keyboard_leds_state.store(cur ^ NUMLOCK_ON);
	}
#endif
	INPUT_RECORD ir{};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = (key.type == SDL_KEYDOWN);
	ir.Event.KeyEvent.wRepeatCount = key.repeat ? key.repeat : 1;
	ir.Event.KeyEvent.wVirtualKeyCode = SDLKeycodeToVK(key.keysym.sym);
	ir.Event.KeyEvent.wVirtualScanCode = key.keysym.scancode;
	ir.Event.KeyEvent.uChar.UnicodeChar = 0;
	ir.Event.KeyEvent.dwControlKeyState = ModifiersToControlState(key.keysym.mod);
	if (IsEnhancedKey(key.keysym.sym, key.keysym.scancode)) {
		ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;
	}
	g_winport_con_in->Enqueue(&ir, 1);
}

SHORT SDLConsoleBackend::ClampColumn(int pixel_x) const
{
	const int cell_w = std::max(1, _renderer.CellWidth());
	const int cols = std::max(1u, _renderer.Columns());
	int win_w = 0;
	int win_h = 0;
	if (_window) {
		SDL_GetWindowSize(_window, &win_w, &win_h);
	}
	int origin_x = 0;
	int origin_y = 0;
	int console_px_w = 0;
	int console_px_h = 0;
	int out_w = 0;
	int out_h = 0;
	_renderer.GetLayoutInfo(origin_x, origin_y, console_px_w, console_px_h, out_w, out_h);
	const float scale_x = (win_w > 0 && out_w > 0)
		? static_cast<float>(out_w) / static_cast<float>(win_w)
		: 1.f;
	const int pixel_scaled = static_cast<int>(std::lround(pixel_x * scale_x));
	const int pixel_local = std::max(0, pixel_scaled - origin_x);
	const int col = std::clamp(pixel_local / cell_w, 0, cols - 1);
	return static_cast<SHORT>(col);
}

SHORT SDLConsoleBackend::ClampRow(int pixel_y) const
{
	const int cell_h = std::max(1, _renderer.CellHeight());
	const int rows = std::max(1u, _renderer.Rows());
	int win_w = 0;
	int win_h = 0;
	if (_window) {
		SDL_GetWindowSize(_window, &win_w, &win_h);
	}
	int origin_x = 0;
	int origin_y = 0;
	int console_px_w = 0;
	int console_px_h = 0;
	int out_w = 0;
	int out_h = 0;
	_renderer.GetLayoutInfo(origin_x, origin_y, console_px_w, console_px_h, out_w, out_h);
	const float scale_y = (win_h > 0 && out_h > 0)
		? static_cast<float>(out_h) / static_cast<float>(win_h)
		: 1.f;
	const int pixel_scaled = static_cast<int>(std::lround(pixel_y * scale_y));
	const int pixel_local = std::max(0, pixel_scaled - origin_y);
	const int row = std::clamp(pixel_local / cell_h, 0, rows - 1);
	return static_cast<SHORT>(row);
}

void SDLConsoleBackend::HandleMouseMotion(const SDL_MouseMotionEvent &motion)
{
	if (!g_winport_con_in) {
		return;
	}
	const SHORT col = ClampColumn(motion.x);
	const SHORT row = ClampRow(motion.y);
	_last_mouse_pos = {col, row};
	if (_mouse_qedit_start_ticks != 0) {
		UpdateMouseQEdit(_last_mouse_pos, true, false);
		return;
	}
	EmitMouseEvent(_mouse_button_state, MOUSE_MOVED, col, row);
}

void SDLConsoleBackend::HandleMouseButton(const SDL_MouseButtonEvent &button)
{
	if (!g_winport_con_in) {
		return;
	}
	DWORD mask = 0;
	switch (button.button) {
	case SDL_BUTTON_LEFT: mask = FROM_LEFT_1ST_BUTTON_PRESSED; break;
	case SDL_BUTTON_MIDDLE: mask = FROM_LEFT_2ND_BUTTON_PRESSED; break;
	case SDL_BUTTON_RIGHT: mask = RIGHTMOST_BUTTON_PRESSED; break;
	default: break;
	}
	DWORD event_flags = 0;
	if (button.type == SDL_MOUSEBUTTONDOWN) {
		_mouse_button_state |= mask;
		if (button.clicks >= 2 && mask != 0) {
			event_flags |= DOUBLE_CLICK;
		}
	} else {
		_mouse_button_state &= ~mask;
	}
	const SHORT col = ClampColumn(button.x);
	const SHORT row = ClampRow(button.y);
	_last_mouse_pos = {col, row};
	if (_adhoc_quickedit || _mouse_qedit_start_ticks != 0) {
		if (button.button == SDL_BUTTON_LEFT) {
			if (button.type == SDL_MOUSEBUTTONDOWN) {
				StartMouseQEdit(_last_mouse_pos);
			} else {
				UpdateMouseQEdit(_last_mouse_pos, false, true);
			}
		}
		return;
	}
	EmitMouseEvent(_mouse_button_state, event_flags, col, row);
}

void SDLConsoleBackend::HandleMouseWheel(const SDL_MouseWheelEvent &wheel)
{
	if (!g_winport_con_in) {
		return;
	}
	const SHORT col = _last_mouse_pos.X;
	const SHORT row = _last_mouse_pos.Y;

	auto emit_axis = [&](double raw_step, DWORD flag) {
		if (raw_step == 0.0) {
			return;
		}
		int clicks = static_cast<int>(std::round(raw_step));
		if (clicks == 0) {
			clicks = raw_step > 0.0 ? 1 : -1;
		}
		const SHORT delta = (clicks > 0) ? WHEEL_DELTA : static_cast<SHORT>(-WHEEL_DELTA);
		for (int i = 0; i < std::abs(clicks); ++i) {
			EmitMouseEvent(0, flag, col, row, delta);
		}
	};

	const double step_y = (wheel.preciseY != 0.0f) ? static_cast<double>(wheel.preciseY) : static_cast<double>(wheel.y);
	const double step_x = (wheel.preciseX != 0.0f) ? static_cast<double>(wheel.preciseX) : static_cast<double>(wheel.x);

	emit_axis(step_y, MOUSE_WHEELED);
	emit_axis(step_x, MOUSE_HWHEELED);
}

void SDLConsoleBackend::NormalizeArea(SMALL_RECT &area)
{
	if (area.Left > area.Right) std::swap(area.Left, area.Right);
	if (area.Top > area.Bottom) std::swap(area.Top, area.Bottom);
}

void SDLConsoleBackend::DamageAreaBetween(COORD c1, COORD c2)
{
	SMALL_RECT area{c1.X, c1.Y, c2.X, c2.Y};
	NormalizeArea(area);
	OnConsoleOutputUpdated(&area, 1);
}

void SDLConsoleBackend::CheckForUnfreeze(bool force)
{
	if (_qedit_unfreeze_start_ticks != 0
		&& (force || WINPORT(GetTickCount)() - _qedit_unfreeze_start_ticks >= kQeditUnfreezeDelayMs)) {
		WINPORT(UnfreezeConsoleOutput)();
		_qedit_unfreeze_start_ticks = 0;
	}
}

void SDLConsoleBackend::CheckPutText2Clip()
{
	if (_text2clip.empty()) {
		return;
	}
	if (WinPortClipboard_IsBusy()) {
		return;
	}
	if (!WINPORT(OpenClipboard)(NULL)) {
		return;
	}
	void *mem = ClipboardAllocFromZeroTerminatedString(_text2clip.c_str());
	if (mem) {
		WINPORT(SetClipboardData)(CF_UNICODETEXT, mem);
	}
	WINPORT(CloseClipboard)();
	_text2clip.clear();
}

void SDLConsoleBackend::UpdateQuickEditOverlay(bool force)
{
	if (_mouse_qedit_start_ticks == 0 || !_mouse_qedit_moved) {
		_renderer.ClearQuickEditRect();
		return;
	}
	const DWORD now = WINPORT(GetTickCount)();
	if (!force && (now - _mouse_qedit_start_ticks) <= kQeditCopyMinimalDelayMs) {
		return;
	}
	SMALL_RECT rect{_mouse_qedit_start.X, _mouse_qedit_start.Y, _mouse_qedit_last.X, _mouse_qedit_last.Y};
	NormalizeArea(rect);
	_renderer.SetQuickEditRect(true, rect);
	DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
}

void SDLConsoleBackend::StartMouseQEdit(COORD pos)
{
	if (_mouse_qedit_start_ticks != 0) {
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
	}
	_mouse_qedit_start = pos;
	_mouse_qedit_last = pos;
	_mouse_qedit_start_ticks = WINPORT(GetTickCount)();
	if (!_mouse_qedit_start_ticks) {
		_mouse_qedit_start_ticks = 1;
	}
	_mouse_qedit_moved = false;
	if (_qedit_unfreeze_start_ticks == 0) {
		WINPORT(FreezeConsoleOutput)();
	} else {
		_qedit_unfreeze_start_ticks = 0;
	}
	DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
	UpdateQuickEditOverlay(true);
}

void SDLConsoleBackend::UpdateMouseQEdit(COORD pos, bool moving, bool button_up)
{
	if (_mouse_qedit_start_ticks == 0) {
		return;
	}
	if (moving) {
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
		DamageAreaBetween(_mouse_qedit_start, pos);
		_mouse_qedit_last = pos;
		_mouse_qedit_moved = true;
		UpdateQuickEditOverlay(false);
		return;
	}
	if (!button_up) {
		return;
	}
	const DWORD now = WINPORT(GetTickCount)();
	if (_mouse_qedit_moved && (now - _mouse_qedit_start_ticks) > kQeditCopyMinimalDelayMs) {
		_text2clip.clear();
		USHORT y1 = _mouse_qedit_start.Y;
		USHORT y2 = pos.Y;
		USHORT x1 = _mouse_qedit_start.X;
		USHORT x2 = pos.X;
		if (y1 > y2) std::swap(y1, y2);
		if (x1 > x2) std::swap(x1, x2);

		for (COORD cur{static_cast<SHORT>(x1), static_cast<SHORT>(y1)}; cur.Y <= y2; ++cur.Y) {
			if (!_text2clip.empty()) {
				_text2clip += NATIVE_EOLW;
			}
			for (cur.X = x1; cur.X <= x2; ++cur.X) {
				CHAR_INFO ch;
				if (g_winport_con_out->Read(ch, cur)) {
					if (CI_USING_COMPOSITE_CHAR(ch)) {
						_text2clip += WINPORT(CompositeCharLookup)(ch.Char.UnicodeChar);
					} else if (ch.Char.UnicodeChar) {
						_text2clip += ch.Char.UnicodeChar;
					}
				}
			}
			if (y2 > y1) {
				while (!_text2clip.empty() && _text2clip[_text2clip.size() - 1] == L' ') {
					_text2clip.resize(_text2clip.size() - 1);
				}
			}
		}
		CheckPutText2Clip();
	}

	_adhoc_quickedit = false;
	_mouse_qedit_moved = false;
	_mouse_qedit_start_ticks = 0;
	DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
	DamageAreaBetween(_mouse_qedit_start, pos);
	_renderer.ClearQuickEditRect();
	RequestFullRefresh();
	_qedit_unfreeze_start_ticks = now;
}

void SDLConsoleBackend::HandleTextInput(const SDL_TextInputEvent &text)
{
	_renderer.SuppressCursorBlinkFor(std::chrono::milliseconds(kCursorBlinkSuppressMs));
	const std::wstring wide = MB2Wide(text.text);
	for (wchar_t ch : wide) {
		EmitTextInputChar(ch);
	}
}

void SDLConsoleBackend::RequestFontDialog()
{
	BackendCommand cmd;
	cmd.type = BackendCommandType::ChangeFont;
	PushCommand(std::move(cmd));
}

void SDLConsoleBackend::ChangeFontInteractive()
{
	SDLFontSelection selection;
	const SDLFontDialogStatus status = SDLShowFontPicker(selection);
	if (_window) {
		SDL_ShowWindow(_window);
		SDL_RaiseWindow(_window);
		SDL_SetWindowGrab(_window, SDL_FALSE);
		SDL_SetWindowInputFocus(_window);
	}
	SDL_StartTextInput();
	SDL_PumpEvents();
	switch (status) {
	case SDLFontDialogStatus::Chosen:
		break;
	case SDLFontDialogStatus::Unsupported:
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "far2l", "Font selection dialog is not available on this platform.", _window);
		return;
	case SDLFontDialogStatus::Failed:
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "far2l", "Failed to open font selection dialog.", _window);
		return;
	case SDLFontDialogStatus::Cancelled:
	default:
		return;
	}

	const std::string descriptor = selection.fc_name.empty() ? selection.path : selection.fc_name;
	const int descriptor_face = selection.fc_name.empty() ? selection.face_index : -1;

	if (descriptor.empty()) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "far2l", "Selected font did not provide a path.", _window);
		return;
	}

	const int chosen_size = (selection.point_size > 0.0f)
		? NormalizeFontPointSize(selection.point_size)
		: LoadFontPointSizeFromConfig();

	if (!SaveFontPreference(descriptor, descriptor_face, chosen_size)) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "far2l", "Unable to save selected font.", _window);
		return;
	}

	if (!_renderer.ReloadFontsFromConfig()) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "far2l", "Failed to load the selected font.", _window);
		return;
	}

	const Uint32 win_flags = _window ? SDL_GetWindowFlags(_window) : 0;
	const bool fullscreen =
		(win_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ||
		(win_flags & SDL_WINDOW_FULLSCREEN) ||
		(win_flags & SDL_WINDOW_MAXIMIZED);
	if (fullscreen) {
		int win_w = 0;
		int win_h = 0;
		if (_window) {
			SDL_GetWindowSize(_window, &win_w, &win_h);
		}
		if (win_w > 0 && win_h > 0) {
			HandleHostResize(win_w, win_h);
		}
		_renderer.SyncConsoleSize(false);
	} else {
		_renderer.SyncConsoleSize(true);
	}
	_dirty_tracker.Resize(_console_model.Output(), _renderer.Columns(), _renderer.Rows());
	_renderer.DrawFullConsole();
	_present_controller.NotifyFrameDrawn();
	_renderer.Present();
	_present_controller.NotifyPresented();
	RequestFullRefresh();
	_dirty_tracker.RequestFullRedraw();
	RequestProcessTick();

	// No modal info dialog here; it can steal focus/input.
}

void SDLConsoleBackend::EmitTextInputChar(wchar_t ch)
{
	if (!g_winport_con_in) {
		return;
	}
	INPUT_RECORD ir{};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = TRUE;
	ir.Event.KeyEvent.wRepeatCount = 1;
	ir.Event.KeyEvent.wVirtualKeyCode = VK_PACKET;
	ir.Event.KeyEvent.wVirtualScanCode = 0;
	ir.Event.KeyEvent.uChar.UnicodeChar = ch;
	ir.Event.KeyEvent.dwControlKeyState = ModifiersToControlState(SDL_GetModState());
	g_winport_con_in->Enqueue(&ir, 1);

	ir.Event.KeyEvent.bKeyDown = FALSE;
	ir.Event.KeyEvent.uChar.UnicodeChar = 0;
	g_winport_con_in->Enqueue(&ir, 1);
}

void SDLConsoleBackend::EmitMouseEvent(DWORD button_state, DWORD event_flags, SHORT x, SHORT y, SHORT wheel_delta)
{
	INPUT_RECORD ir{};
	ir.EventType = MOUSE_EVENT;
	ir.Event.MouseEvent.dwMousePosition.X = x;
	ir.Event.MouseEvent.dwMousePosition.Y = y;
	ir.Event.MouseEvent.dwButtonState = button_state;
	if (event_flags == MOUSE_WHEELED || event_flags == MOUSE_HWHEELED) {
		ir.Event.MouseEvent.dwButtonState = (DWORD)((wheel_delta << 16) & 0xffff0000);
	}
	ir.Event.MouseEvent.dwEventFlags = event_flags;
	ir.Event.MouseEvent.dwControlKeyState = ModifiersToControlState(SDL_GetModState());
	g_winport_con_in->Enqueue(&ir, 1);
}

SDLAppWorker::SDLAppWorker(int argc, char **argv, int (*entry)(int, char **))
	: _argc(argc), _argv(argv), _entry_point(entry)
{
}

SDLAppWorker::~SDLAppWorker()
{
	Join();
}

bool SDLAppWorker::Start(IConsoleOutputBackend *backend)
{
	if (!_entry_point || _started.load()) {
		return true;
	}
	_backend = backend;
	try {
		_thread = std::thread(&SDLAppWorker::ThreadMain, this);
		_started.store(true);
		return true;
	} catch (const std::exception &e) {
		fprintf(stderr, "SDLAppWorker::Start failed: %s\n", e.what());
		return false;
	}
}

void SDLAppWorker::Join()
{
	if (_thread.joinable()) {
		_thread.join();
	}
}

void SDLAppWorker::ThreadMain()
{
	int rc = 0;
	if (_entry_point) {
		rc = _entry_point(_argc, _argv);
	}
	_result.store(rc);
	if (_backend) {
		_backend->OnConsoleExit();
	}
}

SDLConsoleApp::SDLConsoleApp(const WinPortMainBackendArg &arg)
	: _arg(arg)
{
}

SDLConsoleApp::~SDLConsoleApp()
{
	Destroy();
}

bool SDLConsoleApp::Initialize()
{
	WinState win_state;
	int init_w = 960;
	int init_h = 540;
	int init_x = SDL_WINDOWPOS_CENTERED;
	int init_y = SDL_WINDOWPOS_CENTERED;
	if (win_state.valid) {
		if (win_state.width > 0 && win_state.height > 0) {
			init_w = win_state.width;
			init_h = win_state.height;
		}
		if (win_state.pos_x != SDL_WINDOWPOS_UNDEFINED && win_state.pos_y != SDL_WINDOWPOS_UNDEFINED) {
			init_x = win_state.pos_x;
			init_y = win_state.pos_y;
		}
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // keep glyphs pixel-crisp
	SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0"); // do not touch KDE plasma

	_window = SDL_CreateWindow(
		"far2l (SDL backend)",
		init_x,
		init_y,
		init_w,
		init_h,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	if (!_window) {
		fprintf(stderr, "SDLConsoleApp::Initialize: failed to create window: %s\n", SDL_GetError());
		return false;
	}

	const std::string config_path = InMyConfig("sdl_font", false);
	if (config_path.empty() || !TestPath(config_path).Regular()) {
		SDL_HideWindow(_window);
		if (!EnsureFontPreferenceSelected()) {
			return false;
		}
		SDL_ShowWindow(_window);
	}

	_backend = std::make_unique<SDLConsoleBackend>(*this);
	if (!_backend->Initialize(_window)) {
		return false;
	}

	_backend->Attach();

	if (win_state.valid) {
		if (win_state.fullscreen) {
			SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		} 
		/* 
		VK: actually we do not need this as we already have created window with last reminded size and position.

		else if (win_state.maximized) {
			SDL_MaximizeWindow(_window);
		}*/
	}

	if (_arg.app_main) {
		_worker = std::make_unique<SDLAppWorker>(_arg.argc, _arg.argv, _arg.app_main);
		if (!_worker->Start(_backend.get())) {
			return false;
		}
	}

	_running.store(true);
	return true;
}

void SDLConsoleApp::Run()
{
	bool quit_requested = false;
	while (_backend && _backend->IsRunning()) {
		SDL_Event event{};
		const int wait_timeout = quit_requested ? 1 : 10;
		if (SDL_WaitEventTimeout(&event, wait_timeout)) {
			if (event.type == SDL_QUIT) {
				quit_requested = true;
				continue;
			}
			_backend->HandleSDLEvent(event);
		}
		if (g_sigint_pending.exchange(false) && _backend) {
			_backend->RequestQuit();
			quit_requested = true;
		}
		// periodic processing happens via SDL timer ticks
	}

	if (_worker) {
		_worker->Join();
		if (_arg.result) {
			*_arg.result = _worker->Result();
		}
	} else if (_arg.result) {
		*_arg.result = 0;
	}
}

void SDLConsoleApp::RequestQuit()
{
	_running.store(false);
}

void SDLConsoleApp::Destroy()
{
	if (_backend) {
		_backend->Detach();
	}
	_backend.reset();

	if (_window) {
		SDL_DestroyWindow(_window);
		_window = nullptr;
	}
}

} // namespace

extern "C" __attribute__((visibility("default"))) bool WinPortMainBackend(WinPortMainBackendArg *a)
{
	if (!a || a->abi_version != FAR2L_BACKEND_ABI_VERSION) {
		fprintf(stderr, "SDL backend: incompatible ABI\n");
		return false;
	}

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		fprintf(stderr, "SDL backend: SDL_Init failed: %s\n", SDL_GetError());
		return false;
	}

	g_sdl_norgb = a->norgb;
	g_winport_con_out = a->winport_con_out;
	g_winport_con_in = a->winport_con_in;

	ClipboardBackendSetter clipboard_backend_setter;
	if (!a->ext_clipboard) {
		clipboard_backend_setter.Set<SDLClipboardBackend>();
	}
	PrinterSupportBackendSetter printer_backend_setter;
	printer_backend_setter.Set<SDLPrinterSupportBackend>();

	bool ok = false;
	SigIntGuard sig_guard;
	{
		SDLConsoleApp app(*a);
		if (app.Initialize()) {
			ok = true;
			app.Run();
		} else {
			fprintf(stderr, "SDL backend: initialization failed\n");
		}
	}

	g_winport_con_out = nullptr;
	g_winport_con_in = nullptr;
	SDL_Quit();
	return ok;
}
void SDLConsoleBackend::RequestFullRefresh()
{
	BackendCommand cmd;
	cmd.type = BackendCommandType::FullRefresh;
	if (g_sdl_debug_redraw) {
		SDLDebugLog("SDLConsoleBackend: RequestFullRefresh");
	}
	PushCommand(std::move(cmd));
}
