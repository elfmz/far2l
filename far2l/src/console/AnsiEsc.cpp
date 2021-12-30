#include "headers.hpp"
#include "AnsiEsc.hpp"
#include "interf.hpp"
#include "colors.hpp"

namespace AnsiEsc
{

#define FOREGROUND_BLACK 0
#define FOREGROUND_WHITE FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE

#define BACKGROUND_BLACK 0
#define BACKGROUND_WHITE BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE

static const BYTE ForegroundColor[16] = {
	FOREGROUND_BLACK,			// black foreground
	FOREGROUND_RED,			// red foreground
	FOREGROUND_GREEN,			// green foreground
	FOREGROUND_RED | FOREGROUND_GREEN,	// yellow foreground
	FOREGROUND_BLUE,			// blue foreground
	FOREGROUND_BLUE | FOREGROUND_RED,	// magenta foreground
	FOREGROUND_BLUE | FOREGROUND_GREEN,	// cyan foreground
	FOREGROUND_WHITE,			// white foreground

	FOREGROUND_INTENSITY | FOREGROUND_BLACK,			// black foreground
	FOREGROUND_INTENSITY | FOREGROUND_RED,			// red foreground
	FOREGROUND_INTENSITY | FOREGROUND_GREEN,			// green foreground
	FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,	// yellow foreground
	FOREGROUND_INTENSITY | FOREGROUND_BLUE,			// blue foreground
	FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED,	// magenta foreground
	FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN,	// cyan foreground
	FOREGROUND_INTENSITY | FOREGROUND_WHITE,			// white foreground

};

static const BYTE BackgroundColor[16] = {
	BACKGROUND_BLACK,			// black background
	BACKGROUND_RED,			// red background
	BACKGROUND_GREEN,			// green background
	BACKGROUND_RED | BACKGROUND_GREEN,	// yellow background
	BACKGROUND_BLUE,			// blue background
	BACKGROUND_BLUE | BACKGROUND_RED,	// magenta background
	BACKGROUND_BLUE | BACKGROUND_GREEN,	// cyan background
	BACKGROUND_WHITE,			// white background

	BACKGROUND_INTENSITY | BACKGROUND_BLACK,			// black background
	BACKGROUND_INTENSITY | BACKGROUND_RED,			// red background
	BACKGROUND_INTENSITY | BACKGROUND_GREEN,			// green background
	BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,	// yellow background
	BACKGROUND_INTENSITY | BACKGROUND_BLUE,			// blue background
	BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_RED,	// magenta background
	BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN,	// cyan background
	BACKGROUND_INTENSITY | BACKGROUND_WHITE,			// white background
};


static const BYTE Attr2Ansi[8] = {	// map console attribute to ANSI number
	0,					// black
	4,					// blue
	2,					// green
	6,					// cyan
	1,					// red
	5,					// magenta
	3,					// yellow
	7					// white
};

/////////////////
void FontState::ParseSuffixM(const int *args, int argc)
{
	int argz = 0;
	if (argc == 0) {
		argc = 1;
		args = &argz;
	}

	for (int i = 0; i < argc; ++i) {
		const int a = args[i];
		if (30 <= a && a <= 37) {
			foreground = a - 30;

		} else if (90 <= a && a <= 97) {
			foreground = (a - 90) + 8;

		} else if (40 <= a && a <= 47) {
			background = a - 40;

		} else if (100 <= a && a <= 107) {
			background = (a - 100) + 8;

		} else if (a == 38 || a == 48) {
			// This is technically incorrect, but it's what xterm does, so
			// that's what we do.  According to T.416 (ISO 8613-6), there is
			// only one parameter, which is divided into elements.  So where
			// xterm does "38;2;R;G;B" it should really be "38;2:I:R:G:B" (I is
			// a colour space identifier).
			if (i + 1 < argc) {
				if (args[i + 1] == 2)		// rgb
					i += 4;
				else if (args[i + 1] == 5)	// index
					i += 2;
			}

		} else switch (a) {
		case 0:
			rvideo    = false;
			concealed = false;
			bold = false;
			underline = false;

		case 39:
		case 49: {
			const BYTE def_attr = 7;//FOREGROUND_WHITE;
			reverse = false;
			if (def_attr != 49) {
				foreground = Attr2Ansi[def_attr & 15];
			}
			if (def_attr != 39) {
				background = Attr2Ansi[(def_attr >> 4) & 15];
			}
		} break;

		case  1:
			bold      = true;
			break;
		case  5: // blink
		case  4:
			underline = true;
				break;
		case  7:
			rvideo    = 1;
			break;
		case  8:
			concealed = 1;
			break;
		case 21: // oops, this actually turns on double underline
			// but xterm turns off bold too, so that's alright
		case 22:
			bold      = 0;
			break;
		case 25:
		case 24:
			underline = 0;
			break;
		case 27:
			rvideo    = 0;
			break;
		case 28:
			concealed = 0;
			break;
		}
	}
}

void FontState::FromConsoleAttributes(WORD wAttributes)
{
	bold = (wAttributes & FOREGROUND_INTENSITY) != 0;
	underline = (wAttributes & BACKGROUND_INTENSITY) != 0;
	foreground = Attr2Ansi[wAttributes & 7];
	background = Attr2Ansi[(wAttributes >> 4) & 7];
}

WORD FontState::ToConsoleAttributes()
{
	WORD attribut = 0;

	if (concealed) {
		if (rvideo) {
			attribut = ForegroundColor[foreground] | BackgroundColor[foreground];
			if (bold) {
				attribut|= FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
			}
		} else {
			attribut = ForegroundColor[background] | BackgroundColor[background];
			if (underline) {
				attribut|= FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
			}
		}
	} else if (rvideo) {
		attribut = ForegroundColor[background] | BackgroundColor[foreground];
		if (bold) {
			attribut|= BACKGROUND_INTENSITY;
		}
		if (underline) {
			attribut|= FOREGROUND_INTENSITY;
		}
	} else {
		attribut = ForegroundColor[foreground] | BackgroundColor[background];
		if (bold) {
			attribut|= FOREGROUND_INTENSITY;
		}
		if (underline) {
			attribut|= BACKGROUND_INTENSITY;
		}
	}
	if (reverse) {
		attribut = ((attribut >> 4) & 15) | ((attribut & 15) << 4);
	}
	return attribut;
}

//////////////////////////////////////////////////////////////////////////////////////////

const wchar_t *Parser::Parse(const wchar_t *str)
{
	args.clear();
	suffix = 0;

	if (str[0] == 033 && str[1] == '[') {
		int a = 0;
		for (size_t i = 2; str[i]; ++i) {
			if (str[i] == ';'
					|| (str[i] >= L'a' && str[i] <= L'z')
					|| (str[i] >= L'A' && str[i] <= L'Z')) {
				args.push_back(a);
				if (str[i] != ';') {
					suffix = str[i];
					return &str[i + 1];
				}
				a = 0;

			} else if (i < 32 && str[i] >= '0' && str[i] <= '9') {
				a*= 10;
				a+= str[i] - '0';

			} else {
				return nullptr;
			}
		}
	}

	return nullptr;
}

/////////////

Printer::Printer(WORD wAttributes)
	:
	_initial_attr(GetRealColor())
{
	_font_state.FromConsoleAttributes(wAttributes);
}

Printer::~Printer()
{
	SetRealColor(_initial_attr);
}

int Printer::Length(const wchar_t *str, int limit)
{
	Parser tmp_parser;
	size_t out = 0;
	for (const wchar_t *ch = str; *ch && limit != 0;) {
		const wchar_t *end_of_esc = tmp_parser.Parse(ch);
		if (end_of_esc) {
			if (end_of_esc - ch > limit) {
				return out;
			}
			limit-= end_of_esc - ch;
			ch = end_of_esc;
			if (tmp_parser.suffix == L'C') { // skip N chars
				out+= tmp_parser.args[0] ? tmp_parser.args[0] : 1;
			}
			else if (tmp_parser.suffix == L'b') { // repeate last char N times
				out+= tmp_parser.args[0] ? tmp_parser.args[0] : 1;
			}
		} else {
			if (!ShouldSkip(*ch)) {
				++out;
			}
			++ch;
			--limit;
		}
	}
	return out;
}

void Printer::EnforceStateColor()
{
	if (_selection)
		SetColor(COL_VIEWERSELECTEDTEXT);
	else
		SetRealColor(_font_state.ToConsoleAttributes());
}

void Printer::Print(int skip_len, int print_len, const wchar_t *str)
{
	int processed = 0;
	EnforceStateColor();
	for (const wchar_t *ch = str; ;) {
		const wchar_t *end_of_chunk = (*ch && !ShouldSkip(*ch)) ? _parser.Parse(ch) : ch;
		if (end_of_chunk)
		{
			if (skip_len >= processed) {
				int skip_part = std::min(skip_len - processed, int(ch - str));
				processed+= skip_part;
				str+= skip_part;
			}
			if (ch != str && processed < skip_len + print_len) {
				int print_part = int(ch - str);
				if (processed + print_part > skip_len + print_len) {
					print_part = skip_len + print_len - processed;
				}
				Text(str, print_part);
			}
			processed+= ch - str;
			if (!*ch) {
				break;
			}
			if (ShouldSkip(*ch)) {
				str = ++ch;
				continue;
			}
			if (_parser.suffix == L'C') {
				for (int i = 0; i < _parser.args[0] || i < 1; ++i, ++processed ) {
				    if (processed >= skip_len && processed < skip_len + print_len) {
						Text(L" ", 1);
					}
				}
			} else if (_parser.suffix == L'b') {
				for (int i = 0; i < _parser.args[0] || i < 1; ++i, ++processed ) {
				    if (processed >= skip_len && processed < skip_len + print_len) {
						Text(&_last_char, 1);
					}
				}
			} else if (_parser.suffix == L'm') {
				_font_state.ParseSuffixM(_parser.args.data(), (int)_parser.args.size());
				EnforceStateColor();
			}
			str = ch = end_of_chunk;

		} else {
			_last_char = *ch;
			++ch;
		}
	}
    while (processed < skip_len) {
		++processed;
	}
	if (processed < skip_len + print_len) {
		PrintSpaces(skip_len + print_len - processed);
	}
}

}
