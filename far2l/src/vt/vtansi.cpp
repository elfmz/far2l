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
#include "vtansi.h"
#include "AnsiEsc.hpp"
#include "UtfConvert.hpp"

#define is_digit(c) ('0' <= (c) && (c) <= '9')

// ========== Global variables and constants


#include "vtlog.h"

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

static CONSOLE_SCREEN_BUFFER_INFO save_cursor_info = {};
static VTAnsiState g_saved_state;
static std::mutex g_vt_ansi_mutex;
IVTShell *g_vt_shell = nullptr;
static std::string g_title;
static std::atomic<bool> g_output_disabled{false};

static HANDLE	  hConOut = NULL;		// handle to CONOUT$

#define ESC	'\x1B'          // ESCape character
#define BEL	'\x07'
#define SO	'\x0E'          // Shift Out
#define SI	'\x0F'          // Shift In
#define ST	'\x9c'

#define MAX_ARG 16		// max number of args in an escape sequence
static int   state;			// automata state
static char  prefix;			// escape sequence prefix ( '[', ']' or '(' );
static char  prefix2;			// secondary prefix ( '?' or '>' );
static char  suffix;			// escape sequence suffix
static char  suffix2;			// escape sequence secondary suffix
static int   es_argc;			// escape sequence args count
static int   es_argv[MAX_ARG]; 	// escape sequence args
static std::string os_cmd_arg;	// text parameter for Operating System Command
static int   screen_top = -1;		// initial window top when cleared
static TCHAR blank_character = L' ';


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

static bool  charset_shifted = false;
static WCHAR charset_selection[2] = {L'B', L'B'};
static WCHAR &CurrentCharsetSelection()
{
	return charset_selection[charset_shifted ? 1 : 0];
}

#define FIRST_SG '_'
#define LAST_SG  '~'


// color constants

static struct STATE {
	COORD              saved_pos{};	// saved cursor position
	AnsiEsc::FontState font_state{};
	bool               crm = false;		// showing control characters?
} ansiState;

// ========== Print Buffer functions

#define BUFFER_SIZE 2048

static int   nCharInBuffer;
static WCHAR ChBuffer[BUFFER_SIZE];
static WCHAR ChPrev;
static BOOL  fWrapped;


static void ApplyConsoleTitle()
{
	std::wstring title(1, L'[');
	title+= StrMB2Wide(g_title);
	title+= L']';
	WINPORT(SetConsoleTitle)(title.c_str() );
}

//-----------------------------------------------------------------------------
//   FlushBuffer()
// Writes the buffer to the console and empties it.
//-----------------------------------------------------------------------------

static void WriteConsoleIfEnabled(const WCHAR *str, DWORD len)
{
	DWORD written;
	if (!g_output_disabled) {
		WINPORT(WriteConsole)( hConOut, str, len, &written, NULL );
	}
}

static void FlushBuffer( void )
{
	DWORD nWritten;

	if (g_output_disabled) {
		nCharInBuffer = 0;
	}

//fprintf(stderr, "FlushBuffer: %u\n", nCharInBuffer);
	if (nCharInBuffer <= 0)
		return;

	if (ansiState.crm) {
		DWORD mode = 0;
		WINPORT(GetConsoleMode)( hConOut, &mode );
		WINPORT(SetConsoleMode)( hConOut, mode & ~(ENABLE_PROCESSED_OUTPUT) );
		WINPORT(WriteConsole)( hConOut, ChBuffer, nCharInBuffer, &nWritten, NULL );
		WINPORT(SetConsoleMode)( hConOut, mode );
	} else {
		//HANDLE hConWrap;
		//CONSOLE_CURSOR_INFO cci;
		CONSOLE_SCREEN_BUFFER_INFO csbi, csbi_before;

			LPWSTR b = ChBuffer;
			do {
				WINPORT(GetConsoleScreenBufferInfo)( hConOut, &csbi_before );
				WINPORT(WriteConsole)( hConOut, b, 1, &nWritten, NULL );
				if (*b != '\r' && *b != '\b' && *b != '\a') {
					WINPORT(GetConsoleScreenBufferInfo)( hConOut, &csbi );
					if (csbi.dwCursorPosition.X == 0  || csbi.dwCursorPosition.X==csbi_before.dwCursorPosition.X)
						fWrapped = TRUE;
				}
			} while (++b, --nCharInBuffer);

		/*if (nCharInBuffer < 4) {
			LPWSTR b = ChBuffer;
			do {
				WINPORT(WriteConsole)( hConOut, b, 1, &nWritten, NULL );
				if (*b != '\r' && *b != '\b' && *b != '\a') {
					WINPORT(GetConsoleScreenBufferInfo)( hConOut, &csbi );
					if (csbi.dwCursorPosition.X == 0)
						fWrapped = TRUE;
				}
			} while (++b, --nCharInBuffer);
		} else {
			fprintf(stderr, "TODODODODO\n");
			

			// To detect wrapping of multiple characters, create a new buffer, write
			// to the top of it and see if the cursor changes line.  This doesn't
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
			GetConsoleScreenBufferInfo( hConOut, &csbi );
			csbi.dwSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 2;
			SetConsoleScreenBufferSize( hConWrap, csbi.dwSize );
			// Put the cursor on the top line, in the same column.
			csbi.dwCursorPosition.Y = 0;
			SetConsoleCursorPosition( hConWrap, csbi.dwCursorPosition );
			WriteConsole( hConWrap, ChBuffer, nCharInBuffer, &nWritten, NULL );
			GetConsoleScreenBufferInfo( hConWrap, &csbi );
			if (csbi.dwCursorPosition.Y != 0)
				fWrapped = TRUE;
			CloseHandle( hConWrap );
			WINPORT(WriteConsole)( hConOut, ChBuffer, nCharInBuffer, &nWritten, NULL );
		}*/
	}
	nCharInBuffer = 0;
}

//-----------------------------------------------------------------------------
//   PushBuffer( WCHAR c )
// Adds a character in the buffer.
//-----------------------------------------------------------------------------

void PushBuffer( WCHAR c )
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	ChPrev = c;

	if (c == '\n') {
		if (ansiState.crm)
			ChBuffer[nCharInBuffer++] = c;
		FlushBuffer();
		// Avoid writing the newline if wrap has already occurred.
		WINPORT(GetConsoleScreenBufferInfo)( hConOut, &csbi );
		if (ansiState.crm) {
			// If we're displaying controls, then the only way we can be on the left
			// margin is if wrap occurred.
			if (csbi.dwCursorPosition.X != 0)
				WriteConsoleIfEnabled( L"\n", 1);
		} else {
			LPCWSTR nl = L"\n";
			if (fWrapped) {
				// It's wrapped, but was anything more written?  Look at the current
				// row, checking that each character is space in current attributes.
				// If it's all blank we can drop the newline.  If the cursor isn't
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
					WINPORT(ReadConsoleOutput)( hConOut, row, s, c, &r );
					blank.Char.UnicodeChar = blank_character;
					blank.Attributes = csbi.wAttributes;
					while (*(PDWORD)&row[c.X] == *(PDWORD)&blank)
						if (++c.X == s.X) {
							nl = (csbi.dwCursorPosition.X == 0) ? NULL : L"\r";
							break;
						}
					free( row );
				}
				fWrapped = FALSE;
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
		ChBuffer[nCharInBuffer] = c;
		if (++nCharInBuffer == BUFFER_SIZE)
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
	
	fprintf(stderr, "VT: SendSequence - '%s'\n", seq);
	if (g_vt_shell) {
		g_vt_shell->InjectInput(seq);
		return;
	}


	std::vector<INPUT_RECORD> irs(strlen(seq));
	for (size_t i = 0; i < irs.size(); ++i) {
		INPUT_RECORD &ir = irs[i];
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.bKeyDown = TRUE;
		ir.Event.KeyEvent.wRepeatCount = 1;
		ir.Event.KeyEvent.wVirtualKeyCode = 0;
		ir.Event.KeyEvent.wVirtualScanCode = 0;
		ir.Event.KeyEvent.dwControlKeyState = 0;
		ir.Event.KeyEvent.uChar.UnicodeChar = seq[i];
	}

	HANDLE hStdIn = NULL;//WINPORT(GetStdHandle)( STD_INPUT_HANDLE );
	DWORD out;
	WINPORT(WriteConsoleInput)( hStdIn, &irs[0], irs.size(), &out );
}


class AlternativeScreenBuffer
{
	bool _enabled = false;
	struct SavedScreenBuffer : std::vector<CHAR_INFO>
	{
		CONSOLE_SCREEN_BUFFER_INFO info;
		bool valid = false;
	} _other;

	public:

	void Toggle(bool enable)
	{
		if (_enabled == enable)
			return;

		SavedScreenBuffer tmp;
		if (!WINPORT(GetConsoleScreenBufferInfo)(NULL, &tmp.info)) {
			fprintf(stderr, "AlternativeScreenBuffer: csbi failed\n");
			return;
		}
		if (tmp.info.dwSize.Y > 0 && tmp.info.dwSize.X > 0) {
			tmp.resize((size_t)tmp.info.dwSize.Y * (size_t)tmp.info.dwSize.X);
			COORD origin = {0, 0};
			WINPORT(ReadConsoleOutput)(NULL, &tmp[0], tmp.info.dwSize, origin, &tmp.info.srWindow);
		}
		tmp.valid = true;

		if (_other.valid) {
//			WINPORT(SetConsoleWindowInfo)(NULL, TRUE, &_other.info.srWindow);
			COORD origin = {0, 0}, curpos = _other.info.dwCursorPosition;
			SMALL_RECT outrect = _other.info.srWindow;
			if (tmp.info.dwSize.Y < _other.info.dwSize.Y)
				origin.Y = _other.info.dwSize.Y - tmp.info.dwSize.Y;
			if (curpos.X >= tmp.info.dwSize.X)
				curpos.X = (tmp.info.dwSize.X > 0) ? tmp.info.dwSize.X - 1 : 0;
			if (curpos.X >= tmp.info.dwSize.X)
				curpos.Y = (tmp.info.dwSize.Y > 0) ? tmp.info.dwSize.Y - 1 : 0;

			WINPORT(WriteConsoleOutput)(NULL, &_other[0], _other.info.dwSize, origin, &outrect);

			// if new screen bigger than saved - fill oversize with emptiness
			if (tmp.info.dwSize.Y > _other.info.dwSize.Y) {
				COORD pos = {0, 0};
				for (pos.Y = _other.info.dwSize.Y + 1; pos.Y <= tmp.info.dwSize.Y; ++pos.Y) {
					DWORD written = 0;
					WINPORT(FillConsoleOutputCharacter)(NULL, ' ', tmp.info.dwSize.X, pos, &written);
				}
			}

			if (tmp.info.dwSize.X > _other.info.dwSize.X) {
				COORD pos = { SHORT(tmp.info.srWindow.Left + _other.info.dwSize.X + 1), 0};
				for (pos.Y = 0; pos.Y <= tmp.info.dwSize.Y; ++pos.Y) {
					DWORD written = 0;
					WINPORT(FillConsoleOutputCharacter)(NULL, ' ', tmp.info.dwSize.X - _other.info.dwSize.X, pos, &written);
				}
			}

			WINPORT(SetConsoleCursorPosition)(NULL, curpos);
			WINPORT(SetConsoleTextAttribute)(NULL, _other.info.wAttributes);
			ansiState.font_state.FromConsoleAttributes(_other.info.wAttributes);
//			fprintf(stderr, "AlternativeScreenBuffer: %d {%d, %d}\n", enable, _other.info.dwCursorPosition.X, _other.info.dwCursorPosition.Y);
		} else {
			COORD zero_pos = {};
			DWORD written = 0;
			WINPORT(FillConsoleOutputCharacter)(NULL, ' ',
				(DWORD)tmp.info.dwSize.Y * (DWORD)tmp.info.dwSize.X, zero_pos, &written);
			WINPORT(FillConsoleOutputAttribute)(NULL, g_saved_state.csbi.wAttributes,
				(DWORD)tmp.info.dwSize.Y * (DWORD)tmp.info.dwSize.X, zero_pos, &written);
			WINPORT(SetConsoleCursorPosition)(NULL, zero_pos);
			WINPORT(SetConsoleTextAttribute)(NULL, g_saved_state.csbi.wAttributes);
			ansiState.font_state.FromConsoleAttributes(g_saved_state.csbi.wAttributes);
//			fprintf(stderr, "AlternativeScreenBuffer: %d XXX %x\n", enable, g_saved_state.DefaultAttributes());
		}

		std::swap(tmp, _other);

		if (enable) {
			VTLog::Pause();
		} else {
			VTLog::Resume();
		}
		_enabled = enable;
	}

	void Reset()
	{
		Toggle(false);
		_other.valid = false;
	}

} g_alternative_screen_buffer;


static void LimitByScrollRegion(SMALL_RECT &rect)
{
	SHORT scroll_top = rect.Top, scroll_bottom = rect.Bottom;
	WINPORT(GetConsoleScrollRegion)(NULL, &scroll_top, &scroll_bottom);
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

void InterpretEscSeq( void )
{
	int  i;
	WORD attribut;
	CONSOLE_SCREEN_BUFFER_INFO Info;
	CONSOLE_CURSOR_INFO CursInfo;
	DWORD len, NumberOfCharsWritten;
	COORD Pos;
	SMALL_RECT Rect;
	CHAR_INFO  CharInfo;
	DWORD      mode;

#define FillBlank( len, Pos )  \
	WINPORT(FillConsoleOutputCharacter)( hConOut, blank_character, len, Pos, &NumberOfCharsWritten );\
	WINPORT(FillConsoleOutputAttribute)( hConOut, Info.wAttributes, len, Pos, \
	                                     &NumberOfCharsWritten )

	if (prefix == '[') {
		if (prefix2 == '?' && (suffix == 'h' || suffix == 'l')) {
			for (i = 0; i < es_argc; ++i) {
				switch (es_argv[i]) {
//				case 47: case 1047:
//					g_alternative_screen_buffer.Toggle(suffix == 'h');
//					break;

				case 1049:
					g_alternative_screen_buffer.Toggle(suffix == 'h');
					break;

				case 25:
					WINPORT(GetConsoleCursorInfo)( hConOut, &CursInfo );
					CursInfo.bVisible = (suffix == 'h');
					WINPORT(SetConsoleCursorInfo)( hConOut, &CursInfo );
					break;

				case 7:
					mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT;
					if (suffix == 'h')
						mode |= ENABLE_WRAP_AT_EOL_OUTPUT;
					WINPORT(SetConsoleMode)( hConOut, mode );
					break;

				case 1:
					if (g_vt_shell)
						g_vt_shell->OnKeypadChange((suffix == 'h') ? 1 : 0);
					break;

				default:
					fprintf(stderr, "Ignoring: %c %c %u %u\n", prefix2, suffix, es_argc, es_argv[i]);
			} }
			return;
		}
		// Ignore any other private sequences.
		if (prefix2 != 0) {
			fprintf(stderr, "Ignoring: %c %c %u %u\n", prefix2, suffix, es_argc, es_argv[0]);
			return;			
		}

		WINPORT(GetConsoleScreenBufferInfo)( hConOut, &Info );
		//fprintf(stderr, "suffix: %c argc: %u argv: %u %u\n", suffix, es_argc, es_argv[0], es_argv[1]);
		switch (suffix) {
		case 'm':
			ansiState.font_state.ParseSuffixM(es_argv, es_argc);
			attribut = ansiState.font_state.ToConsoleAttributes();
			WINPORT(SetConsoleTextAttribute)( hConOut, attribut );
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
						Rect.Left = 0;
						Rect.Right = (Info.dwSize.X - 1);
						Rect.Top = Info.dwCursorPosition.Y - Info.srWindow.Top;
						Rect.Bottom = Info.dwCursorPosition.Y - 1;
						Pos.X = Pos.Y = 0;
						CharInfo.Char.UnicodeChar = blank_character;
						CharInfo.Attributes = Info.wAttributes;
						WINPORT(ScrollConsoleScreenBuffer)(hConOut, &Rect, NULL, Pos, &CharInfo);
					}
					WINPORT(SetConsoleWindowInfo)( hConOut, TRUE, &Info.srWindow );
					screen_top = Info.srWindow.Top;
				}
				Pos.X = 0;
				Pos.Y = Info.srWindow.Top;
				len   = (Info.srWindow.Bottom - Info.srWindow.Top + 1) * Info.dwSize.X;
				FillBlank( len, Pos );
				// Not technically correct, but perhaps expected.
				WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
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
				LimitByScrollRegion(Info.srWindow);//fprintf(stderr, "!!!scroll 1\n");

				Rect.Left   = Info.srWindow.Left	= 0;
				Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
				Rect.Top    = Info.dwCursorPosition.Y;
				Rect.Bottom = Info.srWindow.Bottom;

				Pos.X = 0;
				Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
				CharInfo.Char.UnicodeChar = blank_character;
				CharInfo.Attributes = Info.wAttributes;
				WINPORT(ScrollConsoleScreenBuffer)( hConOut, &Rect, &Info.srWindow, Pos, &CharInfo );
				// Technically should home the cursor, but perhaps not expeclted.
			}
			return;

		case 'S':                 // ESC[#S Scroll up
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[S == ESC[1S
			if (es_argc != 1) return;
			/*while (es_argv[0]--)*/ {
				Pos.X = 0;
				Pos.Y = Info.srWindow.Top;

				LimitByScrollRegion(Info.srWindow);//fprintf(stderr, "!!!scroll 2\n");

				Rect.Left   = Info.srWindow.Left = 0;
				Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
				Rect.Top    = Pos.Y + es_argv[0];
				Rect.Bottom = Info.srWindow.Bottom;

				CharInfo.Char.UnicodeChar = blank_character;
				CharInfo.Attributes = Info.wAttributes;
				WINPORT(ScrollConsoleScreenBuffer)( hConOut, &Rect, &Info.srWindow, Pos, &CharInfo );
			}
			return;
			
		case 'T':                 // ESC[#T Scroll down
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[T == ESC[1T
			if (es_argc != 1) return;
			/*while (es_argv[0]--)*/ {
				LimitByScrollRegion(Info.srWindow);//fprintf(stderr, "!!!scroll 3\n");

				Rect.Left   = Info.srWindow.Left = 0;
				Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
				Rect.Top    = Info.srWindow.Top;
				Rect.Bottom = Info.srWindow.Bottom - es_argv[0];
				
				Pos.X = 0;
				Pos.Y = Rect.Top + es_argv[0];

				CharInfo.Char.UnicodeChar = blank_character;
				CharInfo.Attributes = Info.wAttributes;
				WINPORT(ScrollConsoleScreenBuffer)( hConOut, &Rect, &Info.srWindow, Pos, &CharInfo );
			}
			return;
			
			
		case 'M':                 // ESC[#M Delete # lines.
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[M == ESC[1M
			if (es_argc != 1) return;
			{
				LimitByScrollRegion(Info.srWindow);//fprintf(stderr, "!!!scroll 4\n");

				Rect.Left   = Info.srWindow.Left	= 0;
				Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
				Rect.Bottom = Info.srWindow.Bottom;
				Rect.Top    = Info.dwCursorPosition.Y + es_argv[0];
				Pos.X = 0;
				Pos.Y = Info.srWindow.Top = Info.dwCursorPosition.Y;
				CharInfo.Char.UnicodeChar = blank_character;
				CharInfo.Attributes = Info.wAttributes;

				Info.srWindow.Bottom-= es_argv[0];
				WINPORT(ScrollConsoleScreenBuffer)( hConOut, &Rect, &Info.srWindow, Pos, &CharInfo );
			}
			// Technically should home the cursor, but perhaps not expected.
			return;

		case 'P':                 // ESC[#P Delete # characters.
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[P == ESC[1P
			if (es_argc != 1) return;
			Rect.Left   = Info.srWindow.Left	= Info.dwCursorPosition.X;  Rect.Left+= es_argv[0];
			Rect.Right  = Info.srWindow.Right = (Info.dwSize.X - 1);
			Pos.X	    = Info.dwCursorPosition.X;
			Pos.Y	    =
			    Rect.Top    =
			        Rect.Bottom = Info.dwCursorPosition.Y;
			CharInfo.Char.UnicodeChar = blank_character;
			CharInfo.Attributes = Info.wAttributes;
			WINPORT(ScrollConsoleScreenBuffer)( hConOut, &Rect, &Info.srWindow, Pos, &CharInfo );
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
			CharInfo.Char.UnicodeChar = blank_character;
			CharInfo.Attributes = Info.wAttributes;
			WINPORT(ScrollConsoleScreenBuffer)( hConOut, &Rect, &Info.srWindow, Pos, &CharInfo );
			return;

		case 'k':                 // ESC[#k
		case 'A':                 // ESC[#A Moves cursor up # lines
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[A == ESC[1A
			if (es_argc != 1) return;
			Pos.Y = Info.dwCursorPosition.Y - es_argv[0];
			if (Pos.Y < Info.srWindow.Top) Pos.Y = Info.srWindow.Top;
			Pos.X = Info.dwCursorPosition.X;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'e':                 // ESC[#e
		case 'B':                 // ESC[#B Moves cursor down # lines
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[B == ESC[1B
			if (es_argc != 1) return;
			Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
			if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
			Pos.X = Info.dwCursorPosition.X;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'a':                 // ESC[#a
		case 'C':                 // ESC[#C Moves cursor forward # spaces
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[C == ESC[1C
			if (es_argc != 1) return;
			Pos.X = Info.dwCursorPosition.X + es_argv[0];
			if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
			Pos.Y = Info.dwCursorPosition.Y;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'j':                 // ESC[#j
		case 'D':                 // ESC[#D Moves cursor back # spaces
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[D == ESC[1D
			if (es_argc != 1) return;
			Pos.X = Info.dwCursorPosition.X - es_argv[0];
			if (Pos.X < 0) Pos.X = 0;
			Pos.Y = Info.dwCursorPosition.Y;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'E':                 // ESC[#E Moves cursor down # lines, column 1.
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[E == ESC[1E
			if (es_argc != 1) return;
			Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
			if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
			Pos.X = 0;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'F':                 // ESC[#F Moves cursor up # lines, column 1.
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[F == ESC[1F
			if (es_argc != 1) return;
			Pos.Y = Info.dwCursorPosition.Y - es_argv[0];
			if (Pos.Y < Info.srWindow.Top) Pos.Y = Info.srWindow.Top;
			Pos.X = 0;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case '`':                 // ESC[#`
		case 'G':                 // ESC[#G Moves cursor column # in current row.
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[G == ESC[1G
			if (es_argc != 1) return;
			Pos.X = es_argv[0] - 1;
			if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
			if (Pos.X < 0) Pos.X = 0;
			Pos.Y = Info.dwCursorPosition.Y;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'd':                 // ESC[#d Moves cursor row #, current column.
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[d == ESC[1d
			if (es_argc != 1) return;
			Pos.Y = es_argv[0] - 1;
			if (Pos.Y < Info.srWindow.Top) Pos.Y = Info.srWindow.Top;
			if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
			Pos.X = Info.dwCursorPosition.X;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
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
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'I':                 // ESC[#I Moves cursor forward # tabs
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[I == ESC[1I
			if (es_argc != 1) return;
			Pos.Y = Info.dwCursorPosition.Y;
			Pos.X = (Info.dwCursorPosition.X & -8) + es_argv[0] * 8;
			if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
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
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'b':                 // ESC[#b Repeat character
			if (es_argc == 0) es_argv[es_argc++] = 1; // ESC[b == ESC[1b
			if (es_argc != 1) return;
			while (--es_argv[0] >= 0)
				PushBuffer( ChPrev );
			return;

		case 's':                 // ESC[s Saves cursor position for recall later
			if (es_argc != 0) return;
			ansiState.saved_pos.X = Info.dwCursorPosition.X;
			ansiState.saved_pos.Y = Info.dwCursorPosition.Y - Info.srWindow.Top;
			return;

		case 'u':                 // ESC[u Return to saved cursor position
			if (es_argc != 0) return;
			Pos.X = ansiState.saved_pos.X;
			Pos.Y = ansiState.saved_pos.Y + Info.srWindow.Top;
			if (Pos.X > (Info.dwSize.X - 1)) Pos.X = (Info.dwSize.X - 1);
			if (Pos.Y > Info.srWindow.Bottom) Pos.Y = Info.srWindow.Bottom;
			WINPORT(SetConsoleCursorPosition)( hConOut, Pos );
			return;

		case 'n':                 // ESC[#n Device status report
			if (es_argc != 1) return; // ESC[n == ESC[0n -> ignored
			switch (es_argv[0]) {
			case 5:		// ESC[5n Report status
				SendSequence( "\x1b[0n" ); // "OK"
				return;

			case 6: {	// ESC[6n Report cursor position
				char buf[64] = {0};
				sprintf( buf, "\x1b[%d;%dR", Info.dwCursorPosition.Y - Info.srWindow.Top + 1, Info.dwCursorPosition.X + 1);
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
				seq.reserve(g_title.size() + 8);
				// Too bad if it's too big or fails.
				seq+= ESC;
				seq+= ']';
				seq+= 'l';
				seq+= g_title;
				seq+= ESC;
				seq+= '\\';
				SendSequence( seq.c_str() );
			}
			return;

		case 'h':                 // ESC[#h Set Mode
			if (es_argc == 1 && es_argv[0] == 3)
				ansiState.crm = TRUE;
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
			WINPORT(SetConsoleScrollRegion)(NULL, es_argv[0] - 1, es_argv[1] - 1);
			return;
		
		default:
			fprintf(stderr, "VTAnsi: unknown suffix %c\n", suffix);
			return;
		}
	} else { // (prefix == ']')
		// Ignore any "private" sequences.
		if (prefix!=']') fprintf(stderr, "VTAnsi: unknown prefix= %c\n", prefix);
		if (prefix2 != 0)
			return;

		if (es_argc == 1 && (es_argv[0] == 0 || // ESC]0;titleST - icon (ignored) &
		                     es_argv[0] == 2)) { // ESC]2;titleST - window
			g_title.swap(os_cmd_arg);
			os_cmd_arg.clear();
			ApplyConsoleTitle();
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

static struct AttrStack : std::vector<AttrStackEntry > {} g_attr_stack;

static bool StrStartsWith(const std::string &str, const char *needle)
{
	size_t l = strlen(needle);
	return (str.size() >= l && memcmp(str.c_str(), needle, l) == 0);
}

static void InterpretControlString()
{
	FlushBuffer();
	if (prefix == '_') {//Application Program Command
		if (StrStartsWith(os_cmd_arg, "set-blank="))  {
			blank_character = (os_cmd_arg.size() > 10) ? os_cmd_arg[10] : L' ';

		} else if (os_cmd_arg == "push-attr")  {
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			if (WINPORT(GetConsoleScreenBufferInfo)( hConOut, &csbi ) ) {
				g_attr_stack.emplace_back(blank_character, csbi.wAttributes);
				if (g_attr_stack.size() > 0x1000) {
					fprintf(stderr, "InterpretControlString: too many pushed attributes\n");
					g_attr_stack.erase(g_attr_stack.begin(), g_attr_stack.begin() + g_attr_stack.size()/4);
				}
			}

		} else if (os_cmd_arg == "pop-attr")  {
			if (!g_attr_stack.empty()) {
				blank_character = g_attr_stack.back().blank_character;
				ansiState.font_state.FromConsoleAttributes(g_attr_stack.back().attributes);
				WINPORT(SetConsoleTextAttribute)( hConOut, g_attr_stack.back().attributes );
				g_attr_stack.pop_back();
			}

		} else if (g_vt_shell) {
			g_vt_shell->OnApplicationProtocolCommand(os_cmd_arg.c_str());
		}
	}
	if (os_cmd_arg.capacity() > 0x1000)
		os_cmd_arg.shrink_to_fit();
	os_cmd_arg.clear();
}

static void PartialLineDown()
{
	fprintf(stderr, "ANSI: TODO: PartialLineDown\n");
}

static void PartialLineUp()
{
	fprintf(stderr, "ANSI: TODO: PartialLineUp\n");
}

static void ForwardIndex()
{
	fprintf(stderr, "ANSI: ForwardIndex\n");
	FlushBuffer();	
}

static void ReverseIndex()
{
	fprintf(stderr, "ANSI: ReverseIndex\n");
	FlushBuffer();
	
	CONSOLE_SCREEN_BUFFER_INFO info;
	WINPORT(GetConsoleScreenBufferInfo)( hConOut, &info );
	SHORT scroll_top = 0, scroll_bottom = 0x7fff;
	WINPORT(GetConsoleScrollRegion)(NULL, &scroll_top, &scroll_bottom);
	
	if (scroll_top < info.srWindow.Top) scroll_top = info.srWindow.Top;
	if (scroll_bottom < info.srWindow.Top) scroll_bottom = info.srWindow.Top;
	
	if (scroll_top > info.srWindow.Bottom) scroll_top = info.srWindow.Bottom;
	
	if (scroll_bottom > info.srWindow.Bottom) scroll_bottom = info.srWindow.Bottom;
		
	if (info.dwCursorPosition.Y != scroll_top) { 
		info.dwCursorPosition.Y--;
		WINPORT(SetConsoleCursorPosition)(hConOut, info.dwCursorPosition);
		return;
	}
	
	if (scroll_top>=scroll_bottom)
		return;

	SMALL_RECT Rect = {info.srWindow.Left, scroll_top, info.srWindow.Right, (SHORT)(scroll_bottom - 1) };
	COORD Pos = {0, (SHORT) (scroll_top + 1) };
	CHAR_INFO  CharInfo;
	CharInfo.Char.UnicodeChar = blank_character;
	CharInfo.Attributes = info.wAttributes;
	WINPORT(ScrollConsoleScreenBuffer)(hConOut, &Rect, NULL, Pos, &CharInfo);
}

static void ResetTerminal()
{
	fprintf(stderr, "ANSI: ResetTerminal\n");
	WINPORT(SetConsoleScrollRegion)(NULL, 0, MAXSHORT);

	nCharInBuffer = 0;
	memset(ChBuffer, 0, sizeof(ChBuffer));
	ansiState = STATE();
	ChPrev = 0;
	fWrapped = 0;

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


static void SaveCursor()
{
	FlushBuffer();
	WINPORT(GetConsoleScreenBufferInfo)( NULL, &save_cursor_info );
}

static void RestoreCursor()
{
	FlushBuffer();
	WINPORT(SetConsoleCursorPosition)(NULL, save_cursor_info.dwCursorPosition);
}

//-----------------------------------------------------------------------------
//   ParseAndPrintString(hDev, lpBuffer, nNumberOfBytesToWrite)
// Parses the string lpBuffer, interprets the escapes sequences and prints the
// characters in the device hDev (console).
// The lexer is a three states automata.
// If the number of arguments es_argc > MAX_ARG, only the MAX_ARG-1 firsts and
// the last arguments are processed (no es_argv[] overflow).
//-----------------------------------------------------------------------------

void ParseAndPrintString( HANDLE hDev,
                          LPCVOID lpBuffer,
                          DWORD nNumberOfBytesToWrite)
{
	DWORD   i;
	LPCWSTR s;

	for (i = nNumberOfBytesToWrite, s = (LPCWSTR)lpBuffer; i > 0; i--, s++) {
		int width = wcwidth(*s);
		if (state == 1) {
			if (*s == ESC) {
				suffix2 = 0;
				//get_state();
				state = (ansiState.crm) ? 7 : 2;
			} else if (*s == SO) charset_shifted = true;
			else if (*s == SI) charset_shifted = false;
			else {
				if ((width > 0) || (*s == 10) || (*s == 13)) {
					PushBuffer( *s );
					if (width > 1) {
						for (int i = 1; i<wcwidth(*s); i++) PushBuffer(' ');
					}
				}
			}
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
			           *s == 'X' ||     // SOS Start Of String
			           *s == '^' ||     // PM  Privacy Message
			           *s == '_') {     // APC Application Program Command
				os_cmd_arg.clear();
				// *Pt_arg = '\0'; Pt_len = 0;
				prefix = *s;
				state = 6;
			} else  {
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
				os_cmd_arg+= *s;

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
			if (*s == 'l') ansiState.crm = FALSE;
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
	assert(i == 0);
}


VTAnsi::VTAnsi(IVTShell *vt_shell)
{
	g_vt_ansi_mutex.lock();	
	g_vt_shell = vt_shell;
	ResetTerminal();
	g_saved_state.InitFromConsole(NULL);
	ansiState.font_state.FromConsoleAttributes(g_saved_state.csbi.wAttributes);
	
	VTLog::Start();
	
//	get_state();
}

VTAnsi::~VTAnsi()
{
	VTLog::Stop();
	g_saved_state.ApplyToConsole(NULL);
	WINPORT(FlushConsoleInputBuffer)(NULL);
	g_vt_shell = nullptr;
	g_vt_ansi_mutex.unlock();
}

void VTAnsi::DisableOutput()
{
	g_output_disabled = true;
}

void VTAnsi::EnableOutput()
{
	if (g_output_disabled) {
		nCharInBuffer = 0;
		g_output_disabled = false;
	}
}

struct VTAnsiState *VTAnsi::Suspend()
{
	VTAnsiState *out = new(std::nothrow) VTAnsiState;
	if (out) {
		out->InitFromConsole(NULL);
		g_saved_state.ApplyToConsole(NULL);
	} else
		perror("VTAnsi::Suspend");

	return out;
}

void VTAnsi::Resume(struct VTAnsiState* state)
{
	state->ApplyToConsole(NULL);
	delete state;
}

void VTAnsi::OnStart()
{
	TCHAR buf[MAX_PATH*2] = {0};
	WINPORT(GetConsoleTitle)( buf, ARRAYSIZE(buf) - 1 );
	_saved_title = buf;
}

void VTAnsi::OnStop()
{
	g_alternative_screen_buffer.Reset();
	g_saved_state.ApplyToConsole(NULL, false);
	ResetTerminal();
	ansiState.font_state.FromConsoleAttributes(g_saved_state.csbi.wAttributes);
	WINPORT(SetConsoleScrollRegion)(NULL, 0, MAXSHORT);
	_incomplete.tail.clear();
	WINPORT(SetConsoleTitle)( _saved_title.c_str());
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

	ParseAndPrintString(NULL, _ws.c_str(), _ws.size());
}

/*
 * This is an implementation of wcwidth() and wcswidth() (defined in
 * IEEE Std 1002.1-2001) for Unicode.
 *
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcwidth.html
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcswidth.html
 *
 * In fixed-width output devices, Latin characters all occupy a single
 * "cell" position of equal width, whereas ideographic CJK characters
 * occupy two such cells. Interoperability between terminal-line
 * applications and (teletype-style) character terminals using the
 * UTF-8 encoding requires agreement on which character should advance
 * the cursor by how many cell positions. No established formal
 * standards exist at present on which Unicode character shall occupy
 * how many cell positions on character terminals. These routines are
 * a first attempt of defining such behavior based on simple rules
 * applied to data provided by the Unicode Consortium.
 *
 * For some graphical characters, the Unicode standard explicitly
 * defines a character-cell width via the definition of the East Asian
 * FullWidth (F), Wide (W), Half-width (H), and Narrow (Na) classes.
 * In all these cases, there is no ambiguity about which width a
 * terminal shall use. For characters in the East Asian Ambiguous (A)
 * class, the width choice depends purely on a preference of backward
 * compatibility with either historic CJK or Western practice.
 * Choosing single-width for these characters is easy to justify as
 * the appropriate long-term solution, as the CJK practice of
 * displaying these characters as double-width comes from historic
 * implementation simplicity (8-bit encoded characters were displayed
 * single-width and 16-bit ones double-width, even for Greek,
 * Cyrillic, etc.) and not any typographic considerations.
 *
 * Much less clear is the choice of width for the Not East Asian
 * (Neutral) class. Existing practice does not dictate a width for any
 * of these characters. It would nevertheless make sense
 * typographically to allocate two character cells to characters such
 * as for instance EM SPACE or VOLUME INTEGRAL, which cannot be
 * represented adequately with a single-width glyph. The following
 * routines at present merely assign a single-cell width to all
 * neutral characters, in the interest of simplicity. This is not
 * entirely satisfactory and should be reconsidered before
 * establishing a formal standard in this area. At the moment, the
 * decision which Not East Asian (Neutral) characters should be
 * represented by double-width glyphs cannot yet be answered by
 * applying a simple rule from the Unicode database content. Setting
 * up a proper standard for the behavior of UTF-8 character terminals
 * will require a careful analysis not only of each Unicode character,
 * but also of each presentation form, something the author of these
 * routines has avoided to do so far.
 *
 * http://www.unicode.org/unicode/reports/tr11/
 *
 * Markus Kuhn -- 2007-05-26 (Unicode 5.0)
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted. The author
 * disclaims all warranties with regard to this software.
 *
 * Latest version: http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
 */

#include <wchar.h>

struct interval {
  int first;
  int last;
};

/* auxiliary function for binary search in interval table */
static int bisearch(wchar_t ucs, const struct interval *table, int max) {
  int min = 0;
  int mid;

  if (ucs < table[0].first || ucs > table[max].last)
    return 0;
  while (max >= min) {
    mid = (min + max) / 2;
    if (ucs > table[mid].last)
      min = mid + 1;
    else if (ucs < table[mid].first)
      max = mid - 1;
    else
      return 1;
  }

  return 0;
}

// from https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c

/* The following two functions define the column width of an ISO 10646
 * character as follows:
 *
 *    - The null character (U+0000) has a column width of 0.
 *
 *    - Other C0/C1 control characters and DEL will lead to a return
 *      value of -1.
 *
 *    - Non-spacing and enclosing combining characters (general
 *      category code Mn or Me in the Unicode database) have a
 *      column width of 0.
 *
 *    - SOFT HYPHEN (U+00AD) has a column width of 1.
 *
 *    - Other format characters (general category code Cf in the Unicode
 *      database) and ZERO WIDTH SPACE (U+200B) have a column width of 0.
 *
 *    - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
 *      have a column width of 0.
 *
 *    - Spacing characters in the East Asian Wide (W) or East Asian
 *      Full-width (F) category as defined in Unicode Technical
 *      Report #11 have a column width of 2.
 *
 *    - All remaining characters (including all printable
 *      ISO 8859-1 and WGL4 characters, Unicode control characters,
 *      etc.) have a column width of 1.
 *
 * This implementation assumes that wchar_t characters are encoded
 * in ISO 10646.
 */

int mk_wcwidth(wchar_t ucs)
{
  /* sorted list of non-overlapping intervals of non-spacing characters */
  /* generated by "uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B c" */
  static const struct interval combining[] = {
    { 0x0300, 0x036F }, { 0x0483, 0x0486 }, { 0x0488, 0x0489 },
    { 0x0591, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
    { 0x05C4, 0x05C5 }, { 0x05C7, 0x05C7 }, { 0x0600, 0x0603 },
    { 0x0610, 0x0615 }, { 0x064B, 0x065E }, { 0x0670, 0x0670 },
    { 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
    { 0x070F, 0x070F }, { 0x0711, 0x0711 }, { 0x0730, 0x074A },
    { 0x07A6, 0x07B0 }, { 0x07EB, 0x07F3 }, { 0x0901, 0x0902 },
    { 0x093C, 0x093C }, { 0x0941, 0x0948 }, { 0x094D, 0x094D },
    { 0x0951, 0x0954 }, { 0x0962, 0x0963 }, { 0x0981, 0x0981 },
    { 0x09BC, 0x09BC }, { 0x09C1, 0x09C4 }, { 0x09CD, 0x09CD },
    { 0x09E2, 0x09E3 }, { 0x0A01, 0x0A02 }, { 0x0A3C, 0x0A3C },
    { 0x0A41, 0x0A42 }, { 0x0A47, 0x0A48 }, { 0x0A4B, 0x0A4D },
    { 0x0A70, 0x0A71 }, { 0x0A81, 0x0A82 }, { 0x0ABC, 0x0ABC },
    { 0x0AC1, 0x0AC5 }, { 0x0AC7, 0x0AC8 }, { 0x0ACD, 0x0ACD },
    { 0x0AE2, 0x0AE3 }, { 0x0B01, 0x0B01 }, { 0x0B3C, 0x0B3C },
    { 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 }, { 0x0B4D, 0x0B4D },
    { 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 }, { 0x0BC0, 0x0BC0 },
    { 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 }, { 0x0C46, 0x0C48 },
    { 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 }, { 0x0CBC, 0x0CBC },
    { 0x0CBF, 0x0CBF }, { 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD },
    { 0x0CE2, 0x0CE3 }, { 0x0D41, 0x0D43 }, { 0x0D4D, 0x0D4D },
    { 0x0DCA, 0x0DCA }, { 0x0DD2, 0x0DD4 }, { 0x0DD6, 0x0DD6 },
    { 0x0E31, 0x0E31 }, { 0x0E34, 0x0E3A }, { 0x0E47, 0x0E4E },
    { 0x0EB1, 0x0EB1 }, { 0x0EB4, 0x0EB9 }, { 0x0EBB, 0x0EBC },
    { 0x0EC8, 0x0ECD }, { 0x0F18, 0x0F19 }, { 0x0F35, 0x0F35 },
    { 0x0F37, 0x0F37 }, { 0x0F39, 0x0F39 }, { 0x0F71, 0x0F7E },
    { 0x0F80, 0x0F84 }, { 0x0F86, 0x0F87 }, { 0x0F90, 0x0F97 },
    { 0x0F99, 0x0FBC }, { 0x0FC6, 0x0FC6 }, { 0x102D, 0x1030 },
    { 0x1032, 0x1032 }, { 0x1036, 0x1037 }, { 0x1039, 0x1039 },
    { 0x1058, 0x1059 }, { 0x1160, 0x11FF }, { 0x135F, 0x135F },
    { 0x1712, 0x1714 }, { 0x1732, 0x1734 }, { 0x1752, 0x1753 },
    { 0x1772, 0x1773 }, { 0x17B4, 0x17B5 }, { 0x17B7, 0x17BD },
    { 0x17C6, 0x17C6 }, { 0x17C9, 0x17D3 }, { 0x17DD, 0x17DD },
    { 0x180B, 0x180D }, { 0x18A9, 0x18A9 }, { 0x1920, 0x1922 },
    { 0x1927, 0x1928 }, { 0x1932, 0x1932 }, { 0x1939, 0x193B },
    { 0x1A17, 0x1A18 }, { 0x1B00, 0x1B03 }, { 0x1B34, 0x1B34 },
    { 0x1B36, 0x1B3A }, { 0x1B3C, 0x1B3C }, { 0x1B42, 0x1B42 },
    { 0x1B6B, 0x1B73 }, { 0x1DC0, 0x1DCA }, { 0x1DFE, 0x1DFF },
    { 0x200B, 0x200F }, { 0x202A, 0x202E }, { 0x2060, 0x2063 },
    { 0x206A, 0x206F }, { 0x20D0, 0x20EF }, { 0x302A, 0x302F },
    { 0x3099, 0x309A }, { 0xA806, 0xA806 }, { 0xA80B, 0xA80B },
    { 0xA825, 0xA826 }, { 0xFB1E, 0xFB1E }, { 0xFE00, 0xFE0F },
    { 0xFE20, 0xFE23 }, { 0xFEFF, 0xFEFF }, { 0xFFF9, 0xFFFB },
    { 0x10A01, 0x10A03 }, { 0x10A05, 0x10A06 }, { 0x10A0C, 0x10A0F },
    { 0x10A38, 0x10A3A }, { 0x10A3F, 0x10A3F }, { 0x1D167, 0x1D169 },
    { 0x1D173, 0x1D182 }, { 0x1D185, 0x1D18B }, { 0x1D1AA, 0x1D1AD },
    { 0x1D242, 0x1D244 }, { 0xE0001, 0xE0001 }, { 0xE0020, 0xE007F },
    { 0xE0100, 0xE01EF }
  };

  /* test for 8-bit control characters */
  if (ucs == 0)
    return 0;
  if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
    return -1;

  /* binary search in table of non-spacing characters */
  if (bisearch(ucs, combining,
	       sizeof(combining) / sizeof(struct interval) - 1))
    return 0;

  /* if we arrive here, ucs is not a combining or C0/C1 control character */

  return 1 + 
    (ucs >= 0x1100 &&
     (ucs <= 0x115f ||                    /* Hangul Jamo init. consonants */
      ucs == 0x2329 || ucs == 0x232a ||
      (ucs >= 0x2e80 && ucs <= 0xa4cf &&
       ucs != 0x303f) ||                  /* CJK ... Yi */
      (ucs >= 0xac00 && ucs <= 0xd7a3) || /* Hangul Syllables */
      (ucs >= 0xf900 && ucs <= 0xfaff) || /* CJK Compatibility Ideographs */
      (ucs >= 0xfe10 && ucs <= 0xfe19) || /* Vertical forms */
      (ucs >= 0xfe30 && ucs <= 0xfe6f) || /* CJK Compatibility Forms */
      (ucs >= 0xff00 && ucs <= 0xff60) || /* Fullwidth Forms */
      (ucs >= 0xffe0 && ucs <= 0xffe6) ||
      (ucs >= 0x20000 && ucs <= 0x2fffd) ||
      (ucs >= 0x30000 && ucs <= 0x3fffd)));
}


int mk_wcswidth(const wchar_t *pwcs, size_t n)
{
  int w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = mk_wcwidth(*pwcs)) < 0)
      return -1;
    else
      width += w;

  return width;
}


/*
 * The following functions are the same as mk_wcwidth() and
 * mk_wcswidth(), except that spacing characters in the East Asian
 * Ambiguous (A) category as defined in Unicode Technical Report #11
 * have a column width of 2. This variant might be useful for users of
 * CJK legacy encodings who want to migrate to UCS without changing
 * the traditional terminal character-width behaviour. It is not
 * otherwise recommended for general use.
 */
int mk_wcwidth_cjk(wchar_t ucs)
{
  /* sorted list of non-overlapping intervals of East Asian Ambiguous
   * characters, generated by "uniset +WIDTH-A -cat=Me -cat=Mn -cat=Cf c" */
  static const struct interval ambiguous[] = {
    { 0x00A1, 0x00A1 }, { 0x00A4, 0x00A4 }, { 0x00A7, 0x00A8 },
    { 0x00AA, 0x00AA }, { 0x00AE, 0x00AE }, { 0x00B0, 0x00B4 },
    { 0x00B6, 0x00BA }, { 0x00BC, 0x00BF }, { 0x00C6, 0x00C6 },
    { 0x00D0, 0x00D0 }, { 0x00D7, 0x00D8 }, { 0x00DE, 0x00E1 },
    { 0x00E6, 0x00E6 }, { 0x00E8, 0x00EA }, { 0x00EC, 0x00ED },
    { 0x00F0, 0x00F0 }, { 0x00F2, 0x00F3 }, { 0x00F7, 0x00FA },
    { 0x00FC, 0x00FC }, { 0x00FE, 0x00FE }, { 0x0101, 0x0101 },
    { 0x0111, 0x0111 }, { 0x0113, 0x0113 }, { 0x011B, 0x011B },
    { 0x0126, 0x0127 }, { 0x012B, 0x012B }, { 0x0131, 0x0133 },
    { 0x0138, 0x0138 }, { 0x013F, 0x0142 }, { 0x0144, 0x0144 },
    { 0x0148, 0x014B }, { 0x014D, 0x014D }, { 0x0152, 0x0153 },
    { 0x0166, 0x0167 }, { 0x016B, 0x016B }, { 0x01CE, 0x01CE },
    { 0x01D0, 0x01D0 }, { 0x01D2, 0x01D2 }, { 0x01D4, 0x01D4 },
    { 0x01D6, 0x01D6 }, { 0x01D8, 0x01D8 }, { 0x01DA, 0x01DA },
    { 0x01DC, 0x01DC }, { 0x0251, 0x0251 }, { 0x0261, 0x0261 },
    { 0x02C4, 0x02C4 }, { 0x02C7, 0x02C7 }, { 0x02C9, 0x02CB },
    { 0x02CD, 0x02CD }, { 0x02D0, 0x02D0 }, { 0x02D8, 0x02DB },
    { 0x02DD, 0x02DD }, { 0x02DF, 0x02DF }, { 0x0391, 0x03A1 },
    { 0x03A3, 0x03A9 }, { 0x03B1, 0x03C1 }, { 0x03C3, 0x03C9 },
    { 0x0401, 0x0401 }, { 0x0410, 0x044F }, { 0x0451, 0x0451 },
    { 0x2010, 0x2010 }, { 0x2013, 0x2016 }, { 0x2018, 0x2019 },
    { 0x201C, 0x201D }, { 0x2020, 0x2022 }, { 0x2024, 0x2027 },
    { 0x2030, 0x2030 }, { 0x2032, 0x2033 }, { 0x2035, 0x2035 },
    { 0x203B, 0x203B }, { 0x203E, 0x203E }, { 0x2074, 0x2074 },
    { 0x207F, 0x207F }, { 0x2081, 0x2084 }, { 0x20AC, 0x20AC },
    { 0x2103, 0x2103 }, { 0x2105, 0x2105 }, { 0x2109, 0x2109 },
    { 0x2113, 0x2113 }, { 0x2116, 0x2116 }, { 0x2121, 0x2122 },
    { 0x2126, 0x2126 }, { 0x212B, 0x212B }, { 0x2153, 0x2154 },
    { 0x215B, 0x215E }, { 0x2160, 0x216B }, { 0x2170, 0x2179 },
    { 0x2190, 0x2199 }, { 0x21B8, 0x21B9 }, { 0x21D2, 0x21D2 },
    { 0x21D4, 0x21D4 }, { 0x21E7, 0x21E7 }, { 0x2200, 0x2200 },
    { 0x2202, 0x2203 }, { 0x2207, 0x2208 }, { 0x220B, 0x220B },
    { 0x220F, 0x220F }, { 0x2211, 0x2211 }, { 0x2215, 0x2215 },
    { 0x221A, 0x221A }, { 0x221D, 0x2220 }, { 0x2223, 0x2223 },
    { 0x2225, 0x2225 }, { 0x2227, 0x222C }, { 0x222E, 0x222E },
    { 0x2234, 0x2237 }, { 0x223C, 0x223D }, { 0x2248, 0x2248 },
    { 0x224C, 0x224C }, { 0x2252, 0x2252 }, { 0x2260, 0x2261 },
    { 0x2264, 0x2267 }, { 0x226A, 0x226B }, { 0x226E, 0x226F },
    { 0x2282, 0x2283 }, { 0x2286, 0x2287 }, { 0x2295, 0x2295 },
    { 0x2299, 0x2299 }, { 0x22A5, 0x22A5 }, { 0x22BF, 0x22BF },
    { 0x2312, 0x2312 }, { 0x2460, 0x24E9 }, { 0x24EB, 0x254B },
    { 0x2550, 0x2573 }, { 0x2580, 0x258F }, { 0x2592, 0x2595 },
    { 0x25A0, 0x25A1 }, { 0x25A3, 0x25A9 }, { 0x25B2, 0x25B3 },
    { 0x25B6, 0x25B7 }, { 0x25BC, 0x25BD }, { 0x25C0, 0x25C1 },
    { 0x25C6, 0x25C8 }, { 0x25CB, 0x25CB }, { 0x25CE, 0x25D1 },
    { 0x25E2, 0x25E5 }, { 0x25EF, 0x25EF }, { 0x2605, 0x2606 },
    { 0x2609, 0x2609 }, { 0x260E, 0x260F }, { 0x2614, 0x2615 },
    { 0x261C, 0x261C }, { 0x261E, 0x261E }, { 0x2640, 0x2640 },
    { 0x2642, 0x2642 }, { 0x2660, 0x2661 }, { 0x2663, 0x2665 },
    { 0x2667, 0x266A }, { 0x266C, 0x266D }, { 0x266F, 0x266F },
    { 0x273D, 0x273D }, { 0x2776, 0x277F }, { 0xE000, 0xF8FF },
    { 0xFFFD, 0xFFFD }, { 0xF0000, 0xFFFFD }, { 0x100000, 0x10FFFD }
  };

  /* binary search in table of non-spacing characters */
  if (bisearch(ucs, ambiguous,
	       sizeof(ambiguous) / sizeof(struct interval) - 1))
    return 2;

  return mk_wcwidth(ucs);
}


int mk_wcswidth_cjk(const wchar_t *pwcs, size_t n)
{
  int w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = mk_wcwidth_cjk(*pwcs)) < 0)
      return -1;
    else
      width += w;

  return width;
}
