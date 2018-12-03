#include "SavedScreen.h"
#include "../../WinPort/WinPort.h"

SavedScreen::SavedScreen()
{
	if (WINPORT(GetConsoleScreenBufferInfo)(NULL, &_csbi)) {
		_content.resize(size_t(_csbi.dwSize.X) * size_t(_csbi.dwSize.Y));

		if (!_content.empty()) {
			SMALL_RECT screen_rect = {0, 0, _csbi.dwSize.X - 1, _csbi.dwSize.Y - 1};
			WINPORT(ReadConsoleOutput)(NULL, &_content[0], _csbi.dwSize, COORD {0, 0}, &screen_rect);
		}
	}
}

SavedScreen::~SavedScreen()
{
	Restore();
}

void SavedScreen::Restore()
{
	if (!_content.empty()) {
		SMALL_RECT screen_rect = {0, 0, _csbi.dwSize.X - 1, _csbi.dwSize.Y - 1};
		WINPORT(WriteConsoleOutput)(NULL, &_content[0], _csbi.dwSize, COORD {0, 0}, &screen_rect);
	}
	WINPORT(SetConsoleTextAttribute)(NULL, _csbi.wAttributes);
	WINPORT(SetConsoleCursorPosition)(NULL, _csbi.dwCursorPosition);
}

