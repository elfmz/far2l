#include <vector>
#include <ConsoleInput.h>
#include <ConsoleOutput.h>
#include <utils.h>
#include <SavedScreen.h>
#include "SudoAskpassImpl.h"

extern ConsoleOutput g_winport_con_out;
extern ConsoleInput g_winport_con_in;

class SudoAskpassScreen
{
	ConsoleInputPriority _cip;
	SavedScreen _ss;
	INPUT_RECORD _ir_resized;
	unsigned int _width = 0, _height = 0;
	std::string _title, _text, _key_hint;
	std::wstring _input;
	enum Result {
		RES_PENDING,
		RES_OK,
		RES_CANCEL
	} _result = RES_PENDING;
	bool _need_repaint = false;

	void DispatchInputKey(const KEY_EVENT_RECORD &rec)
	{
		if (!rec.bKeyDown)
			return;

		if (rec.wVirtualKeyCode == VK_RETURN) {
			_result = RES_OK;

		} else if (rec.wVirtualKeyCode == VK_ESCAPE || rec.wVirtualKeyCode == VK_F10) {
			_result = RES_CANCEL;

		} else if (rec.wVirtualKeyCode == VK_BACK) {
			if (!_input.empty())
				_input.resize(_input.size() - 1);

		} else if (rec.uChar.UnicodeChar) {
			_input+= rec.uChar.UnicodeChar;
		}
	}

	void DispatchInputMouse(const MOUSE_EVENT_RECORD &rec)
	{
	}

	void DispatchInput()
	{
		INPUT_RECORD ir;
		while (g_winport_con_in.Dequeue(&ir, 1, _cip)) {
			switch (ir.EventType) {
				case WINDOW_BUFFER_SIZE_EVENT:
					_ss.Restore();
					_ir_resized = ir;
					_need_repaint = true;
					break;

				case KEY_EVENT:
					DispatchInputKey(ir.Event.KeyEvent);
					break;

				case MOUSE_EVENT:
					DispatchInputMouse(ir.Event.MouseEvent);
					break;
			}
		}
	}

	void WriteCentered(const std::string &str, SHORT t)
	{
		std::wstring wstr;
		StrMB2Wide(str, wstr);
		SHORT l = (_width > wstr.size()) ? (_width - wstr.size()) / 2 : 0;
		COORD pos = {l, t};
		g_winport_con_out.WriteStringAt(wstr.c_str(), wstr.size(), pos);
	}

	void Repaint()
	{
		g_winport_con_out.GetSize(_width, _height);
		SHORT w = std::max(_key_hint.size(), std::max(_title.size(), _text.size())) + 5, h = 5;

		SHORT l = (SHORT(_width) - w) / 2, t = 0;//(_height - h) / 2;
		if (l < 0) l = 0;
//		if (t < 0) t = 0;

		COORD pos;

		for (SHORT y = t; y < t + h; ++y) {
			for (SHORT x = l; x < l + w; ++x) {
				pos = COORD {x, y};
				if (x == l || y == t || x + 1 == l + w || y + 1 == t + h) {
					g_winport_con_out.FillAttributeAt(FOREGROUND_RED | BACKGROUND_RED, 1, pos);
				} else {
					g_winport_con_out.FillAttributeAt(
						FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, 1, pos);
				}
				pos = COORD {x, y};
				g_winport_con_out.FillCharacterAt(' ', 1, pos);
			}
		}

		g_winport_con_out.SetAttributes(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		WriteCentered(_title, t + 1);
		WriteCentered(_key_hint, t + 3);

		g_winport_con_out.SetAttributes(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		WriteCentered(_text, t + 2);
//		COORD pos = {};
//		g_winport_con_out.FillAttributeAt(FOREGROUND_GREEN, _width * _height, pos);
	}

public:
	SudoAskpassScreen(const std::string &title, const std::string &text) :
		_cip(g_winport_con_in),
		_title(title),
		_text(text),
		_key_hint("Confirm by <Enter> Cancel by <Esc>")
	{
		_ir_resized.EventType = NOOP_EVENT;
		Repaint();
	}

	~SudoAskpassScreen()
	{
		if (_ir_resized.EventType != NOOP_EVENT)
				g_winport_con_in.Enqueue(&_ir_resized, 1);
	}

	bool Loop()
	{
		for (;;) {
			if (g_winport_con_in.WaitForNonEmpty(1000, _cip)) {
				DispatchInput();
			}/* else {
				// repaint periodically to ensure not overpainted by somebody else
				_need_repaint = true;
			} */

			if (_result != RES_PENDING)
				return _result == RES_OK;

			if (_need_repaint) {
				_need_repaint = false;
				Repaint();
			}
		}
	}

	void GetInput(std::string &str)
	{
		StrWide2MB(_input, str);
	}
};

bool SudoAskpassImpl::OnSudoAskPassword(const std::string &title, const std::string &text, std::string &password)
{
	SudoAskpassScreen sas(title, text);
	if (!sas.Loop())
		return false;

	sas.GetInput(password);
	return true;
}

bool SudoAskpassImpl::OnSudoConfirm(const std::string &title, const std::string &text)
{
	SudoAskpassScreen sas(title, text);
	return sas.Loop();
}
