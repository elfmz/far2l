#include "WaylandGlobalShortcuts.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include "WinCompat.h"
#include "Backend.h"

// Python proxy script content
static const char* PYTHON_PROXY_SCRIPT = R"EOF(
import dbus
import dbus.mainloop.glib
from gi.repository import GLib
import sys
import os
import signal

# Flush stdout immediately for IPC
sys.stdout.reconfigure(line_buffering=True)

loop = None

def on_activated(session_handle, shortcut_id, timestamp, options):
    print(f"EVENT:{shortcut_id}", flush=True)

def on_bind_response(results):
    print("LOG:Shortcuts bound successfully", flush=True)

def on_bind_error(e):
    print(f"LOG:Bind Error: {e}", flush=True)
    sys.exit(1)

def on_session_response(response, results):
    if response != 0:
        print(f"LOG:Error creating session: {response}", flush=True)
        sys.exit(1)

    session_handle = results['session_handle']

    bus = dbus.SessionBus()
    try:
        portal = bus.get_object('org.freedesktop.portal.Desktop', '/org/freedesktop/portal/desktop')
        iface = dbus.Interface(portal, 'org.freedesktop.portal.GlobalShortcuts')
    except Exception as e:
        print(f"LOG:Failed to get portal interface: {e}", flush=True)
        sys.exit(1)

    shortcuts = [
        ('CtrlEnter', {'description': 'Far2l Ctrl+Enter', 'preferred_trigger': 'Control+Return'}),
        ('CtrlTab', {'description': 'Far2l Ctrl+Tab', 'preferred_trigger': 'Control+Tab'})
    ]

    print("LOG:Binding shortcuts...", flush=True)
    iface.BindShortcuts(
        session_handle,
        shortcuts,
        "",
        {'handle_token': 'far2l_bind_token'},
        reply_handler=on_bind_response,
        error_handler=on_bind_error
    )

    bus.add_signal_receiver(
        on_activated,
        dbus_interface='org.freedesktop.portal.GlobalShortcuts',
        signal_name='Activated',
        path=session_handle
    )

def main():
    global loop
    # Ignore SIGINT/SIGTERM to handle cleanup if needed, but here we just die when parent closes pipe

    try:
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        bus = dbus.SessionBus()

        portal = bus.get_object('org.freedesktop.portal.Desktop', '/org/freedesktop/portal/desktop')
        iface = dbus.Interface(portal, 'org.freedesktop.portal.GlobalShortcuts')

        print("LOG:Creating session...", flush=True)
        # Unique session token per process to avoid conflicts
        token = f"far2l_session_{os.getpid()}"
        request_path = iface.CreateSession(
            {'session_handle_token': token}
        )

        bus.add_signal_receiver(
            on_session_response,
            dbus_interface='org.freedesktop.portal.Request',
            signal_name='Response',
            path=request_path
        )

        loop = GLib.MainLoop()
        loop.run()

    except Exception as e:
        print(f"LOG:Python Exception: {e}", flush=True)
        sys.exit(1)

if __name__ == '__main__':
    main()
)EOF";

// -----------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------

WaylandGlobalShortcuts::WaylandGlobalShortcuts() {}

WaylandGlobalShortcuts::~WaylandGlobalShortcuts() {
	Stop();
}

void WaylandGlobalShortcuts::SetFocused(bool focused) {
	_focused = focused;
}

void WaylandGlobalShortcuts::SetPaused(bool paused) {
	if (_paused != paused) {
		fprintf(stderr, "[WaylandShortcuts] State change: Paused = %s (Terminal extension detected)\n", paused ? "TRUE" : "FALSE");
		_paused = paused;
	}
}

bool WaylandGlobalShortcuts::IsRecentlyActive(uint32_t window_ms) {
	auto now = std::chrono::steady_clock::now().time_since_epoch();
	uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	uint64_t last = _last_activity_ts.load();
	if (last == 0) return false;
	return (now_ms - last) <= window_ms;
}

void WaylandGlobalShortcuts::Start() {
	if (_running) return;
	fprintf(stderr, "[WaylandShortcuts] Start() called (Python Proxy Mode)\n");

	// 1. Create temporary python script
	char tmp_path[] = "/tmp/far2l_wl_proxy_XXXXXX.py";
	int fd = mkstemps(tmp_path, 3); // 3 is length of ".py"
	if (fd == -1) {
		perror("[WaylandShortcuts] Failed to create temp file");
		return;
	}

	if (write(fd, PYTHON_PROXY_SCRIPT, strlen(PYTHON_PROXY_SCRIPT)) < 0) {
		perror("[WaylandShortcuts] Write failed");
		close(fd);
		unlink(tmp_path);
		return;
	}
	close(fd);
	_script_path = tmp_path;
	fprintf(stderr, "[WaylandShortcuts] Script created at %s\n", _script_path.c_str());

	// 2. Create pipes for stdout and stderr
	int out_pipe[2], err_pipe[2];
	if (pipe(out_pipe) == -1 || pipe(err_pipe) == -1) {
		perror("[WaylandShortcuts] pipe creation failed");
		unlink(_script_path.c_str());
		if (out_pipe[0] != -1) { close(out_pipe[0]); close(out_pipe[1]); }
		if (err_pipe[0] != -1) { close(err_pipe[0]); close(err_pipe[1]); }
		return;
	}

	// 3. Fork and Exec
	pid_t pid = fork();
	if (pid == -1) {
		perror("[WaylandShortcuts] fork failed");
		close(out_pipe[0]); close(out_pipe[1]);
		close(err_pipe[0]); close(err_pipe[1]);
		unlink(_script_path.c_str());
		return;
	}

	if (pid == 0) {
		// Child process
		close(out_pipe[0]); // Close read ends
		close(err_pipe[0]);

		dup2(out_pipe[1], STDOUT_FILENO); // Redirect stdout to out_pipe
		dup2(err_pipe[1], STDERR_FILENO); // Redirect stderr to err_pipe

		close(out_pipe[1]);
		close(err_pipe[1]);

		execlp("python3", "python3", _script_path.c_str(), NULL);
		perror("execlp python3"); // This will go to stderr pipe if exec fails
		_exit(127);
	}

	// Parent process
	close(out_pipe[1]); // Close write ends
	close(err_pipe[1]);
	_pipe_fd = out_pipe[0];
	_err_pipe_fd = err_pipe[0];
	_child_pid = pid;
	_running = true;

	_thread = std::thread(&WaylandGlobalShortcuts::WorkerThread, this);
}

void WaylandGlobalShortcuts::Stop() {
	_running = false;
	if (_child_pid != -1) {
		kill(_child_pid, SIGTERM);
		int status;
		waitpid(_child_pid, &status, 0); // Wait for child to exit to prevent zombies
		_child_pid = -1;
	}
	if (_pipe_fd != -1) {
		close(_pipe_fd);
		_pipe_fd = -1;
	}
	if (_err_pipe_fd != -1) {
		close(_err_pipe_fd);
		_err_pipe_fd = -1;
	}
	if (!_script_path.empty()) {
		unlink(_script_path.c_str());
		_script_path.clear();
	}
	if (_thread.joinable()) _thread.join();
}

void WaylandGlobalShortcuts::WorkerThread() {
	fprintf(stderr, "[WaylandShortcuts] Worker started, reading from python proxy...\n");

	char buf[1024];

	while (_running && (_pipe_fd != -1 || _err_pipe_fd != -1)) {
		fd_set fds;
		FD_ZERO(&fds);
		int max_fd = 0;

		if (_pipe_fd != -1) {
			FD_SET(_pipe_fd, &fds);
			if (_pipe_fd > max_fd) max_fd = _pipe_fd;
		}
		if (_err_pipe_fd != -1) {
			FD_SET(_err_pipe_fd, &fds);
			if (_err_pipe_fd > max_fd) max_fd = _err_pipe_fd;
		}

		if (max_fd == 0) break;

		int ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

		if (ret < 0) {
			if (errno == EINTR) continue;
			perror("[WaylandShortcuts] select failed");
			break;
		}

		auto read_and_process = [&](int fd, const char* prefix) -> bool {
			ssize_t n = read(fd, buf, sizeof(buf) - 1);
			if (n <= 0) { // EOF or error
				return false;
			}
			buf[n] = 0;

			// Split by lines and process
			char *line = strtok(buf, "\n");
			while(line) {
				process_line(line, prefix);
				line = strtok(NULL, "\n");
			}
			return true;
		};

		if (_pipe_fd != -1 && FD_ISSET(_pipe_fd, &fds)) {
			if (!read_and_process(_pipe_fd, "[Py STDOUT]")) {
				close(_pipe_fd);
				_pipe_fd = -1;
			}
		}

		if (_err_pipe_fd != -1 && FD_ISSET(_err_pipe_fd, &fds)) {
			if (!read_and_process(_err_pipe_fd, "[Py STDERR]")) {
				close(_err_pipe_fd);
				_err_pipe_fd = -1;
			}
		}
	}

	fprintf(stderr, "[WaylandShortcuts] Proxy pipes closed or EOF. Thread exiting.\n");
}

void WaylandGlobalShortcuts::process_line(char* buf, const char* prefix) {
	// Trim trailing newline which strtok doesn't remove if buffer ends with it
	size_t len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\r') buf[len - 1] = 0;

	if (strncmp(buf, "LOG:", 4) == 0) {
		fprintf(stderr, "[WaylandShortcuts] %s: %s\n", prefix, buf + 4);
		return;
	}

	if (strncmp(buf, "EVENT:", 6) == 0) {
		const char* id = buf + 6;

		if (!_focused) {
			fprintf(stderr, "[WaylandShortcuts] Ignored event (Unfocused): %s\n", id);
			return;
		}
		if (_paused) {
			fprintf(stderr, "[WaylandShortcuts] Ignored event (Paused): %s\n", id);
			return;
		}

		WORD vk = 0;
		DWORD mods = LEFT_CTRL_PRESSED;

		if (strcmp(id, "CtrlEnter") == 0) {
			vk = VK_RETURN;
		} else if (strcmp(id, "CtrlTab") == 0) {
			vk = VK_TAB;
		}

		if (vk != 0) {
			fprintf(stderr, "[WaylandShortcuts] Injecting VK=0x%x Mods=0x%x\n", vk, mods);

			auto now = std::chrono::steady_clock::now().time_since_epoch();
			_last_activity_ts = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

			INPUT_RECORD ir = {};
			ir.EventType = KEY_EVENT;
			ir.Event.KeyEvent.bKeyDown = TRUE;
			ir.Event.KeyEvent.wRepeatCount = 1;
			ir.Event.KeyEvent.wVirtualKeyCode = vk;
			ir.Event.KeyEvent.dwControlKeyState = mods;

			if (g_winport_con_in) {
				g_winport_con_in->Enqueue(&ir, 1);
				ir.Event.KeyEvent.bKeyDown = FALSE;
				g_winport_con_in->Enqueue(&ir, 1);
			}

			const char* id = buf + 6;

			if (!_focused) {
				fprintf(stderr, "[WaylandShortcuts] Ignored event (Unfocused): %s\n", id);
				return;
			}
			if (_paused) {
				fprintf(stderr, "[WaylandShortcuts] Ignored event (Paused): %s\n", id);
				return;
			}

			WORD vk = 0;
			DWORD mods = LEFT_CTRL_PRESSED;

			if (strcmp(id, "CtrlEnter") == 0) {
				vk = VK_RETURN;
			} else if (strcmp(id, "CtrlTab") == 0) {
				vk = VK_TAB;
			}

			if (vk != 0) {
				fprintf(stderr, "[WaylandShortcuts] Injecting VK=0x%x Mods=0x%x\n", vk, mods);

				auto now = std::chrono::steady_clock::now().time_since_epoch();
				_last_activity_ts = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

				INPUT_RECORD ir = {};
				ir.EventType = KEY_EVENT;
				ir.Event.KeyEvent.bKeyDown = TRUE;
				ir.Event.KeyEvent.wRepeatCount = 1;
				ir.Event.KeyEvent.wVirtualKeyCode = vk;
				ir.Event.KeyEvent.dwControlKeyState = mods;

				if (g_winport_con_in) {
					g_winport_con_in->Enqueue(&ir, 1);
					ir.Event.KeyEvent.bKeyDown = FALSE;
					g_winport_con_in->Enqueue(&ir, 1);
				}
			}
		}
	} else {
		// Log any other output from script
		fprintf(stderr, "[WaylandShortcuts] %s: %s\n", prefix, buf);
	}

	// If fgets returns NULL, pipe is broken or closed
	fprintf(stderr, "[WaylandShortcuts] Pipe closed or EOF.\n");

	// Don't close pipe_fp here if _pipe_fd is managed by Stop(),
	// but fdopen takes ownership so we should fclose it if we want to be clean.
	// However, Stop() might close the FD concurrently.
	// To be safe, we let Stop handle FD closing or detach here.
	// Since fdopen was created on _pipe_fd, fclose closes the FD too.
	// Let's set _pipe_fd to -1 to signal Stop() not to double close.
	_pipe_fd = -1;
	//fclose(pipe_fp);
}