#pragma once

#include <string>

class MountOptions
{
public:
    MountOptions();

    std::string user;
    std::string password;

    enum class FileSystem {
        DiskFs,
        Scp,
        Nfs,
        Samba,
        WebDav
    };
};

