#include "TTYInput.h"
#include "ConsoleInput.h"
#include "ConvertUTF.h"
#include "WinPort.h"

extern ConsoleInput g_winport_con_in;

void TTYInput::PostSimpleKeyEvent(wchar_t ch)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = TRUE;
	ir.Event.KeyEvent.wRepeatCount = 1;
	ir.Event.KeyEvent.uChar.UnicodeChar = ch;
	ir.Event.KeyEvent.wVirtualKeyCode = ch;
	ir.Event.KeyEvent.wVirtualScanCode = WINPORT(MapVirtualKey)(ch, MAPVK_VK_TO_VSC);
	g_winport_con_in.Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in.Enqueue(&ir, 1);
}

bool TTYInput::BufParseIterationSimple()
{
	const UTF8* utf8_start = (const UTF8*) &_buf[0];
	UTF32 utf32[2] = {}, *utf32_start = &utf32[0];
	ConvertUTF8toUTF32 ( &utf8_start, utf8_start + _buf.size(),
		&utf32_start, utf32_start + 1, lenientConversion);
	if (utf32_start != &utf32[0]) {
		PostSimpleKeyEvent(utf32[0]);
		_buf.erase(_buf.begin(), _buf.begin() + (utf8_start - (const UTF8*)&_buf[0]) );
		return true;

	} else if (_buf.size() > 6) {
		fprintf(stderr, "Failed to decode UTF8 sequence: 0x%x\n", _buf[0]);
		_buf.erase(_buf.begin());
		return true;
	}

	return false;
}

bool TTYInput::BufParseIterationCSI()
{
	_buf.erase(_buf.begin());//TODO
	return true;
}

void TTYInput::OnBufUpdated()
{
	while (!_buf.empty()) {
		if (_buf[0] == '\x1b') {
			if (!BufParseIterationCSI()) {
				break;
			}
		} else {
			if (!BufParseIterationSimple()) {
				break;
			}
		}
	}
}

void TTYInput::OnChar(char c)
{
	_buf.emplace_back(c);
	OnBufUpdated();
}
