/*
execute.cpp

"Запускатель" программ.
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

#include <sys/stat.h>
#include <fcntl.h>
#include "execute.hpp"
#include "keyboard.hpp"
#include "filepanels.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "savescr.hpp"
#include "chgprior.hpp"
#include "cmdline.hpp"
#include "panel.hpp"
#include "rdrwdsk.hpp"
#include "udlist.hpp"
#include "registry.hpp"
//#include "localOEM.hpp"
#include "manager.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "syslog.hpp"
#include "constitle.hpp"
#include "console.hpp"
#include "constitle.hpp"
#include "vtshell.h"
#include <wordexp.h>
#include <set>
#include <sys/wait.h>

static WCHAR eol[2] = {'\r', '\n'};

class ExecClassifier
{
	bool _file, _executable, _backround;
  std::string _cmd;
	
	bool IsExecutableByExtension(const char *s)
	{
		s = strrchr(s, '.');
		if (!s || strchr(s, GOOD_SLASH))
			return true;//assume files without extension to be scripts
			
		return (strcmp(s, ".sh")==0 || strcasecmp(s, ".pl")==0|| strcasecmp(s, ".py")==0);//todo: extend
	}
	
public:
	ExecClassifier(const char *cmd) 
		: _file(false), _executable(false), _backround(false)
	{
		const char *bg_suffix = strrchr(cmd, '&');
		if (bg_suffix && bg_suffix!=cmd && *(bg_suffix-1)!='\\') {
			for (++bg_suffix; *bg_suffix==' '; ++bg_suffix);
			if (!*bg_suffix) _backround = true;
		}

		const std::vector<std::string> &argv = ExplodeCmdLine(cmd);
		if (argv.empty() || argv[0].empty()) {
			fprintf(stderr, "ExecClassifier('%s') - empty command\n", cmd);
			return;
		}
		_cmd = argv[0];
		int f = open(_cmd.c_str(), O_RDONLY);
		if (f==-1) {
			fprintf(stderr, "ExecClassifier('%s') - open error %u\n", cmd, errno);
			return;
		}
		
		struct stat s = {0};
		if (fstat(f, &s)==0 && S_ISREG(s.st_mode)) {//todo: handle S_ISLNK(s.st_mode)
			_file = true;
			if ((s.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))!=0) {
				char buf[8] = { 0 };
				int r = read(f, buf, sizeof(buf));
				if (r > 4 && buf[0]==0x7f && buf[1]=='E' && buf[2]=='L' && buf[3]=='F') {
					fprintf(stderr, "ExecClassifier('%s') - ELF executable\n", cmd);
					_executable = true;
				} else if (r > 2 && buf[0]=='#' && buf[1]=='!') {
					fprintf(stderr, "ExecClassifier('%s') - script\n", cmd);
					_executable = true;
				} else {
					_executable = IsExecutableByExtension(cmd);
					fprintf(stderr, "ExecClassifier('%s') - unknown: %02x %02x %02x %02x assumed %sexecutable\n", 
						cmd, (unsigned)buf[0], (unsigned)buf[1], (unsigned)buf[2], (unsigned)buf[3], _executable ? "" : "not ");
				}
			} else
				fprintf(stderr, "IsPossibleXDGOpeSubject('%s') - not executable mode=0x%x\n", cmd, s.st_mode);	
		} else 
			fprintf(stderr, "IsPossibleXDGOpeSubject('%s') - not regular mode=0x%x\n", cmd, s.st_mode);
	
		close(f);
	}
	
	const std::string& cmd() const {return _cmd; }
	bool IsFile() const {return _file; }
	bool IsExecutable() const {return _executable; }
	bool IsBackground() const {return _backround; }
};

static void CallExec(const char *CmdStr) 
{
	int r = execl("/bin/sh", "sh", "-c", CmdStr, NULL);
	fprintf(stderr, "CallExec: execl returned %d errno %u\n", r, errno);
	_exit(r);//forget about static object, just exit
	exit(r);
}

static int NotVTExecute(const char *CmdStr, bool NoWait, bool NeedSudo)
{
	int r = -1;
	if (NeedSudo) {
		return sudo_client_execute(CmdStr, false, NoWait);
	}
	int fdr = open("/dev/null", O_RDONLY);
	if (fdr==-1) perror("open stdin error");
	
	//let debug out go to console
	int fdw = open("/dev/null", O_WRONLY);
	//if (fdw==-1) perror("open stdout error");
	int pid = fork();
	if (pid==0) {
		if (fdr!=-1) {
			dup2(fdr, STDIN_FILENO);
			close(fdr);
		}
			
		if (fdw!=-1) {
			dup2(fdw, STDOUT_FILENO);
			dup2(fdw, STDERR_FILENO);
			close(fdw);
		}
		setsid();
		signal( SIGINT, SIG_DFL );
		signal( SIGHUP, SIG_DFL );
		signal( SIGPIPE, SIG_DFL );
		CallExec(CmdStr);
	} else if (pid==-1) {
		perror("fork failed");
	} else if (!NoWait) {
		if (waitpid(pid, &r, 0)==-1) {
			fprintf(stderr, "NotVTExecute('%s', %u): waitpid(0x%x) error %u\n", CmdStr, NoWait, pid, errno);
			r = 1;
		} else {
			fprintf(stderr, "NotVTExecute('%s', %u): r=%d\n", CmdStr, NoWait, r);
		}
	} else {
		PutZombieUnderControl(pid);
		r = 0;
	}
	if (fdr!=-1) close(fdr);
	if (fdw!=-1) close(fdw);
	return r;
}

int WINAPI farExecuteA(const char *CmdStr, unsigned int ExecFlags)
{
//	fprintf(stderr, "TODO: Execute('" WS_FMT "')\n", CmdStr);
	int r;
	if (ExecFlags & EF_HIDEOUT) {
		r = NotVTExecute(CmdStr, (ExecFlags & EF_NOWAIT) != 0, (ExecFlags & EF_SUDO) != 0);
	} else {
		ProcessShowClock++;
		CtrlObject->CmdLine->ShowBackground();
		CtrlObject->CmdLine->Redraw();
		CtrlObject->CmdLine->SetString(L"", TRUE);
		ScrBuf.Flush();
		DWORD saved_mode = 0, dw;
		WINPORT(GetConsoleMode)(NULL, &saved_mode);
		WINPORT(SetConsoleMode)(NULL, saved_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT
			| ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE | WINDOW_BUFFER_SIZE_EVENT);
		const std::wstring &ws = MB2Wide(CmdStr);
		WINPORT(WriteConsole)( NULL, ws.c_str(), ws.size(), &dw, NULL );
		WINPORT(WriteConsole)( NULL, &eol[0], ARRAYSIZE(eol), &dw, NULL );
		if (ExecFlags & (EF_NOWAIT|EF_HIDEOUT) ) {
			r = NotVTExecute(CmdStr, (ExecFlags & EF_NOWAIT) != 0, (ExecFlags & EF_SUDO) != 0);
		} else {
			r = VTShell_Execute(CmdStr, (ExecFlags & EF_SUDO) != 0);
		}
		WINPORT(SetConsoleMode)( NULL, saved_mode | 
			ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT );
		WINPORT(WriteConsole)( NULL, &eol[0], ARRAYSIZE(eol), &dw, NULL );
		WINPORT(SetConsoleMode)(NULL, saved_mode);
		ScrBuf.FillBuf();
		CtrlObject->CmdLine->SaveBackground();
		ProcessShowClock--;
		SetFarConsoleMode(TRUE);
		ScrBuf.Flush();
	}
	fprintf(stderr, "farExecuteA:('%s', 0x%x): r=%d\n", CmdStr, ExecFlags, r);
	
	return r;
}

int WINAPI farExecuteLibraryA(const char *Library, const char *Symbol, const char *CmdStr, unsigned int ExecFlags)
{
	std::string actual_cmd = "\"";
	actual_cmd+= EscapeQuotas(g_strFarModuleName.GetMB());
	actual_cmd+= "\" --libexec \"";
	actual_cmd+= EscapeQuotas(Library);
	actual_cmd+= "\" ";
	actual_cmd+= Symbol;
	actual_cmd+= " ";
	actual_cmd+= CmdStr;
	return farExecuteA(actual_cmd.c_str(), ExecFlags);
}

static int ExecuteA(const char *CmdStr, bool AlwaysWaitFinish, bool SeparateWindow, bool DirectRun, bool FolderRun , bool WaitForIdle , bool Silent , bool RunAs)
{
	int r = -1;
	ExecClassifier ec(CmdStr);
	unsigned int flags = ec.IsBackground() ? EF_NOWAIT | EF_HIDEOUT : 0;
	if (ec.IsFile()) {
		std::string tmp;
		if (!ec.IsExecutable())
			tmp+= "xdg-open ";

  	if (ec.cmd()[0]!='/' && ec.cmd()[0]!='.')
			tmp+= "./"; // it is ok to prefix ./ even to a quoted string
		tmp += CmdStr;

		if (!ec.IsExecutable()) {
			r = farExecuteA(tmp.c_str(), flags | EF_NOWAIT);
			if (r!=0) {
				fprintf(stderr, "ClassifyAndRun: status %d errno %d for %s\n", r, errno, tmp.c_str() );
				//TODO: nicely report if xdg-open exec failed
			}
		} else
			r = farExecuteA(tmp.c_str(), flags);
	} else 
		r = farExecuteA(CmdStr, flags);
	return r;
}


int Execute(const wchar_t *CmdStr, bool AlwaysWaitFinish, bool SeparateWindow, bool DirectRun, bool FolderRun , bool WaitForIdle , bool Silent , bool RunAs)
{
	return ExecuteA(Wide2MB(CmdStr).c_str(), AlwaysWaitFinish, SeparateWindow, DirectRun, FolderRun , WaitForIdle , Silent , RunAs);
}

int CommandLine::CmdExecute(const wchar_t *CmdLine, bool AlwaysWaitFinish, bool SeparateWindow, bool DirectRun, bool WaitForIdle, bool Silent, bool RunAs)
{
	if (!SeparateWindow && CtrlObject->Plugins.ProcessCommandLine(CmdLine))
	{
		/* $ 12.05.2001 DJ - рисуемся только если остались верхним фреймом */
		if (CtrlObject->Cp()->IsTopFrame())
		{
			//CmdStr.SetString(L"");
			GotoXY(X1,Y1);
			FS<<fmt::Width(X2-X1+1)<<L"";
			Show();
			ScrBuf.Flush();
		}

		return -1;
	}


	int r;

	FARString strPrevDir = strCurDir;
	bool PrintCommand = true;
	if (ProcessOSCommands(CmdLine, SeparateWindow, PrintCommand) ) {
		ShowBackground();
		FARString strNewDir = strCurDir;
		strCurDir = strPrevDir;
		Redraw();
		strCurDir = strNewDir;
		if (PrintCommand)
		{
			GotoXY(X2+1,Y1);
			Text(L" ");
			ScrollScreen(2);
		}

		SetString(L"", FALSE);
		SaveBackground();
		
		r = -1; 
	} else {
		r = Execute(CmdLine, AlwaysWaitFinish, SeparateWindow, DirectRun, false , WaitForIdle , Silent , RunAs);
	}

	if (!Flags.Check(FCMDOBJ_LOCKUPDATEPANEL)) {
		ShellUpdatePanels(CtrlObject->Cp()->ActivePanel, FALSE);
		if (Opt.ShowKeyBar)
			CtrlObject->MainKeyBar->Show();
		
	}
	
	return r;
}

const wchar_t *PrepareOSIfExist(const wchar_t *CmdLine)
{
	return L"";
}

bool ProcessOSAliases(FARString &strStr)
{
	return false;
}
