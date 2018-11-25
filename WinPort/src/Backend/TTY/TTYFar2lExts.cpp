#include <string>
#include <vector>
#include <termios.h> 
#include <unistd.h> 
#include <base64.h> 

#include "TTYFar2lExts.h"

#define TTY_REPLY_TIMOUT 10

static bool TTYWriteAndWaitReplyWithStatus(int std_in, int std_out, const std::string &request, std::string &reply)
{
	for (size_t i = 0; i < request.size(); ) {
		ssize_t r = write(std_out, request.c_str() + i, request.size() - i);
		if (r <= 0)
			return false;
		i+= (size_t)r;
	}

	reply.clear();
	for (;;) {
		fd_set fds, fde;
		FD_ZERO(&fds);
		FD_ZERO(&fde);
		FD_SET(std_in, &fds); 
		FD_SET(std_in, &fde); 

		struct timeval tv = {TTY_REPLY_TIMOUT, 0};
		if (select(std_in + 1, &fds, NULL, &fde, &tv) <= 0)
			return false;

		char c = 0;	
		if (read(std_in, &c, sizeof(c)) <= 0)
			return false;

		//not expecting \r\n or ESC-ESC - it looks like human's reaction on stuck
		if (c == '\r' || c == '\n' || c == 0)
			return false;

		if (c == '\x1b' && !reply.empty() && reply[reply.size() - 1] == '\x1b')
			return false;

		reply+= c;

		if (c == 'n') {
			// check if matches to "\x1b[n" "\x1b[#n" "\x1b[#;#n" "\x1b[#;#;#n"...
			for (size_t i = reply.size() - 1;;) {
				if (i == 0)
					break;

				--i;
				if (!isdigit(reply[i]) && reply[i] != ';' && i > 0) {
					if (reply[i] == '[' && reply[i - 1] == '\x1b') {
						reply.resize(i - 1);
						return true;
					}
					break;
				}
			}
		}
	}
}

static bool TTYFar2lExtRequest(int std_in, int std_out, std::string request, std::string &reply)
{
	struct termios ts = {};
	if (tcgetattr(std_out, &ts) != 0)
		return false;

	struct termios ts_ne = ts;
	cfmakeraw(&ts_ne);
	if (tcsetattr(std_out, TCSADRAIN, &ts_ne ) != 0)
		return false;

	request.insert(0, "\x1b_far2l");
	request+= "\x07\x1b[5n";

	bool out = TTYWriteAndWaitReplyWithStatus(std_in, std_out, request, reply);

	tcsetattr(std_out, TCSADRAIN, &ts);
	if (!out)
		return false;

	size_t p = reply.find("\x1b_far2l");
	if (p != std::string::npos) {
		reply.erase(0, p + 7);
		p = reply.find('\x07');
		if (p != std::string::npos) {
			reply.resize(p);
			return true;
		}
	}

	reply.clear();
	return true;
}

bool TTYFar2lExt_Negotiate(int std_in, int std_out, bool enable)
{
	std::string reply;
	if (!TTYFar2lExtRequest(std_in, std_out, enable ? "h\r\n" : "l\r\n", reply))
		return false;
	
	return reply == "ok";
}

bool TTYFar2lExt_Interract(int std_in, int std_out, std::vector<unsigned char> &data, std::vector<unsigned char> &reply)
{
	std::string request_str = ":", reply_str;
	if (!data.empty())
		request_str+= base64_encode(&data[0], data.size());

	if (!TTYFar2lExtRequest(std_in, std_out, request_str, reply_str))
		return false;

	if (!reply_str.empty()) {
		reply = base64_decode(reply_str);
	} else
		reply.clear();

	return true;
}
