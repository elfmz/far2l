#pragma once

#define NODETECT_NONE   0x0000
#define NODETECT_XI     0x0001 // Xi
#define NODETECT_X      0x0002 // X11
#define NODETECT_F      0x0004 // FAR2L
#define NODETECT_W      0x0008 // Windows
#define NODETECT_A      0x0010 // Apple
#define NODETECT_K      0x0020 // Kitty
#define NODETECT_E      0x0040 // Emodji characters (Unicode Variation Selector-16)

struct TTYCaps
{
	uint16_t nodetect;      // command-line option(s)

	enum Kind
	{
		GENERIC = 0,
		KERNEL,
		FAR2L
	} kind : 4;             // detected

	bool DEC_lines : 1;     // detected, need to use 'DEC Line Drawing mode' to draw pseudographics lines
	bool strict_dups : 1;   // detected, terminal doesnt support any of "\e[..b" "\e[..X" that used to optimally output long sequences of same characters
	bool strict_pos : 1;    // detected, terminal doesnt support "\e[#H" or "\e[H", but only "\e[#;#H"

	bool norgb : 1;         // command-line option
	bool ext_clipboard : 1; // command-line option
};
