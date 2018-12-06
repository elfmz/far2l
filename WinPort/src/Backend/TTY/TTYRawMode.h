#pragma once
#include <termios.h>

class TTYRawMode
{
	termios _ts{};
	bool _applied = false;
	int _fd;

public:
	TTYRawMode(int fd);
	~TTYRawMode();

	bool Applied() const {return _applied; }
};
