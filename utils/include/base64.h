#pragma once
#include <string>
#include <vector>


std::string base64_encode(const unsigned char* buf, size_t bufLen);
std::vector<unsigned char> base64_decode(const char *encoded_string, size_t in_len);
std::vector<unsigned char> base64_decode(std::string const& encoded_string);
