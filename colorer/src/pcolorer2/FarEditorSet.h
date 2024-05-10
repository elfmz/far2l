#ifndef _FAREDITORSET_H_
#define _FAREDITORSET_H_

#include <colorer/handlers/LineRegionsSupport.h>
#include <colorer/handlers/StyledHRDMapper.h>
#include <colorer/viewer/TextConsoleViewer.h>
#include <unordered_map>
#include "ChooseTypeMenu.h"
#include "FarEditor.h"
#include "pcolorer.h"
#include "tools.h"

// registry keys
extern const char cRegEnabled[];
extern const char cRegHrdName[];
extern const char cRegHrdNameTm[];
extern const char cRegCatalog[];
extern const char cRegCrossDraw[];
extern const char cRegPairsDraw[];
extern const char cRegSyntaxDraw[];
extern const char cRegOldOutLine[];
extern const char cRegTrueMod[];
extern const char cRegChangeBgEditor[];
extern const char cRegUserHrdPath[];
extern const char cRegUserHrcPath[];

// values of registry keys by default
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

extern const UnicodeString DConsole;
extern const UnicodeString DRgb;
extern const UnicodeString Ddefault;
extern const UnicodeString DAutodetect;

enum {
  IDX_BOX,
  IDX_ENABLED,
  IDX_CROSS,
  IDX_PAIRS,
  IDX_SYNTAX,
  IDX_OLDOUTLINE,
  IDX_CHANGE_BG,
  IDX_HRD,
  IDX_HRD_SELECT,
  IDX_CATALOG,
  IDX_CATALOG_EDIT,
  IDX_USERHRC,
  IDX_USERHRC_EDIT,
  IDX_USERHRD,
  IDX_USERHRD_EDIT,
  IDX_TM_BOX,
  IDX_TRUEMOD,
  IDX_TMMESSAGE,
  IDX_HRD_TM,
  IDX_HRD_SELECT_TM,
  IDX_TM_BOX_OFF,
  IDX_RELOAD_ALL,
  IDX_HRC_SETTING,
  IDX_OK,
  IDX_CANCEL
};

enum {
  IDX_CH_BOX,
  IDX_CH_CAPTIONLIST,
  IDX_CH_SCHEMAS,
  IDX_CH_PARAM_LIST,
  IDX_CH_PARAM_VALUE_CAPTION,
  IDX_CH_PARAM_VALUE_LIST,
  IDX_CH_DESCRIPTION,
  IDX_CH_OK,
  IDX_CH_CANCEL
};

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

  /** Dispatch editor event in the opened editor */
  int editorEvent(int Event, void* Param);
  /** Dispatch editor input event in the opened editor */
  int editorInput(const INPUT_RECORD* ir);

  /** Get the description of HRD, or parameter name if description=null */
  const UnicodeString* getHRDescription(const UnicodeString& name, UnicodeString _hrdClass);
  /** Shows dialog with HRD scheme selection */
  const UnicodeString* chooseHRDName(const UnicodeString* current, UnicodeString _hrdClass);

  /** Reads all registry settings into variables */
  void ReadSettings();
  /**
   * trying to load the database on the specified path
   */
  enum HRC_MODE { HRCM_CONSOLE, HRCM_RGB, HRCM_BOTH };
  bool TestLoadBase(const wchar_t* catalogPath, const wchar_t* userHrdPath,
                    const wchar_t* userHrcPath, const int full, const HRC_MODE hrc_mode);
  UnicodeString* GetCatalogPath() { return sCatalogPath; }
  UnicodeString* GetUserHrdPath() { return sUserHrdPath; }
  bool GetPluginStatus() { return rEnabled; }

  bool SetBgEditor();

  UnicodeString* sTempHrdName;
  UnicodeString* sTempHrdNameTm;

  /** Shows hrc configuration dialog */
  void configureHrc();
  void OnChangeHrc(HANDLE hDlg);
  void OnChangeParam(HANDLE hDlg, int idx);
  void OnSaveHrcParams(HANDLE hDlg);
  bool dialogFirstFocus;
  int menuid;

  bool checkConsoleAnnotationAvailable();
  void addParamAndValue(FileType* filetype, const UnicodeString& name, const UnicodeString& value,
                        const FileType* def_filetype = nullptr);

 private:
  /** add current active editor and return him. */
  FarEditor* addCurrentEditor();
  /** Returns currently active editor. */
  FarEditor* getCurrentEditor();
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
  static const wchar_t* GetMsg(int msg);
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

  bool consoleAnnotationAvailable;

  static void showExceptionMessage(const UnicodeString* message);
  int getCountFileTypeAndGroup();
  FileType* getFileTypeByIndex(int idx);
  void FillTypeMenu(ChooseTypeMenu* Menu, FileType* CurFileType);
  UnicodeString* getCurrentFileName();

  // FarList for dialog objects
  FarList* buildHrcList();
  FarList* buildParamsList(FileType* type);
  // filetype "default"
  FileType* defaultType = nullptr;
  // change combobox type
  void ChangeParamValueListType(HANDLE hDlg, bool dropdownlist);
  // set list of values to combobox
  void setCrossValueListToCombobox(FileType* type, HANDLE hDlg);
  void setCrossPosValueListToCombobox(FileType* type, HANDLE hDlg);
  void setYNListValueToCombobox(FileType* type, HANDLE hDlg, UnicodeString param);
  void setTFListValueToCombobox(FileType* type, HANDLE hDlg, UnicodeString param);
  void setCustomListValueToCombobox(FileType* type, HANDLE hDlg, UnicodeString param);

  FileType* getCurrentTypeInDialog(HANDLE hDlg);

  const UnicodeString* getParamDefValue(FileType* type, UnicodeString param);

  void SaveChangedValueParam(HANDLE hDlg);

  std::unordered_map<intptr_t, FarEditor*> farEditorInstances;
  std::unique_ptr<ParserFactory> parserFactory;
  std::unique_ptr<StyledHRDMapper> regionMapper;

  /**current value*/
  UnicodeString hrdClass;
  UnicodeString hrdName;

  /** registry settings */
  bool rEnabled;  // status plugin
  int drawCross;
  bool drawPairs;
  bool drawSyntax;
  bool oldOutline;
  bool TrueModOn;
  bool ChangeBgEditor;
  UnicodeString* sHrdName;
  UnicodeString* sHrdNameTm;
  UnicodeString* sCatalogPath;
  UnicodeString* sUserHrdPath;
  UnicodeString* sUserHrcPath;

  /** UNC path */
  std::unique_ptr<UnicodeString> sCatalogPathExp;
  std::unique_ptr<UnicodeString> sUserHrdPathExp;
  std::unique_ptr<UnicodeString> sUserHrcPathExp;

  int viewFirst;  // 0 - init;  1 - first run view; 2 - first run editor
  std::string settingsIni;
};

#endif

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
