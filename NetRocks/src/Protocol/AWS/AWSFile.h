#pragma once
#include <string>
#include <aws/core/Aws.h>

class AWSFile
{
public:
	std::string name;
	bool isFile;
	timespec modified;
	long long size = 0;

	AWSFile(std::string name, bool isFile);
	AWSFile(std::string name, bool isFile, const Aws::Utils::DateTime &date, long long size);

    void UpdateModification(const Aws::Utils::DateTime &date);
};
