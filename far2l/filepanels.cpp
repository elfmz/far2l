/*
filepanels.cpp

Файловые панели
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

#include "filepanels.hpp"
#include "keys.hpp"
#include "macroopcode.hpp"
#include "lang.hpp"
#include "ctrlobj.hpp"
#include "filelist.hpp"
#include "rdrwdsk.hpp"
#include "cmdline.hpp"
#include "treelist.hpp"
#include "qview.hpp"
#include "infolist.hpp"
#include "help.hpp"
#include "filefilter.hpp"
#include "findfile.hpp"
#include "savescr.hpp"
#include "manager.hpp"
#include "syslog.hpp"
#include "options.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "interf.hpp"

FilePanels::FilePanels():
	LastLeftFilePanel(0),
	LastRightFilePanel(0),
	LeftPanel(CreatePanel(Opt.LeftPanel.Type)),
	RightPanel(CreatePanel(Opt.RightPanel.Type)),
	ActivePanel(0),
	LastLeftType(0),
	LastRightType(0),
	LeftStateBeforeHide(0),
	RightStateBeforeHide(0)
{
	_OT(SysLog(L"[%p] FilePanels::FilePanels()", this));
	MacroMode = MACRO_SHELL;
	KeyBarVisible = Opt.ShowKeyBar;
//  SetKeyBar(&MainKeyBar);
//  _D(SysLog(L"MainKeyBar=0x%p",&MainKeyBar));
}

static void PrepareOptFolder(FARString &strSrc, int IsLocalPath_FarPath)
{
	if (strSrc.IsEmpty())
	{
		strSrc = g_strFarPath;
		DeleteEndSlash(strSrc);
	}
	else
	{
		apiExpandEnvironmentStrings(strSrc, strSrc);
	}

	if (!StrCmp(strSrc,L"/"))
	{
		strSrc = g_strFarPath;

		if (IsLocalPath_FarPath)
		{
			strSrc.SetLength(2);
			strSrc += L"/";
		}
	}
	else
	{
		CheckShortcutFolder(&strSrc,FALSE,TRUE);
	}

	//ConvertNameToFull(strSrc,strSrc);
}

void FilePanels::Init()
{
	SetPanelPositions(FileList::IsModeFullScreen(Opt.LeftPanel.ViewMode),
	                  FileList::IsModeFullScreen(Opt.RightPanel.ViewMode));
	LeftPanel->SetViewMode(Opt.LeftPanel.ViewMode);
	RightPanel->SetViewMode(Opt.RightPanel.ViewMode);
	LeftPanel->SetSortMode(Opt.LeftPanel.SortMode);
	RightPanel->SetSortMode(Opt.RightPanel.SortMode);
	LeftPanel->SetNumericSort(Opt.LeftPanel.NumericSort);
	RightPanel->SetNumericSort(Opt.RightPanel.NumericSort);
	LeftPanel->SetCaseSensitiveSort(Opt.LeftPanel.CaseSensitiveSort);
	RightPanel->SetCaseSensitiveSort(Opt.RightPanel.CaseSensitiveSort);
	LeftPanel->SetSortOrder(Opt.LeftPanel.SortOrder);
	RightPanel->SetSortOrder(Opt.RightPanel.SortOrder);
	LeftPanel->SetSortGroups(Opt.LeftPanel.SortGroups);
	RightPanel->SetSortGroups(Opt.RightPanel.SortGroups);
	LeftPanel->SetSelectedFirstMode(Opt.LeftSelectedFirst);
	RightPanel->SetSelectedFirstMode(Opt.RightSelectedFirst);
	LeftPanel->SetDirectoriesFirst(Opt.LeftPanel.DirectoriesFirst);
	RightPanel->SetDirectoriesFirst(Opt.RightPanel.DirectoriesFirst);
	SetCanLoseFocus(TRUE);
	Panel *PassivePanel=nullptr;
	int PassiveIsLeftFlag=TRUE;

	if (Opt.LeftPanel.Focus)
	{
		ActivePanel=LeftPanel;
		PassivePanel=RightPanel;
		PassiveIsLeftFlag=FALSE;
	}
	else
	{
		ActivePanel=RightPanel;
		PassivePanel=LeftPanel;
		PassiveIsLeftFlag=TRUE;
	}

	ActivePanel->SetFocus();
	// пытаемся избавится от зависания при запуске
	int IsLocalPath_FarPath=IsLocalPath(g_strFarPath);
	PrepareOptFolder(Opt.strLeftFolder,IsLocalPath_FarPath);
	PrepareOptFolder(Opt.strRightFolder,IsLocalPath_FarPath);

	if (Opt.AutoSaveSetup || !Opt.SetupArgv)
	{
		if (apiGetFileAttributes(Opt.strLeftFolder)!=INVALID_FILE_ATTRIBUTES)
			LeftPanel->InitCurDir(Opt.strLeftFolder);

		if (apiGetFileAttributes(Opt.strRightFolder)!=INVALID_FILE_ATTRIBUTES)
			RightPanel->InitCurDir(Opt.strRightFolder);
	}

	if (!Opt.AutoSaveSetup)
	{
		if (Opt.SetupArgv >= 1)
		{
			if (ActivePanel==RightPanel)
			{
				if (apiGetFileAttributes(Opt.strRightFolder)!=INVALID_FILE_ATTRIBUTES)
					RightPanel->InitCurDir(Opt.strRightFolder);
			}
			else
			{
				if (apiGetFileAttributes(Opt.strLeftFolder)!=INVALID_FILE_ATTRIBUTES)
					LeftPanel->InitCurDir(Opt.strLeftFolder);
			}

			if (Opt.SetupArgv == 2)
			{
				if (ActivePanel==LeftPanel)
				{
					if (apiGetFileAttributes(Opt.strRightFolder)!=INVALID_FILE_ATTRIBUTES)
						RightPanel->InitCurDir(Opt.strRightFolder);
				}
				else
				{
					if (apiGetFileAttributes(Opt.strLeftFolder)!=INVALID_FILE_ATTRIBUTES)
						LeftPanel->InitCurDir(Opt.strLeftFolder);
				}
			}
		}

		const wchar_t *PassiveFolder=PassiveIsLeftFlag?Opt.strLeftFolder:Opt.strRightFolder;

		if (Opt.SetupArgv < 2 && *PassiveFolder && (apiGetFileAttributes(PassiveFolder)!=INVALID_FILE_ATTRIBUTES))
		{
			PassivePanel->InitCurDir(PassiveFolder);
		}
	}

#if 1

	//! Вначале "показываем" пассивную панель
	if (PassiveIsLeftFlag)
	{
		if (Opt.LeftPanel.Visible)
		{
			LeftPanel->Show();
		}

		if (Opt.RightPanel.Visible)
		{
			RightPanel->Show();
		}
	}
	else
	{
		if (Opt.RightPanel.Visible)
		{
			RightPanel->Show();
		}

		if (Opt.LeftPanel.Visible)
		{
			LeftPanel->Show();
		}
	}

#endif

	// при понашенных панелях не забыть бы выставить корректно каталог в CmdLine
	if (!Opt.RightPanel.Visible && !Opt.LeftPanel.Visible)
	{
		CtrlObject->CmdLine->SetCurDir(PassiveIsLeftFlag?Opt.strRightFolder:Opt.strLeftFolder);
	}

	SetKeyBar(&MainKeyBar);
	MainKeyBar.SetOwner(this);
}

FilePanels::~FilePanels()
{
	_OT(SysLog(L"[%p] FilePanels::~FilePanels()", this));

	if (LastLeftFilePanel!=LeftPanel && LastLeftFilePanel!=RightPanel)
		DeletePanel(LastLeftFilePanel);

	if (LastRightFilePanel!=LeftPanel && LastRightFilePanel!=RightPanel)
		DeletePanel(LastRightFilePanel);

	DeletePanel(LeftPanel);
	LeftPanel=nullptr;
	DeletePanel(RightPanel);
	RightPanel=nullptr;
}

void FilePanels::SetPanelPositions(int LeftFullScreen,int RightFullScreen)
{
	if (Opt.WidthDecrement < -(ScrX/2-10))
		Opt.WidthDecrement=-(ScrX/2-10);

	if (Opt.WidthDecrement > (ScrX/2-10))
		Opt.WidthDecrement=(ScrX/2-10);

	Opt.LeftHeightDecrement=Max(0,Min(Opt.LeftHeightDecrement,ScrY-7));
	Opt.RightHeightDecrement=Max(0,Min(Opt.RightHeightDecrement,ScrY-7));

	if (LeftFullScreen)
	{
		LeftPanel->SetPosition(0,Opt.ShowMenuBar?1:0,ScrX,ScrY-1-(Opt.ShowKeyBar)-Opt.LeftHeightDecrement);
		LeftPanel->ViewSettings.FullScreen=1;
	}
	else
	{
		LeftPanel->SetPosition(0,Opt.ShowMenuBar?1:0,ScrX/2-Opt.WidthDecrement,ScrY-1-(Opt.ShowKeyBar)-Opt.LeftHeightDecrement);
	}

	if (RightFullScreen)
	{
		RightPanel->SetPosition(0,Opt.ShowMenuBar?1:0,ScrX,ScrY-1-(Opt.ShowKeyBar)-Opt.RightHeightDecrement);
		RightPanel->ViewSettings.FullScreen=1;
	}
	else
	{
		RightPanel->SetPosition(ScrX/2+1-Opt.WidthDecrement,Opt.ShowMenuBar?1:0,ScrX,ScrY-1-(Opt.ShowKeyBar)-Opt.RightHeightDecrement);
	}
}

void FilePanels::SetScreenPosition()
{
	_OT(SysLog(L"[%p] FilePanels::SetScreenPosition() {%d, %d - %d, %d}", this,X1,Y1,X2,Y2));
	CtrlObject->CmdLine->SetPosition(0,ScrY-(Opt.ShowKeyBar),ScrX-1,ScrY-(Opt.ShowKeyBar));
	TopMenuBar.SetPosition(0,0,ScrX,0);
	MainKeyBar.SetPosition(0,ScrY,ScrX,ScrY);
	SetPanelPositions(LeftPanel->IsFullScreen(),RightPanel->IsFullScreen());
	SetPosition(0,0,ScrX,ScrY);
}

void FilePanels::RedrawKeyBar()
{
	ActivePanel->UpdateKeyBar();
	MainKeyBar.Redraw();
}


Panel* FilePanels::CreatePanel(int Type)
{
	Panel *pResult = nullptr;

	switch (Type)
	{
		case FILE_PANEL:
			pResult = new FileList;
			break;
		case TREE_PANEL:
			pResult = new TreeList;
			break;
		case QVIEW_PANEL:
			pResult = new QuickView;
			break;
		case INFO_PANEL:
			pResult = new InfoList;
			break;
	}

	if (pResult)
		pResult->SetOwner(this);

	return pResult;
}


void FilePanels::DeletePanel(Panel *Deleted)
{
	if (!Deleted)
		return;

	if (Deleted==LastLeftFilePanel)
		LastLeftFilePanel=nullptr;

	if (Deleted==LastRightFilePanel)
		LastRightFilePanel=nullptr;

	delete Deleted;
}

int FilePanels::SetAnhoterPanelFocus()
{
	int Ret=FALSE;

	if (ActivePanel==LeftPanel)
	{
		if (RightPanel->IsVisible())
		{
			RightPanel->SetFocus();
			Ret=TRUE;
		}
	}
	else
	{
		if (LeftPanel->IsVisible())
		{
			LeftPanel->SetFocus();
			Ret=TRUE;
		}
	}

	return Ret;
}


int FilePanels::SwapPanels()
{
	int Ret=FALSE; // это значит ни одна из панелей не видна

	if (LeftPanel->IsVisible() || RightPanel->IsVisible())
	{
		int XL1,YL1,XL2,YL2;
		int XR1,YR1,XR2,YR2;
		LeftPanel->GetPosition(XL1,YL1,XL2,YL2);
		RightPanel->GetPosition(XR1,YR1,XR2,YR2);

		if (!LeftPanel->ViewSettings.FullScreen || !RightPanel->ViewSettings.FullScreen)
		{
			Opt.WidthDecrement=-Opt.WidthDecrement;

			Opt.LeftHeightDecrement^=Opt.RightHeightDecrement;
			Opt.RightHeightDecrement=Opt.LeftHeightDecrement^Opt.RightHeightDecrement;
			Opt.LeftHeightDecrement^=Opt.RightHeightDecrement;

		}

		Panel *Swap;
		int SwapType;
		Swap=LeftPanel;
		LeftPanel=RightPanel;
		RightPanel=Swap;
		Swap=LastLeftFilePanel;
		LastLeftFilePanel=LastRightFilePanel;
		LastRightFilePanel=Swap;
		SwapType=LastLeftType;
		LastLeftType=LastRightType;
		LastRightType=SwapType;
		FileFilter::SwapFilter();
		Ret=TRUE;
	}
	SetScreenPosition();
	FrameManager->RefreshFrame();
	return Ret;
}

int64_t FilePanels::VMProcess(int OpCode,void *vParam,int64_t iParam)
{
	return ActivePanel->VMProcess(OpCode,vParam,iParam);
}

int FilePanels::ProcessKey(int Key)
{
	if (!Key)
		return TRUE;

	if ((Key==KEY_CTRLLEFT || Key==KEY_CTRLRIGHT || Key==KEY_CTRLNUMPAD4 || Key==KEY_CTRLNUMPAD6
	        /* || Key==KEY_CTRLUP   || Key==KEY_CTRLDOWN || Key==KEY_CTRLNUMPAD8 || Key==KEY_CTRLNUMPAD2 */) &&
	        (CtrlObject->CmdLine->GetLength()>0 ||
	         (!LeftPanel->IsVisible() && !RightPanel->IsVisible())))
	{
		CtrlObject->CmdLine->ProcessKey(Key);
		return TRUE;
	}

	switch (Key)
	{
		case KEY_F1:
		{
			if (!ActivePanel->ProcessKey(KEY_F1))
			{
				Help Hlp(L"Contents");
			}

			return TRUE;
		}
		case KEY_TAB:
		{
			if (!SetAnhoterPanelFocus()) {
				CtrlObject->CmdLine->ProcessKey(Key);
			}
			break;
		}
		case KEY_CTRLF1:
		{
			if (LeftPanel->IsVisible())
			{
				LeftPanel->Hide();

				if (RightPanel->IsVisible())
					RightPanel->SetFocus();
			}
			else
			{
				if (!RightPanel->IsVisible())
					LeftPanel->SetFocus();

				LeftPanel->Show();
			}

			Redraw();
			break;
		}
		case KEY_CTRLF2:
		{
			if (RightPanel->IsVisible())
			{
				RightPanel->Hide();

				if (LeftPanel->IsVisible())
					LeftPanel->SetFocus();
			}
			else
			{
				if (!LeftPanel->IsVisible())
					RightPanel->SetFocus();

				RightPanel->Show();
			}

			Redraw();
			break;
		}
		case KEY_CTRLB:
		{
			Opt.ShowKeyBar=!Opt.ShowKeyBar;
			KeyBarVisible = Opt.ShowKeyBar;

			if (!KeyBarVisible)
				MainKeyBar.Hide();

			SetScreenPosition();
			FrameManager->RefreshFrame();
			break;
		}
		case KEY_CTRLL:
		case KEY_CTRLQ:
		case KEY_CTRLT:
		{
			if (ActivePanel->IsVisible())
			{
				Panel *AnotherPanel=GetAnotherPanel(ActivePanel);
				int NewType;

				if (Key==KEY_CTRLL)
					NewType=INFO_PANEL;
				else if (Key==KEY_CTRLQ)
					NewType=QVIEW_PANEL;
				else
					NewType=TREE_PANEL;

				if (ActivePanel->GetType()==NewType)
					AnotherPanel=ActivePanel;

				if (!AnotherPanel->ProcessPluginEvent(FE_CLOSE,nullptr))
				{
					if (AnotherPanel->GetType()==NewType)
						/* $ 19.09.2000 IS
						  Повторное нажатие на ctrl-l|q|t всегда включает файловую панель
						*/
						AnotherPanel=ChangePanel(AnotherPanel,FILE_PANEL,FALSE,FALSE);
					else
						AnotherPanel=ChangePanel(AnotherPanel,NewType,FALSE,FALSE);

					/* $ 07.09.2001 VVM
					  ! При возврате из CTRL+Q, CTRL+L восстановим каталог, если активная панель - дерево. */
					if (ActivePanel->GetType() == TREE_PANEL)
					{
						FARString strCurDir;
						ActivePanel->GetCurDir(strCurDir);
						AnotherPanel->SetCurDir(strCurDir, TRUE);
						AnotherPanel->Update(0);
					}
					else
						AnotherPanel->Update(UPDATE_KEEP_SELECTION);

					AnotherPanel->Show();
				}

				ActivePanel->SetFocus();
			}

			break;
		}
		case KEY_CTRLO:
		{
			{
				int LeftVisible=LeftPanel->IsVisible();
				int RightVisible=RightPanel->IsVisible();
				int HideState=!LeftVisible && !RightVisible;

				if (!HideState)
				{
					LeftStateBeforeHide=LeftVisible;
					RightStateBeforeHide=RightVisible;
					LeftPanel->Hide();
					RightPanel->Hide();
					FrameManager->RefreshFrame();
				}
				else
				{
					if (!LeftStateBeforeHide && !RightStateBeforeHide)
						LeftStateBeforeHide=RightStateBeforeHide=TRUE;

					if (LeftStateBeforeHide)
						LeftPanel->Show();

					if (RightStateBeforeHide)
						RightPanel->Show();

					if (!ActivePanel->IsVisible())
					{
						if (ActivePanel == RightPanel)
							LeftPanel->SetFocus();
						else
							RightPanel->SetFocus();
					}
				}
			}
			break;
		}
		case KEY_CTRLP:
		{
			if (ActivePanel->IsVisible())
			{
				Panel *AnotherPanel=GetAnotherPanel(ActivePanel);

				if (AnotherPanel->IsVisible())
					AnotherPanel->Hide();
				else
					AnotherPanel->Show();

				CtrlObject->CmdLine->Redraw();
			}

			FrameManager->RefreshFrame();
			break;
		}
		case KEY_CTRLI:
		{
			ActivePanel->EditFilter();
			return TRUE;
		}
		case KEY_CTRLU:
		{
			if (!LeftPanel->IsVisible() && !RightPanel->IsVisible())
				CtrlObject->CmdLine->ProcessKey(Key);
			else
				SwapPanels();

			break;
		}
		/* $ 08.04.2002 IS
		   При смене диска установим принудительно текущий каталог на активной
		   панели, т.к. система не знает ничего о том, что у Фара две панели, и
		   текущим для системы после смены диска может быть каталог и на пассивной
		   панели
		*/
		case KEY_ALTF1:
		{
			LeftPanel->ChangeDisk();

			if (ActivePanel!=LeftPanel)
				ActivePanel->SetCurPath();

			break;
		}
		case KEY_ALTF2:
		{
			RightPanel->ChangeDisk();

			if (ActivePanel!=RightPanel)
				ActivePanel->SetCurPath();

			break;
		}
		case KEY_ALTF7:
		{
			{
				FindFiles FindFiles;
			}
			break;
		}
		case KEY_CTRLUP:  case KEY_CTRLNUMPAD8:
		{
			bool Set=false;
			if (Opt.LeftHeightDecrement<ScrY-7)
			{
				Opt.LeftHeightDecrement++;
				Set=true;
			}
			if (Opt.RightHeightDecrement<ScrY-7)
			{
				Opt.RightHeightDecrement++;
				Set=true;
			}
			if(Set)
			{
				SetScreenPosition();
				FrameManager->RefreshFrame();
			}

			break;
		}
		case KEY_CTRLDOWN:  case KEY_CTRLNUMPAD2:
		{
			bool Set=false;
			if (Opt.LeftHeightDecrement>0)
			{
				Opt.LeftHeightDecrement--;
				Set=true;
			}
			if (Opt.RightHeightDecrement>0)
			{
				Opt.RightHeightDecrement--;
				Set=true;
			}
			if(Set)
			{
				SetScreenPosition();
				FrameManager->RefreshFrame();
			}

			break;
		}

		case KEY_CTRLSHIFTUP:  case KEY_CTRLSHIFTNUMPAD8:
		{
			int& HeightDecrement=(ActivePanel==LeftPanel)?Opt.LeftHeightDecrement:Opt.RightHeightDecrement;
			if (HeightDecrement<ScrY-7)
			{
				HeightDecrement++;
				SetScreenPosition();
				FrameManager->RefreshFrame();
			}
			break;
		}

		case KEY_CTRLSHIFTDOWN:  case KEY_CTRLSHIFTNUMPAD2:
		{
			int& HeightDecrement=(ActivePanel==LeftPanel)?Opt.LeftHeightDecrement:Opt.RightHeightDecrement;
			if (HeightDecrement>0)
			{
				HeightDecrement--;
				SetScreenPosition();
				FrameManager->RefreshFrame();
			}
			break;
		}

		case KEY_CTRLLEFT: case KEY_CTRLNUMPAD4:
		{
			if (Opt.WidthDecrement<ScrX/2-10)
			{
				Opt.WidthDecrement++;
				SetScreenPosition();
				FrameManager->RefreshFrame();
			}

			break;
		}
		case KEY_CTRLRIGHT: case KEY_CTRLNUMPAD6:
		{
			if (Opt.WidthDecrement>-(ScrX/2-10))
			{
				Opt.WidthDecrement--;
				SetScreenPosition();
				FrameManager->RefreshFrame();
			}

			break;
		}
		case KEY_CTRLCLEAR:
		{
			if (Opt.WidthDecrement)
			{
				Opt.WidthDecrement=0;
				SetScreenPosition();
				FrameManager->RefreshFrame();
			}

			break;
		}
		case KEY_CTRLALTCLEAR:
		{
			bool Set=false;
			if (Opt.LeftHeightDecrement)
			{
				Opt.LeftHeightDecrement=0;
				Set=true;
			}
			if (Opt.RightHeightDecrement)
			{
				Opt.RightHeightDecrement=0;
				Set=true;
			}
			if(Set)
			{
				SetScreenPosition();
				FrameManager->RefreshFrame();
			}

			break;
		}
		case KEY_F9:
		{
			ShellOptions(0,nullptr);
			return TRUE;
		}
		case KEY_SHIFTF10:
		{
			ShellOptions(1,nullptr);
			return TRUE;
		}
		default:
		{
			if (Key >= KEY_CTRL0 && Key <= KEY_CTRL9)
				ChangePanelViewMode(ActivePanel,Key-KEY_CTRL0,TRUE);
			else if (!ActivePanel->ProcessKey(Key))
				CtrlObject->CmdLine->ProcessKey(Key);

			break;
		}
	}

	return TRUE;
}

int FilePanels::ChangePanelViewMode(Panel *Current,int Mode,BOOL RefreshFrame)
{
	if (Current && Mode >= VIEW_0 && Mode <= VIEW_9)
	{
		Current->SetViewMode(Mode);
		Current=ChangePanelToFilled(Current,FILE_PANEL);
		Current->SetViewMode(Mode);
		// ВНИМАНИЕ! Костыль! Но Работает!
		SetScreenPosition();

		if (RefreshFrame)
			FrameManager->RefreshFrame();

		return TRUE;
	}

	return FALSE;
}

Panel* FilePanels::ChangePanelToFilled(Panel *Current,int NewType)
{
	if (Current->GetType()!=NewType && !Current->ProcessPluginEvent(FE_CLOSE,nullptr))
	{
		Current->Hide();
		Current=ChangePanel(Current,NewType,FALSE,FALSE);
		Current->Update(0);
		Current->Show();

		if (!GetAnotherPanel(Current)->GetFocus())
			Current->SetFocus();
	}

	return(Current);
}

Panel* FilePanels::GetAnotherPanel(Panel *Current)
{
	if (Current==LeftPanel)
		return(RightPanel);
	else
		return(LeftPanel);
}


Panel* FilePanels::ChangePanel(Panel *Current,int NewType,int CreateNew,int Force)
{
	Panel *NewPanel;
	SaveScreen *SaveScr=nullptr;
	// OldType не инициализировался...
	int OldType=Current->GetType(),X1,Y1,X2,Y2;
	int OldViewMode,OldSortMode,OldSortOrder,OldSortGroups,OldSelectedFirst,OldDirectoriesFirst;
	int OldPanelMode,LeftPosition,ChangePosition,OldNumericSort,OldCaseSensitiveSort;
	int OldFullScreen,OldFocus,UseLastPanel=0;
	OldPanelMode=Current->GetMode();

	if (!Force && NewType==OldType && OldPanelMode==NORMAL_PANEL)
		return(Current);

	OldViewMode=Current->GetPrevViewMode();
	OldFullScreen=Current->IsFullScreen();
	OldSortMode=Current->GetPrevSortMode();
	OldSortOrder=Current->GetPrevSortOrder();
	OldNumericSort=Current->GetPrevNumericSort();
	OldCaseSensitiveSort=Current->GetPrevCaseSensitiveSort();
	OldSortGroups=Current->GetSortGroups();
	OldFocus=Current->GetFocus();
	OldSelectedFirst=Current->GetSelectedFirstMode();
	OldDirectoriesFirst=Current->GetPrevDirectoriesFirst();
	LeftPosition=(Current==LeftPanel);
	Panel *(&LastFilePanel)=LeftPosition ? LastLeftFilePanel:LastRightFilePanel;
	Current->GetPosition(X1,Y1,X2,Y2);
	ChangePosition=((OldType==FILE_PANEL && NewType!=FILE_PANEL &&
	                 OldFullScreen) || (NewType==FILE_PANEL &&
	                                    ((OldFullScreen && !FileList::IsModeFullScreen(OldViewMode)) ||
	                                     (!OldFullScreen && FileList::IsModeFullScreen(OldViewMode)))));

	if (!ChangePosition)
	{
		SaveScr=Current->SaveScr;
		Current->SaveScr=nullptr;
	}

	if (OldType==FILE_PANEL && NewType!=FILE_PANEL)
	{
		delete Current->SaveScr;
		Current->SaveScr=nullptr;

		if (LastFilePanel!=Current)
		{
			DeletePanel(LastFilePanel);
			LastFilePanel=Current;
		}

		LastFilePanel->Hide();

		if (LastFilePanel->SaveScr)
		{
			LastFilePanel->SaveScr->Discard();
			delete LastFilePanel->SaveScr;
			LastFilePanel->SaveScr=nullptr;
		}
	}
	else
	{
		Current->Hide();
		DeletePanel(Current);

		if (OldType==FILE_PANEL && NewType==FILE_PANEL)
		{
			DeletePanel(LastFilePanel);
			LastFilePanel=nullptr;
		}
	}

	if (!CreateNew && NewType==FILE_PANEL && LastFilePanel)
	{
		int LastX1,LastY1,LastX2,LastY2;
		LastFilePanel->GetPosition(LastX1,LastY1,LastX2,LastY2);

		if (LastFilePanel->IsFullScreen())
			LastFilePanel->SetPosition(LastX1,Y1,LastX2,Y2);
		else
			LastFilePanel->SetPosition(X1,Y1,X2,Y2);

		NewPanel=LastFilePanel;

		if (!ChangePosition)
		{
			if ((NewPanel->IsFullScreen() && !OldFullScreen) ||
			        (!NewPanel->IsFullScreen() && OldFullScreen))
			{
				Panel *AnotherPanel=GetAnotherPanel(Current);

				if (SaveScr && AnotherPanel->IsVisible() &&
				        AnotherPanel->GetType()==FILE_PANEL && AnotherPanel->IsFullScreen())
					SaveScr->Discard();

				delete SaveScr;
			}
			else
				NewPanel->SaveScr=SaveScr;
		}

		if (!OldFocus && NewPanel->GetFocus())
			NewPanel->KillFocus();

		UseLastPanel=TRUE;
	}
	else
		NewPanel=CreatePanel(NewType);

	if (Current==ActivePanel)
		ActivePanel=NewPanel;

	if (LeftPosition)
	{
		LeftPanel=NewPanel;
		LastLeftType=OldType;
	}
	else
	{
		RightPanel=NewPanel;
		LastRightType=OldType;
	}

	if (!UseLastPanel)
	{
		if (ChangePosition)
		{
			if (LeftPosition)
			{
				NewPanel->SetPosition(0,Y1,ScrX/2-Opt.WidthDecrement,Y2);
				RightPanel->Redraw();
			}
			else
			{
				NewPanel->SetPosition(ScrX/2+1-Opt.WidthDecrement,Y1,ScrX,Y2);
				LeftPanel->Redraw();
			}
		}
		else
		{
			NewPanel->SaveScr=SaveScr;
			NewPanel->SetPosition(X1,Y1,X2,Y2);
		}

		NewPanel->SetSortMode(OldSortMode);
		NewPanel->SetSortOrder(OldSortOrder);
		NewPanel->SetNumericSort(OldNumericSort);
		NewPanel->SetCaseSensitiveSort(OldCaseSensitiveSort);
		NewPanel->SetSortGroups(OldSortGroups);
		NewPanel->SetPrevViewMode(OldViewMode);
		NewPanel->SetViewMode(OldViewMode);
		NewPanel->SetSelectedFirstMode(OldSelectedFirst);
		NewPanel->SetDirectoriesFirst(OldDirectoriesFirst);
	}

	return(NewPanel);
}

int  FilePanels::GetTypeAndName(FARString &strType, FARString &strName)
{
	strType = MSG(MScreensPanels);
	FARString strFullName;

	switch (ActivePanel->GetType())
	{
		case TREE_PANEL:
		case QVIEW_PANEL:
		case FILE_PANEL:
		case INFO_PANEL:
			ActivePanel->GetCurName(strFullName);
			ConvertNameToFull(strFullName, strFullName);
			break;
	}

	strName = strFullName;
	return(MODALTYPE_PANELS);
}

void FilePanels::OnChangeFocus(int f)
{
	_OT(SysLog(L"FilePanels::OnChangeFocus(%i)",f));

	/* $ 20.06.2001 tran
	   баг с отрисовкой при копировании и удалении
	   не учитывался LockRefreshCount */
	if (f)
	{
		/*$ 22.06.2001 SKV
		  + update панелей при получении фокуса
		*/
		CtrlObject->Cp()->GetAnotherPanel(ActivePanel)->UpdateIfChanged(UIC_UPDATE_FORCE_NOTIFICATION);
		ActivePanel->UpdateIfChanged(UIC_UPDATE_FORCE_NOTIFICATION);
		/* $ 13.04.2002 KM
		  ! ??? Я не понял зачем здесь Redraw, если
		    Redraw вызывается следом во Frame::OnChangeFocus.
		*/
//    Redraw();
		Frame::OnChangeFocus(1);
	}
}

void FilePanels::DisplayObject()
{
//  if ( !Focus )
//      return;
	_OT(SysLog(L"[%p] FilePanels::Redraw() {%d, %d - %d, %d}", this,X1,Y1,X2,Y2));
	CtrlObject->CmdLine->ShowBackground();

	if (Opt.ShowMenuBar)
		CtrlObject->TopMenuBar->Show();

	CtrlObject->CmdLine->Show();

	if (Opt.ShowKeyBar)
		MainKeyBar.Show();
	else if (MainKeyBar.IsVisible())
		MainKeyBar.Hide();

	KeyBarVisible=Opt.ShowKeyBar;
#if 1

	if (LeftPanel->IsVisible())
		LeftPanel->Show();

	if (RightPanel->IsVisible())
		RightPanel->Show();

#else
	Panel *PassivePanel=nullptr;
	int PassiveIsLeftFlag=TRUE;

	if (Opt.LeftPanel.Focus)
	{
		ActivePanel=LeftPanel;
		PassivePanel=RightPanel;
		PassiveIsLeftFlag=FALSE;
	}
	else
	{
		ActivePanel=RightPanel;
		PassivePanel=LeftPanel;
		PassiveIsLeftFlag=TRUE;
	}

	//! Вначале "показываем" пассивную панель
	if (PassiveIsLeftFlag)
	{
		if (Opt.LeftPanel.Visible)
		{
			LeftPanel->Show();
		}

		if (Opt.RightPanel.Visible)
		{
			RightPanel->Show();
		}
	}
	else
	{
		if (Opt.RightPanel.Visible)
		{
			RightPanel->Show();
		}

		if (Opt.LeftPanel.Visible)
		{
			LeftPanel->Show();
		}
	}

#endif
}

int  FilePanels::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (!ActivePanel->ProcessMouse(MouseEvent))
		if (!GetAnotherPanel(ActivePanel)->ProcessMouse(MouseEvent))
			if (!MainKeyBar.ProcessMouse(MouseEvent))
				CtrlObject->CmdLine->ProcessMouse(MouseEvent);

	return TRUE;
}

void FilePanels::ShowConsoleTitle()
{
	if (ActivePanel)
		ActivePanel->SetTitle();
}

void FilePanels::ResizeConsole()
{
	Frame::ResizeConsole();
	CtrlObject->CmdLine->ResizeConsole();
	MainKeyBar.ResizeConsole();
	TopMenuBar.ResizeConsole();
	SetScreenPosition();
	_OT(SysLog(L"[%p] FilePanels::ResizeConsole() {%d, %d - %d, %d}", this,X1,Y1,X2,Y2));
}

int FilePanels::FastHide()
{
	return Opt.AllCtrlAltShiftRule & CASR_PANEL;
}

void FilePanels::Refresh()
{
	/*$ 31.07.2001 SKV
	  Вызовем так, а не Frame::OnChangeFocus,
	  который из этого и позовётся.
	*/
	//Frame::OnChangeFocus(1);
	OnChangeFocus(1);
}

void FilePanels::GoToFile(const wchar_t *FileName)
{
	if (FirstSlash(FileName))
	{
		FARString ADir,PDir;
		Panel *PassivePanel = GetAnotherPanel(ActivePanel);
		int PassiveMode = PassivePanel->GetMode();

		if (PassiveMode == NORMAL_PANEL)
		{
			PassivePanel->GetCurDir(PDir);
			AddEndSlash(PDir);
		}

		int ActiveMode = ActivePanel->GetMode();

		if (ActiveMode==NORMAL_PANEL)
		{
			ActivePanel->GetCurDir(ADir);
			AddEndSlash(ADir);
		}

		FARString strNameFile = PointToName(FileName);
		FARString strNameDir = FileName;
		CutToSlash(strNameDir);
		/* $ 10.04.2001 IS
		     Не делаем SetCurDir, если нужный путь уже есть на открытых
		     панелях, тем самым добиваемся того, что выделение с элементов
		     панелей не сбрасывается.
		*/
		BOOL AExist=(ActiveMode==NORMAL_PANEL) && !StrCmpI(ADir,strNameDir);
		BOOL PExist=(PassiveMode==NORMAL_PANEL) && !StrCmpI(PDir,strNameDir);

		// если нужный путь есть на пассивной панели
		if (!AExist && PExist)
			ProcessKey(KEY_TAB);

		if (!AExist && !PExist)
			ActivePanel->SetCurDir(strNameDir,TRUE);

		ActivePanel->GoToFile(strNameFile);
		// всегда обновим заголовок панели, чтобы дать обратную связь, что
		// Ctrl-F10 обработан
		ActivePanel->SetTitle();
	}
}


int  FilePanels::GetMacroMode()
{
	switch (ActivePanel->GetType())
	{
		case TREE_PANEL:
			return MACRO_TREEPANEL;
		case QVIEW_PANEL:
			return MACRO_QVIEWPANEL;
		case INFO_PANEL:
			return MACRO_INFOPANEL;
		default:
			return MACRO_SHELL;
	}
}
