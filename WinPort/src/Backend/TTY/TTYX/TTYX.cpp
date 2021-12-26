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
				fprintf(stderr, "!!!!! %d %d\n", cookie->evtype == XI_RawKeyPress, ev->detail);
				KeySym s = XkbKeycodeToKeysym(_display, ev->detail, 0 /*xkbState.group*/, 0 /*shift level*/);

				if (cookie->evtype == XI_RawKeyPress) {
					_xi_keystate.insert(s);
				} else {
					_xi_keystate.erase(s);
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
					case XK_Num_Lock: {
						_xi_mods&= ~NUMLOCK_ON;
						_xi_mods|= (GetModifiersByXQueryPointer() & NUMLOCK_ON);
					} break;
				}
			}
			return true;
		}
#endif
		return false;
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
				if (event.xselection.property != None) {
					Atom target {};
					int format;
					unsigned long size, n;
					unsigned char* data = NULL;
					XGetWindowProperty(event.xselection.display,
						event.xselection.requestor, event.xselection.property,
						0L,(~0L), 0, AnyPropertyType, &target, &format, &size, &n, &data);
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
			break;

		case SelectionRequest:
			if (event.xselectionrequest.selection == _clipboard_atom) {
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
			break;

		case SelectionClear:
			if (event.xselectionrequest.selection == _clipboard_atom) {
				_set_clipboard.reset();
			}
			break;
		}
	}

	DWORD GetModifiersByXQueryPointer()
	{
		Window root, child;
		int root_x, root_y;
		int win_x, win_y;
		unsigned int mods = 0;

		XQueryPointer (_display, _root_window, &root, &child,
				&root_x, &root_y, &win_x, &win_y, &mods);

		DWORD out = 0;
		if ((mods & ShiftMask) != 0)
			out|= SHIFT_PRESSED;
		if ((mods & LockMask) != 0)
			out|= NUMLOCK_ON;
		if ((mods & ControlMask) != 0)
			out|= LEFT_CTRL_PRESSED;
		if ((mods & Mod1Mask) != 0)
			out|= LEFT_ALT_PRESSED;
		if ((mods & Mod5Mask) != 0)
			out|= RIGHT_ALT_PRESSED;
		return out;
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
			fprintf(stderr, "TTYX: XI not available\n");
			_xi = false;

		} else { // Request XInput 2.0, to guard against changes in future versions
			int major = 2, minor = 0;
			int qr = XIQueryVersion(_display, &major, &minor);
			if (qr == BadRequest) {
				fprintf(stderr, "TTYX: Need XI 2.0 support (got %d.%d)\n", major, minor);
				_xi = false;

			} else if (qr != Success) {
				fprintf(stderr, "TTYX: XIQueryVersion error %d\n", qr);
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
				fprintf(stderr, "TTYX: XISelectEvents %s %d\n", ser ? "error" : "status", ser);
			}
		}
#endif
	}

	~TTYX()
	{
		XDestroyWindow(_display, _window);
		XCloseDisplay(_display);
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
		if (_xi) {
			event.dwControlKeyState|= _xi_mods;
		} else {
			event.dwControlKeyState|= GetModifiersByXQueryPointer();
		}
		if (!event.wVirtualKeyCode && event.uChar.UnicodeChar) {
			struct KeyMap {
				KeySym ks;
				WORD vk;
				char ch;
			} key_remap [] = { // todo: extend
				{XK_KP_Add, VK_ADD, '+', },
				{XK_KP_Subtract, VK_SUBTRACT, '-'},
				{XK_KP_Multiply, VK_MULTIPLY, '*'},
				{XK_KP_Divide, VK_DIVIDE, '/'},
				{XK_KP_Decimal, VK_DECIMAL, '.'},
				{XK_Escape, VK_ESCAPE, '\x1b'}
			};
			for (const auto &krm : key_remap) {
				if (event.uChar.UnicodeChar == krm.ch && _xi_keystate.find(krm.ks) != _xi_keystate.end()) {
					event.wVirtualKeyCode = krm.vk;
					break;
				}
			}
		}
	}

	void Idle(int ipc_fdr)
	{
		for (;;) {
			while (XPending(_display)) {
				DispatchOneEvent();
			}

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
		IPCEndpoint ipc(fdr, fdw);
		TTYX ttyx;
		std::string type;
		const auto cmd = ipc.RecvCommand();
		if (cmd != IPC_INIT)
			throw PipeIPCError("bad IPC init command", (unsigned int)cmd);

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
