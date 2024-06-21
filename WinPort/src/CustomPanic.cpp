#include <utils.h>
#include "WinPort.h"
#include "Backend/Backend.h"

#define KEYPRESS_WAIT_SECONDS   60
#define KEYPRESS_IGNORE_SECONDS 3

extern "C" {

	static void FN_NOINLINE PrintLine(DWORD color, SHORT row, const char *str)
	{
		std::wstring wstr = MB2Wide(str);
		unsigned int w = 0, h = 0;
		g_winport_con_out->GetSize(w, h);
		if (wstr.size() < w) {
			wstr.append(w - wstr.size(), L' ');
		}
		COORD pos = {0, row};
		g_winport_con_out->SetAttributes(color | FOREGROUND_INTENSITY);
		g_winport_con_out->WriteStringAt(wstr.c_str(), wstr.size(), pos);
	}

	// this function intended to be called from Panic() function implemented in utils project
	SHAREDSYMBOL void FN_NORETURN CustomPanic(const char *format, va_list args) noexcept
	{
		try {
			if (g_winport_con_in && g_winport_con_out) {
				const std::string &str = StrPrintfV(format, args);
				ConsoleInputPriority cip(g_winport_con_in);
				char anykey_sz[64]{};
				for (int i = 0; i < KEYPRESS_WAIT_SECONDS;) {
					if (i >= KEYPRESS_IGNORE_SECONDS) {
						snprintf(anykey_sz, sizeof(anykey_sz), "PRESS ANY KEY TO EXIT (%d)", KEYPRESS_WAIT_SECONDS - i);
					}
					PrintLine(FOREGROUND_RED, 0, "-------------------------");
					PrintLine(FOREGROUND_RED, 1, "--- PANIC PANIC PANIC ---");
					PrintLine(FOREGROUND_RED, 2, "-------------------------");

					PrintLine(FOREGROUND_RED  | FOREGROUND_GREEN, 3, str.c_str());
					PrintLine(FOREGROUND_BLUE | FOREGROUND_GREEN, 4, anykey_sz);
					PrintLine(FOREGROUND_BLUE | FOREGROUND_GREEN, 5, "");

					INPUT_RECORD ir;
					if (g_winport_con_in->WaitForNonEmptyWithTimeout(1000, cip)
					 && g_winport_con_in->Dequeue(&ir, 1, cip)) {
						if (i >= KEYPRESS_IGNORE_SECONDS && ir.EventType == KEY_EVENT) {
							break;
						}
					} else {
						++i;
					}
				}
			}
		} catch (...) {
			fprintf(stderr, "CustomPanic: exception\n");
		}

		abort();
	}
}
