#pragma once
#include <vector>
#include "../../WinPort/WinCompat.h"

class SavedScreen
{
	std::vector<CHAR_INFO> _content;
	CONSOLE_SCREEN_BUFFER_INFO _csbi;

public:
	SavedScreen();
	~SavedScreen();
	void Restore();
};

