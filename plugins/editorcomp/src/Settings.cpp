#include "editorcomp.h"
#include "Settings.h"
#include <KeyFileHelper.h>
#include <utils.h>
#include <wchar.h>

static constexpr const char *INI_SECTION = "Settings";
static constexpr const char *ENABLED_ENTRY = "Enabled";
static constexpr const char *FILE_MASKS_ENTRY = "fileMasks";

static constexpr const char *MAX_LINE_DELTA_ENTRY = "maxLineDelta";
static constexpr const char *MAX_LINE_LENGTH_ENTRY = "maxLineLength";
static constexpr const char *MAX_WORD_LENGTH_ENTRY = "maxWordLength";
static constexpr const char *MIN_PREFIX_LENGTH_ENTRY = "minPrefixLength";

Settings::Settings(const PluginStartupInfo &psi)
    : _psi(psi)
{
    _ini_path = InMyConfig("plugins/editorcomp/config.ini");

    KeyFileReadSection kfh(_ini_path, INI_SECTION);
    _auto_enabling = !!kfh.GetInt(ENABLED_ENTRY, _auto_enabling);
    _file_masks = kfh.GetString(FILE_MASKS_ENTRY, _file_masks.c_str());

    _max_line_delta = kfh.GetInt(MAX_LINE_DELTA_ENTRY, _max_line_delta);
    _max_line_length = kfh.GetInt(MAX_LINE_LENGTH_ENTRY, _max_line_length);
    _max_word_length = kfh.GetInt(MAX_WORD_LENGTH_ENTRY, _max_word_length);
    _min_prefix_length = kfh.GetInt(MIN_PREFIX_LENGTH_ENTRY, _min_prefix_length);

    sanitizeValues();
}

const wchar_t *Settings::getMsg(int msgId)
{
    return _psi.GetMsg(_psi.ModuleNumber, msgId);
}

static void sanitizeValue(int &v, int minimum, int maximum)
{
    if (v < minimum) {
        v = minimum;
    } else if (v > maximum) {
        v = maximum;
    }
}

void Settings::sanitizeValues()
{
    sanitizeValue(_max_word_length, 3, 999);
    sanitizeValue(_max_line_delta, 0, 99999);
    sanitizeValue(_max_line_length, _max_word_length, 99999);
    sanitizeValue(_min_prefix_length, 2, _max_word_length);
}

void Settings::configurationMenuDialog()
{
    int w = 50;
    int h = 15;

    struct FarDialogItem fdi[] = {
    /* 0 */ {DI_DOUBLEBOX, 1,       1,  w - 2,   h - 2, 0,     {}, 0, 0,    getMsg(M_TITLE), 0},
    /* 1 */ {DI_TEXT,      3,       2,  w - 4,   0,     FALSE, {}, DIF_CENTERTEXT, 0,    getMsg(M_HINT_SHIFT_TAB), 0},
    /* 2 */ {DI_TEXT,      3,       3,  w - 4,   0,     FALSE, {}, DIF_CENTERTEXT, 0,    getMsg(M_HINT_TAB), 0},
    /* 3 */ {DI_CHECKBOX,  3,       5,  0,       0,     TRUE,  {}, 0, 0,    getMsg(M_TEXT_AUTOENABLE), 0},
    /* 4 */ {DI_EDIT,      3,       6,  w - 4,   0,     0,  {},  0, 0, nullptr, 0},
    /* 5 */ {DI_TEXT,      3,       7,  w - 9,   0,     FALSE, {}, 0, 0,    getMsg(M_TEXT_MAX_LINE_DELTA), 0},
    /* 6 */ {DI_FIXEDIT,   w - 8,   7,  w - 4,   0,     FALSE, {(DWORD_PTR)L"99999"}, DIF_MASKEDIT, 0,    L"", 0},
    /* 7 */ {DI_TEXT,      3,       8,  w - 9,   0,     FALSE, {}, 0, 0,    getMsg(M_TEXT_MAX_LINE_LENGTH), 0},
    /* 8 */ {DI_FIXEDIT,   w - 8,   8,  w - 4,   0,     FALSE, {(DWORD_PTR)L"99999"}, DIF_MASKEDIT, 0,    L"", 0},
    /* 9 */ {DI_TEXT,      3,       9,  w - 9,   0,     FALSE, {}, 0, 0,    getMsg(M_TEXT_MAX_WORD_LENGTH), 0},
    /*10 */ {DI_FIXEDIT,   w - 8,   9,  w - 6,   0,     FALSE, {(DWORD_PTR)L"999"}, DIF_MASKEDIT, 0,    L"", 0},
    /*11 */ {DI_TEXT,      3,       10, w - 9,   0,     FALSE, {}, 0, 0,    getMsg(M_TEXT_MIN_PREFIX_LENGTH), 0},
    /*12 */ {DI_FIXEDIT,   w - 8,   10, w - 6,   0,     FALSE, {(DWORD_PTR)L"999"}, DIF_MASKEDIT, 0,    L"", 0},
    /*13 */ {DI_SINGLEBOX, 2,       11, 0,       0,     FALSE, {}, 0, 0,    L"", 0},
    /*14 */ {DI_BUTTON,    11,      12, 0,       0,     FALSE, {}, 0, TRUE, getMsg(M_OK), 0},
    /*15 */ {DI_BUTTON,    26,      12, 0,       0,     FALSE, {}, 0, 0,    getMsg(M_CANCEL), 0}
    };

    wchar_t str_max_line_delta[32]; swprintf(str_max_line_delta, 31, L"%d", _max_line_delta);
    wchar_t str_max_line_length[32]; swprintf(str_max_line_length, 31, L"%d", _max_line_length);
    wchar_t str_max_word_length[32]; swprintf(str_max_word_length, 31, L"%d", _max_word_length);
    wchar_t str_min_prefix_length[32]; swprintf(str_min_prefix_length, 31, L"%d", _min_prefix_length);


    unsigned int size = sizeof(fdi) / sizeof(fdi[0]);
    fdi[3].Param.Selected = _auto_enabling;
    fdi[4].PtrData = (const TCHAR*)_file_masks.c_str();
    fdi[6].PtrData = (const TCHAR*)str_max_line_delta;
    fdi[8].PtrData = (const TCHAR*)str_max_line_length;
    fdi[10].PtrData = (const TCHAR*)str_max_word_length;
    fdi[12].PtrData = (const TCHAR*)str_min_prefix_length;

    HANDLE hDlg = _psi.DialogInit(_psi.ModuleNumber, -1, -1, w, h, L"config", fdi, size, 0, 0, nullptr, 0);

    int runResult = _psi.DialogRun(hDlg);
    if (runResult == int(size) - 2) {
        _auto_enabling = (_psi.SendDlgMessage(hDlg, DM_GETCHECK, 3, 0) == BSTATE_CHECKED);
        _file_masks = (((const TCHAR *)_psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR, 4, 0)));
        _max_line_delta = wcstol(((const TCHAR *)_psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR, 6, 0)), NULL, 0);
        _max_line_length = wcstol(((const TCHAR *)_psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR, 8, 0)), NULL, 0);
        _max_word_length = wcstol(((const TCHAR *)_psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR, 10, 0)), NULL, 0);
        _min_prefix_length = wcstol(((const TCHAR *)_psi.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR, 12, 0)), NULL, 0);

        sanitizeValues();

        KeyFileHelper kfh(_ini_path);
        kfh.SetInt(INI_SECTION, ENABLED_ENTRY, _auto_enabling);
        kfh.SetString(INI_SECTION, FILE_MASKS_ENTRY, _file_masks.c_str());

        kfh.SetInt(INI_SECTION, MAX_LINE_DELTA_ENTRY, _max_line_delta);
        kfh.SetInt(INI_SECTION, MAX_LINE_LENGTH_ENTRY, _max_line_length);
        kfh.SetInt(INI_SECTION, MAX_WORD_LENGTH_ENTRY, _max_word_length);
        kfh.SetInt(INI_SECTION, MIN_PREFIX_LENGTH_ENTRY, _min_prefix_length);
    }

    _psi.DialogFree(hDlg);
}

void Settings::editorMenuDialog(bool &enabled)
{
//    PluginStartupInfo &info = editors->getInfo();

    int w = 50;
    int h = 10;

    struct FarDialogItem fdi[] = {
            {DI_DOUBLEBOX, 1,  1, w - 2, h - 2, 0,     {}, 0, 0,    getMsg(M_TITLE), 0},
            {DI_TEXT,      3,  2, w - 4, 0,     FALSE, {}, DIF_CENTERTEXT, 0,    getMsg(M_HINT_SHIFT_TAB), 0},
            {DI_TEXT,      3,  3, w - 4, 0,     FALSE, {}, DIF_CENTERTEXT, 0,    getMsg(M_HINT_TAB), 0},
            {DI_CHECKBOX,  3,  5, 0,     0,     TRUE,  {}, 0, 0,    getMsg(M_TEXT_ENABLED), 0},
            {DI_SINGLEBOX, 2,  6, 0,     0,     FALSE, {}, 0, 0,    L"", 0},
            {DI_BUTTON,    11, 7, 0,     0,     FALSE, {}, 0, TRUE, getMsg(M_OK), 0},
            {DI_BUTTON,    26, 7, 0,     0,     FALSE, {}, 0, 0,    getMsg(M_CANCEL), 0}
    };

    unsigned int size = sizeof(fdi) / sizeof(fdi[0]);
    fdi[3].Param.Selected = enabled;

    HANDLE hDlg = _psi.DialogInit(_psi.ModuleNumber, -1, -1, w, h, L"config", fdi, size, 0, 0, nullptr, 0);

    int runResult = _psi.DialogRun(hDlg);
    if (runResult == int(size) - 2) {
        enabled = (_psi.SendDlgMessage(hDlg, DM_GETCHECK, 3, 0) == BSTATE_CHECKED);
    }

    _psi.DialogFree(hDlg);
}
