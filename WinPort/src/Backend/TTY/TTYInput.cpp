#include <assert.h>
#include "TTYInput.h"
#include "Backend.h"
#include "WideMB.h"
#include "WinPort.h"
#include <utils.h>

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


TTYInput::TTYInput(ITTYInputSpecialSequenceHandler *handler) :
	_parser(handler), _handler(handler)
{
}

void TTYInput::PostCharEvent(wchar_t ch)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wRepeatCount = 1;
	ir.Event.KeyEvent.uChar.UnicodeChar = ch;

	if (_handler) {
		ir.Event.KeyEvent.dwControlKeyState|= _handler->OnQueryControlKeys();
	}

	if (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
		ir.Event.KeyEvent.wVirtualKeyCode = WChar2WinVKeyCode(ch);
	} else {
		ir.Event.KeyEvent.wVirtualKeyCode = VK_UNASSIGNED;
	}

	ir.Event.KeyEvent.bKeyDown = TRUE;
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
		if (decoded == TTY_PARSED_WANTMORE)
			break;

		assert(decoded <= _buf.size());
		_buf.erase(_buf.begin(), _buf.begin() + decoded);
	}
}

void TTYInput::OnInput(const char *data, size_t len)
{
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
