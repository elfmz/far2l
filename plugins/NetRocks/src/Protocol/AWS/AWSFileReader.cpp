#include "AWSFileReader.h"
#include "S3Auth.h"
#include <neon/ne_uri.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <algorithm>

AWSFileReader::AWSFileReader(std::shared_ptr<S3Session> session,
                             const S3Credentials &creds,
                             const std::string &endpoint,
                             const std::string &useragent,
                             const std::string &path_prefix,
                             const std::string &key,
                             unsigned long long position,
                             unsigned long long size)
	: _session(std::move(session)), _creds(creds), _endpoint(endpoint),
	  _useragent(useragent), _path_prefix(path_prefix), _key(key),
	  _position(position), _size(size)
{
	std::string uri_path = _path_prefix.empty() ? "/" + _key : _path_prefix + "/" + _key;
	std::string payload_hash = S3SHA256Hex("");
	auto auth_headers = S3SignRequest("GET", _endpoint, uri_path, {},
	                                  payload_hash, _creds.region,
	                                  _creds.access_key, _creds.secret_key);

	std::unique_ptr<char, decltype(&free)> escaped(ne_path_escape(uri_path.c_str()), free);
	std::string neon_path = escaped ? escaped.get() : uri_path;

	_req = ne_request_create(_session->sess, "GET", neon_path.c_str());
	if (!_req) throw ProtocolError("Failed to create GET request");

	for (auto &h : auth_headers) {
		ne_add_request_header(_req, h.first.c_str(), h.second.c_str());
	}
	if (!_useragent.empty()) {
		ne_add_request_header(_req, "User-Agent", _useragent.c_str());
	}

	if (_position > 0 || _size > 0) {
		char range[64] = {};
		if (_size > 0 && _size > _position) {
			snprintf(range, sizeof(range) - 1, "bytes=%llu-%llu",
			         (unsigned long long)_position, (unsigned long long)(_size - 1));
		} else {
			snprintf(range, sizeof(range) - 1, "bytes=%llu-",
			         (unsigned long long)_position);
		}
		ne_add_request_header(_req, "Range", range);
	}

	ne_add_response_body_reader(_req, ne_accept_always, sReadCallback, this);

	if (!StartThread()) {
		ne_request_destroy(_req);
		_req = nullptr;
		throw ProtocolError("Failed to start download thread");
	}
}

AWSFileReader::~AWSFileReader()
{
	EnsureAllDone();
	if (_req) {
		ne_request_destroy(_req);
		_req = nullptr;
	}
}

void AWSFileReader::EnsureAllDone()
{
	{
		std::unique_lock<std::mutex> lock(_mtx);
		_done = true;
		_cond.notify_all();
	}
	WaitThread();
}

void *AWSFileReader::ThreadProc() // NOSONAR(cpp:S5008)
{
	_ne_status = ne_request_dispatch(_req);
	std::unique_lock<std::mutex> lock(_mtx);
	if (_ne_status != NE_OK) {
		const char *err = ne_get_error(_session->sess);
		_ne_error = err ? err : "download failed";
	}
	_done = true;
	_cond.notify_all();
	return nullptr;
}

int AWSFileReader::sReadCallback(void *userdata, const char *buf, size_t len) // NOSONAR(cpp:S5008)
{
	return reinterpret_cast<AWSFileReader *>(userdata)->ReadCallback(buf, len);
}

int AWSFileReader::ReadCallback(const char *buf, size_t len)
{
	std::unique_lock<std::mutex> lock(_mtx);
	for (;;) {
		if (_done) return -1;
		if (len == 0) return 0;
		if (_buf.size() < (size_t)INTERMEDIATE_BUFFER) {
			_buf.insert(_buf.end(), buf, buf + len);
			_cond.notify_all();
			return 0;
		}
		_cond.wait(lock, [this]{ return _done || _buf.size() < (size_t)INTERMEDIATE_BUFFER; });
	}
}

size_t AWSFileReader::TryFetch(char *data, size_t len)
{
	len = std::min(len, _buf.size());
	if (len) {
		std::copy(_buf.begin(), _buf.begin() + len, data);
		_buf.erase(_buf.begin(), _buf.begin() + len);
	}
	return len;
}

size_t AWSFileReader::Read(void *buf, size_t buflen) // NOSONAR(cpp:S5008)
{
	if (buflen == 0) return 0;

	std::unique_lock<std::mutex> lock(_mtx);
	for (;;) {
		size_t got = TryFetch(static_cast<char *>(buf), buflen);
		if (got) {
			_cond.notify_all();
			return got;
		}
		if (_done) {
			if (_ne_status != NE_OK) {
				throw ProtocolError("Read error", _ne_error.c_str(), _ne_status);
			}
			return 0;
		}
		_cond.wait(lock, [this]{ return _done || !_buf.empty(); });
	}
}
