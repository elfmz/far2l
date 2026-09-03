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

#include "vmenu.hpp"

// macos has specific characters ⌥ ⌘ ^ ⇧ 
#ifdef APPLE
#define MAC_CHARS	1
#else
#define MAC_CHARS	0
#endif

static DWORD KeyGroupMaps[][2] = {
		{KBL_MAIN,         0                             },
		{KBL_SHIFT,        KEY_SHIFT                     },
		{KBL_CTRL,         KEY_CTRL                      },
		{KBL_ALT,          KEY_ALT                       },
		{KBL_CTRLSHIFT,    KEY_CTRL | KEY_SHIFT          },
		{KBL_ALTSHIFT,     KEY_ALT | KEY_SHIFT           },
		{KBL_CTRLALT,      KEY_CTRL | KEY_ALT            },
		{KBL_CTRLALTSHIFT, KEY_CTRL | KEY_ALT | KEY_SHIFT},
};

static const wchar_t* prefixes[] = {
#if MAC_CHARS
	L"",
	L"⇧",    // L"Shift+",
	L"^",    // L"Ctrl+", // or ⎈
	L"⌥",    // L"Alt+",
	L"^⇧",   // L"Ctrl+Shift+",
	L"⌥⇧",   // L"Alt+Shift+",
	L"^⌥",   // L"Ctrl+Alt+",
	L"^⌥⇧",  // L"Ctrl+Alt+Shift+",
#else
	L"",
	L"Shift+",
	L"Ctrl+",
	L"Alt+",
	L"Ctrl+Shift+",
	L"Alt+Shift+",
	L"Ctrl+Alt+",
	L"Ctrl+Alt+Shift+",
#endif
};

KeyBar::KeyBar()
	:
	Owner(nullptr) /*, AltState(0), CtrlState(0), ShiftState(0), DisableMask(0),  RegReaded(FALSE) */
{
	_OT(SysLog(L"[%p] KeyBar::KeyBar()", this));

	for(int i = 0; i < KBL_GROUP_COUNT; ++i) {
		KeyBarPlane r;
		r.groupName = prefixes[i];
		r.groupType = i;
		groups.push_back(r);
	}

	SandwichHover = 0;
}

void KeyBar::PushKeyBarAndExposeEmptyNew() {
	KeyBarStackedPlane r;
	r.groups = groups;
	stacked.push_back(r);
	for(int i = 0; i < KBL_GROUP_COUNT; ++i) {
		groups[i].keys.clear();
	}
}

void KeyBar::PopKeyBarBack() {
	KeyBarStackedPlane r = stacked.back();
	groups = r.groups;
	stacked.pop_back();
}

void KeyBar::SetOwner(ScreenObject *Owner){	KeyBar::Owner = Owner; }

void KeyBar::Refresh(bool show, bool force_refresh_fkeys)
{
	if (show) {
		Show();
	} else {
		if (IsVisible()) {
			Hide();
		}
		RefreshObject(false);
	}
}

void KeyBar::DisplayObject(){ RefreshObject(true); }

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
	return groups[group].keys[idx].keyName;
}

FarKey KeyBar::BuildShortcut(int group, int key) {
	static const FarKey prefixes[] = {
		KEY_F1,
		KEY_SHIFTF1,
		KEY_CTRLF1,
		KEY_ALTF1,
		KEY_CTRLSHIFTF1,
		KEY_ALTSHIFTF1,
		KEY_CTRLALTF1,
		KEY_CTRLSHIFTF1 | KEY_ALT,
	};
	return (prefixes[group]) + key;
}

void KeyBar::RefreshObject(bool Render)
{
	AltState = AltPressed && (!CtrlPressed || !ShiftPressed); // C-A-S is a specific case
	CtrlState = CtrlPressed;
	ShiftState = ShiftPressed;

	int group = GetGroup(AltPressed, ShiftPressed, CtrlPressed, 0);

	int KeyWidth = (X2 - X1 - 1) / 12;
	if (KeyWidth < 8) KeyWidth = 8;
	if (KeyWidth > 11) KeyWidth = 11;
	int LabelWidth = KeyWidth - 2;

	if (Render)
		GotoXY(X1, Y1);

	bool FKeyTitlesChanged = false;
	bool palette = false;
	for (int i = 0; i < (int)groups[group].keys.size(); i++) {
		const wchar_t *Label = groups[group].keys[i].text.c_str();

		if (Render) groups[group].keys[i].x1 = groups[group].keys[i].x2 = -1;
		if (Opt.Dialogs.UseModernLook && (!Label || !*Label)) continue;

		std::wstring keyLabel = Render && Label && *Label && Opt.Dialogs.UseModernLook ? GetKeyName(i, group) : L"";

		if (groups[group].keys[i].text != groups[group].keys[i].prevText) {
			FKeyTitlesChanged = true;
			groups[group].keys[i].prevText = groups[group].keys[i].text;
		}

		wchar_t labelHolder[128];
		wcscpy(labelHolder, Label);
		wchar_t* q = wcschr(labelHolder, L' ');
		if(q) *q = 0;

		if (Opt.Dialogs.UseModernLook && Render) 
			LabelWidth = wcslen(labelHolder) + 3 + wcslen(keyLabel.c_str());

		if (Render && WhereX() + LabelWidth + (Opt.Dialogs.UseModernLook ? (int)strExtra.GetLength() : 0) < X2) {
			groups[group].keys[i].x1 = WhereX();
			if (!Opt.Dialogs.UseModernLook) {
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
				uint64_t color1 = SoftenItemColor(FarColorToReal(COL_KEYBARNUM), 0, groups[group].keys[i].hover, 0, 0);
				uint64_t color2 = SoftenItemColor(FarColorToReal(COL_KEYBARTEXT), 0, groups[group].keys[i].hover, 0, 0);

				if (!palette) {
					SetColor(SoftenItemColor(FarColorToReal(COL_KEYBARTEXT), 0, SandwichHover, 0, 0));
					FS << L"🦊☰";
					groups[group].keys[i].x1 = WhereX();
					palette = true;
				}
				SetColor(color2);

				SetColor(color1);
				FS << keyLabel.c_str();
				SetColor(color2);
				FS << L" ";
				FS << (labelHolder);
				FS << L" ";
			}
			groups[group].keys[i].x2 = WhereX() - 1;
		}
	}

	if (Render) {
		int Width = X2 - WhereX() + 1;
		if (!Opt.Dialogs.UseModernLook) {
    		if (Width > 0) {
    			SetFarColor(COL_KEYBARTEXT);
    			FS << fmt::Cells() << fmt::Expand(Width) << L"";
    		}
		}
		else {
    		if (Width > 0) {
            	int extraLen = strExtra.GetLength();
                if (extraLen < Width - 2) extraLen = Width - 2;
    			SetFarColor(COL_KEYBARTEXT);
    			// FS << L"┋ ";
                SetFarColor(COL_KEYBARBACKGROUND);
    			FS << fmt::Cells() << fmt::RightAlign() << fmt::Size(extraLen) << strExtra << L" ";
    		}
		}
	}

	if (FKeyTitlesChanged) {
		std::string str_titles[CONSOLE_FKEYS_COUNT];
		const char *titles[ARRAYSIZE(str_titles)];
		for (int i = 0; i < (int)ARRAYSIZE(str_titles); ++i) {
			if (i < (int)groups[group].keys.size()) {
				StrWide2MB(groups[group].keys[i].text.c_str(), str_titles[i]);
				titles[i] = str_titles[i].c_str();
			} else {
				titles[i] = NULL;
			}
		}

		WINPORT(SetConsoleFKeyTitles)(NULL, titles);
	}
}

/* vk: this legacy code is dead */
void KeyBar::ReadRegGroup(const wchar_t *RegGroup, const wchar_t *Language)
{
}

/* vk: this legacy code is dead */
void KeyBar::SetRegGroup(int Group)
{
}

/* vk: this legacy code is dead */
void KeyBar::SetAllRegGroup()
{
	for (int I = 0; I < KBL_GROUP_COUNT; ++I)
		SetRegGroup(I);
}

void KeyBar::AddExtraKey(int group, FarKey key, const wchar_t* label, const wchar_t* keyName) {
   	KeyBarElement k{key | KeyGroupMaps[group][1], label, groups[group].groupName + keyName };
   	k.group = group;
   	groups[group].keys.push_back(k);
}

void KeyBar::SetGroup(int Group, const wchar_t *const *Key, int KeyCount)
{
	if (!Key)
		return;

	groups[Group].keys.clear();
	for (int i = 0; i < KeyCount && i < KEY_COUNT; i++) {
		if (Key[i]) {
			std::wstring keyLabel = std::wstring(L"F") + std::to_wstring(i + 1);
			AddExtraKey(Group, KEY_F1 + i, Key[i] ? Key[i] : L"", keyLabel.c_str());
		}
	}
}

void KeyBar::ClearGroup(int Group)
{
	groups[Group].keys.clear();
}

// Изменение любого Label
void KeyBar::Change(int Group, const wchar_t *NewStr, int Pos)
{
	if (!NewStr) return;
	//far_wcsncpy(KeyTitles[Group][Pos], NewStr, ARRAYSIZE(KeyTitles[Group][Pos]));
	for(int i = 0; i < (int)groups[Group].keys.size(); ++i) {
		if (groups[Group].keys[i].key == KEY_F1 + Pos) {
			groups[Group].keys[i].text = NewStr;
			return;
		}
	}
}

// Групповая установка идущих подряд строк LNG для указанной группы
void KeyBar::SetAllGroup(int Group, FarLangMsg BaseMsg, int Count)
{
	if (Count > KEY_COUNT)
		Count = KEY_COUNT;

	groups[Group].keys.clear();
	for (int i = 0; i < Count; i++) {
		//far_wcsncpy(KeyTitles[Group][i], (BaseMsg + i).CPtr(), ARRAYSIZE(KeyTitles[Group][i]));
		std::wstring keyLabel = std::wstring(L"F") + std::to_wstring(i + 1);
		AddExtraKey(Group, KEY_F1 + i, (BaseMsg + i).CPtr(), keyLabel.c_str());
	}
}

int KeyBar::ProcessKey(FarKey Key)
{
	switch (Key) {
		case KEY_KILLFOCUS:
		case KEY_GOTFOCUS:
			RedrawIfChanged();
			return TRUE;
	}

	return FALSE;
}

static int GetGroupByEvent(MOUSE_EVENT_RECORD *MouseEvent) {
    if (MouseEvent->dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
    		|| (MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED)) {
    	if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
    		return KBL_ALTSHIFT;
    	else if (MouseEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
    		return KBL_CTRLALT;
    	else
    		return KBL_ALT;
    } 
    else if (MouseEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) {
    	if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
    		return KBL_CTRLSHIFT;
    	else
    		return KBL_CTRL;
    } 
    else if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
    	return KBL_SHIFT;
    else
    	return KBL_MAIN;
}

static int ComposeKey(int group, FarKey key) {
	return key | KeyGroupMaps[group][1]; 
}

int KeyBar::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	INPUT_RECORD rec;
	FarKey Key;

	if (!IsVisible())
		return FALSE;

   	int MsX = MouseEvent->dwMousePosition.X;
    int group = GetGroupByEvent(MouseEvent);

	if (Opt.Dialogs.UseModernLook) {
    	bool needsRedraw = SandwichHover != (MsX <= 2);

        // out oif key bar
    	if (MsX < X1 || MsX > X2 || MouseEvent->dwMousePosition.Y != Y1) {
    		for (int i = 0; i < (int)groups[group].keys.size(); ++i){ 
    			if (groups[group].keys[i].hover) needsRedraw = true;
                groups[group].keys[i].hover = 0;
    		}
    		SandwichHover = 0;
    		if (needsRedraw && Opt.Dialogs.UseModernLook) Redraw();
    		return FALSE;
    	}

    	// Hover effect
    	if (MouseEvent->dwEventFlags == MOUSE_MOVED) {
    		int i;

    		SandwichHover = MsX <= 2;
    		for (i = 0; i < (int)groups[group].keys.size(); i++) {
    			if (groups[group].keys[i].x1 < 0 || groups[group].keys[i].x2 <= 0) continue;

    			if (MsX >= groups[group].keys[i].x1 && MsX <= groups[group].keys[i].x2) {
    				if (!groups[group].keys[i].hover) needsRedraw = true;
    				groups[group].keys[i].hover = 1;
    			}
    			else {
    				groups[group].keys[i].hover = 0; // hidden means no repaints
    			}
    		}

    		if (needsRedraw) {
    			Redraw();
    			return TRUE;
    		}
    	}
	}

	// out of region and no click
	if (MsX < X1 || MsX > X2 || MouseEvent->dwMousePosition.Y != Y1)
		return FALSE;
	if (!(MouseEvent->dwButtonState & 3) /* || MouseEvent->dwEventFlags */) 
		return FALSE;

	// Now click: just fire events, no magic like below
	if (Opt.Dialogs.UseModernLook) {
		int i;
		if (MsX <= 2) {
			ShowContextMenu();
			Redraw();
			return TRUE;
		}

		for (i = 0; i < (int)groups[group].keys.size(); i++) {
			if (groups[group].keys[i].x1 < 0) continue;
			if (groups[group].keys[i].x1 <= MsX && groups[group].keys[i].x2 >= MsX ) {
				// i is our position, fire the key
				Key = ComposeKey(group, groups[group].keys[i].key);

                FrameManager->ProcessKey(Key);
                Redraw();
                return TRUE;
			}
		}
		return FALSE;
	}

	int KeyWidth = (X2 - X1 - 1) / 12;
	if (KeyWidth < 8) KeyWidth = 8;
	if (KeyWidth > 11) KeyWidth = 11;

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

	Key = ComposeKey(group, KEY_F1 + Key);

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

static const wchar_t* GetDescriptionFromLabelIf(const wchar_t* label) {
    return label;
}

void KeyBar::ShowContextMenu() 
{
	int pos = 0;
	int cnt = 0;
	int width = 10;
	int keywidth = 5;

	// count list size and widths
	for (int j = 0; j < (int)groups.size(); ++j) {
		for (int i = 0; i < (int)groups[j].keys.size(); i++) {
			const wchar_t *Label = groups[j].keys[i].text.c_str();
			if (!Label || !*Label) continue;
            ++cnt;

            Label = GetDescriptionFromLabelIf(Label);
            width = std::max(width, (int)wcslen(Label) + 5);

            std::wstring keyLabel = GetKeyName(i, j);
            keywidth = std::max(keywidth, (int)keyLabel.size() + 5);
		}
	}

	// prepare texts
	std::vector<std::wstring> labels;
	for (int j = 0; j < (int)groups.size(); ++j) {
		for (int i = 0; i < (int)groups[j].keys.size(); i++) {
			const wchar_t *Label = groups[j].keys[i].text.c_str();
			if (!Label || !*Label) continue;

            Label = GetDescriptionFromLabelIf(Label);

			std::wstring label = Label;
			std::wstring keyLabel = GetKeyName(i, j);

			std::wstring s = label;
    		if (s.size() < (size_t)width) s.resize(width, L' ');
    		s += keyLabel;

			labels.push_back(s);
		}
	}

	// now we ready to fill menus
	MenuDataEx Groups[cnt];
	for (int j = 0; j < (int)groups.size(); ++j) {
		for (int i = 0; i < (int)groups[j].keys.size(); i++) {
			const wchar_t *Label = groups[j].keys[i].text.c_str();
			if (!Label || !*Label) continue;

			Groups[pos] = {	labels[pos].c_str(), 0, BuildShortcut(j, i) };
            ++pos;
		}
	}
	int GroupsLen = cnt;

	FarKey key = 0;
	{
		int GroupsCode;
		VMenu GroupsMenu(L"", Groups, GroupsLen, 0);

		for (;;) {
			GroupsMenu.SetPosition(3, std::max(Y1 - GroupsLen - 3, 2), 0, 0);
			GroupsMenu.SetFlags(VMENU_WRAPMODE | VMENU_NOTCHANGE);
			GroupsMenu.ClearDone();
			GroupsMenu.Process();

			if ((GroupsCode = GroupsMenu.Modal::GetExitCode()) < 0)
				break;

			if (GroupsCode < 0 || GroupsCode >= GroupsLen) break;

			key = Groups[GroupsCode].AccelKey;
			break;
		}
	}

	if (key) {
		FrameManager->ProcessKey(key);
	}
}
