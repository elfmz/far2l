/*
keybar.cpp

Keybar
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

#include "keybar.hpp"
#include "colors.hpp"
#include "keyboard.hpp"
#include "keys.hpp"
#include "manager.hpp"
#include "syslog.hpp"
#include "lang.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "ConfigRW.hpp"
#include "farcolors.hpp"

KeyBar::KeyBar()
	:
	Owner(nullptr), AltState(0), CtrlState(0), ShiftState(0), DisableMask(0), RegReaded(FALSE)
{
	_OT(SysLog(L"[%p] KeyBar::KeyBar()", this));
	memset(KeyTitles, 0, sizeof(KeyTitles));
	memset(KeyCounts, 0, sizeof(KeyCounts));
	memset(RegKeyTitles, 0, sizeof(RegKeyTitles));

	for (int i = 0; i < KEY_COUNT; i++) Hover[i] = 0;
	for (int i = 0; i < KEY_COUNT; i++) xPos[i] = -1;
}

void KeyBar::SetOwner(ScreenObject *Owner)
{
	KeyBar::Owner = Owner;
}

static std::vector<std::wstring> PrevFKeyTitles;

void KeyBar::Refresh(bool show, bool force_refresh_fkeys)
{
	if (force_refresh_fkeys)
		PrevFKeyTitles.clear();

	if (show) {
		Show();
	} else {
		if (IsVisible()) {
			Hide();
		}
		RefreshObject(false);
	}
}

void KeyBar::DisplayObject()
{
	RefreshObject(true);
}

int KeyBar::GetGroup(int alt, int shift, int ctrl, int meta) {
	if (alt && shift && ctrl){ 
		if (!(Opt.CASRule & 1) || !(Opt.CASRule & 2))
			return KBL_CTRLALTSHIFT;
		return KBL_MAIN;
	}

	if (alt && !shift && ctrl) return KBL_CTRLALT;
	if (alt && shift && !ctrl) return KBL_ALTSHIFT;
	if (!alt && shift && ctrl) return KBL_CTRLSHIFT;

	if (alt && !shift && !ctrl) return KBL_ALT;
	if (!alt && !shift && ctrl) return KBL_CTRL;
	if (!alt && shift && !ctrl) return KBL_SHIFT;

	return KBL_MAIN;
}

std::wstring KeyBar::GetKeyName(int idx, int group) 
{
	static const wchar_t* prefixes[] = {
		L"",
		L"Shift+",
		L"^",
		L"Alt+",
		L"^Shift+",
		L"Alt+Shift+",
		L"^Alt+",
		L"Shift+",
		L"^Alt+Shift+",
	};

	return std::wstring(prefixes[group]) + std::wstring(L"F") + std::to_wstring(idx + 1);
}

void KeyBar::RefreshObject(bool Render)
{
	if (Render)
		GotoXY(X1, Y1);

	AltState = AltPressed && (!CtrlPressed || !ShiftPressed); // C-A-S is a specific case
	CtrlState = CtrlPressed;
	ShiftState = ShiftPressed;

	int group = GetGroup(AltPressed, ShiftPressed, CtrlPressed, 0);

	int KeyWidth = (X2 - X1 - 1) / 12;
	if (KeyWidth < 8) KeyWidth = 8;
	int LabelWidth = KeyWidth - 2;

	bool FKeyTitlesChanged = false;
	for (int i = 0; i < KEY_COUNT; i++) {
		const wchar_t *Label = KeyTitles[group][i];

		xPos[i] = -1;
		if (Opt.Backend.UseModernLook && (!Label || !*Label)) continue;

		std::wstring keyLabel = Label && *Label && Opt.Backend.UseModernLook ? GetKeyName(i, group) : L"";

		if (i >= (int)PrevFKeyTitles.size() || PrevFKeyTitles[i] != Label) {
			FKeyTitlesChanged = true;
			if (i >= (int)PrevFKeyTitles.size()) {
				PrevFKeyTitles.emplace_back(Label);
			} else
				PrevFKeyTitles[i] = Label;
		}

		if (Opt.Backend.UseModernLook) 
			LabelWidth = wcslen(Label) + 3 + wcslen(keyLabel.c_str());

		if (Render && WhereX() + LabelWidth < X2) {
			xPos[i] = WhereX();
			if (!Opt.Backend.UseModernLook) {
				SetFarColor(COL_KEYBARNUM);
				FS << i + 1;

				SetFarColor(COL_KEYBARTEXT);
				FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(LabelWidth) << Label;

				if (i < KEY_COUNT - 1) {
					SetFarColor(COL_KEYBARBACKGROUND);
					Text(L" ");
				}
			}
			else {
				uint64_t color1 = SoftenItemColor(FarColorToReal(COL_KEYBARNUM), 0, Hover[i], 0, 0);
				uint64_t color2 = SoftenItemColor(FarColorToReal(COL_KEYBARTEXT), 0, Hover[i], 0, 0);

				SetColor(color1);
				FS << keyLabel.c_str();
				SetColor(color2);
				FS << L"┋ ";
				FS << Label;
				FS << L" ";
			}
			xPos[i + 1] = WhereX();
		}
	}

	if (Render) {
		int Width = X2 - WhereX() + 1;
		if (!Opt.Backend.UseModernLook) {
    		if (Width > 0) {
    			SetFarColor(COL_KEYBARTEXT);
    			FS << fmt::Cells() << fmt::Expand(Width) << L"";
    		}
		}
		else {
    		strExtra = L" Additional text here to see it if space available";
    		if (Width > 0) {
    			SetFarColor(COL_KEYBARTEXT);
    			FS << L"┋ ";
    			FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(Width - 3) << strExtra << L"";
    		}
		}
	}

	Hint(X1, Y1, X2, Y2, HintKeyBar, HintObjectNone);

	if (FKeyTitlesChanged) {
		std::string str_titles[CONSOLE_FKEYS_COUNT];
		const char *titles[ARRAYSIZE(str_titles)];
		for (int i = 0; i < (int)ARRAYSIZE(str_titles); ++i) {
			if (i < (int)PrevFKeyTitles.size()) {
				StrWide2MB(PrevFKeyTitles[i], str_titles[i]);
				titles[i] = str_titles[i].c_str();
			} else {
				titles[i] = NULL;
			}
		}

		WINPORT(SetConsoleFKeyTitles)(NULL, titles);
	}
}

void KeyBar::ReadRegGroup(const wchar_t *RegGroup, const wchar_t *Language)
{
	if (!RegReaded || StrCmpI(strLanguage, Language) || StrCmpI(strRegGroupName, RegGroup)) {
		memset(RegKeyTitles, 0, sizeof(RegKeyTitles));
		strLanguage = Language;
		strRegGroupName = RegGroup;

		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt("KeyBarLabels/%ls/%ls", strLanguage.CPtr(), strRegGroupName.CPtr());
		const auto &ValueNames = cfg_reader.EnumKeys();
		for (const auto &strValueName : ValueNames) {
			FARString strValue = cfg_reader.GetString(strValueName);
			DWORD Key = KeyNameToKey(StrMB2Wide(strValueName).c_str());
			DWORD Key0 = Key & (~KEY_CTRLMASK);
			DWORD Ctrl = Key & KEY_CTRLMASK;

			if (Key0 >= KEY_F1 && Key0 <= KEY_F24) {
				size_t J;
				static DWORD Area[][2] = {
						{KBL_MAIN,         0                             },
						{KBL_SHIFT,        KEY_SHIFT                     },
						{KBL_CTRL,         KEY_CTRL                      },
						{KBL_ALT,          KEY_ALT                       },
						{KBL_CTRLSHIFT,    KEY_CTRL | KEY_SHIFT          },
						{KBL_ALTSHIFT,     KEY_ALT | KEY_SHIFT           },
						{KBL_CTRLALT,      KEY_CTRL | KEY_ALT            },
						{KBL_CTRLALTSHIFT, KEY_CTRL | KEY_ALT | KEY_SHIFT},
				};

				for (J = 0; J < ARRAYSIZE(Area); ++J)
					if (Area[J][1] == Ctrl)
						break;

				if (J <= ARRAYSIZE(Area)) {
					Key0-= KEY_F1;
					int Group = Area[J][0];
					far_wcsncpy(RegKeyTitles[Group][Key0], strValue, ARRAYSIZE(KeyTitles[Group][Key0]));
				}
			}
		}

		RegReaded = TRUE;
	}
}

void KeyBar::SetRegGroup(int Group)
{
	for (int I = 0; I < KEY_COUNT; I++)
		if (*RegKeyTitles[Group][I])
			far_wcsncpy(KeyTitles[Group][I], RegKeyTitles[Group][I], ARRAYSIZE(KeyTitles[Group][I]));
}

void KeyBar::SetAllRegGroup()
{
	for (int I = 0; I < KBL_GROUP_COUNT; ++I)
		SetRegGroup(I);
}

void KeyBar::SetGroup(int Group, const wchar_t *const *Key, int KeyCount)
{
	if (!Key)
		return;

	for (int i = 0; i < KeyCount && i < KEY_COUNT; i++)
		if (Key[i])
			far_wcsncpy(KeyTitles[Group][i], Key[i], ARRAYSIZE(KeyTitles[Group][i]));

	KeyCounts[Group] = KeyCount;
}

void KeyBar::ClearGroup(int Group)
{
	memset(KeyTitles[Group], 0, sizeof(KeyTitles[Group]));
	KeyCounts[Group] = 0;
}

// Изменение любого Label
void KeyBar::Change(int Group, const wchar_t *NewStr, int Pos)
{
	if (NewStr)
		far_wcsncpy(KeyTitles[Group][Pos], NewStr, ARRAYSIZE(KeyTitles[Group][Pos]));
}

// Групповая установка идущих подряд строк LNG для указанной группы
void KeyBar::SetAllGroup(int Group, FarLangMsg BaseMsg, int Count)
{
	if (Count > KEY_COUNT)
		Count = KEY_COUNT;

	for (int i = 0; i < Count; i++)
		far_wcsncpy(KeyTitles[Group][i], (BaseMsg + i).CPtr(), ARRAYSIZE(KeyTitles[Group][i]));

	KeyCounts[Group] = Count;
}

int KeyBar::ProcessKey(FarKey Key)
{
	switch (Key) {
		case KEY_KILLFOCUS:
		case KEY_GOTFOCUS:
			RedrawIfChanged();
			return TRUE;
	}	/* switch */

	return FALSE;
}

int KeyBar::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	INPUT_RECORD rec;
	FarKey Key;

	for (int i = 0; i < KEY_COUNT; i++) Hover[i] = 0;

	if (!IsVisible())
		return FALSE;

	int MsX = MouseEvent->dwMousePosition.X;

	if (MsX < X1 || MsX > X2 || MouseEvent->dwMousePosition.Y != Y1)
		return FALSE;

	// Hover effect processing
	if (Opt.Backend.UseModernLook && MouseEvent->dwEventFlags == MOUSE_MOVED) {
		int i, j;

		for (i = 0; i < KEY_COUNT; i++) {
			if (xPos[i] < 0) continue;
			for(j = i + 1; j < KEY_COUNT && xPos[j] < 0; ++j);

			if (xPos[i] <= MsX && xPos[j] >= MsX ) {
				Hover[i] = 1;
				Redraw();
				return TRUE;
			}
		}
	}

	if (!(MouseEvent->dwButtonState & 3) || MouseEvent->dwEventFlags)
		return FALSE;

	// Now click
	if (Opt.Backend.UseModernLook) {
		int i, j;
		for (i = 0; i < KEY_COUNT; i++) {
			if (xPos[i] < 0) continue;
			for(j = i + 1; j < KEY_COUNT && xPos[j] < 0; ++j);
			if (xPos[i] <= MsX && xPos[j] >= MsX ) {
				// i is our position, fire the key
				Key = i;

                if (MouseEvent->dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
                		|| (MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED)) {
                	if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
                		Key += KEY_ALTSHIFTF1;
                	else if (MouseEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
                		Key += KEY_CTRLALTF1;
                	else
                		Key += KEY_ALTF1;
                } 
                else if (MouseEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) {
                	if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
                		Key += KEY_CTRLSHIFTF1;
                	else
                		Key += KEY_CTRLF1;
                } 
                else if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
                	Key += KEY_SHIFTF1;
                else
                	Key += KEY_F1;

                FrameManager->ProcessKey(Key);
                return TRUE;
			}
		}
		return FALSE;
	}

	int KeyWidth = (X2 - X1 - 1) / 12;

	if (KeyWidth < 8)
		KeyWidth = 8;

	int X = MsX - X1;

	if (X < KeyWidth * 9)
		Key = X / KeyWidth;
	else
		Key = 9 + (X - KeyWidth * 9) / (KeyWidth + 1);

	for (;;) {
		GetInputRecord(&rec);

		if (rec.EventType == MOUSE_EVENT && !(rec.Event.MouseEvent.dwButtonState & 3))
			break;
	}

	if (rec.Event.MouseEvent.dwMousePosition.X < X1 || rec.Event.MouseEvent.dwMousePosition.X > X2
			|| rec.Event.MouseEvent.dwMousePosition.Y != Y1)
		return FALSE;

	int NewX = MouseEvent->dwMousePosition.X - X1;

	FarKey NewKey = (NewX < KeyWidth * 9)
		? NewX / KeyWidth
		: 9 + (NewX - KeyWidth * 9) / (KeyWidth + 1);

	if (Key != NewKey)
		return FALSE;

	if (Key > 11)
		Key = 11;

	if (MouseEvent->dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
			|| (MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED)) {
		if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
			Key+= KEY_ALTSHIFTF1;
		else if (MouseEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
			Key+= KEY_CTRLALTF1;
		else
			Key+= KEY_ALTF1;
	} else if (MouseEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) {
		if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
			Key+= KEY_CTRLSHIFTF1;
		else
			Key+= KEY_CTRLF1;
	} else if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
		Key+= KEY_SHIFTF1;
	else
		Key+= KEY_F1;

	// if (Owner)
	// Owner->ProcessKey(Key);
	FrameManager->ProcessKey(Key);
	return TRUE;
}

void KeyBar::RedrawIfChanged()
{
	if (ShiftPressed != ShiftState || CtrlPressed != CtrlState || AltPressed != AltState) {
		//_SVS("KeyBar::RedrawIfChanged()");
		Redraw();
	}
}

void KeyBar::SetDisableMask(int Mask)
{
	DisableMask = Mask;
}

void KeyBar::ResizeConsole() {}
