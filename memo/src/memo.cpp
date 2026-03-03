#include <WideMB.h>
#include <WinCompat.h>
#include <WinPort.h>
#include <farkeys.h>
#include <farplug-wide.h>

#ifndef WINAPI
#define WINAPI
#endif

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <fstream>
#include <string>
#include <vector>

// Unique ID for plugin (must be non-zero)
#define SYSID_MEMO 0x53637274
#define MEMO_COUNT 10

// String IDs matching line order in memoe.lng / memor.lng
enum MsgId {
  MMemo = 0,     // "Memo"               - plugin name / menu item
  MOk,           // "&OK"
  MCancel,       // "&Cancel"
  MConfigTitle,  // "Memo Plugin Configuration"
  MEnablePlugin, // "&Enable Memo Plugin"
};

// ---------------------------------------------------------------------------
// Debug logging - completely compiled out in release builds (NDEBUG)
// Log file: ~/.config/far2l/plugins/memo/debug.log
// Usage:    DBG("format %s %d", strVal, intVal)
// ---------------------------------------------------------------------------
#ifdef NDEBUG
#define DBG(fmt, ...) ((void)0)
#else
#define DBG(fmt, ...) DebugLog(fmt, ##__VA_ARGS__)
#endif

// Dialog item indices
enum DialogItems {
  DI_TITLE = 0, // Dialog title
  DI_MEMO,      // Main memo editor (DI_MEMOEDIT)
  DI_INDICATOR, // Page indicator at bottom
};

// Far2l API - set by SetStartupInfoW
PluginStartupInfo g_far;
static int g_currentMemo = 0; // Current memo index (0-9)

// Forward declaration
static const std::wstring &GetMemoDir();

// Debug helper - writes to ~/.config/far2l/plugins/memo/debug.log
// Completely absent from release builds (NDEBUG).
#ifndef NDEBUG
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
#endif

// Get memo storage directory (cached, created on first call)
static const std::wstring &GetMemoDir() {
  static std::wstring dir = []() {
    const char *home = getenv("HOME");
    if (home) {
      std::string h = home;
      std::wstring wh(h.begin(), h.end());
      std::wstring d = wh + L"/.config/far2l/plugins/memo";

      WINPORT(CreateDirectory)((wh + L"/.config").c_str(), NULL);
      WINPORT(CreateDirectory)((wh + L"/.config/far2l").c_str(), NULL);
      WINPORT(CreateDirectory)((wh + L"/.config/far2l/plugins").c_str(), NULL);
      WINPORT(CreateDirectory)(d.c_str(), NULL);

      return d;
    }

    // Fallback: store next to plugin module
    std::wstring modulePath = g_far.ModuleName;
    size_t lastSlash = modulePath.find_last_of(L"/\\");
    if (lastSlash != std::wstring::npos) {
      std::wstring d = modulePath.substr(0, lastSlash) + L"/memos";
      WINPORT(CreateDirectory)(d.c_str(), NULL);
      return d;
    }
    return std::wstring(L"memos");
  }();
  return dir;
}

// Get memo file path: memo-00.txt ... memo-09.txt
static std::wstring GetMemoFilePath(int index) {
  std::wstring name = L"memo-";
  if (index < MEMO_COUNT)
    name += L"0";
  name += std::to_wstring(index);
  return GetMemoDir() + L"/" + name + L".txt";
}

// Get state file path for persisting last memo index
static std::wstring GetStateFilePath() { return GetMemoDir() + L"/state.ini"; }

// Load last selected memo index from state.ini
static int LoadLastMemoIndex() {
  std::wstring statePath = GetStateFilePath();
  std::string mbPath =
      Wide2MB(statePath.c_str()); // UTF-8 path for std::ifstream
  std::ifstream f(mbPath);
  if (f.is_open()) {
    std::string line;
    while (std::getline(f, line)) {
      if (line.find("LastMemo=") == 0) {
        try {
          int idx = std::stoi(line.substr(9));
          if (idx >= 0 && idx < MEMO_COUNT) {
            return idx;
          }
        } catch (...) {
          // invalid value, ignore
        }
      }
    }
  }
  return 0;
}

// Save last selected memo index to state.ini
static void SaveLastMemoIndex(int index) {
  std::wstring statePath = GetStateFilePath();
  std::string mbPath = Wide2MB(statePath.c_str());

  std::vector<std::string> lines;
  bool found = false;

  std::ifstream f_in(mbPath);
  if (f_in.is_open()) {
    std::string line;
    while (std::getline(f_in, line)) {
      if (line.find("LastMemo=") == 0) {
        lines.push_back("LastMemo=" + std::to_string(index));
        found = true;
      } else {
        lines.push_back(line);
      }
    }
  }

  if (!found) {
    lines.push_back("LastMemo=" + std::to_string(index));
  }

  std::ofstream f_out(mbPath);
  if (!f_out.is_open()) {
    return;
  }

  for (const auto &l : lines) {
    f_out << l << "\n";
  }
}

// Load enabled flag from state.ini (default: enabled)
static bool IsEnabled() {
  std::wstring statePath = GetStateFilePath();
  std::string mbPath = Wide2MB(statePath.c_str());
  std::ifstream f(mbPath);
  if (f.is_open()) {
    std::string line;
    while (std::getline(f, line)) {
      if (line.find("Enabled=") == 0) {
        return line.substr(8) != "0";
      }
    }
  }
  return true; // enabled by default
}

// Save enabled flag to state.ini
static void SaveEnabled(bool enabled) {
  std::wstring statePath = GetStateFilePath();
  std::string mbPath = Wide2MB(statePath.c_str());

  std::vector<std::string> lines;
  bool found = false;

  std::ifstream f_in(mbPath);
  if (f_in.is_open()) {
    std::string line;
    while (std::getline(f_in, line)) {
      if (line.find("Enabled=") == 0) {
        lines.push_back(std::string("Enabled=") + (enabled ? "1" : "0"));
        found = true;
      } else {
        lines.push_back(line);
      }
    }
  }

  if (!found) {
    lines.push_back(std::string("Enabled=") + (enabled ? "1" : "0"));
  }

  std::ofstream f_out(mbPath);
  if (!f_out.is_open())
    return;
  for (const auto &l : lines) {
    f_out << l << "\n";
  }
}

// Get user's home directory for default Save As path
static std::wstring GetHomeDir() {
  const char *home = getenv("HOME");
  if (home) {
    std::string h = home;
    return std::wstring(h.begin(), h.end());
  }
  return L".";
}

// Load file content as wide string (UTF-8 -> wchar_t)
static std::wstring LoadFileContent(const std::wstring &path) {
  std::wstring content;
  std::string mbPath = Wide2MB(path.c_str());
  FILE *f = fopen(mbPath.c_str(), "r");
  if (f) {
    char buf[4096];
    std::string mbContent;
    while (fgets(buf, sizeof(buf), f)) {
      mbContent += buf;
    }
    fclose(f);
    if (!mbContent.empty()) {
      content = MB2Wide(mbContent.c_str());
    }
    DBG("file-read: %s (%zu chars)", mbPath.c_str(), content.size());
  } else {
    DBG("file-read: %s -> NOT FOUND (new/empty)", mbPath.c_str());
  }
  return content;
}

// Save wide string to file (wchar_t -> UTF-8)
static bool SaveFileContent(const std::wstring &path,
                            const std::wstring &content) {
  std::string mbPath = Wide2MB(path.c_str());
  FILE *f = fopen(mbPath.c_str(), "w");
  if (!f) {
    DBG("file-write: FAILED to open %s", mbPath.c_str());
    return false;
  }
  std::string mbContent = Wide2MB(content.c_str());
  if (fputs(mbContent.c_str(), f) == EOF) {
    fclose(f);
    DBG("file-write: FAILED to write %s", mbPath.c_str());
    return false;
  }
  fclose(f);
  DBG("file-write: OK %s (%zu chars)", mbPath.c_str(), content.size());
  return true;
}

// Indicator strings with current page marked by brackets: [1] 2 3 ...
static_assert(MEMO_COUNT == 10, "MEMO_COUNT must be 10");
static const wchar_t *GetIndicatorWithX(int targetMemo) {
  static const wchar_t *indicators[MEMO_COUNT] = {
      L"\x2022[&1]\x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 "
      L"8 \x2022 9 \x2022 0 \x2022",
      L"\x2022 1 \x2022[&2]\x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 "
      L"8 \x2022 9 \x2022 0 \x2022",
      L"\x2022 1 \x2022 2 \x2022[&3]\x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 "
      L"8 \x2022 9 \x2022 0 \x2022",
      L"\x2022 1 \x2022 2 \x2022 3 \x2022[&4]\x2022 5 \x2022 6 \x2022 7 \x2022 "
      L"8 \x2022 9 \x2022 0 \x2022",
      L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022[&5]\x2022 6 \x2022 7 \x2022 "
      L"8 \x2022 9 \x2022 0 \x2022",
      L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022[&6]\x2022 7 \x2022 "
      L"8 \x2022 9 \x2022 0 \x2022",
      L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022[&7]\x2022 "
      L"8 \x2022 9 \x2022 0 \x2022",
      L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 "
      L"\x2022[&8]\x2022 9 \x2022 0 \x2022",
      L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 "
      L"8 \x2022[&9]\x2022 0 \x2022",
      L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 "
      L"8 \x2022 9 \x2022[&0]\x2022"};
  if (targetMemo < 0 || targetMemo >= MEMO_COUNT) {
    targetMemo = 0;
  }
  return indicators[targetMemo];
}

// Update indicator text to show current page
static void ShowXInIndicator(HANDLE hDlg, int targetMemo) {
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_INDICATOR,
                       (LONG_PTR)GetIndicatorWithX(targetMemo));
}

// Get text from memo editor via dialog messages
static std::wstring GetMemoText(HANDLE hDlg) {
  size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
  std::wstring content;
  if (len > 0) {
    content.resize(len);
    g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO,
                         (LONG_PTR)content.data());
  }
  return content;
}

// Save current memo content to file
static void SaveCurrentMemo(HANDLE hDlg) {
  std::wstring content = GetMemoText(hDlg);
  SaveFileContent(GetMemoFilePath(g_currentMemo), content);
}

// Switch to different memo - saves current, loads new, updates UI
static void SwitchToMemo(HANDLE hDlg, int newMemo) {
  if (newMemo < 0 || newMemo >= MEMO_COUNT || newMemo == g_currentMemo) {
    DBG("switch-memo: ignored (newMemo=%d currentMemo=%d)", newMemo,
        g_currentMemo);
    return;
  }

  DBG("switch-memo: %d -> %d (auto-saving current first)", g_currentMemo,
      newMemo);
  SaveCurrentMemo(hDlg); // Auto-save before switching

  g_currentMemo = newMemo;
  std::wstring newContent = LoadFileContent(GetMemoFilePath(g_currentMemo));
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_MEMO,
                       (LONG_PTR)newContent.c_str());

  // Update title: "Memo" -> "Memo - 1", etc.
  std::wstring title =
      L"[ Memo - " + std::to_wstring(g_currentMemo + 1) + L" ]";
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_TITLE, (LONG_PTR)title.c_str());

  ShowXInIndicator(hDlg, newMemo);
  DBG("switch-memo: done, now on memo-%d", newMemo);
}

// Save current memo to external file (F2/Shift+F2)
static bool SaveMemoAs(HANDLE hDlg) {
  std::wstring content = GetMemoText(hDlg);

  // Default: memo-01.txt ... memo-10.txt in home directory
  int memoNum = (g_currentMemo + 1) % MEMO_COUNT;
  std::wstring defaultName = L"memo-";
  if (memoNum < MEMO_COUNT)
    defaultName += L"0";
  defaultName += std::to_wstring(memoNum) + L".txt";

  std::wstring homePath = GetHomeDir();
  std::wstring defaultPath = homePath + L"/" + defaultName;

  wchar_t destPath[MAX_PATH];
  wcsncpy(destPath, defaultPath.c_str(), MAX_PATH - 1);
  destPath[MAX_PATH - 1] = 0;

  DBG("key-F2: Save As dialog opened, default=%ls", defaultPath.c_str());

  if (g_far.InputBox(L"Save Memo", L"Enter destination path:", L"MemoSave",
                     defaultPath.c_str(), destPath, MAX_PATH, NULL, FIB_NONE)) {
    DBG("key-F2: user confirmed export to %ls", destPath);
    bool ok = SaveFileContent(destPath, content);
    DBG("key-F2: export %s (%zu chars)", ok ? "OK" : "FAILED", content.size());
    return ok;
  }

  DBG("key-F2: Save As cancelled by user");
  return false;
}

// Dialog procedure - handles keyboard and close events
// DN_KEY: intercepts keys for memo switching
// DN_CLOSE: saves content and state
static LONG_PTR WINAPI MemoDlgProc(HANDLE hDlg, int Msg, int Param1,
                                   LONG_PTR Param2) {
  switch (Msg) {
  case DN_KEY:
    if (Param1 == DI_MEMO) {
      int key = (int)Param2;

      // ESC closes dialog - DN_CLOSE will save
      if (key == KEY_ESC) {
        DBG("key-ESC: closing dialog (auto-save will follow in DN_CLOSE)");
        g_far.SendDlgMessage(hDlg, DM_CLOSE, 0, 0);
        return TRUE;
      }

      // F2/Shift+F2: Save As
      if (key == KEY_F2 || key == KEY_SHIFTF2) {
        DBG("key-F2: Save As triggered (key=0x%x, memo=%d)", key,
            g_currentMemo);
        SaveMemoAs(hDlg);
        return TRUE;
      }

      // Ctrl+0-9 or Alt+0-9: switch memo
      int memoIdx = -1;
      const char *modName = "?";

      if (key == KEY_CTRL0 || key == KEY_ALT0) {
        memoIdx = MEMO_COUNT - 1; // 0 -> 9
        modName = (key == KEY_CTRL0) ? "Ctrl" : "Alt";
      } else if (key >= KEY_CTRL1 && key <= KEY_CTRL9) {
        memoIdx = key - KEY_CTRL1; // 1-9
        modName = "Ctrl";
      } else if (key >= KEY_ALT1 && key <= KEY_ALT9) {
        memoIdx = key - KEY_ALT1;
        modName = "Alt";
      }

      if (memoIdx >= 0 && memoIdx < MEMO_COUNT) {
        DBG("key-%s+%d: switching memo %d -> %d", modName, (memoIdx + 1) % 10,
            g_currentMemo, memoIdx);
        SwitchToMemo(hDlg, memoIdx);
        return TRUE;
      }
    }
    break;

  case DN_CLOSE:
    DBG("dialog-close: saving memo-%d and state", g_currentMemo);
    SaveCurrentMemo(hDlg);
    SaveLastMemoIndex(g_currentMemo);
    DBG("dialog-close: done");
    break;
  }

  return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

// Create and run the memo dialog
static void OpenMemoDialog() {
  g_currentMemo = LoadLastMemoIndex();
  DBG("dialog-open: starting with memo-%d", g_currentMemo);

  // Get console size for dialog dimensions
  SMALL_RECT screenRect;
  int screenWidth = 80;
  int screenHeight = 25;
  if (g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &screenRect,
                       NULL)) {
    screenWidth = screenRect.Right - screenRect.Left + 1;
    screenHeight = screenRect.Bottom - screenRect.Top + 1;
  }

  int dlgWidth = screenWidth - 20;
  int dlgHeight = screenHeight - 10;

  // Center dialog
  int x1 = -1;
  int y1 = -1;
  int x2 = x1 + dlgWidth - 1;
  int y2 = y1 + dlgHeight - 1;

  std::wstring content = LoadFileContent(GetMemoFilePath(g_currentMemo));
  std::wstring title =
      L"[ Memo - " + std::to_wstring(g_currentMemo + 1) + L" ]";

  FarDialogItem items[3] = {};

  // Title at top
  items[DI_TITLE].Type = DI_TEXT;
  items[DI_TITLE].X1 = 1;
  items[DI_TITLE].Y1 = 0;
  items[DI_TITLE].X2 = dlgWidth - 2;
  items[DI_TITLE].Flags = DIF_CENTERTEXT;
  items[DI_TITLE].PtrData = title.c_str();

  // Main memo editor - multiline
  items[DI_MEMO].Type = DI_MEMOEDIT;
  items[DI_MEMO].X1 = 1;
  items[DI_MEMO].Y1 = 1;
  items[DI_MEMO].X2 = dlgWidth - 4;
  items[DI_MEMO].Y2 = dlgHeight - 4;
  items[DI_MEMO].Focus = 1;
  items[DI_MEMO].Flags = DIF_SHOWAMPERSAND;
  items[DI_MEMO].PtrData = content.c_str();

  // Page indicator at bottom
  items[DI_INDICATOR].Type = DI_TEXT;
  items[DI_INDICATOR].X1 = 1;
  items[DI_INDICATOR].Y1 = dlgHeight - 3;
  items[DI_INDICATOR].X2 = dlgWidth - 2;
  items[DI_INDICATOR].Flags = DIF_CENTERTEXT;
  items[DI_INDICATOR].PtrData = GetIndicatorWithX(g_currentMemo);

  HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, x1, y1, x2, y2, NULL,
                                 items, 3, 0, 0, MemoDlgProc, 0);

  if (hDlg != INVALID_HANDLE_VALUE) {
    DBG("dialog-open: UI rendered (memo-%d, size=%dx%d)", g_currentMemo,
        dlgWidth, dlgHeight);
    g_far.DialogRun(hDlg);
    g_far.DialogFree(hDlg);
    DBG("dialog-open: UI closed");
  } else {
    DBG("dialog-open: DialogInit FAILED");
  }
}

// ========== Far2l Plugin API ==========

// Called once at plugin load - save API pointer
SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
  g_far = *Info;
  // [1] Plugin registration
  // Note: GetMemoDir() is safe to call here since g_far is already set above.
  std::string mbModule = Wide2MB(Info->ModuleName);
  std::string mbDataDir = Wide2MB(GetMemoDir().c_str());
  DBG("=== memo plugin loaded ===");
  DBG("startup: module   = %s", mbModule.c_str());
  DBG("startup: data-dir = %s", mbDataDir.c_str());
  DBG("startup: build    = " __DATE__ " " __TIME__);
#if defined(__APPLE__)
  DBG("startup: platform = macOS (arm64/x86_64)");
#elif defined(__linux__)
  DBG("startup: platform = Linux");
#else
  DBG("startup: platform = unknown");
#endif
  // [2] Key registration info (hotkey is set via key_macros.ini, not in code)
  DBG("startup: hotkey   = Ctrl+S / Cmd+S (via key_macros.ini CallPlugin)");
  DBG("startup: prefix   = \"memo\" (command line prefix)");
  DBG("startup: SYSID    = 0x%08X", SYSID_MEMO);
}

// Return plugin info - menu items, command prefix, etc.
SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  Info->StructSize = sizeof(PluginInfo);
  Info->Flags = PF_VIEWER | PF_DIALOG;

  static const wchar_t *menu_strings[1];
  menu_strings[0] = L"Memo";

  Info->PluginConfigStrings = menu_strings;
  Info->PluginConfigStringsNumber = 1;
  Info->PluginMenuStrings = menu_strings;
  Info->PluginMenuStringsNumber = 1;

  Info->CommandPrefix = L"memo";
  Info->SysID = SYSID_MEMO;
}

// Not used - editor events handled by Far2l internally
SHAREDSYMBOL int WINAPI ProcessEditorEventW(int Event, void *Param) {
  return 0;
}

// Not used - plugin doesn't need panel events
SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void *Param) {
  return 0;
}

// Open plugin - show memo dialog
SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item) {
  // [3] Ctrl+S / Cmd+S pressed
  DBG("invoke: OpenPluginW called (OpenFrom=%d)", OpenFrom);

  // Check plugin enabled flag
  if (!IsEnabled()) {
    DBG("invoke: REJECTED - plugin is disabled (Enabled=0 in state.ini)");
    return INVALID_HANDLE_VALUE;
  }

  // DI_MEMOEDIT triggers EE_GOTFOCUS to all plugins (including colorer),
  // which crashes colorer when there is no backing file for the dialog editor.
  // Detect if we are inside an active editor and skip the dialog in that case.
  EditorInfo ei = {};
  g_far.EditorControl(ECTL_GETINFO, &ei);
  if (ei.EditorID != 0) {
    DBG("invoke: REJECTED - called from within editor (EditorID=%d)",
        ei.EditorID);
    return INVALID_HANDLE_VALUE;
  }

  // Also reject if panels don't exist (standalone mode or restricted context).
  // This further protects against crashes in standalone editor/viewer mode.
  if (!g_far.Control(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0, 0)) {
    DBG("invoke: REJECTED - panels don't exist (standalone mode?)");
    return INVALID_HANDLE_VALUE;
  }

  DBG("invoke: ACCEPTED - opening memo dialog");
  OpenMemoDialog();
  return INVALID_HANDLE_VALUE;
}

// Not used - no cleanup needed
SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin) {}

// Configure dialog - called from F11 -> Plugin Config -> Memo
// Layout matches incsrch style: tight margins, no inner separator,
// centred OK/Cancel buttons at the bottom.
SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber) {
  bool curEnabled = IsEnabled();

  // Width: box from col 3 to 35 (inner text 5..33), total dialog width 38
  const int W = 38;
  enum { DI_CFG_BOX = 0, DI_CFG_ENABLE, DI_CFG_OK, DI_CFG_CANCEL };

  FarDialogItem items[4] = {};

  // Outer double box
  items[DI_CFG_BOX].Type = DI_DOUBLEBOX;
  items[DI_CFG_BOX].X1 = 3;
  items[DI_CFG_BOX].Y1 = 1;
  items[DI_CFG_BOX].X2 = W - 3;
  items[DI_CFG_BOX].Y2 = 6;
  items[DI_CFG_BOX].PtrData = g_far.GetMsg(g_far.ModuleNumber, MConfigTitle);

  // Checkbox - flush to left edge of box interior (X1=5)
  items[DI_CFG_ENABLE].Type = DI_CHECKBOX;
  items[DI_CFG_ENABLE].X1 = 5;
  items[DI_CFG_ENABLE].Y1 = 3;
  items[DI_CFG_ENABLE].X2 = W - 5;
  items[DI_CFG_ENABLE].Y2 = 3;
  items[DI_CFG_ENABLE].Param.Selected = curEnabled ? 1 : 0;
  items[DI_CFG_ENABLE].PtrData =
      g_far.GetMsg(g_far.ModuleNumber, MEnablePlugin);

  // OK button
  items[DI_CFG_OK].Type = DI_BUTTON;
  items[DI_CFG_OK].Y1 = 5;
  items[DI_CFG_OK].Flags = DIF_CENTERGROUP;
  items[DI_CFG_OK].DefaultButton = 1;
  items[DI_CFG_OK].PtrData = g_far.GetMsg(g_far.ModuleNumber, MOk);

  // Cancel button
  items[DI_CFG_CANCEL].Type = DI_BUTTON;
  items[DI_CFG_CANCEL].Y1 = 5;
  items[DI_CFG_CANCEL].Flags = DIF_CENTERGROUP;
  items[DI_CFG_CANCEL].PtrData = g_far.GetMsg(g_far.ModuleNumber, MCancel);

  HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, -1, -1, W, 8, nullptr,
                                 items, 4, 0, 0, nullptr, 0);

  if (hDlg == INVALID_HANDLE_VALUE)
    return FALSE;

  int result = g_far.DialogRun(hDlg);
  if (result == DI_CFG_OK) {
    bool newEnabled = (g_far.SendDlgMessage(hDlg, DM_GETCHECK, DI_CFG_ENABLE,
                                            0) == BSTATE_CHECKED);
    SaveEnabled(newEnabled);
    g_far.DialogFree(hDlg);
    return TRUE;
  }

  g_far.DialogFree(hDlg);
  return FALSE;
}
