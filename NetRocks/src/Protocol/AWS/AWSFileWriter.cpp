#include "AWSFileWriter.h"
#include "S3Auth.h"
#include <neon/ne_uri.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <map>

struct WriteResponseCapture {
	std::string body;
	std::string header_name;
	std::string header_value;

	static int sBodyReader(void *ud, const char *buf, size_t len) // NOSONAR(cpp:S5008)
	{
		reinterpret_cast<WriteResponseCapture *>(ud)->body.append(buf, len);
		return 0;
	}
};

static void CheckXmlError(const std::string &body, const std::string &ctx)
{
	std::string code, msg;
	auto extract = [&](const std::string &tag) -> std::string {
		std::string o = "<" + tag + ">", c = "</" + tag + ">";
		auto s = body.find(o);
		if (s == std::string::npos) return "";
		s += o.size();
		auto e = body.find(c, s);
		return (e == std::string::npos) ? "" : body.substr(s, e - s);
	};
	code = extract("Code");
	msg  = extract("Message");
	if (code == "AccessDenied" || code == "InvalidAccessKeyId" ||
	    code == "SignatureDoesNotMatch") {
		throw ProtocolAuthFailedError(code + ": " + msg);
	}
	if (!code.empty()) {
		throw ProtocolError((ctx.empty() ? code : ctx + ": " + code) +
		                    (msg.empty() ? "" : " - " + msg));
	}
}

std::string AWSFileWriter::DoRequest(const std::string &method,
                                     const std::string &path,
                                     const std::map<std::string, std::string> &query_params,
                                     const std::string &body,
                                     const std::string &capture_response_header)
{
	std::string payload_hash = S3SHA256Hex(body);
	auto auth_headers = S3SignRequest(method, _endpoint, path, query_params,
	                                  payload_hash, _creds.region,
	                                  _creds.access_key, _creds.secret_key);

	std::unique_ptr<char, decltype(&free)> escaped(ne_path_escape(path.c_str()), free);
	std::string neon_path = escaped ? escaped.get() : path;
	std::string query_str = BuildQueryString(query_params);
	if (!query_str.empty()) neon_path += "?" + query_str;

	WriteResponseCapture cap;
	cap.header_name = capture_response_header;

	ne_request *req = ne_request_create(_session->sess, method.c_str(), neon_path.c_str());
	if (!req) throw ProtocolError("Failed to create neon request");

	for (auto &h : auth_headers) {
		ne_add_request_header(req, h.first.c_str(), h.second.c_str());
	}
	if (!_useragent.empty()) {
		ne_add_request_header(req, "User-Agent", _useragent.c_str());
	}

	std::string body_copy = body;
	if (!body_copy.empty()) {
		ne_set_request_body_buffer(req, body_copy.c_str(), body_copy.size());
		char cl[32];
		snprintf(cl, sizeof(cl), "%zu", body_copy.size());
		ne_add_request_header(req, "Content-Length", cl);
	} else if (method == "POST") {
		ne_add_request_header(req, "Content-Length", "0");
	}

	ne_add_response_body_reader(req, ne_accept_always, WriteResponseCapture::sBodyReader, &cap);
	int rc = ne_request_dispatch(req);

	if (!capture_response_header.empty()) {
		const char *hv = ne_get_response_header(req, capture_response_header.c_str());
		if (hv) cap.header_value = hv;
	}

	const ne_status *st = ne_get_status(req);
	int http_code = st ? st->code : 0;
	ne_request_destroy(req);

	if (rc != NE_OK) {
		const char *err = ne_get_error(_session->sess);
		throw ProtocolError(err ? err : "S3 write request failed");
	}
	if (http_code < 200 || http_code >= 300) {
		throw ProtocolError("S3 HTTP error: " + std::to_string(http_code) +
		                    " on " + method + " " + path);
	}

	CheckXmlError(cap.body, method + " " + path);

	return capture_response_header.empty() ? cap.body : cap.header_value;
}

AWSFileWriter::AWSFileWriter(std::shared_ptr<S3Session> session,
                             const S3Credentials &creds,
                             const std::string &endpoint,
                             const std::string &useragent,
                             const std::string &path_prefix,
                             const std::string &key)
	: _session(std::move(session)), _creds(creds), _endpoint(endpoint),
	  _useragent(useragent), _path_prefix(path_prefix), _key(key)
{
	StartMultipartUpload();
}

AWSFileWriter::~AWSFileWriter()
{
	if (!_completed && !_aborted) {
		try {
			AbortMultipartUpload();
		} catch (...) { /* must not throw from destructor */ }
	}
}

void AWSFileWriter::StartMultipartUpload()
{
	std::string path = _path_prefix.empty() ? "/" + _key : _path_prefix + "/" + _key;
	std::string body = DoRequest("POST", path, {{"uploads", ""}}, "");
	// Parse <UploadId>
	std::string o = "<UploadId>", c = "</UploadId>";
	auto s = body.find(o);
	if (s == std::string::npos) throw ProtocolError("CreateMultipartUpload: no UploadId in response");
	s += o.size();
	auto e = body.find(c, s);
	if (e == std::string::npos) throw ProtocolError("CreateMultipartUpload: malformed response");
	_upload_id = body.substr(s, e - s);
}

void AWSFileWriter::FlushPart()
{
	if (_buffer.empty()) return;

	std::string path = _path_prefix.empty() ? "/" + _key : _path_prefix + "/" + _key;
	std::map<std::string, std::string> qp = {
		{"partNumber", std::to_string(_part_number)},
		{"uploadId",   _upload_id}
	};
	std::string body(_buffer.data(), _buffer.size());

	std::string payload_hash = S3SHA256Hex(body.data(), body.size());
	auto auth_headers = S3SignRequest("PUT", _endpoint, path, qp,
	                                  payload_hash, _creds.region,
	                                  _creds.access_key, _creds.secret_key);

	std::unique_ptr<char, decltype(&free)> escaped(ne_path_escape(path.c_str()), free);
	std::string neon_path = (escaped ? escaped.get() : path) + "?" + BuildQueryString(qp);

	WriteResponseCapture cap;
	cap.header_name = "ETag";

	ne_request *req = ne_request_create(_session->sess, "PUT", neon_path.c_str());
	if (!req) throw ProtocolError("Failed to create PUT part request");

	for (auto &h : auth_headers) {
		ne_add_request_header(req, h.first.c_str(), h.second.c_str());
	}
	if (!_useragent.empty()) {
		ne_add_request_header(req, "User-Agent", _useragent.c_str());
	}
	ne_set_request_body_buffer(req, _buffer.data(), _buffer.size());
	char cl[32];
	snprintf(cl, sizeof(cl), "%zu", _buffer.size());
	ne_add_request_header(req, "Content-Length", cl);
	ne_add_response_body_reader(req, ne_accept_always, WriteResponseCapture::sBodyReader, &cap);

	int rc = ne_request_dispatch(req);

	const char *etag_hdr = ne_get_response_header(req, "ETag");
	std::string etag = etag_hdr ? etag_hdr : "";
	const ne_status *st = ne_get_status(req);
	int http_code = st ? st->code : 0;
	ne_request_destroy(req);

	if (rc != NE_OK) {
		const char *err = ne_get_error(_session->sess);
		throw ProtocolError(err ? err : "UploadPart failed");
	}
	if (http_code < 200 || http_code >= 300) {
		throw ProtocolError("UploadPart HTTP error: " + std::to_string(http_code));
	}
	CheckXmlError(cap.body, "UploadPart");
	if (etag.empty()) throw ProtocolError("UploadPart: missing ETag in response");

	_etags.push_back(etag);
	++_part_number;
	_buffer.clear();
}

void AWSFileWriter::CompleteMultipartUpload()
{
	if (_buffer.size() > 0) FlushPart();
	if (_etags.empty()) {
		std::string path = _path_prefix.empty() ? "/" + _key : _path_prefix + "/" + _key;
		std::map<std::string, std::string> qp = {{"partNumber", "1"}, {"uploadId", _upload_id}};
		std::string payload_hash = S3SHA256Hex("");
		auto auth_headers = S3SignRequest("PUT", _endpoint, path, qp,
		                                  payload_hash, _creds.region,
		                                  _creds.access_key, _creds.secret_key);
		std::unique_ptr<char, decltype(&free)> escaped(ne_path_escape(path.c_str()), free);
		std::string neon_path = (escaped ? escaped.get() : path) + "?" + BuildQueryString(qp);

		WriteResponseCapture cap;
		ne_request *req = ne_request_create(_session->sess, "PUT", neon_path.c_str());
		if (!req) throw ProtocolError("Failed to create zero-byte part request");
		for (auto &h : auth_headers) {
			ne_add_request_header(req, h.first.c_str(), h.second.c_str());
		}
		ne_add_request_header(req, "Content-Length", "0");
		ne_add_response_body_reader(req, ne_accept_always, WriteResponseCapture::sBodyReader, &cap);
		int rc = ne_request_dispatch(req);
		const char *etag_hdr = ne_get_response_header(req, "ETag");
		std::string etag = etag_hdr ? etag_hdr : "";
		const ne_status *st = ne_get_status(req);
		int http_code = st ? st->code : 0;
		ne_request_destroy(req);
		if (rc != NE_OK) throw ProtocolError("UploadPart(0) failed");
		if (http_code < 200 || http_code >= 300)
			throw ProtocolError("UploadPart(0) HTTP error: " + std::to_string(http_code));
		if (etag.empty()) throw ProtocolError("UploadPart(0): missing ETag");
		_etags.push_back(etag);
	}

	std::string xml = "<CompleteMultipartUpload>";
	for (size_t i = 0; i < _etags.size(); ++i) {
		xml += "<Part><PartNumber>" + std::to_string(i + 1) +
		       "</PartNumber><ETag>" + _etags[i] + "</ETag></Part>";
	}
	xml += "</CompleteMultipartUpload>";

	std::string path = _path_prefix.empty() ? "/" + _key : _path_prefix + "/" + _key;
	DoRequest("POST", path, {{"uploadId", _upload_id}}, xml);
	_completed = true;
}

void AWSFileWriter::AbortMultipartUpload()
{
	if (_upload_id.empty()) return;
	try {
		std::string path = _path_prefix.empty() ? "/" + _key : _path_prefix + "/" + _key;
		DoRequest("DELETE", path, {{"uploadId", _upload_id}}, "");
	} catch (...) { /* best-effort abort — ignore errors */ } // NOSONAR(cpp:S2221)
	_aborted = true;
}

void AWSFileWriter::Write(const void *buf, size_t len) // NOSONAR(cpp:S5008)
{
	const char *data = reinterpret_cast<const char *>(buf);
	while (len > 0) {
		size_t space = MAX_PART_SIZE - _buffer.size();
		size_t chunk = std::min(len, space);
		_buffer.insert(_buffer.end(), data, data + chunk);
		data += chunk;
		len -= chunk;
		if (_buffer.size() >= MAX_PART_SIZE)
			FlushPart();
	}
}

void AWSFileWriter::WriteComplete()
{
	CompleteMultipartUpload();
}
