#include "TTYX.h"

#include "WinCompat.h"
#include <utils.h>
#include <Threaded.h>

#include <os_call.hpp>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#ifdef TTYXI
# include <X11/XKBlib.h>
# include <X11/extensions/XInput2.h>
#endif

#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <memory>

typedef std::map<std::string, std::vector<unsigned char> > Type2Data;

#define XI_BACKLOG_LIMIT 0x100

class TTYX
{
	Display *_display = nullptr;
	int _screen;
	Window _root_window;
	Window _window;
	Atom _targets_atom;
	Atom _text_atom;
	Atom _utf8_atom;
	Atom _clipboard_atom;
	Atom _xsel_data_atom;
	int _display_fd;
	int _xi_opcode;
	bool _xi = false;
	DWORD _xi_mods = 0;
	std::set<KeySym> _xi_keystate;

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

	bool DispatchXIEvent(XEvent &event)
	{
#ifdef TTYXI
		if (!_xi) {
			return false;
		}
		XGenericEventCookie *cookie = (XGenericEventCookie*)&event.xcookie;
		if (XGetEventData(_display, cookie) && cookie->type == GenericEvent && cookie->extension == _xi_opcode) {
			if ((cookie->evtype == XI_RawKeyPress || cookie->evtype == XI_RawKeyRelease) && cookie->data) {
				XIRawEvent *ev = (XIRawEvent *)cookie->data;
				//fprintf(stderr, "TTYXI: !!!!! %d %d\n", cookie->evtype == XI_RawKeyPress, ev->detail);
				KeySym s = XkbKeycodeToKeysym(_display, ev->detail, 0 /*xkbState.group*/, 0 /*shift level*/);

				if (cookie->evtype == XI_RawKeyPress) {
					_xi_keystate.insert(s);
				} else if (_xi_keystate.erase(s) == 0) {
					fprintf(stderr, "TTYXI: keyrelease for nonpressed modifier 0x%lx\n", s);
				}

# define TTYXI_KEYMOD(MOD) { if (cookie->evtype == XI_RawKeyPress) _xi_mods|= MOD; else _xi_mods&= ~MOD; }
				switch (s) {
					case XK_Control_L: TTYXI_KEYMOD(LEFT_CTRL_PRESSED); break;
					case XK_Control_R: TTYXI_KEYMOD(RIGHT_CTRL_PRESSED); break;
					case XK_Shift_L: TTYXI_KEYMOD(SHIFT_PRESSED); break;
					case XK_Shift_R: TTYXI_KEYMOD(SHIFT_PRESSED); break;
					case XK_Alt_L: TTYXI_KEYMOD(LEFT_ALT_PRESSED); break;
					case XK_Alt_R: TTYXI_KEYMOD(RIGHT_ALT_PRESSED); break;
					case XK_Super_L: TTYXI_KEYMOD(LEFT_ALT_PRESSED); break;
					case XK_Super_R: TTYXI_KEYMOD(RIGHT_ALT_PRESSED); break;
					case XK_Num_Lock: // special case: need toggled status but not pressed
						_xi_mods&= ~NUMLOCK_ON;
						if ((GetXKeyModifiers() & LockMask) != 0) {
							_xi_mods|= NUMLOCK_ON;
						}
						break;
				}
			}
			return true;
		}
#endif
		return false;
	}

	unsigned int GetXKeyModifiers()
	{
		Window root, child;
		int root_x, root_y;
		int win_x, win_y;
		unsigned int mods = 0;

		XQueryPointer(_display, _root_window, &root, &child,
				&root_x, &root_y, &win_x, &win_y, &mods);

		return mods;
	}

	DWORD GetWPKeyModifiers()
	{
		DWORD out = 0;
		if (_xi) {
			if ( (_xi_mods & (SHIFT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0) {
				// can't blindly use _xi_mods cause some control keys tend
				// to latch if pressed as part of system hotkey, like Alt+TAB
				const auto mods = GetXKeyModifiers();
				if ((mods & ControlMask) == 0) {
					_xi_mods&= ~LEFT_CTRL_PRESSED;
					_xi_mods&= ~RIGHT_CTRL_PRESSED;
				}
				if ((mods & Mod1Mask) == 0) {
					_xi_mods&= ~LEFT_ALT_PRESSED;
				}
				if ((mods & Mod2Mask) == 0) {
					_xi_mods&= ~RIGHT_ALT_PRESSED;
				}
				if ((mods & ShiftMask) == 0) {
					_xi_mods&= ~SHIFT_PRESSED;
				}
			}
			out|= _xi_mods;

		} else {
			const auto mods = GetXKeyModifiers();
			if ((mods & ControlMask) != 0) {
				out|= LEFT_CTRL_PRESSED;
			}
			if ((mods & Mod1Mask) != 0) {
				out|= LEFT_ALT_PRESSED;
			}
			if ((mods & Mod5Mask) != 0) {
				out|= RIGHT_ALT_PRESSED;
			}
			if ((mods & ShiftMask) != 0) {
				out|= SHIFT_PRESSED;
			}
			if ((mods & LockMask) != 0) {
				out|= NUMLOCK_ON;
			}
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

		if (DispatchXIEvent(event))
			return;

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

	void DispatchLatecomerXIKeydown() // wait for some non-modifier keydown state for max of 100 msec
	{
		for (int i = 0; i < 10; ++i) {
			for (const auto &xk : _xi_keystate) {
				if (xk != XK_Control_L && xk != XK_Control_R
				  && xk != XK_Shift_L && xk != XK_Shift_R
				  && xk != XK_Alt_L && xk != XK_Alt_R
				  && xk != XK_Super_L && xk != XK_Super_L
				  && xk != XK_Hyper_L && xk != XK_Hyper_L) {
					return;
				}
			}
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(_display_fd, &fds);

			struct timeval tv = {0, 10000};
			select(_display_fd + 1, &fds, NULL, NULL, &tv);
			if (FD_ISSET(_display_fd, &fds)) {
				DispatchPendingEvents();
			}
		}
		fprintf(stderr, "TTYXI: DispatchLatecomerXIKeydown - timed out\n");
	}

public:
	TTYX()
	{
		_display = XOpenDisplay(0);
		if (!_display)
			throw std::runtime_error("Cannot open display");

		_display_fd = ConnectionNumber(_display);
		_screen = DefaultScreen(_display);
		_root_window = DefaultRootWindow (_display);
		_window = XCreateSimpleWindow(_display, RootWindow(_display, _screen),
			0, 0, 1, 1, 0, BlackPixel(_display, _screen), WhitePixel(_display, _screen));
		_targets_atom = XInternAtom(_display, "TARGETS", 0);
		_text_atom = XInternAtom(_display, "TEXT", 0);
		_utf8_atom = XInternAtom(_display, "UTF8_STRING", 1);
		_clipboard_atom = XInternAtom(_display, "CLIPBOARD", 0);
		_xsel_data_atom = XInternAtom(_display, "XSEL_DATA", 0);

#ifdef TTYXI
		_xi = true;
		// Test for XInput 2 extension
		int xi_query_event, xi_query_error;
		if (! XQueryExtension(_display, "XInputExtension", &_xi_opcode, &xi_query_event, &xi_query_error)) {
			fprintf(stderr, "TTYXI: XI not available\n");
			_xi = false;

		} else { // Request XInput 2.0, to guard against changes in future versions
			int major = 2, minor = 0;
			int qr = XIQueryVersion(_display, &major, &minor);
			if (qr == BadRequest) {
				fprintf(stderr, "TTYXI: Need XI 2.0 support (got %d.%d)\n", major, minor);
				_xi = false;

			} else if (qr != Success) {
				fprintf(stderr, "TTYXI: XIQueryVersion error %d\n", qr);
				_xi = false;

			} else {
				XIEventMask m{};
				m.deviceid = XIAllMasterDevices;
				m.mask_len = XIMaskLen(XI_LASTEVENT);
				std::vector<unsigned char> mask(m.mask_len);
				m.mask = mask.data();
				XISetMask(m.mask, XI_RawKeyPress);
				XISetMask(m.mask, XI_RawKeyRelease);
				int ser = XISelectEvents(_display, _root_window, &m, 1);  /*number of masks*/
				if (ser != Success) {
					_xi = false;
					fprintf(stderr, "TTYXI: XISelectEvents error %d\n", ser);
				}
			}
		}
#endif
		fprintf(stderr, "%s: initialized\n", _xi ? "TTYXI" : "TTYX");
	}

	~TTYX()
	{
		XDestroyWindow(_display, _window);
		XCloseDisplay(_display);
	}

	bool HasXI() const
	{
		return _xi;
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
		event.dwControlKeyState|= GetWPKeyModifiers();
		if (!_xi)
			return;

		DispatchLatecomerXIKeydown();

		// fixup ambiguous key codes
		static const struct KeyFixup {
			KeySym expect_ks;
			WORD expect_vk;
			char expect_ch;
			bool expect_ctrl;
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
			{'3', 0, '\x1b', true, '3'},
			{'4', VK_OEM_5, 0, true, '4'},
			{'5', VK_OEM_6, 0, true, '5'},
			{'8', VK_BACK, 0, true, '8'},
			{'m', VK_RETURN, 0, true, 'M'},
			// Ctrl+1/6/7/9/0 seems don't need fixup
		};

		for (const auto &kf : key_fixup) {
			if (event.uChar.UnicodeChar == kf.expect_ch && event.wVirtualKeyCode == kf.expect_vk
			  && (!kf.expect_ctrl || (event.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) != 0)
			  && _xi_keystate.find(kf.expect_ks) != _xi_keystate.end()) {
				fprintf(stderr, "TTYXI: InspectKeyEvent ch=0x%x cks=0x%x vk: 0x%x -> 0x%x\n",
					event.uChar.UnicodeChar, event.dwControlKeyState, event.wVirtualKeyCode, kf.actual_vk);
				event.wVirtualKeyCode = kf.actual_vk;
				break;
			}
		}
	}

	void Idle(int ipc_fdr)
	{
		for (;;) {
			DispatchPendingEvents();

			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(_display_fd, &fds);
			FD_SET(ipc_fdr, &fds);

			select(std::max(_display_fd, ipc_fdr) + 1, &fds, NULL, NULL, NULL);
			if (FD_ISSET(ipc_fdr, &fds)) {
				break;
			}
		}
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
		TTYX ttyx;
		std::string type;
		const auto cmd = ipc.RecvCommand();
		if (cmd != IPC_INIT)
			throw PipeIPCError("bad IPC init command", (unsigned int)cmd);

		ipc.SendCommand(IPC_INIT);
		ipc.SendPOD(ttyx.HasXI());

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
