#pragma once
#include <string>
#include <utils.h>

class Request
{
	std::string _request;

public:
	Request(const char *cmd_ending); // expecting ending at the end of cmd
	Request &Add(const std::string &arg, char ending); // appends escaped arg and ending
	Request &AddFmt(const char *cmd_ending, ...); // expecting ending inside and not escaped!

	inline operator const std::string &() const
	{
		return _request;
	}
};
