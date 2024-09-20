#include "pcolorer.h"
#include <utils.h>
#include "FarEditorSet.h"
#include "CerrLogger.h"

FarEditorSet* editorSet = nullptr;
bool inEventProcess = false;
PluginStartupInfo Info;
FarStandardFunctions FSF;
UnicodeString* PluginPath = nullptr;

std::unique_ptr<CerrLogger> logger;

SHAREDSYMBOL void PluginModuleOpen(const char* path)
{
  UnicodeString module(path);
  int pos = module.lastIndexOf('/');
  pos = module.lastIndexOf('/', pos);
  PluginPath = new UnicodeString(UnicodeString(module, 0, pos));

  logger = std::make_unique<CerrLogger>();
  Log::registerLogger(*logger);
}

UnicodeString* GetConfigPath(const UnicodeString& sub)
{
  struct stat s;
  UnicodeString* path = new UnicodeString(*PluginPath);
  path->append(sub);
  if (stat(path->getChars(), &s) == -1) {
    std::wstring str(path->getWChars());
    if (TranslateInstallPath_Lib2Share(str)) {
      path->setLength(0);
      path->append(str.c_str());
    }
  }
  return path;
}

/**
  Returns message from FAR current language.
*/
const wchar_t* GetMsg(int msg)
{
  return (Info.GetMsg(Info.ModuleNumber, msg));
};

/**
  Plugin initialization and creation of editor set support class.
*/
SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo* fei)
{
  Info = *fei;
  FSF = *fei->FSF;
  Info.FSF = &FSF;

  editorSet = nullptr;
  inEventProcess = false;
}

/**
  Plugin strings in FAR interface.
*/
SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo* nInfo)
{
  static wchar_t* PluginMenuStrings;
  memset(nInfo, 0, sizeof(*nInfo));
  nInfo->Flags = PF_EDITOR | PF_DISABLEPANELS;
  nInfo->StructSize = sizeof(*nInfo);
  nInfo->PluginConfigStringsNumber = 1;
  nInfo->PluginMenuStringsNumber = 1;
  PluginMenuStrings = (wchar_t*) GetMsg(mName);
  nInfo->PluginConfigStrings = &PluginMenuStrings;
  nInfo->PluginMenuStrings = &PluginMenuStrings;
}

/**
  On FAR exit. Destroys all internal structures.
*/
SHAREDSYMBOL void WINAPI ExitFARW()
{
  delete editorSet;
}

/**
  Open plugin configuration of actions dialog.
*/
SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
  if (OpenFrom == OPEN_EDITOR) {
    editorSet->openMenu();
  }

  return INVALID_HANDLE_VALUE;
}

/**
  Configures plugin.
*/
SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
  if (!editorSet) {
    editorSet = new FarEditorSet();
  }
  else {
    // ReadSettings need for plugin off mode
    editorSet->ReadSettings();
  }
  editorSet->configure(false);
  return 1;
}

/**
  Processes FAR editor events and
  makes text colorizing here.
*/
SHAREDSYMBOL int WINAPI ProcessEditorEventW(int Event, void* Param)
{
  if (inEventProcess) {
    return 0;
  }

  inEventProcess = true;

  if (!editorSet) {
    editorSet = new FarEditorSet();
  }

  int result = editorSet->editorEvent(Event, Param);

  inEventProcess = false;
  return result;
}

SHAREDSYMBOL int WINAPI ProcessEditorInputW(const INPUT_RECORD* ir)
{
  if (inEventProcess) {
    return 0;
  }

  inEventProcess = true;
  if (!editorSet) {
    editorSet = new FarEditorSet();
  }

  int result = editorSet->editorInput(ir);

  inEventProcess = false;
  return result;
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
