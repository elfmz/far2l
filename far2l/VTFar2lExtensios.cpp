#include <assert.h>
#include <base64.h>
#include <utils.h>
#include <fcntl.h>
#include "../WinPort/src/SavedScreen.h"

#include "VTFar2lExtensios.h"
#include "headers.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "dialog.hpp"
#include "message.hpp"
#include "mix.hpp"


// Potential security issue workaround: do not allow remote to read clipboard unless user
// recently made input that looks like clipboard paste request, like Ctrl+V hotkey.
// Here is expiration period that tells how long time clipboard considered accessible
// after such input processed.
#define CLIPBOARD_READ_ALLOWANCE_EXPIRATION_MSEC    5000

// Each allowed clipboard read request prolongs allowance expiration, here is limit of such prolongs
#define CLIPBOARD_READ_ALLOWANCE_PROLONGS           3


static bool ListFileContains(const std::string &filename, const std::string &line)
{
	FILE *f = fopen(filename.c_str(), "r");
	if (!f)
		return false;

	for (;;) {
		char buf[0x404] = {};
		if (!fgets(buf, sizeof(buf) - 1, f)) {
			fclose(f);
			return false;
		}

		size_t l = strlen(buf);
		while (l > 0 && (buf[l - 1] == '\r' || buf[l - 1] == '\n')) {
			--l;
		}

		if (l == line.size() && memcmp(buf, line.c_str(), l) == 0) {
			fclose(f);
			return true;
		}
	}
}

static void ListFileAppend(const std::string &filename, std::string line)
{
	line+= '\n';
	int fd = open(filename.c_str(), O_APPEND | O_WRONLY | O_CREAT, 0600);
	if (fd != -1) {
		if (write(fd, line.c_str(), line.size()) != (ssize_t)line.size()) {
			perror("ListFileAppend - write");
		}
		close(fd);
	} else {
		perror("ListFileAppend - open");
	}

}

///

VTFar2lExtensios::VTFar2lExtensios(IVTShell *vt_shell)
	: _vt_shell(vt_shell)
{
}

VTFar2lExtensios::~VTFar2lExtensios()
{
	for (;_clipboard_opens > 0; --_clipboard_opens) {
		WINPORT(CloseClipboard)();
	}
}

void VTFar2lExtensios::WriteInputEvent(const StackSerializer &stk_ser)
{
	_tmp_input_event = "\x1b_f2l";
	_tmp_input_event+= stk_ser.ToBase64();
	_tmp_input_event+= '\x07';
	_vt_shell->InjectInput(_tmp_input_event.c_str());
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

	StackSerializer stk_ser;
	stk_ser.PushPOD(MouseEvent.dwMousePosition.X);
	stk_ser.PushPOD(MouseEvent.dwMousePosition.Y);
	stk_ser.PushPOD(MouseEvent.dwButtonState);
	stk_ser.PushPOD(MouseEvent.dwControlKeyState);
	stk_ser.PushPOD(MouseEvent.dwEventFlags);
	stk_ser.PushPOD('M');
	WriteInputEvent(stk_ser);
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

	StackSerializer stk_ser;
	stk_ser.PushPOD(KeyEvent.wRepeatCount);
	stk_ser.PushPOD(KeyEvent.wVirtualKeyCode);
	stk_ser.PushPOD(KeyEvent.wVirtualScanCode);
	stk_ser.PushPOD(KeyEvent.dwControlKeyState);
	stk_ser.PushPOD((uint32_t)KeyEvent.uChar.UnicodeChar);
	stk_ser.PushPOD((KeyEvent.bKeyDown ? 'K' : 'k'));
	WriteInputEvent(stk_ser);
	return true;
}

char VTFar2lExtensios::ClipboardAuthorize(const std::string &client_id)
{
	if (client_id.size() < 0x20 || client_id.size() > 0x100)
		return 0;

	for (const auto &c : client_id) {
		if ( (c < '0' || c > '9') && (c < 'a' || c > 'z') && c != '-' && c != '_')
			return 0;
	}

	if (_autheds.find(client_id) != _autheds.end())
		return 1;

	const std::string &autheds_file = InMyConfig("tty_clipboard/autheds");
	if (ListFileContains(autheds_file, client_id)) {
		_autheds.insert(client_id);
		return 1;
	}

	int choice;

	{
		const wchar_t *lines_wz[] = {
			MSG(MTerminalClipboardAccessText),
			MSG(MTerminalClipboardAccessBlock),		// 0
			MSG(MTerminalClipboardAccessTemporaryRemote),	// 1
			MSG(MTerminalClipboardAccessTemporaryLocal),	// 2
			MSG(MTerminalClipboardAccessAlwaysLocal)};	// 3
		SavedScreen saved_scr;
		choice = Message(MSG_KEEPBACKGROUND, 4,
			MSG(MTerminalClipboardAccessTitle),
			&lines_wz[0], sizeof(lines_wz) / sizeof(lines_wz[0]));
	}

	_vt_shell->OnTerminalResized(); // window could resize during dialog box processing

	switch (choice) {
		case 3: // Always allow access to local clipboard
			ListFileAppend(autheds_file, client_id);
			// fall through

		case 2: // Temporary allow access to local clipboared
			_autheds.insert(client_id);
			return 1;

		case 1: // Tell client that he need to use own clipboard
			return -1;

		default: // Mimic just failed to open clipboard
			return 0;
	}

}

void VTFar2lExtensios::OnInterract_ClipboardOpen(StackSerializer &stk_ser)
{
	std::string client_id;
	stk_ser.PopStr(client_id);
	char out = ClipboardAuthorize(client_id);
	if (out == 1) {
		out = WINPORT(OpenClipboard)(NULL) ? 1 : 0;
		if (out)
			++_clipboard_opens;
	}

	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

void VTFar2lExtensios::OnInterract_ClipboardClose(StackSerializer &stk_ser)
{
	char out = -1;
	if (_clipboard_opens > 0) {
		out = WINPORT(CloseClipboard)() ? 1 : 0;
		if (out)
			--_clipboard_opens;
	}

	stk_ser.Clear();
	stk_ser.PushPOD(out);

	_clipboard_read_allowance_prolongs = 0;
}

void VTFar2lExtensios::OnInterract_ClipboardEmpty(StackSerializer &stk_ser)
{
	char out = -1;
	if (_clipboard_opens > 0) {
		out = WINPORT(EmptyClipboard)() ? 1 : 0;
	}
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
	char out = -1;
	if (_clipboard_opens > 0) {
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

		out = WINPORT(SetClipboardData)(fmt, data) ? 1 : 0;
	}
	stk_ser.Clear();
	stk_ser.PushPOD(out);

}

void VTFar2lExtensios::OnInterract_ClipboardGetData(StackSerializer &stk_ser)
{
	if (_clipboard_opens > 0) {
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
	} else {
		stk_ser.PushPOD((uint32_t)-1);
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

void VTFar2lExtensios::OnInterract_GetLargestWindowSize(StackSerializer &stk_ser)
{
	COORD sz = WINPORT(GetLargestConsoleWindowSize)(NULL);
	stk_ser.Clear();
	stk_ser.PushPOD(sz);
}

void VTFar2lExtensios::OnInterract_DisplayNotification(StackSerializer &stk_ser)
{
	const std::string &title = stk_ser.PopStr();
	const std::string &text = stk_ser.PopStr();
	DisplayNotification(title.c_str(), text.c_str());
	stk_ser.Clear();
}

void VTFar2lExtensios::OnInterract(StackSerializer &stk_ser)
{
	const char code = stk_ser.PopChar();

	switch (code) {
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

		case 'w':
			OnInterract_GetLargestWindowSize(stk_ser);
		break;

		case 'n':
			OnInterract_DisplayNotification(stk_ser);
		break;

		default:
			stk_ser.Clear();
	}
}

