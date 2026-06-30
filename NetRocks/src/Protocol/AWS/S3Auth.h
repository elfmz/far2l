#pragma once
#include <string>
#include <map>

std::string BuildQueryString(const std::map<std::string, std::string> &params);

// AWS Signature V4
std::map<std::string, std::string> S3SignRequest(
	const std::string &method,
	const std::string &host,
	const std::string &uri_path,
	const std::map<std::string, std::string> &query_params,
	const std::string &payload_hash,
	const std::string &region,
	const std::string &access_key,
	const std::string &secret_key
);

std::string S3SHA256Hex(const std::string &data);
std::string S3SHA256Hex(const char *data, size_t len);
