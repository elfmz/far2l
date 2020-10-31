#pragma once
#include <string>
#include <memory>
#include <LocalSocket.h>

class ISudoAskpass
{
public:
	virtual bool OnSudoAskPassword(const std::string &title, const std::string &text, std::string &password) = 0;
	virtual bool OnSudoConfirm(const std::string &title, const std::string &text) = 0;
};


enum SudoAskpassResult
{
	SAR_OK = 0,
	SAR_CANCEL = 1,
	SAR_FAILED = 0x100
};

class SudoAskpassServer
{
	ISudoAskpass *_isa;
	int _kickass[2];
	std::unique_ptr<LocalSocketServer> _sock;

	std::string _srv;
	struct Buffer
	{
		unsigned char code;
		char str[0xfff];
	};

	pthread_t _trd;
	bool _active = false;

	static size_t sFillBuffer(Buffer &buf, unsigned char code, const std::string &str);
	static void *sThread(void *p) { ((SudoAskpassServer *)p)->Thread(); return nullptr; }
	void Thread();

	size_t OnRequest(Buffer &buf, size_t len);

	public:
	SudoAskpassServer(ISudoAskpass *isa);
	~SudoAskpassServer();

	static SudoAskpassResult sRequestToServer(unsigned char code, std::string &str);
};

///

SudoAskpassResult SudoAskpassRequestPassword(std::string &password);
SudoAskpassResult SudoAskpassRequestConfirmation();
