#pragma once
#include <vector>
#include <string>

struct FileInfo
{
    int permissions{0};
    std::string owner;
    std::string group;
    uint64_t size{0};
    std::string path;
    std::string symlink_path;
    bool is_directory{false};
};

void FISHParseLS(std::vector<FileInfo> &files, const std::string &buffer);
