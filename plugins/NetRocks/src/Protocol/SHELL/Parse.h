#pragma once
#include <sys/types.h>
#include <vector>
#include <string>
#include "../../FileInformation.h"
#include <cstdint>


struct FileInfo
{
	mode_t mode{0};
	unsigned long long size{0};
	std::string owner;
	std::string group;
	std::string path;
	timespec access_time{}, modification_time{}, status_change_time{};
};

void SHELLParseEnumByStatOrFind(std::vector<FileInfo> &files, const std::vector<std::string> &lines);
void SHELLParseInfoByStatOrFind(FileInformation &fi, std::string &line);
uint64_t SHELLParseSizeByStatOrFind(std::string &line);
uint32_t SHELLParseModeByStatOrFind(std::string &line);

void SHELLParseEnumByLS(std::vector<FileInfo> &files, const std::vector<std::string> &lines);
void SHELLParseInfoByLS(FileInformation &fi, std::string &line);
uint64_t SHELLParseSizeByLS(std::string &line);
uint32_t SHELLParseModeByLS(std::string &line);

void AppendTrimmedLines(std::string &s, const std::vector<std::string> &lines);
void Substitute(std::string &str, const char *pattern, const std::string &replacement);
