#include "TTYX.h"

#include "WinCompat.h"
#include <utils.h>
#include <Threaded.h>

#include <os_call.hpp>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <string>
#include <map>
#include <vector>
#include <memory>

typedef std::map<std::string, std::vector<unsigned char> > Type2Data;

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

	void DispatchOneEvent()
	{
		XEvent event;
		XNextEvent(_display, &event);
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
	}

	~TTYX()
	{
		XDestroyWindow(_display, _window);
		XCloseDisplay(_display);
	}

	DWORD GetModifiers()
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

	void Idle(int ipc_fdr)
	{
		for (;;) {
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(_display_fd, &fds);
			FD_SET(ipc_fdr, &fds);

			select(std::max(_display_fd, ipc_fdr) + 1, &fds, NULL, NULL, NULL);
			if (FD_ISSET(ipc_fdr, &fds)) {
				break;
			}

			do {
				DispatchOneEvent();
			} while (XPending(_display));
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

				case IPC_MODIFIERS: {
					DWORD dw = ttyx.GetModifiers();
					ipc.SendPOD(dw);
				} break;

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

				default:
					throw PipeIPCError("bad IPC command", (unsigned int)cmd);
			}
		}
	} catch (std::exception &e) {
		fprintf(stderr, "TTYX: %s\n", e.what());
	}

	return -1;
}
