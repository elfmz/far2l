#include "NMBEnum.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <string>
#include <map>
#include <set>
#include <algorithm>

#include <utils.h>

#define NBNS_OP_QUERY                   0
#define NBNS_OP_REGISTER                5
#define NBNS_OP_RELEASE                 6
#define NBNS_OP_WACK                    7
#define NBNS_OP_REFRESH                 8

#define NBNS_FLG_BROADCAST              0x01
#define NBNS_FLG_RECURSION_AVAILABLE    0x08
#define NBNS_FLG_RECURSION_DESIRED      0x10
#define NBNS_FLG_TRUNCATED              0x20
#define NBNS_FLG_AUTHORITATIVE_ANSWER   0x40

#define NBNS_TIP_GENERAL                0x0020
#define NBNS_TIP_NODESTAT               0x0021

#define NBNS_CLS_INTERNET               0x0001

#define BE16(v)     ((((v) & 0xff) << 8) | (((v) >> 8) & 0xff))

struct __attribute__((__packed__)) NBNSPacket
{
	uint16_t transaction_id;
	uint16_t is_response : 1;
	uint16_t opcode     : 4; // NBNS_OP_*
	uint16_t flags      : 7; // NBNS_FLG_*
	uint16_t rcode      : 4;
	uint16_t question_count;
	uint16_t answer_count;
	uint16_t authority_record_count;
	uint16_t additional_record_count;
	uint8_t name_len; // 32 (0x20)
	uint8_t name[32];
	uint8_t name_zero_term;
	uint16_t tip; // NBNS_TIP_*
	uint16_t cls; // NBNS_CLS_*
};

static void EncodeNetbiosName(NBNSPacket &packet, const char *name, uint8_t padding, uint8_t suffix)
{
	packet.name_len = 32;
	for (uint8_t i = 0; i < packet.name_len; i+= 2) {
		uint8_t c = (uint8_t)*name;
		if (c) {
			++name;
		} else {
			c = ((i == (packet.name_len - 2) ) ? suffix : padding);
		}
		packet.name[i] = 'A' + ((c >> 4) & 0xf);
		packet.name[i + 1] = 'A' + (c & 0xf);
	}
}


void NMBEnum::Info::Recharge()
{
	next_request_time = 0;
	total_requests = 0;
}

bool NMBEnum::Info::NeedSendRequest(time_t now)
{
	if (replied || total_requests >= 3) {
		return false;
	}

	if (now < next_request_time) {
		return false;
	}

	next_request_time = now + 1;
	++total_requests;
	return true;
}

void NMBEnum::OnNodeStatusReply(uint32_t addr, const uint8_t *tail, size_t tail_len)
{
	//{ZERO:32; LEN:16; NUM:8; NODE_NAME[]}
	if (tail_len < 4 + 2 + 2)
		return;

	//uint16_t len = BE16(*(const uint16_t *)(tail + 4));
	size_t num = tail[6];

//	fprintf(stderr, "NMBEnum::len=%d num=%ld\n", len, num);
	bool new_group = false;
	for (size_t i = 0; i < num; ++i) {
		if (7 + i * 18 + 16 + 2 > tail_len) {
			fprintf(stderr, "NMBEnum::Truncated node names\n");
			break;
		}
		uint16_t flags = BE16(*(const uint16_t *)&tail[7 + i * 18 + 16]); //(*(const uint16_t *)&tail[7 + i * 18 + 16]);////
		std::string name((const char *)&tail[7 + i * 18], 15);
		while (!name.empty() && name[name.size() - 1] == ' ') {
			name.resize(name.size() - 1);
		}

		if (name.empty()) {
			;
		} else if ( (flags & (1 << 15)) == 0) { // non-group names
			if (_groups.find(name) == _groups.end() && _results.emplace(name, addr).second) {
				fprintf(stderr, "NMBEnum::Node name '%s' flags=0x%lx\n", name.c_str(), (unsigned long)flags);
			}

		} else if (!new_group && _any_group && name[0] != 1 && _groups.insert(name).second) { // new group! force another broadcast query
			new_group = true;
			_addr2info[_bcast_addr].group = name;
			_addr2info[_bcast_addr].Recharge();
			_stop_at = time(NULL) + 4;
			fprintf(stderr, "NMBEnum::Group name='%s' flags=0x%lx\n", name.c_str(), (unsigned long)flags);
		}
	}
}

NMBEnum::NMBEnum(const std::string &group)
	:
	_bcast_addr(htonl(INADDR_BROADCAST)),
	_ns_port(htons(137)),
	_any_group(group.empty() || group == "*")
{
	_addr2info[_bcast_addr].group = _any_group ? "*" : group;
	_groups.insert(_addr2info[_bcast_addr].group);

	_sc = socket(AF_INET, SOCK_DGRAM, 0);
	if (_sc != -1) {
		int one = 1;
		if (setsockopt(_sc, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) == -1) {
			perror("NetBiosNSEnum - SO_BROADCAST");
		}
		struct timeval tv = {};
		tv.tv_usec = 100000;
		if (setsockopt(_sc, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
			perror("NetBiosNSEnum - SO_RCVTIMEO");
		}

		if (!StartThread()) {
			perror("NetBiosNSEnum - thread");
		}
	} else {
		perror("NetBiosNSEnum - socket");
	}
}

NMBEnum::~NMBEnum()
{
	WaitThread();

	if (_sc != -1)
		close(_sc);
}

const Name2IPV4 &NMBEnum::WaitResults()
{
	WaitThread();
	return _results;
}

void *NMBEnum::ThreadProc()
{
	struct __attribute__((__packed__)) {
		NBNSPacket packet;
		uint8_t tail[0x1000];
	} s = {};

	_stop_at = time(NULL) + 4;
	for (;;) {
		time_t now = time(NULL);
		if (now >= _stop_at)
			break;

		struct sockaddr_in sin = {};
		for (auto &i : _addr2info) {
			if (i.second.NeedSendRequest(now)) {
				sin.sin_family = AF_INET;
				sin.sin_port = _ns_port;//i.second.port;
				sin.sin_addr.s_addr = i.first;

				ZeroFill(s.packet);
				fprintf(stderr, "NMBEnum::Query '%s' to 0x%x\n", i.second.group.c_str(), i.first);
				EncodeNetbiosName(s.packet, i.second.group.c_str(), 0x20, 0);

				s.packet.transaction_id = (rand() & 0xffff);
				s.packet.question_count = BE16(1);
				s.packet.cls = BE16(NBNS_CLS_INTERNET);

				if (sin.sin_addr.s_addr == _bcast_addr) {
					//s.packet.flags = 0x11NBNS_FLG_BROADCAST;
					s.packet.tip = BE16(NBNS_TIP_GENERAL);
				} else {
					s.packet.tip = BE16(NBNS_TIP_NODESTAT);
				}

				if (sendto(_sc, &s, sizeof(s.packet), 0, (struct sockaddr *)&sin, sizeof(sin)) <= 0) {
					perror("sendto");
				}
			}
		}

		socklen_t sin_len = sizeof(sin);
		ssize_t r = recvfrom(_sc, &s, sizeof(s), 0, (sockaddr *)&sin, &sin_len);
		if (r >= (ssize_t)sizeof(s.packet)
			&& sin.sin_family == AF_INET && sin.sin_addr.s_addr != _bcast_addr
			&& (s.packet.is_response || s.packet.answer_count)) {
			//Info &info = _addr2info[sin.sin_addr.s_addr];
			if (s.packet.tip == BE16(NBNS_TIP_GENERAL)
			&& s.packet.cls == BE16(NBNS_CLS_INTERNET)) {
				_addr2info[sin.sin_addr.s_addr].group = _addr2info[_bcast_addr].group;//.port = sin.sin_port;
			} else {
				const auto &it = _addr2info.find(sin.sin_addr.s_addr);
				if (it != _addr2info.end() && !it->second.replied
				 && s.packet.tip == BE16(NBNS_TIP_NODESTAT)
				 && s.packet.cls == BE16(NBNS_CLS_INTERNET)) {
					it->second.replied = true;
					OnNodeStatusReply(sin.sin_addr.s_addr, s.tail, (size_t)r - sizeof(s.packet));
					fprintf(stderr, "NMBEnum::Replied 0x%x {%ld)\n", it->first, (unsigned long)r);
				}
			}
		}
	}

	return nullptr;
}
