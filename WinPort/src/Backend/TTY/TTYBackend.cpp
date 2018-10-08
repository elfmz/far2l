#include <assert.h>
#include <errno.h>
#include "utils.h"
#include "WinPortHandle.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "TTYBackend.h"

extern ConsoleOutput g_winport_con_out;
extern ConsoleInput g_winport_con_in;

TTYBackend::TTYBackend(int std_in, int std_out) :
	_stdin(std_in),
	_stdout(std_out),
	_tty_out(std_out)
{
	memset(&_ts, 0 , sizeof(_ts));
	_ts_r = tcgetattr(_stdout, &_ts);
	if (_ts_r == 0) {
		struct termios ts_ne = _ts;
		//ts_ne.c_lflag &= ~(ECHO | ECHONL);
		cfmakeraw(&ts_ne);
		if (tcsetattr( _stdout, TCSADRAIN, &ts_ne ) != 0) {
			perror("TTYBackend: tcsetattr");
		}
	} else {
		perror("TTYBackend: tcgetattr");
	}
}

TTYBackend::~TTYBackend()
{
	if (_reader_trd) {
		pthread_join(_reader_trd, nullptr);
		_reader_trd = 0;
	}
	if (_writer_trd) {
		pthread_join(_writer_trd, nullptr);
		_writer_trd = 0;
	}
	if (_ts_r == 0) {
		if (tcsetattr( _stdout, TCSADRAIN, &_ts ) != 0) {
			perror("~TTYBackend: tcsetattr");
		}
	}
}

bool TTYBackend::Startup()
{
	assert(!_writer_trd);
	assert(!_reader_trd);

	if (pthread_create(&_writer_trd, NULL, sWriterThread, this) != 0) {
		return false;
	}

	if (pthread_create(&_reader_trd, NULL, sReaderThread, this) != 0) {
		_exiting = true;
		return false;
	}
	_cur_width =_cur_height = 64;
	g_winport_con_out.GetSize(_cur_width, _cur_height);
	OnConsoleOutputUpdated(NULL, 0);
	g_winport_con_out.SetBackend(this);
	return true;
}


void TTYBackend::WriterThread()
{
	try {
		_tty_out.SetScreenBuffer(true);
		_tty_out.Flush();
		while (!_exiting) {
			AsyncEvent ae;
			{
				std::unique_lock<std::mutex> lock(_async_mutex);
				_async_cond.wait(lock);
				std::swap(ae, _ae);
			}
			if (ae.output)
				DispatchOutput();

			_tty_out.Flush();
		}
		_tty_out.SetScreenBuffer(false);
		_tty_out.Flush();

	} catch (std::exception &e) {
		fprintf(stderr, "WriterThread: %s <%d>\n", e.what(), errno);
	}
	_exiting = true;
}

void TTYBackend::ReaderThread()
{
	try {
		while (!_exiting) {
			char c = 0;
			if (read(_stdin, &c, 1) <= 0) {
				throw std::runtime_error("read failed");
			}
			_tty_in.OnChar(c);
		}
	} catch (std::exception &e) {
		fprintf(stderr, "ReaderThread: %s <%d>\n", e.what(), errno);
	}
	_exiting = true;
}

/////////////////////////////////////////////////////////////////////////

void TTYBackend::DispatchOutput()
{
	std::lock_guard<std::mutex> lock(_output_mutex);

	std::vector<CHAR_INFO> output(_cur_width * _cur_height);

	COORD data_size = {_cur_width, _cur_height};
	COORD data_pos = {0, 0};
	SMALL_RECT screen_rect = {0, 0, _cur_width - 1, _cur_height - 1};
	g_winport_con_out.Read(&output[0], data_size, data_pos, screen_rect);

#if 1
	for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &output[y * _cur_width];
		if (y >= _last_height) {
			_tty_out.MoveCursor(y, 0);
			_tty_out.WriteLine(cur_line, _cur_width);
		} else {
			const CHAR_INFO *last_line = &_last_output[y * _last_width];
			for (unsigned int x = 0; x < _cur_width; ++x) {
				if (x >= _last_width
				 || cur_line[x].Char.UnicodeChar != last_line[x].Char.UnicodeChar
				 || cur_line[x].Attributes != last_line[x].Attributes) {
					_tty_out.MoveCursor(y, x);
					_tty_out.WriteLine(&cur_line[x], 1);
				}
			}
		}
	}

#else
	for (unsigned int y = 0; y < _cur_height; ++y) {
		const CHAR_INFO *cur_line = &output[y * _cur_width];
		_tty_out.MoveCursor(y, 0);
		_tty_out.WriteLine(cur_line, _cur_width);
	}
#endif
	_last_width = _cur_width;
	_last_height = _cur_height;
	_last_output.swap(output);

	UCHAR cur_height = 1;
	bool cur_visible = false;
	COORD cur_pos = g_winport_con_out.GetCursor(cur_height, cur_visible);
	_tty_out.MoveCursor(cur_pos.Y, cur_pos.X);
	_tty_out.ChangeCursor(cur_visible, cur_height);
}

/////////////////////////////////////////////////////////////////////////

void TTYBackend::OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count)
{
	std::unique_lock<std::mutex> lock(_async_mutex);
	_ae.output  = true;
	_async_cond.notify_all();
}

void TTYBackend::OnConsoleOutputResized()
{
	OnConsoleOutputUpdated(NULL, 0);
}

void TTYBackend::OnConsoleOutputTitleChanged()
{
	//_tty_out.SetWindowTitle(g_winport_con_out.GetTitle());
	//ESC]2;titleST
}

void TTYBackend::OnConsoleOutputWindowMoved(bool absolute, COORD pos)
{
}

COORD TTYBackend::OnConsoleGetLargestWindowSize()
{
	COORD out = {0x100, 0x100};
	return out;
}

void TTYBackend::OnConsoleAdhocQuickEdit()
{
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
}

void TTYBackend::OnConsoleExit()
{
	_exiting = true;
}

bool TTYBackend::OnConsoleIsActive()
{
	return true;
}

//

bool TTYBackend::OnClipboardOpen()
{
	return 0;
}

void TTYBackend::OnClipboardClose()
{
}

void TTYBackend::OnClipboardEmpty()
{
}

bool TTYBackend::OnClipboardIsFormatAvailable(UINT format)
{
	return 0;
}

void *TTYBackend::OnClipboardSetData(UINT format, void *data)
{
	return 0;
}

void *TTYBackend::OnClipboardGetData(UINT format)
{
	return 0;
}

UINT TTYBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	return 0;
}



bool WinPortMainTTY(int std_in, int std_out, int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result)
{
	TTYBackend  vtb(std_in, std_out);
	if (!vtb.Startup()) {
		return false;
	}

	*result = AppMain(argc, argv);

	return true;
}
