#pragma once

#include "plugin.hpp"

#include <string>

class KeyBarTitlesHelper {
public:
    enum class KeyType {
        NORMAL,
        CTRL,
        ALT,
        SHIFT,
        CTRL_SHIFT,
        ALT_SHIFT,
        CTRL_ALT
    };

public:
    KeyBarTitlesHelper();
    KeyBarTitles& getKeyBar();
    void setKey(int index, KeyType, const std::wstring& name);
    void setNormalKey(int index, const std::wstring& name);
    void setAltKey(int index, const std::wstring& name);
    void setShiftKey(int index, const std::wstring& name);
    void setCtrlShiftKey(int index, const std::wstring& name);
    void setAltShiftKey(int index, const std::wstring& name);
    void setCtrlAltKey(int index, const std::wstring& name);
    void setCtrlKey(int index, const std::wstring& name);

private:
    void setKeyGeneric(int index, wchar_t *key[], const std::wstring &name);

    KeyBarTitles m_keyBar;
};
