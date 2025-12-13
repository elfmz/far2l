#include "WaylandGlobalShortcuts.h"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <map>
#include <vector>
#include <string>
#include "WinCompat.h"
#include "Backend.h"

// -----------------------------------------------------------------------
// Dynamic DBus Binding
// -----------------------------------------------------------------------

#define DBUS_BUS_SESSION 1
#define DBUS_TYPE_ARRAY 'a'
#define DBUS_TYPE_VARIANT 'v'
#define DBUS_TYPE_STRING 's'
#define DBUS_TYPE_OBJECT_PATH 'o'
#define DBUS_TYPE_DICT_ENTRY 'e'
#define DBUS_TYPE_STRUCT 'r'
#define DBUS_TYPE_UINT32 'u'
#define DBUS_TYPE_INVALID 0
#define DBUS_MESSAGE_TYPE_METHOD_RETURN 2
#define DBUS_MESSAGE_TYPE_ERROR 3

typedef struct DBusConnection DBusConnection;
typedef struct DBusMessage DBusMessage;
typedef struct DBusPendingCall DBusPendingCall;

struct DBusMessageIter {
	void *dummy1;
	void *dummy2;
	uint32_t dummy3;
	int dummy4;
	int dummy5;
	int dummy6;
	int dummy7;
	int dummy8;
	int dummy9;
	int dummy10;
	int dummy11;
	int dummy12;
	void *dummy13;
	void *dummy14;
	void *dummy15;
	void *dummy16;
	void *dummy17;
};

struct DBusError {
	const char *name;
	const char *message;
	unsigned int dummy1 : 1;
	unsigned int dummy2 : 1;
};

static bool dbus_error_is_set(const DBusError* error) {
	return error->name != nullptr;
}

struct DBusLib {
	void* lib;
	void (*error_init)(DBusError*);
	void (*error_free)(DBusError*);
	DBusConnection* (*bus_get_private)(int, DBusError*);
	DBusConnection* (*bus_get)(int, DBusError*);
	void (*connection_close)(DBusConnection*);
	void (*connection_unref)(DBusConnection*);
	void (*connection_flush)(DBusConnection*);
	bool (*connection_read_write_dispatch)(DBusConnection*, int);
	DBusMessage* (*message_new_method_call)(const char*, const char*, const char*, const char*);
	void (*message_unref)(DBusMessage*);
	bool (*message_iter_init)(DBusMessage*, DBusMessageIter*);
	bool (*message_iter_init_append)(DBusMessage*, DBusMessageIter*);
	bool (*message_iter_open_container)(DBusMessageIter*, int, const char*, DBusMessageIter*);
	bool (*message_iter_close_container)(DBusMessageIter*, DBusMessageIter*);
	bool (*message_iter_append_basic)(DBusMessageIter*, int, const void*);
	int (*message_iter_get_arg_type)(DBusMessageIter*);
	void (*message_iter_recurse)(DBusMessageIter*, DBusMessageIter*);
	bool (*message_iter_next)(DBusMessageIter*);
	void (*message_iter_get_basic)(DBusMessageIter*, void*);
	bool (*connection_send_with_reply)(DBusConnection*, DBusMessage*, DBusPendingCall**, int);
	bool (*pending_call_set_notify)(DBusPendingCall*, void (*)(DBusPendingCall*, void*), void*, void*);
	void (*pending_call_block)(DBusPendingCall*);
	DBusMessage* (*pending_call_steal_reply)(DBusPendingCall*);
	void (*pending_call_unref)(DBusPendingCall*);
	DBusMessage* (*message_pop_message)(DBusConnection*);
	DBusMessage* (*connection_pop_message)(DBusConnection*);
	bool (*message_is_signal)(DBusMessage*, const char*, const char*);
	void (*bus_add_match)(DBusConnection*, const char*, DBusError*);
	const char* (*message_get_path)(DBusMessage*);
	bool (*threads_init_default)(void);
	int (*message_get_type)(DBusMessage*);
	const char* (*message_get_error_name)(DBusMessage*);
	void (*message_set_auto_start)(DBusMessage*, int);

	bool Load() {
		fprintf(stderr, "[WaylandShortcuts] DBusLib::Load: Attempting to load libdbus-1...\n");
		lib = dlopen("libdbus-1.so.3", RTLD_LAZY);
		if (!lib) {
			fprintf(stderr, "[WaylandShortcuts] dlopen('libdbus-1.so.3') failed: %s\n", dlerror());
			lib = dlopen("libdbus-1.so", RTLD_LAZY);
		}
		if (!lib) {
			fprintf(stderr, "[WaylandShortcuts] dlopen('libdbus-1.so') failed: %s\n", dlerror());
			return false;
		}
		fprintf(stderr, "[WaylandShortcuts] DBusLib::Load: Success. Lib handle: %p\n", lib);

		#define BIND(n) if (!(n = (decltype(n))dlsym(lib, "dbus_" #n))) return false;
		BIND(error_init); BIND(error_free); BIND(bus_get_private);
		BIND(bus_get);
		BIND(connection_close); BIND(connection_unref); BIND(connection_flush);
		BIND(connection_read_write_dispatch);
		BIND(message_new_method_call); BIND(message_unref);
		BIND(message_iter_init); BIND(message_iter_init_append);
		BIND(message_iter_open_container); BIND(message_iter_close_container);
		BIND(message_iter_append_basic);
		BIND(message_iter_get_arg_type); BIND(message_iter_recurse);
		BIND(message_iter_next); BIND(message_iter_get_basic);
		BIND(connection_send_with_reply); BIND(pending_call_set_notify);
		BIND(pending_call_block); BIND(pending_call_steal_reply);
		BIND(pending_call_unref); BIND(connection_pop_message);
		BIND(message_is_signal);
		BIND(bus_add_match);
		BIND(message_get_path);
		BIND(threads_init_default);
		BIND(message_get_type);
		BIND(message_get_error_name);
		BIND(message_set_auto_start);
		BIND(message_set_auto_start);
		#undef BIND

		if (threads_init_default) {
			fprintf(stderr, "[WaylandShortcuts] Initializing DBus threads...\n");
			if (!threads_init_default()) {
				fprintf(stderr, "[WaylandShortcuts] WARNING: dbus_threads_init_default failed!\n");
			}
		}
		return true;
	}

	void Close() { if(lib) dlclose(lib); lib = nullptr; }
};

static DBusLib g_dbus;

// -----------------------------------------------------------------------
// Shortcuts Definition
// -----------------------------------------------------------------------

struct ShortcutDef {
	const char* id;
	const char* desc;
	const char* trigger;
	WORD vk;
	DWORD mods;
};

// Helper macros
#define SC(id, desc, trig, vk, mod) { id, desc, trig, vk, mod }

// For function keys and navigation - include Shift
#define MOD_KEYS_FULL(key, vk) \
	SC("Ctrl" key, "Ctrl+" key, "Control+" key, vk, LEFT_CTRL_PRESSED), \
	SC("Alt" key, "Alt+" key, "Alt+" key, vk, LEFT_ALT_PRESSED), \
	SC("Shift" key, "Shift+" key, "Shift+" key, vk, SHIFT_PRESSED), \
	SC("CtrlShift" key, "Ctrl+Shift+" key, "Control+Shift+" key, vk, LEFT_CTRL_PRESSED | SHIFT_PRESSED), \
	SC("AltShift" key, "Alt+Shift+" key, "Alt+Shift+" key, vk, LEFT_ALT_PRESSED | SHIFT_PRESSED), \
	SC("CtrlAlt" key, "Ctrl+Alt+" key, "Control+Alt+" key, vk, LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED)

// For printable keys - exclude Shift as it is handled by terminal text input
#define MOD_KEYS_TYPING(key, vk) \
	SC("Ctrl" key, "Ctrl+" key, "Control+" key, vk, LEFT_CTRL_PRESSED), \
	SC("Alt" key, "Alt+" key, "Alt+" key, vk, LEFT_ALT_PRESSED), \
	SC("CtrlShift" key, "Ctrl+Shift+" key, "Control+Shift+" key, vk, LEFT_CTRL_PRESSED | SHIFT_PRESSED), \
	SC("AltShift" key, "Alt+Shift+" key, "Alt+Shift+" key, vk, LEFT_ALT_PRESSED | SHIFT_PRESSED), \
	SC("CtrlAlt" key, "Ctrl+Alt+" key, "Control+Alt+" key, vk, LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED)

static const ShortcutDef g_shortcuts[] = {
	// Function Keys (F1-F12) and their combinations
	MOD_KEYS_FULL("F1", VK_F1),
	MOD_KEYS_FULL("F2", VK_F2),
	MOD_KEYS_FULL("F3", VK_F3),
	MOD_KEYS_FULL("F4", VK_F4),
	MOD_KEYS_FULL("F5", VK_F5),
	MOD_KEYS_FULL("F6", VK_F6),
	MOD_KEYS_FULL("F7", VK_F7),
	MOD_KEYS_FULL("F8", VK_F8),
	MOD_KEYS_FULL("F9", VK_F9),
	MOD_KEYS_FULL("F10", VK_F10),
	MOD_KEYS_FULL("F11", VK_F11),
	MOD_KEYS_FULL("F12", VK_F12),

	// Numbers 0-9
	MOD_KEYS_TYPING("0", '0'), MOD_KEYS_TYPING("1", '1'), MOD_KEYS_TYPING("2", '2'), MOD_KEYS_TYPING("3", '3'),
	MOD_KEYS_TYPING("4", '4'), MOD_KEYS_TYPING("5", '5'), MOD_KEYS_TYPING("6", '6'), MOD_KEYS_TYPING("7", '7'),
	MOD_KEYS_TYPING("8", '8'), MOD_KEYS_TYPING("9", '9'),

	// Navigation & Editing
	MOD_KEYS_FULL("Up", VK_UP),
	MOD_KEYS_FULL("Down", VK_DOWN),
	MOD_KEYS_FULL("Left", VK_LEFT),
	MOD_KEYS_FULL("Right", VK_RIGHT),
	MOD_KEYS_FULL("Home", VK_HOME),
	MOD_KEYS_FULL("End", VK_END),
	MOD_KEYS_FULL("PgUp", VK_PRIOR),
	MOD_KEYS_FULL("PgDn", VK_NEXT),
	MOD_KEYS_FULL("Ins", VK_INSERT),
	MOD_KEYS_FULL("Del", VK_DELETE),
	MOD_KEYS_TYPING("BackSpace", VK_BACK),
	MOD_KEYS_TYPING("Tab", VK_TAB),
	MOD_KEYS_TYPING("Enter", VK_RETURN),
	MOD_KEYS_TYPING("Space", VK_SPACE),

	// Alpha Keys (A-Z)
	MOD_KEYS_TYPING("A", 'A'), MOD_KEYS_TYPING("B", 'B'), MOD_KEYS_TYPING("C", 'C'), MOD_KEYS_TYPING("D", 'D'),
	MOD_KEYS_TYPING("E", 'E'), MOD_KEYS_TYPING("F", 'F'), MOD_KEYS_TYPING("G", 'G'), MOD_KEYS_TYPING("H", 'H'),
	MOD_KEYS_TYPING("I", 'I'), MOD_KEYS_TYPING("J", 'J'), MOD_KEYS_TYPING("K", 'K'), MOD_KEYS_TYPING("L", 'L'),
	MOD_KEYS_TYPING("M", 'M'), MOD_KEYS_TYPING("N", 'N'), MOD_KEYS_TYPING("O", 'O'), MOD_KEYS_TYPING("P", 'P'),
	MOD_KEYS_TYPING("Q", 'Q'), MOD_KEYS_TYPING("R", 'R'), MOD_KEYS_TYPING("S", 'S'), MOD_KEYS_TYPING("T", 'T'),
	MOD_KEYS_TYPING("U", 'U'), MOD_KEYS_TYPING("V", 'V'), MOD_KEYS_TYPING("W", 'W'), MOD_KEYS_TYPING("X", 'X'),
	MOD_KEYS_TYPING("Y", 'Y'), MOD_KEYS_TYPING("Z", 'Z'),

	// Punctuation and Special Keys
	MOD_KEYS_TYPING("Grave", VK_OEM_3),      // ` ~
	MOD_KEYS_TYPING("Minus", VK_OEM_MINUS),  // - _
	MOD_KEYS_TYPING("Equal", VK_OEM_PLUS),   // = +
	MOD_KEYS_TYPING("LBracket", VK_OEM_4),   // [ {
	MOD_KEYS_TYPING("RBracket", VK_OEM_6),   // ] }
	MOD_KEYS_TYPING("Backslash", VK_OEM_5),  // \ |
	MOD_KEYS_TYPING("Semicolon", VK_OEM_1),  // ; :
	MOD_KEYS_TYPING("Quote", VK_OEM_7),      // ' "
	MOD_KEYS_TYPING("Comma", VK_OEM_COMMA),  // , <
	MOD_KEYS_TYPING("Period", VK_OEM_PERIOD),// . >
	MOD_KEYS_TYPING("Slash", VK_OEM_2)       // / ?
	,
	// Numpad
	MOD_KEYS_TYPING("KP_0", VK_NUMPAD0), MOD_KEYS_TYPING("KP_1", VK_NUMPAD1),
	MOD_KEYS_TYPING("KP_2", VK_NUMPAD2), MOD_KEYS_TYPING("KP_3", VK_NUMPAD3),
	MOD_KEYS_TYPING("KP_4", VK_NUMPAD4), MOD_KEYS_TYPING("KP_5", VK_NUMPAD5),
	MOD_KEYS_TYPING("KP_6", VK_NUMPAD6), MOD_KEYS_TYPING("KP_7", VK_NUMPAD7),
	MOD_KEYS_TYPING("KP_8", VK_NUMPAD8), MOD_KEYS_TYPING("KP_9", VK_NUMPAD9),
	MOD_KEYS_TYPING("KP_Multiply", VK_MULTIPLY),
	MOD_KEYS_TYPING("KP_Add", VK_ADD),
	MOD_KEYS_TYPING("KP_Subtract", VK_SUBTRACT),
	MOD_KEYS_TYPING("KP_Decimal", VK_DECIMAL),
	MOD_KEYS_TYPING("KP_Divide", VK_DIVIDE)
};

// -----------------------------------------------------------------------
// Logic
// -----------------------------------------------------------------------

struct WaylandGlobalShortcuts::DBusState {
	DBusConnection* conn = nullptr;
	std::string session_handle;
};

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
	fprintf(stderr, "[WaylandShortcuts] Start() called\n");
	if (!g_dbus.lib && !g_dbus.Load()) {
		fprintf(stderr, "WaylandShortcuts: Failed to load libdbus-1\n");
		return;
	}

	_running = true;
	_thread = std::thread(&WaylandGlobalShortcuts::WorkerThread, this);
}

void WaylandGlobalShortcuts::Stop() {
	_running = false;
	if (_thread.joinable()) _thread.join();
}

static void AppendDictEntry(DBusMessageIter* iter, const char* key, const char* val) {
	DBusMessageIter entry, variant;
	g_dbus.message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
	g_dbus.message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
	g_dbus.message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
	g_dbus.message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
	g_dbus.message_iter_close_container(&entry, &variant);
	g_dbus.message_iter_close_container(iter, &entry);
}

static std::string ExtractSessionHandleFromResponse(DBusMessage* msg) {
	DBusMessageIter args, variant;
	const char* path = nullptr;
	fprintf(stderr, "[WaylandShortcuts] ExtractSessionHandleFromResponse: Parsing signal...\n");

	// Signal: Response(u, a{sv})
	if (g_dbus.message_iter_init(msg, &args) &&
		g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_UINT32) {

		uint32_t code = 0;
		g_dbus.message_iter_get_basic(&args, &code);
		fprintf(stderr, "[WaylandShortcuts] Response Code: %u\n", code);

		if (code == 0 && // Success
			g_dbus.message_iter_next(&args) &&
			g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {

			fprintf(stderr, "[WaylandShortcuts] Iterating response dictionary...\n");

			DBusMessageIter dict;
			g_dbus.message_iter_recurse(&args, &dict);
			while (g_dbus.message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
				DBusMessageIter entry;
				g_dbus.message_iter_recurse(&dict, &entry);
				const char* key = nullptr;
				g_dbus.message_iter_get_basic(&entry, &key);
				fprintf(stderr, "[WaylandShortcuts] Found Key: '%s'\n", key ? key : "(null)");

				if (key && strcmp(key, "session_handle") == 0) {
					g_dbus.message_iter_next(&entry);
					g_dbus.message_iter_recurse(&entry, &variant);
					if (g_dbus.message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING) {
						g_dbus.message_iter_get_basic(&variant, &path);
					}
				}
				g_dbus.message_iter_next(&dict);
			}
		}
	}
	return path ? std::string(path) : "";
}

void WaylandGlobalShortcuts::WorkerThread() {
	// Debug environment
	const char* xdg_session = getenv("XDG_SESSION_TYPE");
	const char* w_display = getenv("WAYLAND_DISPLAY");
	const char* dbus_addr = getenv("DBUS_SESSION_BUS_ADDRESS");
	const char* xdg_data = getenv("XDG_DATA_DIRS");

	fprintf(stderr, "[WaylandShortcuts] Environment:\n");
	fprintf(stderr, "  XDG_SESSION_TYPE=%s\n", xdg_session ? xdg_session : "(null)");
	fprintf(stderr, "  WAYLAND_DISPLAY=%s\n", w_display ? w_display : "(null)");
	fprintf(stderr, "  DBUS_SESSION_BUS_ADDRESS=%s\n", dbus_addr ? dbus_addr : "(null)");
	fprintf(stderr, "  XDG_DATA_DIRS=%s\n", xdg_data ? xdg_data : "(null)");

	if (!dbus_addr) {
		fprintf(stderr, "[WaylandShortcuts] ERROR: DBUS_SESSION_BUS_ADDRESS is missing.\n");
		return;
	}

	DBusError err;
	g_dbus.error_init(&err);
	fprintf(stderr, "[WaylandShortcuts] WorkerThread: Initializing DBus connection (Shared)...\n");
	fflush(stderr);

	DBusState state;
	// Use shared connection (like Qt/OBS does)
	state.conn = g_dbus.bus_get(DBUS_BUS_SESSION, &err);

	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "[WaylandShortcuts] Bus Error: Name='%s', Msg='%s'\n", err.name, err.message);
		g_dbus.error_free(&err);
		return;
	}
	fprintf(stderr, "[WaylandShortcuts] DBus connected successfully.\n");
	fflush(stderr);

	_dbus = &state;

	// 0. Explicit Activation (Try to force start the portal)
	{
		fprintf(stderr, "[WaylandShortcuts] Step 0: Explicitly starting org.freedesktop.portal.Desktop...\n");
		DBusMessage* msg = g_dbus.message_new_method_call("org.freedesktop.DBus",
			"/org/freedesktop/DBus", "org.freedesktop.DBus", "StartServiceByName");

		DBusMessageIter args;
		g_dbus.message_iter_init_append(msg, &args);
		const char* name = "org.freedesktop.portal.Desktop";
		uint32_t flags = 0;
		g_dbus.message_iter_append_basic(&args, DBUS_TYPE_STRING, &name);
		g_dbus.message_iter_append_basic(&args, DBUS_TYPE_UINT32, &flags);

		DBusPendingCall* pending;
		if (g_dbus.connection_send_with_reply(state.conn, msg, &pending, 2000)) {
			g_dbus.connection_flush(state.conn);
			g_dbus.message_unref(msg);
			g_dbus.pending_call_block(pending);
			DBusMessage* reply = g_dbus.pending_call_steal_reply(pending);

			if (reply) {
				int msg_type = g_dbus.message_get_type(reply);
				if (msg_type == DBUS_MESSAGE_TYPE_ERROR) {
					const char* err_name = g_dbus.message_get_error_name(reply);
					const char* err_msg = "Unknown error";
					DBusMessageIter r_iter;
					if (g_dbus.message_iter_init(reply, &r_iter) &&
						g_dbus.message_iter_get_arg_type(&r_iter) == DBUS_TYPE_STRING) {
						g_dbus.message_iter_get_basic(&r_iter, &err_msg);
					}
					fprintf(stderr, "[WaylandShortcuts] StartServiceByName FAILED: %s - %s\n", err_name, err_msg);
				} else {
					uint32_t ret = 0;
					DBusMessageIter r_iter;
					if (g_dbus.message_iter_init(reply, &r_iter) &&
						g_dbus.message_iter_get_arg_type(&r_iter) == DBUS_TYPE_UINT32) {
						g_dbus.message_iter_get_basic(&r_iter, &ret);
						fprintf(stderr, "[WaylandShortcuts] StartServiceByName SUCCESS. Ret: %u (1=started, 2=already running)\n", ret);
					} else {
						fprintf(stderr, "[WaylandShortcuts] StartServiceByName returned OK but weird signature.\n");
					}
				}
				g_dbus.message_unref(reply);
			} else {
				fprintf(stderr, "[WaylandShortcuts] StartServiceByName timed out.\n");
			}
			g_dbus.pending_call_unref(pending);
		} else {
			g_dbus.message_unref(msg);
		}
	}

	// 1. CreateSession (Request/Response pattern)
	// DEBUG: Check who is on the bus
	{
		fprintf(stderr, "[WaylandShortcuts] DEBUG: Listing Bus Names...\n");
		DBusMessage* msg = g_dbus.message_new_method_call("org.freedesktop.DBus",
			"/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames");

		DBusPendingCall* pending;
		if (g_dbus.connection_send_with_reply(state.conn, msg, &pending, 2000)) {
			g_dbus.connection_flush(state.conn);
			g_dbus.message_unref(msg);
			g_dbus.pending_call_block(pending);
			DBusMessage* reply = g_dbus.pending_call_steal_reply(pending);
			if (reply) {
				DBusMessageIter args, arr;
				if (g_dbus.message_iter_init(reply, &args) &&
					g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {

					g_dbus.message_iter_recurse(&args, &arr);
					int count = 0;
					while (g_dbus.message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING) {
						const char* name = nullptr;
						g_dbus.message_iter_get_basic(&arr, &name);
						// Print only first few and portal-related
						if (count < 5 || (name && strstr(name, "portal"))) {
							fprintf(stderr, "[WaylandShortcuts] Bus Name: %s\n", name);
						}
						count++;
						g_dbus.message_iter_next(&arr);
					}
					fprintf(stderr, "[WaylandShortcuts] Total Bus Names: %d\n", count);
				}
				g_dbus.message_unref(reply);
			} else {
				fprintf(stderr, "[WaylandShortcuts] ListNames failed (NULL reply)\n");
			}
			g_dbus.pending_call_unref(pending);
		} else {
			g_dbus.message_unref(msg);
			fprintf(stderr, "[WaylandShortcuts] Failed to send ListNames\n");
		}
	}
	{
		DBusMessage* msg = g_dbus.message_new_method_call("org.freedesktop.portal.Desktop",
			"/org/freedesktop/portal/desktop", "org.freedesktop.portal.GlobalShortcuts", "CreateSession");
		if (g_dbus.message_set_auto_start) {
			g_dbus.message_set_auto_start(msg, 1);
		}
		fprintf(stderr, "[WaylandShortcuts] Step 1: Sending CreateSession...\n");

		DBusMessageIter args, array;
		g_dbus.message_iter_init_append(msg, &args);
		g_dbus.message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &array);

		char token_buf[64];
		snprintf(token_buf, sizeof(token_buf), "far2l_session_%d", getpid());
		AppendDictEntry(&array, "session_handle_token", token_buf);

		g_dbus.message_iter_close_container(&args, &array);

		DBusPendingCall* pending;
		const char* request_path = nullptr;

		// Use 2000ms timeout. If portal is broken/missing (Cinnamon), it might hang forever on -1.
		if (g_dbus.connection_send_with_reply(state.conn, msg, &pending, 2000)) {
			g_dbus.connection_flush(state.conn);
			g_dbus.message_unref(msg);
			g_dbus.pending_call_block(pending);
			DBusMessage* reply = g_dbus.pending_call_steal_reply(pending);
			fprintf(stderr, "[WaylandShortcuts] CreateSession reply received.\n");

			if (reply) {
				int msg_type = g_dbus.message_get_type(reply);
				if (msg_type == DBUS_MESSAGE_TYPE_ERROR) {
					const char* err_name = g_dbus.message_get_error_name(reply);
					const char* err_msg = "Unknown error";
					DBusMessageIter r_iter;
					if (g_dbus.message_iter_init(reply, &r_iter) &&
						g_dbus.message_iter_get_arg_type(&r_iter) == DBUS_TYPE_STRING) {
						g_dbus.message_iter_get_basic(&r_iter, &err_msg);
					}
					fprintf(stderr, "[WaylandShortcuts] DBUS ERROR: %s - %s\n", err_name, err_msg);
				}
				else if (msg_type == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
					DBusMessageIter r_iter;
					if (g_dbus.message_iter_init(reply, &r_iter) &&
						g_dbus.message_iter_get_arg_type(&r_iter) == DBUS_TYPE_OBJECT_PATH) {
						g_dbus.message_iter_get_basic(&r_iter, &request_path);
						fprintf(stderr, "[WaylandShortcuts] Request Object Path: %s\n", request_path);
					} else {
						fprintf(stderr, "[WaylandShortcuts] ERROR: Unexpected reply signature.\n");
					}
				} else {
					fprintf(stderr, "[WaylandShortcuts] Unexpected message type: %d\n", msg_type);
				}
				// Keep reply alive while we use request_path string reference (or copy it)
				// For safety/simplicity in this scoped block, we trust request_path points to reply internal buf.
				// But we must subscribe before unref.

				if (request_path) {
					char match_rule[512];
					snprintf(match_rule, sizeof(match_rule),
						"type='signal',interface='org.freedesktop.portal.Request',member='Response',path='%s'",
						request_path);

					g_dbus.bus_add_match(state.conn, match_rule, NULL);
					fprintf(stderr, "[WaylandShortcuts] Match rule added: %s. Waiting for signal...\n", match_rule);
					g_dbus.connection_flush(state.conn);

					// Now wait for the signal
					while (state.session_handle.empty() && _running) {
						g_dbus.connection_read_write_dispatch(state.conn, 100);
						DBusMessage* sig = g_dbus.connection_pop_message(state.conn);
						if (sig) {
							if (g_dbus.message_is_signal(sig, "org.freedesktop.portal.Request", "Response") &&
								strcmp(g_dbus.message_get_path(sig), request_path) == 0) {
								state.session_handle = ExtractSessionHandleFromResponse(sig);
							}
							else {
								// fprintf(stderr, "[WaylandShortcuts] Ignored signal: %s\n", g_dbus.message_get_path(sig));
							}
							g_dbus.message_unref(sig);
						}
					}
				}
				g_dbus.message_unref(reply);
			}
			g_dbus.pending_call_unref(pending);
		} else {
			g_dbus.message_unref(msg);
		}
	}

	if (state.session_handle.empty()) {
		fprintf(stderr, "WaylandShortcuts: Failed to get session handle\n");
		fprintf(stderr, "[WaylandShortcuts] CRITICAL: Session handle is empty. Aborting.\n");
		// Shared connection must not be closed
		g_dbus.connection_unref(state.conn);
		return;
	}

	// 2. BindShortcuts
	{
		DBusMessage* msg = g_dbus.message_new_method_call("org.freedesktop.portal.Desktop",
			"/org/freedesktop/portal/desktop", "org.freedesktop.portal.GlobalShortcuts", "BindShortcuts");
		fprintf(stderr, "[WaylandShortcuts] Step 2: Sending BindShortcuts (Handle: %s)...\n", state.session_handle.c_str());

		DBusMessageIter args, shortcuts_arr, opts_arr;
		g_dbus.message_iter_init_append(msg, &args);

		const char* handle_cstr = state.session_handle.c_str();
		g_dbus.message_iter_append_basic(&args, DBUS_TYPE_OBJECT_PATH, &handle_cstr);

		// shortcuts a(sa{sv})
		g_dbus.message_iter_open_container(&args, DBUS_TYPE_ARRAY, "(sa{sv})", &shortcuts_arr);
		for (const auto& s : g_shortcuts) {
			DBusMessageIter struct_iter, dict_iter;
			g_dbus.message_iter_open_container(&shortcuts_arr, DBUS_TYPE_STRUCT, NULL, &struct_iter);
			g_dbus.message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &s.id);
			g_dbus.message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
			AppendDictEntry(&dict_iter, "description", s.desc);
			AppendDictEntry(&dict_iter, "preferred_trigger", s.trigger);
			g_dbus.message_iter_close_container(&struct_iter, &dict_iter);
			g_dbus.message_iter_close_container(&shortcuts_arr, &struct_iter);
		}
		g_dbus.message_iter_close_container(&args, &shortcuts_arr);

		const char* parent = "";
		g_dbus.message_iter_append_basic(&args, DBUS_TYPE_STRING, &parent);

		// options a{sv}
		g_dbus.message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &opts_arr);
		g_dbus.message_iter_close_container(&args, &opts_arr);

		// Asynchronous send is fine, we just need to start loop. 2000ms timeout.
		if (g_dbus.connection_send_with_reply(state.conn, msg, NULL, 2000)) {
			g_dbus.connection_flush(state.conn);
		}
		g_dbus.message_unref(msg);
	}

	// 3. Main Loop
	while (_running && g_dbus.connection_read_write_dispatch(state.conn, 100)) {
		DBusMessage* msg = g_dbus.connection_pop_message(state.conn);
		if (!msg) continue;

		if (g_dbus.message_is_signal(msg, "org.freedesktop.portal.GlobalShortcuts", "Activated")) {
			if (!_focused) {
				fprintf(stderr, "[WaylandShortcuts] Ignored 'Activated' signal (Window not focused)\n");
			} else if (_paused) {
				fprintf(stderr, "[WaylandShortcuts] Ignored 'Activated' signal (Mechanism Paused)\n");
			} else {
				DBusMessageIter args;
				if (g_dbus.message_iter_init(msg, &args) &&
					g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH) {
					g_dbus.message_iter_next(&args); // Skip session handle
					if (g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_STRING) {
						const char* id = nullptr;
						g_dbus.message_iter_get_basic(&args, &id);
						if (id) {
							fprintf(stderr, "[WaylandShortcuts] Activated ID: '%s'\n", id);
							bool found = false;
							for (const auto& s : g_shortcuts) {
								if (strcmp(s.id, id) == 0) {
									fprintf(stderr, "[WaylandShortcuts] Match! Injecting VK=0x%x Mods=0x%x\n", s.vk, s.mods);
									found = true;

									auto now = std::chrono::steady_clock::now().time_since_epoch();
									_last_activity_ts = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

									INPUT_RECORD ir = {};

									ir.EventType = KEY_EVENT;
									ir.Event.KeyEvent.bKeyDown = TRUE;
									ir.Event.KeyEvent.wRepeatCount = 1;
									ir.Event.KeyEvent.wVirtualKeyCode = s.vk;
									ir.Event.KeyEvent.dwControlKeyState = s.mods;

									if (g_winport_con_in) {
										g_winport_con_in->Enqueue(&ir, 1);
										ir.Event.KeyEvent.bKeyDown = FALSE;
										g_winport_con_in->Enqueue(&ir, 1);
									}
									break;
								}
							}
							if (!found) {
								fprintf(stderr, "[WaylandShortcuts] WARNING: ID '%s' not found in internal table.\n", id);
							}
						} else {
							fprintf(stderr, "[WaylandShortcuts] Activated signal has null ID\n");
						}
					}
				}
			}
		}

		g_dbus.message_unref(msg);
	}

	// Shared connection must not be closed
	g_dbus.connection_unref(state.conn);
}