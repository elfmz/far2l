#include"FarEditorSet.h"

int _snwprintf_s (wchar_t *string, size_t sizeInWords, size_t count, const wchar_t *format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  int result = vswprintf(string, count, format, arglist);
  va_end(arglist);
  return result;
}

FarEditorSet::FarEditorSet()
{
  wchar_t key[255];
  swprintf(key,255, L"%ls/colorer", Info.RootKey);

  DWORD res =rOpenKey(HKEY_CURRENT_USER, key, hPluginRegistry);
  if (res == REG_CREATED_NEW_KEY){
    SetDefaultSettings();
  }

  rEnabled = !!rGetValueDw(hPluginRegistry, cRegEnabled, cEnabledDefault);
  parserFactory = NULL;
  regionMapper = NULL;
  hrcParser = NULL;
  sHrdName = NULL;
  sHrdNameTm = NULL;
  sCatalogPath = NULL;
  sCatalogPathExp = NULL;
  sTempHrdName = NULL;
  sTempHrdNameTm = NULL;
  sUserHrdPath = NULL;
  sUserHrdPathExp = NULL;
  sUserHrcPath = NULL;
  sUserHrcPathExp = NULL;

  ReloadBase();
  viewFirst = 0;
}

FarEditorSet::~FarEditorSet()
{
  dropAllEditors(false);
  WINPORT(RegCloseKey)(hPluginRegistry);
  delete sHrdName;
  delete sHrdNameTm;
  delete sCatalogPath;
  delete sCatalogPathExp;
  delete sUserHrdPath;
  delete sUserHrdPathExp;
  delete sUserHrcPath;
  delete sUserHrcPathExp;
  delete regionMapper;
  delete parserFactory;
}


void FarEditorSet::openMenu()
{
  int iMenuItems[] =
  {
    mListTypes, mMatchPair, mSelectBlock, mSelectPair,
    mListFunctions, mFindErrors, mSelectRegion, mLocateFunction, -1,
    mUpdateHighlight, mReloadBase, mConfigure
  };
  FarMenuItem menuElements[sizeof(iMenuItems) / sizeof(iMenuItems[0])];
  memset(menuElements, 0, sizeof(menuElements));

  try{
    if (!rEnabled){
      menuElements[0].Text = GetMsg(mConfigure);
      menuElements[0].Selected = 1;

      if (Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), nullptr, L"menu", NULL, NULL, menuElements, 1) == 0){
        ReadSettings();
        configure(true);
      }

      return;
    };

    for (int i = sizeof(iMenuItems) / sizeof(iMenuItems[0]) - 1; i >= 0; i--){
      if (iMenuItems[i] == -1){
        menuElements[i].Separator = 1;
      }
      else{
        menuElements[i].Text = GetMsg(iMenuItems[i]);
      }
    };

    menuElements[0].Selected = 1;

    FarEditor *editor = getCurrentEditor();
    if (!editor){
      throw Exception(DString("Can't find current editor in array."));
    }
    int res = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), nullptr, L"menu", NULL, NULL,
      menuElements, sizeof(iMenuItems) / sizeof(iMenuItems[0]));
    switch (res)
    {
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
  }
  catch (Exception &e){
    const size_t count_lines = 4;
    const wchar_t* exceptionMessage[count_lines];
    exceptionMessage[0] = GetMsg(mName);
    exceptionMessage[1] = GetMsg(mCantLoad);
    exceptionMessage[3] = GetMsg(mDie);
    StringBuffer msg("openMenu: ");
    exceptionMessage[2] = (msg+e.getMessage()).getWChars();

    if (getErrorHandler()){
      getErrorHandler()->error(*e.getMessage());
    }

    Info.Message(Info.ModuleNumber, FMSG_WARNING, L"exception", &exceptionMessage[0], count_lines, 1);
    disableColorer();
  };
}


void FarEditorSet::viewFile(const String &path)
{
  if (viewFirst==0) viewFirst=1;
  try{
    if (!rEnabled){
      throw Exception(DString("Colorer is disabled"));
    }

    // Creates store of text lines
    TextLinesStore textLinesStore;
    textLinesStore.loadFile(&path, NULL, true);
    // Base editor to make primary parse
    BaseEditor baseEditor(parserFactory, &textLinesStore);
    RegionMapper *regionMap;
    try{
      regionMap=parserFactory->createStyledMapper(&DConsole, sHrdName);
    }
    catch (ParserFactoryException &e){
      if (getErrorHandler() != NULL){
        getErrorHandler()->error(*e.getMessage());
      }
      regionMap = parserFactory->createStyledMapper(&DConsole, NULL);
    };
    baseEditor.setRegionMapper(regionMap);
    baseEditor.chooseFileType(&path);
    // initial event
    baseEditor.lineCountEvent(textLinesStore.getLineCount());
    // computing background color
    int background = 0x1F;
    const StyledRegion *rd = StyledRegion::cast(regionMap->getRegionDefine(DString("def:Text")));

    if (rd != NULL && rd->bfore && rd->bback){
      background = rd->fore + (rd->back<<4);
    }

    // File viewing in console window
    TextConsoleViewer viewer(&baseEditor, &textLinesStore, background, -1);
    viewer.view();
    delete regionMap;
  }
  catch (Exception &e){
    const size_t count_lines = 4;
    const wchar_t* exceptionMessage[count_lines];
    exceptionMessage[0] = GetMsg(mName);
    exceptionMessage[1] = GetMsg(mCantOpenFile);
    exceptionMessage[3] = GetMsg(mDie);
    exceptionMessage[2] = e.getMessage()->getWChars();
    Info.Message(Info.ModuleNumber, FMSG_WARNING, L"exception", &exceptionMessage[0], count_lines, 1);
  };
}

int FarEditorSet::getCountFileTypeAndGroup()
{
  int num = 0;
  const String *group = NULL;
  FileType *type = NULL;

  for (int idx = 0;; idx++, num++){
    type = hrcParser->enumerateFileTypes(idx);

    if (type == NULL){
      break;
    }

    if (group != NULL && !group->equals(type->getGroup())){
      num++;
    }

    group = type->getGroup();
  };
  return num;
}

FileTypeImpl* FarEditorSet::getFileTypeByIndex(int idx)
{
  FileType *type = NULL;
  const String *group = NULL;
  for (int i = 0; idx>=0; idx--, i++){
    type = hrcParser->enumerateFileTypes(i);

    if (!type){
      break;
    }

    if (group != NULL && !group->equals(type->getGroup())){
      idx--;
    }
    group = type->getGroup();
  };

  return (FileTypeImpl*)type;
}

void FarEditorSet::FillTypeMenu(ChooseTypeMenu *Menu, FileType *CurFileType)
{
  const String *group = NULL;
  FileType *type = NULL;

  group = &DAutodetect;

  for (int idx = 0;; idx++){
    type = hrcParser->enumerateFileTypes(idx);

    if (type == NULL){
      break;
    }

    if (group != NULL && !group->equals(type->getGroup())){
      Menu->AddGroup(type->getGroup()->getWChars());
      group = type->getGroup();
    };

    int i;
    const String *v;
    v=((FileTypeImpl*)type)->getParamValue(DFavorite);
    if (v && v->equals(&DTrue)) i= Menu->AddFavorite(type);
    else i=Menu->AddItem(type);
    if (type == CurFileType){
      Menu->SetSelected(i);
    }
  };

}

inline wchar_t __cdecl Upper(wchar_t Ch) { WINPORT(CharUpperBuff)(&Ch, 1); return Ch; }

LONG_PTR WINAPI KeyDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
  int key = Param2;

  if (Msg == DN_KEY && key != KEY_F1)
  {
    if (key == KEY_ESC || key == KEY_ENTER || key == KEY_NUMENTER)
    {
      return FALSE;
    }

    if (key > 31)
    {
      wchar wkey[2];

      if (key > 128) {
        FSF.FarKeyToName(key, wkey, 2);
        wchar_t* c= FSF.XLat(wkey, 0, 1, 0);
        key=FSF.FarNameToKey(c);
      }

      if (key < 0xFFFF)
        key=Upper((wchar_t)(key&0x0000FFFF))|(key&(~0x0000FFFF));

      if((key >= 48 && key <= 57)||(key >= 65 && key <= 90)){
        FSF.FarKeyToName(key, wkey, 2);
        Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, 2, (LONG_PTR)wkey);
      }
      return TRUE;
    }
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

void FarEditorSet::chooseType()
{
  FarEditor *fe = getCurrentEditor();
  if (!fe){
    return;
  }

  ChooseTypeMenu menu(GetMsg(mAutoDetect),GetMsg(mFavorites));
  FillTypeMenu(&menu,fe->getFileType());
 
  wchar_t bottom[20];
  swprintf(bottom, 20, GetMsg(mTotalTypes), hrcParser->getFileTypesCount());
  int BreakKeys[4]={VK_INSERT,VK_DELETE,VK_F4,0};
  int BreakCode,i;
  while (1) {
    i = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT | FMENU_USEEXT,
      GetMsg(mSelectSyntax), bottom, L"filetypechoose", BreakKeys,&BreakCode, (const struct FarMenuItem *)menu.getItems(), menu.getItemsCount());

    if (i>=0){
      if (BreakCode==0){
        if (i!=0 && !menu.IsFavorite(i)) menu.MoveToFavorites(i);
        else menu.SetSelected(i);
      }
      else
      if (BreakCode==1){
        if (i!=0 && menu.IsFavorite(i)) menu.DelFromFavorites(i);
        else menu.SetSelected(i);
      }
      else
        if (BreakCode==2){
          if (i==0)  {
            menu.SetSelected(i);
            continue;
          }

          FarDialogItem KeyAssignDlgData[]=
          { 
            {DI_DOUBLEBOX,3,1,30,4,0,{},0,0,GetMsg(mKeyAssignDialogTitle),0},
            {DI_TEXT,-1,2,0,2,0,{},0,0,GetMsg(mKeyAssignTextTitle),0},
            {DI_EDIT,5,3,28,3,0,{},0,0,L"",0},
          };

          const String *v;
          v=((FileTypeImpl*)menu.GetFileType(i))->getParamValue(DHotkey);
          if (v && v->length())
            KeyAssignDlgData[2].PtrData=v->getWChars();

          HANDLE hDlg = Info.DialogInit(Info.ModuleNumber, -1, -1, 34, 6, L"keyassign", KeyAssignDlgData, ARRAY_SIZE(KeyAssignDlgData), 0, 0, KeyDialogProc, null);
          int res = Info.DialogRun(hDlg);

          if (res!=-1) 
          {
            KeyAssignDlgData[2].PtrData = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,2,0));
            if (menu.GetFileType(i)->getParamValue(DHotkey)==null){
              ((FileTypeImpl*)menu.GetFileType(i))->addParam(&DHotkey);
            }
            delete ((FileTypeImpl*)menu.GetFileType(i))->getParamNotDefaultValue(DHotkey);
			DString ds(KeyAssignDlgData[2].PtrData);
            menu.GetFileType(i)->setParamValue(DHotkey,&ds);
            menu.RefreshItemCaption(i);
          }
          menu.SetSelected(i);
          Info.DialogFree(hDlg);
        }
        else
        {
          if (i==0){
            String *s=getCurrentFileName();
          fe->chooseFileType(s);
          delete s;
          break;
        } 
        fe->setFileType(menu.GetFileType(i));
        break;
      } 
    }else break;
  }

  FarHrcSettings p(parserFactory);
  p.writeUserProfile();
}

const String *FarEditorSet::getHRDescription(const String &name, DString _hrdClass )
{
  const String *descr = NULL;
  if (parserFactory != NULL){
    descr = parserFactory->getHRDescription(_hrdClass, name);
  }

  if (descr == NULL){
    descr = &name;
  }

  return descr;
}

LONG_PTR WINAPI SettingDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2) 
{
  FarEditorSet *fes = (FarEditorSet *)Info.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);; 

  switch (Msg){
  case DN_BTNCLICK:
    switch (Param1){
  case IDX_HRD_SELECT:
    {
      SString *tempSS = new SString(fes->chooseHRDName(fes->sTempHrdName, DConsole));
      delete fes->sTempHrdName;
      fes->sTempHrdName=tempSS;
      const String *descr=fes->getHRDescription(*fes->sTempHrdName,DConsole);
      Info.SendDlgMessage(hDlg,DM_SETTEXTPTR,IDX_HRD_SELECT,(LONG_PTR)descr->getWChars());
      return true;
    }
  case IDX_HRD_SELECT_TM:
    {
      SString *tempSS = new SString(fes->chooseHRDName(fes->sTempHrdNameTm, DRgb));
      delete fes->sTempHrdNameTm;
      fes->sTempHrdNameTm=tempSS;
      const String *descr=fes->getHRDescription(*fes->sTempHrdNameTm,DRgb);
      Info.SendDlgMessage(hDlg,DM_SETTEXTPTR,IDX_HRD_SELECT_TM,(LONG_PTR)descr->getWChars());
      return true;
    }
  case IDX_RELOAD_ALL:
    {
      Info.SendDlgMessage(hDlg,DM_SHOWDIALOG , false,0);
      wchar_t *catalog = trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_CATALOG_EDIT,0));
      wchar_t *userhrd = trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_USERHRD_EDIT,0));
      wchar_t *userhrc = trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_USERHRC_EDIT,0));
      bool trumod = Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_TRUEMOD, 0) && fes->checkConsoleAnnotationAvailable();
      fes->TestLoadBase(catalog, userhrd, userhrc, true, trumod? FarEditorSet::HRCM_BOTH : FarEditorSet::HRCM_CONSOLE);
      Info.SendDlgMessage(hDlg,DM_SHOWDIALOG , true,0);
      return true;
    }
  case IDX_HRC_SETTING:
    {
      fes->configureHrc();
      return true;
    }
  case IDX_OK:
    const wchar_t *temp = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_CATALOG_EDIT,0));
    const wchar_t *userhrd = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_USERHRD_EDIT,0));
    const wchar_t *userhrc = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_USERHRC_EDIT,0));
    bool trumod = Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_TRUEMOD, 0) && fes->checkConsoleAnnotationAvailable();   
    int k = (int)Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_ENABLED, 0);

    if (fes->GetCatalogPath()->compareTo(DString(temp))|| fes->GetUserHrdPath()->compareTo(DString(userhrd)) 
      || (!fes->GetPluginStatus() && k) || (trumod == true)){ 
      if (fes->TestLoadBase(temp, userhrd, userhrc, false, trumod? FarEditorSet::HRCM_BOTH : FarEditorSet::HRCM_CONSOLE)){
        return false;
      }
      else{
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
  try{
    FarDialogItem fdi[] =
    {
      { DI_DOUBLEBOX,3,1,55,22,0,{},0,0,L"",0},                                 //IDX_BOX,
      { DI_CHECKBOX,5,2,0,0,TRUE,{},0,0,L"",0},                                 //IDX_DISABLED,
      { DI_CHECKBOX,5,3,0,0,FALSE,{},DIF_3STATE,0,L"",0},                       //IDX_CROSS,
      { DI_CHECKBOX,5,4,0,0,FALSE,{},0,0,L"",0},                               //IDX_PAIRS,
      { DI_CHECKBOX,5,5,0,0,FALSE,{},0,0,L"",0},                               //IDX_SYNTAX,
      { DI_CHECKBOX,5,6,0,0,FALSE,{},0,0,L"",0},                                //IDX_OLDOUTLINE,
      { DI_CHECKBOX,5,7,0,0,TRUE,{},0,0,L"",0},                                 //IDX_CHANGE_BG,
      { DI_TEXT,5,8,0,8,FALSE,{},0,0,L"",0},                                   //IDX_HRD,
      { DI_BUTTON,20,8,0,0,FALSE,{},0,0,L"",0},                                //IDX_HRD_SELECT,
      { DI_TEXT,5,9,0,9,FALSE,{},0,0,L"",0},                                    //IDX_CATALOG,
      { DI_EDIT,6,10,52,5,FALSE,{(DWORD_PTR)L"catalog"},DIF_HISTORY,0,L"",0},   //IDX_CATALOG_EDIT
      { DI_TEXT,5,11,0,11,FALSE,{},0,0,L"",0},                                    //IDX_USERHRC,
      { DI_EDIT,6,12,52,5,FALSE,{(DWORD_PTR)L"userhrc"},DIF_HISTORY,0,L"",0},   //IDX_USERHRC_EDIT
      { DI_TEXT,5,13,0,13,FALSE,{},0,0,L"",0},                                    //IDX_USERHRD,
      { DI_EDIT,6,14,52,5,FALSE,{(DWORD_PTR)L"userhrd"},DIF_HISTORY,0,L"",0},   //IDX_USERHRD_EDIT
      { DI_SINGLEBOX,4,16,54,16,TRUE,{},0,0,L"",0},                                //IDX_TM_BOX,
      { DI_CHECKBOX,5,17,0,0,TRUE,{},0,0,L"",0},                                //IDX_TRUEMOD,
      { DI_TEXT,20,17,0,17,TRUE,{},0,0,L"",0},                                //IDX_TMMESSAGE,
      { DI_TEXT,5,18,0,18,FALSE,{},0,0,L"",0},                                   //IDX_HRD_TM,
      { DI_BUTTON,20,18,0,0,FALSE,{},0,0,L"",0},                                //IDX_HRD_SELECT_TM,
      { DI_SINGLEBOX,4,19,54,19,TRUE,{},0,0,L"",0},                                //IDX_TM_BOX_OFF,
      { DI_BUTTON,5,20,0,0,FALSE,{},0,0,L"",0},                                //IDX_RELOAD_ALL,
      { DI_BUTTON,30,20,0,0,FALSE,{},0,0,L"",0},                                //IDX_HRC_SETTING,
      { DI_BUTTON,35,21,0,0,FALSE,{},0,TRUE,L"",0},                             //IDX_OK,
      { DI_BUTTON,45,21,0,0,FALSE,{},0,0,L"",0}                                //IDX_CANCEL,
    };// type, x1, y1, x2, y2, focus, sel, fl, def, data, maxlen

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
    fdi[IDX_CATALOG_EDIT].PtrData = sCatalogPath->getWChars();
    fdi[IDX_USERHRC].PtrData = GetMsg(mUserHrcFile);
    fdi[IDX_USERHRC_EDIT].PtrData = sUserHrcPath->getWChars();
    fdi[IDX_USERHRD].PtrData = GetMsg(mUserHrdFile);
    fdi[IDX_USERHRD_EDIT].PtrData = sUserHrdPath->getWChars();
    fdi[IDX_HRD].PtrData = GetMsg(mHRDName);

    const String *descr = NULL;
    sTempHrdName =new SString(sHrdName); 
    descr=getHRDescription(*sTempHrdName,DConsole);

    fdi[IDX_HRD_SELECT].PtrData = descr->getWChars();

    const String *descr2 = NULL;
    sTempHrdNameTm =new SString(sHrdNameTm); 
    descr2=getHRDescription(*sTempHrdNameTm,DRgb);

    fdi[IDX_HRD_TM].PtrData = GetMsg(mHRDNameTrueMod);
    fdi[IDX_HRD_SELECT_TM].PtrData = descr2->getWChars();
    fdi[IDX_CHANGE_BG].PtrData = GetMsg(mChangeBackgroundEditor);
    fdi[IDX_CHANGE_BG].Param.Selected = ChangeBgEditor;
    fdi[IDX_RELOAD_ALL].PtrData = GetMsg(mReloadAll);
    fdi[IDX_HRC_SETTING].PtrData = GetMsg(mUserHrcSetting);
    fdi[IDX_OK].PtrData = GetMsg(mOk);
    fdi[IDX_CANCEL].PtrData = GetMsg(mCancel);
    fdi[IDX_TM_BOX].PtrData = GetMsg(mTrueModSetting);

    if (!checkConsoleAnnotationAvailable() && fromEditor){
      fdi[IDX_HRD_SELECT_TM].Flags = DIF_DISABLE;
      fdi[IDX_TRUEMOD].Flags = DIF_DISABLE;
      if (!checkFarTrueMod()){
        if (!checkConEmu()){
          fdi[IDX_TMMESSAGE].PtrData = GetMsg(mNoFarTMConEmu);
        }
        else{
          fdi[IDX_TMMESSAGE].PtrData = GetMsg(mNoFarTM);
        }
      }
      else{
        if (!checkConEmu()){
          fdi[IDX_TMMESSAGE].PtrData = GetMsg(mNoConEmu);
        }
      }
    }
    /*
    * Dialog activation
    */
    HANDLE hDlg = Info.DialogInit(Info.ModuleNumber, -1, -1, 58, 24, L"config", fdi, ARRAY_SIZE(fdi), 0, 0, SettingDialogProc, (LONG_PTR)this);
    int i = Info.DialogRun(hDlg);

    if (i == IDX_OK){
      fdi[IDX_CATALOG_EDIT].PtrData = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_CATALOG_EDIT,0));
      fdi[IDX_USERHRD_EDIT].PtrData = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_USERHRD_EDIT,0));
      fdi[IDX_USERHRC_EDIT].PtrData = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_USERHRC_EDIT,0));
      //check whether or not to reload the base
      int k = false;

      if (sCatalogPath->compareTo(DString(fdi[IDX_CATALOG_EDIT].PtrData)) || 
          sUserHrdPath->compareTo(DString(fdi[IDX_USERHRD_EDIT].PtrData)) || 
          sUserHrcPath->compareTo(DString(fdi[IDX_USERHRC_EDIT].PtrData)) || 
          sHrdName->compareTo(*sTempHrdName) ||
          sHrdNameTm->compareTo(*sTempHrdNameTm))
      { 
        k = true;
      }

      fdi[IDX_ENABLED].Param.Selected = (int)Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_ENABLED, 0);
      drawCross = (int)Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_CROSS, 0);
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
      sCatalogPath = new SString(DString(fdi[IDX_CATALOG_EDIT].PtrData));
      sUserHrdPath = new SString(DString(fdi[IDX_USERHRD_EDIT].PtrData));
      sUserHrcPath = new SString(DString(fdi[IDX_USERHRC_EDIT].PtrData));

      // if the plugin has been enable, and we will disable
      if (rEnabled && !fdi[IDX_ENABLED].Param.Selected){
        rEnabled = false;
        TrueModOn = !!(fdi[IDX_TRUEMOD].Param.Selected);
        SaveSettings();
        disableColorer();
      }
      else{
        if ((!rEnabled && fdi[IDX_ENABLED].Param.Selected) || k){
          rEnabled = true;
          TrueModOn = !!(fdi[IDX_TRUEMOD].Param.Selected);
          SaveSettings();
          enableColorer(fromEditor);
        }
        else{
          if (TrueModOn !=!!fdi[IDX_TRUEMOD].Param.Selected){
            TrueModOn = !!(fdi[IDX_TRUEMOD].Param.Selected);
            SaveSettings();
            ReloadBase();
          }
          else{
            SaveSettings();
            ApplySettingsToEditors();
            SetBgEditor();
          }
        }
      }
    }

    Info.DialogFree(hDlg);

  }
  catch (Exception &e){
    const size_t count_lines = 4;
    const wchar_t* exceptionMessage[count_lines];
    exceptionMessage[0] = GetMsg(mName);
    exceptionMessage[1] = GetMsg(mCantLoad);
    exceptionMessage[2] = nullptr;
    exceptionMessage[3] = GetMsg(mDie);
    StringBuffer msg("configure: ");
    exceptionMessage[2] = (msg+e.getMessage()).getWChars();

    if (getErrorHandler() != NULL){
      getErrorHandler()->error(*e.getMessage());
    }

    Info.Message(Info.ModuleNumber, FMSG_WARNING, L"exception", &exceptionMessage[0], count_lines, 1);
    disableColorer();
  };
}

const String *FarEditorSet::chooseHRDName(const String *current, DString _hrdClass )
{
  if (parserFactory == NULL){
    return current;
  }

  int count = parserFactory->countHRD(_hrdClass);
  FarMenuItem *menuElements = new FarMenuItem[count];
  memset(menuElements, 0, sizeof(FarMenuItem)*count);

  for (int i = 0; i < count; i++){
    const String *name = parserFactory->enumerateHRDInstances(_hrdClass, i);
    const String *descr = parserFactory->getHRDescription(_hrdClass, *name);

    if (descr == NULL){
      descr = name;
    }

    menuElements[i].Text = descr->getWChars();

    if (current->equals(name)){
      menuElements[i].Selected = 1;
    }
  };

  int result = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT,
    GetMsg(mSelectHRD), nullptr, L"hrd", NULL, NULL, menuElements, count);
  delete[] menuElements;

  if (result == -1){
    return current;
  }

  return parserFactory->enumerateHRDInstances(_hrdClass, result);
}

int FarEditorSet::editorInput(const INPUT_RECORD *ir)
{
  if (rEnabled){
    FarEditor *editor = getCurrentEditor();
    if (editor){
      return editor->editorInput(ir);
    }
  }
  return 0;
}

int FarEditorSet::editorEvent(int Event, void *Param)
{
  // check whether all the editors cleaned
  if (!rEnabled && farEditorInstances.size() && Event==EE_GOTFOCUS){
    dropCurrentEditor(true);
    return 0;
  }

  if (!rEnabled){
    return 0;
  }

  try{
    FarEditor *editor = NULL;
    switch (Event){
    case EE_REDRAW:
      {
        editor = getCurrentEditor();
        if (editor){
          return editor->editorEvent(Event, Param);
        }
        else{
          return 0;
        }
      }
    case EE_GOTFOCUS:
      {
        if (!getCurrentEditor()){
          editor = addCurrentEditor();
          return editor->editorEvent(EE_REDRAW, EEREDRAW_CHANGE);
        }
        return 0;
      }
    case EE_READ:
      {
        addCurrentEditor();
        return 0;
      }
    case EE_CLOSE:
      {
		SString ss(*((int*)Param));
        editor = farEditorInstances.get(&ss);
        farEditorInstances.remove(&ss);
        delete editor;
        return 0;
      }
    }
  }
  catch (Exception &e){
    const size_t count_lines = 4;
    const wchar_t* exceptionMessage[count_lines];
    exceptionMessage[0] = GetMsg(mName);
    exceptionMessage[1] = GetMsg(mCantLoad);
    exceptionMessage[2] = nullptr;
    exceptionMessage[3] = GetMsg(mDie);
    StringBuffer msg("editorEvent: ");
    exceptionMessage[2] = (msg+e.getMessage()).getWChars();

    if (getErrorHandler()){
      getErrorHandler()->error(*e.getMessage());
    }

    Info.Message(Info.ModuleNumber, FMSG_WARNING, L"exception", &exceptionMessage[0], count_lines, 1);
    disableColorer();
  };

  return 0;
}

bool FarEditorSet::TestLoadBase(const wchar_t *catalogPath, const wchar_t *userHrdPath, const wchar_t *userHrcPath, const int full, const HRC_MODE hrc_mode)
{
  bool res = true;
  const wchar_t *marr[2] = { GetMsg(mName), GetMsg(mReloading) };
  HANDLE scr = Info.SaveScreen(0, 0, -1, -1);
  Info.Message(Info.ModuleNumber, 0, NULL, &marr[0], 2, 0);

  ParserFactory *parserFactoryLocal = NULL;
  RegionMapper *regionMapperLocal = NULL;
  HRCParser *hrcParserLocal = NULL;

  SString *catalogPathS = PathToFullS(catalogPath,false);
  SString *userHrdPathS = PathToFullS(userHrdPath,false);
  SString *userHrcPathS = PathToFullS(userHrcPath,false);

  SString *tpath;
  if (!catalogPathS || !catalogPathS->length()){
    tpath = GetConfigPath(DString(FarCatalogXml));
  }
  else{
    tpath = catalogPathS;
  }

  try{
    parserFactoryLocal = new ParserFactory(tpath);
    delete tpath;
    hrcParserLocal = parserFactoryLocal->getHRCParser();
    LoadUserHrd(userHrdPathS, parserFactoryLocal);
    LoadUserHrc(userHrcPathS, parserFactoryLocal);
    FarHrcSettings p(parserFactoryLocal);
    p.readProfile();
    p.readUserProfile();

    if (hrc_mode == HRCM_CONSOLE || hrc_mode == HRCM_BOTH) {
      try{
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DConsole, sTempHrdName);
      }
      catch (ParserFactoryException &e)
      {
        if ((parserFactoryLocal != NULL)&&(parserFactoryLocal->getErrorHandler()!=NULL)){
          parserFactoryLocal->getErrorHandler()->error(*e.getMessage());
        }
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DConsole, NULL);
      };
      delete regionMapperLocal;
      regionMapperLocal=NULL;
    }

    if (hrc_mode == HRCM_RGB || hrc_mode == HRCM_BOTH) {
      try{
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DRgb, sTempHrdNameTm);
      }
      catch (ParserFactoryException &e)
      {
        if ((parserFactoryLocal != NULL)&&(parserFactoryLocal->getErrorHandler()!=NULL)){
          parserFactoryLocal->getErrorHandler()->error(*e.getMessage());
        }
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DRgb, NULL);
      };
    }
    Info.RestoreScreen(scr);
    if (full){
      for (int idx = 0;; idx++){
        FileType *type = hrcParserLocal->enumerateFileTypes(idx);

        if (type == NULL){
          break;
        }

        StringBuffer tname;

        if (type->getGroup() != NULL){
          tname.append(type->getGroup());
          tname.append(DString(": "));
        }

        tname.append(type->getDescription());
        marr[1] = tname.getWChars();
        scr = Info.SaveScreen(0, 0, -1, -1);
        Info.Message(Info.ModuleNumber, 0, NULL, &marr[0], 2, 0);
        type->getBaseScheme();
        Info.RestoreScreen(scr);
      }
    }
  }
  catch (Exception &e){
    const wchar_t *errload[5] = { GetMsg(mName), GetMsg(mCantLoad), 0, GetMsg(mFatal), GetMsg(mDie) };

    errload[2] = e.getMessage()->getWChars();

    if ((parserFactoryLocal != NULL)&&(parserFactoryLocal->getErrorHandler()!=NULL)){
      parserFactoryLocal->getErrorHandler()->error(*e.getMessage());
    }

    Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, &errload[0], 5, 1);
    Info.RestoreScreen(scr);
    res = false;
  };

  delete regionMapperLocal;
  delete parserFactoryLocal;

  return res;
}

void FarEditorSet::ReloadBase()
{
  if (!rEnabled){
    return;
  }

  const wchar_t *marr[2] = { GetMsg(mName), GetMsg(mReloading) };
  HANDLE scr = Info.SaveScreen(0, 0, -1, -1);
  Info.Message(Info.ModuleNumber, 0, NULL, &marr[0], 2, 0);

  dropAllEditors(true);
  delete regionMapper;
  delete parserFactory;
  parserFactory = NULL;
  regionMapper = NULL;

  ReadSettings();
  consoleAnnotationAvailable=checkConsoleAnnotationAvailable() && TrueModOn;
  if (consoleAnnotationAvailable){
    hrdClass = DRgb;
    hrdName = sHrdNameTm;
  }
  else{
    hrdClass = DConsole;
    hrdName = sHrdName;
  }

  try{
    parserFactory = new ParserFactory(sCatalogPathExp);
    hrcParser = parserFactory->getHRCParser();
    LoadUserHrd(sUserHrdPathExp, parserFactory);
    LoadUserHrc(sUserHrcPathExp, parserFactory);
    FarHrcSettings p(parserFactory);
    p.readProfile();
    p.readUserProfile();
	DString dsd("default");
    defaultType= (FileTypeImpl*)hrcParser->getFileType(&dsd);

    try{
      regionMapper = parserFactory->createStyledMapper(&hrdClass, &hrdName);
    }
    catch (ParserFactoryException &e){
      if (getErrorHandler() != NULL){
        getErrorHandler()->error(*e.getMessage());
      }
      regionMapper = parserFactory->createStyledMapper(&hrdClass, NULL);
    };
    //устанавливаем фон редактора при каждой перезагрузке схем.
    SetBgEditor();
  }
  catch (Exception &e){
    const wchar_t *errload[5] = { GetMsg(mName), GetMsg(mCantLoad), 0, GetMsg(mFatal), GetMsg(mDie) };

    errload[2] = e.getMessage()->getWChars();

    if (getErrorHandler() != NULL){
      getErrorHandler()->error(*e.getMessage());
    }

    Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, &errload[0], 5, 1);

    disableColorer();
  };

  Info.RestoreScreen(scr);
}

ErrorHandler *FarEditorSet::getErrorHandler()
{
  if (parserFactory == NULL){
    return NULL;
  }

  return parserFactory->getErrorHandler();
}

FarEditor *FarEditorSet::addCurrentEditor()
{
  if (viewFirst==1){
    viewFirst=2;
    ReloadBase();
  }

  EditorInfo ei;
  Info.EditorControl(ECTL_GETINFO, &ei);

  FarEditor *editor = new FarEditor(&Info, parserFactory);
  SString ss(ei.EditorID);
  farEditorInstances.put(&ss, editor);
  String *s=getCurrentFileName();
  editor->chooseFileType(s);
  delete s;
  editor->setTrueMod(consoleAnnotationAvailable);
  editor->setRegionMapper(regionMapper);
  editor->setDrawCross(drawCross);
  editor->setDrawPairs(drawPairs);
  editor->setDrawSyntax(drawSyntax);
  editor->setOutlineStyle(oldOutline);

  return editor;
}

String* FarEditorSet::getCurrentFileName()
{
  LPWSTR FileName=NULL;
  size_t FileNameSize=Info.EditorControl(ECTL_GETFILENAME,NULL);

  if (FileNameSize){
    FileName=new wchar_t[FileNameSize];

    if (FileName){
      Info.EditorControl(ECTL_GETFILENAME,FileName);
    }
  }

  DString fnpath(FileName);
  int slash_idx = fnpath.lastIndexOf('/');

  SString* s=new SString(fnpath, slash_idx+1);
  delete [] FileName;
  return s;
}

FarEditor *FarEditorSet::getCurrentEditor()
{
  EditorInfo ei;
  Info.EditorControl(ECTL_GETINFO, &ei);
  SString ss(ei.EditorID);
  FarEditor *editor = farEditorInstances.get(&ss);

  return editor;
}

const wchar_t *FarEditorSet::GetMsg(int msg)
{
  return(Info.GetMsg(Info.ModuleNumber, msg));
}

void FarEditorSet::enableColorer(bool fromEditor)
{
  rEnabled = true;
  rSetValue(hPluginRegistry, cRegEnabled, rEnabled);
  ReloadBase();
}

void FarEditorSet::disableColorer()
{
  rEnabled = false;
  rSetValue(hPluginRegistry, cRegEnabled, rEnabled);

  dropCurrentEditor(true);

  delete regionMapper;
  delete parserFactory;
  parserFactory = NULL;
  regionMapper = NULL;
}

void FarEditorSet::ApplySettingsToEditors()
{
  for (FarEditor *fe = farEditorInstances.enumerate(); fe != NULL; fe = farEditorInstances.next()){
    fe->setTrueMod(consoleAnnotationAvailable);
    fe->setDrawCross(drawCross);
    fe->setDrawPairs(drawPairs);
    fe->setDrawSyntax(drawSyntax);
    fe->setOutlineStyle(oldOutline);
  }
}

void FarEditorSet::dropCurrentEditor(bool clean)
{
  EditorInfo ei;
  Info.EditorControl(ECTL_GETINFO, &ei);
  SString ss(ei.EditorID);
  FarEditor *editor = farEditorInstances.get(&ss);
  if (editor){
    if (clean){
      editor->cleanEditor();
    }
	SString ss(ei.EditorID);
    farEditorInstances.remove(&ss);
    delete editor;
  }
  Info.EditorControl(ECTL_REDRAW, NULL);
}

void FarEditorSet::dropAllEditors(bool clean)
{
  if (clean){
    //мы не имеем доступа к другим редакторам, кроме текущего
    dropCurrentEditor(clean);
  }
  for (FarEditor *fe = farEditorInstances.enumerate(); fe != NULL; fe = farEditorInstances.next()){
    delete fe;
  };

  farEditorInstances.clear();
}

void FarEditorSet::ReadSettings()
{
  wchar_t *hrdName = rGetValueSz(hPluginRegistry, cRegHrdName, cHrdNameDefault);
  wchar_t *hrdNameTm = rGetValueSz(hPluginRegistry, cRegHrdNameTm, cHrdNameTmDefault);
  wchar_t *catalogPath = rGetValueSz(hPluginRegistry, cRegCatalog, cCatalogDefault);
  wchar_t *userHrdPath = rGetValueSz(hPluginRegistry, cRegUserHrdPath, cUserHrdPathDefault);
  wchar_t *userHrcPath = rGetValueSz(hPluginRegistry, cRegUserHrcPath, cUserHrcPathDefault);

  delete sHrdName;
  delete sHrdNameTm;
  delete sCatalogPath;
  delete sCatalogPathExp;
  delete sUserHrdPath;
  delete sUserHrdPathExp;
  delete sUserHrcPath;
  delete sUserHrcPathExp;
  sHrdName = NULL;
  sCatalogPath = NULL;
  sCatalogPathExp = NULL;
  sUserHrdPath = NULL;
  sUserHrdPathExp = NULL;
  sUserHrcPath = NULL;
  sUserHrcPathExp = NULL;

  sHrdName = new SString(hrdName);
  sHrdNameTm = new SString(hrdNameTm);
  sCatalogPath = new SString(catalogPath);
  sCatalogPathExp = PathToFullS(catalogPath,false);
  if (!sCatalogPathExp || !sCatalogPathExp->length()){
    delete sCatalogPathExp;
    sCatalogPathExp = GetConfigPath(DString(FarCatalogXml));
  }

  sUserHrdPath = new SString(userHrdPath);
  sUserHrdPathExp = PathToFullS(userHrdPath,false);
  sUserHrcPath = new SString(userHrcPath);
  sUserHrcPathExp = PathToFullS(userHrcPath,false);

  delete[] hrdName;
  delete[] hrdNameTm;
  delete[] catalogPath;
  delete[] userHrdPath;
  delete[] userHrcPath;

  // two '!' disable "Compiler Warning (level 3) C4800" and slightly faster code
  rEnabled = !!rGetValueDw(hPluginRegistry, cRegEnabled, cEnabledDefault);
  drawCross = rGetValueDw(hPluginRegistry, cRegCrossDraw, cCrossDrawDefault);
  drawPairs = !!rGetValueDw(hPluginRegistry, cRegPairsDraw, cPairsDrawDefault);
  drawSyntax = !!rGetValueDw(hPluginRegistry, cRegSyntaxDraw, cSyntaxDrawDefault);
  oldOutline = !!rGetValueDw(hPluginRegistry, cRegOldOutLine, cOldOutLineDefault);
  TrueModOn = !!rGetValueDw(hPluginRegistry, cRegTrueMod, cTrueMod);
  ChangeBgEditor = !!rGetValueDw(hPluginRegistry, cRegChangeBgEditor, cChangeBgEditor);
}

void FarEditorSet::SetDefaultSettings()
{
  rSetValue(hPluginRegistry, cRegEnabled, cEnabledDefault); 
  rSetValue(hPluginRegistry, cRegHrdName, REG_SZ, cHrdNameDefault, static_cast<DWORD>(sizeof(wchar_t)*(wcslen(cHrdNameDefault)+1)));
  rSetValue(hPluginRegistry, cRegHrdNameTm, REG_SZ, cHrdNameTmDefault, static_cast<DWORD>(sizeof(wchar_t)*(wcslen(cHrdNameTmDefault)+1)));
  rSetValue(hPluginRegistry, cRegCatalog, REG_SZ, cCatalogDefault, static_cast<DWORD>(sizeof(wchar_t)*(wcslen(cCatalogDefault)+1)));
  rSetValue(hPluginRegistry, cRegCrossDraw, cCrossDrawDefault); 
  rSetValue(hPluginRegistry, cRegPairsDraw, cPairsDrawDefault); 
  rSetValue(hPluginRegistry, cRegSyntaxDraw, cSyntaxDrawDefault); 
  rSetValue(hPluginRegistry, cRegOldOutLine, cOldOutLineDefault); 
  rSetValue(hPluginRegistry, cRegTrueMod, cTrueMod); 
  rSetValue(hPluginRegistry, cRegChangeBgEditor, cChangeBgEditor); 
  rSetValue(hPluginRegistry, cRegUserHrdPath, REG_SZ, cUserHrdPathDefault, static_cast<DWORD>(sizeof(wchar_t)*(wcslen(cUserHrdPathDefault)+1)));
  rSetValue(hPluginRegistry, cRegUserHrcPath, REG_SZ, cUserHrcPathDefault, static_cast<DWORD>(sizeof(wchar_t)*(wcslen(cUserHrcPathDefault)+1)));
}

void FarEditorSet::SaveSettings()
{
  rSetValue(hPluginRegistry, cRegEnabled, rEnabled); 
  rSetValue(hPluginRegistry, cRegHrdName, REG_SZ, sHrdName->getWChars(), sizeof(wchar_t)*(sHrdName->length()+1));
  rSetValue(hPluginRegistry, cRegHrdNameTm, REG_SZ, sHrdNameTm->getWChars(), sizeof(wchar_t)*(sHrdNameTm->length()+1));
  rSetValue(hPluginRegistry, cRegCatalog, REG_SZ, sCatalogPath->getWChars(), sizeof(wchar_t)*(sCatalogPath->length()+1));
  rSetValue(hPluginRegistry, cRegCrossDraw, drawCross); 
  rSetValue(hPluginRegistry, cRegPairsDraw, drawPairs); 
  rSetValue(hPluginRegistry, cRegSyntaxDraw, drawSyntax); 
  rSetValue(hPluginRegistry, cRegOldOutLine, oldOutline); 
  rSetValue(hPluginRegistry, cRegTrueMod, TrueModOn); 
  rSetValue(hPluginRegistry, cRegChangeBgEditor, ChangeBgEditor); 
  rSetValue(hPluginRegistry, cRegUserHrdPath, REG_SZ, sUserHrdPath->getWChars(), sizeof(wchar_t)*(sUserHrdPath->length()+1));
  rSetValue(hPluginRegistry, cRegUserHrcPath, REG_SZ, sUserHrcPath->getWChars(), sizeof(wchar_t)*(sUserHrcPath->length()+1));
}

bool FarEditorSet::checkConEmu()
{
	return false;/*
  bool conemu;
  wchar_t shareName[255];
  swprintf(shareName, AnnotationShareName, sizeof(AnnotationInfo), GetConsoleWindow());

  HANDLE hSharedMem = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, shareName);
  conemu = (hSharedMem != 0) ? 1 : 0;
  CloseHandle(hSharedMem);
  return conemu;*/
}

bool FarEditorSet::checkFarTrueMod() //TODO
{
  return false;
  /*EditorAnnotation ea;
  ea.StringNumber = 1;
  ea.StartPos = 1;
  ea.EndPos = 2;
  return !!Info.EditorControl(ECTL_ADDANNOTATION, &ea);*/
}

bool FarEditorSet::checkConsoleAnnotationAvailable()
{
  return checkConEmu()&&checkFarTrueMod();
}

bool FarEditorSet::SetBgEditor()
{
  if (rEnabled && ChangeBgEditor && !consoleAnnotationAvailable){
    FarSetColors fsc;
    unsigned char c;

    const StyledRegion* def_text=StyledRegion::cast(regionMapper->getRegionDefine(DString("def:Text")));
    c=(def_text->back<<4) + def_text->fore;

    fsc.Flags=FCLR_REDRAW;
    fsc.ColorCount=1;
    fsc.StartIndex=COL_EDITORTEXT;
    fsc.Colors=&c;
    return !!Info.AdvControl(Info.ModuleNumber,ACTL_SETARRAYCOLOR,&fsc);
  }
  return false;
}

void FarEditorSet::LoadUserHrd(const String *filename, ParserFactory *pf)
{
  if (filename && filename->length()){
    DocumentBuilder docbuilder;
    Document *xmlDocument;
    InputSource *dfis = InputSource::newInstance(filename);
    xmlDocument = docbuilder.parse(dfis);
    Node *types = xmlDocument->getDocumentElement();

    if (*types->getNodeName() != "hrd-sets"){
      docbuilder.free(xmlDocument);
      throw Exception(DString("main '<hrd-sets>' block not found"));
    }
    for (Node *elem = types->getFirstChild(); elem; elem = elem->getNextSibling()){
      if (elem->getNodeType() == Node::ELEMENT_NODE && *elem->getNodeName() == "hrd"){
        pf->parseHRDSetsChild(elem);
      }
    };
    docbuilder.free(xmlDocument);
  }

}

void FarEditorSet::LoadUserHrc(const String *filename, ParserFactory *pf)
{
  if (filename && filename->length()){
    HRCParser *hr = pf->getHRCParser();
    InputSource *dfis = InputSource::newInstance(filename, NULL);
    try{
      hr->loadSource(dfis);
      delete dfis;
    }catch(Exception &e){
      delete dfis;
      throw Exception(e);
    }
  }
}

const String *FarEditorSet::getParamDefValue(FileTypeImpl *type, SString param)
{
  const String *value;
  value = type->getParamDefaultValue(param);
  if (value == NULL) value = defaultType->getParamValue(param);
  StringBuffer *p=new StringBuffer("<default-");
  p->append(DString(value));
  p->append(DString(">"));
  return p;
}

FarList *FarEditorSet::buildHrcList()
{
  int num = getCountFileTypeAndGroup();;
  const String *group = NULL;
  FileType *type = NULL;

  FarListItem *hrcList = new FarListItem[num];
  memset(hrcList, 0, sizeof(FarListItem)*(num));
  group = NULL;

  for (int idx = 0, i = 0;; idx++, i++){
    type = hrcParser->enumerateFileTypes(idx);

    if (type == NULL){
      break;
    }

    if (group != NULL && !group->equals(type->getGroup())){
      hrcList[i].Flags= LIF_SEPARATOR;
      i++;
    };

    group = type->getGroup();

    const wchar_t *groupChars = NULL;

    if (group != NULL){
      groupChars = group->getWChars();
    }
    else{
      groupChars = L"<no group>";
    }

    hrcList[i].Text = new wchar_t[255];
    swprintf((wchar_t*)hrcList[i].Text, 255, L"%ls: %ls", groupChars, type->getDescription()->getWChars());
  };

  hrcList[0].Flags=LIF_SELECTED;
  FarList *ListItems = new FarList;
  ListItems->Items=hrcList;
  ListItems->ItemsNumber=num;

  return ListItems;
}

FarList *FarEditorSet::buildParamsList(FileTypeImpl *type)
{
  //max count params
  size_t size = type->getParamCount()+defaultType->getParamCount();
  FarListItem *fparam= new FarListItem[size];
  memset(fparam, 0, sizeof(FarListItem)*(size));

  int count=0;
  const String *paramname;
  for(int idx=0;;idx++){
    paramname=defaultType->enumerateParameters(idx);
    if (paramname==NULL){
      break;
    }
    fparam[count++].Text=paramname->getWChars();
  }
  for(int idx=0;;idx++){
    paramname=type->enumerateParameters(idx);
    if (paramname==NULL){
      break;
    }
    if (defaultType->getParamValue(*paramname)==null){
      fparam[count++].Text=paramname->getWChars();
    }
  }

  fparam[0].Flags=LIF_SELECTED;
  FarList *lparam = new FarList;
  lparam->Items=fparam;
  lparam->ItemsNumber=count;
  return lparam;

}

void FarEditorSet::ChangeParamValueListType(HANDLE hDlg, bool dropdownlist)
{
  struct FarDialogItem *DialogItem = (FarDialogItem *) malloc(Info.SendDlgMessage(hDlg,DM_GETDLGITEM,IDX_CH_PARAM_VALUE_LIST,0));

  Info.SendDlgMessage(hDlg,DM_GETDLGITEM,IDX_CH_PARAM_VALUE_LIST,(LONG_PTR)DialogItem);
  DialogItem->Flags=DIF_LISTWRAPMODE;
  if (dropdownlist) {
    DialogItem->Flags|=DIF_DROPDOWNLIST;
  }
  Info.SendDlgMessage(hDlg,DM_SETDLGITEM,IDX_CH_PARAM_VALUE_LIST,(LONG_PTR)DialogItem);

  free(DialogItem); 

}

void FarEditorSet::setCrossValueListToCombobox(FileTypeImpl *type, HANDLE hDlg)
{
  const String *value=((FileTypeImpl*)type)->getParamNotDefaultValue(DShowCross);
  const String *def_value=getParamDefValue(type,DShowCross);

  int count = 5;
  FarListItem *fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem)*(count));
  fcross[0].Text = DNone.getWChars();
  fcross[1].Text = DVertical.getWChars();
  fcross[2].Text = DHorizontal.getWChars();
  fcross[3].Text = DBoth.getWChars();
  fcross[4].Text = def_value->getWChars();
  FarList *lcross = new FarList;
  lcross->Items=fcross;
  lcross->ItemsNumber=count;

  int ret=4;
  if (value==NULL || !value->length()){
    ret=4;
  }else{
    if (value->equals(&DNone)){
      ret=0;
    }else 
      if (value->equals(&DVertical)){
        ret=1;
      }else
        if (value->equals(&DHorizontal)){
          ret=2;
        }else
          if (value->equals(&DBoth)){
            ret=3;
          }
  };
  fcross[ret].Flags=LIF_SELECTED;
  ChangeParamValueListType(hDlg,true);
  Info.SendDlgMessage(hDlg,DM_LISTSET,IDX_CH_PARAM_VALUE_LIST,(LONG_PTR)lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setCrossPosValueListToCombobox(FileTypeImpl *type, HANDLE hDlg)
{
  const String *value=type->getParamNotDefaultValue(DCrossZorder);
  const String *def_value=getParamDefValue(type,DCrossZorder);

  int count = 3;
  FarListItem *fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem)*(count));
  fcross[0].Text = DBottom.getWChars();
  fcross[1].Text = DTop.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList *lcross = new FarList;
  lcross->Items=fcross;
  lcross->ItemsNumber=count;

  int ret=2;
  if (value==NULL || !value->length()){
    ret=2;
  }else{
    if (value->equals(&DBottom)){
      ret=0;
    }else 
      if (value->equals(&DTop)){
        ret=1;
      }
  }
  fcross[ret].Flags=LIF_SELECTED;
  ChangeParamValueListType(hDlg,true);
  Info.SendDlgMessage(hDlg,DM_LISTSET,IDX_CH_PARAM_VALUE_LIST,(LONG_PTR)lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setYNListValueToCombobox(FileTypeImpl *type, HANDLE hDlg, DString param)
{
  const String *value=type->getParamNotDefaultValue(param);
  const String *def_value=getParamDefValue(type,param);

  int count = 3;
  FarListItem *fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem)*(count));
  fcross[0].Text = DNo.getWChars();
  fcross[1].Text = DYes.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList *lcross = new FarList;
  lcross->Items=fcross;
  lcross->ItemsNumber=count;

  int ret=2;
  if (value==NULL || !value->length()){
    ret=2;
  }else{
    if (value->equals(&DNo)){
      ret=0;
    }else 
      if (value->equals(&DYes)){
        ret=1;
      }
  }
  fcross[ret].Flags=LIF_SELECTED;
  ChangeParamValueListType(hDlg,true);
  Info.SendDlgMessage(hDlg,DM_LISTSET,IDX_CH_PARAM_VALUE_LIST,(LONG_PTR)lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setTFListValueToCombobox(FileTypeImpl *type, HANDLE hDlg, DString param)
{
  const String *value=type->getParamNotDefaultValue(param);
  const String *def_value=getParamDefValue(type,param);

  int count = 3;
  FarListItem *fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem)*(count));
  fcross[0].Text = DFalse.getWChars();
  fcross[1].Text = DTrue.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList *lcross = new FarList;
  lcross->Items=fcross;
  lcross->ItemsNumber=count;

  int ret=2;
  if (value==NULL || !value->length()){
    ret=2;
  }else{
    if (value->equals(&DFalse)){
      ret=0;
    }else 
      if (value->equals(&DTrue)){
        ret=1;
      }
  }
  fcross[ret].Flags=LIF_SELECTED;
  ChangeParamValueListType(hDlg,true);
  Info.SendDlgMessage(hDlg,DM_LISTSET,IDX_CH_PARAM_VALUE_LIST,(LONG_PTR)lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setCustomListValueToCombobox(FileTypeImpl *type,HANDLE hDlg, DString param)
{
  const String *value=type->getParamNotDefaultValue(param);
  const String *def_value=getParamDefValue(type,param);

  int count = 1;
  FarListItem *fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem)*(count));
  fcross[0].Text = def_value->getWChars();
  FarList *lcross = new FarList;
  lcross->Items=fcross;
  lcross->ItemsNumber=count;

  fcross[0].Flags=LIF_SELECTED;
  ChangeParamValueListType(hDlg,false);
  Info.SendDlgMessage(hDlg,DM_LISTSET,IDX_CH_PARAM_VALUE_LIST,(LONG_PTR)lcross);

  if (value!=NULL){
    Info.SendDlgMessage(hDlg,DM_SETTEXTPTR ,IDX_CH_PARAM_VALUE_LIST,(LONG_PTR)value->getWChars());
  }
  delete def_value;
  delete[] fcross;
  delete lcross;
}

FileTypeImpl *FarEditorSet::getCurrentTypeInDialog(HANDLE hDlg)
{
  int k=(int)Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,IDX_CH_SCHEMAS,0);
  return getFileTypeByIndex(k);
}

void  FarEditorSet::OnChangeHrc(HANDLE hDlg)
{
  if (menuid != -1){
    SaveChangedValueParam(hDlg);
  }
  FileTypeImpl *type = getCurrentTypeInDialog(hDlg);
  FarList *List=buildParamsList(type);

  Info.SendDlgMessage(hDlg,DM_LISTSET,IDX_CH_PARAM_LIST,(LONG_PTR)List);
  delete[] List->Items;
  delete List;
  OnChangeParam(hDlg,0);
}

void FarEditorSet::SaveChangedValueParam(HANDLE hDlg)
{
  FarListGetItem List;
  List.ItemIndex=menuid;
  Info.SendDlgMessage(hDlg,DM_LISTGETITEM,IDX_CH_PARAM_LIST,(LONG_PTR)&List);

  //param name
  DString p=DString(List.Item.Text);
  //param value 
  DString v=DString(trim((wchar_t*)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,IDX_CH_PARAM_VALUE_LIST,0)));
  FileTypeImpl *type = getCurrentTypeInDialog(hDlg);
  const String *value=((FileTypeImpl*)type)->getParamNotDefaultValue(p);
  const String *def_value=getParamDefValue(type,p);
  if (value==NULL || !value->length()){//было default значение
    //если его изменили  
    if (!v.equals(def_value)){
      if (type->getParamValue(p)==null){
        ((FileTypeImpl*)type)->addParam(&p);
      }
      type->setParamValue(p,&v);
    }
  }else{//было пользовательское значение
    if (!v.equals(value)){//changed
      if (v.equals(def_value)){
         //delete value
         delete type->getParamNotDefaultValue(p);
        ((FileTypeImpl*)type)->removeParamValue(&p);
      }else{
        delete type->getParamNotDefaultValue(p);
        type->setParamValue(p,&v);
      }
    }

  }
  delete def_value;
}

void  FarEditorSet::OnChangeParam(HANDLE hDlg, int idx)
{
  if (menuid!=idx && menuid!=-1) {
    SaveChangedValueParam(hDlg);
  }
  FileTypeImpl *type = getCurrentTypeInDialog(hDlg);
  FarListGetItem List;
  List.ItemIndex=idx;
  Info.SendDlgMessage(hDlg,DM_LISTGETITEM,IDX_CH_PARAM_LIST,(LONG_PTR)&List);

  menuid=idx;
  DString p=DString(List.Item.Text);

  const String *value;
  value=type->getParameterDescription(p);
  if (value==NULL){
    value=defaultType->getParameterDescription(p);
  }
  if (value!=NULL){
    Info.SendDlgMessage(hDlg,DM_SETTEXTPTR ,IDX_CH_DESCRIPTION,(LONG_PTR)value->getWChars());
  }

  COORD c;
  c.X=0;
  Info.SendDlgMessage(hDlg,DM_SETCURSORPOS ,IDX_CH_DESCRIPTION,(LONG_PTR)&c);
  if (p.equals(&DShowCross)){
    setCrossValueListToCombobox(type, hDlg);
  }else{
    if (p.equals(&DCrossZorder)){
      setCrossPosValueListToCombobox(type, hDlg);
    }else
      if (p.equals(&DMaxLen)||p.equals(&DBackparse)||p.equals(&DDefFore)||p.equals(&DDefBack)
        ||p.equals("firstlines")||p.equals("firstlinebytes")||p.equals(&DHotkey)){
          setCustomListValueToCombobox(type,hDlg,DString(List.Item.Text));        
      }else
        if (p.equals(&DFullback)){   
          setYNListValueToCombobox(type, hDlg,DString(List.Item.Text));
        }
        else
          setTFListValueToCombobox(type, hDlg,DString(List.Item.Text));
  }

}

void FarEditorSet::OnSaveHrcParams(HANDLE hDlg)
{
   SaveChangedValueParam(hDlg);
   FarHrcSettings p(parserFactory);
   p.writeUserProfile();
}

LONG_PTR WINAPI SettingHrcDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2) 
{
  FarEditorSet *fes = (FarEditorSet *)Info.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);; 

  switch (Msg){
    case DN_GOTFOCUS:
      {
        if (fes->dialogFirstFocus){
          fes->menuid = -1;
          fes->OnChangeHrc(hDlg);
          fes->dialogFirstFocus = false;
        }
        return false;
      }
    case DN_BTNCLICK:
    switch (Param1){
      case IDX_CH_OK:
        fes->OnSaveHrcParams(hDlg);
        return false;
    }
    break;
    case DN_EDITCHANGE:
    switch (Param1){
      case IDX_CH_SCHEMAS:
        fes->menuid = -1;
        fes->OnChangeHrc(hDlg);
        return true;
    }
    break;
    case DN_LISTCHANGE:
    switch (Param1){
      case IDX_CH_PARAM_LIST:
        fes->OnChangeParam(hDlg,(int)Param2);
        return true;
    }
    break;
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

void FarEditorSet::configureHrc()
{
  if (!rEnabled) return;

  FarDialogItem fdi[] =
  {
    { DI_DOUBLEBOX,2,1,56,21,0,{},0,0,L"",0},                                //IDX_CH_BOX,
    { DI_TEXT,3,3,0,3,FALSE,{},0,0,L"",0},                                   //IDX_CH_CAPTIONLIST,
    { DI_COMBOBOX,10,3,54,2,FALSE,{},0,0,L"",0},                             //IDX_CH_SCHEMAS,
    { DI_LISTBOX,3,4,30,17,TRUE,{},0,0,L"",0},                               //IDX_CH_PARAM_LIST,
    { DI_TEXT,32,5,0,5,FALSE,{},0,0,L"",0},                                  //IDX_CH_PARAM_VALUE_CAPTION
    { DI_COMBOBOX,32,6,54,6,FALSE,{},0,0,L"",0},                             //IDX_CH_PARAM_VALUE_LIST
    { DI_EDIT,4,18,54,18,FALSE,{},0,0,L"",0},                                //IDX_CH_DESCRIPTION,
    { DI_BUTTON,37,20,0,0,FALSE,{},0,TRUE,L"",0},                            //IDX_OK,
    { DI_BUTTON,45,20,0,0,FALSE,{},0,0,L"",0}                               //IDX_CANCEL,
  };// type, x1, y1, x2, y2, focus, sel, fl, def, data, maxlen

  fdi[IDX_CH_BOX].PtrData = GetMsg(mUserHrcSettingDialog);
  fdi[IDX_CH_CAPTIONLIST].PtrData = GetMsg(mListSyntax); 
  FarList* l=buildHrcList();
  fdi[IDX_CH_SCHEMAS].Param.ListItems = l;
  fdi[IDX_CH_SCHEMAS].Flags= DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;
  fdi[IDX_CH_OK].PtrData = GetMsg(mOk);
  fdi[IDX_CH_CANCEL].PtrData = GetMsg(mCancel);  
  fdi[IDX_CH_PARAM_LIST].PtrData = GetMsg(mParamList);
  fdi[IDX_CH_PARAM_VALUE_CAPTION].PtrData = GetMsg(mParamValue);
  fdi[IDX_CH_DESCRIPTION].Flags= DIF_READONLY;

  fdi[IDX_CH_PARAM_LIST].Flags= DIF_LISTWRAPMODE | DIF_LISTNOCLOSE;
  fdi[IDX_CH_PARAM_VALUE_LIST].Flags= DIF_LISTWRAPMODE ;

  dialogFirstFocus = true;
  HANDLE hDlg = Info.DialogInit(Info.ModuleNumber, -1, -1, 59, 23, L"confighrc", fdi, ARRAY_SIZE(fdi), 0, 0, SettingHrcDialogProc, (LONG_PTR)this);
  Info.DialogRun(hDlg); //int i = 
  
  for (int idx = 0; idx < l->ItemsNumber; idx++){
    if (l->Items[idx].Text){
      delete[] l->Items[idx].Text;
    }
  }
  delete[] l->Items;
  delete l;
  
  Info.DialogFree(hDlg);
}

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
