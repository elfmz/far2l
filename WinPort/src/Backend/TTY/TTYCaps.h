#pragma once

struct TTYRestrict
{
	bool x11   : 1;
	bool xi    : 1;
	bool far2l : 1;
	bool apple : 1;
	bool kitty : 1;
	bool win32 : 1;
	bool emoji : 1;
	bool rgb   : 1;
};

struct TTYCaps
{
	void Setup(int fdin, int fdout, const TTYRestrict &restrict);
	void Finup(int fdin, int fdout);

	enum Kind
	{
		GENERIC = 0,
		KERNEL,
		FAR2L
	} kind : 4;             // set by Setup()

	bool DEC_lines : 1;     // set by Setup() if need to use 'DEC Line Drawing mode' to draw pseudographics lines
	bool strict_dups : 1;   // set by Setup() if terminal doesnt support any of "\e[..b" "\e[..X" that used to optimally output long sequences of same characters
	bool strict_pos : 1;    // set by Setup() if terminal doesnt support "\e[#H" or "\e[H", but only "\e[#;#H"
	bool emoji_vs16 : 1;    // set by Setup() if terminal supports wide emoji characters (Unicode Variation Selector-16)
	bool norgb : 1;         // set by Setup() if restrict.rgb == true or if terminal doesnt support RGB graphics (e.g. screen)
	bool x11 : 1;           // set by Setup()
	bool wayland : 1;       // set by Setup()
};

unsigned int TTYKernelQueryControlKeys(int stdin);
bool TTYWriteAndDrain(int fd, const std::string &str);
