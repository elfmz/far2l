#pragma once
#include <string>
#include <ctime>

class AWSFile
{
public:
	std::string name;
	bool isFile;
	timespec modified;
	long long size = 0;

	AWSFile(std::string name, bool isFile);
	AWSFile(std::string name, bool isFile, const std::string &iso8601_date, long long size);

	void UpdateModification(const std::string &iso8601_date);

	static timespec FromISO8601(const std::string &s);
};
