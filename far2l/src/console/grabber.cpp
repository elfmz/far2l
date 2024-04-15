/*
grabber.cpp

Screen grabber
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

#include "grabber.hpp"
#include "keyboard.hpp"
#include "colors.hpp"
#include "keys.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "manager.hpp"
#include "frame.hpp"
#include "interf.hpp"
#include "clipboard.hpp"
#include "config.hpp"

Grabber::Grabber()
	:
	_cmm(MACRO_OTHER)
{
	LockCurrentFrame lcf;
	lcf.RefreshOnUnlock();
	_save_scr.reset(new SaveScreen);
	bool visible = false;
	DWORD size = 0;
	GetCursorType(visible, size);

	if (visible)
		GetCursorPos(_area.cur_x, _area.cur_y);

	_area.left = -1;
	SetCursorType(TRUE, 60);
	_prev_area = NormalizedArea();
	_reset_area = true;
	DisplayObject();
}

Grabber::~Grabber() {}


static void FilterGrabbedText(std::wstring &grabbed_text)
{
	for (auto &ch : grabbed_text) {
		const auto orig_ch = ch;
		if (Opt.CleanAscii) switch (orig_ch) {
			case L'.': ch = L'.'; break;
			case 0x07: ch = L'*'; break;
			case 0x10: ch = L'>'; break;
			case 0x11: ch = L'<'; break;
			case 0x18: // fallthrough
			case 0x19: ch = L'|'; break;
			case 0x1E: // fallthrough
			case 0x1F: ch = L'X'; break;
			case 0xFF: ch = L' '; break;
			default:
				if (orig_ch < 0x20)
					ch = L'.';
		}

		if (orig_ch >= 0xB3 && orig_ch <= 0xDA) {
			if (Opt.NoGraphics) switch (orig_ch) {
				case 0xB3: // fallthrough
				case 0xBA: ch = L'|'; break;
				case 0xC4: ch = L'-'; break;
				case 0xCD: ch = L'='; break;
				default:   ch = L'+';
			}

			if (Opt.NoBoxes) switch (orig_ch) {
				case 0xB3: // fallthrough
				case 0xBA: ch = L' '; break;
				case 0xC4: ch = L' '; break;
				case 0xCD: ch = L' '; break;
				default:   ch = L' ';
			}
		}
	}
}


GrabberArea Grabber::NormalizedArea() const
{
	GrabberArea out;
	out.left = Min(_area.left, _area.right);
	out.right = Max(_area.left, _area.right);
	out.top = Min(_area.top, _area.bottom);
	out.bottom = Max(_area.top, _area.bottom);
	out.cur_x = _area.cur_x;
	out.cur_y = _area.cur_y;
	return out;
}

void Grabber::CopyGrabbedArea(bool append)
{
	if (_area.left < 0)
		return;

	const auto &area = NormalizedArea();
	const size_t width = size_t(area.right + 1 - area.left);
	std::vector<CHAR_INFO> char_info(width * (area.bottom + 1 - area.top));
	GetText(area.left, area.top, area.right, area.bottom, char_info.data(), char_info.size() * sizeof(CHAR_INFO));

	std::wstring grabbed_text;
	for (size_t i = 0; i < char_info.size(); ++i) {
		if ( i != 0 && (i % width) == 0) {
			StrTrimRight(grabbed_text);
			grabbed_text+= L'\n';
		}
		if (CI_USING_COMPOSITE_CHAR(char_info[i])) {
			grabbed_text+= WINPORT(CompositeCharLookup)(char_info[i].Char.UnicodeChar);
		} else if (char_info[i].Char.UnicodeChar) {
			grabbed_text+= char_info[i].Char.UnicodeChar;
		}
	}
	StrTrimRight(grabbed_text);

	FilterGrabbedText(grabbed_text);

	Clipboard clip;

	if (clip.Open()) {
		if (append) {
			wchar_t *prev_clip_data = clip.Paste();
			if (prev_clip_data && *prev_clip_data) {
				std::wstring tmp = prev_clip_data;
				if (tmp.back() != '\n') {
					tmp+= '\n';
					tmp+= grabbed_text;
				}
				grabbed_text.swap(tmp);
			}
			free(prev_clip_data);
		}

		clip.Copy(grabbed_text.c_str());

		clip.Close();
	}
}

void Grabber::DisplayObject()
{
	MoveCursor(_area.cur_x, _area.cur_y);

	const auto &area = NormalizedArea();

	if (area.left != _prev_area.left || area.right != _prev_area.right
			|| area.top != _prev_area.top || area.bottom != _prev_area.bottom) {

		if (area.left > _prev_area.left || area.right < _prev_area.right
				|| area.top > _prev_area.top || area.bottom < _prev_area.bottom) {
			_save_scr->RestoreArea(FALSE);
		}

		if (_area.left != -1) {
			const size_t width = size_t(area.right + 1 - area.left);
			std::vector<CHAR_INFO> char_info(width * (area.bottom + 1 - area.top));

			CHAR_INFO *prev_buf = _save_scr->GetBufferAddress();
			GetText(area.left, area.top, area.right, area.bottom, char_info.data(), sizeof(CHAR_INFO) * char_info.size());

			for (auto x = area.left; x <= area.right; ++x)
				for (auto y = area.top; y <= area.bottom; ++y) {
					DWORD64 new_attrs;
					
					if ((prev_buf[size_t(y) * (ScrX + 1) + size_t(x)].Attributes & B_LIGHTGRAY) == B_LIGHTGRAY)
						new_attrs = B_BLACK | F_LIGHTGRAY;
					else
						new_attrs = B_LIGHTGRAY | F_BLACK;

					const size_t index = width * (y - area.top) + size_t(x - area.left);
					char_info[index].Attributes =
						((char_info[index].Attributes & ~0xff) | new_attrs)
							& (~(FOREGROUND_TRUECOLOR | BACKGROUND_TRUECOLOR));
				}

			PutText(area.left, area.top, area.right, area.bottom, char_info.data());
		}

		if (_area.left == -2) {
			_save_scr->RestoreArea(FALSE);
			_area.left = area.right;
		}

		_prev_area = NormalizedArea();
	}
}

int Grabber::ProcessKey(FarKey key)
{
	/*
		$ 14.03.2001 SVS
		[-] Неправильно воспроизводился макрос в режиме грабления экрана.
		При воспроизведении клавиша Home перемещала курсор в координаты
		0,0 консоли.
		Не было учтено режима выполнения макроса.
	*/
	SetCursorType(TRUE, 60);

	if (CtrlObject->Macro.IsExecuting()) {
		if ((key & KEY_SHIFT) && key != KEY_NONE && _reset_area)
			Reset();
		else if (key != KEY_IDLE && key != KEY_NONE && !(key & KEY_SHIFT))
			_reset_area = true;
	} else {
		if ((ShiftPressed || key != KEY_SHIFT) && (key & KEY_SHIFT) && key != KEY_NONE && _reset_area)
			Reset();
		else if (key != KEY_IDLE && key != KEY_NONE && key != KEY_SHIFT && !ShiftPressed && !(key & KEY_SHIFT))
			_reset_area = true;
	}

	switch (key) {
		case KEY_CTRLU:
			Reset();
			_area.left = -2;
			break;
		case KEY_ESC:
			SetExitCode(0);
			break;
		case KEY_NUMENTER:
		case KEY_ENTER:
		case KEY_CTRLINS:
		case KEY_CTRLNUMPAD0:
		case KEY_CTRLADD:
			CopyGrabbedArea(key == KEY_CTRLADD);
			SetExitCode(1);
			break;
		case KEY_LEFT:
		case KEY_NUMPAD4:
		case L'4':

			if (_area.cur_x > 0)
				_area.cur_x--;

			break;
		case KEY_RIGHT:
		case KEY_NUMPAD6:
		case L'6':

			if (_area.cur_x < ScrX)
				_area.cur_x++;

			break;
		case KEY_UP:
		case KEY_NUMPAD8:
		case L'8':

			if (_area.cur_y > 0)
				_area.cur_y--;

			break;
		case KEY_DOWN:
		case KEY_NUMPAD2:
		case L'2':

			if (_area.cur_y < ScrY)
				_area.cur_y++;

			break;
		case KEY_HOME:
		case KEY_NUMPAD7:
		case L'7':
			_area.cur_x = 0;
			break;
		case KEY_END:
		case KEY_NUMPAD1:
		case L'1':
			_area.cur_x = ScrX;
			break;
		case KEY_PGUP:
		case KEY_NUMPAD9:
		case L'9':
			_area.cur_y = 0;
			break;
		case KEY_PGDN:
		case KEY_NUMPAD3:
		case L'3':
			_area.cur_y = ScrY;
			break;
		case KEY_CTRLHOME:
		case KEY_CTRLNUMPAD7:
			_area.cur_x = _area.cur_y = 0;
			break;
		case KEY_CTRLEND:
		case KEY_CTRLNUMPAD1:
			_area.cur_x = ScrX;
			_area.cur_y = ScrY;
			break;
		case KEY_CTRLLEFT:
		case KEY_CTRLNUMPAD4:
		case KEY_CTRLSHIFTLEFT:
		case KEY_CTRLSHIFTNUMPAD4:

			if ((_area.cur_x-= 10) < 0)
				_area.cur_x = 0;

			if (key == KEY_CTRLSHIFTLEFT || key == KEY_CTRLSHIFTNUMPAD4)
				_area.left = _area.cur_x;

			break;
		case KEY_CTRLRIGHT:
		case KEY_CTRLNUMPAD6:
		case KEY_CTRLSHIFTRIGHT:
		case KEY_CTRLSHIFTNUMPAD6:

			if ((_area.cur_x+= 10) > ScrX)
				_area.cur_x = ScrX;

			if (key == KEY_CTRLSHIFTRIGHT || key == KEY_CTRLSHIFTNUMPAD6)
				_area.left = _area.cur_x;

			break;
		case KEY_CTRLUP:
		case KEY_CTRLNUMPAD8:
		case KEY_CTRLSHIFTUP:
		case KEY_CTRLSHIFTNUMPAD8:

			if ((_area.cur_y-= 5) < 0)
				_area.cur_y = 0;

			if (key == KEY_CTRLSHIFTUP || key == KEY_CTRLSHIFTNUMPAD8)
				_area.top = _area.cur_y;

			break;
		case KEY_CTRLDOWN:
		case KEY_CTRLNUMPAD2:
		case KEY_CTRLSHIFTDOWN:
		case KEY_CTRLSHIFTNUMPAD2:

			if ((_area.cur_y+= 5) > ScrY)
				_area.cur_y = ScrY;

			if (key == KEY_CTRLSHIFTDOWN || key == KEY_CTRLSHIFTNUMPAD8)
				_area.top = _area.cur_y;

			break;
		case KEY_SHIFTLEFT:
		case KEY_SHIFTNUMPAD4:

			if (_area.left > 0)
				_area.left--;

			_area.cur_x = _area.left;
			break;
		case KEY_SHIFTRIGHT:
		case KEY_SHIFTNUMPAD6:

			if (_area.left < ScrX)
				_area.left++;

			_area.cur_x = _area.left;
			break;
		case KEY_SHIFTUP:
		case KEY_SHIFTNUMPAD8:

			if (_area.top > 0)
				_area.top--;

			_area.cur_y = _area.top;
			break;
		case KEY_SHIFTDOWN:
		case KEY_SHIFTNUMPAD2:

			if (_area.top < ScrY)
				_area.top++;

			_area.cur_y = _area.top;
			break;
		case KEY_SHIFTHOME:
		case KEY_SHIFTNUMPAD7:
			_area.cur_x = _area.left = 0;
			break;
		case KEY_SHIFTEND:
		case KEY_SHIFTNUMPAD1:
			_area.cur_x = _area.left = ScrX;
			break;
		case KEY_SHIFTPGUP:
		case KEY_SHIFTNUMPAD9:
			_area.cur_y = _area.top = 0;
			break;
		case KEY_SHIFTPGDN:
		case KEY_SHIFTNUMPAD3:
			_area.cur_y = _area.top = ScrY;
			break;
	}

	DisplayObject();
	return TRUE;
}

int Grabber::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (MouseEvent->dwEventFlags == DOUBLE_CLICK
			|| (!MouseEvent->dwEventFlags && (MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED))) {
		ProcessKey(KEY_ENTER);
		return TRUE;
	}

	if (MouseButtonState != FROM_LEFT_1ST_BUTTON_PRESSED)
		return FALSE;

	_area.cur_x = Min(Max(static_cast<SHORT>(0), MouseX), ScrX);
	_area.cur_y = Min(Max(static_cast<SHORT>(0), MouseY), ScrY);

	if (!MouseEvent->dwEventFlags)
		_reset_area = true;
	else if (MouseEvent->dwEventFlags == MOUSE_MOVED) {
		if (_reset_area) {
			_area.right = _area.cur_x;
			_area.bottom = _area.cur_y;
			_reset_area = false;
		}

		_area.left = _area.cur_x;
		_area.top = _area.cur_y;
	}

	// VerticalBlock=MouseEvent->dwControlKeyState&(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED);
	DisplayObject();
	return TRUE;
}

void Grabber::Reset()
{
	_area.left = _area.right = _area.cur_x;
	_area.top = _area.bottom = _area.cur_y;
	_reset_area = false;
	// DisplayObject();
}

bool Grabber::Run()
{
	static bool s_in_grabber = false;

	bool out = false;
	if (!s_in_grabber) {
		s_in_grabber = true;
		try {
			WaitInMainLoop = FALSE;
			if (!WinPortTesting())
				FlushInputBuffer();
			Grabber().Process();
			out = true;

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
		}
		s_in_grabber = false;
	}

	return out;
}
