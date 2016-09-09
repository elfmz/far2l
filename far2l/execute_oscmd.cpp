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
#include "vtlog.h"
#include <vector>
#include <string>
#include <wordexp.h>

// explode cmdline to argv[] array
//
// Bash cmdlines cannot be correctly parsed that way because some bash constructions are not space-separated (such as `...`, $(...), cmd>/dev/null, cmd&)

// echo foo = bar       -> {'echo', 'foo', '=', 'bar'}
// echo foo=bar         -> {'echo', 'foo=bar'}
// echo "foo=long bar"  -> {'echo', 'foo=long bar'}
// echo "a\"b"	          -> {'echo', 'a"b'}
// echo 'a\\b'          -> {'echo', 'a\\b'}
// echo "a\\b"          -> {'echo', 'a\b'}
// echo "a"'!'"b"       -> {'echo', 'a!b'}
// echo "" '' 	          -> {'echo', '', ''}

std::vector<std::string> ExplodeCmdLine(const char *cmd_line) {
	std::vector<std::string> rc;
	fprintf(stderr, "ExplodeCmdLine('%s'): ", cmd_line);
	wordexp_t we = {};
	if (wordexp(cmd_line, &we, 0)==0) {
		for (size_t i = 0; i < we.we_wordc; ++i) {
			rc.push_back(we.we_wordv[i]);
			fprintf(stderr, "'%s' ", we.we_wordv[i]);
		}
		fprintf(stderr, "\n");
		wordfree(&we);
	} else
		perror("failed");
	return rc;
};


bool CommandLine::ProcessOSCommands(const wchar_t *CmdLine, bool SeparateWindow, bool &PrintCommand)
{
	std::vector<std::string> ecl = ExplodeCmdLine(Wide2MB(CmdLine).c_str());
	if (ecl.empty())
		return false;
		
	if (ecl[0] == "reset") {
		if (ecl.size() == 1) {
			ClearScreen(COL_COMMANDLINEUSERSCREEN);
			SaveBackground();
			VTLog::Reset();
			PrintCommand = false;
			return true;
		}
	} else if (ecl[0] == "export") {
		if (ecl.size() == 2) {
			size_t p = ecl[1].find('=');
			if (p != std::string::npos) {
				const std::string &name = ecl[1].substr(0, p);
				if (p == ecl[1].size() - 1)
					unsetenv(name.c_str());
				else
					setenv(name.c_str(), ecl[1].substr(p + 1, ecl[1].size() - p - 1).c_str(), 1);
				return true;
			}
		}
	} else if (ecl[0]=="cd") {
		if (ecl.size() == 2 ) {
			if (IntChDir(StrMB2Wide(ecl[1]).c_str(), true, false))
				return true;
		}
	} else if (ecl[0]=="pushd") {
		if (ecl.size() == 2 ) {
			PushPopRecord prec;
			prec.strName = strCurDir;
			if (IntChDir(StrMB2Wide(ecl[1]).c_str(), true, false)) {
				ppstack.Push(prec);
				setenv("FARDIRSTACK", Wide2MB(prec.strName.CPtr()).c_str(), 1);
				return true;				
			}
		}
	} else if (ecl[0]=="popd") {
		if (ecl.size() == 1 ) {
			PushPopRecord prec;

			if (ppstack.Pop(prec)) {
				int Ret = IntChDir(prec.strName, true, false);
				PushPopRecord *ptrprec=ppstack.Peek();
				if (ptrprec)
					setenv("FARDIRSTACK", Wide2MB(ptrprec->strName.CPtr()).c_str(), 1);
				else
					unsetenv("FARDIRSTACK");
				return Ret;
			}
		}
	}else if (ecl[0]=="exit") {
		if (ecl.size() == 1 ) {
			fprintf(stderr, "Use 'exit far' instead\n");
		} else if (ecl.size() == 2 && ecl[1]=="far") {
			FrameManager->ExitMainLoop(FALSE);
			return true;			
		}
	}

	return false;
}

BOOL CommandLine::IntChDir(const wchar_t *CmdLine,int ClosePlugin,bool Selent)
{
	Panel *SetPanel;
	SetPanel=CtrlObject->Cp()->ActivePanel;
	fprintf(stderr, "CommandLine::IntChDir: %ls\n", CmdLine);

	if (SetPanel->GetType()!=FILE_PANEL && CtrlObject->Cp()->GetAnotherPanel(SetPanel)->GetType()==FILE_PANEL)
		SetPanel=CtrlObject->Cp()->GetAnotherPanel(SetPanel);

	FARString strExpandedDir(CmdLine);
	Unquote(strExpandedDir);
	apiExpandEnvironmentStrings(strExpandedDir,strExpandedDir);

	if (SetPanel->GetMode()!=PLUGIN_PANEL && strExpandedDir.At(0) == L'~' && 
		((!strExpandedDir.At(1) && apiGetFileAttributes(strExpandedDir) == INVALID_FILE_ATTRIBUTES) || IsSlash(strExpandedDir.At(1))))
	{
		const char *home = getenv("HOME");
		if (home) {
			strExpandedDir.Replace(0, 1, MB2Wide(home).c_str());
		} else
			strExpandedDir.Replace(0, 1, L"/root");
	}

	if (wcspbrk(&strExpandedDir[HasPathPrefix(strExpandedDir)?4:0],L"?*")) // это маска?
	{
		FAR_FIND_DATA_EX wfd;

		if (apiGetFindDataEx(strExpandedDir, wfd))
		{
			size_t pos;

			if (FindLastSlash(pos,strExpandedDir))
				strExpandedDir.SetLength(pos+1);
			else
				strExpandedDir.Clear();

			strExpandedDir += wfd.strFileName;
		}
	}

	/* $ 15.11.2001 OT
		Сначала проверяем есть ли такая "обычная" директория.
		если уж нет, то тогда начинаем думать, что это директория плагинная
	*/
	DWORD DirAtt=apiGetFileAttributes(strExpandedDir);

	if (DirAtt!=INVALID_FILE_ATTRIBUTES && (DirAtt & FILE_ATTRIBUTE_DIRECTORY) && IsAbsolutePath(strExpandedDir))
	{
		SetPanel->SetCurDir(strExpandedDir,TRUE);
		return TRUE;
	}

	/* $ 20.09.2002 SKV
	  Это отключает возможность выполнять такие команды как:
	  cd net:server и cd ftp://server/dir
	  Так как под ту же гребёнку попадают и
	  cd s&r:, cd make: и т.д., которые к смене
	  каталога не имеют никакого отношения.
	*/
	/*
	if (CtrlObject->Plugins.ProcessCommandLine(ExpandedDir))
	{
	  //CmdStr.SetString(L"");
	  GotoXY(X1,Y1);
	  FS<<fmt::Width(X2-X1+1)<<L"";
	  Show();
	  return TRUE;
	}
	*/
	strExpandedDir.ReleaseBuffer();

	if (SetPanel->GetType()==FILE_PANEL && SetPanel->GetMode()==PLUGIN_PANEL)
	{
		SetPanel->SetCurDir(strExpandedDir,ClosePlugin);
		return TRUE;
	}

	if (FarChDir(strExpandedDir))
	{
		SetPanel->ChangeDirToCurrent();

		if (!SetPanel->IsVisible())
			SetPanel->SetTitle();
	}
	else
	{
		if (!Selent)
			Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MError),strExpandedDir,MSG(MOk));

		return FALSE;
	}

	return TRUE;
}
