#pragma once
#include <termios.h>

class TTYRawMode
{
	termios _ts{};
	int _fd = -1;

public:
	TTYRawMode(int std_in, int std_out);
	~TTYRawMode();

	bool Applied() const { return _fd != -1; }
};
