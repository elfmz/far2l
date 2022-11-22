#pragma once
#include <string>
#include <vector>


void base64_encode(std::string &ret, const unsigned char* buf, size_t bufLen); // appends to ret
std::string base64_encode(const unsigned char* buf, size_t bufLen);


void base64_decode(std::vector<unsigned char> &ret, const char *encoded_string, size_t in_len);
void base64_decode(std::vector<unsigned char> &ret, std::string const& encoded_string);
std::vector<unsigned char> base64_decode(const char *encoded_string, size_t in_len);
std::vector<unsigned char> base64_decode(std::string const& encoded_string);
