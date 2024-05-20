/*
pick_color.cpp

Pick color dialog
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2020 Far Group
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
#include <wchar.h>
#include "setcolor.hpp"
#include "pick_color256.hpp"
#include "pick_colorRGB.hpp"
#include "pick_color.hpp"
#include "pick_color_common.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "colors.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "ctrlobj.hpp"
#include "savescr.hpp"
#include "scrbuf.hpp"
#include "panel.hpp"
#include "chgmmode.hpp"
#include "interf.hpp"
#include "palette.hpp"
#include "config.hpp"

#include "VT256ColorTable.h" // For g_VT256ColorTable[VT_256COLOR_TABLE_COUNT]

enum enumColorPanelElements
{
	ID_CP_CHECKBOX = 0,
	ID_CP_TEXT,
	ID_CP_COLORS_RECT,
	ID_CP_CHECKBOX_RGB,
	ID_CP_EDIT_RGB,
	ID_CP_BUTTON_256,
	ID_CP_BUTTON_RGB,
	ID_CP_RGB_SAMPLE,
	ID_CP_RGB_PREFIX,
	ID_CP_TOTAL
};

enum enumSetColorDialog
{
	ID_ST_TITLE = 0,

	ID_ST_SEPARATOR,
	ID_ST_SEPARATOR2,
	ID_ST_SEPARATOR3,
	ID_ST_SEPARATOR4,
	ID_ST_SEPARATOR5,

	ID_CP_FIRST,
	/// enumColorPanelElements foreground
	ID_ST_CHECKBOX_FOREGROUND = ID_CP_FIRST,
	ID_ST_TEXT_FOREGROUND,
	ID_ST_FG_COLORS_RECT,
	ID_ST_CHECKBOX_RGB1,
	ID_ST_EDIT_FORERGB,
	ID_ST_BUTTON_F256,
	ID_ST_BUTTON_FRGB,
	ID_ST_FRGB_SMAPLE,
	ID_ST_FRGB_PREFIX,

	/// enumColorPanelElements background
	ID_ST_CHECKBOX_BACKGROUND,
	ID_ST_TEXT_BACKGROUND,
	ID_ST_BG_COLORS_RECT,
	ID_ST_CHECKBOX_RGB2,
	ID_ST_EDIT_BACKRGB,
	ID_ST_BUTTON_B256,
	ID_ST_BUTTON_BRGB,
	ID_ST_BRGB_SMAPLE,
	ID_ST_BRGB_PREFIX,

	/// text style checkboxes
	ID_ST_TEXT_STYLE,
	ID_ST_CHECKBOX_STYLE_ENABLE,
	ID_ST_CHECKBOX_STYLE_FIRST,
	ID_ST_CHECKBOX_STYLE_BOLD = ID_ST_CHECKBOX_STYLE_FIRST,
	ID_ST_CHECKBOX_STYLE_ITALIC,
	ID_ST_CHECKBOX_STYLE_OVERLINE,
	ID_ST_CHECKBOX_STYLE_STRIKEOUT,
	ID_ST_CHECKBOX_STYLE_UNDERLINE,
	ID_ST_CHECKBOX_STYLE_BLINK,
	ID_ST_CHECKBOX_STYLE_INVERSE,
	ID_ST_CHECKBOX_STYLE_LAST = ID_ST_CHECKBOX_STYLE_INVERSE,

	ID_ST_COLOREXAMPLE,

	ID_ST_BUTTON_SETCOLOR,
	ID_ST_BUTTON_RESTORE,
	ID_ST_BUTTON_CANCEL
};

typedef struct st_font_style_bind_s {
	int32_t	id;
	int64_t	flags;
} st_font_style_bind_t;

static st_font_style_bind_t sup_styles[] = {

	{ID_ST_CHECKBOX_STYLE_STRIKEOUT, COMMON_LVB_STRIKEOUT},
	{ID_ST_CHECKBOX_STYLE_UNDERLINE, COMMON_LVB_UNDERSCORE},
	{ID_ST_CHECKBOX_STYLE_INVERSE,   COMMON_LVB_REVERSE_VIDEO},
};

static uint32_t basepalette[32];

#define ST_ALL_FONT_STYLES (\
							  COMMON_LVB_STRIKEOUT\
							| COMMON_LVB_UNDERSCORE\
							| COMMON_LVB_REVERSE_VIDEO\
)

struct color_panel_s
{
	CHAR_INFO vbuff[64];
	CHAR_INFO vbuff_rgb[8];
	uint64_t color;		// rgb
	uint32_t id;
	uint32_t offset;
	uint32_t index;		// foreground or background index 0 - 15
	bool bTransparent;
	bool bRGB;
	wchar_t wsRGB[16];

	void update_index(int32_t newindex) {
		newindex &= 15;
		vbuff[index * 3 + 1].Char.UnicodeChar = 32;
		vbuff[newindex * 3 + 1].Char.UnicodeChar = L'\x2022'; // DOT
		index = newindex;
	}

	void update_rgb_color_from_str( ) {
		color = RGB_2_BGR(wcstoul(wsRGB, nullptr, 16));
		draw_rgb_sample( );
	}

	void update_str_from_rgb_color( ) {
		swprintf(wsRGB, 16, L"%06X", RGB_2_BGR(color));
	}

	void draw_rgb_sample( ) {
		const uint64_t attr = ATTR_RGBBACK_NEGF(color);

		vbuff_rgb[0].Char.UnicodeChar = 32;
		vbuff_rgb[0].Attributes = attr;
		vbuff_rgb[1].Char.UnicodeChar = L'\x2022'; // DOT
		vbuff_rgb[1].Attributes = attr;
		vbuff_rgb[2].Char.UnicodeChar = 32;
		vbuff_rgb[2].Attributes = attr;
	}

	LONG_PTR WINAPI ColorPanelUserProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
};

struct set_color_s
{
	enum {
		IDC_FOREGROUND_PANEL = 0,
		IDC_BACKGROUND_PANEL = 1
	};
	color_panel_s cPanel[2];

	CHAR_INFO samplevbuff[132];
	uint64_t color;
	uint64_t style;
	uint64_t style_inherit;
	uint64_t smpcolor;
	uint64_t mask;
	uint64_t resetcolor;
	uint64_t resetmask;
	uint64_t flags;
	uint32_t basepalbpc;
	bool bTransparencyEnabled;
	bool bRGBEnabled;
	bool bStyleEnabled;
	bool bStyle;

	set_color_s() {

		memset(this, 0, sizeof(set_color_s));
		basepalbpc = WINPORT(GetConsoleColorPalette)(NULL);

		draw_panels_vbuff( );
		cPanel[IDC_FOREGROUND_PANEL].id = IDC_FOREGROUND_PANEL;
		cPanel[IDC_FOREGROUND_PANEL].offset = ID_CP_FIRST;
		cPanel[IDC_BACKGROUND_PANEL].id = IDC_BACKGROUND_PANEL;
		cPanel[IDC_BACKGROUND_PANEL].offset = ID_CP_FIRST + ID_CP_TOTAL;
	}

	inline void enable_RGB(const bool bEnable) {
		bRGBEnabled = bEnable;
	}

	inline void enable_transparency(const bool bEnable) {
		bTransparencyEnabled = bEnable;
	}

	inline void enable_font_styles(const bool bEnable) {
		bStyleEnabled = bEnable;
	}

	void update_panel_indexes(void)
	{
		cPanel[IDC_FOREGROUND_PANEL].update_index(color & 0xF);
		cPanel[IDC_BACKGROUND_PANEL].update_index((color & 0xFF) >> 4);
	}

	void set_color(const uint64_t color)
	{
		this->color = resetcolor = color;
		mask = 0xFFFFFFFFFFFFFFFF;
		flags = (color & 0x000000000000FF00) >> 8;

		cPanel[IDC_FOREGROUND_PANEL].bTransparent = false;
		cPanel[IDC_BACKGROUND_PANEL].bTransparent = false;

		update_panel_indexes();

		if (bRGBEnabled) {
			cPanel[IDC_FOREGROUND_PANEL].color = (color >> 16) & 0xFFFFFF;
			cPanel[IDC_BACKGROUND_PANEL].color = (color >> 40);

			if (color & 0x000000FFFFFF0000 || color & FOREGROUND_TRUECOLOR) {
				cPanel[IDC_FOREGROUND_PANEL].bRGB = true;
			}

			if (color & 0xFFFFFF0000000000 || color & BACKGROUND_TRUECOLOR) {
				cPanel[IDC_BACKGROUND_PANEL].bRGB = true;
			}

			cPanel[IDC_FOREGROUND_PANEL].update_str_from_rgb_color( );
			cPanel[IDC_BACKGROUND_PANEL].update_str_from_rgb_color( );

			cPanel[IDC_FOREGROUND_PANEL].draw_rgb_sample( );
			cPanel[IDC_BACKGROUND_PANEL].draw_rgb_sample( );
		}

		style = style_inherit = 0;

		if (bStyleEnabled) {
			style = color & ST_ALL_FONT_STYLES;
			if (style)
				bStyle = true;
		}
	}

	void set_mask(const uint64_t mask)
	{
		this->mask = resetmask = mask;

		if (bTransparencyEnabled) {
			if (!(mask & 0xF))
				cPanel[IDC_FOREGROUND_PANEL].bTransparent = true;

			if (!(mask & 0xF0))
				cPanel[IDC_BACKGROUND_PANEL].bTransparent = true;
		}

		style_inherit = 0;

		if (bStyleEnabled) {
			style_inherit = (~mask) & ST_ALL_FONT_STYLES;
			style_inherit &= ~style;

			if (style_inherit)
				bStyle = true;
		}
	}

	void reset_color(void)
	{
		set_color(resetcolor);
		if (bTransparencyEnabled) {
			set_mask(resetmask);
		}
		update_color( );
	}

	void update_color(void)
	{
		color = 0;
		color |= cPanel[IDC_FOREGROUND_PANEL].index;
		color |= cPanel[IDC_BACKGROUND_PANEL].index << 4;

		if (bRGBEnabled) {
			if (cPanel[IDC_FOREGROUND_PANEL].bRGB && cPanel[IDC_FOREGROUND_PANEL].color) {
				color |= (cPanel[IDC_FOREGROUND_PANEL].color << 16) & 0x000000FFFFFF0000;
				color |= FOREGROUND_TRUECOLOR;
			}

			if (cPanel[IDC_BACKGROUND_PANEL].bRGB && cPanel[IDC_BACKGROUND_PANEL].color) {
				color |= cPanel[IDC_BACKGROUND_PANEL].color << 40;
				color |= BACKGROUND_TRUECOLOR;
			}
		}

		mask = 0xFFFFFFFFFFFFFFFF;

		if (bStyleEnabled && bStyle)
			color |= style;

		smpcolor = color;

		if (bTransparencyEnabled) {

			if (cPanel[IDC_FOREGROUND_PANEL].bTransparent) {
				mask ^= (0x000000FFFFFF000F | FOREGROUND_TRUECOLOR);
				smpcolor &= (0xFFFFFF00000000F0 | BACKGROUND_TRUECOLOR);
			}

			if (cPanel[IDC_BACKGROUND_PANEL].bTransparent) {
				mask ^= (0xFFFFFF00000000F0 | BACKGROUND_TRUECOLOR);
				smpcolor &= (0x000000FFFFFF000F | FOREGROUND_TRUECOLOR);
			}

			if (bStyle)
				mask ^= style_inherit;
		}

		draw_sample_vbuff();
	}

	void draw_sample_vbuff(void);
	void draw_panels_vbuff(void);
};

void set_color_s::draw_sample_vbuff(void)
{
	static const wchar_t *sample_text_str = L"Text Text Text Text Text Text Text Text Text Text";

	if (bTransparencyEnabled && !(color & 0xFF)) {

		size_t blackonblacklen = wcslen(Msg::SetColorBlackOnBlack);
		size_t defcolorlen = wcslen(Msg::SetColorDefaultColor);
		const uint64_t attr = 15;

		if (blackonblacklen > 33)
			blackonblacklen = 33;
		if (defcolorlen > 33)
			defcolorlen = 33;

		for (size_t g = 0; g < 4 * 33  ; g++) {
			samplevbuff[g].Char.UnicodeChar = 32;
			samplevbuff[g].Attributes = attr;
		}

		for (size_t g = 0; g < blackonblacklen; g++) {
			samplevbuff[g].Char.UnicodeChar = Msg::SetColorBlackOnBlack[g];
			samplevbuff[g].Attributes = attr;
		}

		for (size_t g = 0; g < defcolorlen; g++) {
			samplevbuff[g + 33].Char.UnicodeChar = Msg::SetColorDefaultColor[g];
			samplevbuff[g + 33].Attributes = attr;
		}

		return;
	}

	for (size_t i = 0; i < 4; i++) {
		const uint64_t attr = (i > 0) ? smpcolor : smpcolor & 0xFF;

		for(size_t g = 0; g < 33; g++) {
			CHAR_INFO *const vbuff = samplevbuff + i * 33;

			vbuff[g].Char.UnicodeChar = sample_text_str[g];
			vbuff[g].Attributes = attr;
		}
	}
}

void set_color_s::draw_panels_vbuff(void)
{
	static const uint8_t dotfc[16] = { 15, 15, 0, 0, 15, 15, 15, 0, 15, 0, 0, 0, 0, 0, 0, 0 };

	// For foreground-colored boxes invert Fg&Bg colors and add COMMON_LVB_REVERSE_VIDEO attribute
	// this will put real colors on them if mapping of colors is different for Fg and Bg indexes
	// 0xBF

	// Fill foreground colors rect
	for (size_t i = 0; i < 16; i++) {
		CHAR_INFO *const vbuff = cPanel[IDC_FOREGROUND_PANEL].vbuff;
		uint64_t attr = ATTR_RGBBACK_NEGF(basepalette[i + 16]) + (i << 4) + dotfc[i];

		vbuff[i * 3 + 0].Char.UnicodeChar = 32;
		vbuff[i * 3 + 0].Attributes = attr;
		vbuff[i * 3 + 1].Char.UnicodeChar = 32;
		vbuff[i * 3 + 1].Attributes = attr;
		vbuff[i * 3 + 2].Char.UnicodeChar = 32;
		vbuff[i * 3 + 2].Attributes = attr;
	}

	// Fill background colors rect
	for (size_t i = 0; i < 16; i++) {
		CHAR_INFO *const vbuff = cPanel[IDC_BACKGROUND_PANEL].vbuff;
		uint64_t attr = ATTR_RGBBACK_NEGF(basepalette[i]) + (i << 4) + dotfc[i];

		vbuff[i * 3 + 0].Char.UnicodeChar = 32;
		vbuff[i * 3 + 0].Attributes = attr;
		vbuff[i * 3 + 1].Char.UnicodeChar = 32;
		vbuff[i * 3 + 1].Attributes = attr;
		vbuff[i * 3 + 2].Char.UnicodeChar = 32;
		vbuff[i * 3 + 2].Attributes = attr;
	}
}

#define DM_UPDATECURSOR (DM_USER + 1)
#define DM_UPDATECOLORCODE (DM_USER + 2)

LONG_PTR WINAPI color_panel_s::ColorPanelUserProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	auto update_cursor = [=]() {
		COORD coord;
		coord.X = (index & 7) * 3 + 1;
		coord.Y = (index > 7);
		SendDlgMessage(hDlg, DM_SETCURSORPOS, ID_CP_COLORS_RECT + offset, (LONG_PTR)&coord);
	};

	auto update_index_and_cursor = [&](uint32_t newindex) {
		update_index(newindex);
		update_cursor();
		SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
	};

	switch(Param1) {
	case ID_CP_RGB_PREFIX: {
		if (Msg == DN_CTLCOLORDLGITEM) {
			uint64_t *ItemColor = (uint64_t *)Param2;
			if (bRGB)
				ItemColor[0] = FarColorToReal(COL_DIALOGEDIT);
			else
				ItemColor[0] = FarColorToReal(COL_DIALOGEDITDISABLED);
			return 1;
		}
	}
	break;

	case ID_CP_RGB_SAMPLE:
		if (Msg == DN_DRAWDLGITEM) {
			if (!bRGB)
				return 0;
		}
	break;

	case ID_CP_CHECKBOX:
		if (Msg == DN_BTNCLICK) {
			bTransparent = (!SendDlgMessage(hDlg, DM_GETCHECK, ID_CP_CHECKBOX + offset, 0));
			SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
		}
	break;

	case ID_CP_CHECKBOX_RGB:
		if (Msg == DN_BTNCLICK) {
			bRGB = SendDlgMessage(hDlg, DM_GETCHECK, ID_CP_CHECKBOX_RGB + offset, 0);
			if (bRGB && !color) {
				color = basepalette[id ? index : index + 16];

				update_str_from_rgb_color( );
				draw_rgb_sample( );
				SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_CP_EDIT_RGB + offset, (LONG_PTR)wsRGB);
				SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
			}
			SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
		}
	break;

	case ID_CP_EDIT_RGB:
		if (Msg == DN_EDITCHANGE) {
			SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_CP_EDIT_RGB + offset, (LONG_PTR)wsRGB);
			update_rgb_color_from_str( );
			SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
		}
	break;

	case ID_CP_BUTTON_256:
		if (Msg == DN_BTNCLICK) {
			uint32_t index256 = color;

			if (!GetPickColorDialog256(&index256, false))
				break;

			if (index256 < 16) {
				update_index_and_cursor(index256);
				break;
			}
			color = g_VT256ColorTable[(index256 - 16) & 255];
			update_str_from_rgb_color( );
			draw_rgb_sample( );
			SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_CP_EDIT_RGB + offset, (LONG_PTR)wsRGB);
			SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
		}
	break;

	case ID_CP_BUTTON_RGB:
		if (Msg == DN_BTNCLICK) {
			uint32_t newrgb = (uint32_t)color;

			if (!GetPickColorDialogRGB(&newrgb, false))
				break;

			color = newrgb;
			update_str_from_rgb_color( );
			draw_rgb_sample( );
			SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_CP_EDIT_RGB + offset, (LONG_PTR)wsRGB);
			SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
		}
	break;


	case ID_CP_COLORS_RECT:
		switch(Msg) {

		case DM_UPDATECURSOR:
			update_cursor();
		break;

//		case DN_BTNCLICK:
		case DN_MOUSECLICK: {
			MOUSE_EVENT_RECORD *mEv = (MOUSE_EVENT_RECORD *)Param2;
			uint32_t newindex = mEv->dwMousePosition.X / 3 + (mEv->dwMousePosition.Y & 1) * 8;
			update_index_and_cursor(newindex);
			if (mEv->dwEventFlags == DOUBLE_CLICK) {
				color = basepalette[id ? index : index + 16];
				update_str_from_rgb_color( );
				draw_rgb_sample( );
				SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_CP_EDIT_RGB + offset, (LONG_PTR)wsRGB);
				SendDlgMessage(hDlg, DM_UPDATECOLORCODE, 0, 0);
			}

		}
		break;

		case DN_KEY:
			switch(Param2) {

			case KEY_LEFT:
			case KEY_NUMPAD4:
			case KEY_MSWHEEL_LEFT:
			case KEY_MSWHEEL_UP:
				update_index_and_cursor(index - 1);
			break;

			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_MSWHEEL_RIGHT:
			case KEY_MSWHEEL_DOWN:
				update_index_and_cursor(index + 1);
			break;

			case KEY_UP:
			case KEY_NUMPAD8:
			case KEY_PGUP:
			case KEY_NUMPAD9:
				update_index_and_cursor(index - 8);
			break;

			case KEY_DOWN:
			case KEY_NUMPAD2:
			case KEY_PGDN:
			case KEY_NUMPAD3:
				update_index_and_cursor(index + 8);
			break;

			case KEY_HOME:
			case KEY_NUMPAD7:
				update_index_and_cursor(0);
			break;

			case KEY_END:
			case KEY_NUMPAD1:
				update_index_and_cursor(15);
			break;
			}

		break;
		}

	break;
	}

	return -1;
}

static LONG_PTR WINAPI GetColorDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	set_color_s *colorState = (set_color_s *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

	auto update_dialog_items = [=]() {

		if (colorState->bTransparencyEnabled) {

			for (int g = 0; g < 2; g++ ) {
				const size_t offset = colorState->cPanel[g].offset;
				bool bSet = !colorState->cPanel[g].bTransparent;

				SendDlgMessage(hDlg, DM_SETCHECK, ID_CP_CHECKBOX + offset, bSet);
				for (size_t i = ID_CP_COLORS_RECT + offset; i <= ID_CP_RGB_PREFIX + offset; i++)
					SendDlgMessage(hDlg, DM_SHOWITEM, i, bSet);
			}
		}

		for (size_t i = 0; i < ARRAYSIZE(sup_styles); i++) {
			if (colorState->bTransparencyEnabled)
				SendDlgMessage(hDlg, DM_SET3STATE, sup_styles[i].id, true);

			if (colorState->bTransparencyEnabled && (colorState->style_inherit & sup_styles[i].flags))
				SendDlgMessage(hDlg, DM_SETCHECK, sup_styles[i].id, BSTATE_3STATE);
			else
				SendDlgMessage(hDlg, DM_SETCHECK, sup_styles[i].id, (bool)(colorState->style & sup_styles[i].flags));
		}

		for (int g = 0; g < 2; g++ ) {
			const size_t offset = colorState->cPanel[g].offset;

			bool bSet = colorState->cPanel[g].bRGB;
			SendDlgMessage(hDlg, DM_SETCHECK, ID_CP_CHECKBOX_RGB + offset, bSet);
			for (size_t i = ID_CP_EDIT_RGB + offset; i <= ID_CP_RGB_PREFIX + offset; i++)
				SendDlgMessage(hDlg, DM_ENABLE, i, bSet);

			SendDlgMessage(hDlg, DM_ENABLE, ID_CP_CHECKBOX_RGB + offset, colorState->bRGBEnabled);
		}

		if (colorState->bStyleEnabled) {

			for (size_t i = 0; i < ARRAYSIZE(sup_styles); i++)
				SendDlgMessage(hDlg, DM_ENABLE, sup_styles[i].id, colorState->bStyle);

			SendDlgMessage(hDlg, DM_SETCHECK, ID_ST_CHECKBOX_STYLE_ENABLE, colorState->bStyle);
		}
		SendDlgMessage(hDlg, DM_ENABLE, ID_ST_CHECKBOX_STYLE_ENABLE, colorState->bStyleEnabled);

	};

	auto set_focus = [=]() {

		SendDlgMessage(hDlg, DM_UPDATECURSOR, ID_ST_FG_COLORS_RECT, 0);
		SendDlgMessage(hDlg, DM_UPDATECURSOR, ID_ST_BG_COLORS_RECT, 0);

		if (!colorState->cPanel[set_color_s::IDC_FOREGROUND_PANEL].bTransparent)
			SendDlgMessage(hDlg, DM_SETFOCUS, ID_ST_FG_COLORS_RECT, 0);
		else if (!colorState->cPanel[set_color_s::IDC_BACKGROUND_PANEL].bTransparent)
			SendDlgMessage(hDlg, DM_SETFOCUS, ID_ST_BG_COLORS_RECT, 0);
		else
			SendDlgMessage(hDlg, DM_SETFOCUS, ID_ST_CHECKBOX_FOREGROUND, 0);
	};

	switch (Msg) {

	case DM_UPDATECOLORCODE: {
		colorState->update_color( );
		SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
	}
	break;

	case DN_INITDIALOG: {
		SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID_ST_FG_COLORS_RECT, Opt.CursorSize[1]);
		SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID_ST_BG_COLORS_RECT, Opt.CursorSize[1]);
		set_focus( );
		update_dialog_items( );
	}
	break;

	case DN_BTNCLICK: {
		if (Param1 == ID_ST_CHECKBOX_STYLE_ENABLE) {
			colorState->bStyle = (bool)(SendDlgMessage(hDlg, DM_GETCHECK, ID_ST_CHECKBOX_STYLE_ENABLE, 0));
			colorState->update_color( );
			SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			break;
		}

		if (Param1 >= ID_ST_CHECKBOX_STYLE_FIRST && Param1 <= ID_ST_CHECKBOX_STYLE_LAST) {
			for (size_t i = 0; i < ARRAYSIZE(sup_styles); i++) {
				const int32_t &id = sup_styles[i].id;
				const int32_t &stf = sup_styles[i].flags;
				if (id != Param1)
					continue;

				int iState = (int)SendDlgMessage(hDlg, DM_GETCHECK, id, 0);

				if (iState == BSTATE_UNCHECKED) {
					colorState->style &= (~stf);
					colorState->style_inherit &= (~stf);
				}
				else if (iState == BSTATE_CHECKED) {
					colorState->style |= stf;
					colorState->style_inherit &= (~stf);
				}
				else if (iState == BSTATE_3STATE) {
					colorState->style_inherit |= stf;
					colorState->style &= (~stf);
				}

				colorState->update_color( );
			}
			break;
		}

		if (Param1 == ID_ST_BUTTON_RESTORE) {
			SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
			colorState->reset_color( );
			SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_ST_EDIT_FORERGB, (LONG_PTR)colorState->cPanel[set_color_s::IDC_FOREGROUND_PANEL].wsRGB);
			SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_ST_EDIT_BACKRGB, (LONG_PTR)colorState->cPanel[set_color_s::IDC_BACKGROUND_PANEL].wsRGB);
			set_focus( );
			update_dialog_items( );
			SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
			break;
		}
	}
	break;

	case DN_MOUSECLICK: {
	}
	break;

	} // switch

	if (Param1 >= ID_CP_FIRST && Param1 < ID_CP_FIRST + ID_CP_TOTAL * 2) {
		const uint32_t id = (Param1 >= ID_CP_FIRST + ID_CP_TOTAL);
		const LONG_PTR rv = colorState->cPanel[id].ColorPanelUserProc(hDlg, Msg, (Param1 - id * ID_CP_TOTAL) - ID_CP_FIRST, Param2);

		if (rv != -1)
			return rv;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool GetColorDialogInner(uint64_t *color, uint64_t *mask, bool bRGB, bool bFontStyles, bool bCentered)
{
	const wchar_t VerticalLine[] = { BoxSymbols[BS_T_H2V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1],BoxSymbols[BS_V1],
			BoxSymbols[BS_V1],BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1],
			BoxSymbols[BS_V1],BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_B_H1V1], 0};

	const wchar_t HorizontalLine[] = { BoxSymbols[BS_L_H1V2], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_R_H1V1], 0};

	const wchar_t HorizontalMini[] = { BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1],
			BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1], 0 };

	static const wchar_t *HexMask = L"HHHHHH";

	if (!color) return false;

//	for (int i = 0; i < 32; i++)
//		basepalette[i] = 0;

	WINPORT(GetConsoleBasePalette)(NULL, basepalette);

//	for (int i = 0; i < 16; i++)
//		basepalette[i] = (i * 16) + ((i * 16) << 16);

//	for (int i = 0; i < 16; i++)
//		basepalette[i+16] = (i * 16) << 8;

//	WINPORT(SetConsoleBasePalette)(NULL, basepalette);

	set_color_s	colorState;
	colorState.enable_RGB(bRGB);
	colorState.enable_font_styles(bFontStyles);
	colorState.set_color(*color);
	if (mask) {
		colorState.enable_transparency(true);
		colorState.set_mask(*mask);
	}

	colorState.update_color( );

	size_t extrasize = 10;
	{
		size_t ml;
		if ((ml = wcslen(Msg::PickColorStyleOverline)) > extrasize) extrasize = ml;
		if ((ml = wcslen(Msg::PickColorStyleStrikeout)) > extrasize) extrasize = ml;
		if ((ml = wcslen(Msg::PickColorStyleUnderline)) > extrasize) extrasize = ml;
		if ((ml = wcslen(Msg::PickColorStyleInverse)) > extrasize) extrasize = ml;
		if ((ml = wcslen(Msg::PickColorStyleBlinking)) > extrasize) extrasize = ml;
		if ((ml = wcslen(Msg::PickColorStyleBold)) > extrasize) extrasize = ml;
		if ((ml = wcslen(Msg::PickColorStyleItalic)) > extrasize) extrasize = ml;
	}

	DialogDataEx ColorDlgData[] = {

		{DI_DOUBLEBOX, 3, 1, int16_t(46 + extrasize), 18, {}, 0, Msg::SetColorTitle},

		{DI_TEXT,  0, 16,  0, 16, {}, DIF_SEPARATOR, L""},
		{DI_VTEXT, 39, 1, 39, 16, {}, DIF_BOXCOLOR, VerticalLine},
		{DI_TEXT, 3, 6,  39, 6, {}, DIF_BOXCOLOR, HorizontalLine},
		{DI_TEXT, 3, 11, 39, 11, {}, DIF_BOXCOLOR, HorizontalLine},
		{DI_TEXT, 41, 4, 54, 4, {}, DIF_BOXCOLOR, HorizontalMini},

		{DI_CHECKBOX, 5, 2, 20, 2, {}, DIF_AUTOMATION, Msg::SetColorForeground},
		{DI_TEXT, 5, 2, 20, 2, {}, DIF_HIDDEN, Msg::SetColorForeground},
		{DI_USERCONTROL, 5, 3, 28, 4, {}, 0, L"" },
		{DI_CHECKBOX, 30, 2, 36, 2, {}, DIF_AUTOMATION, L"RGB#"},
		{DI_FIXEDIT, 31, 3, 37, 3, {(DWORD_PTR)HexMask}, DIF_MASKEDIT, colorState.cPanel[set_color_s::IDC_FOREGROUND_PANEL].wsRGB},
		{DI_BUTTON, 30, 4, 36, 4, {}, DIF_BTNNOCLOSE, L"256‥"},
		{DI_BUTTON, 30, 5, 36, 5, {}, DIF_BTNNOCLOSE, L"RGB‥"},
		{DI_USERCONTROL, 26, 5, 28, 5, {}, DIF_NOFOCUS, L"" },
		{DI_TEXT, 30, 3, 30, 3, {}, 0, L"#"},

		{DI_CHECKBOX, 5, 7, 20, 7, {}, DIF_AUTOMATION, Msg::SetColorBackground},
		{DI_TEXT, 5, 7, 20, 7, {}, DIF_HIDDEN, Msg::SetColorBackground},
		{DI_USERCONTROL, 5, 8, 28, 9, {}, 0, L"" },
		{DI_CHECKBOX, 30, 7, 36, 7, {}, DIF_AUTOMATION, L"RGB#"},
		{DI_FIXEDIT, 31, 8, 37, 8, {(DWORD_PTR)HexMask}, DIF_MASKEDIT, colorState.cPanel[set_color_s::IDC_BACKGROUND_PANEL].wsRGB},
		{DI_BUTTON, 30, 9, 36, 9, {}, DIF_BTNNOCLOSE, L"256‥"},
		{DI_BUTTON, 30, 10, 36, 10, {}, DIF_BTNNOCLOSE, L"RGB‥"},
		{DI_USERCONTROL, 26, 10, 28, 10, {}, DIF_NOFOCUS, L"" },
		{DI_TEXT, 30, 8, 30, 8, {}, 0, L"#"},

		{DI_TEXT, 41, 2, 48, 2, {}, 0, Msg::PickColorStyle},
		{DI_CHECKBOX, 41, 3, 48, 3, {}, DIF_AUTOMATION, Msg::PickColorEnableStyle},

		{DI_CHECKBOX, 41, 5, 48, 5, {}, DIF_DISABLE, Msg::PickColorStyleBold},
		{DI_CHECKBOX, 41, 6, 48, 6, {}, DIF_DISABLE, Msg::PickColorStyleItalic},
		{DI_CHECKBOX, 41, 7, 48, 7, {}, DIF_DISABLE, Msg::PickColorStyleOverline},
		{DI_CHECKBOX, 41, 8, 48, 8, {}, DIF_DISABLE, Msg::PickColorStyleStrikeout},
		{DI_CHECKBOX, 41, 9, 48, 9, {}, DIF_DISABLE, Msg::PickColorStyleUnderline},
		{DI_CHECKBOX, 41, 10, 48, 10, {}, DIF_DISABLE, Msg::PickColorStyleBlinking},
		{DI_CHECKBOX, 41, 11, 48, 11, {}, DIF_DISABLE, Msg::PickColorStyleInverse},

		{DI_USERCONTROL, 5, 12, 37, 15, {}, DIF_NOFOCUS, L"" },

		{DI_BUTTON, 0, 17, 0, 17, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::SetColorSet},
		{DI_BUTTON, 0, 17, 0, 17, {}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, Msg::SetColorReset},
		{DI_BUTTON, 0, 17, 0, 17, {}, DIF_CENTERGROUP, Msg::SetColorCancel},
	};

	MakeDialogItemsEx(ColorDlgData, ColorDlg);

	ColorDlg[ID_ST_FG_COLORS_RECT].VBuf = colorState.cPanel[set_color_s::IDC_FOREGROUND_PANEL].vbuff;
	ColorDlg[ID_ST_BG_COLORS_RECT].VBuf = colorState.cPanel[set_color_s::IDC_BACKGROUND_PANEL].vbuff;
	ColorDlg[ID_ST_FRGB_SMAPLE].VBuf = colorState.cPanel[set_color_s::IDC_FOREGROUND_PANEL].vbuff_rgb;
	ColorDlg[ID_ST_BRGB_SMAPLE].VBuf = colorState.cPanel[set_color_s::IDC_BACKGROUND_PANEL].vbuff_rgb;
	ColorDlg[ID_ST_COLOREXAMPLE].VBuf = colorState.samplevbuff;

	if (!mask) {
		ColorDlg[ID_ST_CHECKBOX_FOREGROUND].Flags |= (DIF_HIDDEN | DIF_DISABLE);
		ColorDlg[ID_ST_CHECKBOX_BACKGROUND].Flags |= (DIF_HIDDEN | DIF_DISABLE);
		ColorDlg[ID_ST_TEXT_FOREGROUND].Flags ^= DIF_HIDDEN;
		ColorDlg[ID_ST_TEXT_BACKGROUND].Flags ^= DIF_HIDDEN;
	}

	Dialog Dlg(ColorDlg, ARRAYSIZE(ColorDlg), GetColorDlgProc, (LONG_PTR)&colorState);

	int dialogsizex = 50 + extrasize;
	int dialogsizey = 20;

	if (!bCentered && ScrX >= 37 + dialogsizex - 1 && ScrY >= 2 + dialogsizey - 1)
		Dlg.SetPosition(37, 2, 37 + dialogsizex - 1, 2 + dialogsizey - 1);
	else
		Dlg.SetPosition(-1, -1, dialogsizex, dialogsizey);

	for (size_t i = ID_ST_FG_COLORS_RECT; i <= ID_ST_FRGB_PREFIX; i++)
		Dlg.SetAutomation(ID_ST_CHECKBOX_FOREGROUND, i, DIF_HIDDEN, DIF_NONE, DIF_NONE, DIF_HIDDEN);

	for (size_t i = ID_ST_BG_COLORS_RECT; i <= ID_ST_BRGB_PREFIX; i++)
		Dlg.SetAutomation(ID_ST_CHECKBOX_BACKGROUND, i, DIF_HIDDEN, DIF_NONE, DIF_NONE, DIF_HIDDEN);

	for (size_t i = ID_ST_EDIT_FORERGB; i <= ID_ST_FRGB_PREFIX; i++)
		Dlg.SetAutomation(ID_ST_CHECKBOX_RGB1, i, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);

	for (size_t i = ID_ST_EDIT_BACKRGB; i <= ID_ST_BRGB_PREFIX; i++)
		Dlg.SetAutomation(ID_ST_CHECKBOX_RGB2, i, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);

	for (size_t i = 0; i < ARRAYSIZE(sup_styles); i++) {
		Dlg.SetAutomation(ID_ST_CHECKBOX_STYLE_ENABLE, sup_styles[i].id, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	}

	Dlg.Process();
	int ExitCode = Dlg.GetExitCode();

	if (ExitCode == ID_ST_BUTTON_SETCOLOR) {

		colorState.update_color( );

		*color = colorState.color;
		if (mask) *mask = colorState.mask;

		return true;
	}

	return false;
}

bool GetColorDialogForFileFilter(uint64_t *color, uint64_t *mask)
{
	return GetColorDialogInner(color, mask, true, true, true);
}

bool GetColorDialog(uint64_t *color, bool bCentered)
{
	if (!color) 
		return false;

//	return GetColorDialogInner(color, NULL, true, false, bCentered);
	return GetColorDialogInner(color, NULL, true, true, bCentered);
}

bool GetColorDialog16(uint16_t *color, bool bCentered)
{
	if (!color)
		return false;

	uint64_t color64 = *color;
	bool out = GetColorDialogInner(&color64, NULL, false, false, bCentered);
	*color = (uint16_t)(color64 & 0xFFFF);

	return out;
}
