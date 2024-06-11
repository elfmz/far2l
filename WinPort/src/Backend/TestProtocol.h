#pragma once

#define TEST_PROTOCOL_VERSION	0xF001

enum TestCommand
{
	TEST_CMD_DETACH = 0,
	TEST_CMD_STATUS = 1,
	TEST_CMD_READ_CELL,
	TEST_CMD_WAIT_STRING,
	TEST_CMD_WAIT_NO_STRING,
	TEST_CMD_SEND_KEY,
	TEST_CMD_SYNC,
};

struct TestReplyStatus
{
	uint16_t version; // TEST_PROTOCOL_VERSION
	uint8_t cur_height;
	uint8_t cur_visible;
	uint32_t cur_x;
	uint32_t cur_y;
	uint32_t width;
	uint32_t height;
	char title[2048]; // truncated if longer
};

struct TestRequestReadCell
{
	uint32_t cmd;
	uint32_t x;
	uint32_t y;
};

struct TestReplyReadCell
{
	uint64_t attributes;
	char str[2048]; // more than one UTF character if cell contains composite character
};

struct TestRequestWaitString
{
	uint32_t cmd;
	uint32_t timeout; // msec
	uint32_t left;
	uint32_t top;
	uint32_t width;
	uint32_t height;
	char str[2048]; // double NULL-terminated array of strings of total maximum length 2048
};

struct TestReplyWaitString
{ // string index and position or {-1, -1, -1} if string not found before wait timed out
	uint32_t i;
	uint32_t x;
	uint32_t y;
};

struct TestRequestSendKey
{
	uint32_t cmd;
	uint32_t controls;
	uint32_t chr;
	uint32_t key_code;
	uint32_t scan_code;
	uint8_t  pressed;
	uint8_t  reserved[3];
};

struct TestRequestSync
{
	uint32_t cmd;
	uint32_t timeout; // msec
};

struct TestReplySync
{
	uint8_t waited;
};

