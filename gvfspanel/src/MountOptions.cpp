#include "MountOptions.h"

#include <gtkmm.h>

#include <string>
#include <locale>
#include <codecvt>
#include <iostream>

Glib::RefPtr<Gio::File> file;
Glib::RefPtr<Glib::MainLoop> main_loop;

void on_async_ready(Glib::RefPtr<Gio::AsyncResult>& result)
{
    file->mount_enclosing_volume_finish(result);

    main_loop->quit();
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
    Gio::init();
    Glib::init();

    main_loop = Glib::MainLoop::create(false);

    file = Gio::File::create_for_commandline_arg(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(this->m_resPath));
    Glib::RefPtr<Gio::MountOperation> mount_operation = Gio::MountOperation::create();
    mount_operation->set_domain("domain");
    mount_operation->set_username(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(this->m_user));
    mount_operation->set_password(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(this->m_password));

    try
    {
        file->mount_enclosing_volume(mount_operation, &on_async_ready);
    }
    catch(const Glib::Error& ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    main_loop->run();

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
