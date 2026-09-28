#ifndef FAR_EDITORS_H
#define FAR_EDITORS_H

#include <farplug-wide.h>
#include <map>
#include "Editor.h"
#include "Settings.h"

class Editors {
private:
    PluginStartupInfo info;
    FarStandardFunctions fsf;
    Settings settings;

    std::map<int, Editor *> editors;

public:
    explicit Editors(const PluginStartupInfo &info, const FarStandardFunctions &fsf);

    Editor *getEditor(void *params = nullptr);

    PluginStartupInfo &getInfo();

    void remove(Editor *&editor);

    Settings *getSettings() { return &settings; }
};

#endif //FAR_EDITORS_H
