#include <vector>
#include <ConsoleInput.h>
#include <ConsoleOutput.h>
#include <utils.h>
#include "SudoAskpassImpl.h"

extern ConsoleOutput g_winport_con_out;
extern ConsoleInput g_winport_con_in;

class SavedScreen
{
	std::vector<CHAR_INFO> _content;
	unsigned int _width = 0, _height = 0;

public:
	SavedScreen()
	{
		g_winport_con_out.GetSize(_width, _height);
		_content.resize(size_t(_width) * size_t(_height));
		if (!_content.empty()) {
			COORD data_pos = {}, data_size = {(SHORT)_width, (SHORT)_height};
			SMALL_RECT screen_rect = {0, 0, data_size.X, data_size.Y};
			g_winport_con_out.Read(&_content[0], data_size, data_pos, screen_rect);
		}
	}

	~SavedScreen()
	{
		if (!_content.empty()) {
			COORD data_pos = {}, data_size = {(SHORT)_width, (SHORT)_height};
			SMALL_RECT screen_rect = {0, 0, data_size.X, data_size.Y};
			g_winport_con_out.Write(&_content[0], data_size, data_pos, screen_rect);
		}
	}
};

class SudoAskpassScreen
{
	ConsoleInputPriority _cip;
	SavedScreen _ss;
	INPUT_RECORD _ir_resized;
	unsigned int _width = 0, _height = 0;
	std::wstring _input;
	enum Result {
		RES_PENDING,
		RES_OK,
		RES_CANCEL
	} _result = RES_PENDING;

	void DispatchInputKey(const KEY_EVENT_RECORD &rec)
	{
		if (!rec.bKeyDown)
			return;

		if (rec.wVirtualKeyCode == VK_RETURN) {
			_result = RES_OK;

		} else if (rec.wVirtualKeyCode == VK_ESCAPE) {
			_result = RES_CANCEL;

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
					_ir_resized = ir;
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

public:
	SudoAskpassScreen(const std::string &title, const std::string &text) :  _cip(g_winport_con_in)
	{
		_ir_resized.EventType = NOOP_EVENT;
		g_winport_con_out.GetSize(_width, _height);

		SHORT l = 0, t = 0, w = std::max(title.size(), text.size()) + 5, h = 4;

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

		std::wstring wstr;
		StrMB2Wide(title, wstr);
		pos = COORD{ l + 1, t + 1};
		g_winport_con_out.WriteStringAt(wstr.c_str(), wstr.size(), pos);

		StrMB2Wide(text, wstr);
		pos = COORD{l + 1, t + 2};
		g_winport_con_out.WriteStringAt(wstr.c_str(), wstr.size(), pos);

//		COORD pos = {};
//		g_winport_con_out.FillAttributeAt(FOREGROUND_GREEN, _width * _height, pos);
	}

	~SudoAskpassScreen()
	{
		if (_ir_resized.EventType != NOOP_EVENT)
				g_winport_con_in.Enqueue(&_ir_resized, 1);
	}

	bool Loop()
	{
		for (;;) {
			if (g_winport_con_in.WaitForNonEmpty(500, _cip))
				DispatchInput();

			if (_result != RES_PENDING)
				return _result == RES_OK;
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
