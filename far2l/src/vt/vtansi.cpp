/*
ANSI virtual terminal based on:

-------------------------------------------------------------------------------

Copyright (C) 2005-2014 Jason Hood

This software is provided 'as-is', without any express or implied
warranty.  In no event will the author be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Jason Hood
jadoxa@yahoo.com.au


  ANSI.c - ANSI escape sequence console driver.

  Jason Hood, 21 & 22 October, 2005.

  Derived from ANSI.xs by Jean-Louis Morel, from his Perl package
  Win32::Console::ANSI.  I removed the codepage conversion ("\e(") and added
  WriteConsole hooking.

  v1.01, 11 & 12 March, 2006:
    disable when console has disabled processed output;
    \e[5m (blink) is the same as \e[4m (underline);
    do not conceal control characters (0 to 31);
    \e[m will restore original color.

  v1.10, 22 February, 2009:
    fix MyWriteConsoleW for strings longer than the buffer;
    initialise attributes to current;
    hook into child processes.

  v1.11, 28 February, 2009:
    fix hooking into child processes (only do console executables).

  v1.12, 9 March, 2009:
    really fix hooking (I didn't realise MinGW didn't generate relocations).

  v1.13, 21 & 27 March, 2009:
    alternate injection method, to work with DEP;
    use Unicode and the current output code page (not OEMCP).

  v1.14, 3 April, 2009:
    fix test for empty import section.

  v1.15, 17 May, 2009:
    properly update lpNumberOfCharsWritten in MyWriteConsoleA.

  v1.20, 26 & 29 May, 17 to 21 June, 2009:
    create an ANSICON environment variable;
    hook GetEnvironmentVariable to create ANSICON dynamically;
    use another injection method.

  v1.22, 5 October, 2009:
    hook LoadLibrary to intercept the newly loaded functions.

  v1.23, 11 November, 2009:
    unload gracefully;
    conceal characters by making foreground same as background;
    reverse the bold/underline attributes, too.

  v1.25, 15, 20 & 21 July, 2010:
    hook LoadLibraryEx (now cscript works);
    Win7 support.

  v1.30, 3 August to 7 September, 2010:
    x64 support.

  v1.31, 13 & 19 November, 2010:
    fix multibyte conversion problems.

  v1.32, 4 to 22 December, 2010:
    test for lpNumberOfCharsWritten/lpNumberOfBytesWritten being NULL;
    recognise DSR and xterm window title;
    ignore sequences starting with \e[? & \e[>;
    close the handles opened by CreateProcess.

  v1.40, 25 & 26 February, 1 March, 2011:
    hook GetProcAddress, addresses issues with .NET (work with PowerShell);
    implement SO & SI to use the DEC Special Graphics Character Set (enables
     line drawing via ASCII); ignore \e(X & \e)X (where X is any character);
    add \e[?25h & \e[?25l to show/hide the cursor (DECTCEM).

  v1.50, 7 to 14 December, 2011:
    added dynamic environment variable ANSICON_VER to return version;
    read ANSICON_EXC environment variable to exclude selected modules;
    read ANSICON_GUI environment variable to hook selected GUI programs;
    read ANSICON_DEF environment variable to set the default GR;
    transfer current GR to child, read it on exit.

  v1.51, 15 January, 5, 22 & 24 February, 2012:
    added log mask 16 to log all the imported modules of imported modules;
    ignore the version within the core API DLL names;
    fix 32-bit process trying to identify 64-bit process;
    hook _lwrite & _hwrite.

  v1.52, 10 April, 1 & 2 June, 2012:
    use ansicon.exe to enable 32-bit to inject into 64-bit;
    implement \e[39m & \e[49m (only setting color, nothing else);
    added the character/line equivalents (keaj`) of the cursor movement
     sequences (ABCDG), as well as vertical absolute (d) and erase characters
     (X).

  v1.53, 12 June, 2012:
    fixed Update_GRM when running multiple processes (e.g. "cl /MP").

  v1.60, 22 to 24 November, 2012:
    alternative method to obtain LLW for 64->32 injection;
    support for VC6 (remove section pragma, rename isdigit to is_digit).

  v1.61, 14 February, 2013:
    go back to using ANSI-LLW.exe for 64->32 injection.

  v1.62, 17 & 18 July, 2013:
    another method to obtain LLW for 64->32 injection.

  v1.64, 2 August, 2013:
    better method of determining a console handle (see IsConsoleHandle).

  v1.65, 28 August, 2013:
    fix \e[K (was using window, not buffer).

  v1.66, 20 & 21 September, 2013:
    fix 32-bit process trying to detect 64-bit process.

  v1.70, 25 January to 26 February, 2014:
    don't hook ourself from LoadLibrary or LoadLibraryEx;
    update the LoadLibraryEx flags that should not cause hooking;
    inject by manipulating the import directory table; for 64-bit AnyCPU use
     ntdll's LdrLoadDll via CreateRemoteThread;
    restore original attributes on detach (for LoadLibrary/FreeLibrary usage);
    log: remove the quotes around the CreateProcess command line string and
      distinguish NULL and "" args;
    attributes (and saved position) are local to each console window;
    exclude entire programs, by not using an extension in ANSICON_EXC;
    hook modules injected via CreateRemoteThread+LoadLibrary;
    hook all modules loaded due to LoadLibrary, not just the specified;
    don't hook a module that's already hooked us;
    better parsing of escape & CSI sequences;
    ignore xterm 38 & 48 SGR values;
    change G1 blank from space to U+00A0 - No-Break Space;
    use window height, not buffer;
    added more sequences;
    don't add a newline immediately after a wrap;
    restore cursor visibility on unload.

  v1.71, 23 October, 2015
    Add _CRT_NON_CONFORMING_WCSTOK define
*/

#define _CRT_NON_CONFORMING_WCSTOK

#include "headers.hpp"

#include <mutex>
#include <atomic>
#include <map>
#include "vtansi.h"
#include "AnsiEsc.hpp"
#include "UtfConvert.hpp"

#define is_digit(c) ('0' <= (c) && (c) <= '9')

#define BGR2RGB(COLOR) ((((COLOR) & 0xff0000) >> 16) | ((COLOR) & 0x00ff00) | (((COLOR) & 0x0000ff) << 16))

// ========== Global variables and constants


#include "vtlog.h"

enum AnsiMouseExpectation
{
	AMEX_X10_MOUSE             = 9,
	AMEX_VT200_MOUSE           = 1000,
	AMEX_VT200_HIGHLIGHT_MOUSE = 1001,
	AMEX_BTN_EVENT_MOUSE       = 1002,
	AMEX_ANY_EVENT_MOUSE       = 1003,
	AMEX_SGR_EXT_MOUSE         = 1006
};

struct VTAnsiState
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	CONSOLE_CURSOR_INFO cci;
	DWORD mode;
	WORD attr;
	SHORT scroll_top;
	SHORT scroll_bottom;

	VTAnsiState() : mode(0), attr(0), scroll_top(0), scroll_bottom(0)
	{
		memset(&csbi, 0, sizeof(csbi));
		memset(&cci, 0, sizeof(cci));
	}
	
	void InitFromConsole(HANDLE con)
	{
		WINPORT(GetConsoleScreenBufferInfo)( con, &csbi );
		WINPORT(GetConsoleCursorInfo)( con, &cci );
		WINPORT(GetConsoleMode)( con, &mode );
		WINPORT(GetConsoleScrollRegion)(con, &scroll_top, &scroll_bottom);
	}

	void ApplyToConsole(HANDLE con, bool including_cursor_pos = true)
	{
		WINPORT(SetConsoleMode)( con, mode );
		WINPORT(SetConsoleCursorInfo)( con, &cci );
		WINPORT(SetConsoleTextAttribute)( con, csbi.wAttributes );
		if (including_cursor_pos) {
			WINPORT(SetConsoleCursorPosition)( con, csbi.dwCursorPosition );
		}
		if (scroll_bottom >= csbi.srWindow.Bottom) {
			//window could be expanded bigger than before making previous
			//scrolling region to not cover whole area, so correct this case
			CONSOLE_SCREEN_BUFFER_INFO current_csbi;
			WINPORT(GetConsoleScreenBufferInfo)( con, &current_csbi );
			if (scroll_bottom < current_csbi.srWindow.Bottom) {
				scroll_bottom = current_csbi.srWindow.Bottom;
			}
		}
		WINPORT(SetConsoleScrollRegion)(con, scroll_top, scroll_bottom);
	}

};

#define ESC	'\x1B'          // ESCape character
#define BEL	'\x07'
#define SO	'\x0E'          // Shift Out
#define SI	'\x0F'          // Shift In
#define ST	'\x9c'

#define MAX_ARG 32					// max number of args in an escape sequence


#define FIRST_SG '_'
#define LAST_SG  '~'


#define BUFFER_SIZE 2048

// DEC Special Graphics Character Set from
// http://vt100.net/docs/vt220-rm/table2-4.html
// Some of these may not look right, depending on the font and code page (in
// particular, the Control Pictures probably won't work at all).
const WCHAR DECSpecialGraphicsCharset[] = {
	L'\x00a0',    // _ - No-Break Space
	L'\x2666',    // ` - Black Diamond Suit
	L'\x2592',    // a - Medium Shade
	L'\x2409',    // b - HT
	L'\x240c',    // c - FF
	L'\x240d',    // d - CR
	L'\x240a',    // e - LF
	L'\x00b0',    // f - Degree Sign
	L'\x00b1',    // g - Plus-Minus Sign
	L'\x2424',    // h - NL
	L'\x240b',    // i - VT
	L'\x2518',    // j - Box Drawings Light Up And Left
	L'\x2510',    // k - Box Drawings Light Down And Left
	L'\x250c',    // l - Box Drawings Light Down And Right
	L'\x2514',    // m - Box Drawings Light Up And Right
	L'\x253c',    // n - Box Drawings Light Vertical And Horizontal
	L'\x00af',    // o - SCAN 1 - Macron
	L'\x25ac',    // p - SCAN 3 - Black Rectangle
	L'\x2500',    // q - SCAN 5 - Box Drawings Light Horizontal
	L'_',         // r - SCAN 7 - Low Line
	L'_',         // s - SCAN 9 - Low Line
	L'\x251c',    // t - Box Drawings Light Vertical And Right
	L'\x2524',    // u - Box Drawings Light Vertical And Left
	L'\x2534',    // v - Box Drawings Light Up And Horizontal
	L'\x252c',    // w - Box Drawings Light Down And Horizontal
	L'\x2502',    // x - Box Drawings Light Vertical
	L'\x2264',    // y - Less-Than Or Equal To
	L'\x2265',    // z - Greater-Than Or Equal To
	L'\x03c0',    // { - Greek Small Letter Pi
	L'\x2260',    // | - Not Equal To
	L'\x00a3',    // } - Pound Sign
	L'\x00b7',    // ~ - Middle Dot
};

struct VTAnsiContext
{
	CONSOLE_SCREEN_BUFFER_INFO save_cursor_info = {};
	VTAnsiState saved_state;
	std::mutex title_mutex;
	IVTShell *vt_shell = nullptr;
	std::string cur_title;
	std::atomic<bool> output_disabled{false};
	std::map<DWORD, std::pair<DWORD, DWORD> > orig_palette;

	int   state;					// automata state
	char  prefix;				// escape sequence prefix ( '[', ']' or '(' );
	char  prefix2;				// secondary prefix ( '?' or '>' );
	char  suffix;				// escape sequence suffix
	char  suffix2;				// escape sequence secondary suffix
	int   es_argc;				// escape sequence args count
	int   es_argv[MAX_ARG]; 		// escape sequence args
	std::string os_cmd_arg;		// text parameter for Operating System Command
	int   screen_top = -1;		// initial window top when cleared
	TCHAR blank_character = L' ';

	int   chars_in_buffer;
	WCHAR char_buffer[BUFFER_SIZE];
	WCHAR prev_char;
	bool  wrapped;

	bool  charset_shifted = false;
	WCHAR charset_selection[2] = {L'B', L'B'};
	WCHAR &CurrentCharsetSelection()
	{
		return charset_selection[charset_shifted ? 1 : 0];
	}

	// color constants

	struct STATE {
		COORD              saved_pos{};	// saved cursor position
		AnsiEsc::FontState font_state{};
		bool               crm = false;		// showing control characters?
	} ansi_state;

// ========== Print Buffer functions

	void ApplyConsoleTitle(HANDLE con_hnd)
	{
		std::wstring title(1, L'[');
		{
			std::lock_guard<std::mutex> lock(title_mutex);
			title+= StrMB2Wide(cur_title);
		}
		title+= L']';
		WINPORT(SetConsoleTitle)(con_hnd, title.c_str());
	}

//-----------------------------------------------------------------------------
//   FlushBuffer()
// Writes the buffer to the console and empties it.
//-----------------------------------------------------------------------------

	void WriteConsoleIfEnabled(const WCHAR *str, DWORD len)
	{
		DWORD written;
		if (!output_disabled) {
			WINPORT(WriteConsole)(vt_shell->ConsoleHandle(), str, len, &written, NULL );
		}
	}

	void FlushBuffer( void )
	{
		DWORD nWritten;

		if (output_disabled) {
			chars_in_buffer = 0;
		}

//fprintf(stderr, "FlushBuffer: %u\n", chars_in_buffer);
		if (chars_in_buffer <= 0)
			return;
		HANDLE con_hnd = vt_shell->ConsoleHandle();
		if (ansi_state.crm) {
			DWORD mode = 0;
			WINPORT(GetConsoleMode)( con_hnd, &mode );
			WINPORT(SetConsoleMode)( con_hnd, mode & ~(ENABLE_PROCESSED_OUTPUT) );
			WINPORT(WriteConsole)( con_hnd, char_buffer, chars_in_buffer, &nWritten, NULL );
			WINPORT(SetConsoleMode)( con_hnd, mode );
		} else {
			//HANDLE hConWrap;
			//CONSOLE_CURSOR_INFO cci;
			CONSOLE_SCREEN_BUFFER_INFO csbi, csbi_before;

				LPWSTR b = char_buffer;
				do {
					WINPORT(GetConsoleScreenBufferInfo)( con_hnd, &csbi_before );
					WINPORT(WriteConsole)( con_hnd, b, 1, &nWritten, NULL );
					if (*b != '\r' && *b != '\b' && *b != '\a') {
						WINPORT(GetConsoleScreenBufferInfo)( con_hnd, &csbi );
						if (csbi.dwCursorPosition.X == 0 || csbi.dwCursorPosition.X==csbi_before.dwCursorPosition.X)
							wrapped = true;
					}
				} while (++b, --chars_in_buffer);

			/*if (chars_in_buffer < 4) {
				LPWSTR b = char_buffer;
				do {
					WINPORT(WriteConsole)( _con_hnd, b, 1, &nWritten, NULL );
					if (*b != '\r' && *b != '\b' && *b != '\a') {
						WINPORT(GetConsoleScreenBufferInfo)( _con_hnd, &csbi );
						if (csbi.dwCursorPosition.X == 0)
							wrapped = true;
					}
				} while (++b, --chars_in_buffer);
			} else {
				fprintf(stderr, "TODODODODO\n");
				

				// To detect wrapping of multiple characters, create a new buffer, write
				// to the top of it and see if the cursor changes line. This doesn't
				// always work on the normal buffer, since if you're already on the last
				// line, wrapping scrolls everything up and still leaves you on the last.
				hConWrap = CreateConsoleScreenBuffer( GENERIC_READ|GENERIC_WRITE, 0, NULL,
							CONSOLE_TEXTMODE_BUFFER, NULL );
				// Even though the buffer isn't visible, the cursor still shows up.
				cci.dwSize = 1;
				cci.bVisible = FALSE;
				SetConsoleCursorInfo( hConWrap, &cci );
				// Ensure the buffer is the same width (it gets created using the window
				// width) and more than one line.
				GetConsoleScreenBufferInfo( _con_hnd, &csbi );
				csbi.dwSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 2;
				SetConsoleScreenBufferSize( hConWrap, csbi.dwSize );
				// Put the cursor on the top line, in the same column.
				csbi.dwCursorPosition.Y = 0;
				SetConsoleCursorPosition( hConWrap, csbi.dwCursorPosition );
				WriteConsole( hConWrap, char_buffer, chars_in_buffer, &nWritten, NULL );
				GetConsoleScreenBufferInfo( hConWrap, &csbi );
				if (csbi.dwCursorPosition.Y != 0)
					wrapped = true;
				CloseHandle( hConWrap );
				WINPORT(WriteConsole)( _con_hnd, char_buffer, chars_in_buffer, &nWritten, NULL );
			}*/
		}
		chars_in_buffer = 0;
	}

//-----------------------------------------------------------------------------
//   PushBuffer( WCHAR c )
// Adds a character in the buffer.
//-----------------------------------------------------------------------------

	void PushBuffer( WCHAR c )
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		prev_char = c;

		if (c == '\n') {
			HANDLE con_hnd = vt_shell->ConsoleHandle();
			if (ansi_state.crm)
				char_buffer[chars_in_buffer++] = c;
			FlushBuffer();
			// Avoid writing the newline if wrap has already occurred.
			WINPORT(GetConsoleScreenBufferInfo)( con_hnd, &csbi );
			if (ansi_state.crm) {
				// If we're displaying controls, then the only way we can be on the left
				// margin is if wrap occurred.
				if (csbi.dwCursorPosition.X != 0)
					WriteConsoleIfEnabled( L"\n", 1);
			} else {
				LPCWSTR nl = L"\n";
				if (wrapped) {
					// It's wrapped, but was anything more written? Look at the current
					// row, checking that each character is space in current attributes.
					// If it's all blank we can drop the newline. If the cursor isn't
					// already at the margin, then it was spaces or tabs that caused the
					// wrap, which can be ignored and overwritten.
					CHAR_INFO blank;
					PCHAR_INFO row;
					row = (PCHAR_INFO)malloc( csbi.dwSize.X * sizeof(CHAR_INFO) );
					if (row != NULL) {
						COORD s, c;
						SMALL_RECT r;
						s.X = csbi.dwSize.X;
						s.Y = 1;
						c.X = c.Y = 0;
						r.Left = 0;
						r.Right = s.X - 1;
						r.Top = r.Bottom = csbi.dwCursorPosition.Y;
						WINPORT(ReadConsoleOutput)( con_hnd, row, s, c, &r );
						CI_SET_WCATTR(blank, blank_character, csbi.wAttributes);
						while (*(PDWORD)&row[c.X] == *(PDWORD)&blank)
							if (++c.X == s.X) {
								nl = (csbi.dwCursorPosition.X == 0) ? NULL : L"\r";
								break;
							}
						free( row );
					}
					wrapped = false;
				}
				if (nl)
					WriteConsoleIfEnabled( nl, 1);
			}
		} else {
			switch (CurrentCharsetSelection()) {
				case '2':// TODO or not TODO???

				case '0':
					if (c >= FIRST_SG && c <= LAST_SG)
						c = DECSpecialGraphicsCharset[c - FIRST_SG];
				break;

				case '1': // TODO or not TODO???
				default: ;
			}
			char_buffer[chars_in_buffer] = c;
			if (++chars_in_buffer == BUFFER_SIZE)
				FlushBuffer();
		}
	}

//-----------------------------------------------------------------------------
//   SendSequence( LPWSTR seq )
// Send the string to the input buffer.
//-----------------------------------------------------------------------------

	void SendSequence( const char *seq )
	{
		if (!*seq)
			return;
		if (*seq == '\e') {
			fprintf(stderr, "VT: SendSequence - '\\e%s'\n", seq + 1);
		} else {
			fprintf(stderr, "VT: SendSequence - '%s'\n", seq);
		}
		vt_shell->InjectInput(seq);
	}


	class AlternativeScreenBuffer
	{
		VTAnsiContext &_ctx;
		bool _enabled = false;
		struct SavedScreenBuffer : std::vector<CHAR_INFO>
		{
			CONSOLE_SCREEN_BUFFER_INFO info;
			bool valid = false;
		} _other;

		public:
		AlternativeScreenBuffer(VTAnsiContext &ctx) : _ctx(ctx)
		{
		}

		void Toggle(HANDLE con_hnd, bool enable)
		{
			if (_enabled == enable)
				return;

			SavedScreenBuffer tmp;
//			HANDLE con_hnd = vt_shell->ConsoleHandle();
			if (!WINPORT(GetConsoleScreenBufferInfo)(con_hnd, &tmp.info)) {
				fprintf(stderr, "AlternativeScreenBuffer: csbi failed\n");
				return;
			}
			if (tmp.info.dwSize.Y > 0 && tmp.info.dwSize.X > 0) {
				tmp.resize((size_t)tmp.info.dwSize.Y * (size_t)tmp.info.dwSize.X);
				COORD origin = {0, 0};
				WINPORT(ReadConsoleOutput)(con_hnd, &tmp[0], tmp.info.dwSize, origin, &tmp.info.srWindow);
			}
			tmp.valid = true;

			if (_other.valid) {
	//			WINPORT(SetConsoleWindowInfo)(con_hnd, TRUE, &_other.info.srWindow);
				COORD origin = {0, 0}, curpos = _other.info.dwCursorPosition;
				SMALL_RECT outrect = _other.info.srWindow;
				if (tmp.info.dwSize.Y < _other.info.dwSize.Y)
					origin.Y = _other.info.dwSize.Y - tmp.info.dwSize.Y;
				if (curpos.X >= tmp.info.dwSize.X)
					curpos.X = (tmp.info.dwSize.X > 0) ? tmp.info.dwSize.X - 1 : 0;
				if (curpos.X >= tmp.info.dwSize.X)
					curpos.Y = (tmp.info.dwSize.Y > 0) ? tmp.info.dwSize.Y - 1 : 0;

				WINPORT(WriteConsoleOutput)(con_hnd, &_other[0], _other.info.dwSize, origin, &outrect);

				// if new screen bigger than saved - fill oversize with emptiness
				if (tmp.info.dwSize.Y > _other.info.dwSize.Y) {
					COORD pos = {0, 0};
					for (pos.Y = _other.info.dwSize.Y + 1; pos.Y <= tmp.info.dwSize.Y; ++pos.Y) {
						DWORD written = 0;
						WINPORT(FillConsoleOutputCharacter)(con_hnd, ' ', tmp.info.dwSize.X, pos, &written);
					}
				}

				if (tmp.info.dwSize.X > _other.info.dwSize.X) {
					COORD pos = { SHORT(tmp.info.srWindow.Left + _other.info.dwSize.X + 1), 0};
					for (pos.Y = 0; pos.Y <= tmp.info.dwSize.Y; ++pos.Y) {
						DWORD written = 0;
						WINPORT(FillConsoleOutputCharacter)(con_hnd, ' ', tmp.info.dwSize.X - _other.info.dwSize.X, pos, &written);
					}
				}

				WINPORT(SetConsoleCursorPosition)(con_hnd, curpos);
				WINPORT(SetConsoleTextAttribute)(con_hnd, _other.info.wAttributes);
				_ctx.ansi_state.font_state.FromConsoleAttributes(_other.info.wAttributes);
	//			fprintf(stderr, "AlternativeScreenBuffer: %d {%d, %d}\n", enable, _other.info.dwCursorPosition.X, _other.info.dwCursorPosition.Y);
			} else {
				COORD zero_pos = {};
				DWORD written = 0;
				WINPORT(FillConsoleOutputCharacter)(con_hnd, ' ',
					(DWORD)tmp.info.dwSize.Y * (DWORD)tmp.info.dwSize.X, zero_pos, &written);
				WINPORT(FillConsoleOutputAttribute)(con_hnd, _ctx.saved_state.csbi.wAttributes,
					(DWORD)tmp.info.dwSize.Y * (DWORD)tmp.info.dwSize.X, zero_pos, &written);
				WINPORT(SetConsoleCursorPosition)(con_hnd, zero_pos);
				WINPORT(SetConsoleTextAttribute)(con_hnd, _ctx.saved_state.csbi.wAttributes);
				_ctx.ansi_state.font_state.FromConsoleAttributes(_ctx.saved_state.csbi.wAttributes);
	//			fprintf(stderr, "AlternativeScreenBuffer: %d XXX %x\n", enable, saved_state.DefaultAttributes());
			}

			std::swap(tmp, _other);

			if (enable) {
				VTLog::Pause();
			} else {
				VTLog::Resume();
			}
			_enabled = enable;
		}

		void Reset(HANDLE con_hnd)
		{
			Toggle(con_hnd, false);
			_other.valid = false;
		}

	} alternative_screen_buffer;

	void LimitByScrollRegion(SMALL_RECT &rect)
	{
		SHORT scroll_top = rect.Top, scroll_bottom = rect.Bottom;
		WINPORT(GetConsoleScrollRegion)(vt_shell->ConsoleHandle(), &scroll_top, &scroll_bottom);
		if (rect.Top < scroll_top)
			rect.Top = scroll_top;
		if (rect.Bottom > scroll_bottom)
			rect.Bottom = scroll_bottom;
	}


// ========== Print functions

//-----------------------------------------------------------------------------
//   InterpretEscSeq()
// Interprets the last escape sequence scanned by ParseAndPrintString
//   prefix             escape sequence prefix
//   es_argc            escape sequence args count
//   es_argv[]          escape sequence args array
//   suffix             escape sequence suffix
//
// for instance, with \e[33;45;1m we have
// prefix = '[',
// es_argc = 3, es_argv[0] = 33, es_argv[1] = 45, es_argv[2] = 1
// suffix = 'm'
//-----------------------------------------------------------------------------

	void ParseOSCPalette(int cmd, const char *args, size_t args_size)
	{
		size_t pos = 0;
		fprintf(stderr, "%s: cmd=%d args=%.*s\n", __FUNCTION__, cmd, (int)args_size, args);
		int orig_index = DecToLong(args, args_size, &pos);
		// Win <-> TTY color index adjustment
		int index = (((orig_index) & 0b001) << 2 | ((orig_index) & 0b100) >> 2 | ((orig_index) & 0b1010));

		DWORD fg = (DWORD)-1, bk = (DWORD)-1;
		if (cmd == 4) {
			if (pos + 2 == args_size && args[pos] == ';' && args[pos + 1] == '?') {
				// not a set color but request current color
				fg = bk = (DWORD)-2;
				WINPORT(OverrideConsoleColor)(vt_shell->ConsoleHandle(),
					(orig_index < 0) ? (DWORD)-1 : (DWORD)index, &fg, &bk);
				if (index == -2) {
					fg = bk;
				}
				// reply with OSC 4 ; index ; rgb : [red] / [green] / [blue] ST
				const auto &reply = StrPrintf("\e]4;%d;rgb:%02x/%02x/%02x\a",
					orig_index, fg & 0xff, (fg >> 8) & 0xff, (fg >> 16) & 0xff);
				SendSequence(reply.c_str());
//				abort();
				return;
			}
			if (pos + 2 >= args_size || args[pos] != ';' || args[pos + 1] != '#') {
				fprintf(stderr, "%s(%d): bad args='%s'\n", __FUNCTION__, cmd, args);
				return;
			}
			pos+= 2;
			size_t saved_pos = pos;
			fg = HexToULong(args, args_size, &pos);
			if (pos == saved_pos) {
				return;
			}
			fg = bk = BGR2RGB(fg);
			if (pos + 2 < args_size && args[pos] == ';' && args[pos + 1] == '#') {
				pos+= 2;
				saved_pos = pos;
				bk = HexToULong(args, args_size, &pos);
				bk = (pos == saved_pos) ? fg : BGR2RGB(bk);
			}
		}

		WINPORT(OverrideConsoleColor)(vt_shell->ConsoleHandle(), index, &fg, &bk);
		// remember very first original...
		orig_palette.emplace(index, std::make_pair(fg, bk));
	}

#define FillBlank( len, Pos )  { \
	DWORD NumberOfCharsWritten; \
	WINPORT(FillConsoleOutputCharacter)( con_hnd, blank_character, len, Pos, &NumberOfCharsWritten );\
	WINPORT(FillConsoleOutputAttribute)( con_hnd, Info.wAttributes, len, Pos, &NumberOfCharsWritten );\
}

	void ClearScreenAndHomeCursor(CONSOLE_SCREEN_BUFFER_INFO &Info)
	{
		COORD Pos;
		HANDLE con_hnd = vt_shell->ConsoleHandle();
		if (Info.srWindow.Top != screen_top || Info.srWindow.Bottom == Info.dwSize.Y - 1) {
			// Rather than clearing the existing window, make the current
			// line the new top of the window (assuming this is the first
			// thing a program does).
			int range = Info.srWindow.Bottom - Info.srWindow.Top;
			if (Info.dwCursorPosition.Y + range < Info.dwSize.Y) {
				Info.srWindow.Top = Info.dwCursorPosition.Y;
				Info.srWindow.Bottom = Info.srWindow.Top + range;
			} else {
				Info.srWindow.Bottom = Info.dwSize.Y - 1;
				Info.srWindow.Top = Info.srWindow.Bottom - range;
				SMALL_RECT Rect;
				Rect.Left = 0;
				Rect.Right = (Info.dwSize.X - 1);
				Rect.Top = Info.dwCursorPosition.Y - Info.srWindow.Top;
				Rect.Bottom = Info.dwCursorPosition.Y - 1;
				Pos.X = Pos.Y = 0;
				CHAR_INFO  CharInfo;
				CI_SET_WCATTR(CharInfo, blank_character, Info.wAttributes);
				WINPORT(ScrollConsoleScreenBuffer)(con_hnd, &Rect, NULL, Pos, &CharInfo);
			}
			WINPORT(SetConsoleWindowInfo)( con_hnd, TRUE, &Info.srWindow );
			screen_top = Info.srWindow.Top;
		}
		Pos.X = 0;
		Pos.Y = Info.srWindow.Top;
		DWORD len   = (Info.srWindow.Bottom - Info.srWindow.Top + 1) * Info.dwSize.X;
		FillBlank( len, Pos );
		// Not technically correct, but perhaps expected.
		WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
	}

	void ClearScreenAndHomeCursor()
	{
		CONSOLE_SCREEN_BUFFER_INFO Info;
		WINPORT(GetConsoleScreenBufferInfo)( vt_shell->ConsoleHandle(), &Info );
		ClearScreenAndHomeCursor(Info);
	}

	void LogFailedEscSeq(const std::string &why)
	{
		std::string s = "\\e";
		if (prefix) s+= prefix;
		if (prefix2) s+= prefix2;
		for (int i = 0; i < es_argc; ++i) {
			s+= StrPrintf(i ? ";%d" : "%d", es_argv[i]);
		}
		if (suffix) s+= suffix;
		if (suffix2) s+= suffix2;
		fprintf(stderr, "FailedEscSeq: '%s' due to '%s'\n", s.c_str(), why.c_str());
	}

	void InterpretEscSeq( void )
	{
		int i;
		DWORD64 attribute;
		CONSOLE_SCREEN_BUFFER_INFO Info;
		CONSOLE_CURSOR_INFO CursInfo;
		DWORD len;
		COORD Pos;
		SMALL_RECT Rect;
		CHAR_INFO  CharInfo;
		DWORD      mode;
		HANDLE con_hnd = vt_shell->ConsoleHandle();
		if (prefix == '[') {
			if (prefix2 == '?' && (suffix == 'h' || suffix == 'l')) {
				for (i = 0; i < es_argc; ++i) {
					switch (es_argv[i]) {
					case AMEX_X10_MOUSE:
						vt_shell->OnMouseExpectation(MEX_X10_MOUSE, suffix == 'h');
						break;
					case AMEX_VT200_MOUSE:
					case AMEX_VT200_HIGHLIGHT_MOUSE:
						vt_shell->OnMouseExpectation(MEX_VT200_MOUSE, suffix == 'h');
						break;
					case AMEX_BTN_EVENT_MOUSE:
						vt_shell->OnMouseExpectation(MEX_BTN_EVENT_MOUSE, suffix == 'h');
						break;
					case AMEX_ANY_EVENT_MOUSE:
						vt_shell->OnMouseExpectation(MEX_ANY_EVENT_MOUSE, suffix == 'h');
						break;
					case AMEX_SGR_EXT_MOUSE:
						vt_shell->OnMouseExpectation(MEX_SGR_EXT_MOUSE, suffix == 'h');
						break;

	//				case 47: case 1047:
	//					alternative_screen_buffer.Toggle(suffix == 'h');
	//					break;

					case 2004:
						vt_shell->OnBracketedPasteExpectation(suffix == 'h');
						break;

					case 9001:
						vt_shell->OnWin32InputMode(suffix == 'h');
						break;

					case 1049:
						alternative_screen_buffer.Toggle(con_hnd, suffix == 'h');
						break;

					case 25:
						WINPORT(GetConsoleCursorInfo)( con_hnd, &CursInfo );
						CursInfo.bVisible = (suffix == 'h');
						WINPORT(SetConsoleCursorInfo)( con_hnd, &CursInfo );
						break;

					case 7:
						mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT;
						if (suffix == 'h')
							mode |= ENABLE_WRAP_AT_EOL_OUTPUT;
						WINPORT(SetConsoleMode)( con_hnd, mode );
						break;

					case 1:
						vt_shell->OnKeypadChange((suffix == 'h') ? 1 : 0);
						break;

					default:
						LogFailedEscSeq(StrPrintf("bad arg %d", i));
				} }
				return;
			}

			// kitty keys stuff

			if (suffix == 'u') {
				if (prefix2 == '=') {
					// assuming mode always 1; we do not support other modes currently
					vt_shell->SetKittyFlags(es_argc > 0 ? es_argv[0] : 0);
					return;

				} else if (prefix2 == '>') {
					// assuming mode always 1; we do not support other modes currently
					vt_shell->SetKittyFlags(es_argc > 0 ? es_argv[0] : 0);
					return;

				} else if (prefix2 == '<') {
					// we do not support mode stack currently, just reset flags
					vt_shell->SetKittyFlags(0);
					return;

				} else if (prefix2 == '?') {
					// reply with "CSI ? flags u"
					char buf[64] = {0};
					snprintf( buf, sizeof(buf), "\x1b[?%du", vt_shell->GetKittyFlags());
					SendSequence( buf );
					return;
				}
			}

			// Ignore any other private sequences.
			if (prefix2 != 0) {
				LogFailedEscSeq(StrPrintf("bad prefix2 %c", prefix2));
				return;			
			}

			WINPORT(GetConsoleScreenBufferInfo)( con_hnd, &Info );
			//fprintf(stderr, "suffix: %c argc: %u argv: %u %u\n", suffix, es_argc, es_argv[0], es_argv[1]);
			switch (suffix) {
			case 'm':
				ansi_state.font_state.ParseSuffixM(es_argv, es_argc);
				attribute = ansi_state.font_state.ToConsoleAttributes();
				WINPORT(SetConsoleTextAttribute)( con_hnd, attribute );
				return;

			case 'J':
				if (es_argc == 0) es_argv[es_argc++] = 0; // ESC[J == ESC[0J
				if (es_argc != 1) return;
				switch (es_argv[0]) {
				case 0:		// ESC[0J erase from cursor to end of display
					len = (Info.srWindow.Bottom - Info.dwCursorPosition.Y) * Info.dwSize.X + Info.dwSize.X - Info.dwCursorPosition.X;
					FillBlank( len, Info.dwCursorPosition );
					return;

				case 1:		// ESC[1J erase from start to cursor.
					Pos.X = 0;
					Pos.Y = Info.srWindow.Top;
					len   = (Info.dwCursorPosition.Y - Info.srWindow.Top) * Info.dwSize.X + Info.dwCursorPosition.X + 1;
					FillBlank( len, Pos );
					return;

				case 2:		// ESC[2J Clear screen and home cursor
					ClearScreenAndHomeCursor(Info);
					return;

				default:
					return;
				}

			case 'K':
				if (es_argc == 0) es_argv[es_argc++] = 0; // ESC[K == ESC[0K
				if (es_argc != 1) return;
				switch (es_argv[0]) {
				case 0:		// ESC[0K Clear to end of line
					len = Info.dwSize.X - Info.dwCursorPosition.X;
					FillBlank( len, Info.dwCursorPosition );
					return;

				case 1:		// ESC[1K Clear from start of line to cursor
					Pos.X = 0;
					Pos.Y = Info.dwCursorPosition.Y;
					FillBlank( Info.dwCursorPosition.X + 1, Pos );
					return;

				case 2:		// ESC[2K Clear whole line.
					Pos.X = 0;
					Pos.Y = Info.dwCursorPosition.Y;
					FillBlank( Info.dwSize.X, Pos );
					return;

				default:
					return;
				}

			case 'X':                 // ESC[#X Erase # characters.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[X == ESC[1X
				if (es_argc != 1) return;
				FillBlank( es_argv[0], Info.dwCursorPosition );
				return;

			case 'L':                 // ESC[#L Insert # blank lines.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[L == ESC[1L
				if (es_argc != 1) return;
				{
					LimitByScrollRegion(Info.srWindow); //fprintf(stderr, "!!!scroll 1\n");

					Rect.Left   = Info.srWindow.Left	= 0;
					Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
					Rect.Top    = Info.dwCursorPosition.Y;
					Rect.Bottom = Info.srWindow.Bottom;

					Pos.X = 0;
					Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
					CI_SET_WCATTR(CharInfo, blank_character, Info.wAttributes);
					WINPORT(ScrollConsoleScreenBuffer)( con_hnd, &Rect, &Info.srWindow, Pos, &CharInfo );
					// Technically should home the cursor, but perhaps not expeclted.
				}
				return;

			case 'S':                 // ESC[#S Scroll up
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[S == ESC[1S
				if (es_argc != 1) return;
				/*while (es_argv[0]--)*/ {
					Pos.X = 0;
					Pos.Y = Info.srWindow.Top;

					LimitByScrollRegion(Info.srWindow); //fprintf(stderr, "!!!scroll 2\n");

					Rect.Left   = Info.srWindow.Left = 0;
					Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
					Rect.Top    = Pos.Y + es_argv[0];
					Rect.Bottom = Info.srWindow.Bottom;

					CI_SET_WCATTR(CharInfo, blank_character, Info.wAttributes);
					WINPORT(ScrollConsoleScreenBuffer)( con_hnd, &Rect, &Info.srWindow, Pos, &CharInfo );
				}
				return;
				
			case 'T':                 // ESC[#T Scroll down
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[T == ESC[1T
				if (es_argc != 1) return;
				/*while (es_argv[0]--)*/ {
					LimitByScrollRegion(Info.srWindow); //fprintf(stderr, "!!!scroll 3\n");

					Rect.Left   = Info.srWindow.Left = 0;
					Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
					Rect.Top    = Info.srWindow.Top;
					Rect.Bottom = Info.srWindow.Bottom - es_argv[0];
					
					Pos.X = 0;
					Pos.Y = Rect.Top + es_argv[0];

					CI_SET_WCATTR(CharInfo, blank_character, Info.wAttributes);
					WINPORT(ScrollConsoleScreenBuffer)( con_hnd, &Rect, &Info.srWindow, Pos, &CharInfo );
				}
				return;
				
				
			case 'M':                 // ESC[#M Delete # lines.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[M == ESC[1M
				if (es_argc != 1) return;
				{
					LimitByScrollRegion(Info.srWindow);
					//fprintf(stderr, "!!!scroll 4 srWindow[%d %d %d %d] by %d\n", Info.srWindow.Left, Info.srWindow.Top, Info.srWindow.Right, Info.srWindow.Bottom, es_argv[0]);

					Rect.Left   = Info.srWindow.Left	= 0;
					Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
					Rect.Bottom = Info.srWindow.Bottom;
					Rect.Top    = Info.dwCursorPosition.Y;
					Pos.X = 0;
					Pos.Y = Rect.Top - es_argv[0];
					CI_SET_WCATTR(CharInfo, blank_character, Info.wAttributes);

					WINPORT(ScrollConsoleScreenBuffer)( con_hnd, &Rect, &Rect, Pos, &CharInfo );
				}
				// Technically should home the cursor, but perhaps not expected.
				return;

			case 'P':                 // ESC[#P Delete # characters.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[P == ESC[1P
				if (es_argc != 1) return;
				Rect.Left   = Info.srWindow.Left	= Info.dwCursorPosition.X; Rect.Left+= es_argv[0];
				Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
				Pos.X	    = Info.dwCursorPosition.X;
				Pos.Y	    =
					Rect.Top    =
						Rect.Bottom = Info.dwCursorPosition.Y;
				CI_SET_WCATTR(CharInfo, blank_character, Info.wAttributes);
				WINPORT(ScrollConsoleScreenBuffer)( con_hnd, &Rect, &Info.srWindow, Pos, &CharInfo );
				return;

			case '@':                 // ESC[#@ Insert # blank characters.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[@ == ESC[1@
				if (es_argc != 1) return;
				Rect.Left   = Info.srWindow.Left	= Info.dwCursorPosition.X;
				Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
				Pos.X	    = Info.dwCursorPosition.X + es_argv[0];
				Pos.Y	    =
					Rect.Top    =
						Rect.Bottom = Info.dwCursorPosition.Y;
				CI_SET_WCATTR(CharInfo, blank_character, Info.wAttributes);
				WINPORT(ScrollConsoleScreenBuffer)( con_hnd, &Rect, &Info.srWindow, Pos, &CharInfo );
				return;

			case 'k':                 // ESC[#k
			case 'A':                 // ESC[#A Moves cursor up # lines
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[A == ESC[1A
				if (es_argc != 1) return;
				Pos.Y = Info.dwCursorPosition.Y - es_argv[0];
				if (Pos.Y < Info.srWindow.Top) Pos.Y = Info.srWindow.Top;
				Pos.X = Info.dwCursorPosition.X;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'e':                 // ESC[#e
			case 'B':                 // ESC[#B Moves cursor down # lines
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[B == ESC[1B
				if (es_argc != 1) return;
				Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
				if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
				Pos.X = Info.dwCursorPosition.X;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'a':                 // ESC[#a
			case 'C':                 // ESC[#C Moves cursor forward # spaces
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[C == ESC[1C
				if (es_argc != 1) return;
				Pos.X = Info.dwCursorPosition.X + es_argv[0];
				if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
				Pos.Y = Info.dwCursorPosition.Y;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'j':                 // ESC[#j
			case 'D':                 // ESC[#D Moves cursor back # spaces
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[D == ESC[1D
				if (es_argc != 1) return;
				Pos.X = Info.dwCursorPosition.X - es_argv[0];
				if (Pos.X < 0) Pos.X = 0;
				Pos.Y = Info.dwCursorPosition.Y;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'E':                 // ESC[#E Moves cursor down # lines, column 1.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[E == ESC[1E
				if (es_argc != 1) return;
				Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
				if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
				Pos.X = 0;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'F':                 // ESC[#F Moves cursor up # lines, column 1.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[F == ESC[1F
				if (es_argc != 1) return;
				Pos.Y = Info.dwCursorPosition.Y - es_argv[0];
				if (Pos.Y < Info.srWindow.Top) Pos.Y = Info.srWindow.Top;
				Pos.X = 0;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case '`':                 // ESC[#`
			case 'G':                 // ESC[#G Moves cursor column # in current row.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[G == ESC[1G
				if (es_argc != 1) return;
				Pos.X = es_argv[0] - 1;
				if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
				if (Pos.X < 0) Pos.X = 0;
				Pos.Y = Info.dwCursorPosition.Y;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'd':                 // ESC[#d Moves cursor row #, current column.
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[d == ESC[1d
				if (es_argc != 1) return;
				Pos.Y = es_argv[0] - 1;
				if (Pos.Y < Info.srWindow.Top) Pos.Y = Info.srWindow.Top;
				if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
				Pos.X = Info.dwCursorPosition.X;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'f':                 // ESC[#;#f
			case 'H':                 // ESC[#;#H Moves cursor to line #, column #
				if (es_argc == 0)
					es_argv[es_argc++] = 1; // ESC[H == ESC[1;1H
				if (es_argc == 1)
					es_argv[es_argc++] = 1; // ESC[#H == ESC[#;1H
				if (es_argc > 2) return;
				Pos.X = es_argv[1] - 1;
				if (Pos.X < 0) Pos.X = 0;
				if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
				Pos.Y = es_argv[0] - 1;
				if (Pos.Y < Info.srWindow.Top) Pos.Y = Info.srWindow.Top;
				if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'I':                 // ESC[#I Moves cursor forward # tabs
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[I == ESC[1I
				if (es_argc != 1) return;
				Pos.Y = Info.dwCursorPosition.Y;
				Pos.X = (Info.dwCursorPosition.X & -8) + es_argv[0] * 8;
				if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'Z':                 // ESC[#Z Moves cursor back # tabs
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[Z == ESC[1Z
				if (es_argc != 1) return;
				Pos.Y = Info.dwCursorPosition.Y;
				if ((Info.dwCursorPosition.X & 7) == 0)
					Pos.X = Info.dwCursorPosition.X - es_argv[0] * 8;
				else
					Pos.X = (Info.dwCursorPosition.X & -8) - (es_argv[0] - 1) * 8;
				if (Pos.X < 0) Pos.X = 0;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'b':                 // ESC[#b Repeat character
				if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[b == ESC[1b
				if (es_argc != 1) return;
				while (--es_argv[0] >= 0)
					PushBuffer( prev_char );
				return;

			case 's':                 // ESC[s Saves cursor position for recall later
				if (es_argc != 0) return;
				ansi_state.saved_pos.X = Info.dwCursorPosition.X;
				ansi_state.saved_pos.Y = Info.dwCursorPosition.Y - Info.srWindow.Top;
				return;

			case 'u':                 // ESC[u Return to saved cursor position
				if (es_argc != 0) return;
				Pos.X = ansi_state.saved_pos.X;
				Pos.Y = ansi_state.saved_pos.Y + Info.srWindow.Top;
				if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
				if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
				WINPORT(SetConsoleCursorPosition)( con_hnd, Pos );
				return;

			case 'n':                 // ESC[#n Device status report
				if (es_argc != 1) return; // ESC[n == ESC[0n -> ignored
				switch (es_argv[0]) {
				case 5:		// ESC[5n Report status
					SendSequence( "\x1b[0n" ); // "OK"
					return;

				case 6: {	// ESC[6n Report cursor position
					char buf[64] = {0};
					snprintf( buf, sizeof(buf), "\x1b[%d;%dR", Info.dwCursorPosition.Y - Info.srWindow.Top + 1, Info.dwCursorPosition.X + 1);
					SendSequence( buf );
				}
				return;

				default:
					return;
				}

			case 't':                 // ESC[#t Window manipulation
				if (es_argc != 1) return;
				if (es_argv[0] == 21) {	// ESC[21t Report xterm window's title
					std::string seq;
					{
						std::lock_guard<std::mutex> lock(title_mutex);
						seq.reserve(cur_title.size() + 8);
						// Too bad if it's too big or fails.
						seq+= ESC;
						seq+= ']';
						seq+= 'l';
						seq+= cur_title;
						seq+= ESC;
						seq+= '\\';
					}
					SendSequence( seq.c_str() );
				}
				return;

			case 'h':                 // ESC[#h Set Mode
				if (es_argc == 1 && es_argv[0] == 3)
					ansi_state.crm = TRUE;
				return;

			case 'l':                 // ESC[#l Reset Mode
				return;			// ESC[3l is handled during parsing

			case 'r':
				if (es_argc < 2) {
					es_argv[1] = MAXSHORT;
					if (es_argc < 1) 
						es_argv[0] = 1;
				}
				fprintf(stderr, "VTAnsi: SET SCROLL REGION: %d %d (limits %d %d)\n", 
					es_argv[0] - 1, es_argv[1] - 1, Info.srWindow.Top, Info.srWindow.Bottom);
				WINPORT(SetConsoleScrollRegion)(con_hnd, es_argv[0] - 1, es_argv[1] - 1);
				return;
			
			case 'c': // CSI P s c Send Device Attributes (Primary DA)
				if (prefix2 == 0 && (es_argc < 1 || es_argv[0] == 0)) {
					SendSequence("\e[?1;2c"); // → CSI ? 1 ; 2 c (‘‘VT100 with Advanced Video Option’’)
					return;
				}

			default:
				LogFailedEscSeq(StrPrintf("bad suffix %c", suffix));
				return;
			}

		} else { // (prefix == ']')
			// Ignore any "private" sequences.
			if (prefix!=']')
				LogFailedEscSeq(StrPrintf("bad prefix %c", prefix));
			if (prefix2 != 0)
				return;

			if (es_argc == 1 && (es_argv[0] == 0 || // ESC]0;titleST - icon (ignored) &
					es_argv[0] == 2)) { // ESC]2;titleST - window
				{
					std::lock_guard<std::mutex> lock(title_mutex);
					cur_title.swap(os_cmd_arg);
				}
				os_cmd_arg.clear();
				ApplyConsoleTitle(con_hnd);

			} else if (es_argc >= 1 && (es_argv[0] == 4 || es_argv[0] == 104)) {
				ParseOSCPalette(es_argv[0], os_cmd_arg.c_str(), os_cmd_arg.size());

			} else {
				vt_shell->OnOSCommand(es_argv[0], os_cmd_arg);
			}
		}
	}

	struct AttrStackEntry
	{
		AttrStackEntry(TCHAR blank_character_, WORD attributes_ )
			: blank_character(blank_character_), attributes(attributes_) {}

		TCHAR blank_character;
		WORD attributes;
	};

	struct AttrStack : std::vector<AttrStackEntry > {} _attr_stack;

	static bool StrStartsWith(const std::string &str, const char *needle)
	{
		size_t l = strlen(needle);
		return (str.size() >= l && memcmp(str.c_str(), needle, l) == 0);
	}

	void InterpretControlString()
	{
		FlushBuffer();
		if (prefix == '_') {//Application Program Command
			if (StrStartsWith(os_cmd_arg, "set-blank="))  {
				blank_character = (os_cmd_arg.size() > 10) ? os_cmd_arg[10] : L' ';

			} else if (os_cmd_arg == "push-attr")  {
				CONSOLE_SCREEN_BUFFER_INFO csbi;
				if (WINPORT(GetConsoleScreenBufferInfo)( vt_shell->ConsoleHandle(), &csbi ) ) {
					_attr_stack.emplace_back(blank_character, csbi.wAttributes);
					if (_attr_stack.size() > 0x1000) {
						fprintf(stderr, "InterpretControlString: too many pushed attributes\n");
						_attr_stack.erase(_attr_stack.begin(), _attr_stack.begin() + _attr_stack.size()/4);
					}
				}

			} else if (os_cmd_arg == "pop-attr")  {
				if (!_attr_stack.empty()) {
					blank_character = _attr_stack.back().blank_character;
					ansi_state.font_state.FromConsoleAttributes(_attr_stack.back().attributes);
					WINPORT(SetConsoleTextAttribute)( vt_shell->ConsoleHandle(), _attr_stack.back().attributes );
					_attr_stack.pop_back();
				}

			} else {
				vt_shell->OnApplicationProtocolCommand(os_cmd_arg.c_str());
			}
		}
		if (os_cmd_arg.capacity() > 0x1000)
			os_cmd_arg.shrink_to_fit();
		os_cmd_arg.clear();
	}

	void PartialLineDown()
	{
		fprintf(stderr, "ANSI: TODO: PartialLineDown\n");
	}

	void PartialLineUp()
	{
		fprintf(stderr, "ANSI: TODO: PartialLineUp\n");
	}

	void ForwardIndex()
	{
		fprintf(stderr, "ANSI: ForwardIndex\n");
		FlushBuffer();	
	}

	void ReverseIndex()
	{
		fprintf(stderr, "ANSI: ReverseIndex\n");
		FlushBuffer();
		HANDLE con_hnd = vt_shell->ConsoleHandle();		
		CONSOLE_SCREEN_BUFFER_INFO info;
		WINPORT(GetConsoleScreenBufferInfo)( con_hnd, &info );
		SHORT scroll_top = 0, scroll_bottom = 0x7fff;
		WINPORT(GetConsoleScrollRegion)(con_hnd, &scroll_top, &scroll_bottom);
		
		if (scroll_top < info.srWindow.Top) scroll_top = info.srWindow.Top;
		if (scroll_bottom < info.srWindow.Top) scroll_bottom = info.srWindow.Top;
		
		if (scroll_top > info.srWindow.Bottom) scroll_top = info.srWindow.Bottom;
		
		if (scroll_bottom > info.srWindow.Bottom) scroll_bottom = info.srWindow.Bottom;
			
		if (info.dwCursorPosition.Y != scroll_top) { 
			info.dwCursorPosition.Y--;
			WINPORT(SetConsoleCursorPosition)(con_hnd, info.dwCursorPosition);
			return;
		}
		
		if (scroll_top>=scroll_bottom)
			return;

		SMALL_RECT Rect = {info.srWindow.Left, scroll_top, info.srWindow.Right, (SHORT)(scroll_bottom - 1) };
		COORD Pos = {0, (SHORT) (scroll_top + 1) };
		CHAR_INFO CharInfo;
		CI_SET_WCATTR(CharInfo, blank_character, info.wAttributes);
		WINPORT(ScrollConsoleScreenBuffer)(con_hnd, &Rect, NULL, Pos, &CharInfo);
	}

	void ResetTerminal()
	{
		fprintf(stderr, "ANSI: ResetTerminal\n");
		WINPORT(SetConsoleScrollRegion)(vt_shell->ConsoleHandle(), 0, MAXSHORT);

		chars_in_buffer = 0;
		memset(char_buffer, 0, sizeof(char_buffer));
		ansi_state = STATE();
		prev_char = 0;
		wrapped = false;

		state = 1;
		prefix = 0;
		prefix2 = 0;
		suffix = 0;
		suffix2 = 0;
		es_argc = 0;
		memset(es_argv, 0, sizeof(es_argv));
		os_cmd_arg.clear();
		//memset(Pt_arg, 0, sizeof(Pt_arg)); Pt_len = 0;
		charset_selection[0] = charset_selection[1] = L'B';
		charset_shifted = false;
		screen_top = -1;
	}


	void SaveCursor()
	{
		FlushBuffer();
		WINPORT(GetConsoleScreenBufferInfo)( vt_shell->ConsoleHandle(), &save_cursor_info );
	}

	void RestoreCursor()
	{
		FlushBuffer();
		WINPORT(SetConsoleCursorPosition)(vt_shell->ConsoleHandle(), save_cursor_info.dwCursorPosition);
	}

//-----------------------------------------------------------------------------
//   ParseAndPrintString(hDev, lpBuffer, nNumberOfBytesToWrite)
// Parses the string lpBuffer, interprets the escapes sequences and prints the
// characters in the device hDev (console).
// The lexer is a three states automata.
// If the number of arguments es_argc > MAX_ARG, only the MAX_ARG-1 firsts and
// the last arguments are processed (no es_argv[] overflow).
//-----------------------------------------------------------------------------

	void ParseAndPrintString(
		LPCVOID lpBuffer,
		DWORD nNumberOfBytesToWrite)
	{
		DWORD   i;
		LPCWSTR s;

		for (i = nNumberOfBytesToWrite, s = (LPCWSTR)lpBuffer; i > 0; i--, s++) {
			if (state == 1) {
				if (*s == ESC) {
					suffix2 = 0;
					//get_state();
					state = (ansi_state.crm) ? 7 : 2;
				} else if (*s == SO) charset_shifted = true;
				else if (*s == SI) charset_shifted = false;
				else PushBuffer( *s );
			} else if (state == 2) {
				if (*s == ESC) ;		// \e\e...\e == \e
				else if (*s == '(' || *s == ')' || *s == '*' || *s == '+') {
					FlushBuffer();
					prefix = *s;
					state = 10;
				}
				else if (*s >= '\x20' && *s <= '\x2f')
					suffix2 = *s;
				else if (suffix2 != 0)
					state = 1;
				else if (*s == '[' ||     // CSI Control Sequence Introducer
						*s == ']') {     // OSC Operating System Command
					FlushBuffer();
					prefix = *s;
					prefix2 = 0;
					os_cmd_arg.clear();
					// Pt_len = 0; *Pt_arg = '\0';
					state = 3;
				} else if (*s == 'P' ||   // DCS Device Control String
						*s == 'X' ||      // SOS Start Of String
						*s == '^' ||      // PM  Privacy Message
						*s == '_') {      // APC Application Program Command
					os_cmd_arg.clear();
					// *Pt_arg = '\0'; Pt_len = 0;
					prefix = *s;
					state = 6;
				} else {
					switch (*s) {
						case 'K': PartialLineDown(); break;
						case 'L': PartialLineUp(); break;
						case 'D': ForwardIndex(); break;
						case 'M': ReverseIndex(); break;
						case 'N': charset_shifted = true; break;
						case 'O': charset_shifted = false; break;
						case 'C': ResetTerminal(); break;
						case '7': SaveCursor(); break;
						case '8': RestoreCursor(); break;
						case 'c': ClearScreenAndHomeCursor(); break;
						default: /*fprintf(stderr, "VTAnsi: state=2 *s=0x%x '%lc'\n", (unsigned int)*s, *s) */ ;
					}
					state = 1;
				}
			} else if (state == 3) {
				if (is_digit( *s )) {
					es_argc = 0;
					es_argv[0] = *s - '0';
					state = 4;
				} else if (*s == ';') {
					es_argc = 1;
					es_argv[0] = 0;
					es_argv[1] = 0;
					state = 4;
				} else if (*s == ':') {
					// ignore it
				} else if (*s >= '\x3b' && *s <= '\x3f') {
					prefix2 = *s;
				} else if (*s >= '\x20' && *s <= '\x2f') {
					suffix2 = *s;
				} else if (suffix2 != 0) {
					state = 1;
				} else {
					es_argc = 0;
					suffix = *s;
					InterpretEscSeq();
					state = 1;
				}
			} else if (state == 4) {
				if (is_digit( *s )) {
					es_argv[es_argc] = 10 * es_argv[es_argc] + (*s - '0');
				} else if (*s == ';') {
					if (es_argc < MAX_ARG-1) es_argc++;
					es_argv[es_argc] = 0;
					if (prefix == ']')
						state = 5;
				} else if (*s >= '\x3a' && *s <= '\x3f') {
					// ignore 'em
				} else if (*s >= '\x20' && *s <= '\x2f') {
					suffix2 = *s;
				} else if (suffix2 != 0) {
					state = 1;
				} else {
					es_argc++;
					suffix = *s;
					InterpretEscSeq();
					state = 1;
				}
			} else if (state == 5 || state == 6) {
				bool done = false;
				if ( *s == BEL) {
					//Pt_arg[Pt_len] = '\0';
					done = true;
				} else if (*s == '\\' && !os_cmd_arg.empty() && os_cmd_arg[os_cmd_arg.size() - 1] == ESC) {
					//Pt_arg[--Pt_len] = '\0';
					os_cmd_arg.resize(os_cmd_arg.size() - 1);
					done = true;
				} else try {
					Wide2MB(s, 1, os_cmd_arg, true);

				} catch (std::exception &e) {
					os_cmd_arg.clear();
					std::string empty;
					os_cmd_arg.swap(empty);
					done = true;
					fprintf(stderr, "ParseAndPrintString: %s\n", e.what());
				}

				if (done) {
					if (state == 6) 
						InterpretControlString();					
					else
						InterpretEscSeq();
					state = 1;
				}

			} else if (state == 7) {
				if (*s == '[') state = 8;
				else {
					PushBuffer( ESC );
					PushBuffer( *s );
					state = 1;
				}
			} else if (state == 8) {
				if (*s == '3') state = 9;
				else {
					PushBuffer( ESC );
					PushBuffer( '[' );
					PushBuffer( *s );
					state = 1;
				}
			} else if (state == 9) {
				if (*s == 'l') ansi_state.crm = FALSE;
				else {
					PushBuffer( ESC );
					PushBuffer( '[' );
					PushBuffer( '3' );
					PushBuffer( *s );
				}
				state = 1;

			} else if (state == 10) {
				switch (prefix) {
					case '(': case '*': charset_selection[0] = *s; break;
					case ')': case '+': charset_selection[1] = *s; break;
				}
				state = 1;
			}
		}
		FlushBuffer();
		ASSERT(i == 0);
	}

	VTAnsiContext()
		: alternative_screen_buffer(*this)
	{
	}	
};


VTAnsi::VTAnsi(IVTShell *vtsh)
	: _ctx(new VTAnsiContext)
{
	_ctx->vt_shell = vtsh;
	_ctx->ResetTerminal();
	_ctx->saved_state.InitFromConsole(_ctx->vt_shell->ConsoleHandle());
	_ctx->ansi_state.font_state.FromConsoleAttributes(_ctx->saved_state.csbi.wAttributes);
	
	VTLog::Start();
	
//	get_state();
}

VTAnsi::~VTAnsi()
{
	VTLog::Stop();
	HANDLE con_hnd = _ctx->vt_shell->ConsoleHandle();
	_ctx->saved_state.ApplyToConsole(con_hnd);
	WINPORT(FlushConsoleInputBuffer)(con_hnd);
	_ctx->vt_shell = nullptr;
}

void VTAnsi::DisableOutput()
{
	_ctx->output_disabled = true;
}

void VTAnsi::EnableOutput()
{
	if (_ctx->output_disabled) {
		_ctx->chars_in_buffer = 0;
		_ctx->output_disabled = false;
	}
}

struct VTAnsiState *VTAnsi::Suspend()
{
	VTAnsiState *out = new(std::nothrow) VTAnsiState;
	if (out) {
		HANDLE con_hnd = _ctx->vt_shell->ConsoleHandle();
		out->InitFromConsole(con_hnd);
		_ctx->saved_state.ApplyToConsole(con_hnd);
	} else
		perror("VTAnsi::Suspend");

	return out;
}

void VTAnsi::Resume(struct VTAnsiState* state)
{
	state->ApplyToConsole(_ctx->vt_shell->ConsoleHandle());
	delete state;
}

void VTAnsi::OnStart()
{
	HANDLE con_hnd = _ctx->vt_shell->ConsoleHandle();
	_ctx->saved_state.InitFromConsole(con_hnd);
	TCHAR buf[MAX_PATH*2] = {0};
	WINPORT(GetConsoleTitle)(con_hnd, buf, ARRAYSIZE(buf) - 1 );
	_saved_title = buf;
}

void VTAnsi::OnStop()
{
	HANDLE con_hnd = _ctx->vt_shell->ConsoleHandle();
	RevertConsoleState(con_hnd);
	_incomplete.tail.clear();
	_ctx->orig_palette.clear();
	_ctx->alternative_screen_buffer.Reset(con_hnd);
	//_ctx->saved_state.ApplyToConsole(con_hnd, false);
	_ctx->ResetTerminal();
	_ctx->ansi_state.font_state.FromConsoleAttributes(_ctx->saved_state.csbi.wAttributes);
}

void VTAnsi::OnDetached()
{
	WINPORT(GetConsoleScrollRegion)(NULL, &_detached_state.scrl_top, &_detached_state.scrl_bottom);
	RevertConsoleState(NULL);
}

void VTAnsi::OnReattached()
{
	HANDLE con_hnd = _ctx->vt_shell->ConsoleHandle();
	for (auto it : _detached_state.palette) {
		WINPORT(OverrideConsoleColor)(con_hnd, it.first, &it.second.first, &it.second.second);
	}
	WINPORT(SetConsoleScrollRegion)(con_hnd, _detached_state.scrl_top, _detached_state.scrl_bottom);
	_ctx->ApplyConsoleTitle(con_hnd);
}


void VTAnsi::RevertConsoleState(HANDLE con_hnd)
{
	// restore all changed palette colors with swapping altered values in case will reattach
	_detached_state.palette = _ctx->orig_palette;
	for (auto &it : _detached_state.palette) {
		WINPORT(OverrideConsoleColor)(con_hnd, it.first, &it.second.first, &it.second.second);
	}
	WINPORT(SetConsoleScrollRegion)(con_hnd, 0, MAXSHORT);
	WINPORT(SetConsoleTitle)(con_hnd, _saved_title.c_str());
	_ctx->saved_state.ApplyToConsole(con_hnd);
}

void VTAnsi::Write(const char *str, size_t len)
{
	if (!_incomplete.tail.empty()) {
		_incomplete.tmp = _incomplete.tail;
		_incomplete.tail.clear();
		_incomplete.tmp.append(str, len);
		Write(_incomplete.tmp.c_str(), _incomplete.tmp.size());
		return;
	}

	_ws.clear();
	StdPushBack<std::wstring> pb(_ws);
	while (len) {
		size_t len_cvt = len;
		UtfConvert(str, len_cvt, pb, true);
		str+= len_cvt;
		len-= len_cvt;
		if (len < MAX_MB_CHARS_PER_WCHAR) {
			if (len) {
				_incomplete.tail.assign(str, len);
			}
			break;
		}
		pb.push_back(0xEE00 + *(unsigned char *)str);
		++str;
		--len;
	}

	ConsoleRepaintsDeferScope crds(_ctx->vt_shell->ConsoleHandle());
	_ctx->ParseAndPrintString(_ws.c_str(), _ws.size());
}


std::string VTAnsi::GetTitle()
{
	std::lock_guard<std::mutex> lock(_ctx->title_mutex);
	return _ctx->cur_title;
}
