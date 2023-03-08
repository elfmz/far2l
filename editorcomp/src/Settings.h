#ifndef FAR_SETTINGS_H
#define FAR_SETTINGS_H
#include <string>
#include <stdint.h>

struct Settings
{
    Settings(const PluginStartupInfo &psi);

    const wchar_t *getMsg(int msgId);

    void configurationMenuDialog();
    void editorMenuDialog(bool &enabled);

    int maxLineDelta() const { return _max_line_delta; }
    int maxLineLength() const { return _max_line_length; }
    int maxWordLength() const { return _max_word_length; }
    int minPrefixLength() const { return _min_prefix_length; }

    const std::wstring &fileMasks() const { return _auto_enabling ? _file_masks : _empty; }


private:
    const PluginStartupInfo &_psi;

    int _max_line_delta { 256 };
    int _max_line_length { 512 };
    int _max_word_length { 200 };
    int _min_prefix_length { 2 };
    bool _auto_enabling = true;
    std::wstring _file_masks { L"*.c;*.cpp;*.cxx;*.h;*.hpp;*.s;*.asm;*.pl;*.py;*.js;*.json;*.sh" };
    std::wstring _empty;
    std::string _ini_path;

    void sanitizeValues();
};

#endif // FAR_SETTINGS_H
