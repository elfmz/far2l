#include <KeyFileHelper.h>
#include <WideMB.h>
#include <WinCompat.h>
#include <WinPort.h>
#include <cwchar>
#include <farkeys.h>
#include <farplug-wide.h>
#include <string>
#include <utils.h>
#include <vector>

#ifndef WINAPI
#define WINAPI
#endif

#define SYSID_MEMO 0x4D454D4F
#define MEMO_COUNT 10

enum MsgId {
  MMemo = 0,
  MOk,
  MCancel,
  MConfigTitle,
  MEnablePlugin,
  MUseHotkey,
  MSaveAs,
  MConfig
};
enum DialogItems {
  DI_BOX = 0,
  DI_MEMO,
  DI_F2,
  DI_BTN1,
  DI_BTN2,
  DI_BTN3,
  DI_BTN4,
  DI_BTN5,
  DI_BTN6,
  DI_BTN7,
  DI_BTN8,
  DI_BTN9,
  DI_BTN0,
  DI_F9
};

PluginStartupInfo g_far;
static int g_currentMemo = 0;
static std::wstring g_loadedContent;

static std::wstring GetMemoFilePath(int index) {
  const char *home = getenv("HOME");
  std::wstring dir =
      home ? MB2Wide(home) + L"/.config/far2l/plugins/memo" : L"memos";
  if (home) {
    WINPORT(CreateDirectory)((MB2Wide(home) + L"/.config").c_str(), NULL);
    WINPORT(CreateDirectory)((MB2Wide(home) + L"/.config/far2l").c_str(), NULL);
    WINPORT(CreateDirectory)(
        (MB2Wide(home) + L"/.config/far2l/plugins").c_str(), NULL);
    WINPORT(CreateDirectory)(dir.c_str(), NULL);
  }
  wchar_t name[32];
  swprintf(name, 32, L"/memo-%02d.txt", index + 1);
  return dir + name;
}

static int GetLastMemo() {
  return KeyFileHelper(InMyConfig("plugins/memo/state.ini"))
      .GetInt("Settings", "LastMemo", 0);
}
static void SaveLastMemo(int v) {
  KeyFileHelper kf(InMyConfig("plugins/memo/state.ini"));
  kf.SetInt("Settings", "LastMemo", v);
  kf.Save();
}

static std::wstring LoadFile(int idx) {
  std::string mbP = Wide2MB(GetMemoFilePath(idx).c_str());
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

static void SaveFile(int idx, const std::wstring &content) {
  std::string mbP = Wide2MB(GetMemoFilePath(idx).c_str());
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

static void SwitchToMemo(HANDLE hDlg, int newMemo) {
  if (newMemo < 0 || newMemo >= MEMO_COUNT || newMemo == g_currentMemo)
    return;
  size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
  std::wstring content;
  if (len > 0) {
    content.resize(len);
    g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO,
                         (LONG_PTR)content.data());
  }
  SaveFile(g_currentMemo, content);
  g_currentMemo = newMemo;
  g_loadedContent = LoadFile(g_currentMemo);
  g_far.SendDlgMessage(hDlg, DM_SETTEXTPTR, DI_MEMO,
                       (LONG_PTR)g_loadedContent.c_str());
  wchar_t title[64];
  swprintf(title, 64, L" Memo - %d", g_currentMemo + 1);
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

static void SaveAs(HANDLE hDlg) {
  size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
  std::wstring content;
  if (len > 0) {
    content.resize(len);
    g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO,
                         (LONG_PTR)content.data());
  }
  wchar_t path[MAX_PATH] = L"memo.txt";
  if (g_far.InputBox(L"Save Memo", L"Path:", L"MemoSave", path, path, MAX_PATH,
                     NULL, FIB_NONE)) {
    FILE *f = fopen(Wide2MB(path).c_str(), "w");
    if (f) {
      fputs(Wide2MB(content.c_str()).c_str(), f);
      fclose(f);
    }
  }
}

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber);

static LONG_PTR WINAPI MemoDlgProc(HANDLE hDlg, int Msg, int Param1,
                                   LONG_PTR Param2) {
  if (Msg == DN_INITDIALOG) {
    SMALL_RECT r;
    COORD sz = {75, 20};
    g_far.SendDlgMessage(hDlg, DM_RESIZEDIALOG, 0, (LONG_PTR)&sz);
    r = {0, 0, 74, 19};
    g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_BOX, (LONG_PTR)&r);
    r = {1, 1, 73, 18};
    g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_MEMO, (LONG_PTR)&r);

    // F2 Save: 11 chars [F2 SaveAs]
    r = {1, 19, 9, 19};
    g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_F2, (LONG_PTR)&r);

    // F9 Config: 11 chars [F9 Config]
    r = {63, 19, 73, 19};
    g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_F9, (LONG_PTR)&r);

    // Digits: 10 * 3 = 30 chars. Center them in the space (10 to 62: 52 chars).
    int totalDigitsWidth = 10 * 3;
    int startDigitsX = 10 + (53 - totalDigitsWidth) / 2;
    int curX = startDigitsX;
    for (int i = 0; i < 10; i++) {
      r = {(SHORT)curX, 19, (SHORT)(curX + 2), 19};
      g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, DI_BTN1 + i, (LONG_PTR)&r);
      curX += 3;
    }
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
  }
  if (Msg == DN_CLOSE) {
    size_t len = g_far.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, DI_MEMO, 0);
    std::wstring content;
    if (len > 0) {
      content.resize(len);
      g_far.SendDlgMessage(hDlg, DM_GETTEXTPTR, DI_MEMO,
                           (LONG_PTR)content.data());
    }
    SaveFile(g_currentMemo, content);
    SaveLastMemo(g_currentMemo);
  }
  return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

static void OpenMemo() {
  g_currentMemo = GetLastMemo();
  g_loadedContent = LoadFile(g_currentMemo);
  wchar_t title[64];
  swprintf(title, 64, L" Memo - %d", g_currentMemo + 1);
  FarDialogItem it[14] = {};
  it[DI_BOX].Type = DI_TEXT;
  it[DI_BOX].PtrData = title;
  it[DI_MEMO].Type = DI_MEMOEDIT;
  it[DI_MEMO].Focus = 1;
  it[DI_MEMO].PtrData = g_loadedContent.c_str();
  it[DI_F2].Type = DI_BUTTON;
  it[DI_F2].PtrData = L"[F2 Save]";
  it[DI_F2].Flags = DIF_BTNNOCLOSE | DIF_NOBRACKETS;
  it[DI_F9].Type = DI_BUTTON;
  it[DI_F9].PtrData = L"[F9 Config]";
  it[DI_F9].Flags = DIF_BTNNOCLOSE | DIF_NOBRACKETS;
  static wchar_t btxt[10][4] = {L"[1]", L"[2]", L"[3]", L"[4]", L"[5]",
                                L"[6]", L"[7]", L"[8]", L"[9]", L"[0]"};
  for (int i = 0; i < 10; i++) {
    it[DI_BTN1 + i].Type = DI_BUTTON;
    it[DI_BTN1 + i].PtrData = btxt[i];
    it[DI_BTN1 + i].Flags = DIF_BTNNOCLOSE | DIF_NOBRACKETS;
    it[DI_BTN1 + i].DefaultButton = false;
  }
  HANDLE h = g_far.DialogInit(g_far.ModuleNumber, -1, -1, 75, 20, NULL, it, 14,
                              0, 0, MemoDlgProc, 0);
  if (h != (HANDLE)-1) {
    g_far.DialogRun(h);
    g_far.DialogFree(h);
  }
}

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
  g_far = *Info;
}
SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  Info->StructSize = sizeof(PluginInfo);
  Info->Flags = PF_VIEWER | PF_DIALOG;
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
SHAREDSYMBOL int WINAPI ConfigureW(int) {
  FarDialogItem it[3] = {};
  it[0].Type = DI_DOUBLEBOX;
  it[0].X1 = 3;
  it[0].Y1 = 1;
  it[0].X2 = 35;
  it[0].Y2 = 4;
  it[0].PtrData = L"Config";
  it[1].Type = DI_BUTTON;
  it[1].Y1 = 3;
  it[1].Flags = DIF_CENTERGROUP;
  it[1].DefaultButton = 1;
  it[1].PtrData = L"OK";
  it[2].Type = DI_BUTTON;
  it[2].Y1 = 3;
  it[2].Flags = DIF_CENTERGROUP;
  it[2].PtrData = L"Cancel";
  HANDLE d = g_far.DialogInit(g_far.ModuleNumber, -1, -1, 38, 6, NULL, it, 3, 0,
                              0, NULL, 0);
  if (d != (HANDLE)-1) {
    g_far.DialogRun(d);
    g_far.DialogFree(d);
  }
  return 1;
}
