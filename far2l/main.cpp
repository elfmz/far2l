/*
main.cpp

Функция main.
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#pragma hdrstop

#include "lang.hpp"
#include "keys.hpp"
#include "chgprior.hpp"
#include "colors.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "fileedit.hpp"
#include "fileview.hpp"
#include "lockscrn.hpp"
#include "hilight.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "language.hpp"
#include "syslog.hpp"
#include "registry.hpp"
//#include "localOEM.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "clipboard.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "dirmix.hpp"
#include "cmdline.hpp"
#include "console.hpp"
#include <string>
#include <sys/stat.h>
#include <locale.h>

#ifdef DIRECT_RT
int DirectRT=0;
#endif

static void CopyGlobalSettings();

static void show_help()
{
	WCHAR HelpMsg[]=
	    L"Usage: far [switches] [apath [ppath]]\n\n"
	    L"where\n"
	    L"  apath - path to a folder (or a file or an archive or command with prefix)\n"
	    L"          for the active panel\n"
	    L"  ppath - path to a folder (or a file or an archive or command with prefix)\n"
	    L"          for the passive panel\n\n"
	    L"The following switches may be used in the command line:\n\n"
	    L" /?   This help.\n"
	    L" /a   Disable display of characters with codes 0 - 31 and 255.\n"
	    L" /ag  Disable display of pseudographics characters.\n"
	    L" /co  Forces FAR to load plugins from the cache only.\n"
#ifdef DIRECT_RT
	    L" /do  Direct output.\n"
#endif
	    L" /e[<line>[:<pos>]] <filename>\n"
	    L"      Edit the specified file.\n"
	    L" /i   Set icon for FAR console window.\n"
	    L" /m   Do not load macros.\n"
	    L" /ma  Do not execute auto run macros.\n"
	    L" /p[<path>]\n"
	    L"      Search for \"common\" plugins in the directory, specified by <path>.\n"
	    L" /u <username>\n"
	    L"      Allows to have separate settings for different users.\n"
	    L" /v <filename>\n"
	    L"      View the specified file. If <filename> is -, data is read from the stdin.\n"
	    L" /w   Stretch to console window instead of console buffer.\n"
	    L" /x   Disable exception handling.\n"
#ifdef _DEBUGEXC
	    L" /xd  Enable exception handling.\n"
#endif
		;
	Console.Write(HelpMsg, ARRAYSIZE(HelpMsg)-1);
}

static int MainProcess(
    const wchar_t *lpwszEditName,
    const wchar_t *lpwszViewName,
    const wchar_t *lpwszDestName1,
    const wchar_t *lpwszDestName2,
    int StartLine,
    int StartChar
)
{
	{
		ChangePriority ChPriority(ChangePriority::NORMAL);
		ControlObject CtrlObj;
		WORD InitAttributes=0;
		Console.GetTextAttributes(InitAttributes);
		SetRealColor(COL_COMMANDLINEUSERSCREEN);

		if (*lpwszEditName || *lpwszViewName)
		{
			Opt.OnlyEditorViewerUsed=1;
			Panel *DummyPanel=new Panel;
			_tran(SysLog(L"create dummy panels"));
			CtrlObj.CreateFilePanels();
			CtrlObj.Cp()->LeftPanel=CtrlObj.Cp()->RightPanel=CtrlObj.Cp()->ActivePanel=DummyPanel;
			CtrlObj.Plugins.LoadPlugins();
			CtrlObj.Macro.LoadMacros(TRUE,FALSE);

			if (*lpwszEditName)
			{
				FileEditor *ShellEditor=new FileEditor(lpwszEditName,CP_AUTODETECT,FFILEEDIT_CANNEWFILE|FFILEEDIT_ENABLEF6,StartLine,StartChar);
				_tran(SysLog(L"make shelleditor %p",ShellEditor));

				if (!ShellEditor->GetExitCode())  // ????????????
				{
					FrameManager->ExitMainLoop(0);
				}
			}
			// TODO: Этот else убрать только после разборок с возможностью задавать несколько /e и /v в ком.строке
			else if (*lpwszViewName)
			{
				FileViewer *ShellViewer=new FileViewer(lpwszViewName,FALSE);

				if (!ShellViewer->GetExitCode())
				{
					FrameManager->ExitMainLoop(0);
				}

				_tran(SysLog(L"make shellviewer, %p",ShellViewer));
			}

			FrameManager->EnterMainLoop();
			CtrlObj.Cp()->LeftPanel=CtrlObj.Cp()->RightPanel=CtrlObj.Cp()->ActivePanel=nullptr;
			delete DummyPanel;
			_tran(SysLog(L"editor/viewer closed, delete dummy panels"));
		}
		else
		{
			Opt.OnlyEditorViewerUsed=0;
			Opt.SetupArgv=0;
			FARString strPath;

			// воспользуемся тем, что ControlObject::Init() создает панели
			// юзая Opt.*
			if (*lpwszDestName1)  // актиная панель
			{
				Opt.SetupArgv++;
				strPath = lpwszDestName1;
				CutToNameUNC(strPath);
				DeleteEndSlash(strPath); //BUGBUG!! если конечный слешь не убрать - получаем забавный эффект - отсутствует ".."

//				if ((strPath.At(1)==L':' && !strPath.At(2)) || (HasPathPrefix(strPath) && strPath.At(5)==L':' && !strPath.At(6)))
//					AddEndSlash(strPath);

				// Та панель, которая имеет фокус - активна (начнем по традиции с Левой Панели ;-)
				if (Opt.LeftPanel.Focus)
				{
					Opt.LeftPanel.Type=FILE_PANEL;  // сменим моду панели
					Opt.LeftPanel.Visible=TRUE;     // и включим ее
					Opt.strLeftFolder = strPath;
				}
				else
				{
					Opt.RightPanel.Type=FILE_PANEL;
					Opt.RightPanel.Visible=TRUE;
					Opt.strRightFolder = strPath;
				}

				if (*lpwszDestName2)  // пассивная панель
				{
					Opt.SetupArgv++;
					strPath = lpwszDestName2;
					CutToNameUNC(strPath);
					DeleteEndSlash(strPath); //BUGBUG!! если конечный слешь не убрать - получаем забавный эффект - отсутствует ".."

//					if ((strPath.At(1)==L':' && !strPath.At(2)) || (HasPathPrefix(strPath) && strPath.At(5)==L':' && !strPath.At(6)))
//						AddEndSlash(strPath);

					// а здесь с точнотью наоборот - обрабатываем пассивную панель
					if (Opt.LeftPanel.Focus)
					{
						Opt.RightPanel.Type=FILE_PANEL; // сменим моду панели
						Opt.RightPanel.Visible=TRUE;    // и включим ее
						Opt.strRightFolder = strPath;
					}
					else
					{
						Opt.LeftPanel.Type=FILE_PANEL;
						Opt.LeftPanel.Visible=TRUE;
						Opt.strLeftFolder = strPath;
					}
				}
			}

			// теперь все готово - создаем панели!
			CtrlObj.Init();

			// а теперь "провалимся" в каталог или хост-файл (если получится ;-)
			if (*lpwszDestName1)  // актиная панель
			{
				FARString strCurDir;
				Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
				Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

				if (*lpwszDestName2)  // пассивная панель
				{
					AnotherPanel->GetCurDir(strCurDir);
					FarChDir(strCurDir);

					if (IsPluginPrefixPath(lpwszDestName2))
					{
						AnotherPanel->SetFocus();
						CtrlObject->CmdLine->ExecString(lpwszDestName2,0);
						ActivePanel->SetFocus();
					}
					else
					{
						strPath = PointToNameUNC(lpwszDestName2);

						if (!strPath.IsEmpty())
						{
							if (AnotherPanel->GoToFile(strPath))
								AnotherPanel->ProcessKey(KEY_CTRLPGDN);
						}
					}
				}

				ActivePanel->GetCurDir(strCurDir);
				FarChDir(strCurDir);

				if (IsPluginPrefixPath(lpwszDestName1))
				{
					CtrlObject->CmdLine->ExecString(lpwszDestName1,0);
				}
				else
				{
					strPath = PointToNameUNC(lpwszDestName1);

					if (!strPath.IsEmpty())
					{
						if (ActivePanel->GoToFile(strPath))
							ActivePanel->ProcessKey(KEY_CTRLPGDN);
					}
				}

				// !!! ВНИМАНИЕ !!!
				// Сначала редравим пассивную панель, а потом активную!
				AnotherPanel->Redraw();
				ActivePanel->Redraw();
			}

			FrameManager->EnterMainLoop();
		}

		// очистим за собой!
		SetScreen(0,0,ScrX,ScrY,L' ',COL_COMMANDLINEUSERSCREEN);
		Console.SetTextAttributes(InitAttributes);
		ScrBuf.ResetShadow();
		ScrBuf.Flush();
		MoveRealCursor(0,0);
	}
	CloseConsole();
	return 0;
}

int MainProcessSEH(FARString& strEditName,FARString& strViewName,FARString& DestName1,FARString& DestName2,int StartLine,int StartChar)
{
	int Result=0;
	Result=MainProcess(strEditName,strViewName,DestName1,DestName2,StartLine,StartChar);
	return Result;
}

int FarAppMain(int argc, char **argv)
{
	InitCurrentDirectory();
	//todo if (apiGetModuleFileName(nullptr, g_strFarModuleName)) todo
	{
		apiGetCurrentDirectory(g_strFarModuleName);
		if (argv[0][0]!=GOOD_SLASH) {
			if (argv[0][0]=='.') {
				g_strFarModuleName+= argv[0]+1;
			} else {
				g_strFarModuleName+= GOOD_SLASH;
				g_strFarModuleName+= argv[0];
			}
		} else
			g_strFarModuleName = argv[0];
		fprintf(stderr, "g_strFarModuleName=%ls\n", g_strFarModuleName.CPtr());
		PrepareDiskPath(g_strFarModuleName);
	}
	Opt.IsUserAdmin = (geteuid()==0);

	_OT(SysLog(L"[[[[[[[[New Session of FAR]]]]]]]]]"));
	FARString strEditName;
	FARString strViewName;
	FARString DestNames[2];
	int StartLine=-1,StartChar=-1;
	int CntDestName=0; // количество параметров-имен каталогов
	/*$ 18.04.2002 SKV
	  Попользуем floating point что бы проинициализировался vc-ный fprtl.
	*/
#ifdef _MSC_VER
	float x=1.1f;
	wchar_t buf[15];
	swprintf(buf,L"%f",x);
#endif
	// если под дебагером, то отключаем исключения однозначно,
	//  иначе - смотря что указал юзвер.
#if defined(_DEBUGEXC)
	Opt.ExceptRules=-1;
#else
	Opt.ExceptRules=-1;//IsDebuggerPresent()?0:-1;
#endif
//  Opt.ExceptRules=-1;

#ifdef __GNUC__
	Opt.ExceptRules=0;
#endif

//_SVS(SysLog(L"Opt.ExceptRules=%d",Opt.ExceptRules));
	SetRegRootKey(HKEY_CURRENT_USER);
	Opt.strRegRoot = L"Software/Far2";
	// По умолчанию - брать плагины из основного каталога
	Opt.LoadPlug.MainPluginDir=TRUE;
	Opt.LoadPlug.PluginsPersonal=TRUE;
	Opt.LoadPlug.PluginsCacheOnly=FALSE;

	g_strFarPath=g_strFarModuleName;
	CutToSlash(g_strFarPath,true);
	WINPORT(SetEnvironmentVariable)(L"FARHOME", g_strFarPath);
	AddEndSlash(g_strFarPath);

	// don't inherit from parent process in any case
	WINPORT(SetEnvironmentVariable)(L"FARUSER", nullptr);

	WINPORT(SetEnvironmentVariable)(L"FARADMINMODE", Opt.IsUserAdmin?L"1":nullptr);

	// макросы не дисаблим
	Opt.Macro.DisableMacro=0;
	for (int I=1; I<argc; I++)
	{
		std::wstring arg_w = MB2Wide(argv[I]);
		if ((arg_w[0]==L'/' || arg_w[0]==L'-') && arg_w[1])
		{
			switch (Upper(arg_w[1]))
			{
				case L'A':

					switch (Upper(arg_w[2]))
					{
						case 0:
							Opt.CleanAscii=TRUE;
							break;
						case L'G':

							if (!arg_w[3])
								Opt.NoGraphics=TRUE;

							break;
					}

					break;
				case L'E':

					if (iswdigit(arg_w[2]))
					{
						StartLine=_wtoi((const wchar_t *)&arg_w[2]);
						wchar_t *ChPtr=wcschr((wchar_t *)&arg_w[2],L':');

						if (ChPtr)
							StartChar=_wtoi(ChPtr+1);
					}

					if (I+1<argc)
					{
						strEditName = argv[I+1];
						I++;
					}

					break;
				case L'V':

					if (I+1<argc)
					{
						strViewName = argv[I+1];
						I++;
					}

					break;
				case L'M':

					switch (Upper(arg_w[2]))
					{
						case 0:
							Opt.Macro.DisableMacro|=MDOL_ALL;
							break;
						case L'A':

							if (!arg_w[3])
								Opt.Macro.DisableMacro|=MDOL_AUTOSTART;

							break;
					}

					break;
				case L'I':
					Opt.SmallIcon=TRUE;
					break;
				case L'X':
					Opt.ExceptRules=0;
#if defined(_DEBUGEXC)

					if (Upper(arg_w[2])==L'D' && !arg_w[3])
						Opt.ExceptRules=1;

#endif
					break;

				case L'C':

					if (Upper(arg_w[2])==L'O' && !arg_w[3])
					{
						Opt.LoadPlug.PluginsCacheOnly=TRUE;
						Opt.LoadPlug.PluginsPersonal=FALSE;
					}

					break;
				case L'?':
				case L'H':
					ControlObject::ShowCopyright(1);
					show_help();
					return 0;
#ifdef DIRECT_RT
				case L'D':

					if (Upper(arg_w[2])==L'O' && !arg_w[3])
						DirectRT=1;

					break;
#endif
				case L'W':
					{
						Opt.WindowMode=TRUE;
					}
					break;
			}
		}
		else // простые параметры. Их может быть max две штукА.
		{
			if (CntDestName < 2)
			{
				if (IsPluginPrefixPath((const wchar_t *)arg_w.c_str()))
				{
					DestNames[CntDestName++] = (const wchar_t *)arg_w.c_str();
				}
				else
				{
					apiExpandEnvironmentStrings((const wchar_t *)arg_w.c_str(), DestNames[CntDestName]);
					Unquote(DestNames[CntDestName]);
					ConvertNameToFull(DestNames[CntDestName],DestNames[CntDestName]);

					if (apiGetFileAttributes(DestNames[CntDestName]) != INVALID_FILE_ATTRIBUTES)
						CntDestName++; //???
				}
			}
		}
	}

	//Настройка OEM сортировки. Должна быть после CopyGlobalSettings и перед InitKeysArray!
	//LocalUpperInit();
	//InitLCIDSort();
	//Инициализация массива клавиш. Должна быть после CopyGlobalSettings!
	InitKeysArray();
	//WaitForInputIdle(GetCurrentProcess(),0);
	std::set_new_handler(nullptr);

	if (!Opt.LoadPlug.MainPluginDir) //если есть ключ /p то он отменяет /co
		Opt.LoadPlug.PluginsCacheOnly=FALSE;

	if (Opt.LoadPlug.PluginsCacheOnly)
	{
		Opt.LoadPlug.strCustomPluginsPath.Clear();
		Opt.LoadPlug.MainPluginDir=FALSE;
		Opt.LoadPlug.PluginsPersonal=FALSE;
	}

	InitConsole();
	GetRegKey(L"Language",L"Main",Opt.strLanguage,L"English");

	if (!Lang.Init(g_strFarPath,true,MNewFileName))
	{
		ControlObject::ShowCopyright(1);
		LPCWSTR LngMsg;
		switch(Lang.GetLastError())
		{
		case LERROR_BAD_FILE:
			LngMsg = L"\nError: language data is incorrect or damaged.\n\nPress any key to exit...";
			break;
		case LERROR_FILE_NOT_FOUND:
			LngMsg = L"\nError: cannot find language data.\n\nPress any key to exit...";
			break;
		default:
			LngMsg = L"\nError: cannot load language data.\n\nPress any key to exit...";
			break;
		}
		Console.Write(LngMsg,StrLength(LngMsg));
		Console.FlushInputBuffer();
		WaitKey(); // А стоит ли ожидать клавишу??? Стоит
		return 1;
	}

	WINPORT(SetEnvironmentVariable)(L"FARLANG",Opt.strLanguage);
	SetHighlighting();
	initMacroVarTable(1);

	if (Opt.ExceptRules == -1)
	{
		GetRegKey(L"System",L"ExceptRules",Opt.ExceptRules,1);
	}

	//ErrorMode=SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX|(Opt.ExceptRules?SEM_NOGPFAULTERRORBOX:0)|(GetRegKey(L"System/Exception", L"IgnoreDataAlignmentFaults", 0)?SEM_NOALIGNMENTFAULTEXCEPT:0);
	//SetErrorMode(ErrorMode);

	int Result=MainProcessSEH(strEditName,strViewName,DestNames[0],DestNames[1],StartLine,StartChar);

	EmptyInternalClipboard();
	doneMacroVarTable(1);
	_OT(SysLog(L"[[[[[Exit of FAR]]]]]]]]]"));
	return Result;
}

int _cdecl main(int Argc, char *Argv[])
{
	setlocale(LC_ALL, "");//otherwise non-latin keys missing with XIM input method
	
	apiEnableLowFragmentationHeap();
	return WinPortMain(Argc, Argv, FarAppMain);
}


/* $ 03.08.2000 SVS
  ! Не срабатывал шаблон поиска файлов для под-юзеров
*/
void CopyGlobalSettings()
{
	if (CheckRegKey(L"")) // при существующем - вываливаемся
		return;

	// такого извера нету - перенесем данные!
	SetRegRootKey(HKEY_LOCAL_MACHINE);
	CopyKeyTree(L"Software/Far2",Opt.strRegRoot,L"Software/Far2/Users\0");
	SetRegRootKey(HKEY_CURRENT_USER);
	CopyKeyTree(L"Software/Far2",Opt.strRegRoot,L"Software/Far2/Users/Software/Far2/PluginsCache\0");
	//  "Вспомним" путь по шаблону!!!
	SetRegRootKey(HKEY_LOCAL_MACHINE);
	GetRegKey(L"System",L"TemplatePluginsPath",Opt.LoadPlug.strPersonalPluginsPath,L"");
	// удалим!!!
	DeleteRegKey(L"System");
	// запишем новое значение!
	SetRegRootKey(HKEY_CURRENT_USER);
	SetRegKey(L"System",L"PersonalPluginsPath",Opt.LoadPlug.strPersonalPluginsPath);
}
