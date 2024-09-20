#include <base64.h>
#include <crc64.h>
#include <utils.h>
#include <UtfConvert.hpp>
#include <fcntl.h>
#include "../WinPort/src/SavedScreen.h"
#include "../WinPort/FarTTY.h"

#include "VTFar2lExtensios.h"
#include "headers.hpp"
#include "scrbuf.hpp"
#include "lang.hpp"
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

static uint64_t CalculateDataID(UINT fmt, const void *data, uint32_t len)
{
	if (!data) {
		len = 0;

	} else if (fmt == CF_UNICODETEXT) {
		len = wcsnlen((const wchar_t *)data, len / sizeof(wchar_t)) * sizeof(wchar_t);

	} else if (fmt == CF_TEXT) {
		len = strnlen((const char *)data, len);
	}
	uint64_t id = len;
	if (len) {
		id = crc64(id, (const unsigned char*)data, len);
	}
	if (id == 0) { // zero means failure, but its not failed
		id = 1;
	}
	return id;
}

///

VTFar2lExtensios::VTFar2lExtensios(IVTShell *vt_shell, const std::string &host_id)
	: _vt_shell(vt_shell), _client_id_prefix(host_id)
{
	// host_id passed by NetRocks representing client identity (user@host) and
	// used together with clipboard client_id to guard against spoofed client_id
	// in case malicious client was somehow able to steal id of some other client
	if (!_client_id_prefix.empty()) {
		for (auto &c : _client_id_prefix) {
			if (((unsigned char)c) < 32) {
				c = 32;
			}
		}
		_client_id_prefix+= ':';
	}
}

VTFar2lExtensios::~VTFar2lExtensios()
{
	for (;_clipboard_opens > 0; --_clipboard_opens) {
		WINPORT(CloseClipboard)();
	}
	WINPORT(SetConsoleFKeyTitles)(_vt_shell->ConsoleHandle(), NULL);
}

void VTFar2lExtensios::WriteInputEvent(const StackSerializer &stk_ser)
{
	//fprintf(stderr, "VTFar2lExtensios::WriteInputEvent: %s\n", stk_ser.ToBase64().c_str());
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

void VTFar2lExtensios::OnTerminalResized()
{
	if ( (_xfeatures & FARTTY_FEAT_TERMINAL_SIZE) != 0) {
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)( NULL, &csbi )
					&& csbi.dwSize.X && csbi.dwSize.Y) {
			StackSerializer stk_ser;
			stk_ser.PushNum((uint16_t)csbi.dwSize.X);
			stk_ser.PushNum((uint16_t)csbi.dwSize.Y);
			stk_ser.PushNum(FARTTY_INPUT_TERMINAL_SIZE);
			WriteInputEvent(stk_ser);
		}
	}
}

bool VTFar2lExtensios::OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent)
{
	if (MouseEvent.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) {
		AllowClipboardRead(false);
	}

	StackSerializer stk_ser;
	stk_ser.PushNum(MouseEvent.dwMousePosition.X);
	stk_ser.PushNum(MouseEvent.dwMousePosition.Y);
	if ( (_xfeatures & FARTTY_FEAT_COMPACT_INPUT) != 0
			&& (MouseEvent.dwButtonState & 0xff00ff00) == 0
			&& MouseEvent.dwControlKeyState < 0x100
			&& MouseEvent.dwEventFlags < 0x100) {
		const uint16_t btn_state_encoded = ((MouseEvent.dwButtonState & 0xff)
			| ((MouseEvent.dwButtonState >> 8) & 0xff00));
		stk_ser.PushNum(btn_state_encoded);
		stk_ser.PushNum(uint8_t(MouseEvent.dwControlKeyState));
		stk_ser.PushNum(uint8_t(MouseEvent.dwEventFlags));
		stk_ser.PushNum(FARTTY_INPUT_MOUSE_COMPACT);
		//fprintf(stderr, "VTFar2lExtensios::OnInputMouse: compact\n");

	} else {
		stk_ser.PushNum(MouseEvent.dwButtonState);
		stk_ser.PushNum(MouseEvent.dwControlKeyState);
		stk_ser.PushNum(MouseEvent.dwEventFlags);
		stk_ser.PushNum(FARTTY_INPUT_MOUSE);
		//fprintf(stderr, "VTFar2lExtensios::OnInputMouse: normal\n");
	}
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
	if ((_xfeatures & FARTTY_FEAT_COMPACT_INPUT) != 0
			&& KeyEvent.wRepeatCount <= 1 && KeyEvent.wVirtualScanCode != RIGHT_SHIFT_VSC
			&& ((uint32_t)KeyEvent.uChar.UnicodeChar) < 0x10000
			&& KeyEvent.dwControlKeyState < 0x10000
			&& KeyEvent.wVirtualKeyCode < 0x100) {
		stk_ser.PushNum((uint8_t)KeyEvent.wVirtualKeyCode);
		stk_ser.PushNum((uint16_t)KeyEvent.dwControlKeyState);
		stk_ser.PushNum((uint16_t)(uint32_t)KeyEvent.uChar.UnicodeChar);
		stk_ser.PushNum((KeyEvent.bKeyDown ? FARTTY_INPUT_KEYDOWN_COMPACT: FARTTY_INPUT_KEYUP_COMPACT));
		//fprintf(stderr, "VTFar2lExtensios::OnInputKey: compact\n");

	} else {
		stk_ser.PushNum(KeyEvent.wRepeatCount);
		stk_ser.PushNum(KeyEvent.wVirtualKeyCode);
		stk_ser.PushNum(KeyEvent.wVirtualScanCode);
		stk_ser.PushNum(KeyEvent.dwControlKeyState);
		stk_ser.PushNum((uint32_t)KeyEvent.uChar.UnicodeChar);
		stk_ser.PushNum((KeyEvent.bKeyDown ? FARTTY_INPUT_KEYDOWN : FARTTY_INPUT_KEYUP));
		//fprintf(stderr, "VTFar2lExtensios::OnInputKey: normal\n");
	}

	WriteInputEvent(stk_ser);

	return true;
}

char VTFar2lExtensios::ClipboardAuthorize(std::string client_id)
{
	if (client_id.size() < 0x20 || client_id.size() > 0x100)
		return 0;

	for (const auto &c : client_id) {
		if ( (c < '0' || c > '9') && (c < 'a' || c > 'z') && c != '-' && c != '_')
			return 0;
	}

	client_id.insert(0, _client_id_prefix);

	if (_autheds.find(client_id) != _autheds.end())
		return 1;

	const std::string &autheds_file = InMyConfig("tty_clipboard/autheds");
	if (ListFileContains(autheds_file, client_id)) {
		_autheds.insert(client_id);
		return 1;
	}

	int choice;

	{
		SavedScreen saved_scr;
		ScrBuf.FillBuf();
		do { // prevent quick thoughtless tap Enter or Space or Esc in dialog
			choice = Message(MSG_KEEPBACKGROUND, 5,
				Msg::TerminalClipboardAccessTitle,
				Msg::TerminalClipboardAccessText,
				L"...",	// 0 - stub select for thoughtless tap
				Msg::TerminalClipboardAccessBlock,		// 1
				Msg::TerminalClipboardAccessTemporaryRemote,	// 2
				Msg::TerminalClipboardAccessTemporaryLocal,	// 3
				Msg::TerminalClipboardAccessAlwaysLocal);	// 4
		} while( choice <= 0 );
	}

	_vt_shell->OnTerminalResized(); // window could resize during dialog box processing

	switch (choice) {
		case 4: // Always allow access to local clipboard
			ListFileAppend(autheds_file, client_id);
			// fall through

		case 3: // Temporary allow access to local clipboared
			_autheds.insert(client_id);
			return 1;

		case 2: // Tell client that he need to use own clipboard
			return -1;

		default: // Mimic just failed to open clipboard
			return 0;
	}
}

void VTFar2lExtensios::OnInteract_ClipboardOpen(StackSerializer &stk_ser)
{
	_clipboard_chunks.clear();
	const std::string &client_id = stk_ser.PopStr();
	char out = ClipboardAuthorize(client_id);
	if (out == 1) {
		out = WINPORT(OpenClipboard)(NULL) ? 1 : 0;
		if (out)
			++_clipboard_opens;
	}

	stk_ser.Clear();
	// report supported features
	stk_ser.PushNum(uint64_t(FARTTY_FEATCLIP_DATA_ID | FARTTY_FEATCLIP_CHUNKED_SET));
	stk_ser.PushNum(out);
}

void VTFar2lExtensios::OnInteract_ClipboardClose(StackSerializer &stk_ser)
{
	_clipboard_chunks.clear();
	_clipboard_chunks.shrink_to_fit();

	char out = -1;
	if (_clipboard_opens > 0) {
		out = WINPORT(CloseClipboard)() ? 1 : 0;
		if (out)
			--_clipboard_opens;
	}

	stk_ser.Clear();
	stk_ser.PushNum(out);

	_clipboard_read_allowance_prolongs = 0;
}

void VTFar2lExtensios::OnInteract_ClipboardEmpty(StackSerializer &stk_ser)
{
	char out = -1;
	if (_clipboard_opens > 0) {
		out = WINPORT(EmptyClipboard)() ? 1 : 0;
	}
	stk_ser.Clear();
	stk_ser.PushNum(out);
}

void VTFar2lExtensios::OnInteract_ClipboardIsFormatAvailable(StackSerializer &stk_ser)
{
	UINT fmt;
	stk_ser.PopNum(fmt);
	char out = WINPORT(IsClipboardFormatAvailable)(fmt) ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushNum(out);
}

void VTFar2lExtensios::OnInteract_ClipboardSetDataChunk(StackSerializer &stk_ser)
{
	if (_clipboard_opens > 0) {
		uint16_t encoded_chunk_size = 0;
		stk_ser.PopNum(encoded_chunk_size);
		if (encoded_chunk_size) {
			const size_t chunk_size = size_t(encoded_chunk_size) << 8;
			const size_t prev_size = _clipboard_chunks.size();
			_clipboard_chunks.resize(prev_size + chunk_size);
			if (_clipboard_chunks.size() < prev_size) {
				throw std::runtime_error("Chunked clipboard data overflow");
			}
			stk_ser.Pop(_clipboard_chunks.data() + prev_size, chunk_size);

		} else { // zero chunk length means discard pending chunks
			_clipboard_chunks.clear();
		}
	}
	stk_ser.Clear();
}

void VTFar2lExtensios::OnInteract_ClipboardSetData(StackSerializer &stk_ser)
{
	char out = -1;
	uint64_t id = 0;
	if (_clipboard_opens > 0) {
		UINT fmt;
		uint32_t len;
		unsigned char *data;

		stk_ser.PopNum(fmt);
		stk_ser.PopNum(len);
		if (len && len + uint32_t(_clipboard_chunks.size()) >= len) {
			data = (unsigned char *)WINPORT(ClipboardAlloc)(len + uint32_t(_clipboard_chunks.size()));
			if (data) {
				memcpy(data, _clipboard_chunks.data(), uint32_t(_clipboard_chunks.size()));
				stk_ser.Pop(data + uint32_t(_clipboard_chunks.size()), len);
				len+= uint32_t(_clipboard_chunks.size());
#if (__WCHAR_MAX__ <= 0xffff)
				if (fmt == CF_UNICODETEXT) { // UTF32 -> UTF16
					UtfConverter<uint32_t, uint16_t> cvt((const uint32_t *)data, len / sizeof(uint32_t));
					void *new_data = ClipboardAllocFromVector(cvt, len);
					if (new_data) {
						WINPORT(ClipboardFree)(data);
						data = new_data;
					}
				}
#endif
			}
		} else
			data = nullptr;

		out = WINPORT(SetClipboardData)(fmt, data) ? 1 : 0;
		if (out == 1) {
			id = CalculateDataID(fmt, data, WINPORT(ClipboardSize)(data));
		}
	}
	stk_ser.Clear();
	if (out == 1) {
		stk_ser.PushNum(id);
	}
	stk_ser.PushNum(out);
	_clipboard_chunks.clear();
}

void VTFar2lExtensios::OnInteract_ClipboardGetData(StackSerializer &stk_ser)
{
	if (_clipboard_opens > 0) {
		bool allowed = IsAllowedClipboardRead();

		UINT fmt;
		stk_ser.PopNum(fmt);
		stk_ser.Clear();
		void *data = allowed ? WINPORT(GetClipboardData)(fmt) : nullptr;
		uint64_t id = 0;
		uint32_t len = 0;
		if (data) {
			len = WINPORT(ClipboardSize)(data);
			id = CalculateDataID(fmt, data, len);
		}
		stk_ser.PushNum(id);
		if (len) {
#if (__WCHAR_MAX__ <= 0xffff)
			void *new_data = nullptr;
			if (fmt == CF_UNICODETEXT) { // UTF16 -> UTF32
				UtfConverter<uint16_t, uint32_t> cvt((const uint16_t *)data, len / sizeof(uint16_t));
				new_data = ClipboardAllocFromVector(cvt, len);
			}
			stk_ser.Push(new_data ? new_data : data, len);
			WINPORT(ClipboardFree)(new_data);
#else
			stk_ser.Push(data, len);
#endif
		}
		stk_ser.PushNum(len);

		if (allowed) { // prolong allowance
			AllowClipboardRead(true);
		}
	} else {
		stk_ser.PushNum((uint32_t)-1);
	}
}

void VTFar2lExtensios::OnInteract_ClipboardGetDataID(StackSerializer &stk_ser)
{
	uint64_t id = 0;
	if (_clipboard_opens > 0 && IsAllowedClipboardRead()) {
		UINT fmt = 0;
		stk_ser.PopNum(fmt);
		void *data = WINPORT(GetClipboardData)(fmt);
		if (data) {
			id = CalculateDataID(fmt, data, WINPORT(ClipboardSize)(data));
		}
	}

	stk_ser.Clear();
	stk_ser.PushNum(id);
}

void VTFar2lExtensios::OnInteract_ClipboardRegisterFormat(StackSerializer &stk_ser)
{
	const std::wstring &fmt_name = StrMB2Wide(stk_ser.PopStr());
	UINT out = WINPORT(RegisterClipboardFormat)(fmt_name.c_str());
	stk_ser.Clear();
	stk_ser.PushNum(out);
}

///////////

void VTFar2lExtensios::OnInteract_Clipboard(StackSerializer &stk_ser)
{
	const char code = stk_ser.PopChar();

	switch (code) {
		case FARTTY_INTERACT_CLIP_OPEN: OnInteract_ClipboardOpen(stk_ser); break;
		case FARTTY_INTERACT_CLIP_CLOSE: OnInteract_ClipboardClose(stk_ser); break;
		case FARTTY_INTERACT_CLIP_EMPTY: OnInteract_ClipboardEmpty(stk_ser); break;
		case FARTTY_INTERACT_CLIP_ISAVAIL: OnInteract_ClipboardIsFormatAvailable(stk_ser); break;
		case FARTTY_INTERACT_CLIP_SETDATACHUNK: OnInteract_ClipboardSetDataChunk(stk_ser); break;
		case FARTTY_INTERACT_CLIP_SETDATA: OnInteract_ClipboardSetData(stk_ser); break;
		case FARTTY_INTERACT_CLIP_GETDATA: OnInteract_ClipboardGetData(stk_ser); break;
		case FARTTY_INTERACT_CLIP_GETDATAID: OnInteract_ClipboardGetDataID(stk_ser); break;
		case FARTTY_INTERACT_CLIP_REGISTER_FORMAT: OnInteract_ClipboardRegisterFormat(stk_ser); break;

		default:
			fprintf(stderr, "OnInteract_Clipboard: wrong code %c\n", code);
	}
}

void VTFar2lExtensios::OnInteract_ChangeCursorHeight(StackSerializer &stk_ser)
{
	UCHAR h;
	stk_ser.PopNum(h);
	CONSOLE_CURSOR_INFO cci;
	if (WINPORT(GetConsoleCursorInfo)(NULL, &cci)) {
		cci.dwSize = h;
		WINPORT(SetConsoleCursorInfo)(NULL, &cci);
	}
}

void VTFar2lExtensios::OnInteract_GetLargestWindowSize(StackSerializer &stk_ser)
{
	COORD sz = WINPORT(GetLargestConsoleWindowSize)(NULL);
	stk_ser.Clear();
	stk_ser.PushNum(sz.X);
	stk_ser.PushNum(sz.Y);
}

void VTFar2lExtensios::OnInteract_DisplayNotification(StackSerializer &stk_ser)
{
	const std::string &title = stk_ser.PopStr();
	const std::string &text = stk_ser.PopStr();
	DisplayNotification(title.c_str(), text.c_str());
	stk_ser.Clear();
}

void VTFar2lExtensios::OnInteract_SetFKeyTitles(StackSerializer &stk_ser)
{
	std::string titles_str[CONSOLE_FKEYS_COUNT];
	const char *titles[ARRAYSIZE(titles_str)] {};

	for (unsigned int i = 0; i < ARRAYSIZE(titles_str) && !stk_ser.IsEmpty(); ++i) {
		unsigned char state = 0;
		stk_ser.PopNum(state);
		if (state != 0) {
			stk_ser.PopStr(titles_str[i]);
			titles[i] = titles_str[i].c_str();
		}
	}

	bool out = WINPORT(SetConsoleFKeyTitles)(_vt_shell->ConsoleHandle(), titles) != FALSE;

	stk_ser.Clear();
	stk_ser.PushNum(out);
}

void VTFar2lExtensios::OnInteract_GetColorPalette(StackSerializer &stk_ser)
{
	stk_ser.Clear();
	const uint8_t bits = WINPORT(GetConsoleColorPalette)(_vt_shell->ConsoleHandle());
	const uint8_t reserved = 0;
	stk_ser.PushNum(reserved);
	stk_ser.PushNum(bits);
}

void VTFar2lExtensios::OnInteract(StackSerializer &stk_ser)
{
	const char code = stk_ser.PopChar();

	switch (code) {
		case FARTTY_INTERACT_CHOOSE_EXTRA_FEATURES:
			_xfeatures = 0;
			stk_ser.PopNum(_xfeatures);
			stk_ser.Clear();
			if (_xfeatures & FARTTY_FEAT_TERMINAL_SIZE) {
				OnTerminalResized();
			}
		break;

		case FARTTY_INTERACT_CONSOLE_ADHOC_QEDIT:
			WINPORT(BeginConsoleAdhocQuickEdit)();
			stk_ser.Clear();
		break;

		case FARTTY_INTERACT_WINDOW_MAXIMIZE:
			WINPORT(SetConsoleWindowMaximized)(TRUE);
			stk_ser.Clear();
		break;

		case FARTTY_INTERACT_WINDOW_RESTORE:
			WINPORT(SetConsoleWindowMaximized)(FALSE);
			stk_ser.Clear();
		break;

		case FARTTY_INTERACT_CLIPBOARD:
			OnInteract_Clipboard(stk_ser);
		break;

		case FARTTY_INTERACT_SET_CURSOR_HEIGHT:
			OnInteract_ChangeCursorHeight(stk_ser);
		break;

		case FARTTY_INTERACT_GET_WINDOW_MAXSIZE:
			OnInteract_GetLargestWindowSize(stk_ser);
		break;

		case FARTTY_INTERACT_DESKTOP_NOTIFICATION:
			OnInteract_DisplayNotification(stk_ser);
		break;

		case FARTTY_INTERACT_SET_FKEY_TITLES:
			OnInteract_SetFKeyTitles(stk_ser);
		break;

		case FARTTY_INTERACT_GET_COLOR_PALETTE:
			OnInteract_GetColorPalette(stk_ser);
		break;

		default:
			stk_ser.Clear();
	}
}

