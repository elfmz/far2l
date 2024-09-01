#include "WinPortRGB.h"
#include <utils.h>
#include <KeyFileHelper.h>

/*
    Foreground/Background color palettes are 16 (r,g,b) values.
    More words here from Miotio...
*/
WinPortPalette g_winport_palette __attribute__ ((visibility("default")));

#define PALETTE_CONFIG "palette.ini"

static void InitDefaultPalette()
{
	for ( unsigned int i = 0; i < BASE_PALETTE_SIZE; ++i) {

		switch (i) {
			case FOREGROUND_INTENSITY: {
				g_winport_palette.foreground[i].r =
					g_winport_palette.foreground[i].g =
						g_winport_palette.foreground[i].b = 0x80;
			} break;

			case (FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED): {
				g_winport_palette.foreground[i].r =
					g_winport_palette.foreground[i].g =
						g_winport_palette.foreground[i].b = 0xc0;
			} break;

			// tweaked blue to make it more readable on dark background
			case FOREGROUND_BLUE: {
				g_winport_palette.foreground[i].r = 0;
				g_winport_palette.foreground[i].g = 0x28;
				g_winport_palette.foreground[i].b = 0xa0;
			} break;

			case (FOREGROUND_BLUE|FOREGROUND_INTENSITY): {
				g_winport_palette.foreground[i].r = 0;
				g_winport_palette.foreground[i].g = 0x55;
				g_winport_palette.foreground[i].b = 0xff;
			} break;

			default: {
				const unsigned char lvl = (i & FOREGROUND_INTENSITY) ? 0xff : 0xa0;
				g_winport_palette.foreground[i].r = (i & FOREGROUND_RED) ? lvl : 0;
				g_winport_palette.foreground[i].g = (i & FOREGROUND_GREEN) ? lvl : 0;
				g_winport_palette.foreground[i].b = (i & FOREGROUND_BLUE) ? lvl : 0;
			}
		}

		switch (i) {
			case (BACKGROUND_INTENSITY >> 4): {
				g_winport_palette.background[i].r =
					g_winport_palette.background[i].g =
						g_winport_palette.background[i].b = 0x80;
			} break;

			case (BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_RED) >> 4: {
				g_winport_palette.background[i].r =
					g_winport_palette.background[i].g =
						g_winport_palette.background[i].b = 0xc0;
			} break;

			default: {
				const unsigned char lvl = (i & (BACKGROUND_INTENSITY>>4)) ? 0xff : 0x80;
				g_winport_palette.background[i].r = (i & (BACKGROUND_RED>>4)) ? lvl : 0;
				g_winport_palette.background[i].g = (i & (BACKGROUND_GREEN>>4)) ? lvl : 0;
				g_winport_palette.background[i].b = (i & (BACKGROUND_BLUE>>4)) ? lvl : 0;
			}
		}
	}
}

static bool LoadPaletteEntry(KeyFileHelper &kfh, const char *section, const char *name, WinPortRGB &result)
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

static bool LoadPalette(KeyFileHelper &kfh)
{
	char name[16];
	bool out = true;
	for (unsigned int i = 0; i < BASE_PALETTE_SIZE; ++i) {
		snprintf(name, sizeof(name), "%d", i);

		if (!LoadPaletteEntry(kfh, "foreground", name, g_winport_palette.foreground[i])
		|| !LoadPaletteEntry(kfh, "background", name, g_winport_palette.background[i])) {
			out = false;
		}
	}
	return out;
}

static void SavePalette(KeyFileHelper &kfh)
{
	char name[16], value[16];
	for (int i = 0; i < BASE_PALETTE_SIZE; ++i) {
		snprintf(name, sizeof(name), "%d", i);

		snprintf(value, sizeof(value), "#%02X%02X%02X",
			g_winport_palette.foreground[i].r, g_winport_palette.foreground[i].g, g_winport_palette.foreground[i].b);
		kfh.SetString("foreground", name, value);

		snprintf(value, sizeof(value), "#%02X%02X%02X",
			g_winport_palette.background[i].r, g_winport_palette.background[i].g, g_winport_palette.background[i].b);
		kfh.SetString("background", name, value);
	}
}

void InitPalette()
{
	const std::string &palette_file = InMyConfig(PALETTE_CONFIG);
	KeyFileHelper kfh(palette_file);
	if (!kfh.IsLoaded()) {
		InitDefaultPalette();
		SavePalette(kfh);

	} else if (!LoadPalette(kfh)) {
		fprintf(stderr, "InitPalettes: failed to parse '%s'\n", palette_file.c_str());
		InitDefaultPalette();
	}
}
