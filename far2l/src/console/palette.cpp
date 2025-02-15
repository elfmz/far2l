/*
palette.cpp

Таблица цветов
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "farcolors.hpp"

class Palette Palette::FARPalette;


Palette::Palette() noexcept
{
}

Palette::~Palette() noexcept
{
}

void Palette::ResetToDefault( ) noexcept
{
	fprintf(stderr, "Palette::ResetToDefault( )\n" );

	for ( unsigned int i = 0; i < 16; ++i) {

		switch (i) {
			case FOREGROUND_INTENSITY: {
				foreground[i].r =
					foreground[i].g =
						foreground[i].b = 0x80;
			} break;

			case (FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED): {
				foreground[i].r =
					foreground[i].g =
						foreground[i].b = 0xc0;
			} break;

			// tweaked blue to make it more readable on dark background
			case FOREGROUND_BLUE: {
				foreground[i].r = 0;
				foreground[i].g = 0x28;
				foreground[i].b = 0xa0;
			} break;

			case (FOREGROUND_BLUE|FOREGROUND_INTENSITY): {
				foreground[i].r = 0;
				foreground[i].g = 0x55;
				foreground[i].b = 0xff;
			} break;

			default: {
				const unsigned char lvl = (i & FOREGROUND_INTENSITY) ? 0xff : 0xa0;
				foreground[i].r = (i & FOREGROUND_RED) ? lvl : 0;
				foreground[i].g = (i & FOREGROUND_GREEN) ? lvl : 0;
				foreground[i].b = (i & FOREGROUND_BLUE) ? lvl : 0;
			}
		}

		switch (i) {
			case (BACKGROUND_INTENSITY >> 4): {
				background[i].r =
					background[i].g =
						background[i].b = 0x80;
			} break;

			case (BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_RED) >> 4: {
				background[i].r =
					background[i].g =
						background[i].b = 0xc0;
			} break;

			default: {
				const unsigned char lvl = (i & (BACKGROUND_INTENSITY>>4)) ? 0xff : 0x80;
				background[i].r = (i & (BACKGROUND_RED>>4)) ? lvl : 0;
				background[i].g = (i & (BACKGROUND_GREEN>>4)) ? lvl : 0;
				background[i].b = (i & (BACKGROUND_BLUE>>4)) ? lvl : 0;
			}
		}
	}
}

#define PALETTE_CONFIG "palette.ini"

void Palette::InitFarPalette( ) noexcept
{
	fprintf(stderr, "Palette::InitFarPalette( )\n" );

	const std::string &palette_file = InMyConfig(PALETTE_CONFIG);
	KeyFileHelper kfh(palette_file);
	if (!kfh.IsLoaded()) {
		FARPalette.ResetToDefault();
		FARPalette.Save(kfh);
	} else if (!FARPalette.Load(kfh)) {
		fprintf(stderr, "InitPalettes: failed to parse '%s'\n", palette_file.c_str());
		FARPalette.ResetToDefault();
	}

	return;
}

static bool LoadPaletteEntry(KeyFileHelper &kfh, const char *section, const char *name, rgbcolor_t &result)
{
	const std::string &str = kfh.GetString(section, name, "#000000");
	if (str.empty() || str[0] != '#') {
		fprintf(stderr, "LoadPaletteEntry: bad '%s' at [%s]\n", name, section);
		return false;
	}

	unsigned int value = 0;
	sscanf(str.c_str(), "#%x", &value);
	result.r = (value & 0xff0000) >> 16;
	result.g = (value & 0x00ff00) >> 8;
	result.b = (value & 0x0000ff);
	return true;
}

bool Palette::Load(KeyFileHelper &kfh) noexcept
{
	fprintf(stderr, "Palette::Load( )\n" );
	char name[16];
	bool out = true;
	for (unsigned int i = 0; i < 16; ++i) {
		snprintf(name, sizeof(name), "%d", i);

		if (!LoadPaletteEntry(kfh, "foreground", name, foreground[i])
		|| !LoadPaletteEntry(kfh, "background", name, background[i])) {
			out = false;
		}
	}
	return out;
}

bool Palette::Save(KeyFileHelper &kfh) noexcept
{
	fprintf(stderr, "Palette::Save( )\n" );
	char name[16], value[16];
	for (int i = 0; i < 16; ++i) {
		snprintf(name, sizeof(name), "%d", i);

		snprintf(value, sizeof(value), "#%02X%02X%02X", foreground[i].r, foreground[i].g, foreground[i].b);
		kfh.SetString("foreground", name, value);

		snprintf(value, sizeof(value), "#%02X%02X%02X", background[i].r, background[i].g, background[i].b);
		kfh.SetString("background", name, value);
	}
	return true;
}

/*
  1.65            - 0x52
  1.70 b1 (272)   - 0x54
  1.70 b2 (321)   - 0x54
  1.70 b3 (591)   - 0x54
  1.70 b4 (1282)  - 0x60
  1.70 b5 ()      - 0x70

  1.71 a4 (2468)  - 0x81
  1.80    (606)   - 0x81
  1.75 rc1 (2555) - 0x8B
  2.0 (848)       - 0x8B
*/

void ConvertCurrentPalette()
{
	//  DWORD Size=GetRegKeySize("Colors","CurrentPalette");
}
