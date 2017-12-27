#ifndef FAR_EDITORS_H
#define FAR_EDITORS_H

#include <plugin.hpp>
#include <map>
#include "Editor.h"

class Editors {
private:
    static const bool DEFAULT_ENABLED = true;
    static constexpr const wchar_t *PLUGIN_REGISTRY_BRANCH = L"/editorcomp";
    static constexpr const wchar_t *ENABLED_REGISTRY_ENTRY = L"Enabled";

    PluginStartupInfo info;
    FarStandardFunctions fsf;
    std::wstring registryRootKey;
    bool enabled;

    std::map<int, Editor *> editors;

public:
    explicit Editors(const PluginStartupInfo &info, const FarStandardFunctions &fsf);

    Editor *getEditor(void *params = nullptr);

    PluginStartupInfo &getInfo();

    void setEnabled(bool enabled);

    bool getEnabled();

    void remove(Editor *&editor);
};

#endif //FAR_EDITORS_H
