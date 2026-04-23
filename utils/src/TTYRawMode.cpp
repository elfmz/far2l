#include <stdio.h>
#include <unistd.h>
#include "TTYRawMode.h"

TTYRawMode::TTYRawMode(int std_in, int std_out)
{
	// try first stdout, but if it fails - try stdin
	// this helps to work with specific cases of redirected
	// output, like working with tee
	if (tcgetattr(std_out, &_ts) != 0) {
		if (tcgetattr(std_in, &_ts) != 0) {
			return;
		}
		_fd = dup(std_in);
	} else {
		_fd = dup(std_out);
	}

	struct termios ts_ne = _ts;
	cfmakeraw(&ts_ne);
	if (tcsetattr( _fd, TCSADRAIN, &ts_ne ) != 0) {
		close(_fd);
		_fd = -1;
	}
}

TTYRawMode::~TTYRawMode()
{
	if (_fd != -1) {
		if (tcsetattr(_fd, TCSADRAIN, &_ts) != 0) {
			perror("~TTYRawMode - tcsetattr");
		}
		close(_fd);
	}
}


