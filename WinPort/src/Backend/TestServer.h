#pragma once
#include <utils.h>
#include <LocalSocket.h>
#include <Threaded.h>
#include <atomic>
#include "TestProtocol.h"

class TestServer : protected Threaded
{
	LocalSocketServer _sock;
	int _kickass[2];
	std::atomic<bool> _stop{false};

	union TestBuf
	{
		uint32_t cmd;

		TestReplyStatus rep_status;

		TestRequestReadCell req_read_cell;
		TestReplyReadCell rep_read_cell;

		TestRequestWaitString req_wait_str;
		TestReplyWaitString rep_wait_str;

		TestRequestSendKey req_send_key;

	} _buf;

	virtual void *ThreadProc();
	void ClientLoop();
	size_t ClientDispatchStatus();
	size_t ClientDispatchReadCell(size_t len);
	size_t ClientDispatchWaitString(size_t len);
	size_t ClientDispatchSendKey(size_t len);

public:
	TestServer(const std::string &id);
	~TestServer();
};
