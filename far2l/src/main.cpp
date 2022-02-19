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
#include <sys/ioctl.h>
#include <signal.h>

#include "lang.hpp"
#include "keys.hpp"
#include "chgprior.hpp"
#include "colors.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "fileedit.hpp"
#include "fileview.hpp"
#include "exitcode.hpp"
#include "lockscrn.hpp"
#include "hilight.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "language.hpp"
#include "syslog.hpp"
//#include "localOEM.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "clipboard.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "dirmix.hpp"
#include "Bookmarks.hpp"
#include "cmdline.hpp"
#include "console.hpp"
#include "vtshell.h"
#include "execute.hpp"
#include "mix.hpp"
#include <string>
#include <sys/stat.h>
#include <locale.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include "InterThreadCall.hpp"
#include "SafeMMap.hpp"
#include "ConfigRW.hpp"

#ifdef DIRECT_RT
int DirectRT=0;
#endif

static void print_help(const char *self)
{
	printf( "FAR2L - oldschool file manager, with built-in terminal and other usefullness'es\n"
		"Usage: %s [switches] [-cd apath [-cd ppath]]\n\n"
		"where\n"
		"  apath - path to a folder (or a file or an archive or command with prefix)\n"
		"          for the active panel\n"
		"  ppath - path to a folder (or a file or an archive or command with prefix)\n"
		"          for the passive panel\n\n"
		"The following switches may be used in the command line:\n\n"
		" -h   This help.\n"
		" -a   Disable display of characters with codes 0 - 31 and 255.\n"
		" -ag  Disable display of pseudographics with codes > 127.\n"
		" -an  Disable display of pseudographics characters completely.\n"
		" -co  Forces FAR to load plugins from the cache only.\n"
		" -cd <path> Change panel's directory to specified path.\n"
		" -m   Do not load macros.\n"
		" -ma  Do not execute auto run macros.\n"
//		" -p[<path>]\n"
//		"      Search for \"common\" plugins in the directory, specified by <path>.\n"
		" -u <identity> OR </path/name>\n"
		"      Allows to specify separate settings identity or FS location.\n"
		" -v <filename>\n"
		"      View the specified file.\n"
		" -v - command line\n"
		"      Executes given command line and opens viewer with its output.\n"
		" -e[<line>[:<pos>]] <filename>\n"
		"      Edit the specified file with optional cursor position specification.\n"
		" -e[<line>[:<pos>]] - command line\n"
		"      Executes given command line and opens editor with its output.\n"
		"\n",
		self);
	WinPortHelp();
	//Console.Write(HelpMsg, ARRAYSIZE(HelpMsg)-1);
}

static FARString ReconstructCommandLine(int argc, char **argv)
{
	FARString cmd;
	for (;argc; --argc, ++argv) {
		if (*argv) {
			if (cmd.GetLength()) {
				cmd+= L" ";
			}
			std::string arg = *argv;
			QuoteCmdArg(arg);
			cmd+= arg;
		}
	}
	return cmd;
}

static FARString ExecuteCommandAndGrabItsOutput(FARString cmd)
{

	FARString strTempName;

	if (!FarMkTempEx(strTempName))
		return FARString();

	std::string exec_cmd = "echo Waiting command to complete...; " \
		"echo You can use Ctrl+C to stop it, or Ctrl+Alt+C - to hardly terminate.; ";
	if (cmd.GetLength() != 0)
	{
		exec_cmd+= cmd.GetMB();
	}
	else
	{
		exec_cmd+= "far2l -h";
	}

	exec_cmd+= " >";
	exec_cmd+= strTempName.GetMB();
	exec_cmd+= " 2>&1";

	farExecuteA(exec_cmd.c_str(), EF_NOCMDPRINT);

	return strTempName;
}


static int MainProcess(
    FARString strEditViewArg,
    FARString strDestName1,
    FARString strDestName2,
    int StartLine,
    int StartChar
)
{
	InterThreadCallsDispatcherThread itc_dispatcher_thread;

	{
		clock_t cl_start = clock();
		ChangePriority ChPriority(ChangePriority::NORMAL);
		ControlObject CtrlObj;
		WORD InitAttributes=0;
		Console.GetTextAttributes(InitAttributes);
		SetColor(COL_COMMANDLINEUSERSCREEN, true);

		if (Opt.OnlyEditorViewerUsed != Options::NOT_ONLY_EDITOR_VIEWER)
		{
			Panel *DummyPanel=new Panel;
			_tran(SysLog(L"create dummy panels"));
			CtrlObj.CreateFilePanels();
			CtrlObj.Cp()->LeftPanel=CtrlObj.Cp()->RightPanel=CtrlObj.Cp()->ActivePanel=DummyPanel;
			CtrlObj.Plugins.LoadPlugins();
			CtrlObj.Macro.LoadMacros(TRUE,FALSE);

			if (Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR_ON_CMDOUT || Opt.OnlyEditorViewerUsed == Options::ONLY_VIEWER_ON_CMDOUT)
			{
				strEditViewArg = ExecuteCommandAndGrabItsOutput(strEditViewArg);
			}

			if (Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR || Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR_ON_CMDOUT)
			{
				FileEditor *ShellEditor=new FileEditor(strEditViewArg,CP_AUTODETECT,FFILEEDIT_CANNEWFILE|FFILEEDIT_ENABLEF6,StartLine,StartChar);
				_tran(SysLog(L"make shelleditor %p",ShellEditor));

				if (!ShellEditor->GetExitCode())  // ????????????
				{
					FrameManager->ExitMainLoop(0);
				}
			}
			else
			{
				FileViewer *ShellViewer=new FileViewer(strEditViewArg,FALSE);

				if (!ShellViewer->GetExitCode())
				{
					FrameManager->ExitMainLoop(0);
				}

				_tran(SysLog(L"make shellviewer, %p",ShellViewer));
			}

			fprintf(stderr, "STARTUP(E/V): %llu\n", (unsigned long long)(clock() - cl_start) );
			FrameManager->EnterMainLoop();

			if (Opt.OnlyEditorViewerUsed == Options::ONLY_VIEWER_ON_CMDOUT
			 || Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR_ON_CMDOUT)
			{
				unlink(strEditViewArg.GetMB().c_str());
			}

			CtrlObj.Cp()->LeftPanel=CtrlObj.Cp()->RightPanel=CtrlObj.Cp()->ActivePanel=nullptr;
			delete DummyPanel;
			_tran(SysLog(L"editor/viewer closed, delete dummy panels"));
		}
		else
		{
			Opt.OnlyEditorViewerUsed=Options::NOT_ONLY_EDITOR_VIEWER;
			Opt.SetupArgv=0;
			FARString strPath;

			// воспользуемся тем, что ControlObject::Init() создает панели
			// юзая Opt.*
			if (strDestName1.GetLength())  // актиная панель
			{
				Opt.SetupArgv++;
				strPath = strDestName1;

				if (strPath != "/") {
					DeleteEndSlash(strPath); //BUGBUG!! если конечный слешь не убрать - получаем забавный эффект - отсутствует ".."
				}

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

				if (strDestName2.GetLength())  // пассивная панель
				{
					Opt.SetupArgv++;
					strPath = strDestName2;

					if (strPath != "/") {
						DeleteEndSlash(strPath); //BUGBUG!! если конечный слешь не убрать - получаем забавный эффект - отсутствует ".."
					}

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
			if (strDestName1.GetLength())  // актиная панель
			{
				FARString strCurDir;
				Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
				Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

				if (strDestName2)  // пассивная панель
				{
					AnotherPanel->GetCurDir(strCurDir);
					FarChDir(strCurDir);

					if (IsPluginPrefixPath(strDestName2))
					{
						AnotherPanel->SetFocus();
						CtrlObject->CmdLine->ExecString(strDestName2,0);
						ActivePanel->SetFocus();
					}
					else
					{
						strPath = strDestName2;

						if (!strPath.IsEmpty())
						{
							if (AnotherPanel->GoToFile(strPath))
								AnotherPanel->ProcessKey(KEY_CTRLPGDN);
						}
					}
				}

				ActivePanel->GetCurDir(strCurDir);
				FarChDir(strCurDir);

				if (IsPluginPrefixPath(strDestName1))
				{
					CtrlObject->CmdLine->ExecString(strDestName1,0);
				}
				else
				{
					strPath = strDestName1;

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

			fprintf(stderr, "STARTUP: %llu\n", (unsigned long long)(clock() - cl_start) );
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

static void SetupFarPath(int argc, char **argv)
{
	InMyTemp(); // pre-cache in env temp pathes
	InitCurrentDirectory();

	char buf[PATH_MAX + 1] = {};
	ssize_t buf_sz = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (buf_sz > 0 && buf_sz < (ssize_t)sizeof(buf) - 1 && buf[0] == GOOD_SLASH) {
		buf[buf_sz] = 0;
		g_strFarModuleName = buf;

	} else {
		g_strFarModuleName = LookupExecutable(argv[0]);
	}

	FARString dir = g_strFarModuleName;
	CutToSlash(dir, true);
	const wchar_t *last_element = PointToName(dir);
	if (last_element && wcscmp(last_element, L"bin") == 0) {
		CutToSlash(dir, false);
		SetPathTranslationPrefix(dir);
	}

	PrepareDiskPath(g_strFarModuleName);
}

static unsigned int gMainThreadID;

int FarAppMain(int argc, char **argv)
{
	// avoid killing process due to broker terminated unexpectedly
	signal(SIGPIPE, SIG_IGN);

	fprintf(stderr, "argv[0]='%s' g_strFarModuleName='%ls' translation_prefix='%ls' temp='%s' config='%s'\n", 
		argv[0], g_strFarModuleName.CPtr(), GetPathTranslationPrefix(), InMyTemp().c_str(), InMyConfig().c_str());

	// make current thread to be same as main one to avoid FARString reference-counter
	// from cloning main strings from current one
	OverrideInterThreadID(gMainThreadID);

	Opt.IsUserAdmin = (geteuid()==0);

	_OT(SysLog(L"[[[[[[[[New Session of FAR]]]]]]]]]"));
	FARString strEditViewArg;
	FARString DestNames[2];
	int StartLine=-1,StartChar=-1;
	int CntDestName=0; // количество параметров-имен каталогов
	/*$ 18.04.2002 SKV
	  Попользуем floating point что бы проинициализировался vc-ный fprtl.
	*/
	// если под дебагером, то отключаем исключения однозначно,
	//  иначе - смотря что указал юзвер.

	Opt.strRegRoot = L"Software/Far2";
	CheckForConfigUpgrade();

	// По умолчанию - брать плагины из основного каталога
	Opt.LoadPlug.MainPluginDir=TRUE;
	Opt.LoadPlug.PluginsPersonal=TRUE;
	Opt.LoadPlug.PluginsCacheOnly=FALSE;

	char pid_str[32];
	snprintf(pid_str, sizeof(pid_str) - 1, "%lu", (unsigned long)getpid());
	setenv("FARPID", pid_str, 1);

	g_strFarPath = g_strFarModuleName;

	bool translated = TranslateFarString<TranslateInstallPath_Bin2Share>(g_strFarPath);
	CutToSlash(g_strFarPath, true);
	if (translated) {
		// /usr/bin/something -> /usr/share/far2l
		g_strFarPath.Append("/" APP_BASENAME);
	}
		
	setenv("FARHOME", g_strFarPath.GetMB().c_str(), 1);

	AddEndSlash(g_strFarPath);

	// don't inherit from parent process in any case
	unsetenv("FARUSER");

	if (Opt.IsUserAdmin) {
		setenv("FARADMINMODE", "1", 1);
	} else {
		unsetenv("FARADMINMODE");
	}

	// run by symlinc in editor mode
	if (strstr(argv[0], "far2ledit") != NULL) {
		Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR;
		if (argc > 1) {
			strEditViewArg = argv[argc - 1]; // use last argument
		} else {
			strEditViewArg = "NewFile.txt";
		}
	}

	// макросы не дисаблим
	Opt.Macro.DisableMacro=0;
	for (int I=1; I<argc; I++)
	{
		std::wstring arg_w = MB2Wide(argv[I]);
		if (arg_w.find(L"--") == 0) {
			arg_w.erase(0, 1);
		}
		bool switchHandled = false;
		if ((arg_w[0]==L'/' || arg_w[0]==L'-') && arg_w[1])
		{
			switchHandled = true;
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
						case L'N':

							if (!arg_w[3])
								Opt.NoBoxes=TRUE;

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
						strEditViewArg = argv[I+1];
						if (strEditViewArg == "-")
						{
							Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR_ON_CMDOUT;
							strEditViewArg = ReconstructCommandLine(argc - I - 2, &argv[I+2]);
							I = argc;
						}
						else
						{
							Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR;
							I++;
						}
					}

					break;
				case L'V':

					if (I+1<argc)
					{
						strEditViewArg = argv[I+1];
						if (strEditViewArg == "-")
						{
							Opt.OnlyEditorViewerUsed = Options::ONLY_VIEWER_ON_CMDOUT;
							strEditViewArg = ReconstructCommandLine(argc - I - 2, &argv[I+2]);
							I = argc;
						}
						else
						{
							Opt.OnlyEditorViewerUsed = Options::ONLY_VIEWER;
							I++;
						}
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
					fprintf(stderr, "Unsupported in far2l\n");
					break;

				case L'C':

					if (Upper(arg_w[2])==L'O' && !arg_w[3])
					{
						Opt.LoadPlug.PluginsCacheOnly=TRUE;
						Opt.LoadPlug.PluginsPersonal=FALSE;

					} else if (Upper(arg_w[2]) == L'D' && !arg_w[3]) {
						if (I + 1 < argc) {
							I++;
							arg_w = MB2Wide(argv[I]);
							switchHandled = false;
						}
					}

					break;

				case L'W':
					{
						Opt.WindowMode=TRUE;
					}
					break;
			}
		}
		if (!switchHandled) // простые параметры. Их может быть max две штукА.
		{
			if (CntDestName < 2)
			{
				if (IsPluginPrefixPath((const wchar_t *)arg_w.c_str()))
				{
					DestNames[CntDestName++] = (const wchar_t *)arg_w.c_str();
				}
				else
				{
					DestNames[CntDestName] = arg_w;
					ConvertNameToFull(DestNames[CntDestName],DestNames[CntDestName]);

					if (apiGetFileAttributes(DestNames[CntDestName]) != INVALID_FILE_ATTRIBUTES)
						CntDestName++; //???
				}
			}
		}
	}


    std::unique_ptr<KeyFileHelper> KeyboardLayouts;
    wchar_t *far2l_path = (wchar_t*)g_strFarPath.CPtr();
    std::string kblo_path = StrPrintf("%lskblayouts.ini", far2l_path);
    KeyboardLayouts.reset(new KeyFileHelper(kblo_path.c_str()));

    const char *lc = setlocale(LC_CTYPE, NULL);
    char LangCode[3]; LangCode[0] = lc[0]; LangCode[1] = lc[1]; LangCode[2] = 0;
    
    KbLayoutsTrIn = KeyboardLayouts->GetString(LangCode, "Latin");
    KbLayoutsTrOut = KeyboardLayouts->GetString(LangCode, "Local");


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

	ReadConfig();
	InitConsole();
	static_assert(!IsPtr(Msg::NewFileName._id),
		"Too many language messages. Need to refactor code to eliminate use of IsPtr.");

	if (!Lang.Init(g_strFarPath,true,Msg::NewFileName.ID()))
	{
		LPCWSTR LngMsg;
		switch (Lang.LastError())
		{
		case LERROR_BAD_FILE:
			LngMsg = L"\nError: language data is incorrect or damaged. Press any key to exit...";
			break;
		case LERROR_FILE_NOT_FOUND:
			LngMsg = L"\nError: cannot find language data. Press any key to exit...";
			break;
		default:
			LngMsg = L"\nError: cannot load language data. Press any key to exit...";
		}
		ControlObject::ShowStartupBanner(LngMsg);
		WaitKey(); // А стоит ли ожидать клавишу??? Стоит
		return 1;
	}
	setenv("FARLANG", Opt.strLanguage.GetMB().c_str(), 1);
	initMacroVarTable(1);

	CheckForImportLegacyShortcuts();

	int Result = MainProcess(strEditViewArg,DestNames[0],DestNames[1],StartLine,StartChar);

	EmptyInternalClipboard();
	doneMacroVarTable(1);
	VTShell_Shutdown();//ensure VTShell deinitialized before statics destructors called
	_OT(SysLog(L"[[[[[Exit of FAR]]]]]]]]]"));
	return Result;
}


/*void EncodingTest()
{
	std::wstring v = MB2Wide("\x80hello\x80""aaaaaaaaaaaa\x80""zzzzzzzzzzz\x80");
	printf("%u: '%ls'\n", (unsigned int)v.size(), v.c_str());
	for (size_t i = 0; i<v.size(); ++i)
		printf("%02x ", (unsigned int)v[i]);
		
	printf("\n");
	std::string a = StrWide2MB(v);
	for (size_t i = 0; i<a.size(); ++i)
		printf("%02x ", (unsigned char)a[i]);
	printf("\n");	
}

void SudoTest()
{
	SudoClientRegion sdc_rgn;
	int fd = sdc_open("/root/foo", O_CREAT | O_RDWR, 0666);
	if (fd!=-1) {
		sdc_write(fd, "bar", 3);
		sdc_close(fd);
	} else
		perror("sdc_open");
	exit(0);
}
*/

static int libexec(const char *lib, const char *symbol, int argc, char *argv[])
{
	void *dl = dlopen(lib, RTLD_LOCAL|RTLD_LAZY);
	if (!dl) {
		fprintf(stderr, "libexec('%s', '%s', %u) - dlopen error %u\n", lib, symbol, argc, errno);
		return -1;
	}

	typedef int (*libexec_main_t)(int argc, char *argv[]);
	libexec_main_t libexec_main = (libexec_main_t)dlsym(dl, symbol);
	if (!libexec_main) {
		fprintf(stderr, "libexec('%s', '%s', %u) - dlsym error %u\n", lib, symbol, argc, errno);
		return -1;
	}

	return libexec_main(argc, argv);
}

int FarDispatchAnsiApplicationProtocolCommand(const char *str)
{
	const char *space = strchr(str, ' ');
	if (!space)
		return -1;

	DWORD r;
	std::string command(str, space - str);
	std::string argument(UnescapeUnprintable(space + 1));
	if (command.find("v") == 0) {
		ModalViewFile(argument, false);
		r = 0;
	} else if (command.find("e") == 0) {
		int StartLine = 0, StartChar = 0;
		if (command.size() > 1 && isdigit(command[1])) {
			StartLine = atoi(command.c_str() + 1);
			size_t p = command.find(':');
			if (p != std::string::npos)
				StartChar = atoi(command.c_str() + p + 1);
		}
		//FIXME: modality doesn't work with this FFILEEDIT_ENABLEF6! (see abort after loop)
		FileEditor *Editor=new FileEditor(StrMB2Wide(argument).c_str(), CP_AUTODETECT, 
			FFILEEDIT_DISABLEHISTORY | FFILEEDIT_SAVETOSAVEAS | FFILEEDIT_CANNEWFILE,
			StartLine, StartChar);
		r = Editor->GetExitCode();
		if (r != XC_LOADING_INTERRUPTED && r != XC_OPEN_ERROR) {
			FrameManager->ExecuteModal();
			//abort();
		} else
			delete Editor;
		r = 0;
	} else {
		r = -1;
	}
	
	return r;
}

static void SetCustomSettings(const char *arg)
{
	std::string refined;
	if (arg[0] == '/') {
		refined = arg;

	} else if (arg[0] == '.' && arg[1] == '/') {
		char cwdbuf[MAX_PATH + 1] = {'.', 0};
		const char *cwd = getcwd(cwdbuf, MAX_PATH);
		if (cwd) {
			refined = cwd;
		}
		refined+= &arg[1];

	} else {
		refined = arg;
	}

	while (!refined.empty() && refined.back() == '/') {
		refined.resize(refined.size() - 1);
	}

	fprintf(stderr, "%s: '%s'\n", __FUNCTION__, refined.c_str());

	if (!refined.empty()) {
		// could use FARPROFILE/FARLOCALPROFILE for that but it would be abusing
		setenv("FARSETTINGS", refined.c_str(), 1);
	}
}

int _cdecl main(int argc, char *argv[])
{
	char *name = strrchr(argv[0], GOOD_SLASH);
	if (name) ++name; else name = argv[0];
	if (argc > 0) {
		if (strcmp(name, "far2l_askpass")==0)
			return sudo_main_askpass();
		if (strcmp(name, "far2l_sudoapp")==0)
			return sudo_main_dispatcher(argc - 1, argv + 1);
		if (argc >= 4) {
			if (strcmp(argv[1], "--libexec")==0)
				return libexec(argv[2], argv[3], argc - 4, argv + 4);
		}
		if (argc > 1 &&
		(strncasecmp(argv[1], "--h", 3) == 0
		 || strncasecmp(argv[1], "-h", 2) == 0
		 || strcasecmp(argv[1], "/h") == 0
		 || strcasecmp(argv[1], "/?") == 0)) {

			print_help(name);
			return 0;
		}
	}

	for (int i = 1; i + 1 < argc; ++i) {
		if (strcasecmp(argv[i], "-u") == 0 && argv[i + 1][0] ) {
			SetCustomSettings(argv[i + 1]);
		}
	}

	setlocale(LC_ALL, "");//otherwise non-latin keys missing with XIM input method

	SetupFarPath(argc, argv);

	SafeMMap::SignalHandlerRegistrar smm_shr;

	gMainThreadID = GetInterThreadID();

	return WinPortMain(g_strFarModuleName.GetMB().c_str(), argc, argv, FarAppMain);
}


