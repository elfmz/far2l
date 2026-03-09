#include "KeyFileHelper.h"
#include "WideMB.h"
#include "WinCompat.h"
#include "WinPort.h"
#include "farkeys.h"
#include "farplug-wide.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <cwchar>
#include <string>
#include <utils.h>
#include <vector>

#ifndef WINAPI
#define WINAPI
#endif

#define SYSID_MEMO 0x4D454D4F
#define MEMO_COUNT 10

// Returns base directory for memo files, creates if missing
static std::wstring GetMemoDir() {
  const char *home = getenv("HOME");
  std::wstring dir = home ? MB2Wide(home) + L"/.config/far2l/plugins/memo" : L"memos";
  if (home) {
    WINPORT(CreateDirectory)((MB2Wide(home) + L"/.config").c_str(), NULL);
    WINPORT(CreateDirectory)((MB2Wide(home) + L"/.config/far2l").c_str(), NULL);
    WINPORT(CreateDirectory)((MB2Wide(home) + L"/.config/far2l/plugins").c_str(), NULL);
    WINPORT(CreateDirectory)(dir.c_str(), NULL);
  }
  return dir;
}

// Debug helper - writes to ~/.config/far2l/plugins/memo/debug.log
// Completely absent from release builds (NDEBUG).
#if defined(DEBUG) || defined(_DEBUG)
static void DebugLog(const char *fmt, ...) {
  // Timestamp
  time_t now = time(nullptr);
  struct tm tm_info;
  localtime_r(&now, &tm_info);
  char ts[24];
  strftime(ts, sizeof(ts), "%H:%M:%S", &tm_info);

  // Format message
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  std::wstring wlogPath = GetMemoDir() + L"/debug.log";
  std::string mbLogPath = Wide2MB(wlogPath.c_str());
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

enum MsgId { MMemo = 0,  MOk,  MCancel,  MConfigTitle,  MEnablePlugin,  MUseHotkey,  MSaveAs,  MConfig };

enum DialogItems {
  DI_BOX = 0, DI_MEMO, 
  DI_F2, DI_BTN1, DI_BTN2, DI_BTN3, DI_BTN4, DI_BTN5, DI_BTN6, DI_BTN7, DI_BTN8, DI_BTN9, DI_BTN0, DI_F9
};

PluginStartupInfo g_far;
static int g_currentMemo = 0;
static std::wstring g_loadedContent;
static bool g_enabled = true;
static bool g_useHotkey = true;

static std::wstring GetMemoFilePath(int index) {
  std::wstring dir = GetMemoDir();
  wchar_t name[32];
  swprintf(name, 32, L"/memo-%02d.txt", (index == 9) ? 0 : index + 1);
  return dir + name;
}

// Retrieves last used memo index from state.ini
static int GetLastMemo() {
  int res = KeyFileHelper(InMyConfig("plugins/memo/state.ini")).GetInt("Settings", "LastMemo", 0);
  DBG("GetLastMemo: %d", res);
  return res;
}

// Persists current plugin state to state.ini
static void SaveLastMemo(int v) {
  DBG("SaveLastMemo: %d", v);
  g_currentMemo = v;
  KeyFileHelper kf(InMyConfig("plugins/memo/state.ini"));
  kf.SetInt("Settings", "Enabled", g_enabled ? 1 : 0);
  kf.SetInt("Settings", "HotkeyEnabled", g_useHotkey ? 1 : 0);
  kf.SetInt("Settings", "LastMemo", v);
  kf.Save();
}
static bool GetEnabled() { return g_enabled; }
// Toggles plugin availability and persists the choice
static void SetEnabled(bool v) {
  DBG("SetEnabled: %d", (int)v);
  g_enabled = v;
  KeyFileHelper kf(InMyConfig("plugins/memo/state.ini"));
  kf.SetInt("Settings", "Enabled", v ? 1 : 0);
  kf.SetInt("Settings", "HotkeyEnabled", g_useHotkey ? 1 : 0);
  kf.SetInt("Settings", "LastMemo", g_currentMemo);
  kf.Save();
}
// Updates key_macros.ini to register/unregister Ctrl+S for the plugin
static void UpdateGlobalMacro(bool enabled) {
  DBG("UpdateGlobalMacro: %d", (int)enabled);
  std::string iniPath = InMyConfig("settings/key_macros.ini");
  const char *section = "KeyMacros/Common/CtrlS";

  KeyFileHelper kfh(iniPath);
  if (enabled) {
    if (!kfh.HasSection(section)) {
      char seq[64];
      snprintf(seq, sizeof(seq), "callplugin(0x%08X)", SYSID_MEMO);
      kfh.SetString(section, "Sequence", seq);
      kfh.SetString(section, "Description", "Memo Plugin");
      kfh.SetString(section, "DisableOutput", "1");
      kfh.Save();

      ActlKeyMacro akm = {MCMD_LOADALL};
      g_far.AdvControl(g_far.ModuleNumber, ACTL_KEYMACRO, &akm, NULL);
    }
  } else {
    if (kfh.HasSection(section)) {
      kfh.RemoveSection(section);
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
  KeyFileHelper kf(InMyConfig("plugins/memo/state.ini"));
  kf.SetInt("Settings", "Enabled", g_enabled ? 1 : 0);
  kf.SetInt("Settings", "HotkeyEnabled", v ? 1 : 0);
  kf.SetInt("Settings", "LastMemo", g_currentMemo);
  kf.Save();
  UpdateGlobalMacro(v);
}

// Reads memo content from file
static std::wstring LoadFile(int idx) {
  std::wstring path = GetMemoFilePath(idx);
  DBG("LoadFile(%d): %ls", idx, path.c_str());
  std::string mbP = Wide2MB(path.c_str());
  FILE *f = fopen(mbP.c_str(), "r");
  if (!f)
    return L"";
  char buf[4096];
  std::string mb;
  while (fgets(buf, sizeof(buf), f))
    mb += buf;
  fclose(f);
  return MB2Wide(mb.c_str());
}

// Writes memo content to file, removes file if empty
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
  }
}

// Saves current memo and loads another one into the dialog
static void SwitchToMemo(HANDLE hDlg, int newMemo) {
  if (newMemo < 0 || newMemo >= MEMO_COUNT || newMemo == g_currentMemo)
    return;
  DBG("SwitchToMemo: %d -> %d", g_currentMemo, newMemo);
  size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
  std::wstring content;
  if (len > 0) {
    content.resize(len);
    g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO, (LONG_PTR)&content[0]);
  }
  if (content != g_loadedContent) {
    SaveFile(g_currentMemo, content);
  }
  g_currentMemo = newMemo;
  g_loadedContent = LoadFile(g_currentMemo);
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_MEMO, (LONG_PTR)g_loadedContent.c_str());

  wchar_t title[64];
  swprintf(title, 64, L" Memo - %d", (g_currentMemo == 9) ? 0 : g_currentMemo + 1);
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_BOX, (LONG_PTR)title);
  for (int i = 0; i < MEMO_COUNT; i++) {
    FarDialogItem it;
    if (g_far.SendDlgMessage(hDlg, DM_GETDLGITEM, DI_BTN1 + i, (LONG_PTR)&it)) {
      it.DefaultButton = false;
      g_far.SendDlgMessage(hDlg, DM_SETDLGITEM, DI_BTN1 + i, (LONG_PTR)&it);
    }
  }
  g_far.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
  g_far.SendDlgMessage(hDlg, DM_SETFOCUS, DI_MEMO, 0);
}

// Prompts for a path and saves current memo content there
static void SaveAs(HANDLE hDlg) {
  size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
  std::wstring content;
  if (len > 0) {
    content.resize(len);
    g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO, (LONG_PTR)&content[0]);
  }
  wchar_t path[MAX_PATH];
  swprintf(path, MAX_PATH, L"memo-%02d.txt", (g_currentMemo == 9) ? 0 : g_currentMemo + 1);
  if (g_far.InputBox(L"Save Memo", L"Path:", L"MemoSave", path, path, MAX_PATH, NULL, FIB_NONE)) {
    DBG("SaveAs: %ls", path);
    FILE *f = fopen(Wide2MB(path).c_str(), "w");
    if (f) {
      fputs(Wide2MB(content.c_str()).c_str(), f);
      fclose(f);
    }
  }
}

// Plugin configuration dialog
SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber) {
  FarDialogItem it[] = {
      {DI_DOUBLEBOX, 3, 1, 32, 8, 0, {0}, 0, 0, g_far.GetMsg(g_far.ModuleNumber, MConfigTitle), 0},
      {DI_CHECKBOX, 6, 3, 0, 0, 0, {(DWORD_PTR)(GetEnabled() ? BSTATE_CHECKED : BSTATE_UNCHECKED)}, 0, 0,
          g_far.GetMsg(g_far.ModuleNumber, MEnablePlugin), 0},
      {DI_CHECKBOX, 6, 4, 0, 0, 0, {(DWORD_PTR)(GetUseHotkey() ? BSTATE_CHECKED : BSTATE_UNCHECKED)}, 0, 0,
          g_far.GetMsg(g_far.ModuleNumber, MUseHotkey), 0},
      {DI_TEXT, -1, 6, 0, 0, 0, {0}, DIF_SEPARATOR, 0, NULL, 0},
      {DI_BUTTON, 0, 7, 0, 0, 0, {0}, DIF_CENTERGROUP, 1, g_far.GetMsg(g_far.ModuleNumber, MOk), 0},
      {DI_BUTTON, 0, 7, 0, 0, 0, {0}, DIF_CENTERGROUP, 0, g_far.GetMsg(g_far.ModuleNumber, MCancel), 0}
  };
  
  HANDLE d = g_far.DialogInit(g_far.ModuleNumber, -1, -1, 36, 10, NULL, it, 
    sizeof(it) / sizeof(it[0]), 0, 0, NULL, 0);
  if (d != (HANDLE)-1) {
    if (g_far.DialogRun(d) == 4) { // Index of OK button
      DBG("ConfigureW: status OK");
      SetEnabled(g_far.SendDlgMessage(d, DM_GETCHECK, 1, 0) == BSTATE_CHECKED);
      SetUseHotkey(g_far.SendDlgMessage(d, DM_GETCHECK, 2, 0) == BSTATE_CHECKED);
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
      COORD pos = {(SHORT)((pConsoleSize->X - w) / 2),
                   (SHORT)((pConsoleSize->Y - h) / 2)};
      g_far.SendDlgMessage(hDlg, DM_MOVEDIALOG, 1, (LONG_PTR)&pos);
    }
  }

  SMALL_RECT r;
  // Box
  r = {0, 0, (SHORT)(w - 1), (SHORT)(h - 1)};
  g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_BOX, (LONG_PTR)&r);

  // Memo
  r = {1, 1, (SHORT)(w - 2), (SHORT)(h - 2)};
  g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_MEMO, (LONG_PTR)&r);

  // F2 SaveAs: 11 chars [F2 SaveAs]
  r = {1, (SHORT)(h - 1), 11, (SHORT)(h - 1)};
  g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_F2, (LONG_PTR)&r);

  // F9 Config: 11 chars [F9 Config]
  r = {(SHORT)(w - 12), (SHORT)(h - 1), (SHORT)(w - 2), (SHORT)(h - 1)};
  g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_F9, (LONG_PTR)&r);

  // Digits: 10 * 3 = 30 chars. Center them in the space
  int totalDigitsWidth = 10 * 3;
  int availableSpace = (w - 2) - 11 - 11; // between F2 and F9
  int startDigitsX = 12 + (availableSpace - totalDigitsWidth) / 2;
  int curX = startDigitsX;
  for (int i = 0; i < 10; i++) {
    r = {(SHORT)curX, (SHORT)(h - 1), (SHORT)(curX + 2), (SHORT)(h - 1)};
    g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_BTN1 + i, (LONG_PTR)&r);
    curX += 3;
  }
}

static LONG_PTR WINAPI MemoDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2) {
  if (Msg == DN_INITDIALOG) {
    SMALL_RECT r;
    if (g_far.SendDlgMessage(hDlg, DM_GETDLGRECT, 0, (LONG_PTR)&r)) {
      UpdateLayout(hDlg, r.Right - r.Left + 1, r.Bottom - r.Top + 1, false);
    }
    return TRUE;
  }
  if (Msg == DN_RESIZECONSOLE) {
    COORD *pCoord = (COORD *)Param2;
    int w = pCoord->X * 7 / 10;
    int h = pCoord->Y * 7 / 10;
    if (w < 75)
      w = 75;
    if (h < 20)
      h = 20;
    UpdateLayout(hDlg, w, h, true, pCoord);
    return TRUE;
  }
  if (Msg == DN_CTLCOLORDLGITEM) {
    if (Param1 >= DI_BTN1 && Param1 <= DI_BTN0) {
      if (static_cast<int>(Param1 - DI_BTN1) == g_currentMemo) {
        uint64_t *colors = reinterpret_cast<uint64_t *>(Param2);
        colors[0] = colors[4]; // Normal -> Focused
      }
    }
    return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
  }
  if (Msg == DN_BTNCLICK) {
    if (Param1 == DI_F2)
      SaveAs(hDlg);
    else if (Param1 == DI_F9)
      ConfigureW(0);
    else if (Param1 >= DI_BTN1 && Param1 <= DI_BTN0)
      SwitchToMemo(hDlg, Param1 - DI_BTN1);
    return TRUE;
  }
  if (Msg == DN_KEY) {
    if (Param2 == KEY_F2) {
      SaveAs(hDlg);
      return TRUE;
    }
    if (Param2 == KEY_F9) {
      ConfigureW(0);
      return TRUE;
    }
    if (Param2 >= KEY_CTRL1 && Param2 <= KEY_CTRL9) {
      SwitchToMemo(hDlg, (int)(Param2 - KEY_CTRL1));
      return TRUE;
    }
    if (Param2 == KEY_CTRL0) {
      SwitchToMemo(hDlg, 9);
      return TRUE;
    }
    if (Param2 >= KEY_ALT1 && Param2 <= KEY_ALT9) {
      SwitchToMemo(hDlg, (int)(Param2 - KEY_ALT1));
      return TRUE;
    }
    if (Param2 == KEY_ALT0) {
      SwitchToMemo(hDlg, 9);
      return TRUE;
    }
    if (Param2 == KEY_CTRLS) {
      g_far.SendDlgMessage(hDlg, DM_CLOSE, -1, 0);
      return TRUE;
    }
  }
  if (Msg == DN_CLOSE) {
    size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
    std::wstring content;
    if (len > 0) {
      content.resize(len);
      g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO, (LONG_PTR)&content[0]);
    }
    if (content != g_loadedContent) {
      SaveFile(g_currentMemo, content);
    }
    SaveLastMemo(g_currentMemo);
  }
  return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

// Main entry point for opening the memo editor dialog
static void OpenMemo() {
  g_currentMemo = GetLastMemo();
  g_loadedContent = LoadFile(g_currentMemo);
  DBG("OpenMemo: cur=%d, len=%d", g_currentMemo, (int)g_loadedContent.length());
  wchar_t title[64];
  swprintf(title, 64, L" Memo - %d", (g_currentMemo == 9) ? 0 : g_currentMemo + 1);

  FarDialogItem it[] = {
      {DI_TEXT, 0, 0, 0, 0, 0, {0}, 0, 0, title, 0},
      {DI_MEMOEDIT, 0, 0, 0, 0, 1, {0}, 0, 0, g_loadedContent.c_str(), 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[F2 Save]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[1]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[2]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[3]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[4]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[5]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[6]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[7]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[8]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[9]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[0]", 0},
      {DI_BUTTON, 0, 0, 0, 0, 0, {0}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, 0, L"[F9 Config]", 0}};

  int dw = 75, dh = 20;
  SMALL_RECT farRect;
  if (g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &farRect, nullptr)) {
    dw = (farRect.Right - farRect.Left + 1) * 7 / 10;
    dh = (farRect.Bottom - farRect.Top + 1) * 7 / 10;
    if (dw < 75) dw = 75;
    if (dh < 20) dh = 20;
  }

  HANDLE h = g_far.DialogInit(g_far.ModuleNumber, -1, -1, dw, dh, NULL, it, 
    sizeof(it) / sizeof(it[0]), 0, 0, MemoDlgProc, 0);
  if (h != (HANDLE)-1) {
    g_far.DialogRun(h);
    g_far.DialogFree(h);
  }
}

// FAR API: Initializes plugin, loads configuration
SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
  g_far = *Info;
  KeyFileHelper kf(InMyConfig("plugins/memo/state.ini"));
  g_enabled = kf.GetInt("Settings", "Enabled", 1);
  g_useHotkey = kf.GetInt("Settings", "HotkeyEnabled", 1);
  g_currentMemo = kf.GetInt("Settings", "LastMemo", 0);
  DBG("SetStartupInfoW: enabled=%d, hotkey=%d, last=%d", (int)g_enabled, (int)g_useHotkey, g_currentMemo);
  if (g_useHotkey) {
    UpdateGlobalMacro(true);
  }
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  Info->StructSize = sizeof(PluginInfo);
  if (!GetEnabled()) {
    Info->Flags = 0;
    Info->PluginMenuStringsNumber = 0;
    Info->PluginConfigStringsNumber = 1; // Keep it in config so we can re-enable

    static const wchar_t *m;
    m = g_far.GetMsg(g_far.ModuleNumber, MMemo);
    Info->PluginConfigStrings = &m;
    return;
  }
  Info->Flags = PF_VIEWER | PF_DIALOG | PF_EDITOR;
  static const wchar_t *m;
  m = g_far.GetMsg(g_far.ModuleNumber, MMemo);
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
