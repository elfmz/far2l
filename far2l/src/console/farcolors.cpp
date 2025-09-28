/*
farcolors.cpp
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

#include "colors.hpp"
#include "headers.hpp"
//#include "palette.hpp"
//#include "color.hpp"
#include "farcolors.hpp"
#include "config.hpp"
#include "ConfigRW.hpp"
#include "farcolorexp.h"

static struct ColorsInit_s
{
//	const char *name;
	const std::string name;
	unsigned char index;

	static constexpr unsigned char Default = 0x0F;
}
ColorsInit[]
{
	{"Menu.Text",                                   F_WHITE | B_CYAN,         }, // COL_MENUTEXT,
	{"Menu.Text.Selected",                          F_WHITE | B_BLACK,        }, // COL_MENUSELECTEDTEXT,
	{"Menu.Highlight",                              F_YELLOW | B_CYAN,        }, // COL_MENUHIGHLIGHT,
	{"Menu.Highlight.Selected",                     F_YELLOW | B_BLACK,       }, // COL_MENUSELECTEDHIGHLIGHT,
	{"Menu.Box",                                    F_WHITE | B_CYAN,         }, // COL_MENUBOX,
	{"Menu.Title",                                  F_WHITE | B_CYAN,         }, // COL_MENUTITLE,
	{"HMenu.Text",                                  F_BLACK | B_CYAN,         }, // COL_HMENUTEXT,
	{"HMenu.Text.Selected",                         F_WHITE | B_BLACK,        }, // COL_HMENUSELECTEDTEXT,
	{"HMenu.Highlight",                             F_YELLOW | B_CYAN,        }, // COL_HMENUHIGHLIGHT,
	{"HMenu.Highlight.Selected",                    F_YELLOW | B_BLACK,       }, // COL_HMENUSELECTEDHIGHLIGHT,
	{"Panel.Text",                                  F_LIGHTCYAN | B_BLUE,     }, // COL_PANELTEXT,
	{"Panel.Text.Selected",                         F_YELLOW | B_BLUE,        }, // COL_PANELSELECTEDTEXT,
	{"Panel.Text.Highlight",                        F_LIGHTGRAY | B_BLUE,     }, // COL_PANELHIGHLIGHTTEXT,
	{"Panel.Text.Info",                             F_YELLOW | B_BLUE,        }, // COL_PANELINFOTEXT,
	{"Panel.Cursor",                                F_BLACK | B_CYAN,         }, // COL_PANELCURSOR,
	{"Panel.Cursor.Selected",                       F_YELLOW | B_CYAN,        }, // COL_PANELSELECTEDCURSOR,
	{"Panel.Title",                                 F_LIGHTCYAN | B_BLUE,     }, // COL_PANELTITLE,
	{"Panel.Title.Selected",                        F_BLACK | B_CYAN,         }, // COL_PANELSELECTEDTITLE,
	{"Panel.Title.Column",                          F_YELLOW | B_BLUE,        }, // COL_PANELCOLUMNTITLE,
	{"Panel.Info.Total",                            F_LIGHTCYAN | B_BLUE,     }, // COL_PANELTOTALINFO,
	{"Panel.Info.Selected",                         F_YELLOW | B_CYAN,        }, // COL_PANELSELECTEDINFO,
	{"Dialog.Text",                                 F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGTEXT,
	{"Dialog.Text.Highlight",                       F_YELLOW | B_LIGHTGRAY,   }, // COL_DIALOGHIGHLIGHTTEXT,
	{"Dialog.Box",                                  F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGBOX,
	{"Dialog.Box.Title",                            F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGBOXTITLE,
	{"Dialog.Box.Title.Highlight",                  F_YELLOW | B_LIGHTGRAY,   }, // COL_DIALOGHIGHLIGHTBOXTITLE,
	{"Dialog.Edit",                                 F_BLACK | B_CYAN,         }, // COL_DIALOGEDIT,
	{"Dialog.Button",                               F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGBUTTON,
	{"Dialog.Button.Selected",                      F_BLACK | B_CYAN,         }, // COL_DIALOGSELECTEDBUTTON,
	{"Dialog.Button.Highlight",                     F_YELLOW | B_LIGHTGRAY,   }, // COL_DIALOGHIGHLIGHTBUTTON,
	{"Dialog.Button.Highlight.Selected",            F_YELLOW | B_CYAN,        }, // COL_DIALOGHIGHLIGHTSELECTEDBUTTON,
	{"Dialog.List.Text",                            F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGLISTTEXT,
	{"Dialog.List.Text.Selected",                   F_WHITE | B_BLACK,        }, // COL_DIALOGLISTSELECTEDTEXT,
	{"Dialog.List.Highlight",                       F_YELLOW | B_LIGHTGRAY,   }, // COL_DIALOGLISTHIGHLIGHT,
	{"Dialog.List.Highlight.Selected",              F_YELLOW | B_BLACK,       }, // COL_DIALOGLISTSELECTEDHIGHLIGHT,
	{"WarnDialog.Text",                             F_WHITE | B_RED,          }, // COL_WARNDIALOGTEXT,
	{"WarnDialog.Text.Highlight",                   F_YELLOW | B_RED,         }, // COL_WARNDIALOGHIGHLIGHTTEXT,
	{"WarnDialog.Box",                              F_WHITE | B_RED,          }, // COL_WARNDIALOGBOX,
	{"WarnDialog.Box.Title",                        F_WHITE | B_RED,          }, // COL_WARNDIALOGBOXTITLE,
	{"WarnDialog.Box.Title.Highlight",              F_YELLOW | B_RED,         }, // COL_WARNDIALOGHIGHLIGHTBOXTITLE,
	{"WarnDialog.Edit",                             F_BLACK | B_CYAN,         }, // COL_WARNDIALOGEDIT,
	{"WarnDialog.Button",                           F_WHITE | B_RED,          }, // COL_WARNDIALOGBUTTON,
	{"WarnDialog.Button.Selected",                  F_BLACK | B_LIGHTGRAY,    }, // COL_WARNDIALOGSELECTEDBUTTON,
	{"WarnDialog.Button.Highlight",                 F_YELLOW | B_RED,         }, // COL_WARNDIALOGHIGHLIGHTBUTTON,
	{"WarnDialog.Button.Highlight.Selected",        F_YELLOW | B_LIGHTGRAY,   }, // COL_WARNDIALOGHIGHLIGHTSELECTEDBUTTON,
	{"Keybar.Num",                                  F_LIGHTGRAY | B_BLACK,    }, // COL_KEYBARNUM,
	{"Keybar.Text",                                 F_BLACK | B_CYAN,         }, // COL_KEYBARTEXT,
	{"Keybar.Background",                           F_LIGHTGRAY | B_BLACK,    }, // COL_KEYBARBACKGROUND,
	{"CommandLine",                                 ColorsInit_s::Default,    }, // COL_COMMANDLINE,
	{"Clock",                                       F_BLACK | B_CYAN,         }, // COL_CLOCK,
	{"Viewer.Text",                                 F_LIGHTCYAN | B_BLUE,     }, // COL_VIEWERTEXT,
	{"Viewer.Text.Selected",                        F_BLACK | B_CYAN,         }, // COL_VIEWERSELECTEDTEXT,
	{"Viewer.Status",                               F_BLACK | B_CYAN,         }, // COL_VIEWERSTATUS,
	{"Editor.Text",                                 F_LIGHTCYAN | B_BLUE,     }, // COL_EDITORTEXT,
	{"Editor.Text.Selected",                        F_BLACK | B_CYAN,         }, // COL_EDITORSELECTEDTEXT,
	{"Editor.Status",                               F_BLACK | B_CYAN,         }, // COL_EDITORSTATUS,
	{"Help.Text",                                   F_BLACK | B_CYAN,         }, // COL_HELPTEXT,
	{"Help.Text.Highlight",                         F_WHITE | B_CYAN,         }, // COL_HELPHIGHLIGHTTEXT,
	{"Help.Topic",                                  F_YELLOW | B_CYAN,        }, // COL_HELPTOPIC,
	{"Help.Topic.Selected",                         F_WHITE | B_BLACK,        }, // COL_HELPSELECTEDTOPIC,
	{"Help.Box",                                    F_BLACK | B_CYAN,         }, // COL_HELPBOX,
	{"Help.Box.Title",                              F_BLACK | B_CYAN,         }, // COL_HELPBOXTITLE,
	{"Panel.DragText",                              F_YELLOW | B_CYAN,        }, // COL_PANELDRAGTEXT,
	{"Dialog.Edit.Unchanged",                       F_LIGHTGRAY | B_CYAN,     }, // COL_DIALOGEDITUNCHANGED,
	{"Panel.Scrollbar",                             F_LIGHTCYAN | B_BLUE,     }, // COL_PANELSCROLLBAR,
	{"Help.Scrollbar",                              F_BLACK | B_CYAN,         }, // COL_HELPSCROLLBAR,
	{"Panel.Box",                                   F_LIGHTCYAN | B_BLUE,     }, // COL_PANELBOX,
	{"Panel.ScreensNumber",                         F_LIGHTCYAN | B_BLACK,    }, // COL_PANELSCREENSNUMBER,
	{"Dialog.Edit.Selected",                        F_WHITE | B_BLACK,        }, // COL_DIALOGEDITSELECTED,
	{"CommandLine.Selected",                        F_BLACK | B_CYAN,         }, // COL_COMMANDLINESELECTED,
	{"Viewer.Arrows",                               F_YELLOW | B_BLUE,        }, // COL_VIEWERARROWS,
	{"Not.Used",                                    F_WHITE | B_BLACK,        }, // COL_RESERVED0 *************************************************,
	{"Dialog.List.Scrollbar",                       F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGLISTSCROLLBAR,
	{"Menu.Scrollbar",                              F_WHITE | B_CYAN,         }, // COL_MENUSCROLLBAR,
	{"Viewer.Scrollbar",                            F_LIGHTCYAN | B_BLUE,     }, // COL_VIEWERSCROLLBAR,
	{"CommandLine.Prefix",                          ColorsInit_s::Default,    }, // COL_COMMANDLINEPREFIX,
	{"Dialog.Disabled",                             F_DARKGRAY | B_LIGHTGRAY, }, // COL_DIALOGDISABLED,
	{"Dialog.Edit.Disabled",                        F_DARKGRAY | B_CYAN,      }, // COL_DIALOGEDITDISABLED,
	{"Dialog.List.Disabled",                        F_DARKGRAY | B_LIGHTGRAY, }, // COL_DIALOGLISTDISABLED,
	{"WarnDialog.Disabled",                         F_DARKGRAY | B_RED,       }, // COL_WARNDIALOGDISABLED,
	{"WarnDialog.Edit.Disabled",                    F_DARKGRAY | B_CYAN,      }, // COL_WARNDIALOGEDITDISABLED,
	{"WarnDialog.List.Disabled",                    F_DARKGRAY | B_RED,       }, // COL_WARNDIALOGLISTDISABLED,
	{"Menu.Text.Disabled",                          F_DARKGRAY | B_CYAN,      }, // COL_MENUDISABLEDTEXT,
	{"Editor.Clock",                                F_BLACK | B_CYAN,         }, // COL_EDITORCLOCK,
	{"Viewer.Clock",                                F_BLACK | B_CYAN,         }, // COL_VIEWERCLOCK,
	{"Dialog.List.Title",                           F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGLISTTITLE
	{"Dialog.List.Box",                             F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGLISTBOX,
	{"WarnDialog.Edit.Selected",                    F_WHITE | B_BLACK,        }, // COL_WARNDIALOGEDITSELECTED,
	{"WarnDialog.Edit.Unchanged",                   F_LIGHTGRAY | B_CYAN,     }, // COL_WARNDIALOGEDITUNCHANGED,
	{"Dialog.Combo.Text",                           F_WHITE | B_CYAN,         }, // COL_DIALOGCBOXTEXT,
	{"Dialog.Combo.Text.Selected",                  F_WHITE | B_BLACK,        }, // COL_DIALOGCBOXSELECTEDTEXT,
	{"Dialog.Combo.Highlight",                      F_YELLOW | B_CYAN,        }, // COL_DIALOGCBOXHIGHLIGHT,
	{"Dialog.Combo.Highlight.Selected",             F_YELLOW | B_BLACK,       }, // COL_DIALOGCBOXSELECTEDHIGHLIGHT,
	{"Dialog.Combo.Box",                            F_WHITE | B_CYAN,         }, // COL_DIALOGCBOXBOX,
	{"Dialog.Combo.Title",                          F_WHITE | B_CYAN,         }, // COL_DIALOGCBOXTITLE,
	{"Dialog.Combo.Disabled",                       F_DARKGRAY | B_CYAN,      }, // COL_DIALOGCBOXDISABLED,
	{"Dialog.Combo.Scrollbar",                      F_WHITE | B_CYAN,         }, // COL_DIALOGCBOXSCROLLBAR,
	{"WarnDialog.List.Text",                        F_WHITE | B_RED,          }, // COL_WARNDIALOGLISTTEXT,
	{"WarnDialog.List.Text.Selected",               F_BLACK | B_LIGHTGRAY,    }, // COL_WARNDIALOGLISTSELECTEDTEXT,
	{"WarnDialog.List.Highlight",                   F_YELLOW | B_RED,         }, // COL_WARNDIALOGLISTHIGHLIGHT,
	{"WarnDialog.List.Highlight.Selected",          F_YELLOW | B_LIGHTGRAY,   }, // COL_WARNDIALOGLISTSELECTEDHIGHLIGHT,
	{"WarnDialog.List.Box",                         F_WHITE | B_RED,          }, // COL_WARNDIALOGLISTBOX,
	{"WarnDialog.List.Title",                       F_WHITE | B_RED,          }, // COL_WARNDIALOGLISTTITLE,
	{"WarnDialog.List.Scrollbar",                   F_WHITE | B_RED,          }, // COL_WARNDIALOGLISTSCROLLBAR,
	{"WarnDialog.Combo.Text",                       F_WHITE | B_CYAN,         }, // COL_WARNDIALOGCBOXTEXT,
	{"WarnDialog.Combo.Text.Selected",              F_WHITE | B_BLACK,        }, // COL_WARNDIALOGCBOXSELECTEDTEXT,
	{"WarnDialog.Combo.Highlight",                  F_YELLOW | B_CYAN,        }, // COL_WARNDIALOGCBOXHIGHLIGHT,
	{"WarnDialog.Combo.Highlight.Selected",         F_YELLOW | B_BLACK,       }, // COL_WARNDIALOGCBOXSELECTEDHIGHLIGHT,
	{"WarnDialog.Combo.Box",                        F_WHITE | B_CYAN,         }, // COL_WARNDIALOGCBOXBOX,
	{"WarnDialog.Combo.Title",                      F_WHITE | B_CYAN,         }, // COL_WARNDIALOGCBOXTITLE,
	{"WarnDialog.Combo.Disabled",                   F_DARKGRAY | B_CYAN,      }, // COL_WARNDIALOGCBOXDISABLED,
	{"WarnDialog.Combo.Scrollbar",                  F_WHITE | B_CYAN,         }, // COL_WARNDIALOGCBOXSCROLLBAR,
	{"Dialog.List.Arrows",                          F_YELLOW | B_LIGHTGRAY,   }, // COL_DIALOGLISTARROWS,
	{"Dialog.List.Arrows.Disabled",                 F_DARKGRAY | B_LIGHTGRAY, }, // COL_DIALOGLISTARROWSDISABLED,
	{"Dialog.List.Arrows.Selected",                 F_YELLOW | B_BLACK,       }, // COL_DIALOGLISTARROWSSELECTED,
	{"Dialog.Combo.Arrows",                         F_YELLOW | B_CYAN,        }, // COL_DIALOGCOMBOARROWS,
	{"Dialog.Combo.Arrows.Disabled",                F_DARKGRAY | B_CYAN,      }, // COL_DIALOGCOMBOARROWSDISABLED,
	{"Dialog.Combo.Arrows.Selected",                F_YELLOW | B_BLACK,       }, // COL_DIALOGCOMBOARROWSSELECTED,
	{"WarnDialog.List.Arrows",                      F_YELLOW | B_RED,         }, // COL_WARNDIALOGLISTARROWS,
	{"WarnDialog.List.Arrows.Disabled",             F_DARKGRAY | B_RED,       }, // COL_WARNDIALOGLISTARROWSDISABLED,
	{"WarnDialog.List.Arrows.Selected",             F_YELLOW | B_LIGHTGRAY,   }, // COL_WARNDIALOGLISTARROWSSELECTED,
	{"WarnDialog.Combo.Arrows",                     F_YELLOW | B_CYAN,        }, // COL_WARNDIALOGCOMBOARROWS,
	{"WarnDialog.Combo.Arrows.Disabled",            F_DARKGRAY | B_CYAN,      }, // COL_WARNDIALOGCOMBOARROWSDISABLED,
	{"WarnDialog.Combo.Arrows.Selected",            F_YELLOW | B_BLACK,       }, // COL_WARNDIALOGCOMBOARROWSSELECTED,
	{"Menu.Arrows",                                 F_YELLOW | B_CYAN,        }, // COL_MENUARROWS,
	{"Menu.Arrows.Disabled",                        F_DARKGRAY | B_CYAN,      }, // COL_MENUARROWSDISABLED,
	{"Menu.Arrows.Selected",                        F_YELLOW | B_BLACK,       }, // COL_MENUARROWSSELECTED,
	{"CommandLine.UserScreen",                      ColorsInit_s::Default,    }, // COL_COMMANDLINEUSERSCREEN,
	{"Editor.Scrollbar",                            F_LIGHTCYAN | B_BLUE,     }, // COL_EDITORSCROLLBAR,
	{"Menu.GrayText",                               F_DARKGRAY | B_CYAN,      }, // COL_MENUGRAYTEXT,
	{"Menu.GrayText.Selected",                      F_LIGHTGRAY | B_BLACK,    }, // COL_MENUSELECTEDGRAYTEXT,
	{"Dialog.Combo.GrayText",                       F_DARKGRAY | B_CYAN,      }, // COL_DIALOGCOMBOGRAY,
	{"Dialog.Combo.GrayText.Selected",              F_LIGHTGRAY | B_BLACK,    }, // COL_DIALOGCOMBOSELECTEDGRAYTEXT,
	{"Dialog.List.GrayText",                        F_DARKGRAY | B_LIGHTGRAY, }, // COL_DIALOGLISTGRAY,
	{"Dialog.List.GrayText.Selected",               F_LIGHTGRAY | B_BLACK,    }, // COL_DIALOGLISTSELECTEDGRAYTEXT,
	{"WarnDialog.Combo.GrayText",                   F_DARKGRAY | B_CYAN,      }, // COL_WARNDIALOGCOMBOGRAY,
	{"WarnDialog.Combo.GrayText.Selected",          F_LIGHTGRAY | B_BLACK,    }, // COL_WARNDIALOGCOMBOSELECTEDGRAYTEXT,
	{"WarnDialog.List.GrayText",                    F_DARKGRAY | B_RED,       }, // COL_WARNDIALOGLISTGRAY,
	{"WarnDialog.List.GrayText.Selected",           F_BLACK | B_LIGHTGRAY,    }, // COL_WARNDIALOGLISTSELECTEDGRAYTEXT,
	{"Dialog.DefaultButton",                        F_BLACK | B_LIGHTGRAY,    }, // COL_DIALOGDEFAULTBUTTON,
	{"Dialog.DefaultButton.Selected",               F_BLACK | B_CYAN,         }, // COL_DIALOGSELECTEDDEFAULTBUTTON,
	{"Dialog.DefaultButton.Highlight",              F_YELLOW | B_LIGHTGRAY,   }, // COL_DIALOGHIGHLIGHTDEFAULTBUTTON,
	{"Dialog.DefaultButton.Highlight.Selected",     F_YELLOW | B_CYAN,        }, // COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON,
	{"WarnDialog.DefaultButton",                    F_WHITE | B_RED,          }, // COL_WARNDIALOGDEFAULTBUTTON,
	{"WarnDialog.DefaultButton.Selected",           F_BLACK | B_LIGHTGRAY,    }, // COL_WARNDIALOGSELECTEDDEFAULTBUTTON,
	{"WarnDialog.DefaultButton.Highlight",          F_YELLOW | B_RED,         }, // COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON,
	{"WarnDialog.DefaultButton.Highlight.Selected", F_YELLOW | B_LIGHTGRAY,   }, // COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON,
	{"Dialog.OverflowArrow",                        F_YELLOW | B_CYAN,        }, // COL_DIALOGOVERFLOWARROW,
	{"WarnDialog.OverflowArrow",                    F_YELLOW | B_RED,         }, // COL_WARNDIALOGOVERFLOWARROW,
	{"Dialog.List",                                 F_DARKGRAY | B_LIGHTGRAY, }, // COL_DIALOGLISTPREFIX,
	{"Dialog.List.Selected",                        F_LIGHTGRAY | B_BLACK,    }, // COL_DIALOGLISTSELPREFIX,
	{"Dialog.Combo",                                F_DARKGRAY | B_CYAN,      }, // COL_DIALOGCOMBOPREFIX,
	{"Dialog.Combo.Selected",                       F_DARKGRAY | B_RED,       }, // COL_DIALOGCOMBOSELPREFIX,
	{"WarnDialog.List",                             F_BLACK | B_LIGHTGRAY,    }, // COL_WARNDIALOGLISTPREFIX,
	{"WarnDialog.List.Selected",                    F_DARKGRAY | B_RED,       }, // COL_WARNDIALOGLISTSELPREFIX,
	{"WarnDialog.Combo",                            F_DARKGRAY | B_CYAN,      }, // COL_WARNDIALOGCOMBOPREFIX,
	{"WarnDialog.Combo.Selected",                   F_LIGHTGRAY | B_BLACK,    }, // COL_WARNDIALOGCOMBOSELPREFIX,
	{"Menu.Prefix",                                 F_DARKGRAY | B_CYAN,      }, // COL_MENUPREFIX,
	{"Menu.Prefix.Selected",                        F_LIGHTGRAY | B_BLACK,    }, // COL_MENUSELPREFIX,
};

static_assert(ARRAYSIZE(ColorsInit) == COL_LASTPALETTECOLOR);

class FarColors FarColors::FARColors;
uint64_t FarColors::setcolors[SIZE_ARRAY_FARCOLORS];
uint32_t FarColors::GammaCorrection = 0;
bool FarColors::GammaChanged = false;


FarColors::FarColors() noexcept {

}

FarColors::~FarColors() noexcept {

}

#define FARCOLORS_SECTION "farcolors"
#define FARCOLORS_CONFIG "settings/farcolors.ini"
#define FARCOLORS_DEFCOLOR "(B_RED | F_WHITE)"

bool FarColors::Save(KeyFileHelper &kfh) noexcept {

	fprintf(stderr, "FarColors::Save(KeyFileHelper &kfh)\n" );

	char tmp[256];
	for (size_t i = 0; i < SIZE_ARRAY_FARCOLORS; i++) {
		FarColorToExpr(FARColors.colors[i], tmp, 256);
		kfh.SetString(FARCOLORS_SECTION, ColorsInit[i].name, tmp);
	}

	return true;
}

bool FarColors::Load(KeyFileHelper &kfh) noexcept {

	fprintf(stderr, "FarColors::Load(KeyFileHelper &kfh)\n" );
	if (!kfh.HasSection(FARCOLORS_SECTION)) return false;

	for (size_t i = 0; i < SIZE_ARRAY_FARCOLORS; i++) {
		const std::string &expstr = kfh.GetString(FARCOLORS_SECTION, ColorsInit[i].name);

		uint64_t color = expstr.empty() ? DefaultColorsIndex16[i] : ExprToFarColor(expstr.c_str(), expstr.size());

//		if (rez)
		FARColors.colors[i] = color;
//		else {
//			FARColors.colors[i] = 4 * 16 + 15;
//			fprintf(stderr, " %s = %s = %lu rez = %u\n", ColorsInit[i].name.c_str(), expstr.c_str(), color, rez );
//		}
	}

	return true;
}

#include "def16index.h"

void FarColors::ResetToDefaultIndexRGB( uint8_t *indexes ) noexcept {

	fprintf(stderr, "FarColors::ResetToDefaultRGB( )\n" );

	if (!indexes) indexes = DefaultColorsIndex16;
	for(size_t i = 0; i < SIZE_ARRAY_FARCOLORS; i++) {
		uint8_t color = indexes[i];

		uint32_t c = Palette::FARPalette[16 + (color & 0x0F)];
		colors[i] = ((uint64_t)(c & 0x00ffffff) << 16);
		uint32_t c2 = Palette::FARPalette[color >> 4];
		colors[i] += ((uint64_t)(c2 & 0x00ffffff) << 40);

		colors[i] += FOREGROUND_TRUECOLOR + BACKGROUND_TRUECOLOR;
		colors[i] += color;
	}
}

void FarColors::ResetToDefaultIndex( uint8_t *indexes ) noexcept {

	fprintf(stderr, "FarColors::ResetToDefaultIndex( )\n" );

	if (!indexes) indexes = DefaultColorsIndex16;
	for(size_t i = 0; i < SIZE_ARRAY_FARCOLORS; i++) {
		uint8_t color = indexes[i];
		colors[i] = color;
	}
}

void FarColors::SaveFarColors( ) noexcept {

	fprintf(stderr, "void FarColors::SaveFarColors( ) {\n" );

	const std::string &colors_file = InMyConfig(FARCOLORS_CONFIG);
	KeyFileHelper kfh(colors_file);
	FARColors.Save(kfh);
	kfh.Save();
}

void FarColors::InitFarColors( ) noexcept {

	fprintf(stderr, "void FarColors::InitFarColors( ) {\n" );

	Palette::InitFarPalette();
	const std::string &colors_file = InMyConfig(FARCOLORS_CONFIG);

	KeyFileHelper kfh(colors_file);

	if (kfh.IsLoaded()) {
		if (FARColors.Load(kfh)) {
			FARColors.Set();
			return;
		}
		else
			fprintf(stderr, "InitFarColors(): failed to parse '%s'\n", colors_file.c_str());
	}

	ConfigReader cfg_reader("Colors");
	if (cfg_reader.HasSection()) {
		const std::string strCurrentPaletteRGB = "CurrentPaletteRGB";
		const std::string strCurrentPalette = "CurrentPalette";
		if (cfg_reader.HasKey(strCurrentPaletteRGB)) {
			if (cfg_reader.GetBytes((unsigned char*)FARColors.colors, SIZE_ARRAY_FARCOLORS * sizeof(uint64_t), strCurrentPaletteRGB) == SIZE_ARRAY_FARCOLORS * sizeof(uint64_t)) {
				FARColors.Set();
				return;
			}
		}
		if (cfg_reader.HasKey(strCurrentPalette)) {
			uint8_t indexes[SIZE_ARRAY_FARCOLORS];
			if (cfg_reader.GetBytes((unsigned char*)indexes, SIZE_ARRAY_FARCOLORS, strCurrentPalette) == SIZE_ARRAY_FARCOLORS) {
				FARColors.ResetToDefaultIndex(indexes);
				FARColors.Set();
				return;
			}
		}
	}

	FARColors.ResetToDefaultIndexRGB(DefaultColorsIndex16);
	FARColors.Set();
}
