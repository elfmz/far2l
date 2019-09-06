#include <assert.h>
#include "TTYInput.h"
#include "ConsoleInput.h"
#include "ConvertUTF.h"
#include "WinPort.h"
#include <utils.h>

extern ConsoleInput g_winport_con_in;

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
	ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_PERIOD;
	if (_handler)
		ir.Event.KeyEvent.dwControlKeyState|= _handler->OnQueryControlKeys();

	ir.Event.KeyEvent.bKeyDown = TRUE;
	g_winport_con_in.Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in.Enqueue(&ir, 1);
}

size_t TTYInput::BufTryDecodeUTF8()
{
	const UTF8* utf8_start = (const UTF8*) &_buf[0];

#if (__WCHAR_MAX__ > 0xffff)
	UTF32 utf32[2] = {}, *utf32_start = &utf32[0];
	ConvertUTF8toUTF32 ( &utf8_start, utf8_start + _buf.size(),
		&utf32_start, utf32_start + 1, lenientConversion);
	if (utf32_start != &utf32[0]) {
		PostCharEvent(utf32[0]);
		return (utf8_start - (const UTF8*)&_buf[0]);

	}
#else
	UTF16 utf16[2] = {}, *utf16_start = &utf16[0];
	ConvertUTF8toUTF16 ( &utf8_start, utf8_start + _buf.size(),
		&utf16_start, utf16_start + 1, lenientConversion);
	if (utf16_start != &utf16[0]) {
		PostCharEvent(utf16[0]);
		return (utf8_start - (const UTF8*)&_buf[0]);

	}
#endif
	_buf.erase(_buf.begin(), _buf.begin() + (utf8_start - (const UTF8*)&_buf[0]) );

	return 0;
}

void TTYInput::OnBufUpdated()
{
	while (!_buf.empty()) {
		size_t decoded = _parser.Parse(&_buf[0], _buf.size());
		switch (decoded) {
			case (size_t)-1:
				decoded = BufTryDecodeUTF8();
				break;

			case (size_t)-2:
				decoded = 1; // discard unrecognized sequences
				break;

			default:
				;
		}
		if (!decoded)
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

		OnBufUpdated();

	} catch (std::exception &e) {
		_buf.clear();
		_buf.shrink_to_fit();
		fprintf(stderr, "TTYInput: %s\n", e.what());
		_handler->OnInputBroken();
	}
}
