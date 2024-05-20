/*
pick_colorRGB.cpp

Pick color RGB dialog
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

uint32_t g_tempcolorsRGB[20] = {
		0x48533A, 0x637350, 0x48533A, 0x637350, 0x48533A, 0x637350, 0x48533A, 0x637350, 
		0x48533A, 0x637350, 0x48533A, 0x637350, 0x48533A, 0x637350, 0x48533A, 0x637350, 
		0x48533A, 0x637350, 0x48533A, 0x637350
};

rgbcolor_t HSV_2_RGB(const hsvcolor_t hsv)
{
	rgbcolor_t rgb;
	uint8_t region, remainder, p, q, t;

	if (hsv.s == 0) {

		rgb.r = hsv.v;
		rgb.g = hsv.v;
		rgb.b = hsv.v;
		return rgb;
	}

	region = hsv.h / 43;
	remainder = (hsv.h - (region * 43)) * 6; 

	p = (hsv.v * (255 - hsv.s)) >> 8;
	q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
	t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

	switch (region)
	{
		case 0:
			rgb.r = hsv.v; rgb.g = t; rgb.b = p;
			break;
		case 1:
			rgb.r = q; rgb.g = hsv.v; rgb.b = p;
			break;
		case 2:
			rgb.r = p; rgb.g = hsv.v; rgb.b = t;
			break;
		case 3:
			rgb.r = p; rgb.g = q; rgb.b = hsv.v;
			break;
		case 4:
			rgb.r = t; rgb.g = p; rgb.b = hsv.v;
			break;
		default:
			rgb.r = hsv.v; rgb.g = p; rgb.b = q;
			break;
	}

	return rgb;
}

hsvcolor_t RGB_2_HSV(const rgbcolor_t rgb)
{
	hsvcolor_t hsv;
	uint8_t rgbMin, rgbMax;

	rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
	rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

	hsv.v = rgbMax;
	if (hsv.v == 0) {
		hsv.h = 0;
		hsv.s = 0;
		return hsv;
	}

	hsv.s = 255 * uint32_t(rgbMax - rgbMin) / hsv.v;
	if (hsv.s == 0) {
		hsv.h = 0;
		return hsv;
	}

	if (rgbMax == rgb.r)
		hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
	else if (rgbMax == rgb.g)
		hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
	else
		hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

	return hsv;
}

enum enumSetColorDialog
{
	ID_PCRGB_TITLE = 0,

	ID_PCRGB_SEPARATOR,
	ID_PCRGB_SEPARATOR2,
	ID_PCRGB_SEPARATOR3,

	ID_PCRGB_COLOR_TABLE,
	ID_PCRGB_COLOR_CONTROL1,
	ID_PCRGB_COLOR_CONTROL2,

	ID_PCRGB_RADIO_HUE,
	ID_PCRGB_RADIO_TINT_SHADE,
	ID_PCRGB_RADIO_COLORS,

	ID_PCRGB_BUTTON_TO_TEMP,
	ID_PCRGB_COLOR_TEMP,

	ID_PCRGB_VTEXT_HSV,
	ID_PCRGB_COLOR_HUE,
	ID_PCRGB_COLOR_SAT,
	ID_PCRGB_COLOR_VAL,

	ID_PCRGB_VTEXT_RGB,
	ID_PCRGB_COLOR_RED,
	ID_PCRGB_COLOR_GREEN,
	ID_PCRGB_COLOR_BLUE,

	ID_PCRGB_PREFIX_RGB,
	ID_PCRGB_EDIT_RGB,

	ID_PCRGB_COLOR_SAMPLE,

	ID_PCRGB_BUTTON_OK,
	ID_PCRGB_BUTTON_CANCEL
};

struct pick_colorRGB_s
{
	CHAR_INFO tablevbuff[64 * 12];
	CHAR_INFO control1vbuff[64];
	CHAR_INFO control2vbuff[64];
	CHAR_INFO samplevbuff[32];
	CHAR_INFO hsvrgbvbuff[16 * 8];
	CHAR_INFO tempcolorsvbuff[20 * 2];

	uint32_t index;
	uint32_t tempindex;
	uint32_t mode;

	bool	bTableColorPreview;

	union {
		struct {
			union {
				uint32_t hsv;
				uint8_t hsvbuff[4];
				hsvcolor_t hsvt;
			};
			union {
				uint32_t rgb;
				uint8_t rgbbuff[4];
				rgbcolor_t rgbt;
			};
		};
		uint8_t hsvrgbbuff[8];
	};

	uint32_t hsvrgbfocus;
	wchar_t wsRGB[16];
	wchar_t wsRGBHSV[8 * 8];

	pick_colorRGB_s(uint32_t color = 0)
	{
		hsvrgbfocus = 0xFFFFFFFF;
		bTableColorPreview = false;
		tempindex = 0;
		index = 0;
		rgb = color;
		mode = color ? 0 : 2;

		on_update_hsvrgb(1);
		draw_table_vbuff();
		draw_tempcolors_vbuff();
	}

	void draw_table_vbuff(void);
	void draw_sample_vbuff(void);
	void draw_tempcolors_vbuff(void);
	void draw_hsvrgb_vbuff(void);
	void draw_controls_vbuff(void);

	void on_update_hsvrgb(uint32_t id);
	void update_temp_index(uint32_t newindex);
	void on_mode_change(void);
	void update_table_index(void);
	void draw_table_colorindex(wchar_t ch);

	void change_RGB_focus(uint32_t focus) {
		hsvrgbfocus = focus;
		draw_hsvrgb_vbuff();
	}

	void update_wsRGB( )
	{
		swprintf(wsRGB, 16, L"%06X", RGB_2_BGR(rgb));
	}

	void update_wsRGBHSV( )
	{
		for (size_t i = 0; i < 8; i++) {
			if ((i & 3) == 3) continue;
			swprintf(wsRGBHSV + i * 8, 8, L"%3u", hsvrgbbuff[i]);
		}
	}
};

void pick_colorRGB_s::update_table_index(void)
{
	if (mode < 2) {
		uint32_t y = 12 - (hsvbuff[1 + mode] / 20);
		index = y > 11 ? 11 * 64 + (hsvbuff[0 + mode] >> 2) : y * 64 + (hsvbuff[0 + mode] >> 2);
	}
}

void pick_colorRGB_s::draw_table_colorindex(wchar_t ch)
{
	if (index == 0xFFFFFFFF) return;

	size_t y = index >> 4, x = index & 15;
	tablevbuff[y * 64 * 2 + 0 * 64 + x * 4 + 1].Char.UnicodeChar = ch;
	tablevbuff[y * 64 * 2 + 0 * 64 + x * 4 + 2].Char.UnicodeChar = ch;
	tablevbuff[y * 64 * 2 + 1 * 64 + x * 4 + 1].Char.UnicodeChar = ch;
	tablevbuff[y * 64 * 2 + 1 * 64 + x * 4 + 2].Char.UnicodeChar = ch;
}

void pick_colorRGB_s::on_update_hsvrgb(uint32_t upfl)
{
	if (upfl)
		hsvt = RGB_2_HSV(rgbt);
	else
		rgbt = HSV_2_RGB(hsvt);

	if (mode < 2) {
		if (mode == 1) {
			update_table_index();
			draw_table_vbuff();
		}
		else {
			tablevbuff[index].Char.UnicodeChar = 32;
			update_table_index();
			tablevbuff[index].Char.UnicodeChar = L'\x2022'; // DOT
		}
	}
	else {
		draw_table_colorindex(L'\x2022');
	}

	if (mode != 1 && bTableColorPreview)
		draw_table_vbuff();

	update_wsRGB( );
	update_wsRGBHSV( );
	draw_hsvrgb_vbuff();
	draw_sample_vbuff( );
	draw_controls_vbuff( );
}

void pick_colorRGB_s::on_mode_change(void)
{
	if (mode == 2) {
		index = 0xFFFFFFFF;
	}
	else
		index = 0;

	update_table_index();
	draw_table_vbuff();
	draw_controls_vbuff( );
}

void pick_colorRGB_s::draw_sample_vbuff(void)
{
	for (size_t i = 0; i < 32; i++) {
		samplevbuff[i].Char.UnicodeChar = 32;
		samplevbuff[i].Attributes = ATTR_RGBBACK_NEGF2(rgb);
	}
}

void pick_colorRGB_s::draw_hsvrgb_vbuff(void)
{
	static const wchar_t wscontrol[] = L"[ < ]     [ > ]";

	for (size_t i = 0; i < 8; i++) {
		if ((i & 3) == 3) continue;

		for (size_t g = 0; g < 15; g++) {
			CHAR_INFO *const vbuff = hsvrgbvbuff + 16 * i;

			vbuff[g].Attributes = (i == hsvrgbfocus) ? FarColorToReal(COL_DIALOGSELECTEDBUTTON) : FarColorToReal(COL_DIALOGBUTTON);
			vbuff[g].Char.UnicodeChar = (g >= 6 && g < 9) ? wsRGBHSV[i * 8 + g - 6] : wscontrol[g];
		}
	}
}

void pick_colorRGB_s::update_temp_index(uint32_t newindex)
{
	CHAR_INFO *const vbuff = tempcolorsvbuff;
	newindex %= 20;

	vbuff[tempindex * 2 + 0].Char.UnicodeChar = 32;
	vbuff[tempindex * 2 + 1].Char.UnicodeChar = 32;

	vbuff[newindex * 2 + 0].Char.UnicodeChar = L'\x23F5';
	vbuff[newindex * 2 + 1].Char.UnicodeChar = L'\x23F4';

	tempindex = newindex;
}

void pick_colorRGB_s::draw_tempcolors_vbuff(void)
{
	CHAR_INFO *const vbuff = tempcolorsvbuff;

	for (size_t g = 0; g < 20; g++) {
		const uint64_t attr = ATTR_RGBBACK_NEGF2(g_tempcolorsRGB[g]);

		vbuff[g * 2 + 0].Char.UnicodeChar = 32;
		vbuff[g * 2 + 1].Char.UnicodeChar = 32;
		vbuff[g * 2 + 0].Attributes = attr;
		vbuff[g * 2 + 1].Attributes = attr;
	}

	vbuff[tempindex * 2 + 0].Char.UnicodeChar = L'\x23F5';
	vbuff[tempindex * 2 + 1].Char.UnicodeChar = L'\x23F4';
}

void pick_colorRGB_s::draw_controls_vbuff(void)
{
	union {
		rgbcolor_t rgbt;
		uint64_t rgb;
	};
	hsvcolor_t hsv = hsvt;

	hsv.v = 255;
	(mode & 1) ? (hsv.h = 0, hsv.s = 255) : hsv.s = 0;
	for (size_t i = 0; i < 64; i++) {
		CHAR_INFO *const vbuff = control1vbuff;
		rgbt = HSV_2_RGB(hsv);
		vbuff[i].Attributes = ATTR_RGBBACK_NEGF2(rgb);
		vbuff[i].Char.UnicodeChar = 32;
		(mode & 1) ? hsv.h += 4 : hsv.s += 4;
	}

	uint32_t dotpos = (mode & 1) ? hsvt.h >> 2 : hsvt.s >> 2;
	control1vbuff[dotpos].Char.UnicodeChar = L'\x2022'; // DOT

	hsv = hsvt;
	hsv.v = 0;
	for (size_t i = 0; i < 64; i++) {
		CHAR_INFO *const vbuff = control2vbuff;
		rgbt = HSV_2_RGB(hsv);
		vbuff[i].Attributes = ATTR_RGBBACK_NEGF2(rgb);
		vbuff[i].Char.UnicodeChar = 32;
		hsv.v += 4;
	}

	control2vbuff[hsvt.v >> 2].Char.UnicodeChar = L'\x2022'; // DOT
}

void pick_colorRGB_s::draw_table_vbuff(void)
{
	CHAR_INFO *const vbuff = tablevbuff;
	hsvcolor_t hsv;
	union {
		rgbcolor_t rgbt;
		uint64_t rgb;
	};

	if (bTableColorPreview) {
		for (size_t i = 0; i < 768; i++) {
			vbuff[i].Attributes = ATTR_RGBBACK_NEGF2(this->rgb);
			vbuff[i].Char.UnicodeChar = 32;
		}
		if (mode == 2)
			draw_table_colorindex(L'\x2022');
		else
			vbuff[index].Char.UnicodeChar = L'\x2022'; // DOT

		return;
	}

	if (mode == 2) {
		for (size_t y = 0; y < 6; y++) {
			hsv.s = y < 3 ? (y + 1) * 85 : 255;
			hsv.v = y > 2 ? 255 - (y - 2) * 64 : 255;
			for (size_t x = 0; x < 16; x++) {
				hsv.h = x << 4;
				rgbt = HSV_2_RGB(hsv);
				const uint64_t attr = ATTR_RGBBACK_NEGF2(rgb);
				for (size_t x2 = 0; x2 < 4; x2++)
					for (size_t y2 = 0; y2 < 2; y2++) {
						vbuff[y * 64 * 2 + y2 * 64 + x * 4 + x2].Attributes = attr;
						vbuff[y * 64 * 2 + y2 * 64 + x * 4 + x2].Char.UnicodeChar = 32;
					}
			}
		}
		draw_table_colorindex(L'\x2022');
		return;
	}

	hsv = hsvt;
	hsv.v = 255;
	if (!mode) hsv.s = 255;
	for (size_t y = 0; y < 12; y++) {
		(!mode) ? hsv.h = 0 : hsv.s = 0;
		for (size_t x = 0; x < 64; x++) {
			rgbt = HSV_2_RGB(hsv);
			vbuff[y * 64 + x].Attributes = ATTR_RGBBACK_NEGF2(rgb);
			vbuff[y * 64 + x].Char.UnicodeChar = 32;
			(!mode) ? hsv.h += 4 : hsv.s += 4;
		}
		(!mode) ? hsv.s -= 20 : hsv.v -= 20;
	}

	vbuff[index].Char.UnicodeChar = L'\x2022'; // DOT
}

static LONG_PTR WINAPI PickColorRGBDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	pick_colorRGB_s *colorState = (pick_colorRGB_s *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
	volatile uint32_t &mode = colorState->mode;
	static uint32_t mhon = 0;

	auto update_table_cursor = [=]() {
		COORD coord;
		if (colorState->mode < 2)
			coord = { int16_t(colorState->index & 63), int16_t(colorState->index >> 6) };
		else
			coord = { int16_t(((colorState->index & 15) << 2) + 1), int16_t(((colorState->index >> 4) << 1) + 1)};
		SendDlgMessage(hDlg, DM_SETCURSORPOS, ID_PCRGB_COLOR_TABLE, (LONG_PTR)&coord);
	};

	auto update_controls_cursor = [=]() {
		COORD coord = {(colorState->mode == 1) ? int16_t(colorState->hsvt.h >> 2) : int16_t(colorState->hsvt.s >> 2), 0};
		SendDlgMessage(hDlg, DM_SETCURSORPOS, ID_PCRGB_COLOR_CONTROL1, (LONG_PTR)&coord);
		coord = {int16_t(colorState->hsvt.v >> 2), 0 };
		SendDlgMessage(hDlg, DM_SETCURSORPOS, ID_PCRGB_COLOR_CONTROL2, (LONG_PTR)&coord);
	};

	auto on_update_hsvrgb = [=](uint32_t upid) {
		SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
		colorState->on_update_hsvrgb(upid);
		SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_PCRGB_EDIT_RGB, (LONG_PTR)colorState->wsRGB);
		update_table_cursor();
		update_controls_cursor();
		SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
	};

	auto update_tab_index_and_cursor = [=](int32_t newindex) {
		if (newindex < 0)
			newindex += (mode < 2) ? 768 : 96;
		newindex %= (mode < 2) ? 768 : 96;

		if (mode < 2 ) {
			colorState->hsvbuff[0 + mode] = (newindex & 63) << 2;
			colorState->hsvbuff[1 + mode] = 255 - ((newindex >> 6) * 20);
			on_update_hsvrgb(0);
		}
		else {
			colorState->draw_table_colorindex(32);
			size_t y = newindex >> 4, x = newindex & 15;
			colorState->hsvt.s = y < 3 ? (y + 1) * 85 : 255;
			colorState->hsvt.v = y > 2 ? 255 - (y - 2) * 64 : 255;
			colorState->hsvt.h = x << 4;
			colorState->index = newindex;
			on_update_hsvrgb(0);
		}
	};

	auto update_temp_index_and_cursor = [=](int32_t newindex, const bool bRedraw) {
		if (newindex < 0)
			newindex += 20;
		newindex %= 20;
		COORD coord = { int16_t(newindex * 2), 0 };
		SendDlgMessage(hDlg, DM_SETCURSORPOS, ID_PCRGB_COLOR_TEMP, (LONG_PTR)&coord);
		colorState->update_temp_index(newindex);
		if (bRedraw)
			SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
	};

	auto set_temp_color = [=](const bool bRedraw) {
		g_tempcolorsRGB[colorState->tempindex] = colorState->rgb;
		colorState->draw_tempcolors_vbuff();
		if (bRedraw)
			SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
	};

	auto on_mode_change = [=]() {
		SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
		colorState->on_mode_change();
		update_table_cursor();
		update_controls_cursor();
		SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
	};

	switch (Msg) {

	case DN_INITDIALOG: {
		SendDlgMessage(hDlg, DM_SETMOUSEEVENTNOTIFY, TRUE, 0);
		SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID_PCRGB_COLOR_TABLE, Opt.CursorSize[1]);
		SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID_PCRGB_COLOR_CONTROL1, Opt.CursorSize[1]);
		SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID_PCRGB_COLOR_CONTROL2, Opt.CursorSize[1]);
		SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID_PCRGB_COLOR_TEMP, Opt.CursorSize[1]);
		SendDlgMessage(hDlg, DM_SETFOCUS, ID_PCRGB_COLOR_TABLE, 0);
		update_table_cursor();
		update_controls_cursor();
	}
	break;

	case DN_CTLCOLORDLGITEM:
		if (Param1 == ID_PCRGB_PREFIX_RGB) {
			uint64_t *ItemColor = (uint64_t *)Param2;
			ItemColor[0] = FarColorToReal(COL_DIALOGEDIT);
			return 1;
		}
	break;

	case DN_KILLFOCUS:
		if (Param1 >= ID_PCRGB_COLOR_HUE && Param1 <= ID_PCRGB_COLOR_BLUE) {
			colorState->change_RGB_focus(0xFFFFFFFF);
			break;
		}
		if (Param1 == ID_PCRGB_EDIT_RGB) {
			SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_PCRGB_EDIT_RGB, (LONG_PTR)colorState->wsRGB);
			break;
		}
	break;

	case DN_GOTFOCUS:
		if (Param1 >= ID_PCRGB_COLOR_HUE && Param1 <= ID_PCRGB_COLOR_BLUE) {
			colorState->change_RGB_focus(Param1 - ID_PCRGB_COLOR_HUE);
			break;
		}
	break;

	case DN_MOUSEEVENT: {
		MOUSE_EVENT_RECORD *mEv = (MOUSE_EVENT_RECORD *)Param2;
		const auto &mouseX = mEv->dwMousePosition.X;
		const auto &mouseY = mEv->dwMousePosition.Y;
		const auto &mouseB = mEv->dwButtonState;
		union { SMALL_RECT drect; uint64_t i64drect; };
		union { SMALL_RECT irect; uint64_t i64irect; };
		SendDlgMessage(hDlg, DM_GETDLGRECT, 0, (LONG_PTR)&drect);
		drect.Right = drect.Left;
		drect.Bottom = drect.Top;

		SendDlgMessage(hDlg, DM_GETITEMPOSITION, ID_PCRGB_COLOR_TABLE, (LONG_PTR)&irect);
		i64irect += i64drect;

		if (!mouseB)
			mhon = 0;

		if (mouseX >= irect.Left && mouseX <= irect.Right)
		{
			if (mouseY >= irect.Top && mouseY <= irect.Bottom)
			{
				if (mouseB == 1 && (!mhon || mhon == 0x999))
				{
					if (mode < 2)
						update_tab_index_and_cursor( ((mouseY - irect.Top) << 6) + (mouseX - irect.Left) );
					else
						update_tab_index_and_cursor( (((mouseY - irect.Top) >> 1) << 4) + ((mouseX - irect.Left) >> 2) );

					mhon = 0x999;
				}
				break;
			}

			irect.Top = irect.Bottom + 2;
			irect.Bottom = irect.Top + 1;

			if (mouseY >= irect.Top && mouseY <= irect.Bottom) {
				uint32_t id = (mouseY - irect.Top) + 1;
				if (mouseB == 1 && (!mhon || mhon == id)) {
					uint8_t val = (mouseX - irect.Left) << 2;
					if (val >= 252) val = 255;
					colorState->hsvbuff[(mode & 1) ? id & 2 : id] = val;
					on_update_hsvrgb(0);
					mhon = id;
				}
				break;
			}
		}

	}
	break;

	case DN_BTNCLICK:
		if (Param1 >= ID_PCRGB_RADIO_HUE && Param1 <= ID_PCRGB_RADIO_COLORS) {
			colorState->mode = Param1 - ID_PCRGB_RADIO_HUE;
			on_mode_change();
			break;
		}

		if (Param1 == ID_PCRGB_BUTTON_TO_TEMP) {
			set_temp_color(true);
			break;
		}
	break;

	case DN_EDITCHANGE:
		if (Param1 == ID_PCRGB_EDIT_RGB) {
			SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_PCRGB_EDIT_RGB, (LONG_PTR)colorState->wsRGB);
			uint32_t newrgb = RGB_2_BGR(wcstoul(colorState->wsRGB, nullptr, 16));
			if (newrgb != colorState->rgb) {
				colorState->rgb = newrgb;
				colorState->on_update_hsvrgb(1);
				update_table_cursor();
				update_controls_cursor();
				SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			}
		}
	break;

	case DN_MOUSECLICK: {
		const MOUSE_EVENT_RECORD *const mEv = (MOUSE_EVENT_RECORD *)Param2;
		const auto &mouseX = mEv->dwMousePosition.X;
		const auto &mouseB = mEv->dwButtonState;

		if (Param1 == ID_PCRGB_COLOR_SAMPLE && mouseB == FROM_LEFT_1ST_BUTTON_PRESSED) {
			colorState->bTableColorPreview ^= true;
			colorState->draw_table_vbuff();
			SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			break;
		}

		if (Param1 == ID_PCRGB_COLOR_TABLE) {
			if (mouseB == FROM_LEFT_1ST_BUTTON_PRESSED && mEv->dwEventFlags == DOUBLE_CLICK) {
				set_temp_color(true);
				break;
			}
			if (mouseB == RIGHTMOST_BUTTON_PRESSED) {
				colorState->bTableColorPreview ^= true;
				colorState->draw_table_vbuff();
				SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
				break;
			}
		}

		if (Param1 == ID_PCRGB_COLOR_TEMP && mouseB == FROM_LEFT_1ST_BUTTON_PRESSED) {
			if (mEv->dwEventFlags == DOUBLE_CLICK) {
				colorState->rgb = g_tempcolorsRGB[colorState->tempindex];
				on_update_hsvrgb(1);
			}
			else
				update_temp_index_and_cursor(mouseX >> 1, true);
			break;
		}

		if (Param1 >= ID_PCRGB_COLOR_HUE && Param1 <= ID_PCRGB_COLOR_BLUE) {
			const uint32_t id = Param1 - ID_PCRGB_COLOR_HUE;

			if (mouseX < 5) {
				if (colorState->hsvrgbbuff[id] <= 0)
					break;
				colorState->hsvrgbbuff[id] --;
				on_update_hsvrgb(uint32_t(id > 3));
			}
			else if (mouseX > 9) {
				if (colorState->hsvrgbbuff[id] >= 255)
					break;
				colorState->hsvrgbbuff[id] ++;
				on_update_hsvrgb(uint32_t(id > 3));
			}
			break;
		}

	}
	break;

	case DN_KEY: {
		if (Param1 >= ID_PCRGB_COLOR_CONTROL1 && Param1 <= ID_PCRGB_COLOR_CONTROL2) {
			uint32_t id = Param1 - ID_PCRGB_COLOR_CONTROL1 + 1;
			if (mode & 1) id &= 2;
			switch(Param2) {
			case KEY_DOWN:
			case KEY_NUMPAD2:
			case KEY_MSWHEEL_DOWN:
			case KEY_PGDN:
			case KEY_NUMPAD3:
			case KEY_LEFT:
			case KEY_NUMPAD4:
			case KEY_MSWHEEL_LEFT: {
				if (colorState->hsvbuff[id] < 4)
					colorState->hsvbuff[id] = 0;
				else
					colorState->hsvbuff[id] -= 4;
				on_update_hsvrgb(0);
			}
			break;
			case KEY_UP:
			case KEY_NUMPAD8:
			case KEY_MSWHEEL_UP:
			case KEY_PGUP:
			case KEY_NUMPAD9:
			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_MSWHEEL_RIGHT: {
				if (colorState->hsvbuff[id] > 251)
					colorState->hsvbuff[id] = 255;
				else
					colorState->hsvbuff[id] += 4;
				on_update_hsvrgb(0);
			}
			break;
			}
			break;
		}

		if (Param1 >= ID_PCRGB_COLOR_HUE && Param1 <= ID_PCRGB_COLOR_BLUE) {
			const uint32_t id = Param1 - ID_PCRGB_COLOR_HUE;
			switch(Param2) {
			case KEY_LEFT:
			case KEY_NUMPAD4:
			case KEY_MSWHEEL_LEFT: {
				if (colorState->hsvrgbbuff[id] <= 0)
					break;
				colorState->hsvrgbbuff[id] --;
				on_update_hsvrgb(uint32_t(id > 3));
			}
			break;
			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_MSWHEEL_RIGHT: {
				if (colorState->hsvrgbbuff[id] >= 255)
					break;
				colorState->hsvrgbbuff[id] ++;
				on_update_hsvrgb(uint32_t(id > 3));
			}
			break;
			case KEY_UP:
			case KEY_NUMPAD8:
			case KEY_MSWHEEL_UP:
			case KEY_PGUP:
			case KEY_NUMPAD9:
				if (Param1 == ID_PCRGB_COLOR_RED)
					SendDlgMessage(hDlg, DM_SETFOCUS, Param1 - 2, 0);
				else
					SendDlgMessage(hDlg, DM_SETFOCUS, Param1 - 1, 0);
			break;
			case KEY_DOWN:
			case KEY_NUMPAD2:
			case KEY_MSWHEEL_DOWN:
			case KEY_PGDN:
			case KEY_NUMPAD3:
				if (Param1 == ID_PCRGB_COLOR_VAL)
					SendDlgMessage(hDlg, DM_SETFOCUS, Param1 + 2, 0);
				else
					SendDlgMessage(hDlg, DM_SETFOCUS, Param1 + 1, 0);
			break;
			}
			break;
		}

		if (Param1 == ID_PCRGB_COLOR_TEMP) {
			uint32_t &index = colorState->tempindex;
			switch(Param2) {
			case KEY_UP:
			case KEY_NUMPAD8:
			case KEY_MSWHEEL_UP:
			case KEY_PGUP:
			case KEY_NUMPAD9:
			case KEY_LEFT:
			case KEY_NUMPAD4:
			case KEY_MSWHEEL_LEFT:
				update_temp_index_and_cursor(index - 1, true);
			break;
			case KEY_DOWN:
			case KEY_NUMPAD2:
			case KEY_MSWHEEL_DOWN:
			case KEY_PGDN:
			case KEY_NUMPAD3:
			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_MSWHEEL_RIGHT:
				update_temp_index_and_cursor(index + 1, true);
			break;
			case KEY_HOME:
			case KEY_NUMPAD7:
				update_temp_index_and_cursor(0, true);
			break;
			case KEY_END:
			case KEY_NUMPAD1:
				update_temp_index_and_cursor(19, true);
			break;
			case KEY_SPACE:
				colorState->rgb = g_tempcolorsRGB[index];
				on_update_hsvrgb(1);
			break;
			}
			break;
		}

		if (Param1 == ID_PCRGB_COLOR_TABLE) {
			uint32_t &index = colorState->index;
			switch(Param2) {
			case KEY_LEFT:
			case KEY_NUMPAD4:
			case KEY_MSWHEEL_LEFT:
				update_tab_index_and_cursor(index - 1);
			break;
			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_MSWHEEL_RIGHT:
				update_tab_index_and_cursor(index + 1);
			break;
			case KEY_UP:
			case KEY_NUMPAD8:
			case KEY_MSWHEEL_UP:
			case KEY_PGUP:
			case KEY_NUMPAD9:
				update_tab_index_and_cursor(mode < 2 ? index - 64 : index - 16);
			break;
			case KEY_DOWN:
			case KEY_NUMPAD2:
			case KEY_MSWHEEL_DOWN:
			case KEY_PGDN:
			case KEY_NUMPAD3:
				update_tab_index_and_cursor(mode < 2 ? index + 64 : index + 16);
			break;
			case KEY_HOME:
			case KEY_NUMPAD7:
				update_tab_index_and_cursor(0);
			break;
			case KEY_END:
			case KEY_NUMPAD1:
					update_tab_index_and_cursor(mode < 2 ? 768 - 1 : 96 - 1);
			break;
			}
			break;
		}
	break;
	}

	} // switch


	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool PickColorRGBDialogInner(uint32_t *color, bool bCentered)
{
	static const wchar_t *HexMask = L"HHHHHH";

	if (!color) return false;
	pick_colorRGB_s colorState(*color);

	DialogDataEx ColorRGBDlgData[] = {

		{DI_DOUBLEBOX, 3, 1, 70, 24, {}, 0, Msg::SetColorTitle},
		{DI_TEXT,  0, 14, 0, 14, {}, DIF_SEPARATOR, L""},
		{DI_TEXT,  0, 17, 0, 17, {}, DIF_SEPARATOR, L""},
		{DI_TEXT,  0, 22, 0, 22, {}, DIF_SEPARATOR, L""},

		{DI_USERCONTROL, 5, 2, 5 + 64 - 1, 2 + 11, {}, 0, L"" },
		{DI_USERCONTROL, 5, 15, 5 + 64 - 1, 15, {}, 0, L"" },
		{DI_USERCONTROL, 5, 16, 5 + 64 - 1, 16, {}, 0, L"" },

		{DI_RADIOBUTTON,  5, 18, 5 + 15, 18, {}, DIF_GROUP, Msg::PickColorHue},
		{DI_RADIOBUTTON,  5, 19, 5 + 15, 19, {}, 0, Msg::PickColorSat},
		{DI_RADIOBUTTON,  5, 20, 5 + 15, 20, {}, 0, Msg::PickColorColors},

		{DI_BUTTON, 5, 21, 5+5, 21, {}, DIF_BTNNOCLOSE, L"+"},
		{DI_USERCONTROL, 11, 21, 11 + 20 * 2 - 1, 21, {}, 0, L"" },

		{DI_VTEXT,  26, 18, 44, 20, {}, 0, L"&HSV"},
		{DI_USERCONTROL, 27, 18, 27 + 15 - 1, 18, {}, 0, L"" }, // hsv
		{DI_USERCONTROL, 27, 19, 27 + 15 - 1, 19, {}, 0, L"" },
		{DI_USERCONTROL, 27, 20, 27 + 15 - 1, 20, {}, 0, L"" },

		{DI_VTEXT,  44, 18, 44, 20, {}, 0, L"&RGB"},
		{DI_USERCONTROL, 45, 18, 45 + 15 - 1, 18, {}, 0, L"" }, // rgb
		{DI_USERCONTROL, 45, 19, 45 + 15 - 1, 19, {}, 0, L"" },
		{DI_USERCONTROL, 45, 20, 45 + 15 - 1, 20, {}, 0, L"" },

		{DI_TEXT, 52, 21, 52, 21, {}, 0, L"#"},
		{DI_FIXEDIT, 53, 21, 53 + 6, 21, {(DWORD_PTR)HexMask}, DIF_MASKEDIT, colorState.wsRGB},

		{DI_USERCONTROL, 61, 18, 61 + 8 - 1, 18 + 4 - 1, {}, DIF_NOFOCUS, L"" },

		{DI_BUTTON, 0, 23, 0, 23, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::Ok},
		{DI_BUTTON, 0, 23, 0, 23, {}, DIF_CENTERGROUP, Msg::Cancel},
	};

	MakeDialogItemsEx(ColorRGBDlgData, ColorDlg);

	ColorDlg[ID_PCRGB_COLOR_TABLE].VBuf = colorState.tablevbuff;
	ColorDlg[ID_PCRGB_COLOR_CONTROL1].VBuf = colorState.control1vbuff;
	ColorDlg[ID_PCRGB_COLOR_CONTROL2].VBuf = colorState.control2vbuff;

	ColorDlg[ID_PCRGB_COLOR_SAMPLE].VBuf = colorState.samplevbuff;
	ColorDlg[ID_PCRGB_COLOR_TEMP].VBuf = colorState.tempcolorsvbuff;

	if (!*color)
		ColorDlg[ID_PCRGB_RADIO_COLORS].Selected = 1;
	else
		ColorDlg[ID_PCRGB_RADIO_HUE].Selected = 1;

	for (size_t i = 0; i < 8; i ++) {
		if ((i & 3) == 3) continue;
		ColorDlg[ID_PCRGB_COLOR_HUE + i].VBuf = colorState.hsvrgbvbuff + i * 16;
	}

	Dialog Dlg(ColorDlg, ARRAYSIZE(ColorDlg), PickColorRGBDlgProc, (LONG_PTR)&colorState);

	int dialog_sizex = 74;
	int dialog_sizey = 26;
	int dialog_posx = 0;
	int dialog_posy = 0;

	if (ScrX <= dialog_sizex)
		dialog_posx = 0;
	else
		dialog_posx = (ScrX - dialog_sizex) >> 1;

	if (ScrY <= dialog_sizey)
		dialog_posy = 0;
	else
		dialog_posy = (ScrY - dialog_sizey) >> 1;

	if (!bCentered) {
		if (dialog_posy > 7 )
			dialog_posy -= 7;
		else
			dialog_posy = 0;
	}

	Dlg.SetPosition(dialog_posx, dialog_posy, dialog_posx + dialog_sizex - 1, dialog_posy + dialog_sizey - 1);

	Dlg.Process();
	int ExitCode = Dlg.GetExitCode();

	if (ExitCode == ID_PCRGB_BUTTON_OK) {

		*color = colorState.rgb;
		return true;
	}

	return false;
}

bool GetPickColorDialogRGB(uint32_t *color, bool bCentered)
{
	if (!color) return false;

	return PickColorRGBDialogInner(color, bCentered);
}
