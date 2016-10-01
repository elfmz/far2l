#include "MountOptions.h"

#include <string>

MountPoint::MountPoint(const std::wstring &resPath, const std::wstring &login, const std::wstring &password) :
    m_resPath(resPath),
    m_user(login),
    m_password(password)
{

}

std::wstring &MountPoint::getFsPath()
{
    return this->m_mountPointPath;
}

MountPoint::FileSystem MountPoint::getFsType() const
{
    return this->type;
}
