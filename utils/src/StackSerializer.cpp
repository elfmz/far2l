#include <string.h>
#include "StackSerializer.h"
#include "base64.h"

StackSerializer::StackSerializer()
{
}

StackSerializer::StackSerializer(const char *str, size_t len)
{
	FromBase64(str, len);
}


StackSerializer::StackSerializer(const std::string &str)
{
	FromBase64(str);
}

void StackSerializer::FromBase64(const char *str, size_t len)
{
	_data.clear();
	if (len)
		base64_decode(_data, str, len);
}

void StackSerializer::FromBase64(const std::string &str)
{
	FromBase64(str.c_str(), str.size());
}

void StackSerializer::ToBase64(std::string &str) const
{
	str.clear();
	if (!_data.empty())
		base64_encode(str, &_data[0], _data.size());
}

std::string StackSerializer::ToBase64() const
{
	std::string str;
	if (!_data.empty())
		base64_encode(str, &_data[0], _data.size());
	return str;
}

void StackSerializer::Clear()
{
	_data.clear();
}

void StackSerializer::Swap(StackSerializer &other)
{
	_data.swap(other._data);
}

bool StackSerializer::IsEmpty() const
{
	return _data.empty();
}

///////////////

void StackSerializer::Push(const void *ptr, size_t len)
{
	size_t sz = _data.size();
	_data.resize(sz + len);
	memcpy(&_data[sz], ptr, len);
}

void StackSerializer::Pop(void *ptr, size_t len)
{
	size_t sz = _data.size();
	if (len > sz)
		throw std::range_error("StackSerializer::Pop: too big");

	memcpy(ptr, &_data[sz - len], len);
	_data.resize(sz - len);
}

///

void StackSerializer::PushStr(const char *str)
{
	uint32_t str_sz = (uint32_t)strlen(str);
	if (str_sz)
		Push(str, str_sz);
	PushNum(str_sz);
}

void StackSerializer::PushStr(const std::string &str)
{
	uint32_t str_sz = (uint32_t)str.size();
	if ( (size_t)str_sz != str.size())
		throw std::range_error("StackSerializer::PushStr: too long");

	if (str_sz)
		Push(str.c_str(), str_sz);
	PushNum(str_sz);
}

void StackSerializer::PopStr(std::string &str)
{
	uint32_t str_sz = 0;
	PopNum(str_sz);
	str.resize(str_sz);
	if (str_sz)
		Pop(&str[0], str_sz);
}

std::string StackSerializer::PopStr()
{
	std::string out;
	PopStr(out);
	return out;
}

///
char StackSerializer::PopChar()
{
	char out;
	PopNum(out);
	return out;
}

uint8_t StackSerializer::PopU8()
{
	uint8_t out;
	PopNum(out);
	return out;
}

uint16_t StackSerializer::PopU16()
{
	uint16_t out;
	PopNum(out);
	return out;
}

uint32_t StackSerializer::PopU32()
{
	uint32_t out;
	PopNum(out);
	return out;
}

