#include "Editors.h"
#include <KeyFileHelper.h>
#include <utils.h>

static int getEditorId(const PluginStartupInfo &info) {
    EditorInfo ei = {0};
    info.EditorControl(ECTL_GETINFO, &ei);
    return ei.EditorID;
}

Editors::Editors(const PluginStartupInfo &info, const FarStandardFunctions &fsf) : info(info), fsf(fsf) {
    this->info.FSF = &this->fsf;

    this->iniPath = InMyConfig("plugins/editorcomp/config.ini");
    this->iniSection  = "Settings";

    KeyFileReadSection kfh(this->iniPath, this->iniSection);
    this->autoEnabling = !!kfh.GetInt(ENABLED_ENTRY, DEFAULT_ENABLED);
    this->fileMasks = kfh.GetString(FILE_MASKS_ENTRY, DEFAULT_FILE_MASKS);
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
    KeyFileHelper kfh(this->iniPath);
    kfh.SetInt(this->iniSection, ENABLED_ENTRY, enabled);
    if (kfh.Save())
        this->autoEnabling = enabled;
}

bool Editors::getAutoEnabling() {
    return this->autoEnabling;
}

void Editors::setFileMasks(const std::wstring &masks) {
    KeyFileHelper kfh(this->iniPath);
    kfh.SetString(this->iniSection, FILE_MASKS_ENTRY, masks.c_str());
    if (kfh.Save())
        this->fileMasks = masks;
}

const std::wstring &Editors::getFileMasks() {
    return this->fileMasks;
}

