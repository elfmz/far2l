#include <assert.h>
#include <base64.h>
#include <utils.h>
#include "VTFar2lExtensios.h"


// Potential security issue workaround: do not allow remote to read clipboard unless user
// recently made input that looks like clipboard paste request, like Ctrl+V hotkey.
// Here is expiration period that tells how long time clipboard considered accessible
// after such input processed.
#define CLIPBOARD_READ_ALLOWANCE_EXPIRATION_MSEC    5000

// Each allowed clipboard read request prolongs allowance expiration, here is limit of such prolongs
#define CLIPBOARD_READ_ALLOWANCE_PROLONGS           3

VTFar2lExtensios::VTFar2lExtensios(IVTAnsiCommands *ansi_commands)
	: _ansi_commands(ansi_commands)
{
}

VTFar2lExtensios::~VTFar2lExtensios()
{
	for (;_clipboard_opens > 0; --_clipboard_opens) {
		WINPORT(CloseClipboard)();
	}
}

void VTFar2lExtensios::WriteFar2lEvent(char code, uint32_t argc, ...)
{
	_tmp_far2l_event = "\x1b_f2l";
	if (code) {
		std::vector<unsigned char> buf;
		buf.push_back((unsigned char)code);
		va_list va;
		va_start(va, argc);
		for (; argc; --argc) {
			uint32_t a = va_arg(va, uint32_t);
			buf.resize(buf.size() + sizeof(a));
			memcpy(&buf[buf.size() - sizeof(a)], &a, sizeof(a));
		}
		va_end(va);

		_tmp_far2l_event+= base64_encode(&buf[0], buf.size());

	} else {
		assert(argc == 0);
	}
	_tmp_far2l_event+= '\x07';

	_ansi_commands->WriteRawInput(_tmp_far2l_event.c_str());
}

bool VTFar2lExtensios::IsAllowedClipboardRead()
{
	const DWORD now = WINPORT(GetTickCount)();
	if (now >= _clipboard_read_allowance)
		return (now - _clipboard_read_allowance) < CLIPBOARD_READ_ALLOWANCE_EXPIRATION_MSEC;

	return ((0xffffffff - _clipboard_read_allowance) + now) < CLIPBOARD_READ_ALLOWANCE_EXPIRATION_MSEC;
}

void VTFar2lExtensios::AllowClipboardRead(bool prolong)
{
	if (prolong) {
		if (_clipboard_read_allowance_prolongs >= CLIPBOARD_READ_ALLOWANCE_PROLONGS)
			return;

		++_clipboard_read_allowance_prolongs;

	} else {
		_clipboard_read_allowance_prolongs = 0;
	}
	_clipboard_read_allowance = WINPORT(GetTickCount)();
}

///

bool VTFar2lExtensios::OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent)
{
	if (MouseEvent.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) {
		AllowClipboardRead(false);
	}

	WriteFar2lEvent('M', 5, MouseEvent.dwMousePosition.X, MouseEvent.dwMousePosition.Y,
		MouseEvent.dwButtonState, MouseEvent.dwControlKeyState, MouseEvent.dwEventFlags);
	return true;
}

bool VTFar2lExtensios::OnInputKey(const KEY_EVENT_RECORD &KeyEvent)
{
	const bool ctrl = (KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) != 0;
	const bool alt = (KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED)) != 0;
	const bool shift = (KeyEvent.dwControlKeyState & (SHIFT_PRESSED)) != 0;

	if (KeyEvent.bKeyDown) {
		if (ctrl && alt && !shift && KeyEvent.wVirtualKeyCode == 'C') {
			// triple cltr+alt+c looks like red alert, let host kill shell
			if (++_ctrl_alt_c_counter == 3) {
				return false;
			}

		} else if (KeyEvent.wVirtualKeyCode != VK_SHIFT
			&& KeyEvent.wVirtualKeyCode != VK_MENU
			&& KeyEvent.wVirtualKeyCode != VK_CONTROL
			&& KeyEvent.wVirtualKeyCode != VK_RCONTROL) {
			_ctrl_alt_c_counter = 0;
		}

		if ((!alt && ctrl && KeyEvent.wVirtualKeyCode == 'V')
		|| (!alt && !ctrl && shift && KeyEvent.wVirtualKeyCode == VK_INSERT)){
			AllowClipboardRead(false);
		}
	}

	WriteFar2lEvent(KeyEvent.bKeyDown ? 'K' : 'k', 5, KeyEvent.wRepeatCount, KeyEvent.wVirtualKeyCode,
		KeyEvent.wVirtualScanCode, KeyEvent.uChar.UnicodeChar, KeyEvent.dwControlKeyState);
	return true;
}


void VTFar2lExtensios::OnInterract_ClipboardOpen(StackSerializer &stk_ser)
{
	char out = WINPORT(OpenClipboard)(NULL) ? 1 : 0;
	if (out)
		++_clipboard_opens;

	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

void VTFar2lExtensios::OnInterract_ClipboardClose(StackSerializer &stk_ser)
{
	char out = WINPORT(CloseClipboard)() ? 1 : 0;
	if (out)
		--_clipboard_opens;

	stk_ser.Clear();
	stk_ser.PushPOD(out);

	_clipboard_read_allowance_prolongs = 0;
}

void VTFar2lExtensios::OnInterract_ClipboardEmpty(StackSerializer &stk_ser)
{
	char out = WINPORT(EmptyClipboard)() ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

void VTFar2lExtensios::OnInterract_ClipboardIsFormatAvailable(StackSerializer &stk_ser)
{
	UINT fmt;
	stk_ser.PopPOD(fmt);
	char out = WINPORT(IsClipboardFormatAvailable)(fmt) ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

void VTFar2lExtensios::OnInterract_ClipboardSetData(StackSerializer &stk_ser)
{
	UINT fmt;
	uint32_t len;
	void *data;

	stk_ser.PopPOD(fmt);
	stk_ser.PopPOD(len);
	if (len) {
		data = malloc(len);
		stk_ser.Pop(data, len);
	} else
		data = nullptr;

	char out = WINPORT(SetClipboardData)(fmt, data) ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushPOD(out);

}

void VTFar2lExtensios::OnInterract_ClipboardGetData(StackSerializer &stk_ser)
{
	bool allowed = IsAllowedClipboardRead();

	UINT fmt;
	stk_ser.PopPOD(fmt);
	void *ptr = allowed ? WINPORT(GetClipboardData)(fmt) : nullptr;
	stk_ser.Clear();
	uint32_t len = ptr ? GetMallocSize(ptr) : 0;
	if (len)
		stk_ser.Push(ptr, len);
	stk_ser.PushPOD(len);

	if (allowed) { // prolong allowance
		AllowClipboardRead(true);
	}
}

void VTFar2lExtensios::OnInterract_ClipboardRegisterFormat(StackSerializer &stk_ser)
{
	const std::wstring &fmt_name = StrMB2Wide(stk_ser.PopStr());
	UINT out = WINPORT(RegisterClipboardFormat)(fmt_name.c_str());
	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

///////////

void VTFar2lExtensios::OnInterract_Clipboard(StackSerializer &stk_ser)
{
	const char code = stk_ser.PopChar();

	switch (code) {
		case 'o': OnInterract_ClipboardOpen(stk_ser); break;
		case 'c': OnInterract_ClipboardClose(stk_ser); break;
		case 'e': OnInterract_ClipboardEmpty(stk_ser); break;
		case 'a': OnInterract_ClipboardIsFormatAvailable(stk_ser); break;
		case 's': OnInterract_ClipboardSetData(stk_ser); break;
		case 'g': OnInterract_ClipboardGetData(stk_ser); break;
		case 'r': OnInterract_ClipboardRegisterFormat(stk_ser); break;

		default:
			fprintf(stderr, "OnInterract_Clipboard: wrong code %c\n", code);
	}
}

void VTFar2lExtensios::OnInterract_ChangeCursorHeigth(StackSerializer &stk_ser)
{
	UCHAR h;
	stk_ser.PopPOD(h);
	CONSOLE_CURSOR_INFO cci;
	if (WINPORT(GetConsoleCursorInfo)(NULL, &cci)) {
		cci.dwSize = h;
		WINPORT(SetConsoleCursorInfo)(NULL, &cci);
	}
}

void VTFar2lExtensios::OnInterract(StackSerializer &stk_ser)
{
	const char code = stk_ser.PopChar();

	switch (code) {
		case 't': {
			std::string title;
			stk_ser.PopStr(title);
			WINPORT(SetConsoleTitle)( StrMB2Wide(title).c_str() );
			stk_ser.Clear();
		} break;

		case 'e':
			WINPORT(BeginConsoleAdhocQuickEdit)();
			stk_ser.Clear();
		break;

		case 'M':
			WINPORT(SetConsoleWindowMaximized)(TRUE);
			stk_ser.Clear();
		break;

		case 'm':
			WINPORT(SetConsoleWindowMaximized)(FALSE);
			stk_ser.Clear();
		break;

		case 'c':
			OnInterract_Clipboard(stk_ser);
		break;

		case 'h':
			OnInterract_ChangeCursorHeigth(stk_ser);
		break;

		default:
			stk_ser.Clear();
	}
}

