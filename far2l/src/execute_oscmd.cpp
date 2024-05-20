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
#include "syslog.hpp"
#include "constitle.hpp"
#include "console.hpp"
#include "vtlog.h"
#include <vector>
#include <string>
#include <utils.h>

//
static bool MatchCommand(const std::string &cmd_line, const char *cmd)
{
	const size_t cmd_len = strlen(cmd);

	if (cmd_line.size() == cmd_len) {
		return cmd_line == cmd;
	}

	if (cmd_line.size() > cmd_len) {
		return memcmp(cmd_line.c_str(), cmd, cmd_len) == 0 && isspace(cmd_line[cmd_len]);
	}

	return false;
}

// explode cmdline to argv[] array
//
// Bash cmdlines cannot be correctly parsed that way because some bash constructions are not space-separated
// (such as `...`, $(...), cmd>/dev/null, cmd&)
// echo foo = bar       -> {'echo', 'foo', '=', 'bar'}
// echo foo=bar         -> {'echo', 'foo=bar'}
// echo "foo=long bar"  -> {'echo', 'foo=long bar'}
// echo "a\"b"	        -> {'echo', 'a"b'}
// echo 'a\\b'          -> {'echo', 'a\\b'}
// echo "a\\b"          -> {'echo', 'a\b'}
// echo "a"'!'"b"       -> {'echo', 'a!b'}
// echo "" '' 	        -> {'echo', '', ''}

bool CommandLine::ProcessOSCommands(const wchar_t *CmdLine, bool SeparateWindow, bool &PrintCommand)
{
	Environment::ExplodeCommandLine ecl(Wide2MB(CmdLine));
	if (ecl.empty())
		return false;

	if (ecl[0] == "reset") {
		if (ecl.size() == 1) {
			ClearScreen(FarColorToReal(COL_COMMANDLINEUSERSCREEN));
			SaveBackground();
			VTLog::Reset(NULL);
		}

	} else if (ecl[0] == "pushd") {
		if (PushDirStackSize < 10) {
			++PushDirStackSize;
		}

	} else if (ecl[0] == "popd") {
		if (PushDirStackSize > 0) {
			--PushDirStackSize;
		}
#if 0
	} else if (ecl[0]=="crash" && ecl.size() == 2 && ecl[1]=="far") {
		*(volatile int *)100 = 200;
#endif
	} else if (ecl[0] == "exit") {
		if (ecl.size() == 2 && ecl[1] == "far") {
			FrameManager->ExitMainLoop(FALSE);
			return true;
		}
		PushDirStackSize = 0;
	}

	return false;
}

BOOL CommandLine::IntChDir(const wchar_t *CmdLine, int ClosePlugin, bool Silent)
{
	Panel *SetPanel;
	SetPanel = CtrlObject->Cp()->ActivePanel;
	fprintf(stderr, "CommandLine::IntChDir: %ls\n", CmdLine);

	if (SetPanel->GetType() != FILE_PANEL
			&& CtrlObject->Cp()->GetAnotherPanel(SetPanel)->GetType() == FILE_PANEL)
		SetPanel = CtrlObject->Cp()->GetAnotherPanel(SetPanel);

	FARString strDir(CmdLine);

	/*
		$ 15.11.2001 OT
		Сначала проверяем есть ли такая "обычная" директория.
		если уж нет, то тогда начинаем думать, что это директория плагинная
	*/
	DWORD DirAtt = apiGetFileAttributes(strDir);

	if (DirAtt != INVALID_FILE_ATTRIBUTES && (DirAtt & FILE_ATTRIBUTE_DIRECTORY) && IsAbsolutePath(strDir)) {
		SetPanel->SetCurDir(strDir, TRUE);
		return TRUE;
	}

	/*
		$ 20.09.2002 SKV
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
		FS<<fmt::Expand(X2-X1+1)<<L"";
		Show();
		return TRUE;
	}
	*/

	if (SetPanel->GetType() == FILE_PANEL && SetPanel->GetMode() == PLUGIN_PANEL) {
		SetPanel->SetCurDir(strDir, ClosePlugin);
		return TRUE;
	}

	if (FarChDir(strDir)) {
		SetPanel->ChangeDirToCurrent();

		if (!SetPanel->IsVisible())
			SetPanel->SetTitle();
	} else {
		if (!Silent)
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, strDir, Msg::Ok);

		return FALSE;
	}

	return TRUE;
}
