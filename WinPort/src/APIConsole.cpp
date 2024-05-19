#include <mutex>
#include <map>
#include <vector>
#include <stdexcept>
#include <debug.h>

#include "WinPort.h"
#include "Backend.h"

#define FORKED_CONSOLE_MAGIC 0xc001ba11f00dbabe

struct ForkedConsole
{
	uint64_t magic{FORKED_CONSOLE_MAGIC};
	IConsoleInput *con_in{nullptr};
	IConsoleOutput *con_out{nullptr};
};

static IConsoleOutput *ChooseConOut(HANDLE hConsole)
{
	if (!hConsole) {
		return g_winport_con_out;
	}
	ForkedConsole *fc = (ForkedConsole *)hConsole;
	ASSERT(fc->magic == FORKED_CONSOLE_MAGIC);
	return fc->con_out;
}

static IConsoleInput *ChooseConIn(HANDLE hConsole)
{
	if (!hConsole) {
		return g_winport_con_in;
	}
	ForkedConsole *fc = (ForkedConsole *)hConsole;
	ASSERT(fc->magic == FORKED_CONSOLE_MAGIC);
	return fc->con_in;
}

extern "C" {

	WINPORT_DECL(ForkConsole,HANDLE,())
	{
		ForkedConsole *fc = NULL;
		try {
			fc = new ForkedConsole;
			fc->con_in = g_winport_con_in->ForkConsoleInput(fc);
			fc->con_out = g_winport_con_out->ForkConsoleOutput(fc);

		} catch(...) {
			WINPORT(JoinConsole)(fc);
			fc = NULL;
		}
		return fc;
	}

	WINPORT_DECL(JoinConsole,VOID,(HANDLE hConsole))
	{
		if (hConsole) {
			ForkedConsole *fc = (ForkedConsole *)hConsole;
			ASSERT(fc->magic == FORKED_CONSOLE_MAGIC);
			fc->magic^= 0x0f0f0f0f0f0f0f0f;
			if (fc->con_in)
				g_winport_con_in->JoinConsoleInput(fc->con_in);
			if (fc->con_out)
				g_winport_con_out->JoinConsoleOutput(fc->con_out);
			delete fc;
		}
	}
	
	WINPORT_DECL(GetLargestConsoleWindowSize,COORD,(HANDLE hConsoleOutput))
	{
		return ChooseConOut(hConsoleOutput)->GetLargestConsoleWindowSize();
	}

	WINPORT_DECL(SetConsoleWindowInfo,BOOL,(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow))
	{
		ChooseConOut(hConsoleOutput)->SetWindowInfo(bAbsolute!=FALSE, *lpConsoleWindow);
		return TRUE;
	}

	WINPORT_DECL(SetConsoleTitle,BOOL,(HANDLE hConsoleOutput, const WCHAR *title))
	{
		ChooseConOut(hConsoleOutput)->SetTitle(title);
		return TRUE;
	}

	WINPORT_DECL(GetConsoleTitle,DWORD,(HANDLE hConsoleOutput, WCHAR *title, DWORD max_size))
	{
		const std::wstring &s = ChooseConOut(hConsoleOutput)->GetTitle();
		wcsncpy(title, s.c_str(), max_size);
		return (DWORD)(s.size() + 1);
	}

	WINPORT_DECL(SetConsoleScreenBufferSize,BOOL,(HANDLE hConsoleOutput,COORD dwSize))
	{
		ChooseConOut(hConsoleOutput)->SetSize(dwSize.X, dwSize.Y);
		return TRUE;
	}

	WINPORT_DECL(SetConsoleDisplayMode,BOOL,(DWORD ModeFlags))
	{
		return (ModeFlags==CONSOLE_WINDOWED_MODE) ? TRUE : FALSE;
	}

	WINPORT_DECL(GetConsoleDisplayMode,BOOL,(LPDWORD lpModeFlags))
	{
		*lpModeFlags = 0;//WTF??? GetConsoleDisplayMode/SetConsoleDisplayMode returns different meanings!!!
		return TRUE;
	}

	WINPORT_DECL(ScrollConsoleScreenBuffer,BOOL,(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, 
		const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill))
	{
		return ChooseConOut(hConsoleOutput)->Scroll(lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill) ? TRUE : FALSE;
	}

	WINPORT_DECL(SetConsoleWindowMaximized,VOID,(BOOL Maximized))
	{
		g_winport_con_out->SetWindowMaximized(Maximized!=FALSE);
	}


	WINPORT_DECL(GetConsoleScreenBufferInfo,BOOL,(HANDLE hConsoleOutput,CONSOLE_SCREEN_BUFFER_INFO *lpConsoleScreenBufferInfo))
	{
		unsigned int width = 0, height = 0;
		auto *con_out = ChooseConOut(hConsoleOutput);
		con_out->GetSize(width, height);
		lpConsoleScreenBufferInfo->dwCursorPosition = con_out->GetCursor();
		lpConsoleScreenBufferInfo->wAttributes = con_out->GetAttributes();
		lpConsoleScreenBufferInfo->dwSize.X = width;
		lpConsoleScreenBufferInfo->dwSize.Y = height;
		lpConsoleScreenBufferInfo->srWindow.Left = 0;
		lpConsoleScreenBufferInfo->srWindow.Top = 0;
		lpConsoleScreenBufferInfo->srWindow.Right = width - 1;
		lpConsoleScreenBufferInfo->srWindow.Bottom = height - 1;
		lpConsoleScreenBufferInfo->dwMaximumWindowSize.X = width;
		lpConsoleScreenBufferInfo->dwMaximumWindowSize.Y = height;
		
		return TRUE;
	}

	WINPORT_DECL(SetConsoleCursorPosition,BOOL,(HANDLE hConsoleOutput,COORD dwCursorPosition))
	{
		ChooseConOut(hConsoleOutput)->SetCursor(dwCursorPosition);
		return TRUE;
	}

	WINPORT_DECL(SetConsoleCursorInfo,BOOL,(HANDLE hConsoleOutput,const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo))
	{
		DWORD height = lpConsoleCursorInfo->dwSize;
		if (height > 100) height = 100;
		else if (height == 0) height = 1;
		ChooseConOut(hConsoleOutput)->SetCursor((UCHAR)height, lpConsoleCursorInfo->bVisible!=FALSE);
		return TRUE;
	}

	WINPORT_DECL(SetConsoleCursorBlinkTime,VOID,(HANDLE hConsoleOutput, DWORD dwMilliseconds ))
	{
		ChooseConOut(hConsoleOutput)->SetCursorBlinkTime(dwMilliseconds);
	}

	WINPORT_DECL(GetConsoleCursorInfo,BOOL,(HANDLE hConsoleOutput,CONSOLE_CURSOR_INFO *lpConsoleCursorInfo))
	{
		UCHAR height;
		bool visible;
		ChooseConOut(hConsoleOutput)->GetCursor(height, visible);
		lpConsoleCursorInfo->dwSize = height;
		lpConsoleCursorInfo->bVisible = visible ? TRUE : FALSE;
		return TRUE;
	}

	WINPORT_DECL(GetConsoleMode,BOOL,(HANDLE hConsoleHandle,LPDWORD lpMode))
	{
		*lpMode = ChooseConOut(hConsoleHandle)->GetMode();
		return TRUE;
	}
	
	WINPORT_DECL(SetConsoleMode,BOOL,(HANDLE hConsoleHandle, DWORD dwMode))
	{
		ChooseConOut(hConsoleHandle)->SetMode(dwMode);
		return TRUE;
	}


	WINPORT_DECL(SetConsoleTextAttribute,BOOL,(HANDLE hConsoleOutput, DWORD64 qAttributes))
	{
		ChooseConOut(hConsoleOutput)->SetAttributes(qAttributes);
		return TRUE;
	}

	WINPORT_DECL(WriteConsole,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved))
	{
		*lpNumberOfCharsWritten = ChooseConOut(hConsoleOutput)->WriteString(lpBuffer, nNumberOfCharsToWrite);
		return TRUE;
	}

	WINPORT_DECL(WriteConsoleOutput,BOOL,(HANDLE hConsoleOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpScreenRegion))
	{
		ChooseConOut(hConsoleOutput)->Write(lpBuffer, dwBufferSize, dwBufferCoord, *lpScreenRegion);
		return TRUE;
	}

	WINPORT_DECL(ReadConsoleOutput, BOOL, (HANDLE hConsoleOutput, CHAR_INFO *lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, PSMALL_RECT lpScreenRegion))
	{
		ChooseConOut(hConsoleOutput)->Read(lpBuffer, dwBufferSize, dwBufferCoord, *lpScreenRegion);
		return TRUE;
	}

	WINPORT_DECL(WriteConsoleOutputCharacter,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten))
	{
		*lpNumberOfCharsWritten = ChooseConOut(hConsoleOutput)->WriteStringAt(lpCharacter, nLength, dwWriteCoord);
		return TRUE;
	}

	WINPORT_DECL(FillConsoleOutputAttribute, BOOL, (HANDLE hConsoleOutput, DWORD64 qAttributes, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfAttrsWritten))
	{
		*lpNumberOfAttrsWritten = ChooseConOut(hConsoleOutput)->FillAttributeAt(qAttributes, nLength, dwWriteCoord);
		return TRUE;
	}

	WINPORT_DECL(FillConsoleOutputCharacter, BOOL, (HANDLE hConsoleOutput, WCHAR cCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten))
	{
		*lpNumberOfCharsWritten = ChooseConOut(hConsoleOutput)->FillCharacterAt(cCharacter, nLength, dwWriteCoord);
		return TRUE;
	}

	WINPORT_DECL(SetConsoleActiveScreenBuffer, BOOL,(HANDLE hConsoleOutput))
	{
		return TRUE;
	}

	WINPORT_DECL(FlushConsoleInputBuffer,BOOL,(HANDLE hConsoleInput))
	{
		ChooseConIn(hConsoleInput)->Flush();
		return TRUE;
	}

	WINPORT_DECL(GetNumberOfConsoleInputEvents,BOOL,(HANDLE hConsoleInput, LPDWORD lpcNumberOfEvents))
	{
		*lpcNumberOfEvents = ChooseConIn(hConsoleInput)->Count();
		return TRUE;
	}

	WINPORT_DECL(PeekConsoleInput,BOOL,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead))
	{
		*lpNumberOfEventsRead = ChooseConIn(hConsoleInput)->Peek(lpBuffer, nLength);
		return TRUE;
	}

	WINPORT_DECL(ReadConsoleInput,BOOL,(HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead))
	{
		*lpNumberOfEventsRead = 0;
		while (nLength) {
			DWORD cnt = ChooseConIn(hConsoleInput)->Dequeue(lpBuffer, nLength);
			if (cnt) {
				*lpNumberOfEventsRead+= cnt;
				nLength-= cnt;
				lpBuffer+= cnt;
				break;//or not break?
			} else
				ChooseConIn(hConsoleInput)->WaitForNonEmpty();
		}
		return TRUE;
	}

	WINPORT_DECL(CheckForKeyPress,DWORD,(HANDLE hConsoleInput, const WORD *KeyCodes, DWORD KeyCodesCount, DWORD Flags))
	{
		std::vector<INPUT_RECORD> backlog;
		DWORD out = 0;
		while (ChooseConIn(hConsoleInput)->WaitForNonEmptyWithTimeout(0)) {
			INPUT_RECORD rec;
			if (!ChooseConIn(hConsoleInput)->Dequeue(&rec, 1)) {
				break;
			}
			if (rec.EventType == KEY_EVENT) {
				DWORD i;
				for (i = 0; i != KeyCodesCount; ++i) {
					if (KeyCodes[i] == rec.Event.KeyEvent.wVirtualKeyCode) {
						if (rec.Event.KeyEvent.bKeyDown && out == 0) {
							out = i + 1;
						}
						break;
					}
				}
				if (i == KeyCodesCount && (Flags & CFKP_KEEP_UNMATCHED_KEY_EVENTS) != 0) {
					backlog.emplace_back(rec);
				}
				if (i != KeyCodesCount && (Flags & CFKP_KEEP_MATCHED_KEY_EVENTS) != 0) {
					backlog.emplace_back(rec);
				}
			} else if (rec.EventType == MOUSE_EVENT) {
				if ((Flags & CFKP_KEEP_MOUSE_EVENTS) != 0) {
					backlog.emplace_back(rec);
				}
			} else if ((Flags & CFKP_KEEP_OTHER_EVENTS) != 0) {
				backlog.emplace_back(rec);
			}
		}
		if (!backlog.empty()) {
			ChooseConIn(hConsoleInput)->Enqueue(backlog.data(), backlog.size());
		}
		return out;
	}

	WINPORT_DECL(WaitConsoleInput,BOOL,(HANDLE hConsoleInput, DWORD dwTimeout))
	{
		if (dwTimeout == INFINITE) {
			ChooseConIn(hConsoleInput)->WaitForNonEmpty();
			return TRUE;
		}
		return ChooseConIn(hConsoleInput)->WaitForNonEmptyWithTimeout(dwTimeout) ? TRUE : FALSE;
	}

	WINPORT_DECL(WriteConsoleInput,BOOL,(HANDLE hConsoleInput, const INPUT_RECORD *lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten))
	{
		ChooseConIn(hConsoleInput)->Enqueue(lpBuffer, nLength);
		*lpNumberOfEventsWritten = nLength;
		return TRUE;
	}

	static PHANDLER_ROUTINE gHandlerRoutine = NULL;

	WINPORT_DECL(GenerateConsoleCtrlEvent, BOOL, (DWORD dwCtrlEvent, DWORD dwProcessGroupId ))
	{
		if (!gHandlerRoutine || !gHandlerRoutine(dwCtrlEvent)) {
			if (dwCtrlEvent == CTRL_CLOSE_EVENT) {
				fprintf(stderr, "GenerateConsoleCtrlEvent(%u) - exiting\n", dwCtrlEvent);
				exit(0);
			}
		}
		fprintf(stderr, "GenerateConsoleCtrlEvent(%u)\n", dwCtrlEvent);
		return TRUE;
	}

	WINPORT_DECL(SetConsoleCtrlHandler, BOOL, (PHANDLER_ROUTINE HandlerRoutine, BOOL Add ))
	{
		if (Add) {
			gHandlerRoutine = HandlerRoutine;
			return TRUE;
		} else if (HandlerRoutine==gHandlerRoutine) {
			gHandlerRoutine = FALSE;
			return TRUE;
		}

		return FALSE;
	}
	
	WINPORT_DECL(SetConsoleScrollRegion, VOID, (HANDLE hConsoleOutput, SHORT top, SHORT bottom))
	{
		ChooseConOut(hConsoleOutput)->SetScrollRegion(top, bottom);
	}
	
	WINPORT_DECL(GetConsoleScrollRegion, VOID, (HANDLE hConsoleOutput, SHORT *top, SHORT *bottom))
	{
		ChooseConOut(hConsoleOutput)->GetScrollRegion(*top, *bottom);
	}
	
	WINPORT_DECL(SetConsoleScrollCallback, VOID, (HANDLE hConsoleOutput, PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext))
	{
		ChooseConOut(hConsoleOutput)->SetScrollCallback(pCallback, pContext);
	}
	
	WINPORT_DECL(BeginConsoleAdhocQuickEdit, BOOL, ())
	{
		if (g_winport_con_out->GetMode() & ENABLE_QUICK_EDIT_MODE) {
			fprintf(stderr, "BeginConsoleAdhocQuickEdit: meaningless when enabled ENABLE_QUICK_EDIT_MODE\n");
			return FALSE;
		}
		
		//here is possible non-critical race with enabling ENABLE_QUICK_EDIT_MODE
		g_winport_con_out->AdhocQuickEdit();
		return TRUE;
	}

	WINPORT_DECL(SetConsoleTweaks, DWORD64, (DWORD64 tweaks))
	{
		return g_winport_con_out->SetConsoleTweaks(tweaks);
	}

	WINPORT_DECL(SaveConsoleWindowState,VOID,())
	{
		return g_winport_con_out->ConsoleSaveWindowState();
	}

	WINPORT_DECL(ConsoleChangeFont, VOID, ())
	{
		return g_winport_con_out->ConsoleChangeFont();
	}

	WINPORT_DECL(IsConsoleActive, BOOL, ())
	{
		return g_winport_con_out->IsActive() ? TRUE : FALSE;
	}

	WINPORT_DECL(ConsoleDisplayNotification, VOID, (const WCHAR *title, const WCHAR *text))
	{
		g_winport_con_out->ConsoleDisplayNotification(title, text);
	}

	WINPORT_DECL(ConsoleBackgroundMode, BOOL, (BOOL TryEnterBackgroundMode))
	{
		return g_winport_con_out->ConsoleBackgroundMode(TryEnterBackgroundMode != FALSE) ? TRUE : FALSE;
	}

	WINPORT_DECL(SetConsoleFKeyTitles, BOOL, (HANDLE hConsoleOutput, const CHAR **titles))
	{
		return ChooseConOut(hConsoleOutput)->SetFKeyTitles(titles) ? TRUE : FALSE;
	}

	WINPORT_DECL(GetConsoleColorPalette,BYTE,(HANDLE hConsoleOutput))
	{
		return ChooseConOut(hConsoleOutput)->GetColorPalette();
	}

	WINPORT_DECL(GetConsoleBasePalette,VOID,(HANDLE hConsoleOutput, VOID *p))
	{
		return ChooseConOut(hConsoleOutput)->GetBasePalette(p);
	}

	WINPORT_DECL(SetConsoleBasePalette,BOOL,(HANDLE hConsoleOutput, VOID *p))
	{
		return ChooseConOut(hConsoleOutput)->SetBasePalette(p);
	}

	WINPORT_DECL(OverrideConsoleColor, VOID, (HANDLE hConsoleOutput, DWORD Index, DWORD *ColorFG, DWORD *ColorBK))
	{
		return ChooseConOut(hConsoleOutput)->OverrideColor(Index, ColorFG, ColorBK);
	}

	WINPORT_DECL(SetConsoleRepaintsDefer, VOID, (HANDLE hConsoleOutput, BOOL Deferring))
	{
		if (Deferring) {
			ChooseConOut(hConsoleOutput)->RepaintsDeferStart();
		} else {
			ChooseConOut(hConsoleOutput)->RepaintsDeferFinish();
		}
	}

	static struct {
		struct Cmp
		{
			bool operator()(const WCHAR *a, const WCHAR *b) const { return wcscmp(a, b) < 0; }
		};
		std::mutex mtx;
		std::vector<WCHAR *> id2str;
		std::map<const WCHAR *, COMP_CHAR, Cmp> str2id;
	} s_composite_chars;

	WINPORT_DECL(CompositeCharRegister,COMP_CHAR,(const WCHAR *lpSequence))
	{
		if (!lpSequence[0]) {
			return 0;
		}
		if (!lpSequence[1]) {
			return lpSequence[0];
		}

		std::lock_guard<std::mutex> lock(s_composite_chars.mtx);
		auto it = s_composite_chars.str2id.find(lpSequence);
		if (it != s_composite_chars.str2id.end()) {
			return it->second | COMPOSITE_CHAR_MARK;
		}
		wchar_t *wd = wcsdup(lpSequence);
		try {
			if (!wd)
				throw std::logic_error("wcsdup failed");

			const COMP_CHAR id = COMP_CHAR(s_composite_chars.id2str.size());
			s_composite_chars.id2str.emplace_back(wd);
			s_composite_chars.str2id.emplace(wd, id);
			return id | COMPOSITE_CHAR_MARK;

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s for '%ls'\n", __FUNCTION__, e.what(), lpSequence);
			free(wd);
		}
		return 0;
	}

	WINPORT_DECL(CompositeCharLookup,const WCHAR *,(COMP_CHAR CompositeChar))
	{
		if ((CompositeChar & COMPOSITE_CHAR_MARK) == 0) {
			fprintf(stderr, "%s: invoked for not composite-char 0x%llx\n",
				__FUNCTION__, (unsigned long long)CompositeChar);
			return L"\u2022";
		}

		const COMP_CHAR id = CompositeChar & (~COMPOSITE_CHAR_MARK);

		std::lock_guard<std::mutex> lock(s_composite_chars.mtx);
		if (id >= (COMP_CHAR)s_composite_chars.id2str.size()) {
			fprintf(stderr, "%s: out of range composite-char 0x%llx\n",
				__FUNCTION__, (unsigned long long)CompositeChar);
			return L"\u2022";
		}
		return s_composite_chars.id2str[id];
	}
}
