#pragma once

extern const char *VT_TranslateSpecialKey(const WORD key, bool ctrl, bool alt, bool shift, unsigned char keypad = 0, WCHAR uc = 0);
extern std::string VT_TranslateKeyToKitty(const KEY_EVENT_RECORD &KeyEvent, int kitty_kb_flags, unsigned char keypad);

