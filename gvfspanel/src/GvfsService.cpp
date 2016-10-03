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

    bool bAnonymous = true;

    mount_operation->signal_ask_password().connect(
    [mount_operation, bAnonymous](const Glib::ustring& msg,
                                  const Glib::ustring& defaultUser,
                                  const Glib::ustring& defaultdomain,
                                  Gio::AskPasswordFlags flags)
    {
        std::cerr << "ask password\n";
        std::cerr << msg << "\n";
        std::cerr << "default user: " << defaultUser << "\n";

        if ((flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED) && bAnonymous)
        {
            mount_operation->set_anonymous(true);
        }
        else
        {
            // trigger functor for entering user credentials
            if (flags & G_ASK_PASSWORD_NEED_USERNAME)
            {
                // trigger user name enter callback, call passwd functor
            }

            if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
            {
                // trigger domain name enter callback, call passwd functor
            }
            if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
            {
                // trigger password name enter callback, call passwd functor
            }
        }
        if (bAnonymous)
        {
             mount_operation->reply(Gio::MOUNT_OPERATION_HANDLED);
        }
    });
    mount_operation->signal_ask_question().connect(
    [mount_operation](const Glib::ustring& msg, const Glib::StringArrayHandle& choices)
    {
        std::cerr << "ask question\n";
        std::cerr << msg << "\n";
    });

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
        m_mountName = file->find_enclosing_mount()->get_name();
        m_mountPath = file->find_enclosing_mount()->get_default_location()->get_path();
        std::cout << "mount: " << m_mountName << "\n";
        std::cout << "mount: " << m_mountPath << "\n";
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
