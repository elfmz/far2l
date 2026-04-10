#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h> 
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>


#ifdef __linux__
# include <termios.h>
# include <linux/kd.h>
# include <linux/keyboard.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/kbio.h>
#endif

#include <string>
#include <vector>

#include <os_call.hpp>
#include "TTYCaps.h"
#include "WinCompat.h"
//NB TTYCaps_Startup() and stuff it uses must be signal-safe
// see http://man7.org/linux/man-pages/man7/signal-safety.7.html

unsigned int TTYKernelQueryControlKeys(int fd)
{
	unsigned int out = 0;

#ifdef __linux__
	unsigned char state = 6;
/* #ifndef KG_SHIFT
# define KG_SHIFT        0
# define KG_CTRL         2
# define KG_ALT          3
# define KG_ALTGR        1
# define KG_SHIFTL       4
# define KG_KANASHIFT    4
# define KG_SHIFTR       5
# define KG_CTRLL        6
# define KG_CTRLR        7
# define KG_CAPSSHIFT    8
#endif */

	if (ioctl(fd, TIOCLINUX, &state) == 0) {
		if (state & ((1 << KG_SHIFT) | (1 << KG_SHIFTL) | (1 << KG_SHIFTR))) {
			out|= SHIFT_PRESSED;
		}
		if (state & (1 << KG_CTRLL)) {
			out|= LEFT_CTRL_PRESSED;
		}
		if (state & (1 << KG_CTRLR)) {
			out|= RIGHT_CTRL_PRESSED;
		}
		if ( (state & (1 << KG_CTRL)) != 0
		&& ((state & ((1 << KG_CTRLL) | (1 << KG_CTRLR))) == 0) ) {
			out|= LEFT_CTRL_PRESSED;
		}

		if (state & (1 << KG_ALTGR)) {
			out|= RIGHT_ALT_PRESSED;
		}
		else if (state & (1 << KG_ALT)) {
			out|= LEFT_ALT_PRESSED;
		}
	}
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__linux__)
	unsigned long int leds = 0;
	if (ioctl(fd, KDGETLED, &leds) == 0) {
		if (leds & 1) {
			out|= SCROLLLOCK_ON;
		}
		if (leds & 2) {
			out|= NUMLOCK_ON;
		}
		if (leds & 4) {
			out|= CAPSLOCK_ON;
		}
	}
#endif
	return out;
}

bool TTYWriteAndDrain(int fd, const std::string &str)
{
	for (size_t ofs = 0; ofs < str.size(); ) {
		ssize_t written = write(fd, str.data() + ofs, str.size() - ofs);
		if (written > 0) {
			ofs+= (size_t) written;
		} else if (written == 0 || (errno != EINTR && errno != EAGAIN)) {
			return false;
		}
	}
	tcdrain(fd);
	return true;
}

static char ReadCharWithTimeout(int fdin)
{
	fd_set fds, fde;
	FD_ZERO(&fds);
	FD_ZERO(&fde);
	FD_SET(fdin, &fds);
	FD_SET(fdin, &fde);
	struct timeval tv = {10, 0};

	if (os_call_int(select, fdin + 1, &fds, (fd_set*)nullptr, &fde, &tv) <= 0) {
		return 0;
	}
	char c = 0;
	if (os_call_ssize(read, fdin, (void *)&c, sizeof(c)) <= 0) {
		return 0;
	}
	return c ? c : ' ';
}


struct ReplyWithArgs
{
	std::vector<long> args;
	bool fetched{false};

	void Fetch(std::string &s, const char *prefix, const char *suffix = nullptr)
	{
		if (fetched) {
			return;
		}
		size_t b = s.find(prefix);
		if (b == std::string::npos) {
			return;
		}
		b+= strlen(prefix);
		if (suffix) {
			size_t e = s.find(suffix, b);
			if (e == std::string::npos) {
				return;
			}
			for (const char *p = s.c_str() + b, *ep = s.c_str() + e; p < ep;) {
				args.push_back(strtol(p, (char **)&p, 10));
				do {
					++p;
				} while (p < ep && (*p == ',' || *p == ';' || *p == ':'));
			}
			b = e + 1;
		}
		s.erase(0, b);

		fetched = true;
	}	
};

void TTYCaps::Setup(int fdin, int fdout, const TTYRestrict &restrict)
{
	// set detectable things to default values
	DEC_lines = false;
	strict_dups = false;
	strict_pos = false;
	emoji_vs16 = false;
	norgb = false;

	kind = GENERIC;

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
	unsigned long int leds = 0;
	if (ioctl(fdout, KDGETLED, &leds) == 0) {
		kind = KERNEL;
	} else {
		int kd_mode;
# if defined(__linux__)
		if (ioctl(fdin, KDGETMODE, &kd_mode) == 0) {
# else
		if (ioctl(fdin, KDGKBMODE, &kd_mode) == 0) {
# endif
			kind = KERNEL;
		}
	}
#endif

	const bool detect_far2l = (kind == GENERIC && !restrict.far2l);
	const bool detect_emoji = (!restrict.emoji);

	if (detect_far2l || detect_emoji) {
		// message for the human being and set cursor to beginning of next line
		std::string s = "Press <ENTER> if tired of watching this message\n";
		if (detect_far2l) {
			// far2l supports both BEL and ST APC finalizers, however screen supports only ST
			s+= "\e_far2l1\e\\";
		}
		if (detect_emoji) {
			// request DSR/cursor position
			s+= "\e[6n";  
		}
		// finally request DSR/terminal status ans this is supported by (almost) all terminals so use this fact
		// to avoid long waits for terminals that doent reply on any other request asked in this string
		s+= "\e[5n";
		if (TTYWriteAndDrain(fdout, s)) {
			s.clear();
			ReplyWithArgs reply_on_far2l, reply_on_emoji, reply_on_status;
			while (s.size() < 0x10000  && !reply_on_status.fetched) {
				char c = ReadCharWithTimeout(fdin);
				if (!c) {
					break;
				}
				s+= c;

				if (detect_far2l) {
					reply_on_far2l.Fetch(s, "\e_far2lok\a");
				}
				if (detect_emoji) {
					reply_on_emoji.Fetch(s, "\e[", "R");
				}
				reply_on_status.Fetch(s, "\e[", "n");
			}
			if (reply_on_far2l.fetched) {
				kind = FAR2L;
			}
			if (reply_on_emoji.fetched && reply_on_emoji.args.size() > 1) {
				if (reply_on_emoji.args[1] == 2) { //row, col
					emoji_vs16 = true;
				}
			}

			// erase stuff printed for detection:
			//  move cursor to beginning of current line
			//  clear current line
			//  move cursor to beginning of previsous line
			//  clear current line
			//  print empty line
			TTYWriteAndDrain(fdout, "\e[G\e[2K\e[F\e[2K\r\n");
		}
	}

	const char *env = getenv("TERM");
	if (env && (strcmp(env, "wsvt25") == 0 || strcmp(env, "vt100") == 0 || strcmp(env, "vt220") == 0) ) {
		DEC_lines = true;
	}
	if (env && strncmp(env, "screen", 6) == 0) { // TERM=screen.xterm-256color
		strict_dups = true;
		norgb = true; // If in GNU Screen, default "norgb = true" to avoid unusable colours
	}
	if (restrict.rgb) {
		norgb = true;
	}

	env = getenv("TERM_PROGRAM");
	if (env && strcasecmp(env, "WezTerm") == 0) {
		strict_pos = true;
	}

	fprintf(stderr, "TTYCaps: %s %s%s%s restrict=%s%s%s%s%s%s%s\n",
			(kind == FAR2L) ? "FAR2L" : ((kind == KERNEL) ? "KERNEL" : "GENERIC"),
			DEC_lines ? "DECLines " : "",
			strict_dups ? "StrictDups " : "",
			strict_pos ? "StrictPos " : "",
			restrict.x11 ? "X11 " : "",
			restrict.xi ? "Xi " : "",
			restrict.far2l ? "F2L " : "",
			restrict.apple ? "APL " : "",
			restrict.kitty ? "KTY " : "",
			restrict.win32 ? "W32 " : "",
			restrict.emoji ? "EMJ " : "");
}

void TTYCaps::Finup(int fdin, int fdout)
{
	if (kind == FAR2L && TTYWriteAndDrain(fdout, "\e_far2l0\a\e[5n")) {
		std::string s;
		ReplyWithArgs reply_on_far2l, reply_on_status;
		while (s.size() < 0x10000  && !reply_on_status.fetched) {
			char c = ReadCharWithTimeout(fdin);
			if (!c) {
				break;
			}
			s+= c;
			reply_on_far2l.Fetch(s, "\e_far2lok\a");
			reply_on_status.Fetch(s, "\e[", "n");
		}
		if (!reply_on_status.fetched) {
			fprintf(stderr, "TTYCaps::Finup: no reply_on_status\n");
		} else if (!reply_on_far2l.fetched) {
			fprintf(stderr, "TTYCaps::Finup: no reply_on_far2l\n");
		}
		kind = GENERIC;
	}
}
