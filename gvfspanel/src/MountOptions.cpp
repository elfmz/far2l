#include "MountOptions.h"

#include "GvfsService.h"

#include <string>
#include <locale>
#include <codecvt>
#include <iostream>


MountPoint::MountPoint() :
    m_bMounted(false)
{

}

MountPoint::MountPoint(const std::wstring &resPath, const std::wstring &login, const std::wstring &password) :
    m_bMounted(false),
    m_resPath(resPath),
    m_user(login),
    m_password(password)
{

}

bool MountPoint::isMounted()
{
    return m_bMounted;
}

bool MountPoint::mount()
{
    std::string domain = "domain";
    std::string resPath = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(this->m_resPath);
    std::string userName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(this->m_user);
    std::string password = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(this->m_password);

    GvfsService service;
    service.mount(resPath, userName, password);
    return true;
}

bool MountPoint::unmount()
{
    return false;
}

std::wstring &MountPoint::getFsPath()
{
    return this->m_mountPointPath;
}

MountPoint::FileSystem MountPoint::getFsType() const
{
    return this->type;
}
