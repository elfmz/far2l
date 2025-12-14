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

	// 2. Create pipe
	int pipe_fds[2];
	if (pipe(pipe_fds) == -1) {
		perror("[WaylandShortcuts] pipe");
		unlink(_script_path.c_str());
		return;
	}

	// 3. Fork and Exec
	pid_t pid = fork();
	if (pid == -1) {
		perror("[WaylandShortcuts] fork");
		close(pipe_fds[0]);
		close(pipe_fds[1]);
		unlink(_script_path.c_str());
		return;
	}

	if (pid == 0) {
		// Child process
		close(pipe_fds[0]); // Close read end
		dup2(pipe_fds[1], STDOUT_FILENO); // Redirect stdout to pipe
		close(pipe_fds[1]);
		
		// Optional: Redirect stderr to null or keep it for debugging
		// int null_fd = open("/dev/null", O_WRONLY);
		// dup2(null_fd, STDERR_FILENO);
		// close(null_fd);

		execlp("python3", "python3", _script_path.c_str(), NULL);
		perror("execlp python3");
		_exit(127);
	}

	// Parent process
	close(pipe_fds[1]); // Close write end
	_pipe_fd = pipe_fds[0];
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
	if (!_script_path.empty()) {
		unlink(_script_path.c_str());
		_script_path.clear();
	}
	if (_thread.joinable()) _thread.join();
}

void WaylandGlobalShortcuts::WorkerThread() {
	fprintf(stderr, "[WaylandShortcuts] Worker started, reading from python proxy...\n");
	
	FILE* pipe_fp = fdopen(_pipe_fd, "r");
	if (!pipe_fp) {
		perror("[WaylandShortcuts] fdopen failed");
		return;
	}

	char buf[256];
	while (_running && fgets(buf, sizeof(buf), pipe_fp)) {
		// Trim newline
		size_t len = strlen(buf);
		while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
			buf[len-1] = 0;
			len--;
		}

		if (strncmp(buf, "LOG:", 4) == 0) {
			fprintf(stderr, "[WaylandShortcuts] Py: %s\n", buf + 4);
			continue;
		}

		if (strncmp(buf, "EVENT:", 6) == 0) {
			const char* id = buf + 6;
			
			if (!_focused) {
				fprintf(stderr, "[WaylandShortcuts] Ignored event (Unfocused): %s\n", id);
				continue;
			}
			if (_paused) {
				fprintf(stderr, "[WaylandShortcuts] Ignored event (Paused): %s\n", id);
				continue;
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
	fclose(pipe_fp); 
}