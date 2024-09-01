#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <exception>
#include <sys/ioctl.h>

#ifdef __linux__
# include <termios.h>
# include <linux/kd.h>
# include <linux/keyboard.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/ioctl.h>
# include <sys/kbio.h>
#endif

#include <os_call.hpp>
#include <ScopeHelpers.h>
#include <sudo.h>
#include "utils.h"
#include "CheckedCast.hpp"
#include "WinPortHandle.h"
#include "Backend.h"
#include "TTYBackend.h"
#include "TTYRevive.h"
#include "TTYFar2lClipboardBackend.h"
#include "TTYNegotiateFar2l.h"
#include "FarTTY.h"
#include "../FSClipboardBackend.h"

static uint16_t g_far2l_term_width = 80, g_far2l_term_height = 25;
static volatile long s_terminal_size_change_id = 0;
static TTYBackend * g_vtb = nullptr;

long _iterm2_cmd_ts = 0;
bool _iterm2_cmd_state = 0;

static void OnSigHup(int signo);

static bool IsEnhancedKey(WORD code)
{
	return (code==VK_LEFT || code==VK_RIGHT || code==VK_UP || code==VK_DOWN
		|| code==VK_HOME || code==VK_END || code==VK_NEXT || code==VK_PRIOR );
}

static WORD WChar2WinVKeyCode(WCHAR wc)
{
	if ((wc >= L'0' && wc <= L'9') || (wc >= L'A' && wc <= L'Z')) {
		return (WORD)wc;
	}
	if (wc >= L'a' && wc <= L'z') {
		return (WORD)wc - (L'a' - L'A');
	}
	switch (wc) {
		case L' ': return VK_SPACE;
		case L'.': return VK_OEM_PERIOD;
		case L',': return VK_OEM_COMMA;
		case L'_': case L'-': return VK_OEM_MINUS;
		case L'+': return VK_OEM_PLUS;
		case L';': case L':': return VK_OEM_1;
		case L'/': case L'?': return VK_OEM_2;
		case L'~': case L'`': return VK_OEM_3;
		case L'[': case L'{': return VK_OEM_4;
		case L'\\': case L'|': return VK_OEM_5;
		case L']': case '}': return VK_OEM_6;
		case L'\'': case '\"': return VK_OEM_7;
		case L'!': return '1';
		case L'@': return '2';
		case L'#': return '3';
		case L'$': return '4';
		case L'%': return '5';
		case L'^': return '6';
		case L'&': return '7';
		case L'*': return '8';
		case L'(': return '9';
		case L')': return '0';
	}
	fprintf(stderr, "%s: not translated %u '%lc'\n", __FUNCTION__, (unsigned int)wc, wc);
	return VK_UNASSIGNED;
}


TTYBackend::TTYBackend(const char *full_exe_path, int std_in, int std_out, bool ext_clipboard, bool norgb, DWORD nodetect, bool far2l_tty, unsigned int esc_expiration, int notify_pipe, int *result) :
	_full_exe_path(full_exe_path),
	_stdin(std_in),
	_stdout(std_out),
	_ext_clipboard(ext_clipboard),
	_norgb(norgb),
	_nodetect(nodetect),
	_far2l_tty(far2l_tty),
	_esc_expiration(esc_expiration),
	_notify_pipe(notify_pipe),
	_result(result),
	_largest_window_size_ready(false)

{
	if (pipe_cloexec(_kickass) == -1) {
		_kickass[0] = _kickass[1] = -1;
	} else {
		MakeFDNonBlocking(_kickass[1]);
	}

	struct winsize w{};
	GetWinSize(w);
	g_winport_con_out->SetSize(w.ws_col, w.ws_row);
	g_winport_con_out->GetSize(_cur_width, _cur_height);
	g_vtb = this;
}

TTYBackend::~TTYBackend()
{
	if (g_vtb == this)
		g_vtb =  nullptr;

	OnConsoleExit();

	_exiting = 1;

	if (_reader_trd) {
		if (os_call_ssize(write, _kickass[1], (const void*)&_kickass, (size_t)1) == -1) {
			perror("~TTYBackend: write kickass");
		}
		pthread_join(_reader_trd, nullptr);
		_reader_trd = 0;
	}

	CheckedCloseFDPair(_kickass);
	DetachNotifyPipe();
}

void TTYBackend::GetWinSize(struct winsize &w)
{
	int r = ioctl(_stdout, TIOCGWINSZ, &w);
	if (UNLIKELY(r != 0)) {
		r = ioctl(_stdin, TIOCGWINSZ, &w);
		if (UNLIKELY(r != 0)) {
			perror("TIOCGWINSZ");
			w.ws_row = g_far2l_term_height;
			w.ws_col = g_far2l_term_width;
		}
	}
}

void TTYBackend::DetachNotifyPipe()
{
	if (_notify_pipe != -1) {
		MakeFDNonBlocking(_notify_pipe);

		if (_result && write(_notify_pipe, _result,
				sizeof(*_result)) != sizeof(*_result)) {
			perror("DetachNotifyPipe - write");
		}

		CheckedCloseFD(_notify_pipe);
	}
}

bool TTYBackend::Startup()
{
	assert(!_reader_trd);

	_cur_width =_cur_height = 64;
	g_winport_con_out->GetSize(_cur_width, _cur_height);
	g_winport_con_out->SetBackend(this);

	if (pthread_create(&_reader_trd, nullptr, sReaderThread, this) != 0) {
		return false;
	}

	return true;
}

static wchar_t s_backend_identification[8] = L"TTY";

static void AppendBackendIdentificationChar(char ch)
{
	const size_t l = wcslen(s_backend_identification);
	if (l + 1 >= ARRAYSIZE(s_backend_identification)) {
		abort();
	}
	s_backend_identification[l + 1] = 0;
	s_backend_identification[l] = (unsigned char)ch;
}

void TTYBackend::UpdateBackendIdentification()
{
	s_backend_identification[3] = 0;

	if (_far2l_tty || _ttyx || _using_extension) {
		AppendBackendIdentificationChar('|');
	}

	if (_far2l_tty) {
		AppendBackendIdentificationChar('F');

	} else if (_ttyx || _using_extension) {
		if (_ttyx) {
			AppendBackendIdentificationChar('X');
		}
		if (_using_extension) {
			AppendBackendIdentificationChar(_using_extension);
		} else if (_ttyx && _ttyx->HasXi()) {
			AppendBackendIdentificationChar('i');
		}
	}

	g_winport_backend = s_backend_identification;
}

static bool UnderWayland()
{
	const char *xdg_st = getenv("XDG_SESSION_TYPE");
	if (xdg_st && strcasecmp(xdg_st, "wayland") == 0)
		return true;
	if (getenv("WAYLAND_DISPLAY"))
		return true;
	return false;
}

void TTYBackend::ReaderThread()
{
	bool prev_far2l_tty = false;
	while (!_exiting) {
		_far2l_cursor_height = -1; // force cursor height update on next output dispatch
		_fkeys_support = _far2l_tty ? FKS_UNKNOWN : FKS_NOT_SUPPORTED;

		if (_far2l_tty) {
			if (!prev_far2l_tty && !_ext_clipboard) {
				IFar2lInteractor *interactor = this;
				_clipboard_backend_setter.Set<TTYFar2lClipboardBackend>(interactor);
			}

		} else {
			if ((_nodetect & NODETECT_X)==0) {

				// disable xi on Wayland as it not work there anyway and also causes delays
				_ttyx = StartTTYX(_full_exe_path, ((_nodetect & NODETECT_XI)==0) && !UnderWayland());
			}
			if (_ttyx) {
				if (!_ext_clipboard) {
					_clipboard_backend_setter.Set<TTYXClipboard>(_ttyx);
				}

			} else {
				ChooseSimpleClipboardBackend();
			}
		}
		UpdateBackendIdentification();
		prev_far2l_tty = _far2l_tty;

		{
			std::unique_lock<std::mutex> lock(_async_mutex);
			_deadio = false;
			_ae.term_resized = true;
		}


		pthread_t writer_trd = 0;
		if (pthread_create(&writer_trd, nullptr, sWriterThread, this) != 0) {
			break;
		}

		try {
			ReaderLoop();

		} catch (const std::exception &e) {
			fprintf(stderr, "ReaderLoop: %s <%d>\n", e.what(), errno);
		}
		OnUsingExtension(0);

		OnInputBroken();

		{
			std::unique_lock<std::mutex> lock(_async_mutex);
			_deadio = true;
			_async_cond.notify_all();
		}

		pthread_join(writer_trd, nullptr);
		DetachNotifyPipe();
		_ttyx.reset();

		while (!_exiting) {
			const std::string &info = StrWide2MB(g_winport_con_out->GetTitle());
			_notify_pipe = TTYReviveMe(_stdin, _stdout, _far2l_tty, _kickass[0], info);
			if (_notify_pipe != -1) {
				break;
			}
		}
	}
}

void TTYBackend::ReaderLoop()
{
	std::unique_ptr<TTYInput> tty_in(new TTYInput(this));

	fd_set fds, fde;
	bool idle_expired = false;
	while (!_exiting && !_deadio) {
		int maxfd = (_kickass[0] > _stdin) ? _kickass[0] : _stdin;

		FD_ZERO(&fds);
		FD_ZERO(&fde);
		FD_SET(_kickass[0], &fds);
		FD_SET(_stdin, &fds);
		FD_SET(_stdin, &fde);

		int rs;

		if (!idle_expired && _esc_expiration > 0 && !_far2l_tty) {
			struct timeval tv;
			tv.tv_sec = _esc_expiration / 1000;
			tv.tv_usec = suseconds_t((_esc_expiration - tv.tv_sec * 1000) * 1000);

			rs = os_call_int(select, maxfd + 1, &fds, (fd_set*)nullptr, &fde, &tv);
		} else {
			rs = os_call_int(select, maxfd + 1, &fds, (fd_set*)nullptr, &fde, (timeval*)nullptr);
		}

		if (rs == -1) {
			throw std::runtime_error("select failed");
		}

		if (rs != 0) {
			idle_expired = false;

		} else if (!idle_expired) {
			idle_expired = true;
			tty_in->OnIdleExpired();
		}

		if (_flush_input_queue) {
			_flush_input_queue = false;
			tty_in.reset();
			tty_in.reset(new TTYInput(this));
			OnInputBroken();
		}

		if (FD_ISSET(_stdin, &fds)) {
			char buf[0x1000];
			ssize_t rd = os_call_ssize(read, _stdin, (void*)buf, sizeof(buf));
			if (rd <= 0) {
				throw std::runtime_error("stdin read failed");
			}
			//fprintf(stderr, "ReaderThread: CHAR 0x%x\n", (unsigned char)c);
			tty_in->OnInput(buf, (size_t)rd);

			// iTerm2 cmd+v workaround
			if (_iterm2_cmd_state || _iterm2_cmd_ts) {
				std::unique_lock<std::mutex> lock(_async_mutex);
				_ae.output = true;
				_async_cond.notify_all();
			}
		}

		if (FD_ISSET(_stdin, &fde)) {
			throw std::runtime_error("stdin exception");
		}

		if (FD_ISSET(_kickass[0], &fds)) {
			char cmd = 0;
			if (read(_kickass[0], &cmd, 1) <= 0) {
				throw std::runtime_error("kickass read failed");
			}

			long terminal_size_change_id =
				__sync_val_compare_and_swap(&s_terminal_size_change_id, 0, 0);
			if (_terminal_size_change_id != terminal_size_change_id) {
				_terminal_size_change_id = terminal_size_change_id;
				std::unique_lock<std::mutex> lock(_async_mutex);
				_ae.term_resized = true;
				_async_cond.notify_all();
			}
		}
	}
}

void TTYBackend::WriterThread()
{
	bool gone_background = false;
	try {
		TTYOutput tty_out(_stdout, _far2l_tty, _norgb, _nodetect);
		DispatchPalette(tty_out);
//		DispatchTermResized(tty_out);
		while (!_exiting && !_deadio) {
			AsyncEvent ae{};
			do {
				std::unique_lock<std::mutex> lock(_async_mutex);
				if (!_ae.HasAny()) {
					_async_cond.wait(lock);
				}
				if (_ae.HasAny()) {
					std::swap(ae, _ae);
					if (ae.palette) {
						_async_cond.notify_all();
					}
					break;
				}
			} while (!_exiting && !_deadio);

			if (ae.palette) {
				DispatchPalette(tty_out);
			}

			if (ae.term_resized) {
				DispatchTermResized(tty_out);
				ae.output = true;
			}

			if (ae.output)
				DispatchOutput(tty_out);

			if (ae.title_changed) {
				tty_out.ChangeTitle(StrWide2MB(g_winport_con_out->GetTitle()));
			}

			if (ae.far2l_interact)
				DispatchFar2lInteract(tty_out);

			if (ae.osc52clip_set) {
				DispatchOSC52ClipSet(tty_out);
			}

			// iTerm2 cmd+v workaround
			if (_iterm2_cmd_state || _iterm2_cmd_ts) {
				tty_out.CheckiTerm2Hack();
			}

			tty_out.Flush();
			tcdrain(_stdout);

			if (ae.go_background) {
				gone_background = true;
				break;
			}
		}

	} catch (const std::exception &e) {
		fprintf(stderr, "WriterThread: %s <%d>\n", e.what(), errno);
	}
	_deadio = true;

	if (gone_background) {
		OnSigHup(SIGHUP);
	}
}


/////////////////////////////////////////////////////////////////////////

void TTYBackend::DispatchPalette(TTYOutput &tty_out)
{
	TTYBasePalette palette;
	{
		std::lock_guard<std::mutex> lock(_palette_mtx);
		palette = _palette;
		if (_override_default_palette) {
			for (size_t i = 0; i < BASE_PALETTE_SIZE; ++i) {
				if (palette.background[i] == (DWORD)-1) {
					palette.background[i] = g_winport_palette.background[i].AsRGB();
				}
				if (palette.foreground[i] == (DWORD)-1) {
					palette.foreground[i] = g_winport_palette.foreground[i].AsRGB();
				}
			}
		}
	}
	tty_out.ChangePalette(palette);
}

void TTYBackend::DispatchTermResized(TTYOutput &tty_out)
{
	struct winsize w{};
	GetWinSize(w);

	if (_cur_width != w.ws_col || _cur_height != w.ws_row) {
		g_winport_con_out->SetSize(w.ws_col, w.ws_row);
		g_winport_con_out->GetSize(_cur_width, _cur_height);
		INPUT_RECORD ir = {};
		ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
		ir.Event.WindowBufferSizeEvent.dwSize.X = _cur_width;
		ir.Event.WindowBufferSizeEvent.dwSize.Y = _cur_height;
		g_winport_con_in->Enqueue(&ir, 1);
	}
	std::vector<CHAR_INFO> tmp;
	tty_out.MoveCursorStrict(1, 1);
	_prev_height = _prev_width = 0;
	_prev_output.swap(tmp);// ensure memory released
}

//#define LOG_OUTPUT_COUNT
void TTYBackend::DispatchOutput(TTYOutput &tty_out)
{
	_cur_output.resize(size_t(_cur_width) * _cur_height);

	COORD data_size = {CheckedCast<SHORT>(_cur_width), CheckedCast<SHORT>(_cur_height) };
	COORD data_pos = {0, 0};
	SMALL_RECT screen_rect = {0, 0, CheckedCast<SHORT>(_cur_width - 1), CheckedCast<SHORT>(_cur_height - 1)};
	g_winport_con_out->Read(&_cur_output[0], data_size, data_pos, screen_rect);
#ifdef LOG_OUTPUT_COUNT
	unsigned long printed_count = 0, printed_skipable = 0;
#endif
	if (_cur_output.empty()) {
		;

	} else if (_cur_width != _prev_width || _cur_height != _prev_height) {
		for (unsigned int y = 0; y < _cur_height; ++y) {
			const CHAR_INFO *cur_line = &_cur_output[size_t(y) * _cur_width];
			tty_out.MoveCursorLazy(y + 1, 1);
			tty_out.WriteLine(cur_line, _cur_width);
		}

	} else for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &_cur_output[size_t(y) * _cur_width];
		const CHAR_INFO *prev_line = &_prev_output[size_t(y) * _prev_width];

		const auto ApproxWeight = [&](unsigned int x_)
		{
			if (CI_USING_COMPOSITE_CHAR(cur_line[x_])) {
				return 4;
			}
			return ((cur_line[x_].Char.UnicodeChar > 0x7f) ? 2 : 1);
		};

		const auto Modified = [&](unsigned int x_)
		{
			return (cur_line[x_].Char.UnicodeChar != prev_line[x_].Char.UnicodeChar
				|| cur_line[x_].Attributes != prev_line[x_].Attributes);
		};

		for (unsigned int x = 0, skipped_start = 0, skipped_weight = 0; x < _cur_width; ++x) {
			if (!Modified(x)) {
				skipped_weight+= ApproxWeight(x);
				continue;
			}

			// Current char doesn't match to what was on this position before
			// so have to print it at right position.
			// Note that cursor moving directive has its own output 'weight',
			// so if skipped chars sequence is not bigger than cursor move then
			// its better to print skipped chars instead of moving cursor.

			bool print_skipped = false;
			if (x != skipped_start && tty_out.WeightOfHorizontalMoveCursor(y + 1, skipped_start + 1) == 0) { // is cursor at expected pos?
				const int move_cursor_weight = tty_out.WeightOfHorizontalMoveCursor(y + 1, x + 1);
				print_skipped = (move_cursor_weight >= 0 && skipped_weight <= (unsigned int)move_cursor_weight);
			}
			if (print_skipped) {
				tty_out.WriteLine(&cur_line[skipped_start], x + 1 - skipped_start);
#ifdef LOG_OUTPUT_COUNT
				printed_skipable+= x - skipped_start;
#endif
			} else {
				tty_out.MoveCursorLazy(y + 1, x + 1);
				tty_out.WriteLine(&cur_line[x], 1);
			}
#ifdef LOG_OUTPUT_COUNT
			printed_count++;
#endif
			skipped_start = x + 1;
			skipped_weight = 0;
		}
	}
#ifdef LOG_OUTPUT_COUNT
	fprintf(stderr, "!!! OUTPUT_COUNT: (normal=%lu + skipable=%lu) = %lu of %lu\n",
		printed_count, printed_skipable,
		printed_count + printed_skipable,
		(unsigned long)_cur_output.size());
#endif
	_prev_width = _cur_width;
	_prev_height = _cur_height;
	_prev_output.swap(_cur_output);

	UCHAR cursor_height = 1;
	bool cursor_visible = false;
	COORD cursor_pos = g_winport_con_out->GetCursor(cursor_height, cursor_visible);
	tty_out.MoveCursorLazy(cursor_pos.Y + 1, cursor_pos.X + 1);
	tty_out.ChangeCursor(cursor_visible);

	if (_far2l_cursor_height != (int)(unsigned int)cursor_height) {
		_far2l_cursor_height = (int)(unsigned int)cursor_height;
		tty_out.ChangeCursorHeight(cursor_height);
	}
}


void TTYBackend::DispatchFar2lInteract(TTYOutput &tty_out)
{
	Far2lInteractV queued;
	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		queued.swap(_far2l_interacts_queued);
	}

	std::unique_lock<std::mutex> lock_sent(_far2l_interacts_sent);

	for (auto & i : queued) {
		uint8_t id = 0;
		if (i->waited) {
			if (_far2l_interacts_sent.size() >= 0xff) {
				fprintf(stderr,
					"TTYBackend::DispatchFar2lInteract: too many sent interacts - %ld\n",
					_far2l_interacts_sent.size());
				i->stk_ser.Clear();
				i->evnt.Signal();
				return;
			}
			for (;;) {
				id = ++_far2l_interacts_sent._id_counter;
				if (id && _far2l_interacts_sent.find(id) == _far2l_interacts_sent.end()) break;
			}
		}
		i->stk_ser.PushNum(id);

		if (i->waited)
			_far2l_interacts_sent.emplace(id, i);

		tty_out.SendFar2lInteract(i->stk_ser);
	}
}

void TTYBackend::DispatchOSC52ClipSet(TTYOutput &tty_out)
{
	std::string osc52clip;
	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		osc52clip.swap(_osc52clip);
	}
	tty_out.SendOSC52ClipSet(osc52clip);
}

/////////////////////////////////////////////////////////////////////////

void TTYBackend::KickAss(bool flush_input_queue)
{
	if (flush_input_queue)
		_flush_input_queue = true;

	unsigned char c = 0;
	if (os_call_ssize(write, _kickass[1], (const void*)&c, (size_t)1) != 1)
		perror("write(_kickass[1]");
}

void TTYBackend::OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count)
{
	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.output = true;
	_async_cond.notify_all();
}

void TTYBackend::OnConsoleOutputResized()
{
	OnConsoleOutputUpdated(nullptr, 0);
}

void TTYBackend::OnConsoleOutputTitleChanged()
{
	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.title_changed = true;
	_async_cond.notify_all();
}

void TTYBackend::OnConsoleOutputWindowMoved(bool absolute, COORD pos)
{
}

COORD TTYBackend::OnConsoleGetLargestWindowSize()
{
	COORD out = {CheckedCast<SHORT>(_cur_width ? _cur_width : 0x10), CheckedCast<SHORT>(_cur_height ? _cur_height : 0x10)};

	if (_far2l_tty) {
		if (_largest_window_size_ready)
			return _largest_window_size;

		try {
			StackSerializer stk_ser;
			stk_ser.PushNum(FARTTY_INTERACT_GET_WINDOW_MAXSIZE);
			if (Far2lInteract(stk_ser, true)) {
				stk_ser.PopNum(out.Y);
				stk_ser.PopNum(out.X);
				_largest_window_size = out;
				_largest_window_size_ready = true;
			}
		} catch(std::exception &) {; }
	}

	return out;
}

bool TTYBackend::OnConsoleSetFKeyTitles(const char **titles)
{
	if (_fkeys_support == FKS_NOT_SUPPORTED) {
		return false;
	}

	try {
		bool detect_support = (_fkeys_support == FKS_UNKNOWN);
		StackSerializer stk_ser;
		for (int i = CONSOLE_FKEYS_COUNT - 1; i >= 0; --i) {
			unsigned char state = (titles != NULL && titles[i] != NULL) ? 1 : 0;
			if (state != 0) {
				stk_ser.PushStr(titles[i]);
			}
			stk_ser.PushNum(state);
		}
		stk_ser.PushNum(FARTTY_INTERACT_SET_FKEY_TITLES);

		if (Far2lInteract(stk_ser, detect_support)) {
			if (detect_support) {
				bool supported = false;
				stk_ser.PopNum(supported);
				fprintf(stderr, "%s: %ssupported\n",
					__FUNCTION__, supported ? "" : "not ");
				_fkeys_support = supported
					? FKS_SUPPORTED : FKS_NOT_SUPPORTED;
			}
		}

	} catch(std::exception &e) {

		fprintf(stderr, "%s: exception - %s\n", __FUNCTION__, e.what());
		_fkeys_support = FKS_NOT_SUPPORTED;
	}

	return (_fkeys_support == FKS_SUPPORTED);
}

BYTE TTYBackend::OnConsoleGetColorPalette()
{
	if (_norgb) {
		return 4;
	}

	if (_far2l_tty) try {
		StackSerializer stk_ser;
		stk_ser.PushNum(FARTTY_INTERACT_GET_COLOR_PALETTE);
		Far2lInteract(stk_ser, true);
		uint8_t bits, reserved;
		stk_ser.PopNum(bits);
		stk_ser.PopNum(reserved);
		return bits;

	} catch (std::exception &) {
		return 4;
	}

	static BYTE s_out = []() {
		const char *env = getenv("COLORTERM");
		if (env) {
			return (strcmp(env, "truecolor") == 0 || strcmp(env, "24bit") == 0) ? 24 : 8;
		}
		env = getenv("TERM");
		if (env && strstr(env, "256") != nullptr) {
			return 8;
		}
		return 4;
	} ();

	return s_out;
}

void TTYBackend::OnConsoleAdhocQuickEdit()
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushNum(FARTTY_INTERACT_CONSOLE_ADHOC_QEDIT);
		Far2lInteract(stk_ser, false);
	} catch (std::exception &) {}
}

DWORD64 TTYBackend::OnConsoleSetTweaks(DWORD64 tweaks)
{
	const auto prev_osc52clip_set = _osc52clip_set;
	_osc52clip_set = (tweaks & CONSOLE_OSC52CLIP_SET) != 0;

	if (_osc52clip_set != prev_osc52clip_set && !_far2l_tty && !_ttyx) {
		ChooseSimpleClipboardBackend();
	}

	bool override_default_palette = (tweaks & CONSOLE_TTY_PALETTE_OVERRIDE) != 0;
	{
		std::lock_guard<std::mutex> lock(_palette_mtx);
		std::swap(override_default_palette, _override_default_palette);
	}

	if (override_default_palette != ((tweaks & CONSOLE_TTY_PALETTE_OVERRIDE) != 0)) {
		std::unique_lock<std::mutex> lock(_async_mutex);
		_ae.palette = true;
		_async_cond.notify_all();
		while (_ae.palette) {
			_async_cond.wait(lock);
		}
	}

//

	DWORD64 out = TWEAK_STATUS_SUPPORT_TTY_PALETTE;

	if (!_far2l_tty && !_ttyx) {
		out|= TWEAK_STATUS_SUPPORT_OSC52CLIP_SET;
	}

	return out;
}

void TTYBackend::OnConsoleOverrideColor(DWORD Index, DWORD *ColorFG, DWORD *ColorBK)
{
	if (Index == (DWORD)-1) {
		const DWORD64 orig_attrs = g_winport_con_out->GetAttributes();
		DWORD64 new_attrs = orig_attrs;
		if ((*ColorFG & 0xff000000) == 0) {
			SET_RGB_FORE(new_attrs, *ColorFG);
		}
		if ((*ColorBK & 0xff000000) == 0) {
			SET_RGB_BACK(new_attrs, *ColorBK);
		}
		if (new_attrs != orig_attrs) {
			g_winport_con_out->SetAttributes(new_attrs);
		}

		*ColorFG = ConsoleForeground2RGB(g_winport_palette, orig_attrs & ~(DWORD64)COMMON_LVB_REVERSE_VIDEO).AsRGB();
		*ColorBK = ConsoleBackground2RGB(g_winport_palette, orig_attrs & ~(DWORD64)COMMON_LVB_REVERSE_VIDEO).AsRGB();
		return;
	}

	if (Index >= BASE_PALETTE_SIZE) {
		fprintf(stderr, "%s: too big index=%u\n", __FUNCTION__, Index);
		return;
	}

	const DWORD fg = (*ColorFG == (DWORD)-1) ? g_winport_palette.foreground[Index].AsRGB() : *ColorFG;
	const DWORD bk = (*ColorBK == (DWORD)-1) ? g_winport_palette.background[Index].AsRGB() : *ColorBK;
	bool palette_changed = false;
	{
		std::unique_lock<std::mutex> lock(_palette_mtx);
		*ColorFG = _palette.foreground[Index];
		*ColorBK = _palette.background[Index];
		if (fg != (DWORD)-2 && _palette.foreground[Index] != fg) {
			_palette.foreground[Index] = fg;
			palette_changed = true;
		}
		if (bk != (DWORD)-2 && _palette.background[Index] != bk) {
			_palette.background[Index] = bk;
			palette_changed = true;
		}
	}

	if (palette_changed) {
		std::unique_lock<std::mutex> lock(_async_mutex);
		_ae.palette = true;
		_async_cond.notify_all();
		while (_ae.palette) {
			_async_cond.wait(lock);
		}
	}
}

void TTYBackend::OnConsoleGetBasePalette(void *pbuff)
{
	memcpy(pbuff, &g_winport_palette, BASE_PALETTE_SIZE * sizeof(DWORD) * 2);

	return;
}

bool TTYBackend::OnConsoleSetBasePalette(void *pbuff)
{
	if (!pbuff) return false;

	{
		std::unique_lock<std::mutex> lock(_palette_mtx);
		memcpy(&_palette, pbuff, BASE_PALETTE_SIZE * sizeof(DWORD) * 2);
	}

	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.palette = true;
	_async_cond.notify_all();
	while (_ae.palette) {
		_async_cond.wait(lock);
	}

	return true;
}

void TTYBackend::OnConsoleChangeFont()
{
}

void TTYBackend::OnConsoleSaveWindowState()
{
}

void TTYBackend::OnConsoleSetCursorBlinkTime(DWORD interval)
{

}

void TTYBackend::OnConsoleSetMaximized(bool maximized)
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushNum(maximized ? FARTTY_INTERACT_WINDOW_MAXIMIZE : FARTTY_INTERACT_WINDOW_RESTORE);
		Far2lInteract(stk_ser, false);
	} catch (std::exception &) {}
}

void TTYBackend::ChooseSimpleClipboardBackend()
{
	if (_ext_clipboard) {
		return;
	}

	if (_osc52clip_set) {
		IOSC52Interactor *interactor = this;
		_clipboard_backend_setter.Set<OSC52ClipboardBackend>(interactor);
	} else {
		_clipboard_backend_setter.Set<FSClipboardBackend>();
	}
}

void TTYBackend::OSC52SetClipboard(const char *text)
{
	fprintf(stderr, "TTYBackend::OSC52SetClipboard\n");
	std::unique_lock<std::mutex> lock(_async_mutex);
	_osc52clip = text;
	_ae.osc52clip_set = true;
	_async_cond.notify_all();
}

bool TTYBackend::Far2lInteract(StackSerializer &stk_ser, bool wait)
{
	if (!_far2l_tty || _exiting)
		return false;

	std::shared_ptr<Far2lInteractData> pfi = std::make_shared<Far2lInteractData>();
	pfi->stk_ser.Swap(stk_ser);
	pfi->waited = wait;

	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		_far2l_interacts_queued.emplace_back(pfi);
		_ae.far2l_interact = 1;
		_async_cond.notify_all();
	}

	if (!wait)
		return true;

	pfi->evnt.Wait();

	std::unique_lock<std::mutex> lock_sent(_far2l_interacts_sent);
	if (_exiting)
		return false;

	pfi->stk_ser.Swap(stk_ser);

	return true;
}

void TTYBackend::OnConsoleExit()
{
	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		_exiting = true;
		_async_cond.notify_all();
	}
	KickAss();
}

bool TTYBackend::OnConsoleIsActive()
{
	return false;//true;
}

void TTYBackend_OnTerminalDamaged(bool flush_input_queue)
{
	__sync_add_and_fetch ( &s_terminal_size_change_id, 1);
	if (g_vtb) {
		g_vtb->KickAss(flush_input_queue);
	}
}

static void OnFar2lKey(bool down, StackSerializer &stk_ser)
{
	try {
		INPUT_RECORD ir {};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.bKeyDown = down ? TRUE : FALSE;

		ir.Event.KeyEvent.uChar.UnicodeChar = (wchar_t)stk_ser.PopU32();
		stk_ser.PopNum(ir.Event.KeyEvent.dwControlKeyState);
		stk_ser.PopNum(ir.Event.KeyEvent.wVirtualScanCode);
		stk_ser.PopNum(ir.Event.KeyEvent.wVirtualKeyCode);
		stk_ser.PopNum(ir.Event.KeyEvent.wRepeatCount);
		g_winport_con_in->Enqueue(&ir, 1);

	} catch (std::exception &) {
		fprintf(stderr, "OnFar2lKey: broken args!\n");
	}
}

static void OnFar2lTerminalSize(StackSerializer &stk_ser)
{
	stk_ser.PopNum(g_far2l_term_height);
	stk_ser.PopNum(g_far2l_term_width);
	TTYBackend_OnTerminalDamaged(false);
}

static void OnFar2lKeyCompact(bool down, StackSerializer &stk_ser)
{
	try {
		INPUT_RECORD ir {};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.bKeyDown = down ? TRUE : FALSE;
		ir.Event.KeyEvent.wRepeatCount = 1;
		ir.Event.KeyEvent.uChar.UnicodeChar = (wchar_t)(uint32_t)stk_ser.PopU16();
		ir.Event.KeyEvent.dwControlKeyState = stk_ser.PopU16();
		ir.Event.KeyEvent.wVirtualKeyCode = stk_ser.PopU8();
		ir.Event.KeyEvent.wVirtualScanCode = WINPORT(MapVirtualKey)(ir.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
		g_winport_con_in->Enqueue(&ir, 1);

	} catch (std::exception &e) {
		fprintf(stderr, "OnFar2lKeyCompact: %s\n", e.what());
	}
}

static void OnFar2lMouse(bool compact, StackSerializer &stk_ser)
{
	try {
		INPUT_RECORD ir {};
		ir.EventType = MOUSE_EVENT;

		if (compact) {
			ir.Event.MouseEvent.dwEventFlags = stk_ser.PopU8();
			ir.Event.MouseEvent.dwControlKeyState = stk_ser.PopU8();
			ir.Event.MouseEvent.dwButtonState = stk_ser.PopU16();

			ir.Event.MouseEvent.dwButtonState = (ir.Event.MouseEvent.dwButtonState & 0xff)
				| ( (ir.Event.MouseEvent.dwButtonState & 0xff00) << 8);

		} else {
			stk_ser.PopNum(ir.Event.MouseEvent.dwEventFlags);
			stk_ser.PopNum(ir.Event.MouseEvent.dwControlKeyState);
			stk_ser.PopNum(ir.Event.MouseEvent.dwButtonState);
		}

		stk_ser.PopNum(ir.Event.MouseEvent.dwMousePosition.Y);
		stk_ser.PopNum(ir.Event.MouseEvent.dwMousePosition.X);

		g_winport_con_in->Enqueue(&ir, 1);

	} catch (std::exception &) {
		fprintf(stderr, "OnFar2lMouse: broken args!\n");
	}
}

void TTYBackend::OnUsingExtension(char extension)
{
	if (_using_extension != extension) {
		_using_extension = extension;
		UpdateBackendIdentification();
	}
}

void TTYBackend::OnInspectKeyEvent(KEY_EVENT_RECORD &event)
{
	bool in_kernel = 0;
	// In kernel console use kernel control keys info even if using TTY|X for clipboard
	#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
		int kd_mode;
		#if defined(__linux__)
		if (ioctl(_stdin, KDGETMODE, &kd_mode) == 0) {
		#else
		if (ioctl(_stdin, KDGKBMODE, &kd_mode) == 0) {
		#endif
			in_kernel = 1;
		}
	#endif

	if (_ttyx && !_using_extension && !in_kernel) {
		_ttyx->InspectKeyEvent(event);

	} else {
		event.dwControlKeyState|= QueryControlKeys();
	}

	if (!event.wVirtualKeyCode) {
		event.wVirtualKeyCode = WChar2WinVKeyCode(event.uChar.UnicodeChar);
	}
	if (!event.uChar.UnicodeChar && IsEnhancedKey(event.wVirtualKeyCode)) {
		event.dwControlKeyState|= ENHANCED_KEY;
	}
}

void TTYBackend::OnFar2lEvent(StackSerializer &stk_ser)
{
	if (!_far2l_tty) {
		fprintf(stderr, "Far2lEvent unexpected!\n");
		return;
	}

	char code = stk_ser.PopChar();

	switch (code) {
		case FARTTY_INPUT_MOUSE: case FARTTY_INPUT_MOUSE_COMPACT:
			OnFar2lMouse(code == FARTTY_INPUT_MOUSE_COMPACT, stk_ser);
			break;

		case FARTTY_INPUT_KEYDOWN: case FARTTY_INPUT_KEYUP:
			OnFar2lKey(code == FARTTY_INPUT_KEYDOWN, stk_ser);
			break;

		case FARTTY_INPUT_KEYDOWN_COMPACT: case FARTTY_INPUT_KEYUP_COMPACT:
			OnFar2lKeyCompact(code == FARTTY_INPUT_KEYDOWN_COMPACT, stk_ser);
			break;

		case FARTTY_INPUT_TERMINAL_SIZE:
			OnFar2lTerminalSize(stk_ser);
			break;

		default:
			fprintf(stderr, "Far2lEvent unknown code=0x%x!\n", (unsigned int)(unsigned char)code);
	}
}

void TTYBackend::OnFar2lReply(StackSerializer &stk_ser)
{
	if (!_far2l_tty) {
		fprintf(stderr, "OnFar2lReply: unexpected!\n");
		return;
	}

	uint8_t id;
	stk_ser.PopNum(id);

	std::unique_lock<std::mutex> lock_sent(_far2l_interacts_sent);

	auto i = _far2l_interacts_sent.find(id);
	if (i != _far2l_interacts_sent.end()) {
		auto pfi = i->second;
		_far2l_interacts_sent.erase(i);
		pfi->stk_ser.Swap(stk_ser);
		pfi->evnt.Signal();
	}
}

void TTYBackend::OnInputBroken()
{
	std::unique_lock<std::mutex> lock_sent(_far2l_interacts_sent);
	for (auto &i : _far2l_interacts_sent) {
		i.second->stk_ser.Clear();
		i.second->evnt.Signal();
	}
	_far2l_interacts_sent.clear();
}

DWORD TTYBackend::QueryControlKeys()
{
	DWORD out = 0;

#ifdef __linux__
	unsigned char state = 6;
/* #ifndef KG_SHIFT
# define KG_SHIFT        0
# define KG_CTRL         2
# define KG_ALT          3
# define KG_ALTGR        1
# define KG_SHIFTL       4
# define KG_KANASHIFT    4
# define KG_SHIFTR       5
# define KG_CTRLL        6
# define KG_CTRLR        7
# define KG_CAPSSHIFT    8
#endif */

	if (ioctl(_stdin, TIOCLINUX, &state) == 0) {
		if (state & ((1 << KG_SHIFT) | (1 << KG_SHIFTL) | (1 << KG_SHIFTR))) {
			out|= SHIFT_PRESSED;
		}
		if (state & (1 << KG_CTRLL)) {
			out|= LEFT_CTRL_PRESSED;
		}
		if (state & (1 << KG_CTRLR)) {
			out|= RIGHT_CTRL_PRESSED;
		}
		if ( (state & (1 << KG_CTRL)) != 0
		&& ((state & ((1 << KG_CTRLL) | (1 << KG_CTRLR))) == 0) ) {
			out|= LEFT_CTRL_PRESSED;
		}

		if (state & (1 << KG_ALTGR)) {
			out|= RIGHT_ALT_PRESSED;
		}
		else if (state & (1 << KG_ALT)) {
			out|= LEFT_ALT_PRESSED;
		}
	}
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__linux__)
	unsigned long int leds = 0;
	if (ioctl(_stdin, KDGETLED, &leds) == 0) {
		if (leds & 1) {
			out|= SCROLLLOCK_ON;
		}
		if (leds & 2) {
			out|= NUMLOCK_ON;
		}
		if (leds & 4) {
			out|= CAPSLOCK_ON;
		}
	}
#endif
	return out;
}

void TTYBackend::OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text)
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushStr(Wide2MB(text));
		stk_ser.PushStr(Wide2MB(title));
		stk_ser.PushNum(FARTTY_INTERACT_DESKTOP_NOTIFICATION);
		Far2lInteract(stk_ser, false);
	} catch (std::exception &) {}
}

bool TTYBackend::OnConsoleBackgroundMode(bool TryEnterBackgroundMode)
{
	if (_notify_pipe == -1) {
		return false;
	}

	if (TryEnterBackgroundMode) {
		std::unique_lock<std::mutex> lock(_async_mutex);
		_ae.go_background = true;
		_async_cond.notify_all();
	}

	return true;
}


static void OnSigWinch(int)
{
	TTYBackend_OnTerminalDamaged(false);
}


static int g_std_in = -1, g_std_out = -1;
static bool g_far2l_tty = false;
static struct termios g_ts_cont {};


static void OnSigTstp(int signo)
{
	if (g_far2l_tty)
		TTYNegotiateFar2l(g_std_in, g_std_out, false);

	if (tcgetattr(g_std_out, &g_ts_cont) == -1) {
		perror("OnSigTstp - tcgetattr");
	}
	raise(SIGSTOP);
}


static void OnSigCont(int signo)
{
	if (tcsetattr(g_std_out, TCSADRAIN, &g_ts_cont ) == -1) {
		perror("OnSigCont - tcsetattr");
	}
	if (g_far2l_tty)
		TTYNegotiateFar2l(g_std_in, g_std_out, true);

	TTYBackend_OnTerminalDamaged(true);
}

static void OnSigHup(int signo)
{
	// drop sudo privileges once pending sudo operation completes
	// leaving them is similar to leaving root console unattended
	sudo_client_drop();

	FDScope dev_null(open("/dev/null", O_RDWR));
	if (dev_null.Valid()) {
//		dup2(dev_null, 2);
//		dup2(dev_null, 1);
//		dup2(dev_null, 0);
		if (g_std_out != -1)
			dup2(dev_null, g_std_out);
		if (g_std_in != -1)
			dup2(dev_null, g_std_in);
	}
	if (g_vtb) {
		g_vtb->KickAss(true);
	}
}


bool WinPortMainTTY(const char *full_exe_path, int std_in, int std_out, bool ext_clipboard, bool norgb, DWORD nodetect, bool far2l_tty, unsigned int esc_expiration, int notify_pipe, int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result)
{
	TTYBackend vtb(full_exe_path, std_in, std_out, ext_clipboard, norgb, nodetect, far2l_tty, esc_expiration, notify_pipe, result);

	if (!vtb.Startup()) {
		return false;
	}

	g_std_in = std_in;
	g_std_out = std_out;
	g_far2l_tty = far2l_tty;

	auto orig_winch = signal(SIGWINCH, OnSigWinch);
	auto orig_tstp = signal(SIGTSTP, OnSigTstp);
	auto orig_cont = signal(SIGCONT, OnSigCont);
	auto orig_hup = signal(SIGHUP, (notify_pipe != -1) ? OnSigHup : SIG_DFL); // notify_pipe == -1 means --mortal specified

	*result = 0; // set OK status for case if app will go background
	*result = AppMain(argc, argv);

	signal(SIGHUP, orig_hup);
	signal(SIGCONT, orig_tstp);
	signal(SIGTSTP, orig_cont);
	signal(SIGWINCH, orig_winch);

	return true;
}
