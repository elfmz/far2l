#pragma once
#include <gtkmm.h>

class GvfsService
{
    Glib::RefPtr<Gio::File> file;
    Glib::RefPtr<Glib::MainLoop> main_loop;
    int m_mountCount;

    void mount_cb(Glib::RefPtr<Gio::AsyncResult>& result);
    void unmount_cb(Glib::RefPtr<Gio::AsyncResult>& result);
public:
    std::string m_mountName;
    std::string m_mountPath;
public:
    void mount(const std::string& resPath, const std::string &userName, const std::string &password);
    void umount(const std::string& resPath);
public:
    GvfsService();
};


