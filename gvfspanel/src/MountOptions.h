#pragma once

#include <string>

class MountPoint
{
private:

    std::wstring m_mountPointPath;
    std::wstring m_shareName;

    enum class FileSystem {
        DiskFs,
        Scp,
        Nfs,
        Samba,
        WebDav
    } type;
    bool m_bMounted;

public:
    MountPoint();
    MountPoint(const std::wstring& resPath, const std::wstring& login, const std::wstring& password);

public:
    bool isMounted();
    bool mount();
    bool unmount();

public:
    std::wstring m_resPath;
    std::wstring m_user;
    std::wstring m_password;

    std::wstring& getFsPath();
    FileSystem getFsType() const;
};

