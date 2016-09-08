/*
execute.cpp

"Çàïóñêàòåëü" ïðîãðàìì.
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

extern std::vector<std::string> ExplodeCmdLine(const char *cmd_line);

static WCHAR eol[2] = {'\r', '\n'};

class ExecClassifier
{
	bool _file, _executable, _backround;
	
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
		if (bg_suffix) {
			for (++bg_suffix; *bg_suffix==' '; ++bg_suffix);
			if (!*bg_suffix) _backround = true;
		}
		
		std::vector<std::string> cmds = ExplodeCmdLine(cmd);
		if (cmds.empty() || cmds[0].empty()) {
			fprintf(stderr, "ExecClassifier(%s) - empty cmd\n", cmd);
			return;
		}
		if (cmds[0][0]!='/' && cmds[0][0]!='.')
			cmds[0] = "./" + cmds[0];

		cmd = cmds[0].c_str();
		int f = open(cmd, O_RDONLY);
		if (f==-1) {
			fprintf(stderr, "ExecClassifier(%s) - open error %u\n", cmd, errno);
			return;
		}
		
		struct stat s = {0};
		if (fstat(f, &s)==0 && S_ISREG(s.st_mode)) {//todo: handle S_ISLNK(s.st_mode)
			_file = true;
			if ((s.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))!=0) {
				char buf[8] = { 0 };
				int r = read(f, buf, sizeof(buf));
				if (r > 4 && buf[0]==0x7f && buf[1]=='E' && buf[2]=='L' && buf[3]=='F') {
					fprintf(stderr, "ExecClassifier(%s) - ELF executable\n", cmd);
					_executable = true;
				} else if (r > 2 && buf[0]=='#' && buf[1]=='!') {
					fprintf(stderr, "ExecClassifier(%s) - script\n", cmd);
					_executable = true;
				} else {
					_executable = IsExecutableByExtension(cmd);
					fprintf(stderr, "ExecClassifier(%s) - unknown: %02x %02x %02x %02x assumed %sexecutable\n", 
						cmd, (unsigned)buf[0], (unsigned)buf[1], (unsigned)buf[2], (unsigned)buf[3], _executable ? "" : "not ");
				}
			} else
				fprintf(stderr, "IsPossibleXDGOpeSubject(%s) - not executable mode=0x%x\n", cmd, s.st_mode);	
		} else 
			fprintf(stderr, "IsPossibleXDGOpeSubject(%s) - not regular mode=0x%x\n", cmd, s.st_mode);
	
		close(f);
	}
	
	bool IsFile() const {return _file; }
	bool IsExecutable() const {return _executable; }
	bool IsBackground() const {return _backround; }
};

void ExecuteOrForkProc(const char *CmdStr, int (WINAPI *ForkProc)(int argc, char *argv[]) ) 
{
	int r = -1;

	if (ForkProc) {
		wordexp_t we = {};
		if (wordexp(CmdStr, &we, 0)==0) {
			r = ForkProc(we.we_wordc, we.we_wordv);
			wordfree(&we);
		} else
			fprintf(stderr, "ExecuteOrForkProc: wordexp(%s) errno %u\n", CmdStr, errno);
	} else {
		const char *shell = getenv("SHELL");
		if (!shell)
			shell = "/bin/sh";
		r = execl(shell, shell, "-ic", CmdStr, NULL);
		fprintf(stderr, "ExecuteOrForkProc: execl returned %d errno %u\n", r, errno);
	}
	exit(r);
}

class : std::set<pid_t>
{
	std::mutex _mutex;
	
public:
	void Watch(pid_t pid)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		insert(pid);
		for (iterator i = begin(); i!=end(); ) {
			int r;
			if (waitpid(*i, &r, WNOHANG)==*i)
				 i = erase(i);
			else 
				++i;
		}
	}
} g_dezombify;

static int NotVTExecute(const char *CmdStr, bool NoWait, int (WINAPI *ForkProc)(int argc, char *argv[]) )
{
	int r = -1;
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
		ExecuteOrForkProc(CmdStr, ForkProc) ;
	} else if (pid==-1) {
		perror("fork failed");
	} else if (!NoWait) {
		if (waitpid(pid, &r, 0)==-1) {
			fprintf(stderr, "NotVTExecuteForkProc('%s', %u, %p): waitpid(0x%x) error %u\n", CmdStr, NoWait, ForkProc, pid, errno);
			r = 1;
		} else {
			fprintf(stderr, "NotVTExecuteForkProc:('%s', %u, %p): r=%d\n", CmdStr, NoWait, ForkProc, r);
		}
	} else {
		g_dezombify.Watch(pid);
		r = 0;
	}
	if (fdr!=-1) close(fdr);
	if (fdw!=-1) close(fdw);
	return r;
}

int WINAPI farExecuteA(const char *CmdStr, unsigned int ExecFlags, int (WINAPI *ForkProc)(int argc, char *argv[]) )
{
//	fprintf(stderr, "TODO: Execute('" WS_FMT "')\n", CmdStr);
	ProcessShowClock++;
	CtrlObject->CmdLine->ShowBackground();
	CtrlObject->CmdLine->Redraw();
	CtrlObject->CmdLine->SetString(L"", TRUE);
	ScrBuf.Flush();
	
	DWORD saved_mode = 0, dw;
	WINPORT(GetConsoleMode)(NULL, &saved_mode);
	WINPORT(SetConsoleMode)(NULL, saved_mode | 
		ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	const std::wstring &ws = MB2Wide(CmdStr);
	WINPORT(WriteConsole)( NULL, ws.c_str(), ws.size(), &dw, NULL );
	WINPORT(WriteConsole)( NULL, &eol[0], ARRAYSIZE(eol), &dw, NULL );
	int r;
	if (ExecFlags & (EF_NOWAIT|EF_HIDEOUT) ) {
		r = NotVTExecute(CmdStr, (ExecFlags & EF_NOWAIT) != 0, ForkProc);
	} else {
		r = VTShell_Execute(CmdStr, ForkProc);
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
	fprintf(stderr, "farExecuteA:('%s', 0x%x, %p): r=%d\n", CmdStr, ExecFlags, ForkProc, r);
	
	return r;
}

static std::string MakeCommandLine(const std::vector<std::string>& cmds) {
	std::string tmp;
	for (size_t i=0; i<cmds.size(); i++) {
		if (i != 0)
			tmp += ' ';
		std::wstring ws = StrMB2Wide(cmds[i]); // TODO: avoid conversion to UTF16 and then back
		fprintf(stderr, "ws[%u]=(%ls)\n", i, ws.c_str());
		FARString fs(ws.c_str(), ws.size());
		EscapeSpace(fs); // TODO: were some of the cmds' parts made by ExplodeCmdLine(), escape them closer to the original (using ' or " or \)
		fprintf(stderr, "fs[%u]=(%ls)\n", i, fs.CPtr());
		tmp += Wide2MB(fs.CPtr());
	}
  return tmp;
}

static int ExecuteA(const char *CmdStr, bool AlwaysWaitFinish, bool SeparateWindow, bool DirectRun, bool FolderRun , bool WaitForIdle , bool Silent , bool RunAs)
{
	int r = -1;

	std::vector<std::string> cmds = ExplodeCmdLine(CmdStr);
	if (cmds.empty() || cmds[0].empty()) {
		fprintf(stderr, "ExecuteA(%s) - empty cmd\n", CmdStr);
		return -1;
	}

	ExecClassifier ec(CmdStr);
	unsigned int flags = ec.IsBackground() ? EF_NOWAIT | EF_HIDEOUT : 0;
	if (ec.IsFile()) {
		if (cmds[0][0]!='/' && cmds[0][0]!='.')
			cmds[0] = "./" + cmds[0];

		if (!ec.IsExecutable()) {
			cmds.insert(cmds.begin(), "xdg-open");
			r = farExecuteA(MakeCommandLine(cmds).c_str(), flags | EF_NOWAIT, NULL);
			if (r!=0) {
				fprintf(stderr, "ClassifyAndRun: status %d errno %d for %s\n", r, errno, MakeCommandLine(cmds).c_str() );
				//TODO: nicely report if xdg-open exec failed
			}
		} else
			r = farExecuteA(MakeCommandLine(cmds).c_str(), flags, NULL);
	} else
		r = farExecuteA(MakeCommandLine(cmds).c_str(), flags, NULL);
	return r;
}


int Execute(const wchar_t *CmdStr, bool AlwaysWaitFinish, bool SeparateWindow, bool DirectRun, bool FolderRun , bool WaitForIdle , bool Silent , bool RunAs)
{
	return ExecuteA(Wide2MB(CmdStr).c_str(), AlwaysWaitFinish, SeparateWindow, DirectRun, FolderRun , WaitForIdle , Silent , RunAs);
}

int CommandLine::CmdExecute(const wchar_t *CmdLine, bool AlwaysWaitFinish, bool SeparateWindow, bool DirectRun, bool WaitForIdle, bool Silent, bool RunAs)
{
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
