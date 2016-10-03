#include "KeyBarTitlesHelper.h"

#include "plugin.hpp"

KeyBarTitlesHelper::KeyBarTitlesHelper() :
    m_keyBar { }
{

}

KeyBarTitles& KeyBarTitlesHelper::getKeyBar()
{
    return m_keyBar;
}

void KeyBarTitlesHelper::setKey(int index, KeyBarTitlesHelper::KeyType type, const std::wstring &name)
{
    switch(type)
    {
    case KeyType::NORMAL:
        setNormalKey(index, name);
        break;
    case KeyType::ALT:
        setAltKey(index, name);
        break;
    case KeyType::ALT_SHIFT:
        setAltShiftKey(index, name);
        break;
    case KeyType::CTRL:
        setCtrlKey(index, name);
        break;
    case KeyType::CTRL_ALT:
        setCtrlAltKey(index, name);
        break;
    case KeyType::CTRL_SHIFT:
        setCtrlShiftKey(index, name);
        break;
    case KeyType::SHIFT:
        setShiftKey(index, name);
        break;
    }
}

void KeyBarTitlesHelper::setNormalKey(int index, const std::wstring &name)
{
    setKeyGeneric(index, this->m_keyBar.Titles, name);
}

void KeyBarTitlesHelper::setAltKey(int index, const std::wstring &name)
{
    setKeyGeneric(index, this->m_keyBar.AltTitles, name);
}

void KeyBarTitlesHelper::setShiftKey(int index, const std::wstring &name)
{
    setKeyGeneric(index, this->m_keyBar.ShiftTitles, name);
}

void KeyBarTitlesHelper::setCtrlShiftKey(int index, const std::wstring &name)
{
    setKeyGeneric(index, this->m_keyBar.CtrlShiftTitles, name);
}

void KeyBarTitlesHelper::setAltShiftKey(int index, const std::wstring &name)
{
    setKeyGeneric(index, this->m_keyBar.AltShiftTitles, name);
}

void KeyBarTitlesHelper::setCtrlAltKey(int index, const std::wstring &name)
{
    setKeyGeneric(index, this->m_keyBar.CtrlAltTitles, name);
}

void KeyBarTitlesHelper::setCtrlKey(int index, const std::wstring &name)
{
    setKeyGeneric(index, this->m_keyBar.CtrlTitles, name);
}

void KeyBarTitlesHelper::setKeyGeneric(int index, wchar_t *key[], const std::wstring &name)
{
    std::copy(name.begin(), name.end(), key[index]);
}
