#pragma once
#include <vector>
#include <string>

struct FileInfo
{
    unsigned int mode{0};
    std::string owner;
    std::string group;
    uint64_t size{0};
    std::string path;
};

void FISHParseLS(std::vector<FileInfo> &files, const std::string &buffer);
