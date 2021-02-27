#ifndef FAR_EDITORS_H
#define FAR_EDITORS_H

#include <plugin.hpp>
#include <map>
#include "Editor.h"

class Editors {
private:
    static const bool DEFAULT_ENABLED = true;
    static constexpr const char *ENABLED_ENTRY = "Enabled";
    static constexpr const char *FILE_MASKS_ENTRY = "fileMasks";
    static constexpr const wchar_t *DEFAULT_FILE_MASKS = L"*.c;*.cpp;*.cxx;*.h;*.s;*.asm;*.pl;*.py;*.js;*.json;*.sh";

    PluginStartupInfo info;
    FarStandardFunctions fsf;
    std::string iniPath, iniSection;
    std::wstring fileMasks;
    const std::wstring emptyString;
    bool autoEnabling;

    std::map<int, Editor *> editors;

public:
    explicit Editors(const PluginStartupInfo &info, const FarStandardFunctions &fsf);

    Editor *getEditor(void *params = nullptr);

    PluginStartupInfo &getInfo();

    void setAutoEnabling(bool enabled);
    bool getAutoEnabling();

    void setFileMasks(const std::wstring &masks);
    const std::wstring &getFileMasks();


    void remove(Editor *&editor);
};

#endif //FAR_EDITORS_H
