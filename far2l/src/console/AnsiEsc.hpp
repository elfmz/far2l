#pragma once
#include <vector>
#include <string>
#include "ViewerPrinter.hpp"
#include <WinCompat.h>

namespace AnsiEsc
{
	BYTE ConsoleColorToAnsi(BYTE clr);

	struct FontState
	{
		FontState();

		BYTE	foreground = 0;	    // ANSI base color (0 to 7; add 30)
		BYTE	background = 0;	    // ANSI base color (0 to 7; add 40)
		bool	bold = false;		//
		bool	underline = false;	//
		bool	strikeout = false;  //
		bool	rvideo = false; 	// swap console foreground & background attributes
		bool	concealed = false;	// set foreground/bold to background/underline

		bool	use_rgb_foreground = false;
		bool	use_rgb_background = false;

		DWORD	rgb_foreground = 0;
		DWORD	rgb_background = 0;

		void ParseSuffixM(const int *args, int argc);
		void FromConsoleAttributes(DWORD64 qAttributes);
		DWORD64 ToConsoleAttributes();
	};

	struct Parser
	{
		std::vector<int> args;
		wchar_t suffix = 0;

		const wchar_t *Parse(const wchar_t *str);
	};

	struct Printer : ViewerPrinter
	{
		Printer(WORD wAttributes);
		virtual ~Printer();

		virtual int Length(const wchar_t *str, int limit = -1);
		virtual void Print(int skip_len, int print_len, const wchar_t *str);

	private:
		Parser _parser;
		FontState _font_state;
		DWORD64 _initial_attr;
		wchar_t _last_char = L' ';

		void EnforceStateColor();
	};
}
