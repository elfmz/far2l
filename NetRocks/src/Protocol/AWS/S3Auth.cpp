#include "S3Auth.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <ctime>
#include <sstream>
#include <iomanip>

static std::string HexEncode(const unsigned char *data, size_t len)
{
	std::ostringstream ss;
	ss << std::hex << std::setfill('0');
	for (size_t i = 0; i < len; ++i) {
		ss << std::setw(2) << (unsigned int)data[i];
	}
	return ss.str();
}

std::string S3SHA256Hex(const char *data, size_t len)
{
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char *>(data), len, digest);
	return HexEncode(digest, SHA256_DIGEST_LENGTH);
}

std::string S3SHA256Hex(const std::string &data)
{
	return S3SHA256Hex(data.data(), data.size());
}

static std::string HMACSHA256Raw(const std::string &key, const std::string &data)
{
	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int digest_len = 0;
	HMAC(EVP_sha256(),
		reinterpret_cast<const unsigned char *>(key.data()), (int)key.size(),
		reinterpret_cast<const unsigned char *>(data.data()), (int)data.size(),
		digest, &digest_len);
	return std::string(reinterpret_cast<char *>(digest), digest_len);
}

static std::string HMACSHA256Hex(const std::string &key, const std::string &data)
{
	std::string raw = HMACSHA256Raw(key, data);
	return HexEncode(reinterpret_cast<const unsigned char *>(raw.data()), raw.size());
}

static bool IsUnreserved(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
	       (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
}

static std::string URIEncode(const std::string &s, bool encode_slash)
{
	std::ostringstream ss;
	ss << std::hex << std::uppercase << std::setfill('0');
	for (unsigned char c : s) {
		if (IsUnreserved((char)c) || (!encode_slash && c == '/')) {
			ss << (char)c;
		} else {
			ss << '%' << std::setw(2) << (unsigned int)c;
		}
	}
	return ss.str();
}

std::string BuildQueryString(const std::map<std::string, std::string> &params)
{
	std::string result;
	for (auto &p : params) {
		if (!result.empty()) result += '&';
		result += URIEncode(p.first, true) + '=' + URIEncode(p.second, true);
	}
	return result;
}

std::map<std::string, std::string> S3SignRequest(
	const std::string &method,
	const std::string &host,
	const std::string &uri_path,
	const std::map<std::string, std::string> &query_params,
	const std::string &payload_hash,
	const std::string &region,
	const std::string &access_key,
	const std::string &secret_key)
{
	// Timestamp
	time_t now = time(nullptr);
	struct tm utc = {};
	gmtime_r(&now, &utc);
	char datestamp[9];   // YYYYMMDD
	char timestamp[17];  // YYYYMMDDTHHmmssZ
	strftime(datestamp,  sizeof(datestamp),  "%Y%m%d",       &utc);
	strftime(timestamp,  sizeof(timestamp),  "%Y%m%dT%H%M%SZ", &utc);

	std::string canonical_uri = URIEncode(uri_path.empty() ? "/" : uri_path, false);
	if (canonical_uri.empty() || canonical_uri[0] != '/') {
		canonical_uri.insert(0, "/");
	}

	std::string canonical_qs = BuildQueryString(query_params);

	std::string header_content_sha256 = "x-amz-content-sha256:" + payload_hash + "\n";
	std::string header_date           = "x-amz-date:" + std::string(timestamp) + "\n";
	std::string header_host           = "host:" + host + "\n";

	std::string canonical_headers = header_host + header_content_sha256 + header_date;
	std::string signed_headers = "host;x-amz-content-sha256;x-amz-date";

	std::string canonical_request =
		method + "\n" +
		canonical_uri + "\n" +
		canonical_qs + "\n" +
		canonical_headers + "\n" +
		signed_headers + "\n" +
		payload_hash;

	std::string scope = std::string(datestamp) + "/" + region + "/s3/aws4_request";

	std::string string_to_sign =
		"AWS4-HMAC-SHA256\n" +
		std::string(timestamp) + "\n" +
		scope + "\n" +
		S3SHA256Hex(canonical_request);

	std::string k_date    = HMACSHA256Raw("AWS4" + secret_key, datestamp);
	std::string k_region  = HMACSHA256Raw(k_date, region);
	std::string k_service = HMACSHA256Raw(k_region, "s3");
	std::string k_signing = HMACSHA256Raw(k_service, "aws4_request");

	std::string signature = HMACSHA256Hex(k_signing, string_to_sign);

	std::string auth_header =
		"AWS4-HMAC-SHA256 Credential=" + access_key + "/" + scope +
		", SignedHeaders=" + signed_headers +
		", Signature=" + signature;

	return {
		{"Authorization",          auth_header},
		{"x-amz-date",            timestamp},
		{"x-amz-content-sha256",  payload_hash},
	};
}
