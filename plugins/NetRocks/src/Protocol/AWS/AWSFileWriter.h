#pragma once
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <Threaded.h>
#include <neon/ne_session.h>
#include <neon/ne_request.h>
#include "../Protocol.h"
#include "S3Repository.h"

class AWSFileWriter : public IFileWriter
{
public:
	AWSFileWriter(std::shared_ptr<S3Session> session,
	              const S3Credentials &creds,
	              const std::string &endpoint,
	              const std::string &useragent,
	              const std::string &path_prefix,
	              const std::string &key);
	~AWSFileWriter() override;

	void Write(const void *buf, size_t len) override; // NOSONAR(cpp:S5008)
	void WriteComplete() override;

private:
	std::shared_ptr<S3Session> _session;
	S3Credentials              _creds;
	std::string                _endpoint;
	std::string                _useragent;
	std::string                _path_prefix;
	std::string                _key;

	std::string              _upload_id;
	std::vector<char>        _buffer;
	int                      _part_number = 1;
	std::vector<std::string> _etags;   // one per completed part
	bool                     _completed = false;
	bool                     _aborted   = false;

	static constexpr size_t MAX_PART_SIZE = 5 * 1024 * 1024;

	std::string DoRequest(const std::string &method,
	                      const std::string &path,
	                      const std::map<std::string, std::string> &query_params,
	                      const std::string &body,
	                      const std::string &capture_response_header = "");

	void StartMultipartUpload();
	void FlushPart();
	void CompleteMultipartUpload();
	void AbortMultipartUpload();
};
