#pragma once
#include <stdint.h>
#include <time.h>
#include <map>
#include <set>
#include <string>
#include <Threaded.h>

typedef std::map<std::string, uint32_t> Name2IPV4;

class NMBEnum : protected Threaded
{
	class Info
	{
		time_t next_request_time = 0;
		unsigned int total_requests = 0;

	public:
		bool replied = false;
		std::string group;

		void Recharge();
		bool NeedSendRequest(time_t now);
	};

	const uint32_t _bcast_addr;
	const uint16_t _ns_port;
	const bool _any_group;

	Name2IPV4 _results;
	struct Addr2Info : std::map<uint32_t, Info> {} _addr2info;
	std::set<std::string> _groups;
	time_t _stop_at = 0;
	int _sc = -1;

	void OnNodeStatusReply(uint32_t addr, const uint8_t *tail, size_t tail_len);

	virtual void *ThreadProc();

public:
	NMBEnum(const std::string &group);
	virtual ~NMBEnum();

	const Name2IPV4 &WaitResults();
};
