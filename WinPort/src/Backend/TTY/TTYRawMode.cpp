#include <stdio.h>
#include "TTYRawMode.h"

TTYRawMode::TTYRawMode(int fd)
	: _fd(fd)
{
	if (tcgetattr(_fd, &_ts) == 0) {
		struct termios ts_ne = _ts;
		cfmakeraw(&ts_ne);
		if (tcsetattr( _fd, TCSADRAIN, &ts_ne ) == 0) {
			_applied = true;
		}
	}
}

TTYRawMode::~TTYRawMode()
{
	if (_applied) {
		if (tcsetattr(_fd, TCSADRAIN, &_ts) != 0) {
			perror("~TTYRawMode - tcsetattr");
		}
	}
}


