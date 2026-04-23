#include <KeyFileHelper.h>
#include <WideMB.h>
#include <WinCompat.h>
#include <WinPort.h>
#include <utils.h>
#include <farkeys.h>
#include <farplug-wide.h>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <cwchar>
#include <string>

#ifndef WINAPI
#define WINAPI
#endif

#define SYSID_MEMO 0x4D454D4F // 'MEMO'
#define MEMO_COUNT 10

static const char *INI_PATH = "plugins/memo/state.ini";
static const char *MEMO_DIR = "plugins/memo/";
static const char *MACROS_INI = "settings/key_macros.ini";
static const char *MACRO_SECTION = "KeyMacros/Common/CtrlAltS";
static const char *MACRO_SECTION_OLD_CTRL_S = "KeyMacros/Common/CtrlS";
static const char *MACRO_SECTION_OLD_CTRL_SHIFT_S = "KeyMacros/Common/CtrlShiftS";

static const char *INI_S_SETTINGS = "Settings";
static const char *INI_K_ENABLED = "Enabled";
static const char *INI_K_HOTKEY = "HotkeyEnabled";
static const char *INI_K_LAST = "LastMemo";

static const char *MACRO_K_SEQ = "Sequence";
static const char *MACRO_K_DESC = "Description";
static const char *MACRO_K_DISOUT = "DisableOutput";

static const char *MEMO_FILE_FMT = "%smemo-%02d.txt";

// Debug helper - writes to plugins/memo/debug.log <- completely absent from release builds
#if defined(DEBUG) || defined(_DEBUG)
static const char *LOG_PATH = "plugins/memo/debug.log";
static void DebugLog(const char *fmt, ...) {
  time_t now = time(nullptr);
  struct tm tm_info;
  localtime_r(&now, &tm_info);
  char ts[24];
  strftime(ts, sizeof(ts), "%d-%m-%y, %H:%M:%S", &tm_info);

  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  std::string mbLogPath = InMyConfig(LOG_PATH);
  FILE *f = fopen(mbLogPath.c_str(), "a");
  if (f) {
    fprintf(f, "[%s][memo] %s\n", ts, buf);
    fclose(f);
  }
}
#define DBG(fmt, ...) DebugLog(fmt, ##__VA_ARGS__)
#else
#define DebugLog(...)
#define DBG(fmt, ...) ((void)0)
#endif

enum MsgId {
  MMemo = 0, MOk, MCancel, MConfigTitle, MEnablePlugin, MUseHotkey, MSaveAs, MConfig,
  MSaveMemo, MPath, MMemoTitle, MUseHotkeyHint
};

enum DialogItems {
  DI_BOX = 0, DI_MEMO,
  DI_F2, DI_BTN1, DI_BTN2, DI_BTN3, DI_BTN4, DI_BTN5, DI_BTN6, DI_BTN7, DI_BTN8, DI_BTN9, DI_BTN0, DI_F9
};

static PluginStartupInfo g_far;
static int g_currentMemo = 0;
static std::wstring g_loadedContent;
static bool g_enabled = true;
static bool g_useHotkey = true;

static std::wstring NormalizeMemoContent(const std::wstring &input);

static inline const wchar_t *GetMsg(int msgId) {
  return g_far.GetMsg(g_far.ModuleNumber, msgId);
}
static inline int MemoDisplayNum(int idx) { return (idx == 9) ? 0 : idx + 1; }

static std::wstring GetMemoFilePath(int index) {
  char buf[MAX_PATH];
  snprintf(buf, sizeof(buf), MEMO_FILE_FMT, MEMO_DIR, MemoDisplayNum(index));
  return MB2Wide(InMyConfig(buf, false).c_str());
}

static void SaveState() {
  KeyFileHelper kf(InMyConfig(INI_PATH));
  kf.SetInt(INI_S_SETTINGS, INI_K_ENABLED, g_enabled ? 1 : 0);
  kf.SetInt(INI_S_SETTINGS, INI_K_HOTKEY, g_useHotkey ? 1 : 0);
  kf.SetInt(INI_S_SETTINGS, INI_K_LAST, g_currentMemo);
  kf.Save();
}

static int GetLastMemo() {
  KeyFileHelper kf(InMyConfig(INI_PATH, false));
  int res = kf.GetInt(INI_S_SETTINGS, INI_K_LAST, 0);
  DBG("GetLastMemo: %d", res);
  return res;
}

static void SaveLastMemo(int v) {
  DBG("SaveLastMemo: %d", v);
  g_currentMemo = v;
  SaveState();
}

static bool GetEnabled() { return g_enabled; }
static void SetEnabled(bool v) {
  if (g_enabled == v) return;
  DBG("SetEnabled: %d", (int)v);
  g_enabled = v;
  SaveState();
}

static bool IsMacroOurs(KeyFileHelper &kfh, const char *section) {
  if (!kfh.HasSection(section)) return false;
  char ours[32];
  snprintf(ours, sizeof(ours), "callplugin(0x%08X)", SYSID_MEMO);
  return kfh.GetString(section, MACRO_K_SEQ, "").find(ours) != std::string::npos;
}

static bool IsCurrentMacroOurs(KeyFileHelper &kfh) {
  return IsMacroOurs(kfh, MACRO_SECTION);
}

// Backs up the macros INI before overwriting a foreign CtrlAltS binding.
// Tries key_macros.bak, then key_macros-1.bak, key_macros-2.bak, ... until a free slot is found.
static void BackupMacrosFile() {
  std::string src = InMyConfig(MACROS_INI, false);
  std::string base = src;
  size_t dot = base.rfind('.');
  if (dot != std::string::npos) base.resize(dot);
  std::string dst = base + ".bak";
  for (int n = 1; n <= 999; ++n) {
    FILE *probe = fopen(dst.c_str(), "rb");
    if (!probe) break;  // slot is free
    fclose(probe);
    if (n == 999) { DBG("BackupMacrosFile: no free slot"); return; }
    char sfx[24];
    snprintf(sfx, sizeof(sfx), "-%d.bak", n);
    dst = base + sfx;
  }
  FILE *in = fopen(src.c_str(), "rb");
  if (!in) { DBG("BackupMacrosFile: cannot open src"); return; }
  FILE *out = fopen(dst.c_str(), "wb");
  if (!out) { fclose(in); DBG("BackupMacrosFile: cannot create dst"); return; }
  char buf[4096]; size_t rd;
  bool ok = true;
  while (ok && (rd = fread(buf, 1, sizeof(buf), in)) > 0)
    if (fwrite(buf, 1, rd, out) != rd) { DBG("BackupMacrosFile: write error"); ok = false; }
  fclose(in); fclose(out);
  if (!ok) { remove(dst.c_str()); return; }  // remove partial backup on write failure
  DBG("BackupMacrosFile: saved to %s", dst.c_str());
}

static void UpdateGlobalMacro(bool enabled) {
  DBG("UpdateGlobalMacro: %d", (int)enabled);
  KeyFileHelper kfh(InMyConfig(MACROS_INI));
  bool changed = false;
  if (enabled) {
    // Remove plugin-owned legacy bindings so only CtrlAltS remains active.
    if (IsMacroOurs(kfh, MACRO_SECTION_OLD_CTRL_S)) {
      kfh.RemoveSection(MACRO_SECTION_OLD_CTRL_S);
      changed = true;
    }
    if (IsMacroOurs(kfh, MACRO_SECTION_OLD_CTRL_SHIFT_S)) {
      kfh.RemoveSection(MACRO_SECTION_OLD_CTRL_SHIFT_S);
      changed = true;
    }

    if (!kfh.HasSection(MACRO_SECTION) || !IsCurrentMacroOurs(kfh)) {  // explicit user action may overwrite foreign
      if (kfh.HasSection(MACRO_SECTION))  // about to overwrite a foreign binding — back it up first
        BackupMacrosFile();
      char seq[64];
      snprintf(seq, sizeof(seq), "callplugin(0x%08X)", SYSID_MEMO);
      kfh.SetString(MACRO_SECTION, MACRO_K_SEQ, seq);
      kfh.SetString(MACRO_SECTION, MACRO_K_DESC, "Memo Plugin");
      kfh.SetString(MACRO_SECTION, MACRO_K_DISOUT, "1");
      changed = true;
    }

    if (changed) {
      kfh.Save();

      ActlKeyMacro akm = {MCMD_LOADALL};
      g_far.AdvControl(g_far.ModuleNumber, ACTL_KEYMACRO, &akm, NULL);
    }
  } else {
    if (IsCurrentMacroOurs(kfh)) {  // never touch a macro we didn't write
      kfh.RemoveSection(MACRO_SECTION);
      changed = true;
    }
    if (IsMacroOurs(kfh, MACRO_SECTION_OLD_CTRL_S)) {
      kfh.RemoveSection(MACRO_SECTION_OLD_CTRL_S);
      changed = true;
    }
    if (IsMacroOurs(kfh, MACRO_SECTION_OLD_CTRL_SHIFT_S)) {
      kfh.RemoveSection(MACRO_SECTION_OLD_CTRL_SHIFT_S);
      changed = true;
    }

    if (changed) {
      kfh.Save();

      ActlKeyMacro akm = {MCMD_LOADALL};
      g_far.AdvControl(g_far.ModuleNumber, ACTL_KEYMACRO, &akm, NULL);
    }
  }
}

static bool GetUseHotkey() { return g_useHotkey; }
static void SetUseHotkey(bool v) {
  if (g_useHotkey == v)
    return;
  g_useHotkey = v;
  SaveState();
  UpdateGlobalMacro(v);
}

static std::wstring LoadFile(int idx) {
  std::wstring path = GetMemoFilePath(idx);
  DBG("LoadFile(%d): %ls", idx, path.c_str());
  FILE *f = fopen(Wide2MB(path.c_str()).c_str(), "r");
  if (!f) return L"";
  char buf[4096];
  std::string mb;
  while (fgets(buf, sizeof(buf), f)) mb += buf;
  fclose(f);
  return NormalizeMemoContent(MB2Wide(mb.c_str()));
}

static void SaveFile(int idx, const std::wstring &content) {
  std::wstring path = GetMemoFilePath(idx);
  DBG("SaveFile(%d, len=%d): %ls", idx, (int)content.length(), path.c_str());
  std::string mbP = Wide2MB(path.c_str());
  if (content.empty()) {
    remove(mbP.c_str());
    return;
  }
  FILE *f = fopen(mbP.c_str(), "w");
  if (f) {
    fputs(Wide2MB(content.c_str()).c_str(), f);
    fclose(f);
  } else {
    DBG("SaveFile(%d): write failed: %ls", idx, path.c_str());
  }
}

static std::wstring NormalizeMemoContent(const std::wstring &input) {
  std::wstring normalized;
  normalized.reserve(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    wchar_t ch = input[i];
    if (ch == L'\r') {
      if (i + 1 < input.size() && input[i + 1] == L'\n')
        ++i;
      normalized.push_back(L'\n');
    } else {
      normalized.push_back(ch);
    }
  }

  for (wchar_t ch : normalized) {
    if (ch != L'\n')
      return normalized;
  }
  return L"";
}

static std::wstring GetMemoText(HANDLE hDlg) {
  size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
  if (len == 0)
    return L"";

  // DM_GETTEXTPTR copies text including trailing NUL, so allocate len + 1.
  std::wstring content(len + 1, L'\0');
  g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO, (LONG_PTR)content.data());
  content.resize(wcslen(content.c_str()));
  return NormalizeMemoContent(content);
}

static std::wstring BuildMemoTitle() {
  std::wstring title = GetMsg(MMemoTitle);
  std::wstring memoNum = std::to_wstring(MemoDisplayNum(g_currentMemo));
  size_t pos = title.find(L"%d");
  if (pos != std::wstring::npos) {
    title.replace(pos, 2, memoNum);
  } else {
    title += L" ";
    title += memoNum;
  }
  return title;
}

static void SetMemoTitle(HANDLE hDlg) {
  std::wstring title = BuildMemoTitle();
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_BOX, (LONG_PTR)title.c_str());
}

static void SwitchToMemo(HANDLE hDlg, int newMemo) {
  if (newMemo < 0 || newMemo >= MEMO_COUNT || newMemo == g_currentMemo)
    return;
  DBG("SwitchToMemo: %d -> %d", g_currentMemo, newMemo);
  std::wstring content = GetMemoText(hDlg);
  if (content != g_loadedContent) SaveFile(g_currentMemo, content);
  g_currentMemo = newMemo;
  g_loadedContent = LoadFile(g_currentMemo);
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_MEMO, (LONG_PTR)g_loadedContent.c_str());
  SetMemoTitle(hDlg);
  for (int i = 0; i < MEMO_COUNT; i++) {
    FarDialogItem it;
    if (g_far.SendDlgMessage(hDlg, DM_GETDLGITEMSHORT, DI_BTN1 + i, (LONG_PTR)&it)) {
      it.DefaultButton = false;
      g_far.SendDlgMessage(hDlg, DM_SETDLGITEMSHORT, DI_BTN1 + i, (LONG_PTR)&it);
    }
  }
  g_far.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
  g_far.SendDlgMessage(hDlg, DM_SETFOCUS, DI_MEMO, 0);
}

static void SaveAs(HANDLE hDlg) {
  std::wstring content = GetMemoText(hDlg);
  wchar_t path[MAX_PATH];
  swprintf(path, MAX_PATH, L"memo-%02d.txt", MemoDisplayNum(g_currentMemo));
  if (g_far.InputBox(GetMsg(MSaveMemo), GetMsg(MPath), L"MemoSave", path, path, MAX_PATH, NULL, FIB_NONE)) {
    DBG("SaveAs: %ls", path);
    FILE *f = fopen(Wide2MB(path).c_str(), "w");
    if (f) {
      fputs(Wide2MB(content.c_str()).c_str(), f);
      fclose(f);
    } else {
      DBG("SaveAs: write failed: %ls", path);
    }
  }
}

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber) {
  enum { CFG_BOX, CFG_ENABLED, CFG_HOTKEY, CFG_HOTKEY_HINT, CFG_SEP, CFG_OK, CFG_CANCEL };
  FarDialogItem it[] = {
    {DI_DOUBLEBOX, 3, 1, 36, 9, 0, {0}, 0, 0, GetMsg(MConfigTitle), 0},
    {DI_CHECKBOX,  5, 3, 0, 0, 0, {(DWORD_PTR)(GetEnabled() ? BSTATE_CHECKED : BSTATE_UNCHECKED)}, 0, 0, GetMsg(MEnablePlugin), 0},
    {DI_CHECKBOX,  5, 4, 0, 0, 0, {(DWORD_PTR)(GetUseHotkey() ? BSTATE_CHECKED : BSTATE_UNCHECKED)}, 0, 0, GetMsg(MUseHotkey), 0},
    {DI_TEXT,      9, 5, 0, 0, 0, {0}, 0, 0, GetMsg(MUseHotkeyHint), 0},
    {DI_TEXT,     -1, 7, 0, 0, 0, {0}, DIF_SEPARATOR, 0, NULL, 0},
    {DI_BUTTON,    0, 8, 0, 0, 0, {0}, DIF_CENTERGROUP, 1, GetMsg(MOk), 0},
    {DI_BUTTON,    0, 8, 0, 0, 0, {0}, DIF_CENTERGROUP, 0, GetMsg(MCancel), 0}};

  HANDLE d = g_far.DialogInit(g_far.ModuleNumber, -1, -1, 40, 11, NULL, it, sizeof(it)/sizeof(it[0]), 0, 0, NULL, 0);
  if (d != (HANDLE)-1) {
    if (g_far.DialogRun(d) == CFG_OK) {
      DBG("ConfigureW: status OK");
      SetEnabled(g_far.SendDlgMessage(d, DM_GETCHECK, CFG_ENABLED, 0) == BSTATE_CHECKED);
      SetUseHotkey(g_far.SendDlgMessage(d, DM_GETCHECK, CFG_HOTKEY, 0) == BSTATE_CHECKED);
    }
    g_far.DialogFree(d);
  }
  return 1;
}

static void UpdateLayout(HANDLE hDlg, int w, int h, bool resizeDialog = true, COORD *pConsoleSize = nullptr) {
  if (resizeDialog) {
    COORD sz = {(SHORT)w, (SHORT)h};
    g_far.SendDlgMessage(hDlg, DM_RESIZEDIALOG, 0, (LONG_PTR)&sz);
    if (pConsoleSize) {
      COORD pos = {(SHORT)((pConsoleSize->X - w) / 2), (SHORT)((pConsoleSize->Y - h) / 2)};
      g_far.SendDlgMessage(hDlg, DM_MOVEDIALOG, 1, (LONG_PTR)&pos);
    }
  }

  SMALL_RECT r;
  r = {1, 0, 0, 0};
  g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_BOX, (LONG_PTR)&r);
  SetMemoTitle(hDlg);
  r = {1, 1, (SHORT)(w - 2), (SHORT)(h - 2)};
  g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_MEMO, (LONG_PTR)&r);
  int f2w = (int)wcslen(GetMsg(MSaveAs));
  if (f2w < 1) f2w = 1;
  r = {1, (SHORT)(h - 1), (SHORT)(1 + f2w - 1), (SHORT)(h - 1)};
  g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_F2, (LONG_PTR)&r);
  int f9w = (int)wcslen(GetMsg(MConfig));
  if (f9w < 1) f9w = 1;
  r = {(SHORT)(w - 2 - f9w + 1), (SHORT)(h - 1), (SHORT)(w - 2), (SHORT)(h - 1)};
  g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_F9, (LONG_PTR)&r);

  int totalDigitsWidth = 10 * 3;
  int leftEdge = 1 + f2w;
  int rightEdge = (w - 2 - f9w + 1) - 1;
  int availableSpace = rightEdge - leftEdge + 1;
  if (availableSpace < totalDigitsWidth) availableSpace = totalDigitsWidth;
  int curX = leftEdge + (availableSpace - totalDigitsWidth) / 2;
  for (int i = 0; i < 10; i++) {
    r = {(SHORT)curX, (SHORT)(h - 1), (SHORT)(curX + 2), (SHORT)(h - 1)};
    g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_BTN1 + i, (LONG_PTR)&r);
    curX += 3;
  }
}

static LONG_PTR WINAPI MemoDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2) {
  if (Msg == DN_INITDIALOG) {
    SMALL_RECT r;
    if (g_far.SendDlgMessage(hDlg, DM_GETDLGRECT, 0, (LONG_PTR)&r))
      UpdateLayout(hDlg, r.Right - r.Left + 1, r.Bottom - r.Top + 1, false);
    return TRUE;
  }
  if (Msg == DN_RESIZECONSOLE) {
    COORD *p = (COORD *)Param2;
    int w = p->X * 7 / 10,
        h = p->Y * 7 / 10;
    if (w < 75) w = 75;
    if (h < 20) h = 20;
    UpdateLayout(hDlg, w, h, true, p);
    return TRUE;
  }
  if (Msg == DN_CTLCOLORDLGITEM && Param1 >= DI_BTN1 && Param1 <= DI_BTN0) {
    if (static_cast<int>(Param1 - DI_BTN1) == g_currentMemo) {
      uint64_t *colors = reinterpret_cast<uint64_t *>(Param2);
      colors[0] = colors[4];
    }
    return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
  }
  if (Msg == DN_BTNCLICK) {
    if (Param1 == DI_F2) SaveAs(hDlg);
    else if (Param1 == DI_F9) ConfigureW(0);
    else if (Param1 >= DI_BTN1 && Param1 <= DI_BTN0) SwitchToMemo(hDlg, Param1 - DI_BTN1);
    return TRUE;
  }
  if (Msg == DN_KEY) {
    if (Param2 == KEY_F2) { SaveAs(hDlg); return TRUE; }
    if (Param2 == KEY_F9) { ConfigureW(0); return TRUE; }
    if (Param2 >= KEY_CTRL1 && Param2 <= KEY_CTRL9) { SwitchToMemo(hDlg, (int)(Param2 - KEY_CTRL1)); return TRUE; }
    if (Param2 == KEY_CTRL0) { SwitchToMemo(hDlg, 9); return TRUE; }
    if (Param2 >= KEY_ALT1 && Param2 <= KEY_ALT9) { SwitchToMemo(hDlg, (int)(Param2 - KEY_ALT1)); return TRUE; }
    if (Param2 == KEY_ALT0) { SwitchToMemo(hDlg, 9); return TRUE; }
  }
  if (Msg == DN_CLOSE) {
    std::wstring content = GetMemoText(hDlg);
    if (content != g_loadedContent)
      SaveFile(g_currentMemo, content);
    SaveLastMemo(g_currentMemo);
  }
  return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

static void OpenMemo() {
  g_currentMemo = GetLastMemo();
  g_loadedContent = LoadFile(g_currentMemo);
  DBG("OpenMemo: cur=%d, len=%d", g_currentMemo, (int)g_loadedContent.length());
  std::wstring title = BuildMemoTitle();

  FarDialogItem it[] = {
    {DI_TEXT,     0, 0, 0, 0, 0, {0}, 0, 0, title.c_str(), 0},
    {DI_MEMOEDIT, 0, 0, 0, 0, 1, {0}, 0, 0, g_loadedContent.c_str(), 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, GetMsg(MSaveAs), 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[1]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[2]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[3]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[4]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[5]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[6]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[7]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[8]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[9]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[0]", 0},
    {DI_BUTTON,   0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, GetMsg(MConfig), 0}};

  int dw = 75, dh = 20; SMALL_RECT farRect;
  if (g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &farRect, nullptr)) {
    dw = (farRect.Right - farRect.Left + 1) * 7 / 10;
    dh = (farRect.Bottom - farRect.Top + 1) * 7 / 10;
    if (dw < 75) dw = 75;
    if (dh < 20) dh = 20;
  }

  HANDLE h = g_far.DialogInit(g_far.ModuleNumber, -1, -1, dw, dh, NULL, it, sizeof(it)/sizeof(it[0]), 0, 0, MemoDlgProc, 0);
  if (h != (HANDLE)-1) {
    g_far.DialogRun(h);
    g_far.DialogFree(h);
  }
}

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
  g_far = *Info; InMyConfig(MEMO_DIR);
  KeyFileHelper kf(InMyConfig(INI_PATH, false));
  g_enabled = kf.GetInt(INI_S_SETTINGS, INI_K_ENABLED, 1);
  g_useHotkey = kf.GetInt(INI_S_SETTINGS, INI_K_HOTKEY, 1);
  g_currentMemo = kf.GetInt(INI_S_SETTINGS, INI_K_LAST, 0);
  DBG("SetStartupInfoW: enabled=%d, hotkey=%d, last=%d", (int)g_enabled, (int)g_useHotkey, g_currentMemo);
  if (g_useHotkey) {
    // If CtrlAltS already belongs to someone else, respect it and quietly disable our hotkey
    KeyFileHelper kfh(InMyConfig(MACROS_INI, false));
    if (kfh.HasSection(MACRO_SECTION) && !IsCurrentMacroOurs(kfh)) {
      DBG("SetStartupInfoW: CtrlAltS macro is foreign, disabling hotkey");
      g_useHotkey = false;
      SaveState();
    } else {
      UpdateGlobalMacro(true);
    }
  }
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  Info->StructSize = sizeof(PluginInfo);
  if (!GetEnabled()) {
    Info->Flags = 0; Info->PluginMenuStringsNumber = 0; Info->PluginConfigStringsNumber = 1;
    static const wchar_t *m; m = GetMsg(MMemo); Info->PluginConfigStrings = &m; return;
  }
  Info->Flags = PF_VIEWER | PF_DIALOG | PF_EDITOR;
  static const wchar_t *m; m = GetMsg(MMemo);
  Info->PluginMenuStrings = Info->PluginConfigStrings = &m;
  Info->PluginMenuStringsNumber = Info->PluginConfigStringsNumber = 1;
  Info->SysID = SYSID_MEMO;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int, INT_PTR) {
  OpenMemo();
  return (HANDLE)-1;
}

SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE) {}
SHAREDSYMBOL int WINAPI ProcessEditorEventW(int, void *) { return 0; }
SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE, int, void *) { return 0; }
