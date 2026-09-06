#pragma once
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <Threaded.h>
#include <neon/ne_session.h>
#include <neon/ne_request.h>
#include "../Protocol.h"
#include "S3Repository.h"

class AWSFileReader : public IFileReader, protected Threaded
{
public:
	AWSFileReader(std::shared_ptr<S3Session> session,
	              const S3Credentials &creds,
	              const std::string &endpoint,
	              const std::string &useragent,
	              const std::string &path_prefix,
	              const std::string &key,
	              unsigned long long position,
	              unsigned long long size);

	~AWSFileReader() override;
	size_t Read(void *buf, size_t len) override; // NOSONAR(cpp:S5008)

private:
	std::shared_ptr<S3Session> _session;
	S3Credentials              _creds;
	std::string                _endpoint;
	std::string                _useragent;
	std::string                _path_prefix;
	std::string                _key;
	unsigned long long         _position;
	unsigned long long         _size;
	ne_request                *_req = nullptr;

	int         _ne_status = NE_ERROR;
	std::string _ne_error;
	bool        _done = false;

	std::deque<char>        _buf;
	std::mutex              _mtx;
	std::condition_variable _cond;

	enum { INTERMEDIATE_BUFFER = 10 * 1024 * 1024 };

	static int sReadCallback(void *userdata, const char *buf, size_t len); // NOSONAR(cpp:S5008)
	int ReadCallback(const char *buf, size_t len);

	size_t TryFetch(char *data, size_t len);

	void EnsureAllDone();

protected:
	void *ThreadProc() override; // NOSONAR(cpp:S5008)
};
