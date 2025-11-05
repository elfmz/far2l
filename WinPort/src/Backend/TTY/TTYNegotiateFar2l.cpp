#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <os_call.hpp>
#include "TTYNegotiateFar2l.h"


//NB TTYNegotiateFar2l() and stuff it uses must be signal-safe
// see http://man7.org/linux/man-pages/man7/signal-safety.7.html

static bool WriteStr2TC(int fd, const char *str)
{
	for (size_t len = strlen(str), ofs = 0; ofs < len; ) {
		ssize_t written = write(fd, str + ofs, len - ofs);
		if (written <= 0)
			return false;

		ofs+= (size_t) written;
	}
	tcdrain(fd);
	return true;
}

bool TTYNegotiateFar2l(int fdin, int fdout, bool enable)
{
	// far2l supports both BEL and ST APC finalizers, however screen supports only ST,
	// so use it as most compatible
	if (!WriteStr2TC(fdout, enable ? "\x1b_far2l1\x1b\\\x1b[5n" : "\x1b_far2l0\x07\x1b[5n"))
		return false;

	if (enable) {
		if (!WriteStr2TC(fdout, "Press <ENTER> if tired of watching this message"))
			return false;
	}

	char inbuf[0x10000] = {};
	size_t inlen = 0;
	for (bool status_replied = false; !status_replied;) {
		fd_set fds, fde;
		FD_ZERO(&fds);
		FD_ZERO(&fde);
		FD_SET(fdin, &fds);
		FD_SET(fdin, &fde);
		struct timeval tv = {10, 0};

		if (inlen >= sizeof(inbuf) - 1) {
			break;
		}

		if (os_call_int(select, fdin + 1, &fds, (fd_set*)nullptr, &fde, &tv) <= 0) {
			break;
		}

		if (read(fdin, &inbuf[inlen], 1) != 1) {
			break;
		}
		if (inbuf[inlen] == 0) {
			inbuf[inlen] = ' ';
		}
		inlen++;
//		inbuf[inlen] = 0;

		if (strstr(inbuf, "\x1b\x1b") != nullptr // ..if terminal doesnt report status and user in panic
		 || strpbrk(inbuf, "\r\n") != nullptr) {
			break;
		}

		// status reply typically looks like "\x1b[0n"
		// but parse anything that is "\x1b[???n" where ??? is empty or sequence of nubers with ;
		for (const char *p = strstr(inbuf, "\x1b["); p != nullptr; p = strstr(p + 1, "\x1b[")) {
			const char *e = strchr(p + 1, 'n');
			if (e != nullptr) {
				status_replied = true;
				for (const char *i = p + 2; i < e; ++i) {
					if ((*i < '0' || *i > '9') && *i != ';') {
						status_replied = false;
					}
				}
				if (status_replied) break;
			}
		}
	}

	if (enable) { // erase stuff printed for detection
		WriteStr2TC(fdout, "\x1b[2K\r\n");
	}

	return (strstr(inbuf, "\x1b_far2lok\x07") != nullptr);
}

