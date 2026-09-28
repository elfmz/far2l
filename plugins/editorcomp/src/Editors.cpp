#include "Editors.h"
#include <utils.h>

static int getEditorId(const PluginStartupInfo &info) {
    EditorInfo ei = {0};
    info.EditorControl(ECTL_GETINFO, &ei);
    return ei.EditorID;
}

Editors::Editors(const PluginStartupInfo &info_, const FarStandardFunctions &fsf_)
    : info(info_), fsf(fsf_), settings(info)
{
    this->info.FSF = &this->fsf;
}

Editor *Editors::getEditor(void *params) {
    int id;
    if (params == nullptr)
        id = getEditorId(info);
    else
        id = *((int *) params);

    Editor *editor = editors[id];
    if (editor == nullptr) {
        editor = new Editor(id, info, fsf, &settings);
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

