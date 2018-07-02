#include "Editors.h"
#include "Registry.h"

static int getEditorId(const PluginStartupInfo &info) {
    EditorInfo ei = {0};
    info.EditorControl(ECTL_GETINFO, &ei);
    return ei.EditorID;
}

Editors::Editors(const PluginStartupInfo &info, const FarStandardFunctions &fsf) : info(info), fsf(fsf) {
    this->info.FSF = &this->fsf;

    this->registryRootKey = info.RootKey;
    this->registryRootKey += PLUGIN_REGISTRY_BRANCH;

    Registry reg(this->registryRootKey);
    this->autoEnabling = reg.read(ENABLED_REGISTRY_ENTRY, DEFAULT_ENABLED);
    this->fileMasks = DEFAULT_FILE_MASKS;
    reg.read(FILE_MASKS_ENTRY, this->fileMasks);
}

Editor *Editors::getEditor(void *params) {
    int id;
    if (params == nullptr)
        id = getEditorId(info);
    else
        id = *((int *) params);

    Editor *editor = editors[id];
    if (editor == nullptr) {
        editor = new Editor(id, info, fsf, getAutoEnabling() ? getFileMasks() : emptyString);
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

void Editors::setAutoEnabling(bool enabled) {
    Registry reg(this->registryRootKey);
    if (reg.write(ENABLED_REGISTRY_ENTRY, enabled))
        this->autoEnabling = enabled;
}

bool Editors::getAutoEnabling() {
    return this->autoEnabling;
}

void Editors::setFileMasks(const std::wstring &masks) {
    Registry reg(this->registryRootKey);
    if (reg.write(FILE_MASKS_ENTRY, masks))
        this->fileMasks = masks;
}

const std::wstring &Editors::getFileMasks() {
    return this->fileMasks;
}

