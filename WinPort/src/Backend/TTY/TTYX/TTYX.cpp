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

	std::string *_get_clipboard = nullptr;
	std::string _set_clipboard;

	void DispatchOneEvent()
	{
		XEvent event;
		XNextEvent(_display, &event);
		switch (event.type) {
		case SelectionNotify:
			if (event.xselection.property && event.xselection.selection == _clipboard_atom) {
				Atom target {};
				int format;
				unsigned long size, n;
				char* data{};
				XGetWindowProperty(event.xselection.display,
					event.xselection.requestor, event.xselection.property,
					0L,(~0L), 0, AnyPropertyType, &target, &format, &size, &n, (unsigned char**)&data);
				if (target == _utf8_atom || target == XA_STRING) {
					if (_get_clipboard) {
						_get_clipboard->assign(data, size);
						_get_clipboard = nullptr;
					}
					XFree(data);
				}
				XDeleteProperty(event.xselection.display, event.xselection.requestor, event.xselection.property);
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
					Atom supported[] = {_targets_atom, XA_STRING, _utf8_atom};
					r = XChangeProperty(ev.display, ev.requestor, ev.property, XA_ATOM, 32,
						PropModeReplace, (unsigned char*)(&supported), sizeof(supported)/sizeof(supported[0]));

				} else if (ev.target == XA_STRING || ev.target == _text_atom || ev.target == _utf8_atom) {
					r = XChangeProperty(ev.display, ev.requestor, ev.property, ev.target, 8,
					PropModeReplace, (unsigned char*)_set_clipboard.data(), _set_clipboard.size());

				} else {
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
				_set_clipboard.clear();
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

	void GetClipboard(std::string &s)
	{
		_get_clipboard = &s;
		XConvertSelection(_display, _clipboard_atom,
			(_utf8_atom == None ? XA_STRING : _utf8_atom), _xsel_data_atom, _window, CurrentTime);
		XSync(_display, 0);
		do {
			DispatchOneEvent();
		} while (_get_clipboard != nullptr);
	}

	void SetClipboard(std::string &s)
	{
		_set_clipboard.swap(s);
		XSetSelectionOwner(_display, _clipboard_atom, _window, 0);
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
		fprintf(stderr, "This process not indended to be started by user\n");
		return -1;
	}

	try {
		const int fdr = atoi(argv[1]);
		const int fdw = atoi(argv[2]);
		IPCEndpoint ipc(fdr, fdw);
		TTYX ttyx;
		std::string str;
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

				case IPC_CLIPBOARD_GET: {
					ttyx.GetClipboard(str);
					ipc.SendString(str);
				} break;

				case IPC_CLIPBOARD_SET: {
					ipc.RecvString(str);
					ttyx.SetClipboard(str);
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
