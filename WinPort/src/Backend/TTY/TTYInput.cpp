#include <assert.h>
#include "TTYInput.h"
#include "Backend.h"
#include "WideMB.h"
#include "WinPort.h"
#include <utils.h>

#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>  // For getuid() and getpwuid()
#include <pwd.h>     // For struct passwd

TTYInput::TTYInput(ITTYInputSpecialSequenceHandler *handler) :
	_parser(handler), _handler(handler)
{
}

void TTYInput::PostCharEvent(wchar_t ch)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = TRUE;
	ir.Event.KeyEvent.wRepeatCount = 1;
	ir.Event.KeyEvent.uChar.UnicodeChar = ch;

	if (_handler) {
		_handler->OnInspectKeyEvent(ir.Event.KeyEvent);
	}

	g_winport_con_in->Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in->Enqueue(&ir, 1);
}

size_t TTYInput::BufTryDecodeUTF8()
{
	wchar_t wc;
	size_t l = _buf.size();
	const auto cr = MB2Wide_Unescaped(_buf.data(), l, wc, false);
	if (cr & CONV_NEED_MORE_SRC) {
		return TTY_PARSED_WANTMORE;
	}
	assert(l);
	PostCharEvent(wc);
	return l;
}

void TTYInput::OnBufUpdated(bool idle)
{
	while (!_buf.empty()) {
		size_t decoded = _parser.Parse(&_buf[0], _buf.size(), idle);

		//work-around for double encoded mouse events in win32-input mode
		//here we parse mouse sequence from accumulated buffer
		_parser.ParseWinDoubleBuffer(idle);

		switch (decoded) {
			case TTY_PARSED_PLAINCHARS:
				decoded = BufTryDecodeUTF8();
				break;

			case TTY_PARSED_BADSEQUENCE:
				decoded = 1; // discard unrecognized sequences
				break;

			default:
				;
		}
		if (decoded == TTY_PARSED_WANTMORE) {
			if (!_buf.empty() && _buf.front() == 0x1b && _handler) {
				// check with TTYX if this is physical Escape key press
				INPUT_RECORD ir = {};
				ir.EventType = KEY_EVENT;
				ir.Event.KeyEvent.wRepeatCount = 1;
				ir.Event.KeyEvent.uChar.UnicodeChar = 0x1b;
				ir.Event.KeyEvent.bKeyDown = TRUE;
				_handler->OnInspectKeyEvent(ir.Event.KeyEvent);
				if (ir.Event.KeyEvent.wVirtualKeyCode != 0 && ir.Event.KeyEvent.wVirtualKeyCode != VK_UNASSIGNED) {
					g_winport_con_in->Enqueue(&ir, 1);
					ir.Event.KeyEvent.bKeyDown = FALSE;
					g_winport_con_in->Enqueue(&ir, 1);
					_buf.erase(_buf.begin());
					continue;
				}
			}
			break;
		}

		assert(decoded <= _buf.size());
		_buf.erase(_buf.begin(), _buf.begin() + decoded);
	}
}

void TTYInput::OnInput(const char *data, size_t len)
{

	// Get home directory
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;

	// Construct log file path
	std::string logFilePath = std::string(homedir) + "/far2l_ttyin.log";

	// Open log file in append mode
	std::ofstream logFile(logFilePath, std::ios::app);

	if (logFile.is_open()) {
		for (size_t i = 0; i < len; ++i) {
			if (data[i] == '\033') { // Check for escape character (27 in decimal or 033 in octal)
				logFile << std::endl; // Add newline before ESC
			}
			logFile << data[i];
		}
		logFile << std::flush; // Ensure data is written to the file immediately
		logFile.close();
	} else {
		std::cerr << "Error opening log file: " << logFilePath << std::endl;
	}

	if (len) try {
		_buf.reserve(_buf.size() + len);

		for (size_t i = 0; i < len; ++i)
			_buf.emplace_back(data[i]);

		OnBufUpdated(false);

	} catch (std::exception &e) {
		_buf.clear();
		_buf.shrink_to_fit();
		fprintf(stderr, "TTYInput::OnInput: %s\n", e.what());
		_handler->OnInputBroken();
	}
}

void TTYInput::OnIdleExpired()
{
	if (!_buf.empty()) try {
		OnBufUpdated(true);

	} catch (std::exception &e) {
		_buf.clear();
		_buf.shrink_to_fit();
		fprintf(stderr, "TTYInput::OnIdleExpired: %s\n", e.what());
		_handler->OnInputBroken();
	}
}
