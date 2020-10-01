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
#endif
#include <os_call.hpp>
#include <ScopeHelpers.h>
#include "utils.h"
#include "CheckedCast.hpp"
#include "WinPortHandle.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "TTYBackend.h"
#include "TTYRevive.h"
#include "TTYFar2lClipboardBackend.h"
#include "TTYNegotiateFar2l.h"
#include "../FSClipboardBackend.h"

extern ConsoleOutput g_winport_con_out;
extern ConsoleInput g_winport_con_in;

static volatile long s_terminal_size_change_id = 0;
static TTYBackend * g_vtb = nullptr;

TTYBackend::TTYBackend(int std_in, int std_out, bool far2l_tty, unsigned int esc_expiration, int notify_pipe) :
	_stdin(std_in),
	_stdout(std_out),
	_far2l_tty(far2l_tty),
	_esc_expiration(esc_expiration),
	_notify_pipe(notify_pipe),
	_largest_window_size_ready(false)

{
	if (pipe_cloexec(_kickass) == -1) {
		_kickass[0] = _kickass[1] = -1;
	} else {
		fcntl(_kickass[1], F_SETFL,
			fcntl(_kickass[1], F_GETFL, 0) | O_NONBLOCK);
	}

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
	CheckedCloseFD(_notify_pipe);
}

bool TTYBackend::Startup()
{
	assert(!_reader_trd);

	_cur_width =_cur_height = 64;
	g_winport_con_out.GetSize(_cur_width, _cur_height);
	g_winport_con_out.SetBackend(this);

	if (pthread_create(&_reader_trd, nullptr, sReaderThread, this) != 0) {
		return false;
	}

	return true;
}

void TTYBackend::ReaderThread()
{
	bool prev_far2l_tty = false;
	while (!_exiting) {
		if (prev_far2l_tty != _far2l_tty || !_clipboard_backend_setter.IsSet()) {
			if (_far2l_tty) {
				IFar2lInterractor *interractor = this;
				_clipboard_backend_setter.Set<TTYFar2lClipboardBackend>(interractor);
			} else {
				_clipboard_backend_setter.Set<FSClipboardBackend>();
			}
			prev_far2l_tty = _far2l_tty;
		}

		{
			std::unique_lock<std::mutex> lock(_async_mutex);
			_deadio = false;
			_ae.flags.term_resized = true;
		}


		pthread_t writer_trd = 0;
		if (pthread_create(&writer_trd, nullptr, sWriterThread, this) == -1) {
			break;
		}

		try {
			ReaderLoop();

		} catch (const std::exception &e) {
			fprintf(stderr, "ReaderLoop: %s <%d>\n", e.what(), errno);
		}

		OnInputBroken();

		{
			std::unique_lock<std::mutex> lock(_async_mutex);
			_deadio = true;
			_async_cond.notify_all();
		}

		pthread_join(writer_trd, nullptr);

		CheckedCloseFD(_notify_pipe);

		while (!_exiting) {
			const std::string &info = StrWide2MB(g_winport_con_out.GetTitle());
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
			tv.tv_usec = (_esc_expiration - tv.tv_sec * 1000) * 1000;

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
				_ae.flags.term_resized = true;
				_async_cond.notify_all();
			}
		}
	}
}

void TTYBackend::WriterThread()
{
	try {
		TTYOutput tty_out(_stdout);

		while (!_exiting && !_deadio) {
			AsyncEvent ae;
			ae.all = 0;
			do {
				std::unique_lock<std::mutex> lock(_async_mutex);
				if (_ae.all == 0) {
					_async_cond.wait(lock);
				}
				if (_ae.all != 0) {
					std::swap(ae, _ae);
					break;
				}
			} while (!_exiting && !_deadio);

			if (ae.flags.term_resized) {
				DispatchTermResized(tty_out);
				ae.flags.output = true;
			}

			if (ae.flags.output)
				DispatchOutput(tty_out);

			if (ae.flags.title_changed) {
				tty_out.ChangeTitle(StrWide2MB(g_winport_con_out.GetTitle()));
			}

			if (ae.flags.far2l_interract)
				DispatchFar2lInterract(tty_out);

			tty_out.Flush();
		}

	} catch (const std::exception &e) {
		fprintf(stderr, "WriterThread: %s <%d>\n", e.what(), errno);
	}
	_deadio = true;
}


/////////////////////////////////////////////////////////////////////////

void TTYBackend::DispatchTermResized(TTYOutput &tty_out)
{
	struct winsize w = {};
	if (ioctl(_stdout, TIOCGWINSZ, &w) == 0) {
		if (_cur_width != w.ws_col || _cur_height != w.ws_row) {
			_cur_width = w.ws_col;
			_cur_height = w.ws_row;
			g_winport_con_out.SetSize(_cur_width, _cur_height);
			g_winport_con_out.GetSize(_cur_width, _cur_height);
			INPUT_RECORD ir = {};
			ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
			ir.Event.WindowBufferSizeEvent.dwSize.X = _cur_width;
			ir.Event.WindowBufferSizeEvent.dwSize.Y = _cur_height;
			g_winport_con_in.Enqueue(&ir, 1);
		}
		std::vector<CHAR_INFO> tmp;
		std::lock_guard<std::mutex> lock(_output_mutex);
		tty_out.MoveCursor(1, 1);
		_prev_height = _prev_width = 0;
		_prev_output.swap(tmp);// ensure memory released
	}
}

void TTYBackend::DispatchOutput(TTYOutput &tty_out)
{
	std::lock_guard<std::mutex> lock(_output_mutex);

	_cur_output.resize(_cur_width * _cur_height);

	COORD data_size = {CheckedCast<SHORT>(_cur_width), CheckedCast<SHORT>(_cur_height) };
	COORD data_pos = {0, 0};
	SMALL_RECT screen_rect = {0, 0, CheckedCast<SHORT>(_cur_width - 1), CheckedCast<SHORT>(_cur_height - 1)};
	g_winport_con_out.Read(&_cur_output[0], data_size, data_pos, screen_rect);

#if 1
	for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &_cur_output[y * _cur_width];
		if (y >= _prev_height) {
			tty_out.MoveCursor(y + 1, 1);
			tty_out.WriteLine(cur_line, _cur_width);
		} else {
			const CHAR_INFO *last_line = &_prev_output[y * _prev_width];
			for (unsigned int x = 0; x < _cur_width; ++x) {
				// print current character:
				//  if it doesnt match to its previos version
				// OR
				//  if some of next 2 characters dont match to their previous versions and
				//  printing current character doesnt require cursor pos update with
				//  tty_out.MoveCursor, cuz tty_out.MoveCursor generates 5 bytes and thus its
				//  more efficient to print 1..2 matching characters moving cursor by one than
				//  skip them and update cursor position later by sending 5-bytes ESC sequence
				if (x >= _prev_width
					  || cur_line[x].Char.UnicodeChar != last_line[x].Char.UnicodeChar
					  || cur_line[x].Attributes != last_line[x].Attributes) {
					if (tty_out.ShouldMoveCursor(y + 1, x + 1)) {
						tty_out.MoveCursor(y + 1, x + 1);
					}
					tty_out.WriteLine(&cur_line[x], 1);

				} else if (tty_out.ShouldMoveCursor(y + 1, x + 1)) {
					// matching character located at position that already requires update
					;

				} else if (x + 2 < _cur_width && ( x + 2 >= _prev_width
					 || cur_line[x + 2].Char.UnicodeChar != last_line[x + 2].Char.UnicodeChar
					 || cur_line[x + 2].Attributes != last_line[x + 2].Attributes) ) {
					// character x + 2 requires print, so avoid cursor pos update by
					// printing all 3 chars from current pos
					tty_out.WriteLine(&cur_line[x], 3);
					x+= 2;

				} else if (x + 1 < _cur_width && ( x + 1 >= _prev_width
					 || cur_line[x + 1].Char.UnicodeChar != last_line[x + 1].Char.UnicodeChar
					 || cur_line[x + 1].Attributes != last_line[x + 1].Attributes) ) {
					// character x + 1 requires print, so avoid cursor pos update by
					// printing all 2 chars from current pos
					tty_out.WriteLine(&cur_line[x], 2);
					x+= 1;
				}
			}
		}
	}

#else
	for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &_cur_output[y * _cur_width];
		if (tty_out.ShouldMoveCursor(y + 1, 1)) {
			tty_out.MoveCursor(y + 1, 1);
		}
		tty_out.WriteLine(cur_line, _cur_width);
	}
#endif
	_prev_width = _cur_width;
	_prev_height = _cur_height;
	_prev_output.swap(_cur_output);

	UCHAR cursor_height = 1;
	bool cursor_visible = false;
	COORD cursor_pos = g_winport_con_out.GetCursor(cursor_height, cursor_visible);
	if (tty_out.ShouldMoveCursor(cursor_pos.Y + 1, cursor_pos.X + 1)) {
		tty_out.MoveCursor(cursor_pos.Y + 1, cursor_pos.X + 1);
	}
	tty_out.ChangeCursor(cursor_visible);

	if (_far2l_cursor_height != (int)(unsigned int)cursor_height && _far2l_tty) {
		_far2l_cursor_height = (int)(unsigned int)cursor_height;

		StackSerializer stk_ser;
		stk_ser.PushPOD(cursor_height);
		stk_ser.PushPOD('h');
		stk_ser.PushPOD((uint8_t)0); // zero ID means not expecting reply
		tty_out.SendFar2lInterract(stk_ser);
	}
}


void TTYBackend::DispatchFar2lInterract(TTYOutput &tty_out)
{
	Far2lInterractV queued;
	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		queued.swap(_far2l_interracts_queued);
	}

	std::unique_lock<std::mutex> lock_sent(_far2l_interracts_sent);

	for (auto & i : queued) {
		uint8_t id = 0;
		if (i->waited) {
			if (_far2l_interracts_sent.size() >= 0xff) {
				fprintf(stderr,
					"TTYBackend::DispatchFar2lInterract: too many sent interracts - %ld\n",
					_far2l_interracts_sent.size());
				i->stk_ser.Clear();
				i->evnt.Signal();
				return;
			}
			for (;;) {
				id = ++_far2l_interracts_sent._id_counter;
				if (id && _far2l_interracts_sent.find(id) == _far2l_interracts_sent.end()) break;
			}
		}
		i->stk_ser.PushPOD(id);

		if (i->waited)
			_far2l_interracts_sent.emplace(id, i);

		tty_out.SendFar2lInterract(i->stk_ser);
	}
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
	_ae.flags.output = true;
	_async_cond.notify_all();
}

void TTYBackend::OnConsoleOutputResized()
{
	OnConsoleOutputUpdated(nullptr, 0);
}

void TTYBackend::OnConsoleOutputTitleChanged()
{
	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.flags.title_changed = true;
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
			stk_ser.PushPOD('w');
			if (Far2lInterract(stk_ser, true)) {
				stk_ser.PopPOD(out);
				_largest_window_size = out;
				_largest_window_size_ready = true;
			}
		} catch(std::exception &) {; }
	}

	return out;
}

void TTYBackend::OnConsoleAdhocQuickEdit()
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD('e');
		Far2lInterract(stk_ser, false);
	} catch (std::exception &) {}
}

DWORD TTYBackend::OnConsoleSetTweaks(DWORD tweaks)
{
	return 0;
}

void TTYBackend::OnConsoleChangeFont()
{
}

void TTYBackend::OnConsoleSetMaximized(bool maximized)
{
	try {
		StackSerializer stk_ser;
		stk_ser.PushPOD((maximized ? 'M' : 'm'));
		Far2lInterract(stk_ser, false);
	} catch (std::exception &) {}
}

bool TTYBackend::Far2lInterract(StackSerializer &stk_ser, bool wait)
{
	if (!_far2l_tty || _exiting)
		return false;

	std::shared_ptr<Far2lInterractData> pfi = std::make_shared<Far2lInterractData>();
	pfi->stk_ser.Swap(stk_ser);
	pfi->waited = wait;

	{
		std::unique_lock<std::mutex> lock(_async_mutex);
		_far2l_interracts_queued.emplace_back(pfi);
		_ae.flags.far2l_interract = 1;
		_async_cond.notify_all();
	}

	if (!wait)
		return true;

	pfi->evnt.Wait();

	std::unique_lock<std::mutex> lock_sent(_far2l_interracts_sent);
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

void TTYBackend::OnFar2lKey(bool down, StackSerializer &stk_ser)
{
	try {
		INPUT_RECORD ir {};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.bKeyDown = down ? TRUE : FALSE;

		uint32_t key;
		stk_ser.PopPOD(key);
		ir.Event.KeyEvent.uChar.UnicodeChar = (wchar_t)key;
		stk_ser.PopPOD(ir.Event.KeyEvent.dwControlKeyState);
		stk_ser.PopPOD(ir.Event.KeyEvent.wVirtualScanCode);
		stk_ser.PopPOD(ir.Event.KeyEvent.wVirtualKeyCode);
		stk_ser.PopPOD(ir.Event.KeyEvent.wRepeatCount);
		g_winport_con_in.Enqueue(&ir, 1);

	} catch (std::exception &) {
		fprintf(stderr, "OnFar2lKey: broken args!\n");
	}
}

void TTYBackend::OnFar2lMouse(StackSerializer &stk_ser)
{
	try {
		INPUT_RECORD ir {};
		ir.EventType = MOUSE_EVENT;

		stk_ser.PopPOD(ir.Event.MouseEvent.dwEventFlags);
		stk_ser.PopPOD(ir.Event.MouseEvent.dwControlKeyState);
		stk_ser.PopPOD(ir.Event.MouseEvent.dwButtonState);
		stk_ser.PopPOD(ir.Event.MouseEvent.dwMousePosition.Y);
		stk_ser.PopPOD(ir.Event.MouseEvent.dwMousePosition.X);

		g_winport_con_in.Enqueue(&ir, 1);

	} catch (std::exception &) {
		fprintf(stderr, "OnFar2lMouse: broken args!\n");
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
		case 'M':
			OnFar2lMouse(stk_ser);
			break;

		case 'K': case 'k':
			OnFar2lKey(code == 'K', stk_ser);
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
	stk_ser.PopPOD(id);

	std::unique_lock<std::mutex> lock_sent(_far2l_interracts_sent);

	auto i = _far2l_interracts_sent.find(id);
	if (i != _far2l_interracts_sent.end()) {
		auto pfi = i->second;
		_far2l_interracts_sent.erase(i);
		pfi->stk_ser.Swap(stk_ser);
		pfi->evnt.Signal();
	}
}

void TTYBackend::OnInputBroken()
{
	std::unique_lock<std::mutex> lock_sent(_far2l_interracts_sent);
	for (auto &i : _far2l_interracts_sent) {
		i.second->stk_ser.Clear();
		i.second->evnt.Signal();
	}
	_far2l_interracts_sent.clear();
}

DWORD TTYBackend::OnQueryControlKeys()
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

	state = 0;
	if (ioctl (_stdin, KDGETLED, &state) == 0) {
		if (state & 1) {
			out|= SCROLLLOCK_ON;
		}
		if (state & 2) {
			out|= NUMLOCK_ON;
		}
		if (state & 4) {
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
		stk_ser.PushPOD('n');
		Far2lInterract(stk_ser, false);
	} catch (std::exception &) {}
}

static void OnSigHup(int signo);

bool TTYBackend::OnConsoleBackgroundMode(bool TryEnterBackgroundMode)
{
	if (_notify_pipe == -1) {
		return false;
	}

	if (TryEnterBackgroundMode) {
		OnSigHup(SIGHUP);
//		raise(SIGHUP);
	}

	return true;
}


void TTYBackend_OnTerminalDamaged(bool flush_input_queue)
{
	__sync_add_and_fetch ( &s_terminal_size_change_id, 1);
	if (g_vtb) {
		g_vtb->KickAss(flush_input_queue);
	}
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

	tcgetattr(g_std_out, &g_ts_cont);
	raise(SIGSTOP);
}


static void OnSigCont(int signo)
{
	tcsetattr(g_std_out, TCSADRAIN, &g_ts_cont );
	if (g_far2l_tty)
		TTYNegotiateFar2l(g_std_in, g_std_out, true);

	TTYBackend_OnTerminalDamaged(true);
}

static void OnSigHup(int signo)
{
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


bool WinPortMainTTY(int std_in, int std_out, bool far2l_tty, unsigned int esc_expiration, int notify_pipe, int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result)
{
	TTYBackend vtb(std_in, std_out, far2l_tty, esc_expiration, notify_pipe);

	if (!vtb.Startup()) {
		return false;
	}

	g_std_in = std_in;
	g_std_out = std_out;
	g_far2l_tty = far2l_tty;

	auto orig_winch = signal(SIGWINCH,  OnSigWinch);
	auto orig_tstp = signal(SIGTSTP, OnSigTstp);
	auto orig_cont = signal(SIGCONT, OnSigCont);
	auto orig_hup = signal(SIGHUP, (notify_pipe != -1) ? OnSigHup : SIG_DFL); // notify_pipe == -1 means --mortal specified

	*result = AppMain(argc, argv);

	signal(SIGHUP, orig_hup);
	signal(SIGCONT, orig_tstp);
	signal(SIGTSTP, orig_cont);
	signal(SIGWINCH, orig_winch);

	return true;
}

