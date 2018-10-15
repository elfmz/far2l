#include <assert.h>
#include "TTYInput.h"
#include "ConsoleInput.h"
#include "ConvertUTF.h"
#include "WinPort.h"
#include <utils.h>

extern ConsoleInput g_winport_con_in;

static bool IsEnhancedKey(WORD code)
{
	return (code==VK_LEFT || code==VK_RIGHT || code==VK_UP || code==VK_DOWN
		|| code==VK_HOME || code==VK_END || code==VK_NEXT || code==VK_PRIOR );
}


TTYInput::TTYInput()
{
}

void TTYInput::PostCharEvent(wchar_t ch)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = TRUE;
	ir.Event.KeyEvent.wRepeatCount = 1;
	ir.Event.KeyEvent.uChar.UnicodeChar = ch;
	ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_PERIOD;
	g_winport_con_in.Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in.Enqueue(&ir, 1);
}

void TTYInput::PostKeyEvent(const TTYInputKey &k)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wRepeatCount = 1;
//	ir.Event.KeyEvent.uChar.UnicodeChar = i.second.unicode_char;
	ir.Event.KeyEvent.wVirtualKeyCode = k.vk;
	ir.Event.KeyEvent.dwControlKeyState = k.control_keys;
	if (IsEnhancedKey(k.vk)) {
		ir.Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
	}
	ir.Event.KeyEvent.wVirtualScanCode = WINPORT(MapVirtualKey)(k.vk,MAPVK_VK_TO_VSC);
	ir.Event.KeyEvent.bKeyDown = TRUE;
	g_winport_con_in.Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in.Enqueue(&ir, 1);
}

size_t TTYInput::BufTryDecodeUTF8()
{
	const UTF8* utf8_start = (const UTF8*) &_buf[0];
	UTF32 utf32[2] = {}, *utf32_start = &utf32[0];
	ConvertUTF8toUTF32 ( &utf8_start, utf8_start + _buf.size(),
		&utf32_start, utf32_start + 1, lenientConversion);
	if (utf32_start != &utf32[0]) {
		PostCharEvent(utf32[0]);
		return (utf8_start - (const UTF8*)&_buf[0]);

	}
	_buf.erase(_buf.begin(), _buf.begin() + (utf8_start - (const UTF8*)&_buf[0]) );

	return 0;
}

void TTYInput::OnBufUpdated()
{
	TTYInputKey k;
	while (!_buf.empty()) {
		size_t decoded = _parser.Parse(k, &_buf[0], _buf.size());
		switch (decoded) {
			case (size_t)-1:
				decoded = BufTryDecodeUTF8();
				break;

			case (size_t)-2:
				decoded = 1; // discard unrecognized sequences
				break;

			case 0:
				break;

			default:
				PostKeyEvent(k);
		}
		if (!decoded)
			break;

		assert(decoded <= _buf.size());
		_buf.erase(_buf.begin(), _buf.begin() + decoded);
	}
}

void TTYInput::OnChar(char c)
{
	_buf.emplace_back(c);
	OnBufUpdated();
}
