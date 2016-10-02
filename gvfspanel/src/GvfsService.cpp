#include "GvfsService.h"

#include <gtkmm.h>

#include <string>
#include <iostream>

void GvfsService::mount(const std::string &resPath, const std::string &userName, const std::string &password)
{
    Gio::init();
    Glib::init();

    main_loop = Glib::MainLoop::create(false);

    file = Gio::File::create_for_parse_name(resPath);
    Glib::RefPtr<Gio::MountOperation> mount_operation = Gio::MountOperation::create();
    mount_operation->set_domain("WORKGROUP");
    mount_operation->set_username(userName);
    mount_operation->set_password(password);

    mount_operation->set_anonymous(true);

    try
    {
        file->mount_enclosing_volume(mount_operation,
                                     [this] (Glib::RefPtr<Gio::AsyncResult>& result)
                                     {
                                        this->mount_cb(result);
                                     });
        m_mountCount++;
        std::cerr << "inc m_mountCount: " << m_mountCount;
        if(m_mountCount > 0)
        {
            main_loop->run();
        }
    }
    catch(const Glib::Error& ex)
    {
        std::cerr << ex.what() << std::endl;
        std::cerr << "--- m_mountCount: " << m_mountCount;
    }
}

void GvfsService::umount(const std::string &resPath)
{
    Gio::init();
    Glib::init();

    main_loop = Glib::MainLoop::create(false);

    file = Gio::File::create_for_parse_name(resPath);
    Glib::RefPtr<Gio::MountOperation> mount_operation = Gio::MountOperation::create();

    try
    {
        Glib::RefPtr<Gio::Mount> mount = file->find_enclosing_mount();
        mount->unmount(mount_operation,
                                     [this] (Glib::RefPtr<Gio::AsyncResult>& result)
                                     {
                                         this->unmount_cb(result);
                                     });
        m_mountCount++;
        std::cerr << "inc m_mountCount: " << m_mountCount << "\n";
        if(m_mountCount > 0)
        {
            main_loop->run();
        }
    }
    catch(const Glib::Error& ex)
    {
        std::cerr << ex.what() << std::endl;
        std::cerr << "--- m_mountCount: " << m_mountCount;
    }
}

GvfsService::GvfsService() :
    m_mountCount(0)
{

}

void GvfsService::mount_cb(Glib::RefPtr<Gio::AsyncResult>& result)
{
    std::cerr << "mount_cb\n";
    try
    {
        file->mount_enclosing_volume_finish(result);

        std::cout << "mount: " << file->find_enclosing_mount()->get_name() << "\n";
        std::cout << "mount: " << file->find_enclosing_mount()->get_default_location()->get_path() << "\n";
    }
    catch(const Glib::Error& ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    m_mountCount--;
    std::cerr << "dec m_mountCount: " << m_mountCount << "\n";

    if(m_mountCount == 0)
    {
        main_loop->quit();
    }
}

void GvfsService::unmount_cb(Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::RefPtr<Gio::Mount> mount = Glib::RefPtr<Gio::Mount>::cast_dynamic(result->get_source_object());

    std::cerr << "unmount_cb\n";
    try
    {
        mount->unmount_finish(result);
    }
    catch(const Glib::Error& ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    m_mountCount--;
    std::cerr << "dec m_mountCount: " << m_mountCount << "\n";

    if(m_mountCount == 0)
    {
        main_loop->quit();
    }
}
