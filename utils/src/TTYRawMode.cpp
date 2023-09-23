#include <stdio.h>
#include <os_call.hpp>
#include "TTYRawMode.h"

TTYRawMode::TTYRawMode(int std_in, int std_out)
{
	// try first stdout, but if it fails - try stdin
	// this helps to work with specific cases of redirected
	// output, like working with tee
	if (os_call_int(tcgetattr, std_out, &_ts) != 0) {
		if (os_call_int(tcgetattr, std_in, &_ts) != 0) {
			return;
		}
		_fd = std_in;
	} else {
		_fd = std_out;
	}

	struct termios ts_ne = _ts;
	cfmakeraw(&ts_ne);
	if (os_call_int(tcsetattr, _fd, TCSADRAIN, (const struct termios*)&ts_ne ) != 0) {
		_fd = -1;
	}
}

TTYRawMode::~TTYRawMode()
{
	if (_fd != -1) {
		if (os_call_int(tcsetattr, _fd, TCSADRAIN, (const struct termios*)&_ts) != 0) {
			perror("~TTYRawMode - tcsetattr");
		}
	}
}


