#include <WideMB.h>
#include <WinCompat.h>
#include <WinPort.h>
#include <farkeys.h>
#include <farplug-wide.h>

#ifndef WINAPI
#define WINAPI
#endif

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <string>
#include <vector>

// Unique ID for plugin (must be non-zero)
#define SYSID_MEMO 0x53637274
#define MEMO_COUNT 10

#ifdef NDEBUG
#define DBG(msg) ((void)0)
#else
#define DBG(msg) DebugLog(msg)
#endif

// Dialog item indices
enum DialogItems {
  DI_TITLE = 0,    // Dialog title
  DI_MEMO,        // Main memo editor (DI_MEMOEDIT)
  DI_INDICATOR,   // Page indicator at bottom
};

// Far2l API - set by SetStartupInfoW
PluginStartupInfo g_far;
static int g_currentMemo = 0;  // Current memo index (0-9)

// Forward declaration
static const std::wstring& GetMemoDir();

// Debug helper - writes to ~/.config/far2l/plugins/memo/debug.log
#ifndef NDEBUG
static void DebugLog(const char *msg) {
  std::wstring wlogPath = GetMemoDir() + L"/debug.log";
  std::string mbLogPath = Wide2MB(wlogPath.c_str());
  FILE *f = fopen(mbLogPath.c_str(), "a");
  if (f) {
    fprintf(f, "[memo] %s\n", msg);
    fclose(f);
  }
}
#endif

// Get memo storage directory (cached, created on first call)
static const std::wstring& GetMemoDir() {
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
  if (index < MEMO_COUNT) name += L"0";
  name += std::to_wstring(index);
  return GetMemoDir() + L"/" + name + L".txt";
}

// Get state file path for persisting last memo index
static std::wstring GetStateFilePath() {
  return GetMemoDir() + L"/state.ini";
}

// Load last selected memo index from state.ini
static int LoadLastMemoIndex() {
  std::wstring statePath = GetStateFilePath();
  std::string mbPath = Wide2MB(statePath.c_str());  // UTF-8 path for std::ifstream
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

  for (const auto& l : lines) {
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
  }
  return content;
}

// Save wide string to file (wchar_t -> UTF-8)
static bool SaveFileContent(const std::wstring &path, const std::wstring &content) {
  std::string mbPath = Wide2MB(path.c_str());
  FILE *f = fopen(mbPath.c_str(), "w");
  if (!f) {
    return false;
  }
  std::string mbContent = Wide2MB(content.c_str());
  if (fputs(mbContent.c_str(), f) == EOF) {
    fclose(f);
    return false;
  }
  fclose(f);
  return true;
}

// Indicator strings with current page marked by brackets: [1] 2 3 ...
static_assert(MEMO_COUNT == 10, "MEMO_COUNT must be 10");
static const wchar_t* GetIndicatorWithX(int targetMemo) {
  static const wchar_t* indicators[MEMO_COUNT] = {
    L"\x2022[&1]\x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 8 \x2022 9 \x2022 0 \x2022",
    L"\x2022 1 \x2022[&2]\x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 8 \x2022 9 \x2022 0 \x2022",
    L"\x2022 1 \x2022 2 \x2022[&3]\x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 8 \x2022 9 \x2022 0 \x2022",
    L"\x2022 1 \x2022 2 \x2022 3 \x2022[&4]\x2022 5 \x2022 6 \x2022 7 \x2022 8 \x2022 9 \x2022 0 \x2022",
    L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022[&5]\x2022 6 \x2022 7 \x2022 8 \x2022 9 \x2022 0 \x2022",
    L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022[&6]\x2022 7 \x2022 8 \x2022 9 \x2022 0 \x2022",
    L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022[&7]\x2022 8 \x2022 9 \x2022 0 \x2022",
    L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022[&8]\x2022 9 \x2022 0 \x2022",
    L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 8 \x2022[&9]\x2022 0 \x2022",
    L"\x2022 1 \x2022 2 \x2022 3 \x2022 4 \x2022 5 \x2022 6 \x2022 7 \x2022 8 \x2022 9 \x2022[&0]\x2022"
  };
  if (targetMemo < 0 || targetMemo >= MEMO_COUNT) {
    targetMemo = 0;
  }
  return indicators[targetMemo];
}

// Update indicator text to show current page
static void ShowXInIndicator(HANDLE hDlg, int targetMemo) {
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_INDICATOR, (LONG_PTR)GetIndicatorWithX(targetMemo));
}

// Get text from memo editor via dialog messages
static std::wstring GetMemoText(HANDLE hDlg) {
  size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
  std::wstring content;
  if (len > 0) {
    content.resize(len);
    g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO, (LONG_PTR)content.data());
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
    return;
  }

  SaveCurrentMemo(hDlg);  // Auto-save before switching

  g_currentMemo = newMemo;
  std::wstring newContent = LoadFileContent(GetMemoFilePath(g_currentMemo));
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_MEMO, (LONG_PTR)newContent.c_str());

  // Update title: "Memo" -> "Memo - 1", etc.
  std::wstring title = L"[ Memo - " + std::to_wstring(g_currentMemo + 1) + L" ]";
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_TITLE, (LONG_PTR)title.c_str());

  ShowXInIndicator(hDlg, newMemo);
}

// Save current memo to external file (F2/Shift+F2)
static bool SaveMemoAs(HANDLE hDlg) {
  std::wstring content = GetMemoText(hDlg);

  // Default: memo-01.txt ... memo-10.txt in home directory
  int memoNum = (g_currentMemo + 1) % MEMO_COUNT;
  std::wstring defaultName = L"memo-";
  if (memoNum < MEMO_COUNT) defaultName += L"0";
  defaultName += std::to_wstring(memoNum) + L".txt";

  std::wstring homePath = GetHomeDir();
  std::wstring defaultPath = homePath + L"/" + defaultName;

  wchar_t destPath[MAX_PATH];
  wcsncpy(destPath, defaultPath.c_str(), MAX_PATH - 1);
  destPath[MAX_PATH - 1] = 0;

  if (g_far.InputBox(L"Save Memo", L"Enter destination path:", L"MemoSave",
                     defaultPath.c_str(), destPath, MAX_PATH, NULL, FIB_NONE)) {
    return SaveFileContent(destPath, content);
  }

  return false;
}

// Dialog procedure - handles keyboard and close events
// DN_KEY: intercepts keys for memo switching
// DN_CLOSE: saves content and state
static LONG_PTR WINAPI MemoDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
  switch (Msg) {
  case DN_KEY:
    if (Param1 == DI_MEMO) {
      int key = (int)Param2;

      // ESC closes dialog - DN_CLOSE will save
      if (key == KEY_ESC) {
        g_far.SendDlgMessage(hDlg, DM_CLOSE, 0, 0);
        return TRUE;
      }

      // F2/Shift+F2: Save As
      if (key == KEY_F2 || key == KEY_SHIFTF2) {
        SaveMemoAs(hDlg);
        return TRUE;
      }

      // Ctrl+0-9 or Alt+0-9: switch memo
      int memoIdx = -1;

      if (key == KEY_CTRL0 || key == KEY_ALT0)
        memoIdx = MEMO_COUNT - 1;  // 0 -> 9
      else if (key >= KEY_CTRL1 && key <= KEY_CTRL9)
        memoIdx = key - KEY_CTRL1;  // 1-9
      else if (key >= KEY_ALT1 && key <= KEY_ALT9)
        memoIdx = key - KEY_ALT1;

      if (memoIdx >= 0 && memoIdx < MEMO_COUNT) {
        SwitchToMemo(hDlg, memoIdx);
        return TRUE;
      }
    }
    break;

  case DN_CLOSE:
    SaveCurrentMemo(hDlg);
    SaveLastMemoIndex(g_currentMemo);
    break;
  }

  return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

// Create and run the memo dialog
static void OpenMemoDialog() {
  g_currentMemo = LoadLastMemoIndex();

  // Get console size for dialog dimensions
  SMALL_RECT screenRect;
  int screenWidth = 80;
  int screenHeight = 25;
  if (g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &screenRect, NULL)) {
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

  FarDialogItem items[3] = {};

  // Title at top
  items[DI_TITLE].Type = DI_TEXT;
  items[DI_TITLE].X1 = 1;
  items[DI_TITLE].Y1 = 0;
  items[DI_TITLE].X2 = dlgWidth - 2;
  items[DI_TITLE].Flags = DIF_CENTERTEXT;
  items[DI_TITLE].PtrData = L"Memo";

  // Main memo editor - multiline
  items[DI_MEMO].Type = DI_MEMOEDIT;
  items[DI_MEMO].X1 = 1;
  items[DI_MEMO].Y1 = 1;
  items[DI_MEMO].X2 = dlgWidth - 4;
  items[DI_MEMO].Y2 = dlgHeight - 4;
  items[DI_MEMO].Focus = 1;
  items[DI_MEMO].Flags = DIF_EDITOR | DIF_SHOWAMPERSAND;
  items[DI_MEMO].PtrData = content.c_str();

  // Page indicator at bottom
  items[DI_INDICATOR].Type = DI_TEXT;
  items[DI_INDICATOR].X1 = 1;
  items[DI_INDICATOR].Y1 = dlgHeight - 3;
  items[DI_INDICATOR].X2 = dlgWidth - 2;
  items[DI_INDICATOR].Flags = DIF_CENTERTEXT;
  items[DI_INDICATOR].PtrData = GetIndicatorWithX(g_currentMemo);

  HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, x1, y1, x2, y2, NULL, items, 3, 0, 0, MemoDlgProc, 0);

  if (hDlg != INVALID_HANDLE_VALUE) {
    g_far.DialogRun(hDlg);
    g_far.DialogFree(hDlg);
  }
}

// ========== Far2l Plugin API ==========

// Called once at plugin load - save API pointer
SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
  g_far = *Info;
  DBG("SetStartupInfoW: called");
}

// Return plugin info - menu items, command prefix, etc.
SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  Info->StructSize = sizeof(PluginInfo);
  // Don't use PF_EDITOR - memo uses DI_MEMOEDIT (internal dialog editor),
  // not a real file editor. PF_EDITOR causes colorer to send editor events
  // which crash when there's no valid file context.
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
  OpenMemoDialog();
  return INVALID_HANDLE_VALUE;
}

// Not used - no cleanup needed
SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin) {}
