#include "FarEditorSet.h"
#include <KeyFileHelper.h>
#include <sys/stat.h>
#include <utils.h>
#include "FarHrcSettings.h"

const std::string cSectionName = "Settings";
// registry keys
const char cRegEnabled[] = "Enabled";
const char cRegHrdName[] = "HrdName";
const char cRegHrdNameTm[] = "HrdNameTm";
const char cRegCatalog[] = "Catalog";
const char cRegCrossDraw[] = "CrossDraw";
const char cRegPairsDraw[] = "PairsDraw";
const char cRegSyntaxDraw[] = "SyntaxDraw";
const char cRegOldOutLine[] = "OldOutlineView";
const char cRegTrueMod[] = "TrueMod";
const char cRegChangeBgEditor[] = "ChangeBgEditor";
const char cRegUserHrdPath[] = "UserHrdPath";
const char cRegUserHrcPath[] = "UserHrcPath";

// values of registry keys by default
const bool cEnabledDefault = true;
const wchar_t cHrdNameDefault[] = L"default";
const wchar_t cHrdNameTmDefault[] = L"default";
const wchar_t cCatalogDefault[] = L"";
const int cCrossDrawDefault = 2;
const bool cPairsDrawDefault = true;
const bool cSyntaxDrawDefault = true;
const bool cOldOutLineDefault = true;
const bool cTrueMod = true;
const bool cChangeBgEditor = false;
const wchar_t cUserHrdPathDefault[] = L"";
const wchar_t cUserHrcPathDefault[] = L"";

const UnicodeString DConsole("console");
const UnicodeString DRgb("rgb");
const UnicodeString Ddefault("<default>");
const UnicodeString DAutodetect("autodetect");

int _snwprintf_s(wchar_t* string, size_t sizeInWords, size_t count, const wchar_t* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  int result = vswprintf(string, count, format, arglist);
  va_end(arglist);
  return result;
}

FarEditorSet::FarEditorSet()
    : sTempHrdName(nullptr),
      sTempHrdNameTm(nullptr),
      dialogFirstFocus(false),
      menuid(-1),
      parserFactory(nullptr),
      regionMapper(nullptr),
      hrdClass(""),
      hrdName(""),
      rEnabled(false),
      drawCross(false),
      drawPairs(false),
      drawSyntax(false),
      oldOutline(false),
      TrueModOn(false),
      ChangeBgEditor(false),
      sHrdName(nullptr),
      sHrdNameTm(nullptr),
      sCatalogPath(nullptr),
      sUserHrdPath(nullptr),
      sUserHrcPath(nullptr),
      viewFirst(0)
{
  settingsIni = InMyConfig("plugins/colorer/config.ini");
  struct stat s;
  if (stat(settingsIni.c_str(), &s) == -1) {
    SetDefaultSettings();
  }

  ReloadBase();
}

FarEditorSet::~FarEditorSet()
{
  dropAllEditors(false);
  delete sHrdName;
  delete sHrdNameTm;
  delete sCatalogPath;
  delete sUserHrdPath;
  delete sUserHrcPath;
}

void FarEditorSet::openMenu()
{
  int iMenuItems[] = {mListTypes,  mMatchPair,    mSelectBlock,    mSelectPair, mListFunctions,
                      mFindErrors, mSelectRegion, mLocateFunction, -1,          mUpdateHighlight,
                      mReloadBase, mConfigure};
  FarMenuItem menuElements[sizeof(iMenuItems) / sizeof(iMenuItems[0])];
  memset(menuElements, 0, sizeof(menuElements));

  try {
    if (!rEnabled) {
      menuElements[0].Text = GetMsg(mConfigure);
      menuElements[0].Selected = 1;

      if (Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), nullptr, L"menu",
                    nullptr, nullptr, menuElements, 1) == 0)
      {
        ReadSettings();
        configure(true);
      }

      return;
    };

    for (int i = sizeof(iMenuItems) / sizeof(iMenuItems[0]) - 1; i >= 0; i--) {
      if (iMenuItems[i] == -1) {
        menuElements[i].Separator = 1;
      }
      else {
        menuElements[i].Text = GetMsg(iMenuItems[i]);
      }
    };

    menuElements[0].Selected = 1;

    FarEditor* editor = getCurrentEditor();
    if (!editor) {
      throw Exception("Can't find current editor in array.");
    }
    int res =
        Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), nullptr, L"menu",
                  nullptr, nullptr, menuElements, sizeof(iMenuItems) / sizeof(iMenuItems[0]));
    switch (res) {
      case 0:
        chooseType();
        break;
      case 1:
        editor->matchPair();
        break;
      case 2:
        editor->selectBlock();
        break;
      case 3:
        editor->selectPair();
        break;
      case 4:
        editor->listFunctions();
        break;
      case 5:
        editor->listErrors();
        break;
      case 6:
        editor->selectRegion();
        break;
      case 7:
        editor->locateFunction();
        break;
      case 9:
        editor->updateHighlighting();
        break;
      case 10:
        ReloadBase();
        break;
      case 11:
        configure(true);
        break;
    };
  } catch (Exception& e) {
    COLORER_LOG_ERROR("%", e.what());
    UnicodeString msg("openMenu: ");
    msg.append(e.what());
    showExceptionMessage(&msg);
    disableColorer();
  };
}

int FarEditorSet::getCountFileTypeAndGroup()
{
  int num = 0;
  UnicodeString group;
  FileType* type = nullptr;
  auto& hrcParser = parserFactory->getHrcLibrary();

  for (int idx = 0;; idx++, num++) {
    type = hrcParser.enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (group.compare(type->getGroup()) != 0) {
      num++;
    }

    group = type->getGroup();
  };
  return num;
}

FileType* FarEditorSet::getFileTypeByIndex(int idx)
{
  FileType* type = nullptr;
  UnicodeString group;
  auto& hrcParser = parserFactory->getHrcLibrary();
  for (int i = 0; idx >= 0; idx--, i++) {
    type = hrcParser.enumerateFileTypes(i);

    if (!type) {
      break;
    }

    if (group.compare(type->getGroup()) != 0) {
      idx--;
    }
    group = type->getGroup();
  };

  return (FileType*) type;
}

void FarEditorSet::FillTypeMenu(ChooseTypeMenu* Menu, FileType* CurFileType)
{
  UnicodeString group = DAutodetect;
  FileType* type = nullptr;
  auto& hrcParser = parserFactory->getHrcLibrary();

  if (!CurFileType) {
    Menu->SetSelected(1);
  }

  for (int idx = 0;; idx++) {
    type = hrcParser.enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (group.compare(type->getGroup()) != 0) {
      Menu->AddGroup(UStr::to_stdwstr(type->getGroup()).c_str());
      group = type->getGroup();
    };

    int i;
    const UnicodeString* v;
    v = ((FileType*) type)->getParamValue(DFavorite);
    if (v && v->equals(&DTrue))
      i = Menu->AddFavorite(type);
    else
      i = Menu->AddItem(type);
    if (type == CurFileType) {
      Menu->SetSelected(i);
    }
  };
}

inline wchar_t __cdecl Upper(wchar_t Ch)
{
  WINPORT(CharUpperBuff)(&Ch, 1);
  return Ch;
}

LONG_PTR WINAPI KeyDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
  int key = Param2;

  if (Msg == DN_KEY && key != KEY_F1) {
    if (key == KEY_ESC || key == KEY_ENTER || key == KEY_NUMENTER) {
      return FALSE;
    }

    if (key > 31) {
      wchar_t wkey[2];

      if (key > 128) {
        FSF.FarKeyToName(key, wkey, 2);
        wchar_t* c = FSF.XLat(wkey, 0, 1, 0);
        key = FSF.FarNameToKey(c);
      }

      if (key < 0xFFFF)
        key = Upper((wchar_t) (key & 0x0000FFFF)) | (key & (~0x0000FFFF));

      if ((key >= 48 && key <= 57) || (key >= 65 && key <= 90)) {
        FSF.FarKeyToName(key, wkey, 2);
        Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, 2, (LONG_PTR) wkey);
      }
      return TRUE;
    }
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

void FarEditorSet::chooseType()
{
  FarEditor* fe = getCurrentEditor();
  if (!fe) {
    return;
  }

  ChooseTypeMenu menu(GetMsg(mAutoDetect), GetMsg(mFavorites));
  FillTypeMenu(&menu, fe->getFileType());

  wchar_t bottom[20];
  swprintf(bottom, 20, GetMsg(mTotalTypes), parserFactory->getHrcLibrary().getFileTypesCount());
  int BreakKeys[4] = {VK_INSERT, VK_DELETE, VK_F4, 0};
  int BreakCode, i;
  while (true) {
    i = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT | FMENU_USEEXT,
                  GetMsg(mSelectSyntax), bottom, L"filetypechoose", BreakKeys, &BreakCode,
                  (const struct FarMenuItem*) menu.getItems(), menu.getItemsCount());

    if (i >= 0) {
      if (BreakCode == 0) {
        if (i != 0 && !menu.IsFavorite(i)) {
          auto f = menu.GetFileType(i);
          menu.MoveToFavorites(i);
          addParamAndValue(f, DFavorite, DTrue);
        }
        else {
          menu.SetSelected(i);
        }
      }
      else if (BreakCode == 1) {
        if (i != 0 && menu.IsFavorite(i))
          menu.DelFromFavorites(i);
        else
          menu.SetSelected(i);
      }
      else if (BreakCode == 2) {
        if (i == 0) {
          menu.SetSelected(i);
          continue;
        }

        const int KeyAssignDlgDataCount = 3;
        FarDialogItem KeyAssignDlgData[KeyAssignDlgDataCount] = {
            {DI_DOUBLEBOX,  3, 1, 30, 4, 0, {}, 0, 0, GetMsg(mKeyAssignDialogTitle), 0},
            {     DI_TEXT, -1, 2,  0, 2, 0, {}, 0, 0,   GetMsg(mKeyAssignTextTitle), 0},
            {     DI_EDIT,  5, 3, 28, 3, 0, {}, 0, 0,                           L"", 0},
        };

        const UnicodeString* v;
        v = ((FileType*) menu.GetFileType(i))->getParamValue(DHotkey);
        if (v && v->length()) {
          KeyAssignDlgData[2].PtrData = v->getWChars();
        }

        HANDLE hDlg =
            Info.DialogInit(Info.ModuleNumber, -1, -1, 34, 6, L"keyassign", KeyAssignDlgData,
                            KeyAssignDlgDataCount, 0, 0, KeyDialogProc, (LONG_PTR) this);
        int res = Info.DialogRun(hDlg);

        if (res != -1) {
          KeyAssignDlgData[2].PtrData =
              trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, 2, 0));
          auto ftype = menu.GetFileType(i);
          auto param_name = UnicodeString(DHotkey);
          auto hotkey = UnicodeString(KeyAssignDlgData[2].PtrData);
          addParamAndValue(ftype, param_name, hotkey);
          menu.RefreshItemCaption(i);
        }
        menu.SetSelected(i);
        Info.DialogFree(hDlg);
      }
      else {
        if (i == 0) {
          UnicodeString* s = getCurrentFileName();
          fe->chooseFileType(s);
          delete s;
          break;
        }
        fe->setFileType(menu.GetFileType(i));
        break;
      }
    }
    else
      break;
  }

  FarHrcSettings p(this, parserFactory.get());
  p.writeUserProfile();
}

const UnicodeString* FarEditorSet::getHRDescription(const UnicodeString& name,
                                                    UnicodeString _hrdClass)
{
  const UnicodeString* descr = nullptr;
  if (parserFactory) {
    const HrdNode& node = parserFactory->getHrdNode(_hrdClass, name);
    descr = &node.hrd_description;
  }

  if (descr == nullptr) {
    descr = &name;
  }

  return descr;
}

LONG_PTR WINAPI SettingDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
  FarEditorSet* fes = (FarEditorSet*) Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
  ;

  switch (Msg) {
    case DN_BTNCLICK:
      switch (Param1) {
        case IDX_HRD_SELECT: {
          UnicodeString* tempSS =
              new UnicodeString(*fes->chooseHRDName(fes->sTempHrdName, DConsole));
          delete fes->sTempHrdName;
          fes->sTempHrdName = tempSS;

          const UnicodeString* descr = fes->getHRDescription(*fes->sTempHrdName, DConsole);
          Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_HRD_SELECT, (LONG_PTR) descr->getWChars());
          return true;
        }
        case IDX_HRD_SELECT_TM: {
          UnicodeString* tempSS = new UnicodeString(*fes->chooseHRDName(fes->sTempHrdNameTm, DRgb));
          delete fes->sTempHrdNameTm;
          fes->sTempHrdNameTm = tempSS;

          const UnicodeString* descr = fes->getHRDescription(*fes->sTempHrdNameTm, DRgb);
          Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_HRD_SELECT_TM,
                              (LONG_PTR) descr->getWChars());
          return true;
        }
        case IDX_RELOAD_ALL: {
          Info.SendDlgMessage(hDlg, DM_SHOWDIALOG, false, 0);
          wchar_t* catalog =
              trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CATALOG_EDIT, 0));
          wchar_t* userhrd =
              trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRD_EDIT, 0));
          wchar_t* userhrc =
              trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRC_EDIT, 0));
          bool trumod = Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_TRUEMOD, 0) &&
              fes->checkConsoleAnnotationAvailable();
          fes->TestLoadBase(catalog, userhrd, userhrc, true,
                            trumod ? FarEditorSet::HRCM_BOTH : FarEditorSet::HRCM_CONSOLE);
          Info.SendDlgMessage(hDlg, DM_SHOWDIALOG, true, 0);
          return true;
        }
        case IDX_HRC_SETTING: {
          fes->configureHrc();
          return true;
        }
        case IDX_OK:
          const wchar_t* temp =
              trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CATALOG_EDIT, 0));
          const wchar_t* userhrd =
              trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRD_EDIT, 0));
          const wchar_t* userhrc =
              trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRC_EDIT, 0));
          bool trumod = Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_TRUEMOD, 0) &&
              fes->checkConsoleAnnotationAvailable();
          int k = (int) Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_ENABLED, 0);

          if (fes->GetCatalogPath()->compare(UnicodeString(temp)) ||
              fes->GetUserHrdPath()->compare(UnicodeString(userhrd)) ||
              (!fes->GetPluginStatus() && k) || (trumod == true))
          {
            if (fes->TestLoadBase(temp, userhrd, userhrc, false,
                                  trumod ? FarEditorSet::HRCM_BOTH : FarEditorSet::HRCM_CONSOLE))
            {
              return false;
            }
            else {
              return true;
            }
          }

          return false;
      }
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

void FarEditorSet::configure(bool fromEditor)
{
  try {
    const int fdiCount = 25;
    FarDialogItem fdi[fdiCount] = {
        {DI_DOUBLEBOX,3,  1,55, 22,0,                       {},0,    0,L"", 0                                                                                         }, //  IDX_BOX,
        { DI_CHECKBOX,  5,  2,  0,  0,  TRUE,                       {},          0,    0, L"", 0}, //  IDX_DISABLED,
        { DI_CHECKBOX,  5,  3,  0,  0, FALSE,                       {}, DIF_3STATE,    0, L"", 0}, //  IDX_CROSS,
        { DI_CHECKBOX,  5,  4,  0,  0, FALSE,                       {},          0,    0, L"", 0}, //  IDX_PAIRS,
        { DI_CHECKBOX,  5,  5,  0,  0, FALSE,                       {},          0,    0, L"", 0}, //  IDX_SYNTAX,
        { DI_CHECKBOX,  5,  6,  0,  0, FALSE,                       {},          0,    0, L"", 0}, //  IDX_OLDOUTLINE,
        { DI_CHECKBOX,  5,  7,  0,  0,  TRUE,                       {},          0,    0, L"", 0}, //  IDX_CHANGE_BG,
        {     DI_TEXT,  5,  8,  0,  8, FALSE,                       {},          0,    0, L"", 0}, //  IDX_HRD,
        {   DI_BUTTON, 20,  8,  0,  0, FALSE,                       {},          0,    0, L"", 0}, //  IDX_HRD_SELECT,
        {     DI_TEXT,  5,  9,  0,  9, FALSE,                       {},          0,    0, L"", 0}, //  IDX_CATALOG,
        {     DI_EDIT,
         6, 10,
         52,  5,
         FALSE, {(DWORD_PTR) L"catalog"},
         DIF_HISTORY,    0,
         L"", 0                                                                                 }, //  IDX_CATALOG_EDIT
        {     DI_TEXT,  5, 11,  0, 11, FALSE,                       {},          0,    0, L"", 0}, //  IDX_USERHRC,
        {     DI_EDIT,
         6, 12,
         52,  5,
         FALSE, {(DWORD_PTR) L"userhrc"},
         DIF_HISTORY,    0,
         L"", 0                                                                                 }, //  IDX_USERHRC_EDIT
        {     DI_TEXT,  5, 13,  0, 13, FALSE,                       {},          0,    0, L"", 0}, //  IDX_USERHRD,
        {     DI_EDIT,
         6, 14,
         52,  5,
         FALSE, {(DWORD_PTR) L"userhrd"},
         DIF_HISTORY,    0,
         L"", 0                                                                                 }, //  IDX_USERHRD_EDIT
        {DI_SINGLEBOX,  4, 16, 54, 16,  TRUE,                       {},          0,    0, L"", 0}, //  IDX_TM_BOX,
        { DI_CHECKBOX,  5, 17,  0,  0,  TRUE,                       {},          0,    0, L"", 0}, //  IDX_TRUEMOD,
        {     DI_TEXT, 20, 17,  0, 17,  TRUE,                       {},          0,    0, L"", 0}, //  IDX_TMMESSAGE,
        {     DI_TEXT,  5, 18,  0, 18, FALSE,                       {},          0,    0, L"", 0}, //  IDX_HRD_TM,
        {   DI_BUTTON, 20, 18,  0,  0, FALSE,                       {},          0,    0, L"", 0}, //  IDX_HRD_SELECT_TM,
        {DI_SINGLEBOX,  4, 19, 54, 19,  TRUE,                       {},          0,    0, L"", 0}, //  IDX_TM_BOX_OFF,
        {   DI_BUTTON,  5, 20,  0,  0, FALSE,                       {},          0,    0, L"", 0}, //  IDX_RELOAD_ALL,
        {   DI_BUTTON, 30, 20,  0,  0, FALSE,                       {},          0,    0, L"", 0}, //  IDX_HRC_SETTING,
        {   DI_BUTTON, 35, 21,  0,  0, FALSE,                       {},          0, TRUE, L"", 0}, //  IDX_OK,
        {   DI_BUTTON, 45, 21,  0,  0, FALSE,                       {},          0,    0, L"", 0}  //  IDX_CANCEL,
    };  // type, x1, y1, x2, y2, focus, sel, fl, def, data, maxlen

    fdi[IDX_BOX].PtrData = GetMsg(mSetup);
    fdi[IDX_ENABLED].PtrData = GetMsg(mTurnOff);
    fdi[IDX_ENABLED].Param.Selected = rEnabled;
    fdi[IDX_TRUEMOD].PtrData = GetMsg(mTrueMod);
    fdi[IDX_TRUEMOD].Param.Selected = TrueModOn;
    fdi[IDX_CROSS].PtrData = GetMsg(mCross);
    fdi[IDX_CROSS].Param.Selected = drawCross;
    fdi[IDX_PAIRS].PtrData = GetMsg(mPairs);
    fdi[IDX_PAIRS].Param.Selected = drawPairs;
    fdi[IDX_SYNTAX].PtrData = GetMsg(mSyntax);
    fdi[IDX_SYNTAX].Param.Selected = drawSyntax;
    fdi[IDX_OLDOUTLINE].PtrData = GetMsg(mOldOutline);
    fdi[IDX_OLDOUTLINE].Param.Selected = oldOutline;
    fdi[IDX_CATALOG].PtrData = GetMsg(mCatalogFile);
    fdi[IDX_CATALOG_EDIT].PtrData = (wchar_t*) sCatalogPath->getWChars();
    fdi[IDX_USERHRC].PtrData = GetMsg(mUserHrcFile);
    fdi[IDX_USERHRC_EDIT].PtrData = (wchar_t*) sUserHrcPath->getWChars();
    fdi[IDX_USERHRD].PtrData = GetMsg(mUserHrdFile);
    fdi[IDX_USERHRD_EDIT].PtrData = (wchar_t*) sUserHrdPath->getWChars();
    fdi[IDX_HRD].PtrData = GetMsg(mHRDName);

    const UnicodeString* descr = nullptr;
    sTempHrdName = new UnicodeString(*sHrdName);

    descr = getHRDescription(*sTempHrdName, DConsole);
    fdi[IDX_HRD_SELECT].PtrData = (wchar_t*) descr->getWChars();

    const UnicodeString* descr2 = nullptr;
    sTempHrdNameTm = new UnicodeString(*sHrdNameTm);
    descr2 = getHRDescription(*sTempHrdNameTm, DRgb);

    fdi[IDX_HRD_TM].PtrData = GetMsg(mHRDNameTrueMod);
    fdi[IDX_HRD_SELECT_TM].PtrData = (wchar_t*) descr2->getWChars();
    fdi[IDX_CHANGE_BG].PtrData = GetMsg(mChangeBackgroundEditor);
    fdi[IDX_CHANGE_BG].Param.Selected = ChangeBgEditor;
    fdi[IDX_RELOAD_ALL].PtrData = GetMsg(mReloadAll);
    fdi[IDX_HRC_SETTING].PtrData = GetMsg(mUserHrcSetting);
    fdi[IDX_OK].PtrData = GetMsg(mOk);
    fdi[IDX_CANCEL].PtrData = GetMsg(mCancel);
    fdi[IDX_TM_BOX].PtrData = GetMsg(mTrueModSetting);

    if (!checkConsoleAnnotationAvailable() && fromEditor) {
      fdi[IDX_HRD_SELECT_TM].Flags = DIF_DISABLE;
      fdi[IDX_TRUEMOD].Flags = DIF_DISABLE;
      fdi[IDX_TMMESSAGE].PtrData = GetMsg(mNoFarTM);
    }
    /*
     * Dialog activation
     */
    HANDLE hDlg = Info.DialogInit(Info.ModuleNumber, -1, -1, 58, 24, L"config", fdi, fdiCount, 0, 0,
                                  SettingDialogProc, (LONG_PTR) this);
    int i = Info.DialogRun(hDlg);

    if (i == IDX_OK) {
      fdi[IDX_CATALOG_EDIT].PtrData =
          trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CATALOG_EDIT, 0));
      fdi[IDX_USERHRD_EDIT].PtrData =
          trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRD_EDIT, 0));
      fdi[IDX_USERHRC_EDIT].PtrData =
          trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRC_EDIT, 0));
      // check whether or not to reload the base
      int k = false;

      if (sCatalogPath->compare(UnicodeString(fdi[IDX_CATALOG_EDIT].PtrData)) ||
          sUserHrdPath->compare(UnicodeString(fdi[IDX_USERHRD_EDIT].PtrData)) ||
          sUserHrcPath->compare(UnicodeString(fdi[IDX_USERHRC_EDIT].PtrData)) ||
          sHrdName->compare(*sTempHrdName) || sHrdNameTm->compare(*sTempHrdNameTm))
      {
        k = true;
      }

      fdi[IDX_ENABLED].Param.Selected =
          (int) Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_ENABLED, 0);
      drawCross = (int) Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_CROSS, 0);
      drawPairs = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_PAIRS, 0);
      drawSyntax = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_SYNTAX, 0);
      oldOutline = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_OLDOUTLINE, 0);
      ChangeBgEditor = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_CHANGE_BG, 0);
      fdi[IDX_TRUEMOD].Param.Selected = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_TRUEMOD, 0);
      delete sHrdName;
      delete sHrdNameTm;
      delete sCatalogPath;
      delete sUserHrdPath;
      delete sUserHrcPath;
      sHrdName = sTempHrdName;
      sHrdNameTm = sTempHrdNameTm;
      sCatalogPath = new UnicodeString(fdi[IDX_CATALOG_EDIT].PtrData);
      sUserHrdPath = new UnicodeString(fdi[IDX_USERHRD_EDIT].PtrData);
      sUserHrcPath = new UnicodeString(fdi[IDX_USERHRC_EDIT].PtrData);

      // if the plugin has been enable, and we will disable
      if (rEnabled && !fdi[IDX_ENABLED].Param.Selected) {
        rEnabled = false;
        TrueModOn = !!(fdi[IDX_TRUEMOD].Param.Selected);
        SaveSettings();
        disableColorer();
      }
      else {
        if ((!rEnabled && fdi[IDX_ENABLED].Param.Selected) || k) {
          rEnabled = true;
          TrueModOn = !!(fdi[IDX_TRUEMOD].Param.Selected);
          SaveSettings();
          enableColorer(fromEditor);
        }
        else {
          if (TrueModOn != !!fdi[IDX_TRUEMOD].Param.Selected) {
            TrueModOn = !!(fdi[IDX_TRUEMOD].Param.Selected);
            SaveSettings();
            ReloadBase();
          }
          else {
            SaveSettings();
            ApplySettingsToEditors();
            SetBgEditor();
          }
        }
      }
    }

    Info.DialogFree(hDlg);

  } catch (Exception& e) {
    COLORER_LOG_ERROR("%", e.what());

    UnicodeString msg("configure: ");
    msg.append(UnicodeString(e.what()));
    showExceptionMessage(&msg);
    disableColorer();
  }
}

const UnicodeString* FarEditorSet::chooseHRDName(const UnicodeString* current,
                                                 UnicodeString _hrdClass)
{
  if (parserFactory == nullptr) {
    return current;
  }

  std::vector<const HrdNode*> nodes = parserFactory->enumHrdInstances(_hrdClass);
  std::vector<const UnicodeString*> names;
  FarMenuItem* menuElements = new FarMenuItem[nodes.size()];
  memset(menuElements, 0, sizeof(FarMenuItem) * nodes.size());
  int i = 0;
  for (const auto node : nodes) {
    const UnicodeString* name = &node->hrd_name;
    const UnicodeString* descr = &node->hrd_description;

    names.push_back(name);
    if (!descr->length()) {
      descr = name;
    }
    // names.push_back(name);

    menuElements[i].Text = (wchar_t*) descr->getWChars();

    if (current->equals(name)) {
      menuElements[i].Selected = 1;
    }
    i++;
  };

  int result =
      Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT,
                GetMsg(mSelectHRD), 0, L"hrd", nullptr, nullptr, menuElements, nodes.size());
  delete[] menuElements;

  if (result == -1) {
    return current;
  }

  return &(parserFactory->getHrdNode(_hrdClass, *names[result]).hrd_name);
}

int FarEditorSet::editorInput(const INPUT_RECORD* ir)
{
  if (rEnabled) {
    FarEditor* editor = getCurrentEditor();
    if (editor) {
      return editor->editorInput(ir);
    }
  }
  return 0;
}

int FarEditorSet::editorEvent(int Event, void* Param)
{
  // check whether all the editors cleaned
  if (!rEnabled && farEditorInstances.size() && Event == EE_GOTFOCUS) {
    dropCurrentEditor(true);
    return 0;
  }

  if (!rEnabled) {
    return 0;
  }

  try {
    FarEditor* editor = nullptr;
    switch (Event) {
      case EE_REDRAW: {
        editor = getCurrentEditor();
        if (editor) {
          return editor->editorEvent(Event, Param);
        }
        else {
          return 0;
        }
      }
      case EE_GOTFOCUS: {
        if (!getCurrentEditor()) {
          editor = addCurrentEditor();
          return editor->editorEvent(EE_REDRAW, EEREDRAW_CHANGE);
        }
        return 0;
      }
      case EE_READ: {
        addCurrentEditor();
        return 0;
      }
      case EE_CLOSE: {
        auto it = farEditorInstances.find(*((int*) Param));
        if (it != farEditorInstances.end()) {
          delete it->second;
          farEditorInstances.erase(it);
        }
        return 0;
      }
    }
  } catch (Exception& e) {
    COLORER_LOG_ERROR("%", e.what());

    UnicodeString msg("editorEvent: ");
    msg.append(UnicodeString(e.what()));
    showExceptionMessage(&msg);
    disableColorer();
  }

  return 0;
}

bool FarEditorSet::TestLoadBase(const wchar_t* catalogPath, const wchar_t* userHrdPath,
                                const wchar_t* userHrcPath, const int full, const HRC_MODE hrc_mode)
{
  bool res = true;
  const wchar_t* marr[2] = {GetMsg(mName), GetMsg(mReloading)};
  HANDLE scr = Info.SaveScreen(0, 0, -1, -1);
  Info.Message(Info.ModuleNumber, 0, nullptr, &marr[0], 2, 0);

  std::unique_ptr<ParserFactory> parserFactoryLocal;
  std::unique_ptr<StyledHRDMapper> regionMapperLocal;

  UnicodeString* catalogPathS = PathToFullS(catalogPath, false);
  UnicodeString* userHrdPathS = PathToFullS(userHrdPath, false);
  UnicodeString* userHrcPathS = PathToFullS(userHrcPath, false);

  UnicodeString* tpath;
  if (!catalogPathS || !catalogPathS->length()) {
    tpath = GetConfigPath(UnicodeString(FarCatalogXml));
  }
  else {
    tpath = catalogPathS;
  }

  try {
    parserFactoryLocal = std::make_unique<ParserFactory>();
    parserFactoryLocal->loadCatalog(tpath);
    delete tpath;
    HrcLibrary& hrcLibraryLocal = parserFactoryLocal->getHrcLibrary();
    auto def_type = hrcLibraryLocal.getFileType("default");
    FarHrcSettings p(this, parserFactoryLocal.get());
    p.loadUserHrd(userHrdPathS);
    p.loadUserHrc(userHrcPathS);
    p.readProfile();
    p.readUserProfile(def_type);

    if (hrc_mode == HRCM_CONSOLE || hrc_mode == HRCM_BOTH) {
      try {
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DConsole, sTempHrdName);
      } catch (ParserFactoryException& e) {
        COLORER_LOG_ERROR("%", e.what());
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DConsole, nullptr);
      }
      regionMapperLocal = nullptr;
    }

    if (hrc_mode == HRCM_RGB || hrc_mode == HRCM_BOTH) {
      try {
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DRgb, sTempHrdNameTm);
      } catch (ParserFactoryException& e) {
        COLORER_LOG_ERROR("%", e.what());
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DRgb, nullptr);
      }
    }
    Info.RestoreScreen(scr);
    if (full) {
      for (int idx = 0;; idx++) {
        FileType* type = hrcLibraryLocal.enumerateFileTypes(idx);

        if (type == nullptr) {
          break;
        }

        UnicodeString tname;

        tname.append(type->getGroup());
        tname.append(": ");

        tname.append(type->getDescription());
        marr[1] = tname.getWChars();
        scr = Info.SaveScreen(0, 0, -1, -1);
        Info.Message(Info.ModuleNumber, 0, nullptr, &marr[0], 2, 0);
        type->getBaseScheme();
        Info.RestoreScreen(scr);
      }
    }
  } catch (Exception& e) {
    COLORER_LOG_ERROR("%", e.what());
    auto error_mes = UnicodeString(e.what());
    showExceptionMessage(&error_mes);
    Info.RestoreScreen(scr);
    res = false;
  }

  return res;
}

void FarEditorSet::ReloadBase()
{
  ReadSettings();
  if (!rEnabled) {
    return;
  }

  const wchar_t* marr[2] = {GetMsg(mName), GetMsg(mReloading)};
  HANDLE scr = Info.SaveScreen(0, 0, -1, -1);
  Info.Message(Info.ModuleNumber, 0, nullptr, &marr[0], 2, 0);

  dropAllEditors(true);
  regionMapper = nullptr;

  consoleAnnotationAvailable = checkConsoleAnnotationAvailable() && TrueModOn;
  if (consoleAnnotationAvailable) {
    hrdClass = DRgb;
    hrdName = *sHrdNameTm;
  }
  else {
    hrdClass = DConsole;
    hrdName = *sHrdName;
  }

  try {
    parserFactory = std::make_unique<ParserFactory>();
    parserFactory->loadCatalog(sCatalogPathExp.get());
    HrcLibrary& hrcLibrary = parserFactory->getHrcLibrary();
    defaultType = hrcLibrary.getFileType("default");
    FarHrcSettings p(this, parserFactory.get());
    p.loadUserHrd(sUserHrdPathExp.get());
    p.loadUserHrc(sUserHrcPathExp.get());
    p.readProfile();
    p.readUserProfile();

    try {
      regionMapper = parserFactory->createStyledMapper(&hrdClass, &hrdName);
    } catch (ParserFactoryException& e) {
      COLORER_LOG_ERROR("%", e.what());
      regionMapper = parserFactory->createStyledMapper(&hrdClass, nullptr);
    };
    // устанавливаем фон редактора при каждой перезагрузке схем.
    SetBgEditor();
  } catch (Exception& e) {
    COLORER_LOG_ERROR("%", e.what());
    auto error_mes = UnicodeString(e.what());
    showExceptionMessage(&error_mes);
    disableColorer();
  }

  Info.RestoreScreen(scr);
}

FarEditor* FarEditorSet::addCurrentEditor()
{
  if (viewFirst == 1) {
    viewFirst = 2;
    ReloadBase();
  }

  EditorInfo ei;
  Info.EditorControl(ECTL_GETINFO, &ei);

  FarEditor* editor = new FarEditor(&Info, parserFactory.get());
  farEditorInstances.emplace(ei.EditorID, editor);
  UnicodeString* s = getCurrentFileName();
  editor->chooseFileType(s);
  delete s;
  editor->setTrueMod(consoleAnnotationAvailable);
  editor->setRegionMapper(regionMapper.get());
  editor->setDrawCross(drawCross);
  editor->setDrawPairs(drawPairs);
  editor->setDrawSyntax(drawSyntax);
  editor->setOutlineStyle(oldOutline);

  return editor;
}

UnicodeString* FarEditorSet::getCurrentFileName()
{
  LPWSTR FileName = nullptr;
  size_t FileNameSize = Info.EditorControl(ECTL_GETFILENAME, nullptr);

  if (FileNameSize) {
    FileName = new wchar_t[FileNameSize];

    if (FileName) {
      Info.EditorControl(ECTL_GETFILENAME, FileName);
    }
  }

  UnicodeString fnpath(FileName);
  int slash_idx = fnpath.lastIndexOf('/');

  UnicodeString* s = new UnicodeString(fnpath, slash_idx + 1);
  delete[] FileName;
  return s;
}

FarEditor* FarEditorSet::getCurrentEditor()
{
  EditorInfo ei {};
  Info.EditorControl(ECTL_GETINFO, &ei);
  auto if_editor = farEditorInstances.find(ei.EditorID);
  if (if_editor != farEditorInstances.end()) {
    return if_editor->second;
  }
  return nullptr;
}

const wchar_t* FarEditorSet::GetMsg(int msg)
{
  return (Info.GetMsg(Info.ModuleNumber, msg));
}

void FarEditorSet::enableColorer(bool fromEditor)
{
  rEnabled = true;
  {
    KeyFileHelper(settingsIni).SetInt(cSectionName, cRegEnabled, rEnabled);
  }
  ReloadBase();
}

void FarEditorSet::disableColorer()
{
  rEnabled = false;
  {
    KeyFileHelper(settingsIni).SetInt(cSectionName, cRegEnabled, rEnabled);
  }
  dropCurrentEditor(true);

  regionMapper.reset();
  parserFactory.reset();
}

void FarEditorSet::ApplySettingsToEditors()
{
  for (auto fe = farEditorInstances.begin(); fe != farEditorInstances.end(); ++fe) {
    fe->second->setTrueMod(consoleAnnotationAvailable);
    fe->second->setDrawCross(drawCross);
    fe->second->setDrawPairs(drawPairs);
    fe->second->setDrawSyntax(drawSyntax);
    fe->second->setOutlineStyle(oldOutline);
  }
}

void FarEditorSet::dropCurrentEditor(bool clean)
{
  EditorInfo ei {};
  Info.EditorControl(ECTL_GETINFO, &ei);
  auto it = farEditorInstances.find(ei.EditorID);
  if (it != farEditorInstances.end()) {
    if (clean) {
      it->second->cleanEditor();
    }
    delete it->second;
    farEditorInstances.erase(it);
    Info.EditorControl(ECTL_REDRAW, nullptr);
  }
}

void FarEditorSet::dropAllEditors(bool clean)
{
  if (clean) {
    // мы не имеем доступа к другим редакторам, кроме текущего
    dropCurrentEditor(clean);
  }
  for (auto fe = farEditorInstances.begin(); fe != farEditorInstances.end(); ++fe) {
    delete fe->second;
  }

  farEditorInstances.clear();
}

void FarEditorSet::ReadSettings()
{
  KeyFileReadSection kfh(settingsIni, cSectionName);
  std::wstring hrdName = kfh.GetString(cRegHrdName, cHrdNameDefault);
  std::wstring hrdNameTm = kfh.GetString(cRegHrdNameTm, cHrdNameTmDefault);
  std::wstring catalogPath = kfh.GetString(cRegCatalog, cCatalogDefault);
  std::wstring userHrdPath = kfh.GetString(cRegUserHrdPath, cUserHrdPathDefault);
  std::wstring userHrcPath = kfh.GetString(cRegUserHrcPath, cUserHrcPathDefault);

  delete sHrdName;
  delete sHrdNameTm;
  delete sCatalogPath;
  delete sUserHrdPath;
  delete sUserHrcPath;
  sHrdName = nullptr;
  sCatalogPath = nullptr;
  sUserHrdPath = nullptr;
  sUserHrcPath = nullptr;

  sHrdName = new UnicodeString(hrdName.c_str());
  sHrdNameTm = new UnicodeString(hrdNameTm.c_str());
  sCatalogPath = new UnicodeString(catalogPath.c_str());
  sCatalogPathExp.reset(PathToFullS(catalogPath.c_str(), false));
  if (!sCatalogPathExp || !sCatalogPathExp->length()) {
    sCatalogPathExp.reset(GetConfigPath(UnicodeString(FarCatalogXml)));
  }

  sUserHrdPath = new UnicodeString(userHrdPath.c_str());
  sUserHrdPathExp.reset(PathToFullS(userHrdPath.c_str(), false));
  sUserHrcPath = new UnicodeString(userHrcPath.c_str());
  sUserHrcPathExp.reset(PathToFullS(userHrcPath.c_str(), false));

  // two '!' disable "Compiler Warning (level 3) C4800" and slightly faster code
  rEnabled = !!kfh.GetInt(cRegEnabled, cEnabledDefault);
  drawCross = kfh.GetInt(cRegCrossDraw, cCrossDrawDefault);
  drawPairs = !!kfh.GetInt(cRegPairsDraw, cPairsDrawDefault);
  drawSyntax = !!kfh.GetInt(cRegSyntaxDraw, cSyntaxDrawDefault);
  oldOutline = !!kfh.GetInt(cRegOldOutLine, cOldOutLineDefault);
  TrueModOn = !!kfh.GetInt(cRegTrueMod, cTrueMod);
  ChangeBgEditor = !!kfh.GetInt(cRegChangeBgEditor, cChangeBgEditor);
}

void FarEditorSet::SetDefaultSettings()
{
  KeyFileHelper kfh(settingsIni);
  kfh.SetInt(cSectionName, cRegEnabled, cEnabledDefault);
  kfh.SetString(cSectionName, cRegHrdName, cHrdNameDefault);
  kfh.SetString(cSectionName, cRegHrdNameTm, cHrdNameTmDefault);
  kfh.SetString(cSectionName, cRegCatalog, cCatalogDefault);
  kfh.SetInt(cSectionName, cRegCrossDraw, cCrossDrawDefault);
  kfh.SetInt(cSectionName, cRegPairsDraw, cPairsDrawDefault);
  kfh.SetInt(cSectionName, cRegSyntaxDraw, cSyntaxDrawDefault);
  kfh.SetInt(cSectionName, cRegOldOutLine, cOldOutLineDefault);
  kfh.SetInt(cSectionName, cRegTrueMod, cTrueMod);
  kfh.SetInt(cSectionName, cRegChangeBgEditor, cChangeBgEditor);
  kfh.SetString(cSectionName, cRegUserHrdPath, cUserHrdPathDefault);
  kfh.SetString(cSectionName, cRegUserHrcPath, cUserHrcPathDefault);
}

void FarEditorSet::SaveSettings()
{
  KeyFileHelper kfh(settingsIni);
  kfh.SetInt(cSectionName, cRegEnabled, rEnabled);
  kfh.SetString(cSectionName, cRegHrdName, sHrdName->getWChars());
  kfh.SetString(cSectionName, cRegHrdNameTm, sHrdNameTm->getWChars());
  kfh.SetString(cSectionName, cRegCatalog, sCatalogPath->getWChars());
  kfh.SetInt(cSectionName, cRegCrossDraw, drawCross);
  kfh.SetInt(cSectionName, cRegPairsDraw, drawPairs);
  kfh.SetInt(cSectionName, cRegSyntaxDraw, drawSyntax);
  kfh.SetInt(cSectionName, cRegOldOutLine, oldOutline);
  kfh.SetInt(cSectionName, cRegTrueMod, TrueModOn);
  kfh.SetInt(cSectionName, cRegChangeBgEditor, ChangeBgEditor);
  kfh.SetString(cSectionName, cRegUserHrdPath, sUserHrdPath->getWChars());
  kfh.SetString(cSectionName, cRegUserHrcPath, sUserHrcPath->getWChars());
}

bool FarEditorSet::checkConsoleAnnotationAvailable()
{
  return WINPORT(GetConsoleColorPalette)(NULL) >= 24;
}

bool FarEditorSet::SetBgEditor()
{
  if (rEnabled && ChangeBgEditor && !consoleAnnotationAvailable) {
    FarSetColors fsc;
    uint64_t c;

    const StyledRegion* def_text =
        StyledRegion::cast(regionMapper->getRegionDefine(UnicodeString("def:Text")));
    c = (def_text->back << 4) + def_text->fore;

    fsc.Flags = FCLR_REDRAW;
    fsc.ColorCount = 1;
    fsc.StartIndex = COL_EDITORTEXT;
    fsc.Colors = &c;
    return !!Info.AdvControl(Info.ModuleNumber, ACTL_SETARRAYCOLOR, &fsc, nullptr);
  }
  return false;
}

const UnicodeString* FarEditorSet::getParamDefValue(FileType* type, UnicodeString param)
{
  const UnicodeString* value;
  value = type->getParamDefaultValue(param);
  if (value == nullptr)
    value = defaultType->getParamValue(param);
  ASSERT_MSG(value != nullptr, "no value for '%ls'", param.getWChars());
  UnicodeString* p = new UnicodeString("<default-");
  p->append(*value);
  p->append(">");
  return p;
}

FarList* FarEditorSet::buildHrcList()
{
  int num = getCountFileTypeAndGroup();
  ;
  UnicodeString group;
  FileType* type = nullptr;

  FarListItem* hrcList = new FarListItem[num];
  memset(hrcList, 0, sizeof(FarListItem) * (num));

  auto& hrcParser = parserFactory->getHrcLibrary();
  for (int idx = 0, i = 0;; idx++, i++) {
    type = hrcParser.enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (group.compare(type->getGroup()) != 0) {
      hrcList[i].Flags = LIF_SEPARATOR;
      i++;
    };

    group = type->getGroup();

    hrcList[i].Text = new wchar_t[256];
    swprintf((wchar_t*) hrcList[i].Text, 255, L"%ls: %ls", group.getWChars(),
             type->getDescription().getWChars());
  }

  hrcList[1].Flags = LIF_SELECTED;
  FarList* ListItems = new FarList;
  ListItems->Items = hrcList;
  ListItems->ItemsNumber = num;

  return ListItems;
}

FarList* FarEditorSet::buildParamsList(FileType* type)
{
  // max count params
  size_t size = type->getParamCount() + defaultType->getParamCount();
  FarListItem* fparam = new FarListItem[size];
  memset(fparam, 0, sizeof(FarListItem) * (size));

  int count = 0;
  std::vector<UnicodeString> params = defaultType->enumParams();
  for (const auto& paramname : params) {
    fparam[count++].Text = wcsdup(paramname.getWChars());
  }
  params = type->enumParams();
  for (const auto& paramname : params) {
    if (defaultType->getParamValue(paramname) == nullptr) {
      fparam[count++].Text = wcsdup(paramname.getWChars());
    }
  }

  fparam[0].Flags = LIF_SELECTED;
  FarList* lparam = new FarList;
  lparam->Items = fparam;
  lparam->ItemsNumber = count;
  return lparam;
}

void FarEditorSet::ChangeParamValueListType(HANDLE hDlg, bool dropdownlist)
{
  struct FarDialogItem* DialogItem =
      (FarDialogItem*) malloc(Info.SendDlgMessage(hDlg, DM_GETDLGITEM, IDX_CH_PARAM_VALUE_LIST, 0));

  Info.SendDlgMessage(hDlg, DM_GETDLGITEM, IDX_CH_PARAM_VALUE_LIST, (LONG_PTR) DialogItem);
  DialogItem->Flags = DIF_LISTWRAPMODE;
  if (dropdownlist) {
    DialogItem->Flags |= DIF_DROPDOWNLIST;
  }
  Info.SendDlgMessage(hDlg, DM_SETDLGITEM, IDX_CH_PARAM_VALUE_LIST, (LONG_PTR) DialogItem);

  free(DialogItem);
}

void FarEditorSet::setCrossValueListToCombobox(FileType* type, HANDLE hDlg)
{
  const UnicodeString* value = ((FileType*) type)->getParamUserValue(DShowCross);
  const UnicodeString* def_value = getParamDefValue(type, DShowCross);

  int count = 5;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DNone.getWChars();
  fcross[1].Text = DVertical.getWChars();
  fcross[2].Text = DHorizontal.getWChars();
  fcross[3].Text = DBoth.getWChars();
  fcross[4].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;

  int ret = 4;
  if ((value == nullptr) || !value->length()) {
    ret = 4;
  }
  else {
    if (value->equals(&DNone)) {
      ret = 0;
    }
    else if (value->equals(&DVertical)) {
      ret = 1;
    }
    else if (value->equals(&DHorizontal)) {
      ret = 2;
    }
    else if (value->equals(&DBoth)) {
      ret = 3;
    }
  };
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, true);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, (LONG_PTR) lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setCrossPosValueListToCombobox(FileType* type, HANDLE hDlg)
{
  const UnicodeString* value = type->getParamUserValue(DCrossZorder);
  const UnicodeString* def_value = getParamDefValue(type, DCrossZorder);

  int count = 3;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DBottom.getWChars();
  fcross[1].Text = DTop.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;

  int ret = 2;
  if ((value == nullptr) || !value->length()) {
    ret = 2;
  }
  else {
    if (value->equals(&DBottom)) {
      ret = 0;
    }
    else if (value->equals(&DTop)) {
      ret = 1;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, true);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, (LONG_PTR) lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setYNListValueToCombobox(FileType* type, HANDLE hDlg, UnicodeString param)
{
  const UnicodeString* value = type->getParamUserValue(param);
  const UnicodeString* def_value = getParamDefValue(type, param);

  int count = 3;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DNo.getWChars();
  fcross[1].Text = DYes.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;

  int ret = 2;
  if ((value == nullptr) || !value->length()) {
    ret = 2;
  }
  else {
    if (value->equals(&DNo)) {
      ret = 0;
    }
    else if (value->equals(&DYes)) {
      ret = 1;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, true);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, (LONG_PTR) lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setTFListValueToCombobox(FileType* type, HANDLE hDlg, UnicodeString param)
{
  const UnicodeString* value = type->getParamUserValue(param);
  const UnicodeString* def_value = getParamDefValue(type, param);

  int count = 3;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DFalse.getWChars();
  fcross[1].Text = DTrue.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;

  int ret = 2;
  if ((value == nullptr) || !value->length()) {
    ret = 2;
  }
  else {
    if (value->equals(&DFalse)) {
      ret = 0;
    }
    else if (value->equals(&DTrue)) {
      ret = 1;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, true);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, (LONG_PTR) lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setCustomListValueToCombobox(FileType* type, HANDLE hDlg, UnicodeString param)
{
  const UnicodeString* value = type->getParamUserValue(param);
  const UnicodeString* def_value = getParamDefValue(type, param);

  int count = 1;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;

  fcross[0].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, false);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, (LONG_PTR) lcross);

  if (value) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_CH_PARAM_VALUE_LIST,
                        (LONG_PTR) value->getWChars());
  }
  delete def_value;
  delete[] fcross;
  delete lcross;
}

FileType* FarEditorSet::getCurrentTypeInDialog(HANDLE hDlg)
{
  int k = (int) Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, IDX_CH_SCHEMAS, 0);
  return getFileTypeByIndex(k);
}

void FarEditorSet::OnChangeHrc(HANDLE hDlg)
{
  if (menuid != -1) {
    SaveChangedValueParam(hDlg);
  }
  FileType* type = getCurrentTypeInDialog(hDlg);
  FarList* List = buildParamsList(type);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_LIST, (LONG_PTR) List);
  for (int i = 0; i < List->ItemsNumber; i++) {
    delete[] List->Items[i].Text;
  }
  delete[] List->Items;
  delete List;
  OnChangeParam(hDlg, 0);
}

void FarEditorSet::SaveChangedValueParam(HANDLE hDlg)
{
  FarListGetItem List;
  List.ItemIndex = menuid;
  Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, (LONG_PTR) &List);

  // param name
  UnicodeString p(List.Item.Text);
  // param value
  UnicodeString v(
      trim((wchar_t*) Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CH_PARAM_VALUE_LIST, 0)));
  FileType* type = getCurrentTypeInDialog(hDlg);
  const UnicodeString* value = ((FileType*) type)->getParamUserValue(p);
  const UnicodeString* def_value = getParamDefValue(type, p);
  if (v.compare(*def_value) == 0) {
    if (value != nullptr)
      type->setParamValue(p, nullptr);
  }
  else if (value == nullptr || v.compare(*value) != 0) {  // changed
    addParamAndValue(type, p, v);
  }
  delete def_value;
}

void FarEditorSet::OnChangeParam(HANDLE hDlg, int idx)
{
  if (menuid != idx && menuid != -1) {
    SaveChangedValueParam(hDlg);
  }
  FileType* type = getCurrentTypeInDialog(hDlg);
  FarListGetItem List {};
  List.ItemIndex = idx;
  Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, (LONG_PTR) &List);

  menuid = idx;
  UnicodeString p(List.Item.Text);

  const UnicodeString* value;
  value = type->getParamDescription(p);
  if (value == nullptr) {
    value = defaultType->getParamDescription(p);
  }
  if (value) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_CH_DESCRIPTION, (LONG_PTR) value->getWChars());
  }

  COORD c;
  c.X = 0;
  Info.SendDlgMessage(hDlg, DM_SETCURSORPOS, IDX_CH_DESCRIPTION, (LONG_PTR) &c);
  if (p.equals(&DShowCross)) {
    setCrossValueListToCombobox(type, hDlg);
  }
  else {
    if (p.equals(&DCrossZorder)) {
      setCrossPosValueListToCombobox(type, hDlg);
    }
    else if (p.equals(&DMaxLen) || p.equals(&DBackparse) || p.equals(&DDefFore) ||
             p.equals(&DDefBack) || p.equals(&DFirstLines) || p.equals(&DFirstLineBytes) ||
             p.equals(&DHotkey))
    {
      setCustomListValueToCombobox(type, hDlg, UnicodeString(List.Item.Text));
    }
    else if (p.equals(&DFullback)) {
      setYNListValueToCombobox(type, hDlg, UnicodeString(List.Item.Text));
    }
    else
      setTFListValueToCombobox(type, hDlg, UnicodeString(List.Item.Text));
  }
}

void FarEditorSet::OnSaveHrcParams(HANDLE hDlg)
{
  SaveChangedValueParam(hDlg);
  FarHrcSettings p(this, parserFactory.get());
  p.writeUserProfile();
}

LONG_PTR WINAPI SettingHrcDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
  FarEditorSet* fes = (FarEditorSet*) Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
  ;

  switch (Msg) {
    case DN_GOTFOCUS: {
      if (fes->dialogFirstFocus) {
        fes->menuid = -1;
        fes->OnChangeHrc(hDlg);
        fes->dialogFirstFocus = false;
      }
      return false;
    }
    case DN_BTNCLICK:
      switch (Param1) {
        case IDX_CH_OK:
          fes->OnSaveHrcParams(hDlg);
          return false;
      }
      break;
    case DN_EDITCHANGE:
      switch (Param1) {
        case IDX_CH_SCHEMAS:
          fes->menuid = -1;
          fes->OnChangeHrc(hDlg);
          return true;
      }
      break;
    case DN_LISTCHANGE:
      switch (Param1) {
        case IDX_CH_PARAM_LIST:
          fes->OnChangeParam(hDlg, (int) Param2);
          return true;
      }
      break;
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

void FarEditorSet::configureHrc()
{
  if (!rEnabled)
    return;

  const int fdiCount = 9;
  FarDialogItem fdi[fdiCount] = {
      {DI_DOUBLEBOX,  2,  1, 56, 21,     0, {}, 0,    0, L"", 0}, //  IDX_CH_BOX,
      {     DI_TEXT,  3,  3,  0,  3, FALSE, {}, 0,    0, L"", 0}, //  IDX_CH_CAPTIONLIST,
      { DI_COMBOBOX, 10,  3, 54,  2, FALSE, {}, 0,    0, L"", 0}, //  IDX_CH_SCHEMAS,
      {  DI_LISTBOX,  3,  4, 30, 17,  TRUE, {}, 0,    0, L"", 0}, //  IDX_CH_PARAM_LIST,
      {     DI_TEXT, 32,  5,  0,  5, FALSE, {}, 0,    0, L"", 0}, //  IDX_CH_PARAM_VALUE_CAPTION
      { DI_COMBOBOX, 32,  6, 54,  6, FALSE, {}, 0,    0, L"", 0}, //  IDX_CH_PARAM_VALUE_LIST
      {     DI_EDIT,  4, 18, 54, 18, FALSE, {}, 0,    0, L"", 0}, //  IDX_CH_DESCRIPTION,
      {   DI_BUTTON, 37, 20,  0,  0, FALSE, {}, 0, TRUE, L"", 0}, //  IDX_OK,
      {   DI_BUTTON, 45, 20,  0,  0, FALSE, {}, 0,    0, L"", 0}  //  IDX_CANCEL,
  };  // type, x1, y1, x2, y2, focus, sel, fl, def, data, maxlen

  fdi[IDX_CH_BOX].PtrData = GetMsg(mUserHrcSettingDialog);
  fdi[IDX_CH_CAPTIONLIST].PtrData = GetMsg(mListSyntax);
  FarList* l = buildHrcList();
  fdi[IDX_CH_SCHEMAS].Param.ListItems = l;
  fdi[IDX_CH_SCHEMAS].Flags = DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;
  fdi[IDX_CH_OK].PtrData = GetMsg(mOk);
  fdi[IDX_CH_CANCEL].PtrData = GetMsg(mCancel);
  fdi[IDX_CH_PARAM_LIST].PtrData = GetMsg(mParamList);
  fdi[IDX_CH_PARAM_VALUE_CAPTION].PtrData = GetMsg(mParamValue);
  fdi[IDX_CH_DESCRIPTION].Flags = DIF_READONLY;

  fdi[IDX_CH_PARAM_LIST].Flags = DIF_LISTWRAPMODE | DIF_LISTNOCLOSE;
  fdi[IDX_CH_PARAM_VALUE_LIST].Flags = DIF_LISTWRAPMODE;

  dialogFirstFocus = true;
  HANDLE hDlg = Info.DialogInit(Info.ModuleNumber, -1, -1, 59, 23, L"confighrc", fdi, fdiCount, 0,
                                0, SettingHrcDialogProc, (LONG_PTR) this);
  Info.DialogRun(hDlg);

  for (int idx = 0; idx < l->ItemsNumber; idx++) {
    if (l->Items[idx].Text) {
      delete[] l->Items[idx].Text;
    }
  }
  delete[] l->Items;
  delete l;

  Info.DialogFree(hDlg);
}

void FarEditorSet::addParamAndValue(FileType* filetype, const UnicodeString& name,
                                    const UnicodeString& value, const FileType* def_filetype)
{
  if (filetype->getParamValue(name) == nullptr) {
    const UnicodeString* default_value;
    if (def_filetype) {
      default_value = def_filetype->getParamValue(name);
    }
    else {
      default_value = defaultType->getParamValue(name);
    }
    filetype->addParam(name, *default_value);
  }
  filetype->setParamValue(name, &value);
}

void FarEditorSet::showExceptionMessage(const UnicodeString* message)
{
  auto str_mes = UStr::to_stdwstr(message);
  const wchar_t* exceptionMessage[3] = {GetMsg(mName), str_mes.c_str(), GetMsg(mDie)};
  Info.Message(Info.ModuleNumber, FMSG_WARNING, L"exception", &exceptionMessage[0],
               std::size(exceptionMessage), 1);
}

/* ***** BEGIN LICENSE BLOCK *****
 * Copyright (C) 1999-2009 Cail Lomecb <irusskih at gmail dot com>.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 * ***** END LICENSE BLOCK ***** */
