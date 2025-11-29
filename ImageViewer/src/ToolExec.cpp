#include <unordered_set>
#include "Common.h"
#include "ToolExec.h"
#include "Settings.h"

// how long msec wait before showing progress message window
#define COMMAND_TIMEOUT_BEFORE_MESSAGE 300
// how long msec wait between checking for cancel signalled while command is running
#define COMMAND_TIMEOUT_CHECK_CANCEL 100
// how long msec wait after gracefull kill before doing kill -9
#define COMMAND_TIMEOUT_HARD_KILL 300

// keep following settings across plugin invokations
static std::unordered_set<std::wstring> s_warned_tools;
static std::atomic<int> s_in_progress_dialog{0};


void ToolExec::KillAndWait()
{
	KillSoftly();
	if (!Wait(COMMAND_TIMEOUT_HARD_KILL)) {
		KillHardly();
		Wait();
	}
}

ToolExec::ToolExec(volatile bool *cancel)
	:
	_cancel(cancel)
{
}

void ToolExec::ErrorDialog(const char *pkg, int err)
{
	std::wstring ws_tool;
	const auto &args = GetArguments();
	if (!args.empty()) {
		ws_tool = StrMB2Wide(args.front());
	}
	if (s_warned_tools.insert(ws_tool).second) {
		const auto &ws_pkg = MB2Wide(pkg);
		const wchar_t *MsgItems[] = { g_settings.Msg(M_TITLE),
			L"Failed to run tool:", ws_tool.c_str(),
			L"Please install package:", ws_pkg.c_str(),
			L"Ok"
		};
		errno = err;
		g_far.Message(g_far.ModuleNumber, FMSG_WARNING | FMSG_ERRORTYPE, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
	}
}

void ToolExec::InfoDialog(const char *pkg)
{
	std::wstring tmp = g_settings.Msg(M_TITLE);
	tmp+= L" - operation details\n";
	tmp+= L"Package: ";
	tmp+= MB2Wide(pkg);
	tmp+= L'\n';
	tmp+= L"Command:";
	for (const auto &a : GetArguments()) {
		tmp+= L" \"";
		StrMB2Wide(a, tmp, true);
		tmp+= L'"';
	}
	g_far.Message(g_far.ModuleNumber, FMSG_MB_OK | FMSG_ALLINONE, nullptr, (const wchar_t * const *) tmp.c_str(), 0, 0);
}

void ToolExec::ProgressDialog(const std::string &file, const std::string &size_str, const char *pkg, const std::string &info)
{
	WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);

	std::wstring tmp = g_settings.Msg(M_TITLE);
	tmp+= L'\n';
	wchar_t buf[0x100]{}; swprintf(buf, ARRAYSIZE(buf) - 1, L"File of %s:", size_str.c_str());
	tmp+= buf;
	StrMB2Wide(file, tmp, true);
	tmp+= L'\n';
	StrMB2Wide(info, tmp, true);
	tmp+= L'\n';
	tmp+= L"\n&Skip";
	tmp+= L"\n&Info";
	++s_in_progress_dialog;
	while (!_exited && g_far.Message(g_far.ModuleNumber,
			FMSG_ALLINONE, nullptr, (const wchar_t * const *) tmp.c_str(), 0, 2) == 1) {
		InfoDialog(pkg);
	}
	--s_in_progress_dialog;
}

VOID ToolExec::sCallback(VOID *Context)
{
	// callbacks called withing UI thread, so there can be no race condition at dialog creation/s_in_progress_dialog counter update
	if (s_in_progress_dialog != 0) { // inject ESCAPE keypress that will close dialog
		DWORD dw;
		INPUT_RECORD ir{KEY_EVENT, {}};
		ir.Event.KeyEvent.bKeyDown = 1;
		ir.Event.KeyEvent.wRepeatCount = 1;
		ir.Event.KeyEvent.wVirtualKeyCode = VK_ESCAPE;
		ir.Event.KeyEvent.wVirtualScanCode = 0;
		ir.Event.KeyEvent.uChar.UnicodeChar = 0;
		ir.Event.KeyEvent.dwControlKeyState = 0;
		WINPORT(WriteConsoleInput)(NULL, &ir, 1, &dw);
		ir.Event.KeyEvent.bKeyDown = 0;
		WINPORT(WriteConsoleInput)(NULL, &ir, 1, &dw);
	}
}

bool FN_PRINTF_ARGS(5) ToolExec::Run(const std::string &file, const std::string &size_str, const char *pkg, const char *info_fmt, ...)
{
	if (Start() && !Wait(COMMAND_TIMEOUT_BEFORE_MESSAGE)) {
		if (_cancel) { // Quick View: dont show any UI, seamless cancellation by navigation to another file
			do {
				if (*_cancel) {
					KillAndWait();
					return false;
				}
			} while (!Wait(COMMAND_TIMEOUT_CHECK_CANCEL));
			return true;
		}

		// Full View: progress UI on long processing allowing to cancel processing skipping current file or show extra info
		va_list args;
		va_start(args, info_fmt);
		const std::string &info = StrPrintfV(info_fmt, args);
		va_end(args);
		ProgressDialog(file, size_str, pkg, info);
		if (_exited) {
			Wait();
		} else {
			KillAndWait();
		}
		// purge injected escape that could remain or anything else user could press
		PurgeAccumulatedInputEvents();
	}
	if (ExecError() != 0) {
		ErrorDialog(pkg, ExecError());
	}
	return true;
}

void *ToolExec::ThreadProc()
{
	void *out = ExecAsync::ThreadProc();
	_exited = true;
	DWORD dw;
	INPUT_RECORD ir{CALLBACK_EVENT, {}};
	ir.Event.CallbackEvent.Function = sCallback;
	WINPORT(WriteConsoleInput)(NULL, &ir, 1, &dw);
	return out;
}
