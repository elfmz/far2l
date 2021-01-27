#pragma once
#include <vector>
#include <string>

namespace AnsiEsc
{
	struct FontState
	{
		BYTE	foreground = false;	// ANSI base color (0 to 7; add 30)
		BYTE	background = false;	// ANSI base color (0 to 7; add 40)
		bool	bold = false;		//
		bool	underline = false;	//
		bool	rvideo = false; 	// swap foreground/bold & background/underline
		bool	concealed = false;	// set foreground/bold to background/underline
		bool	reverse = false;	// swap console foreground & background attributes

		void ParseSuffixM(const int *args, int argc);
		void FromConsoleAttributes(WORD wAttributes);
		WORD ToConsoleAttributes();
	};

	struct Parser
	{
		std::vector<int> args;
		wchar_t suffix = 0;

		const wchar_t *Parse(const wchar_t *str);
	};

	struct Printer : protected Parser
	{
		Printer(WORD wAttributes);
		~Printer();

		int Length(const wchar_t *str);
		void Print(int skip_len, int print_len, const wchar_t *str);

	private:
		FontState _font_state;
		WORD _initial_attr;
	};
}
