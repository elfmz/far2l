#include "TestServer.h"
#include "Backend.h"
#include "MatchWildcard.hpp"
#include "CheckedCast.hpp"
#include "os_call.hpp"
#include "WinPort.h"

TestServer::TestServer(const std::string &id)
	: _sock(LocalSocket::DATAGRAM, id)
{
	if (pipe_cloexec(_kickass) != -1) {
		MakeFDNonBlocking(_kickass[1]);
	} else {
		_kickass[0] = _kickass[1] = -1;
	}
	StartThread();
}

TestServer::~TestServer()
{
	_stop = true;
	if (os_call_ssize(write, _kickass[1], (const void*)&_stop, sizeof(_stop)) != 1) {
		perror("TestServer - write kickass");
	}
	WaitThread();
	CheckedCloseFDPair(_kickass);
}

void *TestServer::ThreadProc()
{
	fprintf(stderr, "TestServer: thread started\n");
	while (!_stop) try {
		_sock.WaitForClient(_kickass[0]);
		fprintf(stderr, "TestServer: got client\n");
		ClientLoop();
	} catch (std::exception &e) {
		fprintf(stderr, "TestServer: %s\n", e.what());
	}
	fprintf(stderr, "TestServer: thread stopped\n");
	return nullptr;
}

void TestServer::ClientLoop()
{
	for (;;) {
		size_t len = _sock.Recv(&_buf, sizeof(_buf));
		if (len < sizeof(_buf.cmd)) {
			throw std::runtime_error(StrPrintf("too small len %lu", (unsigned long)len));
		}
		fprintf(stderr, "TestServer: got command %u\n", _buf.cmd);

		switch (_buf.cmd) {
			case TEST_CMD_STATUS:
				len = ClientDispatchStatus();
				break;

			case TEST_CMD_READ_CELL:
				len = ClientDispatchReadCell(len);
				break;

			case TEST_CMD_WAIT_STRING:
				len = ClientDispatchWaitString(len);
				break;

			case TEST_CMD_SEND_KEY:
				len = ClientDispatchSendKey(len);
				break;

			default:
				throw std::runtime_error(StrPrintf("bad command %u", _buf.cmd));
		}
		if (len) {
			_sock.Send(&_buf, len);
		}
	}
}

size_t TestServer::ClientDispatchStatus()
{
	const auto &title = g_winport_con_out->GetTitle();
	UCHAR cur_height{};
	bool cur_visible{};
	COORD cur_pos = g_winport_con_out->GetCursor(cur_height, cur_visible);
	unsigned int width{}, height{};
	g_winport_con_out->GetSize(width, height);
	_buf.rep_status.version = TEST_PROTOCOL_VERSION;
	_buf.rep_status.cur_height = cur_height;
	_buf.rep_status.cur_visible = cur_visible ? 1 : 0;
	_buf.rep_status.cur_x = cur_pos.X;
	_buf.rep_status.cur_y = cur_pos.Y;
	_buf.rep_status.width = width;
	_buf.rep_status.height = height;
	for (size_t i = 0; i < ARRAYSIZE(_buf.rep_status.title); ++i) {
		_buf.rep_status.title[i] = (i < title.size()) ? title[i] : 0;
	}
	return sizeof(_buf.rep_status);
}

size_t TestServer::ClientDispatchReadCell(size_t len)
{
	if (len < sizeof(TestRequestReadCell)) {
		throw std::runtime_error(StrPrintf("len=%lu < sizeof(TestRequestReadCell)", (unsigned long)len));
	}

	CHAR_INFO ci{};
	if (g_winport_con_out->Read(ci,
			COORD{
				CheckedCast<SHORT>(_buf.req_read_cell.x),
				CheckedCast<SHORT>(_buf.req_read_cell.y)
			}
		)) {
		_buf.rep_read_cell.attributes = ci.Attributes;
		ZeroFill(_buf.rep_read_cell.str);
		std::string str;
		if (CI_USING_COMPOSITE_CHAR(ci)) {
			Wide2MB(WINPORT(CompositeCharLookup)(ci.Char.UnicodeChar), str);
		} else {
			const wchar_t wc = (wchar_t)ci.Char.UnicodeChar;
			Wide2MB(&wc, 1, str);
		}
		strncpy(_buf.rep_read_cell.str, str.c_str(), ARRAYSIZE(_buf.rep_read_cell.str) - 1);

	} else {
		ZeroFill(_buf.rep_read_cell);
	}
	return sizeof(_buf.rep_read_cell);
}

size_t TestServer::ClientDispatchWaitString(size_t len)
{
	if (len < sizeof(TestRequestWaitString)) {
		throw std::runtime_error(StrPrintf("len=%lu < sizeof(TestRequestWaitString)", (unsigned long)len));
	}
	std::vector<CHAR_INFO> data(_buf.req_wait_str.width * _buf.req_wait_str.height);
	if (data.empty()) {
		_buf.rep_wait_str.x = -1;
		_buf.rep_wait_str.y = -1;
		return sizeof(_buf.rep_wait_str);
	}

	std::wstring str;
	for (DWORD countdown = _buf.req_wait_str.timeout;;) {
		COORD data_size{
			CheckedCast<SHORT>(_buf.req_wait_str.width),
			CheckedCast<SHORT>(_buf.req_wait_str.height)
		};
		COORD data_pos{0, 0};
		SMALL_RECT screen_rect{
			CheckedCast<SHORT>(_buf.req_wait_str.left),
			CheckedCast<SHORT>(_buf.req_wait_str.top),
			CheckedCast<SHORT>(_buf.req_wait_str.left + _buf.req_wait_str.width - 1),
			CheckedCast<SHORT>(_buf.req_wait_str.top + _buf.req_wait_str.height - 1)
		};
		memset(data.data(), 0, data.size() * sizeof(CHAR_INFO));
		g_winport_con_out->Read(data.data(), data_size, data_pos, screen_rect);
		_buf.req_wait_str.str[ARRAYSIZE(_buf.req_wait_str.str) - 1] = 0;
		const auto &str_match = MB2Wide(_buf.req_wait_str.str);
		for (uint32_t y = 0; y < _buf.req_wait_str.height; ++y) {
			str.clear();
			for (uint32_t x = 0; x < _buf.req_wait_str.width; ++x) {
				const auto &ci = data[y * _buf.req_wait_str.width + x];
				if (CI_USING_COMPOSITE_CHAR(ci)) {
					str+= WINPORT(CompositeCharLookup)(ci.Char.UnicodeChar);
				} else {
					str+= (wchar_t)ci.Char.UnicodeChar;
				}
			}
			size_t pos = str.find(str_match.c_str());
			if (pos != std::wstring::npos) {
				_buf.rep_wait_str.x = 0;//TODO
				_buf.rep_wait_str.y = y;
				return sizeof(_buf.rep_wait_str);
			}
		}
		if (countdown <= 10) {
			if (!countdown) {
				_buf.rep_wait_str.x = -1;
				_buf.rep_wait_str.y = -1;
				return sizeof(_buf.rep_wait_str);
			}
			usleep(countdown * 1000);
			countdown = 0;
		} else {
			countdown-= 10;
			usleep(10000);
		}
	}
}

size_t TestServer::ClientDispatchSendKey(size_t len)
{
	if (len < sizeof(TestRequestSendKey)) {
		throw std::runtime_error(StrPrintf("len=%lu < sizeof(TestRequestSendKey)", (unsigned long)len));
	}
	INPUT_RECORD ir{};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = _buf.req_send_key.pressed ? TRUE : FALSE;
	ir.Event.KeyEvent.wRepeatCount = 1;
	ir.Event.KeyEvent.wVirtualKeyCode = _buf.req_send_key.key_code;
	ir.Event.KeyEvent.wVirtualScanCode = _buf.req_send_key.scan_code;
	ir.Event.KeyEvent.uChar.UnicodeChar = _buf.req_send_key.chr;
	ir.Event.KeyEvent.dwControlKeyState = _buf.req_send_key.controls;
	g_winport_con_in->Enqueue(&ir, 1);
	return 0;
}
