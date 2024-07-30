#include "TTYX.h"

#include "WinCompat.h"
#include <utils.h>
#include <Threaded.h>

#include <os_call.hpp>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#ifdef TTYXI
# include <X11/extensions/XInput2.h>
#endif

#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <chrono>
#include <assert.h>
#include <string>
#include <map>
#include <vector>
#include <memory>

#define INVALID_MODS                    0xffffffff

typedef std::map<std::string, std::vector<unsigned char> > Type2Data;

class TTYX
{
	Display *_display = nullptr;
	int _screen;
	Window _root_window;
	Window _window;
	XkbDescPtr _xkb_en = nullptr;
	Atom _targets_atom;
	Atom _text_atom;
	Atom _utf8_atom;
	Atom _clipboard_atom;
	Atom _xsel_data_atom;
	int _display_fd;

#ifdef TTYXI
	const std::chrono::time_point<std::chrono::steady_clock> _never;

	int _xi_opcode;
	bool _xi = false;
	DWORD _xi_leds = INVALID_MODS;
	std::map<KeySym, std::chrono::time_point<std::chrono::steady_clock>> _xi_keys;

	// Max time to wait for Xi keydown event if its not yet arrived upon TTY keypress
	unsigned int _xi_keydown_maxwait_msec = 100; // default value, can be increased on slow connections
	const unsigned int _xi_keydown_maxwait_msec_max = 3000; // max value on slow connections

	// Threshold of time since key modifier was pressed after which latch check has to be performed for it
	unsigned int _xi_modifier_check_trsh_msec = 500; // default value, can be increased on slow connections
	const unsigned int _xi_modifier_check_trsh_msec_max = 1000; // max value on slow connections

	// recently released non-modifier key used in workaround to handle
	// lagged TTY keypress arrived after Xi release due to slow connection
	KeySym _xi_recent_nonmod_keyup = 0;
#endif

	struct GetClipboardContext {
		std::vector<unsigned char> *data;
		Atom a;
		enum {
			MISSING,
			REQUESTING,
			PRESENT
		} state = MISSING;
	} _get_clipboard;

	std::unique_ptr<Type2Data> _set_clipboard;

	std::vector<std::pair<Atom, std::string> > _atoms;

	Atom CustomFormatAtom(const std::string &type)
	{
		for (const auto &it : _atoms) {
			if (it.second == type)
				return it.first;
		}
		Atom out = XInternAtom(_display, type.c_str(), 0);
		_atoms.emplace_back(out, type);
		return out;
	}

	std::string CustomFormatFromAtom(Atom a)
	{
		for (const auto &it : _atoms) {
			if (it.first == a)
				return it.second;
		}

		return std::string();
	}

#ifdef TTYXI
	static bool IsXiKeyModifier(KeySym ks)
	{
		switch (ks) {
			case XK_Control_L: case XK_Control_R:
			case XK_Shift_L: case XK_Shift_R:
			case XK_Alt_L: case XK_Alt_R:
			case XK_Super_L: case XK_Super_R:
			case XK_Hyper_L: case XK_Hyper_R:
			case XK_Meta_L: case XK_Meta_R:
			case XK_Caps_Lock: case XK_Shift_Lock:
			case XK_Num_Lock: case XK_Scroll_Lock:
#if 0
			case XK_Mode_switch: case XK_Multi_key: case XK_Codeinput:
			case XK_SingleCandidate: case XK_MultipleCandidate: case XK_PreviousCandidate:

			case XK_Kanji: case XK_Muhenkan: case XK_Henkan: case XK_Romaji:
			case XK_Hiragana: case XK_Katakana: case XK_Hiragana_Katakana:
			case XK_Zenkaku: case XK_Hankaku: case XK_Zenkaku_Hankaku:
			case XK_Touroku: case XK_Massyo: case XK_Kana_Lock:
			case XK_Kana_Shift: case XK_Eisu_Shift: case XK_Eisu_toggle:
#endif
				return true;

			default:
				return false;
		}
	}

	// wait for having any non-modifier key pressed for max of XI_KEYDOWN_MAXWAIT_MSEC
	void WaitForNonModifierXiKeydown()
	{
		for (unsigned int i = 0; i < _xi_keydown_maxwait_msec; i+= 10) {
			for (const auto &xki : _xi_keys) if (!IsXiKeyModifier(xki.first)) {
				return;
			}
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(_display_fd, &fds);

			struct timeval tv = {0, 10000}; // 10msec wait on each iteration
			select(_display_fd + 1, &fds, NULL, NULL, &tv);
			if (FD_ISSET(_display_fd, &fds)) {
				DispatchPendingEvents();
			}
		}
		fprintf(stderr, "TTYXi: WaitForNonModifierXiKeydown - timed out\n");
	}

	bool DispatchXiEvent(XEvent &event)
	{
		if (!_xi) {
			return false;
		}
		XGenericEventCookie *cookie = (XGenericEventCookie*)&event.xcookie;
		if (XGetEventData(_display, cookie) && cookie->type == GenericEvent && cookie->extension == _xi_opcode) {
			if ((cookie->evtype == XI_RawKeyPress || cookie->evtype == XI_RawKeyRelease) && cookie->data) {
				const XIRawEvent *ev = (const XIRawEvent *)cookie->data;
				//fprintf(stderr, "TTYXi: !!!!! %d %d\n", cookie->evtype == XI_RawKeyPress, ev->detail);

				if (!_xkb_en) {

					char keycodes[] = "evdev";
					char types[] = "complete";
					char compat[] = "complete";
					char symbols[] = "pc+us+inet(evdev)";

					XkbComponentNamesRec component_names = {0};
					component_names.keycodes = keycodes;
					component_names.types = types;
					component_names.compat = compat;
					component_names.symbols = symbols;

					_xkb_en = XkbGetKeyboardByName(
						_display, XkbUseCoreKbd, &component_names,
						XkbGBN_AllComponentsMask, XkbGBN_AllComponentsMask, False);

					XkbGetControls(_display, XkbGroupsWrapMask, _xkb_en);
					XkbGetNames(_display, XkbGroupNamesMask, _xkb_en);
				}

				KeySym ks;
				unsigned int mods;
				if (!_xkb_en || !XkbTranslateKeyCode(_xkb_en, ev->detail, 0, &mods, &ks)) {
					ks = XkbKeycodeToKeysym(_display, ev->detail, 0, 0); // fallback to old method
				}

				if (cookie->evtype == XI_RawKeyPress) {
					_xi_keys[ks] = std::chrono::steady_clock::now();

				} else {
					if (_xi_keys.erase(ks) == 0) {
						fprintf(stderr, "TTYXi: keyrelease for nonpressed 0x%lx\n", ks);
					}
					if (!IsXiKeyModifier(ks)) {
						_xi_recent_nonmod_keyup = ks;
					}
				}

				if (ks == XK_Num_Lock || ks == XK_Caps_Lock || ks == XK_Scroll_Lock) {
					_xi_leds = INVALID_MODS; // invalidate now, will update when needed
				}
			}
			return true;
		}
		return false;
	}

	DWORD GetXiLeds()
	{
		if (_xi_leds == INVALID_MODS) {
			_xi_leds = 0;
			XKeyboardState kbd_state{};
			XGetKeyboardControl(_display, &kbd_state);
			_xi_leds|= (kbd_state.led_mask & 1) ? SCROLLLOCK_ON : 0;
			_xi_leds|= (kbd_state.led_mask & 2) ? NUMLOCK_ON : 0;
			_xi_leds|= (kbd_state.led_mask & 4) ? CAPSLOCK_ON : 0;
		}

		return _xi_leds;
	}
#endif

	DWORD GetKeyModifiersByXQueryPointer()
	{
		Window root, child;
		int root_x, root_y;
		int win_x, win_y;
		unsigned int xmods = 0;

		XQueryPointer(_display, _root_window, &root, &child,
				&root_x, &root_y, &win_x, &win_y, &xmods);

		DWORD out = 0;
		if ((xmods & ControlMask) != 0) {
			out|= LEFT_CTRL_PRESSED;
		}
		if ((xmods & (Mod1Mask | Mod4Mask)) != 0) { // Mod1Mask - left Alt, Mod4Mask - left Win
			out|= LEFT_ALT_PRESSED;
		}
		if ((xmods & Mod5Mask) != 0) {
			out|= RIGHT_ALT_PRESSED;
		}
		if ((xmods & ShiftMask) != 0) {
			out|= SHIFT_PRESSED;
		}
		if ((xmods & LockMask) != 0) {
			out|= CAPSLOCK_ON;
		}
		return out;
	}

	DWORD GetKeyModifiers()
	{
		DWORD out = 0;
#ifdef TTYXI
		if (_xi) {
			auto now = _never;
			DWORD mods = INVALID_MODS;
			const auto CheckXiMod = [&](KeySym ks, DWORD mod, DWORD latch_check_mods)
			{
				auto it = _xi_keys.find(ks);
				if (it == _xi_keys.end()) {
					return;
				}

				// can't blindly use key state cause some control keys tend
				// to latch if pressed as part of system hotkey, like Alt+TAB
				// so check by XQueryPointer if modifier still down if it was
				// pressed rather long time ago.
				if (now == _never) {
					now = std::chrono::steady_clock::now();
				}

				const auto &delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
				if (delta > std::chrono::milliseconds(_xi_modifier_check_trsh_msec)) {
					if (mods == INVALID_MODS) {
						mods = GetKeyModifiersByXQueryPointer();
					}
					// XQueryPointer may not distinct LEFT/RIGHT for CTRL and ALT,
					// thats why checking against 'wider' latch_check_mods
					if ((mods & latch_check_mods) == 0) {
						fprintf(stderr, "TTYXi: latched keymod 0x%lx\n", ks);
						_xi_keys.erase(it);
						return;
					}
				}
				out|= mod;
			};

			CheckXiMod(XK_Control_L, LEFT_CTRL_PRESSED, LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
			CheckXiMod(XK_Control_R, RIGHT_CTRL_PRESSED, LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
			CheckXiMod(XK_Alt_L, LEFT_ALT_PRESSED, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
			CheckXiMod(XK_Super_L, LEFT_ALT_PRESSED, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
			CheckXiMod(XK_Alt_R, RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
			CheckXiMod(XK_Super_R, RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
			CheckXiMod(XK_Shift_L, SHIFT_PRESSED, SHIFT_PRESSED);
			CheckXiMod(XK_Shift_R, SHIFT_PRESSED, SHIFT_PRESSED);

			out|= GetXiLeds();

		} else
#endif
		{
			out|= GetKeyModifiersByXQueryPointer();
		}

		return out;
	}

	void DispatchSelectionNotify(XEvent &event)
	{
		if (event.xselection.property != None) {
			Atom target {};
			int format;
			unsigned long size, n;
			unsigned char* data = NULL;
			XGetWindowProperty(event.xselection.display,
				event.xselection.requestor, event.xselection.property,
				0L, (~0L), 0, AnyPropertyType, &target, &format, &size, &n, &data);
			if (target == _get_clipboard.a) {
				if (_get_clipboard.state == GetClipboardContext::REQUESTING) {
					_get_clipboard.state = GetClipboardContext::PRESENT;
					if (_get_clipboard.data) {
						_get_clipboard.data->resize(size);
						memcpy(_get_clipboard.data->data(), data, size);
					}
				}
				XFree(data);

			} else {
				fprintf(stderr, "DispatchSelectionNotify: wrong target=0x%lx\n", (unsigned long)target);
				if (!target) {
					_get_clipboard.state = GetClipboardContext::MISSING;
				}
			}
			XDeleteProperty(event.xselection.display, event.xselection.requestor, event.xselection.property);

		} else if (_get_clipboard.state == GetClipboardContext::REQUESTING) {
			_get_clipboard.state = GetClipboardContext::MISSING;
		}
	}

	void DispatchSelectionRequest(XEvent &event)
	{
		XSelectionEvent ev = {0};
		ev.type = SelectionNotify;
		ev.display = event.xselectionrequest.display;
		ev.requestor = event.xselectionrequest.requestor;
		ev.selection = event.xselectionrequest.selection;
		ev.time = event.xselectionrequest.time;
		ev.target = event.xselectionrequest.target;
		ev.property = event.xselectionrequest.property;

		int r;
		if (ev.target == _targets_atom) {
			std::vector<Atom> supported {_targets_atom};
			if (_set_clipboard) for ( const auto &it : *_set_clipboard) {
				if (it.first.empty()) {
					supported.emplace_back(XA_STRING);
					supported.emplace_back(_utf8_atom);
				} else {
					supported.emplace_back(CustomFormatAtom(it.first));
				}

			} else {
				fprintf(stderr, "SelectionRequest: no targets to report\n");
			}
			r = XChangeProperty(ev.display, ev.requestor, ev.property, XA_ATOM, 32,
				PropModeReplace, (unsigned char*)supported.data(), supported.size());

		} else try {
			if (!_set_clipboard) {
				throw std::runtime_error("no data to set");
			}
			std::string format_name;
			if (ev.target != XA_STRING && ev.target != _text_atom && ev.target != _utf8_atom) {
				format_name = CustomFormatFromAtom(ev.target);
				if (format_name.empty()) {
					throw std::runtime_error("unregistered target");
				}
			}

			const auto &it = _set_clipboard->find(format_name);
			if (it == _set_clipboard->end()) {
				throw std::runtime_error("mismatching target");
			}
			r = XChangeProperty(ev.display, ev.requestor, ev.property, ev.target, 8,
				PropModeReplace, it->second.data(), it->second.size());

			} catch (std::exception &e) {
			fprintf(stderr, "SelectionRequest: %s\n", e.what());
			ev.property = None;
			r = 0;
		}
		if ((r & 2) == 0) {
			XSendEvent(_display, ev.requestor, 0, 0, (XEvent *)&ev);
		}
	}

	void DispatchOneEvent()
	{
		XEvent event;
		XNextEvent(_display, &event);

		//fprintf(stderr, "TTYX: DispatchOneEvent\n");
#ifdef TTYXI
		if (DispatchXiEvent(event))
			return;
#endif

		switch (event.type) {
			case SelectionNotify:
				if (event.xselection.selection == _clipboard_atom) {
					DispatchSelectionNotify(event);
				}
				break;
			case SelectionRequest:
				if (event.xselectionrequest.selection == _clipboard_atom) {
					DispatchSelectionRequest(event);
				}
				break;
			case SelectionClear:
				if (event.xselectionrequest.selection == _clipboard_atom) {
					_set_clipboard.reset();
				}
				break;
		}
	}

	void DispatchPendingEvents()
	{
		while (XPending(_display)) {
			DispatchOneEvent();
		}
	}

public:
	TTYX(bool allow_xi)
	{
		const auto start_time = std::chrono::steady_clock::now();
		_display = XOpenDisplay(0);
		if (!_display)
			throw std::runtime_error("Cannot open display");

		_display_fd = ConnectionNumber(_display);
		_screen = DefaultScreen(_display);
		_root_window = RootWindow(_display, _screen);
		auto color = BlackPixel(_display, _screen);
		_window = XCreateSimpleWindow(_display, _root_window, 0, 0, 1, 1, 0, color, color);

#ifdef TTYXI
		_xi = true;
		// Test for XInput 2 extension
		int xi_query_event, xi_query_error;
		if (!allow_xi) {
			fprintf(stderr, "TTYXi: XI disallowed\n");
			_xi = false;

		} else if (! XQueryExtension(_display, "XInputExtension", &_xi_opcode, &xi_query_event, &xi_query_error)) {
			fprintf(stderr, "TTYXi: XI not available\n");
			_xi = false;

		} else { // Request XInput 2.0, to guard against changes in future versions
			int major = 2, minor = 0;
			int qr = XIQueryVersion(_display, &major, &minor);
			if (qr == BadRequest) {
				fprintf(stderr, "TTYXi: Need XI 2.0 support (got %d.%d)\n", major, minor);
				_xi = false;

			} else if (qr != Success) {
				fprintf(stderr, "TTYXi: XIQueryVersion error %d\n", qr);
				_xi = false;

			} else {
				XIEventMask m{};
				m.deviceid = XIAllMasterDevices;
				m.mask_len = XIMaskLen(XI_LASTEVENT);
				std::vector<unsigned char> mask(m.mask_len);
				m.mask = mask.data();
				XISetMask(m.mask, XI_RawKeyPress);
				XISetMask(m.mask, XI_RawKeyRelease);
				int ser = XISelectEvents(_display, _root_window, &m, 1); /*number of masks*/
				if (ser != Success) {
					_xi = false;
					fprintf(stderr, "TTYXi: XISelectEvents error %d\n", ser);
				}
			}
		}
#endif
		const unsigned int init_msec = std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::steady_clock::now() - start_time).count();

#ifdef TTYXI
		if (_xi) { // few empiric magic coefficients here...
			if (init_msec * 3 > _xi_keydown_maxwait_msec) {
				_xi_keydown_maxwait_msec = std::min(init_msec * 3, _xi_keydown_maxwait_msec_max);
			}
			if (init_msec * 2 > _xi_modifier_check_trsh_msec) {
				_xi_modifier_check_trsh_msec = std::min(init_msec * 2, _xi_modifier_check_trsh_msec_max);
			}
			fprintf(stderr, "TTYXi: initialized in %u msec, keydown_maxwait=%u modifier_check_trsh=%u\n",
				init_msec, _xi_keydown_maxwait_msec, _xi_modifier_check_trsh_msec);
		} else
#endif
		{
			fprintf(stderr, "TTYX: initialized in %u msec\n", init_msec);
		}
	}

	~TTYX()
	{
		if (_xkb_en) { XkbFreeKeyboard(_xkb_en, 0, True); }
		XDestroyWindow(_display, _window);
		XCloseDisplay(_display);
	}

	void LateInitialization()
	{
		_targets_atom = XInternAtom(_display, "TARGETS", 0);
		_text_atom = XInternAtom(_display, "TEXT", 0);
		_utf8_atom = XInternAtom(_display, "UTF8_STRING", 1);
		_clipboard_atom = XInternAtom(_display, "CLIPBOARD", 0);
		_xsel_data_atom = XInternAtom(_display, "XSEL_DATA", 0);
	}

	bool HasXi() const
	{
#ifdef TTYXI
		return _xi;
#else
		return false;
#endif
	}

	bool GetClipboard(const std::string &type, std::vector<unsigned char> *data = nullptr)
	{
		_get_clipboard.a = type.empty()
			? (_utf8_atom == None ? XA_STRING : _utf8_atom)
			: CustomFormatAtom(type);
		_get_clipboard.data = data;
		_get_clipboard.state = GetClipboardContext::REQUESTING;
		XConvertSelection(_display, _clipboard_atom,
			_get_clipboard.a, _xsel_data_atom, _window, CurrentTime);
		XSync(_display, 0);
		do {
			DispatchOneEvent();
		} while (_get_clipboard.state == GetClipboardContext::REQUESTING);

		return _get_clipboard.state == GetClipboardContext::PRESENT;
	}

	void SetClipboard(std::unique_ptr<Type2Data> &&td)
	{
		_set_clipboard = std::move(td);
		XSetSelectionOwner(_display, _clipboard_atom, _window, 0);
		XSync(_display, 0);
	}

	void InspectKeyEvent(KEY_EVENT_RECORD &event)
	{

#ifdef TTYXI
		if ((event.uChar.UnicodeChar > 127) && (!event.wVirtualKeyCode)) {
			for (const auto &xki : _xi_keys) {
				if (xki.first && !IsXiKeyModifier(xki.first)) {
					switch (xki.first) {
						case XK_minus:        event.wVirtualKeyCode = VK_OEM_MINUS;   break;
						case XK_equal:        event.wVirtualKeyCode = VK_OEM_PLUS;    break;
						case XK_bracketleft:  event.wVirtualKeyCode = VK_OEM_4;       break;
						case XK_bracketright: event.wVirtualKeyCode = VK_OEM_6;       break;
						case XK_semicolon:    event.wVirtualKeyCode = VK_OEM_1;       break;
						case XK_apostrophe:   event.wVirtualKeyCode = VK_OEM_7;       break;
						case XK_grave:        event.wVirtualKeyCode = VK_OEM_3;       break;
						case XK_backslash:    event.wVirtualKeyCode = VK_OEM_5;       break;
						case XK_comma:        event.wVirtualKeyCode = VK_OEM_COMMA;   break;
						case XK_period:       event.wVirtualKeyCode = VK_OEM_PERIOD;  break;
						case XK_slash:        event.wVirtualKeyCode = VK_OEM_2;       break;
						default:
							char* kstr = XKeysymToString(xki.first);
							if (strlen(kstr) == 1) { event.wVirtualKeyCode = toupper(kstr[0]); }
					}
				}
			}
		}
#endif

		auto x_keymods = GetKeyModifiers();
#ifndef TTYXI
		event.dwControlKeyState|= x_keymods;
#else
		if (!_xi) {
			event.dwControlKeyState|= x_keymods;
			return;
		}

		// fixup ambiguous key codes
		static const struct KeyFixup {
			KeySym expect_ks;
			WORD expect_vk;
			char expect_ch;
			bool need_ctrl;
			WORD actual_vk;

		} key_fixup [] = { // todo: extend
			{XK_KP_Add, 0, '+', false, VK_ADD},
			{XK_KP_Subtract, 0, '-', false, VK_SUBTRACT},
			{XK_KP_Multiply, 0, '*', false, VK_MULTIPLY},
			{XK_KP_Divide, 0, '/', false, VK_DIVIDE},
			{XK_KP_Decimal, 0, '.', false, VK_DECIMAL},
			{XK_Escape, 0, '\x1b', false, VK_ESCAPE},
			{XK_BackSpace, 'H', 0, true, VK_BACK},
			{'2', VK_SPACE, ' ', true, '2'},
			{XK_grave, VK_SPACE, ' ', true, VK_OEM_3},
			{'3', 0, '\x1b', true, '3'},
			{XK_bracketleft, 0, '\x1b', true, VK_OEM_4},
			{XK_slash, '7', 0, true, VK_DIVIDE},
			{'4', VK_OEM_5, 0, true, '4'},
			{'5', VK_OEM_6, 0, true, '5'},
			{'8', VK_BACK, 0, true, '8'},
			{'m', VK_RETURN, 0, true, 'M'},
			// Ctrl+1/6/7/9/0 seems don't need fixup
		};
		WaitForNonModifierXiKeydown();
#if 0 /* change to 1 to see pressed XK_* codes */
		for (const auto &xik : _xi_keys) {
			fprintf(stderr, "!!! TTYXi: xik=0x%lx vk=0x%x ch=0x%x\n",
				xik.first, event.wVirtualKeyCode, event.uChar.UnicodeChar);
		}
#endif

		if (event.wVirtualKeyCode == VK_RETURN
				&& _xi_keys.find(XK_KP_Enter) == _xi_keys.end()
				&& _xi_keys.find(XK_Return) == _xi_keys.end()) {
			/* Workaround for https://github.com/elfmz/far2l/issues/1193
			 * In case pressing Shift+Ins terminal sends characters
			 * however Shift+Enter treated in far2l's editor in a way
			 * that breaks pasting behavior. */
			x_keymods&= ~SHIFT_PRESSED;
		}
		event.dwControlKeyState|= x_keymods;

		for (const auto &kf : key_fixup) {
			if (event.uChar.UnicodeChar == kf.expect_ch && event.wVirtualKeyCode == kf.expect_vk
				&& (!kf.need_ctrl || (event.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0)
				&& (_xi_keys.find(kf.expect_ks) != _xi_keys.end() || kf.expect_ks == _xi_recent_nonmod_keyup))
			{
				fprintf(stderr, "TTYXi: InspectKeyEvent ch=0x%x cks=0x%x vk: 0x%x -> 0x%x\n",
					event.uChar.UnicodeChar, event.dwControlKeyState, event.wVirtualKeyCode, kf.actual_vk);
				event.wVirtualKeyCode = kf.actual_vk;
				break;
			}
		}

		// lagged TTY workaround has one-shot nature, so clear it just in case
		_xi_recent_nonmod_keyup = 0;
#endif
	}

	void Idle(int ipc_fdr)
	{
		fd_set fds;
		do {
			DispatchPendingEvents();

			FD_ZERO(&fds);
			FD_SET(_display_fd, &fds);
			FD_SET(ipc_fdr, &fds);

			select(std::max(_display_fd, ipc_fdr) + 1, &fds, NULL, NULL, NULL);

		} while (!FD_ISSET(ipc_fdr, &fds));
	}
};

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "This file is not intended to be started by user\n");
		return -1;
	}

	try {
		const int fdr = atoi(argv[1]);
		const int fdw = atoi(argv[2]);
		TTYXIPCEndpoint ipc(fdr, fdw);
		std::string type;
		const auto cmd = ipc.RecvCommand();
		if (cmd != IPC_INIT)
			throw PipeIPCError("bad IPC init command", (unsigned int)cmd);

		bool allow_xi = true;
		ipc.RecvPOD(allow_xi);

		TTYX ttyx(allow_xi);
		ipc.SendCommand(IPC_INIT);
		ipc.SendPOD(ttyx.HasXi());

		ttyx.LateInitialization();

		for (;;) {
			ttyx.Idle(fdr);
			const auto cmd = ipc.RecvCommand();
			switch (cmd) {
				case IPC_FINI:
					return 0;

				case IPC_CLIPBOARD_CONTAINS: {
					ipc.RecvString(type);
					bool b = ttyx.GetClipboard(type);
					ipc.SendPOD(b);
				} break;

				case IPC_CLIPBOARD_GET: {
					ipc.RecvString(type);
					std::vector<unsigned char> data;
					ssize_t sz = ttyx.GetClipboard(type, &data) ? data.size() : -1;
					ipc.SendPOD(sz);
					if (sz > 0) {
						ipc.Send(data.data(), data.size());
					}
				} break;

				case IPC_CLIPBOARD_SET: {
					std::unique_ptr<Type2Data> t2d(new Type2Data);
					for (;;) {
						size_t size{};
						ipc.RecvPOD(size);
						if (size == size_t(-1)) {
							break;
						}
						ipc.RecvString(type);
						auto &data = (*t2d)[type];
						data.resize(size);
						if (!data.empty()) {
							ipc.Recv(data.data(), data.size());
						}
					}
					ttyx.SetClipboard(std::move(t2d));
				} break;

				case IPC_INSPECT_KEY_EVENT: {
					KEY_EVENT_RECORD KeyEvent;
					ipc.RecvPOD(KeyEvent);
					ttyx.InspectKeyEvent(KeyEvent);
					ipc.SendPOD(KeyEvent);
				} break;

				default:
					throw PipeIPCError("bad IPC command", (unsigned int)cmd);
			}
		}
	} catch (std::exception &e) {
		fprintf(stderr, "TTYX: %s\n", e.what());
	}

	return -1;
}
