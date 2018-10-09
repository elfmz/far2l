#include "TTYInput.h"
#include "ConsoleInput.h"
#include "ConvertUTF.h"
#include "WinPort.h"
#include <utils.h>
#include <VTTranslation.h>

extern ConsoleInput g_winport_con_in;

static bool IsEnhancedKey(WORD code)
{
	return (code==VK_LEFT || code==VK_RIGHT || code==VK_UP || code==VK_DOWN
		|| code==VK_HOME || code==VK_END);// || code==VK_PAGEDOWN || code==VK_PAGEUP );
}


TTYInput::TTYInput()
{
	for (int controls = 0; controls < 4; ++controls) { // 0 ctrl alt shift
	for (WORD key_code = 1; key_code <= 0xff; ++key_code) {

		const char *csi = VT_TranslateSpecialKey(key_code, controls == 1, controls == 2, controls == 3, 0);
		if (csi != nullptr && *csi) {
			Key k = {};
			k.key_code = key_code;
			switch (controls) {
				case 1: k.control_keys = LEFT_CTRL_PRESSED; break;
				case 2: k.control_keys = LEFT_ALT_PRESSED; break;
				case 3: k.control_keys = SHIFT_PRESSED; break;
			}
			if (IsEnhancedKey(key_code))
				k.control_keys = ENHANCED_KEY;
			if (csi[1]) {
				_csi2key.emplace(csi, k);

			} else {
				_spec_char2key.emplace(csi[0], k);
			}
		}
	} }
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

void TTYInput::PostKeyEvent(const Key &k)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wRepeatCount = 1;
//	ir.Event.KeyEvent.uChar.UnicodeChar = i.second.unicode_char;
	ir.Event.KeyEvent.wVirtualKeyCode = k.key_code;
	ir.Event.KeyEvent.dwControlKeyState = k.control_keys;
	ir.Event.KeyEvent.wVirtualScanCode = WINPORT(MapVirtualKey)(k.key_code,MAPVK_VK_TO_VSC);
	ir.Event.KeyEvent.bKeyDown = TRUE;
	g_winport_con_in.Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in.Enqueue(&ir, 1);
}

bool TTYInput::BufParseIterationSimple()
{
	const auto &spec_i = _spec_char2key.find(_buf[0]);
	if (spec_i != _spec_char2key.end()) {
		PostKeyEvent(spec_i->second);
		_buf.erase(_buf.begin());
		return true;
	}

	const UTF8* utf8_start = (const UTF8*) &_buf[0];
	UTF32 utf32[2] = {}, *utf32_start = &utf32[0];
	ConvertUTF8toUTF32 ( &utf8_start, utf8_start + _buf.size(),
		&utf32_start, utf32_start + 1, lenientConversion);
	if (utf32_start != &utf32[0]) {
		PostCharEvent(utf32[0]);
		_buf.erase(_buf.begin(), _buf.begin() + (utf8_start - (const UTF8*)&_buf[0]) );
		return true;

	} else if (_buf.size() > 6) {
		fprintf(stderr, "Failed to decode UTF8 sequence: 0x%x '%c'\n", _buf[0], _buf[0]);
		_buf.erase(_buf.begin());
		return true;
	}

	return false;
}

bool TTYInput::BufParseIterationCSI()
{
//	abort();
	// TODO: optimized lookup
	if (_buf.size() > 1 && _buf[1] == '\x1b' ) {
		_buf.erase(_buf.begin(), _buf.begin() + 2);
		Key k = {};
		k.key_code = VK_ESCAPE;
		PostKeyEvent(k);
		return true;
	}
	size_t max_len = 0;
	for (const auto &i : _csi2key) {
		if (_buf.size() >= i.first.size() && memcmp(&_buf[0], i.first.c_str(), i.first.size()) == 0) {
//			fprintf(stderr, "RECOGNIZED: '%s'\n", i.first.c_str() + 1);

			_buf.erase(_buf.begin(), _buf.begin() + i.first.size());
			PostKeyEvent(i.second);
			return true;
		}
		if (max_len < i.first.size())
			max_len = i.first.size();
	}
	if (_buf.size() >= max_len) {
		fprintf(stderr, "NOT RECOGNIZED: '%s'\n", &_buf[1]);
		_buf.erase(_buf.begin());//TODO
		return true;
	}


	return false;
}

void TTYInput::OnBufUpdated()
{
	while (!_buf.empty()) {

		if (_buf[0] == '\x1b') {
//			fprintf(stderr, "!!!CSI [%x]\n", _buf[0]);
			if (!BufParseIterationCSI()) {
				break;
			}

		} else if (!BufParseIterationSimple()) {
			break;
		}
	}
}

void TTYInput::OnChar(char c)
{
	_buf.emplace_back(c);
	OnBufUpdated();
}
