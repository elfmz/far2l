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
// #include "localOEM.hpp"
#include "manager.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "mix.hpp"
#include "syslog.hpp"
#include "constitle.hpp"
#include "console.hpp"
#include "constitle.hpp"
#include "chgmmode.hpp"
#include "vtshell.h"
#include "InterThreadCall.hpp"
#include "ScopeHelpers.h"
#include <set>
#include <sys/wait.h>
#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <signal.h>
#endif

static WCHAR eol[2] = {'\r', '\n'};

class ExecClassifier
{
    bool _dir, _file, _executable, _prefixed, _brokensymlink;

	bool IsExecutableByExtension(const char *s)
	{
		s = strrchr(s, '.');
		if (!s || strchr(s, GOOD_SLASH))
			return true;																			// assume files without extension to be scripts

		return (strcmp(s, ".sh") == 0 || strcasecmp(s, ".pl") == 0 || strcasecmp(s, ".py") == 0);	// todo: extend
	}

public:
	ExecClassifier(const char *cmd, bool direct)
		:
        _dir(false), _file(false), _executable(false), _prefixed(false), _brokensymlink(false)
	{
		Environment::ExplodeCommandLine ecl(cmd);
		if (!ecl.empty() && ecl.back() == "&") {
			ecl.pop_back();
		}

		if (ecl.empty() || ecl[0].empty()) {
			fprintf(stderr, "ExecClassifier('%s', %d) - empty command\n", cmd, direct);
			return;
		}

		const std::string &arg0 = ecl[0];

		_prefixed = PathHasParentPrefix(arg0);

		if (!direct && !_prefixed) {
			fprintf(stderr, "ExecClassifier('%s', %d) - nor direct nor rooted\n", cmd, direct);
			return;
		}

		struct stat s = {0};
		if (stat(arg0.c_str(), &s) == -1) {
			fprintf(stderr, "ExecClassifier('%s', %d) - stat error %u\n", cmd, direct, errno);
            if ((errno==ENOENT || errno==EACCES) && lstat(arg0.c_str(), &s) != -1)
            {
                _brokensymlink=true;
                fprintf(stderr, "ExecClassifier: broken or inaccessible symbolic link\n");
            }
			return;
		}

		if (S_ISDIR(s.st_mode)) {
			_dir = true;
			return;
		}

		if (!S_ISREG(s.st_mode)) {
			fprintf(stderr, "ExecClassifier('%s', %d) - not regular mode=0x%x\n", cmd, direct, s.st_mode);
			return;
		}

		FDScope f(open(arg0.c_str(), O_RDONLY));
		if (!f.Valid()) {
			fprintf(stderr, "ExecClassifier('%s') - open error %u\n", cmd, errno);
			return;
		}

		_file = true;
		if ((s.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0) {
			char buf[8] = {0};
			int r = read(f, buf, sizeof(buf));
			if (r > 4 && buf[0] == 0x7f && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F') {
				fprintf(stderr, "ExecClassifier('%s') - ELF executable\n", cmd);
				_executable = true;

			} else if (r > 2 && buf[0] == '#' && buf[1] == '!') {
				fprintf(stderr, "ExecClassifier('%s') - script\n", cmd);
				_executable = true;

			} else {
				_executable = IsExecutableByExtension(arg0.c_str());
				fprintf(stderr, "ExecClassifier('%s') - unknown: %02x %02x %02x %02x assumed %sexecutable\n",
						cmd, (unsigned)buf[0], (unsigned)buf[1], (unsigned)buf[2], (unsigned)buf[3],
						_executable ? "" : "not ");
			}

		} else {
			fprintf(stderr, "ExecClassifier('%s') - not executable mode=0x%x\n", cmd, s.st_mode);
		}
	}

	bool IsPrefixed() const { return _prefixed; }
	bool IsFile() const { return _file; }
	bool IsDir() const { return _dir; }
	bool IsExecutable() const { return _executable; }
	bool IsBrokenSymlink() const { return _brokensymlink; }
};

static std::string GetOpenShVerb(const char *verb)
{
	std::string out = GetMyScriptQuoted("open.sh");
	out+= ' ';
	out+= verb;
	out+= ' ';
	return out;
}

bool IsDirectExecutableFilePath(const char *path)
{
	ExecClassifier ec(path, true);
	return ec.IsExecutable();
}

static void CallExec(const char *CmdStr)
{
	int r = execl("/bin/sh", "sh", "-c", CmdStr, NULL);
	fprintf(stderr, "CallExec: execl returned %d errno %u\n", r, errno);
	_exit(r);	// forget about static object, just exit
	exit(r);
}

static int NotVTExecute(const char *CmdStr, bool NoWait, bool NeedSudo)
{
	int r = -1, fdr = -1, fdw = -1;
	if (NeedSudo) {
		return sudo_client_execute(CmdStr, false, NoWait);
	}

	// DEBUG
	//	fdr = open(DEVNULL, O_RDONLY);
	//	if (fdr==-1) perror("stdin error opening " DEVNULL);

	// let debug out go to console
	//	fdw = open(DEVNULL, O_WRONLY);
	// if (fdw==-1) perror("open stdout error");
	int pid = fork();
	if (pid == 0) {
		if (fdr != -1) {
			dup2(fdr, STDIN_FILENO);
			close(fdr);
		}

		if (fdw != -1) {
			dup2(fdw, STDOUT_FILENO);
			dup2(fdw, STDERR_FILENO);
			close(fdw);
		}
		setsid();
		signal(SIGINT, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGPIPE, SIG_DFL);
		CallExec(CmdStr);
	} else if (pid == -1) {
		perror("fork failed");
	} else if (!NoWait) {
		if (waitpid(pid, &r, 0) == -1) {
			fprintf(stderr, "NotVTExecute('%s', %u): waitpid(0x%x) error %u\n", CmdStr, NoWait, pid, errno);
			r = 1;
		} else {
			fprintf(stderr, "NotVTExecute('%s', %u): r=%d\n", CmdStr, NoWait, r);
		}
	} else {
		PutZombieUnderControl(pid);
		r = 0;
	}
	if (fdr != -1)
		close(fdr);
	if (fdw != -1)
		close(fdw);
	return r;
}

class FarExecuteScope
{
	DWORD _dw;
	DWORD _saved_mode{0};

public:
	FarExecuteScope(const char *cmd_str)
	{
		ProcessShowClock++;
		if (CtrlObject && CtrlObject->CmdLine) {
			CtrlObject->CmdLine->ShowBackground();
			CtrlObject->CmdLine->RedrawWithoutComboBoxMark();
		}
		//		CtrlObject->CmdLine->SetString(L"", TRUE);
		ScrBuf.Flush();
		WINPORT(GetConsoleMode)(NULL, &_saved_mode);
		WINPORT(SetConsoleMode) (NULL, _saved_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT
			| ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE | ENABLE_ECHO_INPUT);	// ENABLE_QUICK_EDIT_MODE
		if (cmd_str) {
			const std::wstring &ws = MB2Wide(cmd_str);
			WINPORT(WriteConsole)(NULL, ws.c_str(), ws.size(), &_dw, NULL);
			WINPORT(WriteConsole)(NULL, &eol[0], ARRAYSIZE(eol), &_dw, NULL);
		}
		WINPORT(SetConsoleFKeyTitles)(NULL, NULL);
	}

	~FarExecuteScope()
	{
		WINPORT(SetConsoleMode)(NULL, _saved_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
		WINPORT(WriteConsole)(NULL, &eol[0], ARRAYSIZE(eol), &_dw, NULL);
		WINPORT(SetConsoleMode)(NULL, _saved_mode);
		ScrBuf.FillBuf();
		if (CtrlObject && CtrlObject->CmdLine) {
			CtrlObject->CmdLine->SaveBackground();
		}
		ProcessShowClock--;
		SetFarConsoleMode(TRUE);
		ScrBuf.Flush();
		if (CtrlObject && CtrlObject->MainKeyBar) {
			CtrlObject->MainKeyBar->Refresh(Opt.ShowKeyBar, true);
		}
	}
};

static int farExecuteASynched(const char *CmdStr, unsigned int ExecFlags)
{
	//	fprintf(stderr, "TODO: Execute('%ls')\n", CmdStr);
	int r;
	if (ExecFlags & EF_OPEN) {
		std::string OpenCmd = GetOpenShVerb("other");
		OpenCmd+= ' ';
		OpenCmd+= CmdStr;
		return farExecuteASynched(OpenCmd.c_str(), ExecFlags & (~EF_OPEN));
	}

	const bool may_notify = (ExecFlags & (EF_NOTIFY | EF_NOWAIT)) == EF_NOTIFY && Opt.NotifOpt.OnConsole;
	if (ExecFlags & (EF_HIDEOUT | EF_NOWAIT)) {
		r = NotVTExecute(CmdStr, (ExecFlags & EF_NOWAIT) != 0, (ExecFlags & EF_SUDO) != 0);
		//		CtrlObject->CmdLine->SetString(L"", TRUE);//otherwise command remain in cmdline
		if (may_notify) {
			DisplayNotification(
				r ? Msg::ConsoleCommandFailed : Msg::ConsoleCommandComplete,
				(ExecFlags & EF_NOCMDPRINT) ? "..." : CmdStr);
		}

	} else {
		FarExecuteScope fes((ExecFlags & EF_NOCMDPRINT) ? "" : CmdStr);
		r = VTShell_Execute(CmdStr, (ExecFlags & EF_SUDO) != 0, (ExecFlags & EF_MAYBGND) != 0, may_notify);
	}
	fprintf(stderr, "farExecuteA:('%s', 0x%x): r=%d\n", CmdStr, ExecFlags, r);

	return r;
}

int WINAPI farExecuteA(const char *CmdStr, unsigned int ExecFlags)
{
	return InterThreadCall<int, -1>(std::bind(farExecuteASynched, CmdStr, ExecFlags));
}

int WINAPI
farExecuteLibraryA(const char *Library, const char *Symbol, const char *CmdStr, unsigned int ExecFlags)
{
	char cd_prev[MAX_PATH + 1] = {'.', 0};
	if (!sdc_getcwd(cd_prev, MAX_PATH)) {
		cd_prev[0] = 0;
	}
	if (sdc_chdir(g_strFarPath.GetMB().c_str()) == -1) {
		perror("chdir farpath");
	}

	std::string actual_cmd = "\"";
	actual_cmd+= EscapeCmdStr(g_strFarModuleName.GetMB());
	actual_cmd+= "\" --libexec \"";
	actual_cmd+= EscapeCmdStr(Library);
	actual_cmd+= "\" \"";
	actual_cmd+= EscapeCmdStr(cd_prev);
	actual_cmd+= "\" ";
	actual_cmd+= Symbol;
	actual_cmd+= " ";
	actual_cmd+= CmdStr;
	int r = farExecuteA(actual_cmd.c_str(), ExecFlags);

	if (sdc_chdir(cd_prev) == -1) {
		perror("chdir prev");
	}

	return r;
}

void WaitForClose(const wchar_t *Name)
{
	fprintf(stderr, "%s: Name='%ls'\n", __FUNCTION__, Name);

	std::string cmd = Wide2MB(Name);
	QuoteCmdArgIfNeed(cmd);
	cmd.insert(0, " ");
	cmd.insert(0, GetMyScriptQuoted("closewait.sh"));

	farExecuteA(cmd.c_str(), EF_HIDEOUT);
}

void QueueDeleteOnClose(const wchar_t *Name)
{
	fprintf(stderr, "%s: Name='%ls'\n", __FUNCTION__, Name);

	std::string cmd = Wide2MB(Name);
	QuoteCmdArgIfNeed(cmd);
	cmd.insert(0, " --delete ");
	cmd.insert(0, GetMyScriptQuoted("closewait.sh"));

	farExecuteA(cmd.c_str(), EF_NOWAIT | EF_HIDEOUT);
}

static int
ExecuteA(const char *CmdStr, bool SeparateWindow, bool DirectRun, bool WaitForIdle, bool Silent, bool RunAs)
{
	fprintf(stderr,
			"ExecuteA: SeparateWindow=%d DirectRun=%d WaitForIdle=%d Silent=%d RunAs=%d CmdStr='%s'\n",
			SeparateWindow, DirectRun, WaitForIdle, Silent, RunAs, CmdStr);

	int r = -1;
	ExecClassifier ec(CmdStr, DirectRun);
	unsigned int flags = ((Silent || SeparateWindow) ? 0 : EF_NOTIFY);
	if (!SeparateWindow) {
		flags|= EF_MAYBGND;
	}
	std::string tmp;
	if (ec.IsDir() && SeparateWindow) {
		tmp = GetOpenShVerb("dir");
	} else if (ec.IsFile()) {
		if (ec.IsExecutable()) {
			if (SeparateWindow) {
				tmp = GetOpenShVerb("exec");
			}
		} else {
			tmp = GetOpenShVerb("other");
		}
	} else if (SeparateWindow) {
        if(ec.IsBrokenSymlink()) {
            return -1;
        }
		tmp = GetOpenShVerb("exec");
    } else {
        return farExecuteA(CmdStr, flags);
    }

	if (!tmp.empty()) {
		flags|= EF_NOWAIT | EF_HIDEOUT;		// open.sh doesnt print anything
	}
	if ((ec.IsFile() || ec.IsDir()) && DirectRun && !ec.IsPrefixed()) {
		tmp+= "./";		// it is ok to prefix ./ even to a quoted string
	}
	tmp+= CmdStr;

	r = farExecuteA(tmp.c_str(), flags);
	if (r != 0) {
		fprintf(stderr, "ClassifyAndRun: status %d errno %d for %s\n", r, errno, tmp.c_str());
		// TODO: nicely report if xdg-open exec failed
	}

	return r;
}

int Execute(const wchar_t *CmdStr, bool SeparateWindow, bool DirectRun, bool WaitForIdle, bool Silent,
		bool RunAs)
{
	return ExecuteA(Wide2MB(CmdStr).c_str(), SeparateWindow, DirectRun, WaitForIdle, Silent, RunAs);
}

void CommandLine::CheckForKeyPressAfterCmd(int r)
{
	if (CtrlObject && (Opt.CmdLine.WaitKeypress > 1 || (Opt.CmdLine.WaitKeypress == 1 && r != 0))) {
		auto *cp = CtrlObject->Cp();
		if (!CloseFAR && cp && cp->LeftPanel && cp->RightPanel
				&& (cp->LeftPanel->IsVisible() || cp->RightPanel->IsVisible())) {
			FarKey Key;
			{
				ChangeMacroMode cmm(MACRO_OTHER);	// prevent macros from intercepting key (#1003)
				Key = WaitKey();
			}
			// allow user to open console log etc directly from pause-on-error state
			if (Key == KEY_MSWHEEL_UP) {
				Key|= KEY_CTRL | KEY_SHIFT;
			}
			if (Key == (KEY_MSWHEEL_UP | KEY_CTRL | KEY_SHIFT) || Key == KEY_CTRLSHIFTF3 || Key == KEY_F3
					|| Key == KEY_CTRLSHIFTF4 || Key == KEY_F4 || Key == KEY_F8) {
				ProcessKey(Key);
			}
		}
	}
}

void CommandLine::SwitchToBackgroundTerminal(size_t vt_index)
{
	int r;
	{
		FarExecuteScope fes(nullptr);
		r = VTShell_Switch(vt_index);
	}
	CheckForKeyPressAfterCmd(r);
}


int CommandLine::CmdExecute(const wchar_t *CmdLine, bool SeparateWindow, bool DirectRun, bool WaitForIdle,
		bool Silent, bool RunAs)
{
	if (!SeparateWindow && CtrlObject->Plugins.ProcessCommandLine(CmdLine)) {
		/* $ 12.05.2001 DJ - рисуемся только если остались верхним фреймом */
		if (CtrlObject->Cp()->IsTopFrame()) {
			// CmdStr.SetString(L"");
			GotoXY(X1, Y1);
			FS << fmt::Cells() << fmt::Expand(X2 - X1 + 1) << L"";
			Show();
			ScrBuf.Flush();
		}

		return -1;
	}

	int r;

	FARString strPrevDir = strCurDir;
	bool PrintCommand = true;
	if (ProcessOSCommands(CmdLine, SeparateWindow, PrintCommand)) {
		ShowBackground();
		FARString strNewDir = strCurDir;
		strCurDir = strPrevDir;
		Redraw();
		strCurDir = strNewDir;
		if (PrintCommand) {
			GotoXY(X2 + 1, Y1);
			Text(L" ");
			ScrollScreen(2);
		}

		SetString(L"", FALSE);
		SaveBackground();

		r = -1;
	} else {
		if (CtrlObject)
			CtrlObject->CmdLine->SetString(L"", TRUE);

		char cd_prev[MAX_PATH + 1] = {'.', 0};
		if (!sdc_getcwd(cd_prev, MAX_PATH)) {
			cd_prev[0] = 0;
		}

		r = Execute(CmdLine, SeparateWindow, DirectRun, WaitForIdle, Silent, RunAs);

		char cd[MAX_PATH + 1] = {'.', 0};
		if (sdc_getcwd(cd, MAX_PATH)) {
			if (strcmp(cd_prev, cd) != 0) {
				if (!IntChDir(MB2Wide(cd).c_str(), true, false)) {
					perror("IntChDir");
				}
			}
		} else {
			perror("sdc_getcwd");
		}
		if (!SeparateWindow && !Silent)
			CheckForKeyPressAfterCmd(r);
	}

	if (!Flags.Check(FCMDOBJ_LOCKUPDATEPANEL) && CtrlObject) {
		ShellUpdatePanels(CtrlObject->Cp()->ActivePanel, FALSE);
		CtrlObject->MainKeyBar->Refresh(Opt.ShowKeyBar);
	}

	return r;
}

const wchar_t *PrepareOSIfExist(const wchar_t *CmdLine)
{
	return L"";
}

FARString ExecuteCommandAndGrabItsOutput(FARString cmd, const char *cmd_stub)
{
	if (cmd.GetLength() == 0 && (cmd_stub == nullptr || strlen(cmd_stub)<=0) )
		return FARString();

	FARString strTempName;

	if (!FarMkTempEx(strTempName))
		return FARString();

	std::string exec_cmd =
			"echo Waiting command to complete...; "
			"echo You can use Ctrl+C to stop it, or Ctrl+Alt+C - to hardly terminate.; ";
	if (cmd.GetLength() != 0) {
		exec_cmd+= cmd.GetMB();
	} else {
		exec_cmd+= cmd_stub;
	}

	exec_cmd+= " >";
	exec_cmd+= strTempName.GetMB();
	exec_cmd+= " 2>&1";

	farExecuteA(exec_cmd.c_str(), EF_NOCMDPRINT);

	return strTempName;
}
