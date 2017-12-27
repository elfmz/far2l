#include <windows.h>
#include "Editors.h"

static int getEditorId(const PluginStartupInfo &info) {
    EditorInfo ei = {0};
    info.EditorControl(ECTL_GETINFO, &ei);
    return ei.EditorID;
}

static bool readRegistry(HKEY hKey, const std::wstring &name, bool defaultValue) {
    DWORD data = 0;
    DWORD i = sizeof(DWORD);
    if (WINPORT(RegQueryValueEx)(hKey, name.c_str(), nullptr, nullptr, (PBYTE) &data, &i) == ERROR_SUCCESS)
        return data != 0;
    else
        return defaultValue;
};

bool writeRegistry(HKEY hKey, const std::wstring &name, bool value) {
    DWORD dwValue = value ? 1 : 0;
    return WINPORT(RegSetValueEx)(hKey, name.c_str(), 0, REG_DWORD,
                                  reinterpret_cast<const BYTE *>(&dwValue), sizeof(dwValue)) == ERROR_SUCCESS;
};

Editors::Editors(const PluginStartupInfo &info, const FarStandardFunctions &fsf) : info(info), fsf(fsf) {
    this->info.FSF = &this->fsf;

    this->registryRootKey = info.RootKey;
    this->registryRootKey += PLUGIN_REGISTRY_BRANCH;

    HKEY hKey = nullptr;
    if (WINPORT(RegCreateKeyEx)(HKEY_CURRENT_USER, this->registryRootKey.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS,
                                nullptr, &hKey, nullptr) == ERROR_SUCCESS)
        this->enabled = readRegistry(hKey, ENABLED_REGISTRY_ENTRY, DEFAULT_ENABLED);
    else
        this->enabled = DEFAULT_ENABLED;


    if (hKey != nullptr)
        WINPORT(RegCloseKey)(hKey);
}

Editor *Editors::getEditor(void *params) {
    int id;
    if (params == nullptr)
        id = getEditorId(info);
    else
        id = *((int *) params);

    Editor *editor = editors[id];
    if (editor == nullptr) {
        editor = new Editor(id, info, fsf);
        editors[id] = editor;
    }

    return editor;
}

void Editors::remove(Editor *&editor) {
    if (editor != nullptr) {
        editors.erase(editor->getId());
        delete editor;
        editor = nullptr;
    }
}

PluginStartupInfo &Editors::getInfo() {
    return info;
}

void Editors::setEnabled(bool enabled) {
    HKEY hKey = nullptr;

    if (WINPORT(RegCreateKeyEx)(HKEY_CURRENT_USER, this->registryRootKey.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS,
                                nullptr, &hKey, nullptr) == ERROR_SUCCESS
        && writeRegistry(hKey, ENABLED_REGISTRY_ENTRY, enabled))
        this->enabled = enabled;
    else
        this->enabled = DEFAULT_ENABLED;

    if (hKey != nullptr)
        WINPORT(RegCloseKey)(hKey);
}

bool Editors::getEnabled() {
    return this->enabled;
}
