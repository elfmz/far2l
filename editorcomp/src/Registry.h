#ifndef FAR_REGISTRY_H
#define FAR_REGISTRY_H
#include <plugin.hpp>
#include <string>

struct Registry
{
    Registry(const std::wstring &key);
    ~Registry();

    bool read(const std::wstring &name, bool defaultValue);
    bool write(const std::wstring &name, bool value);
    void read(const std::wstring &name, std::wstring &value);
    bool write(const std::wstring &name, const std::wstring &value);

    private:
    HKEY hKey;
};

#endif //FAR_REGISTRY_H
