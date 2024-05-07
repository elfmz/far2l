/*
ctrlobj.cpp

Управление остальными объектами, раздача сообщений клавиатуры и мыши
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

#include "ctrlobj.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "manager.hpp"
#include "cmdline.hpp"
#include "hilight.hpp"
#include "poscache.hpp"
#include "history.hpp"
#include "treelist.hpp"
#include "filefilter.hpp"
#include "filepanels.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "ConfigOptSaveLoad.hpp"
#include "fileowner.hpp"
#include "dirmix.hpp"
#include "console.hpp"

#include "farversion.h"

ControlObject *CtrlObject;

ControlObject::ControlObject()
	:
	FPanels(0),
	CmdLine(0),
	CmdHistory(0),
	FolderHistory(0),
	ViewHistory(0),
	MainKeyBar(0),
	TopMenuBar(0),
	HiFiles(0),
	ViewerPosCache(0),
	EditorPosCache(0)
{
	_OT(SysLog(L"[%p] ControlObject::ControlObject()", this));
	CtrlObject = this;
	HiFiles = new HighlightFiles;
	ViewerPosCache = new FilePositionCache(FPCK_VIEWER);
	EditorPosCache = new FilePositionCache(FPCK_EDITOR);
	FrameManager = new Manager;
	// Macro.LoadMacros();
	ApplyConfig();
	CmdHistory = new History(HISTORYTYPE_CMD, Opt.HistoryCount, "SavedHistory", &Opt.SaveHistory, false);
	FolderHistory = new History(HISTORYTYPE_FOLDER, Opt.FoldersHistoryCount, "SavedFolderHistory",
			&Opt.SaveFoldersHistory, true);
	ViewHistory = new History(HISTORYTYPE_VIEW, Opt.ViewHistoryCount, "SavedViewHistory",
			&Opt.SaveViewHistory, true);
	FolderHistory->SetAddMode(true, 2, true);
	ViewHistory->SetAddMode(true, 1, true);
}

void ControlObject::Init()
{
	TreeList::ClearCache(0);
	SetFarColor(COL_COMMANDLINEUSERSCREEN);
	GotoXY(0, ScrY - 3);
	ShowStartupBanner();
	GotoXY(0, ScrY - 2);
	MoveCursor(0, ScrY - 1);
	FPanels = new FilePanels();
	CmdLine = new CommandLine();
	CmdLine->SaveBackground(0, 0, ScrX, ScrY);
	this->MainKeyBar = &(FPanels->MainKeyBar);
	this->TopMenuBar = &(FPanels->TopMenuBar);
	FPanels->Init();
	FPanels->SetScreenPosition();

	if (Opt.ShowMenuBar)
		this->TopMenuBar->Show();

	// FPanels->Redraw();
	CmdLine->Show();

	this->MainKeyBar->Refresh(Opt.ShowKeyBar);

	FrameManager->InsertFrame(FPanels);
	FrameManager->PluginCommit();

	Cp()->LeftPanel->Update(0);
	Cp()->RightPanel->Update(0);

	Cp()->LeftPanel->GoToFile(Opt.strLeftCurFile);
	Cp()->RightPanel->GoToFile(Opt.strRightCurFile);

	FARString strStartCurDir;
	Cp()->ActivePanel->GetCurDir(strStartCurDir);
	FarChDir(strStartCurDir);
	Cp()->ActivePanel->SetFocus();
	{
		FARString strOldTitle;
		Console.GetTitle(strOldTitle);
		FrameManager->PluginCommit();
		Plugins.LoadPlugins();
		Console.SetTitle(strOldTitle);
	}
	Macro.LoadMacros();

	auto *CurFrame = FrameManager->GetCurrentFrame();
	if (LIKELY(CurFrame))
		CurFrame->Show();	// otherwise panels displayed empty on start sometimes
							/*
								FarChDir(StartCurDir);
							*/
							//_SVS(SysLog(L"ActivePanel->GetCurDir='%ls'",StartCurDir));
							//_SVS(char PPP[NM];Cp()->GetAnotherPanel(Cp()->ActivePanel)->GetCurDir(PPP);SysLog(L"AnotherPanel->GetCurDir='%ls'",PPP));
}

void ControlObject::CreateFilePanels()
{
	FPanels = new FilePanels();
}

ControlObject::~ControlObject()
{
	if (CriticalInternalError)
		return;

	_OT(SysLog(L"[%p] ControlObject::~ControlObject()", this));

	if (Cp() && Cp()->ActivePanel) {
		if (Opt.AutoSaveSetup)
			ConfigOptSave(false);

		if (Cp()->ActivePanel->GetMode() != PLUGIN_PANEL) {
			FARString strCurDir;
			Cp()->ActivePanel->GetCurDir(strCurDir);
			FolderHistory->AddToHistory(strCurDir);
		}
	}

	FrameManager->CloseAll();
	FPanels = nullptr;
	FileFilter::CloseFilter();
	delete CmdHistory;
	delete FolderHistory;
	delete ViewHistory;
	delete CmdLine;
	delete HiFiles;

	delete ViewerPosCache;
	delete EditorPosCache;

	delete FrameManager;
	TreeList::FlushCache();
	Lang.Close();
	CtrlObject = nullptr;
}

void ControlObject::ShowStartupBanner(LPCWSTR EmergencyMsg)
{
	std::vector<FARString> Lines;

	std::string tmp_mb;
	for (const char *p = Copyright; *p; ++p) {
		if (*p == '\n') {
			Lines.emplace_back(tmp_mb);
			tmp_mb.clear();
		} else {
			tmp_mb+= *p;
		}
	}
	if (!tmp_mb.empty()) {
		Lines.emplace_back(tmp_mb);
	}

	COORD Size{}, CursorPosition{};
	uint64_t SavedAttr{};
	Console.GetSize(Size);
	Console.GetCursorPosition(CursorPosition);
	Console.GetTextAttributes(SavedAttr);

	if (EmergencyMsg) {
		for (const auto &Line : Lines) {
			Console.Write(Line, static_cast<DWORD>(Line.GetLength()));
			CursorPosition.Y++;
			Console.SetCursorPosition(CursorPosition);
		}

		Console.SetTextAttributes(F_YELLOW | B_BLACK);
		Console.Write(EmergencyMsg, wcslen(EmergencyMsg));
		CursorPosition.Y++;
		Console.SetCursorPosition(CursorPosition);
		Console.SetTextAttributes(SavedAttr);

	} else {
		Lines.emplace_back();
		const size_t ConsoleHintsIndex = Lines.size();
		Lines.reserve(ConsoleHintsIndex + 12);
		Lines.emplace_back(Msg::VTStartTipNoCmdTitle);
		Lines.emplace_back(Msg::VTStartTipNoCmdCtrlO);
		Lines.emplace_back(Msg::VTStartTipNoCmdCtrlArrow);
		Lines.emplace_back(Msg::VTStartTipNoCmdShiftTAB);
		Lines.emplace_back(Msg::VTStartTipNoCmdFn);
		Lines.emplace_back(Msg::VTStartTipNoCmdMouse);
		Lines.emplace_back(Msg::VTStartTipPendCmdTitle);
		Lines.emplace_back(Msg::VTStartTipPendCmdFn);
		Lines.emplace_back(Msg::VTStartTipPendCmdCtrlAltC);
		Lines.emplace_back(Msg::VTStartTipPendCmdCtrlAltZ);
		Lines.emplace_back(Msg::VTStartTipPendCmdMouse);
		Lines.emplace_back(Msg::VTStartTipMouseSelect);

		const int FreeSpace = Size.Y - CursorPosition.Y - 1;
		const int LineCount = 4 + Lines.size();

		if (FreeSpace < LineCount)
			ScrollScreen(LineCount - FreeSpace);

		const auto SavedColor = GetColor();
		for (size_t i = 0; i < Lines.size(); ++i) {
			if (i >= ConsoleHintsIndex) {
				SetFarColor(Lines[i].Begins(L' ') ? COL_HELPTEXT : COL_HELPTOPIC);		// COL_HELPBOXTITLE
			}
			if (!Lines[i].IsEmpty()) {
				GotoXY(0, ScrY - (Lines.size() - i + 2));
				Text(Lines[i]);
			}
		}
		SetColor(SavedColor);
	}
}

FilePanels *ControlObject::Cp()
{
	return FPanels;
}
