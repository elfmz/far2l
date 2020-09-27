#ifndef _FAREDITORSET_H_
#define _FAREDITORSET_H_

#include <unordered_map>
#if 0
// new colorer don't have a public error handlers
#include<colorer/handlers/FileErrorHandler.h>
#endif // if 0
#include<colorer/handlers/LineRegionsSupport.h>
#include<colorer/handlers/StyledHRDMapper.h>
#include<colorer/viewer/TextConsoleViewer.h>
#if 0
#include<common/Logging.h>
#endif // if 0
#include<colorer/unicode/Encodings.h>

#include "pcolorer.h"
#include "tools.h"
#include "FarEditor.h"
#include "FarHrcSettings.h"
#include "ChooseTypeMenu.h"

//registry keys
extern const wchar_t cRegEnabled[];
extern const wchar_t cRegHrdName[];
extern const wchar_t cRegHrdNameTm[];
extern const wchar_t cRegCatalog[];
extern const wchar_t cRegCrossDraw[];
extern const wchar_t cRegPairsDraw[];
extern const wchar_t cRegSyntaxDraw[];
extern const wchar_t cRegOldOutLine[];
extern const wchar_t cRegTrueMod[];
extern const wchar_t cRegChangeBgEditor[];
extern const wchar_t cRegUserHrdPath[];
extern const wchar_t cRegUserHrcPath[];

//values of registry keys by default
extern const bool cEnabledDefault;
extern const wchar_t cHrdNameDefault[];
extern const wchar_t cHrdNameTmDefault[];
extern const wchar_t cCatalogDefault[];
extern const int cCrossDrawDefault;
extern const bool cPairsDrawDefault;
extern const bool cSyntaxDrawDefault;
extern const bool cOldOutLineDefault;
extern const bool cTrueMod;
extern const bool cChangeBgEditor;
extern const wchar_t cUserHrdPathDefault[];
extern const wchar_t cUserHrcPathDefault[];

extern const SString DConsole;
extern const SString DRgb;
extern const SString Ddefault;
extern const SString DAutodetect;

enum
{ IDX_BOX, IDX_ENABLED, IDX_CROSS, IDX_PAIRS, IDX_SYNTAX, IDX_OLDOUTLINE,IDX_CHANGE_BG,
IDX_HRD, IDX_HRD_SELECT, IDX_CATALOG, IDX_CATALOG_EDIT, IDX_USERHRC, IDX_USERHRC_EDIT,
IDX_USERHRD, IDX_USERHRD_EDIT, IDX_TM_BOX, IDX_TRUEMOD,IDX_TMMESSAGE,IDX_HRD_TM, 
IDX_HRD_SELECT_TM, IDX_TM_BOX_OFF, IDX_RELOAD_ALL, IDX_HRC_SETTING, IDX_OK, IDX_CANCEL};

enum
{ IDX_CH_BOX, IDX_CH_CAPTIONLIST, IDX_CH_SCHEMAS, 
IDX_CH_PARAM_LIST,IDX_CH_PARAM_VALUE_CAPTION,IDX_CH_PARAM_VALUE_LIST, IDX_CH_DESCRIPTION, IDX_CH_OK, IDX_CH_CANCEL};

LONG_PTR WINAPI SettingDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
LONG_PTR WINAPI SettingHrcDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);

/**
 * FAR Editors container.
 * Manages all library resources and creates FarEditor class instances.
 * @ingroup far_plugin
 */
class FarEditorSet
{
public:
  /** Creates set and initialises it with PluginStartupInfo structure */
  FarEditorSet();
  /** Standard destructor */
  ~FarEditorSet();

  /** Shows editor actions menu */
  void openMenu();
  /** Shows plugin's configuration dialog */
  void configure(bool fromEditor);
  /** Views current file with internal viewer */
  void viewFile(const String &path);

  /** Dispatch editor event in the opened editor */
  int  editorEvent(int Event, void *Param);
  /** Dispatch editor input event in the opened editor */
  int  editorInput(const INPUT_RECORD *ir);

  /** Get the description of HRD, or parameter name if description=null */
  const String *getHRDescription(const String &name, SString _hrdClass);
  /** Shows dialog with HRD scheme selection */
  const String *chooseHRDName(const String *current, SString _hrdClass );

  /** Reads all registry settings into variables */
  void ReadSettings();
  /**
  * trying to load the database on the specified path
  */
  enum HRC_MODE {HRCM_CONSOLE, HRCM_RGB, HRCM_BOTH};
  bool TestLoadBase(const wchar_t *catalogPath, const wchar_t *userHrdPath, const wchar_t *userHrcPath, const int full, const HRC_MODE hrc_mode);
  SString *GetCatalogPath() {return sCatalogPath;}
  SString *GetUserHrdPath() {return sUserHrdPath;}
  bool GetPluginStatus() {return rEnabled;}

  bool SetBgEditor();
  /**
    Stub method, because Colorer API changed.
  */
  void LoadUserHrd(const String *filename, ParserFactory *pf);
  void LoadUserHrc(const String *filename, ParserFactory *pf);

  SString *sTempHrdName;
  SString *sTempHrdNameTm;

  /** Shows hrc configuration dialog */
  void configureHrc();
  void OnChangeHrc(HANDLE hDlg);
  void OnChangeParam(HANDLE hDlg, int idx);
  void OnSaveHrcParams(HANDLE hDlg);
  bool dialogFirstFocus;
  int menuid;

  bool checkConsoleAnnotationAvailable();
private:
#if 0
  /** Returns current global error handler. */
  ErrorHandler *getErrorHandler();
#endif // if 0
  /** add current active editor and return him. */
  FarEditor *addCurrentEditor();
  /** Returns currently active editor. */
  FarEditor *getCurrentEditor();
  /**
  * Reloads HRC database.
  * Drops all currently opened editors and their
  * internal structures. Prepares to work with newly
  * loaded database. Read settings from registry
  */
  void ReloadBase();

  /** Shows dialog of file type selection */
  void chooseType();
  /** FAR localized messages */
  const wchar_t *GetMsg(int msg);
  /** Applies the current settings for editors*/
  void ApplySettingsToEditors();
  /** writes the default settings in the registry*/
  void SetDefaultSettings();
  void SaveSettings();

  /** Kills all currently opened editors*/
  void dropAllEditors(bool clean);
  /** kill the current editor*/
  void dropCurrentEditor(bool clean);
  /** Disables all plugin processing*/
  void disableColorer();
  /** Enables plugin processing*/
  void enableColorer(bool fromEditor);
  
  bool checkConEmu();
  bool checkFarTrueMod();
  bool consoleAnnotationAvailable;

  int getCountFileTypeAndGroup();
  FileTypeImpl* getFileTypeByIndex(int idx);
  void FillTypeMenu(ChooseTypeMenu *Menu, FileType *CurFileType);
  String* getCurrentFileName();

  // FarList for dialog objects
  FarList *buildHrcList();
  FarList *buildParamsList(FileTypeImpl *type);
  // filetype "default"
  FileTypeImpl *defaultType;
  //change combobox type
  void ChangeParamValueListType(HANDLE hDlg, bool dropdownlist);
  //set list of values to combobox
  void setCrossValueListToCombobox(FileTypeImpl *type,HANDLE hDlg);
  void setCrossPosValueListToCombobox(FileTypeImpl *type,HANDLE hDlg);
  void setYNListValueToCombobox(FileTypeImpl *type,HANDLE hDlg, SString param);
  void setTFListValueToCombobox(FileTypeImpl *type,HANDLE hDlg, SString param);
  void setCustomListValueToCombobox(FileTypeImpl *type,HANDLE hDlg, SString param);

  FileTypeImpl *getCurrentTypeInDialog(HANDLE hDlg);

  const String *getParamDefValue(FileTypeImpl *type, SString param);

  void SaveChangedValueParam(HANDLE hDlg);

#if 0
  Hashtable<FarEditor*> farEditorInstances;
#endif // if 0
  std::unordered_map<intptr_t, FarEditor*> farEditorInstances;
  ParserFactory *parserFactory;
  RegionMapper *regionMapper;
  HRCParser *hrcParser;

  HKEY hPluginRegistry;
  /**current value*/
  SString hrdClass;
  SString hrdName;

  /** registry settings */
  bool rEnabled; // status plugin
  int drawCross;
  bool drawPairs; 
  bool drawSyntax;
  bool oldOutline;
  bool TrueModOn;
  bool ChangeBgEditor;
  SString *sHrdName;
  SString *sHrdNameTm;
  SString *sCatalogPath;
  SString *sUserHrdPath;
  SString *sUserHrcPath;
  
  /** UNC path */
  SString *sCatalogPathExp;
  SString *sUserHrdPathExp;
  SString *sUserHrcPathExp;

  int viewFirst; // 0 - init;  1 - first run view; 2 - first run editor
};

#endif
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
