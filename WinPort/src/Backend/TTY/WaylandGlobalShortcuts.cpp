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

	bool Load() {
		lib = dlopen("libdbus-1.so.3", RTLD_LAZY);
		if (!lib) lib = dlopen("libdbus-1.so", RTLD_LAZY);
		if (!lib) return false;

		#define BIND(n) if (!(n = (decltype(n))dlsym(lib, "dbus_" #n))) return false;
		BIND(error_init); BIND(error_free); BIND(bus_get_private);
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
		#undef BIND
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

void WaylandGlobalShortcuts::Start() {
	if (_running) return;
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

	// Signal: Response(u, a{sv})
	if (g_dbus.message_iter_init(msg, &args) &&
		g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_UINT32) {

		uint32_t code = 0;
		g_dbus.message_iter_get_basic(&args, &code);

		if (code == 0 && // Success
			g_dbus.message_iter_next(&args) &&
			g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {

			DBusMessageIter dict;
			g_dbus.message_iter_recurse(&args, &dict);
			while (g_dbus.message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
				DBusMessageIter entry;
				g_dbus.message_iter_recurse(&dict, &entry);
				const char* key = nullptr;
				g_dbus.message_iter_get_basic(&entry, &key);

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
	DBusError err;
	g_dbus.error_init(&err);

	DBusState state;
	state.conn = g_dbus.bus_get_private(DBUS_BUS_SESSION, &err);

	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "WaylandShortcuts: Bus Error: %s\n", err.message);
		g_dbus.error_free(&err);
		return;
	}

	_dbus = &state;

	// 1. CreateSession (Request/Response pattern)
	{
		DBusMessage* msg = g_dbus.message_new_method_call("org.freedesktop.portal.Desktop",
			"/org/freedesktop/portal/desktop", "org.freedesktop.portal.GlobalShortcuts", "CreateSession");

		DBusMessageIter args, array;
		g_dbus.message_iter_init_append(msg, &args);
		g_dbus.message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &array);
		AppendDictEntry(&array, "session_handle_token", "far2l_session");
		g_dbus.message_iter_close_container(&args, &array);

		DBusPendingCall* pending;
		const char* request_path = nullptr;

		if (g_dbus.connection_send_with_reply(state.conn, msg, &pending, -1)) {
			g_dbus.connection_flush(state.conn);
			g_dbus.message_unref(msg);
			g_dbus.pending_call_block(pending);
			DBusMessage* reply = g_dbus.pending_call_steal_reply(pending);

			if (reply) {
				// Method returns (o) - the request object path
				DBusMessageIter r_iter;
				if (g_dbus.message_iter_init(reply, &r_iter) &&
					g_dbus.message_iter_get_arg_type(&r_iter) == DBUS_TYPE_OBJECT_PATH) {
					g_dbus.message_iter_get_basic(&r_iter, &request_path);
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
		g_dbus.connection_close(state.conn);
		g_dbus.connection_unref(state.conn);
		return;
	}

	// 2. BindShortcuts
	{
		DBusMessage* msg = g_dbus.message_new_method_call("org.freedesktop.portal.Desktop",
			"/org/freedesktop/portal/desktop", "org.freedesktop.portal.GlobalShortcuts", "BindShortcuts");

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

		// Asynchronous send is fine, we just need to start loop
		if (g_dbus.connection_send_with_reply(state.conn, msg, NULL, -1)) {
			g_dbus.connection_flush(state.conn);
		}
		g_dbus.message_unref(msg);
	}

	// 3. Main Loop
	while (_running && g_dbus.connection_read_write_dispatch(state.conn, 100)) {
		DBusMessage* msg = g_dbus.connection_pop_message(state.conn);
		if (!msg) continue;

		if (g_dbus.message_is_signal(msg, "org.freedesktop.portal.GlobalShortcuts", "Activated") && _focused) {
			DBusMessageIter args;
			if (g_dbus.message_iter_init(msg, &args) &&
				g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH) {
				g_dbus.message_iter_next(&args); // Skip session handle
				if (g_dbus.message_iter_get_arg_type(&args) == DBUS_TYPE_STRING) {
					const char* id = nullptr;
					g_dbus.message_iter_get_basic(&args, &id);
					if (id) {
						for (const auto& s : g_shortcuts) {
							if (strcmp(s.id, id) == 0) {
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
					}
				}
			}
		}
		g_dbus.message_unref(msg);
	}

	g_dbus.connection_close(state.conn);
	g_dbus.connection_unref(state.conn);
}