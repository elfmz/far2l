#if 0
#include <g3log/g3log.hpp>
#endif // #if 0
#include <iostream>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include"pcolorer.h"
#include"tools.h"
#include"FarEditorSet.h"
#include <utils.h>

#ifdef USESPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

std::shared_ptr<spdlog::logger> logger;
#else
std::shared_ptr<DummyLogger> logger;
#endif

XERCES_CPP_NAMESPACE_USE

FarEditorSet *editorSet = nullptr;
PluginStartupInfo Info;
FarStandardFunctions FSF;
UnicodeString *PluginPath = nullptr;
#if 0
std::unique_ptr<g3::LogWorker> logworker;
#endif // #if 0

// ---------------------------------------------------------------------------
// This is a simple class that lets us do easy (though not terribly efficient)
// trancoding of XMLCh data to local code page for display.
// ---------------------------------------------------------------------------
class StrX
{
  public:
    // -----------------------------------------------------------------------
    // Constructors and Destructor
    // -----------------------------------------------------------------------
    StrX(const XMLCh* const origin): fLocalForm(nullptr)
    {
      // Call the private transcoding method
      if (origin) fLocalForm = XMLString::transcode(origin);
    }

    ~StrX()
    {
      if (fLocalForm) XMLString::release(&fLocalForm);
    }

    // -----------------------------------------------------------------------
    // Getter methods
    // -----------------------------------------------------------------------
    const char* operator()() const
    {
      return fLocalForm;
    }

  private:
    // -----------------------------------------------------------------------
    // Private data members
    //
    //  fLocalForm
    //      This is the local code page form of the string.
    // -----------------------------------------------------------------------
    char* fLocalForm;
};

SHAREDSYMBOL void PluginModuleOpen(const char *path)
{
      UnicodeString module(path);
      int pos = module.lastIndexOf('/');
      pos = module.lastIndexOf('/',pos);
      PluginPath=new UnicodeString(UnicodeString(module, 0, pos));
#ifdef USESPDLOG
      logger = spdlog::stderr_logger_mt("far2l-colorer");
#else
      logger = std::make_shared<DummyLogger>();
#endif
}

UnicodeString *GetConfigPath(const UnicodeString &sub)
{
  struct stat s;
  UnicodeString *path=new UnicodeString(*PluginPath);
  path->append(sub);
  if (stat(path->getChars(), &s) == -1) {
          std::wstring str(path->getWChars());
          if (TranslateInstallPath_Lib2Share(str) ) {
            path->setLength(0);
            path->append(str.c_str());
          }
  }
  return path;
}


//todo:
/**
  Returns message from FAR current language.
*/
const wchar_t *GetMsg(int msg)
{
  return(Info.GetMsg(Info.ModuleNumber, msg));
};

/**
  Plugin initialization and creation of editor set support class.
*/
SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *fei)
{
  Info = *fei;
  FSF = *fei->FSF;
  Info.FSF = &FSF;
#if 0
  logworker = g3::LogWorker::createLogWorker();
  g3::initializeLogging(logworker.get());
#ifndef NDEBUG
  g3::only_change_at_initialization::setLogLevel(DEBUG, false);
  g3::only_change_at_initialization::setLogLevel(INFO, false);
#endif
#endif // #if 0
  XMLPlatformUtils::Initialize();
};

/**
  Plugin strings in FAR interface.
*/
SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *nInfo)
{
  static wchar_t* PluginMenuStrings;
  memset(nInfo, 0, sizeof(*nInfo));
  nInfo->Flags = PF_EDITOR | PF_DISABLEPANELS;
  nInfo->StructSize = sizeof(*nInfo);
  nInfo->PluginConfigStringsNumber = 1;
  nInfo->PluginMenuStringsNumber = 1;
  PluginMenuStrings = (wchar_t*)GetMsg(mName);
  nInfo->PluginConfigStrings = &PluginMenuStrings;
  nInfo->PluginMenuStrings = &PluginMenuStrings;
  nInfo->CommandPrefix = L"clr";
};

/**
  On FAR exit. Destroys all internal structures.
*/
SHAREDSYMBOL void WINAPI ExitFARW()
{
  if (editorSet){
    delete editorSet;
  }
  XMLPlatformUtils::Terminate();
#if 0
  g3::internal::shutDownLogging();
#endif // #if 0
};

/**
  Open plugin configuration of actions dialog.
*/
SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
  if (OpenFrom == OPEN_EDITOR){
    editorSet->openMenu();
  }
  else
    if (OpenFrom == OPEN_COMMANDLINE){
      //file name, which we received
      wchar_t *file = (wchar_t*)Item;

      wchar_t *nfile = PathToFull(file,true);
      if (nfile){
        if (!editorSet){
          editorSet = new FarEditorSet();
        }
        editorSet->viewFile(UnicodeString(nfile));
      }

      delete[] nfile;
    }

  return INVALID_HANDLE_VALUE;
};

/**
  Configures plugin.
*/
SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
  if (!editorSet){
    editorSet = new FarEditorSet();
  }else{
  // ReadSettings need for plugin off mode
		editorSet->ReadSettings();
	}
  editorSet->configure(false);
  return 1;
};

/**
  Processes FAR editor events and
  makes text colorizing here.
*/
SHAREDSYMBOL int WINAPI ProcessEditorEventW(int Event, void *Param)
{
  if (!editorSet){
    editorSet = new FarEditorSet();
  }
  return editorSet->editorEvent(Event, Param);
};

SHAREDSYMBOL int WINAPI ProcessEditorInputW(const INPUT_RECORD *ir)
{
  return editorSet->editorInput(ir);
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
