#include "Registry.h"
#include <windows.h>

Registry::Registry(const std::wstring &key) {
    if (WINPORT(RegCreateKeyEx)(HKEY_CURRENT_USER,
            key.c_str(), 0, nullptr, 0,
            KEY_ALL_ACCESS, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        hKey = nullptr;
    }
}

Registry::~Registry() {
    if (hKey != nullptr)
        WINPORT(RegCloseKey)(hKey);
}

bool Registry::read(const std::wstring &name, bool defaultValue) {
    DWORD data = 0;
    DWORD i = sizeof(DWORD);
    if (hKey != nullptr && WINPORT(RegQueryValueEx)(hKey, name.c_str(),
            nullptr, nullptr, (PBYTE) &data, &i) == ERROR_SUCCESS) {
        return data != 0;

    } else
        return defaultValue;
}

bool Registry::write(const std::wstring &name, bool value) {
    DWORD dwValue = value ? 1 : 0;
    return (hKey != nullptr && WINPORT(RegSetValueEx)(hKey, name.c_str(), 0, REG_DWORD,
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(dwValue)) == ERROR_SUCCESS);
}

void Registry::read(const std::wstring &name, std::wstring &value) {
    wchar_t data[0x1000] = {0};
    DWORD i = sizeof(data) - sizeof(data[0]);
    if (hKey != nullptr && WINPORT(RegQueryValueEx)(hKey,
            name.c_str(), nullptr, nullptr, (PBYTE)&data[0], &i) == ERROR_SUCCESS) {
        value = data;
    }
}

bool Registry::write(const std::wstring &name, const std::wstring &value) {
    return (hKey != nullptr && WINPORT(RegSetValueEx)(hKey, name.c_str(), 0, REG_SZ,
        reinterpret_cast<const BYTE *>(value.c_str()),
        (value.size() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS);
}
