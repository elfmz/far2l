#pragma once
#include <utils.h>
#include <Threaded.h>
#include <atomic>
#include <string>
#include "TestProtocol.h"

class TestController : protected Threaded
{
	std::atomic<bool> _stop{false};
	std::string _ipc_server;

	union TestBuf
	{
		uint32_t cmd;

		TestReplyStatus rep_status;

		TestRequestReadCell req_read_cell;
		TestReplyReadCell rep_read_cell;

		TestRequestWaitString req_wait_str;
		TestReplyWaitString rep_wait_str;

		TestRequestSendKey req_send_key;

		TestRequestSync req_sync;
		TestReplySync rep_sync;
	} _buf;

	virtual void *ThreadProc();
	void ClientLoop(const std::string &ipc_client);
	size_t ClientDispatchStatus();
	size_t ClientDispatchReadCell(size_t len);
	size_t ClientDispatchWaitString(size_t len, bool need_presence);
	size_t ClientDispatchSendKey(size_t len);
	size_t ClientDispatchSync(size_t len);

public:
	TestController(const std::string &id);
	~TestController();
};
