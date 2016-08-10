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

int Execute(const wchar_t *CmdStr, bool AlwaysWaitFinish, bool SeparateWindow, bool DirectRun, bool FolderRun , bool WaitForIdle , bool Silent , bool RunAs)
{
//	fprintf(stderr, "TODO: Execute('" WS_FMT "')\n", CmdStr);
	ProcessShowClock++;
	CtrlObject->CmdLine->ShowBackground();
	CtrlObject->CmdLine->Redraw();
	CtrlObject->CmdLine->SetString(L"", TRUE);
	ScrBuf.Flush();
	DWORD saved_mode = 0;
	WINPORT(GetConsoleMode)(NULL, &saved_mode);
	WINPORT(SetConsoleMode)(NULL, saved_mode | 
		ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	VTShell_SendCommand(UTF16to8(CmdStr).c_str());
	WINPORT(SetConsoleMode)(NULL, saved_mode);
	ScrBuf.FillBuf();
	CtrlObject->CmdLine->SaveBackground();
	ProcessShowClock--;
	SetFarConsoleMode(TRUE);
	ScrBuf.Flush();
	

	
	
	return 1;
}

int CommandLine::CmdExecute(const wchar_t *CmdLine, bool AlwaysWaitFinish, bool SeparateWindow, bool DirectRun, bool WaitForIdle, bool Silent, bool RunAs)
{
	int r = Execute(CmdLine, AlwaysWaitFinish, SeparateWindow, DirectRun, false , WaitForIdle , Silent , RunAs);

	if (!Flags.Check(FCMDOBJ_LOCKUPDATEPANEL)) {
		ShellUpdatePanels(CtrlObject->Cp()->ActivePanel, FALSE);
		if (Opt.ShowKeyBar)
			CtrlObject->MainKeyBar->Show();
		
	}
	
	return r;
}


// Ïðîâåðèòü "Ýòî áàòíèê?"
bool IsBatchExtType(const wchar_t *ExtPtr)
{
	return false;
}

const wchar_t *PrepareOSIfExist(const wchar_t *CmdLine)
{
	return L"";
}

bool ProcessOSAliases(string &strStr)
{
	return false;
}
