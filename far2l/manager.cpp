/*
manager.cpp

Ïåðåêëþ÷åíèå ìåæäó íåñêîëüêèìè file panels, viewers, editors, dialogs
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

#include "manager.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "frame.hpp"
#include "vmenu.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "savescr.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "syslog.hpp"
#include "registry.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "grabber.hpp"
#include "message.hpp"
#include "config.hpp"
#include "plist.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "exitcode.hpp"
#include "scrbuf.hpp"
#include "console.hpp"

Manager *FrameManager;

Manager::Manager():
	ModalStack(nullptr),
	ModalStackCount(0),
	ModalStackSize(0),
	FrameCount(0),
	FrameList(reinterpret_cast<Frame **>(xf_malloc(sizeof(Frame*)*(FrameCount+1)))),
	FrameListSize(0),
	FramePos(-1),
	InsertedFrame(nullptr),
	DeletedFrame(nullptr),
	ActivatedFrame(nullptr),
	RefreshedFrame(nullptr),
	ModalizedFrame(nullptr),
	UnmodalizedFrame(nullptr),
	DeactivatedFrame(nullptr),
	ExecutedFrame(nullptr),
	CurrentFrame(nullptr),
	ModalEVCount(0),
	EndLoop(FALSE),
	StartManager(FALSE)
{
}

Manager::~Manager()
{
	if (FrameList)
		xf_free(FrameList);

	if (ModalStack)
		xf_free(ModalStack);

	/*if (SemiModalBackFrames)
	  xf_free(SemiModalBackFrames);*/
}


/* $ 29.12.2000 IS
  Àíàëîã CloseAll, íî ðàçðåøàåò ïðîäîëæåíèå ïîëíîöåííîé ðàáîòû â ôàðå,
  åñëè ïîëüçîâàòåëü ïðîäîëæèë ðåäàêòèðîâàòü ôàéë.
  Âîçâðàùàåò TRUE, åñëè âñå çàêðûëè è ìîæíî âûõîäèòü èç ôàðà.
*/
BOOL Manager::ExitAll()
{
	_MANAGER(CleverSysLog clv(L"Manager::ExitAll()"));

	for (int i=this->ModalStackCount-1; i>=0; i--)
	{
		Frame *iFrame=this->ModalStack[i];

		if (!iFrame->GetCanLoseFocus(TRUE))
		{
			int PrevFrameCount=ModalStackCount;
			iFrame->ProcessKey(KEY_ESC);
			Commit();

			if (PrevFrameCount==ModalStackCount)
			{
				return FALSE;
			}
		}
	}

	for (int i=FrameCount-1; i>=0; i--)
	{
		Frame *iFrame=FrameList[i];

		if (!iFrame->GetCanLoseFocus(TRUE))
		{
			ActivateFrame(iFrame);
			Commit();
			int PrevFrameCount=FrameCount;
			iFrame->ProcessKey(KEY_ESC);
			Commit();

			if (PrevFrameCount==FrameCount)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

void Manager::CloseAll()
{
	_MANAGER(CleverSysLog clv(L"Manager::CloseAll()"));
	Frame *iFrame;

	for (int i=ModalStackCount-1; i>=0; i--)
	{
		iFrame=ModalStack[i];
		DeleteFrame(iFrame);
		DeleteCommit();
		DeletedFrame=nullptr;
	}

	for (int i=FrameCount-1; i>=0; i--)
	{
		iFrame=(*this)[i];
		DeleteFrame(iFrame);
		DeleteCommit();
		DeletedFrame=nullptr;
	}

	xf_free(FrameList);
	FrameList=nullptr;
	FrameCount=FramePos=0;
}

BOOL Manager::IsAnyFrameModified(int Activate)
{
	for (int I=0; I<FrameCount; I++)
		if (FrameList[I]->IsFileModified())
		{
			if (Activate)
			{
				ActivateFrame(I);
				Commit();
			}

			return TRUE;
		}

	return FALSE;
}

void Manager::InsertFrame(Frame *Inserted, int Index)
{
	_MANAGER(CleverSysLog clv(L"Manager::InsertFrame(Frame *Inserted, int Index)"));
	_MANAGER(SysLog(L"Inserted=%p, Index=%i",Inserted, Index));

	if (Index==-1)
		Index=FramePos;

	InsertedFrame=Inserted;
}

void Manager::DeleteFrame(Frame *Deleted)
{
	_MANAGER(CleverSysLog clv(L"Manager::DeleteFrame(Frame *Deleted)"));
	_MANAGER(SysLog(L"Deleted=%p",Deleted));

	for (int i=0; i<FrameCount; i++)
	{
		Frame *iFrame=FrameList[i];

		if (iFrame->RemoveModal(Deleted))
		{
			return;
		}
	}

	if (!Deleted)
	{
		DeletedFrame=CurrentFrame;
	}
	else
	{
		DeletedFrame=Deleted;
	}
}

void Manager::DeleteFrame(int Index)
{
	_MANAGER(CleverSysLog clv(L"Manager::DeleteFrame(int Index)"));
	_MANAGER(SysLog(L"Index=%i",Index));
	DeleteFrame(this->operator[](Index));
}


void Manager::ModalizeFrame(Frame *Modalized, int Mode)
{
	_MANAGER(CleverSysLog clv(L"Manager::ModalizeFrame (Frame *Modalized, int Mode)"));
	_MANAGER(SysLog(L"Modalized=%p",Modalized));
	ModalizedFrame=Modalized;
	ModalizeCommit();
}

void Manager::UnmodalizeFrame(Frame *Unmodalized)
{
	_MANAGER(CleverSysLog clv(L"Manager::UnmodalizeFrame (Frame *Unmodalized)"));
	_MANAGER(SysLog(L"Unmodalized=%p",Unmodalized));
	UnmodalizedFrame=Unmodalized;
	UnmodalizeCommit();
}

void Manager::ExecuteNonModal()
{
	_MANAGER(CleverSysLog clv(L"Manager::ExecuteNonModal ()"));
	_MANAGER(SysLog(L"ExecutedFrame=%p, InsertedFrame=%p, DeletedFrame=%p",ExecutedFrame, InsertedFrame, DeletedFrame));
	Frame *NonModal=InsertedFrame?InsertedFrame:(ExecutedFrame?ExecutedFrame:ActivatedFrame);

	if (!NonModal)
	{
		return;
	}

	/* $ 14.05.2002 SKV
	  Ïîëîæèì òåêóùèé ôðýéì â ñïèñîê "ðîäèòåëåé" ïîëóìîäàëüíûõ ôðýéìîâ
	*/
	//Frame *SaveFrame=CurrentFrame;
	//AddSemiModalBackFrame(SaveFrame);
	int NonModalIndex=IndexOf(NonModal);

	if (-1==NonModalIndex)
	{
		InsertedFrame=NonModal;
		ExecutedFrame=nullptr;
		InsertCommit();
		InsertedFrame=nullptr;
	}
	else
	{
		ActivateFrame(NonModalIndex);
	}

	//Frame* ModalStartLevel=NonModal;
	for (;;)
	{
		Commit();

		if (CurrentFrame!=NonModal || EndLoop)
		{
			break;
		}

		ProcessMainLoop();
	}

	//ExecuteModal(NonModal);
	/* $ 14.05.2002 SKV
	  ... è óáåð¸ì åãî æå.
	*/
	//RemoveSemiModalBackFrame(SaveFrame);
}

void Manager::ExecuteModal(Frame *Executed)
{
	_MANAGER(CleverSysLog clv(L"Manager::ExecuteModal (Frame *Executed)"));
	_MANAGER(SysLog(L"Executed=%p, ExecutedFrame=%p",Executed,ExecutedFrame));

	if (!Executed && !ExecutedFrame)
	{
		return;
	}

	if (Executed)
	{
		if (ExecutedFrame)
		{
			_MANAGER(SysLog(L"WARNING! Ïîïûòêà â îäíîì öèêëå çàïóñòèòü â ìîäàëüíîì ðåæèìå äâà ôðåéìà. Executed=%p, ExecitedFrame=%p",Executed, ExecutedFrame));
			return;// nullptr; //?? Îïðåäåëèòü, êàêîå çíà÷åíèå ïðàâèëüíî âîçâðàùàòü â ýòîì ñëó÷àå
		}
		else
		{
			ExecutedFrame=Executed;
		}
	}

	int ModalStartLevel=ModalStackCount;
	int OriginalStartManager=StartManager;
	StartManager=TRUE;

	for (;;)
	{
		Commit();

		if (ModalStackCount<=ModalStartLevel)
		{
			break;
		}

		ProcessMainLoop();
	}

	StartManager=OriginalStartManager;
	return;// GetModalExitCode();
}

int Manager::GetModalExitCode()
{
	return ModalExitCode;
}

/* $ 11.10.2001 IS
   Ïîäñ÷èòàòü êîëè÷åñòâî ôðåéìîâ ñ óêàçàííûì èìåíåì.
*/
int Manager::CountFramesWithName(const wchar_t *Name, BOOL IgnoreCase)
{
	int Counter=0;
	typedef int (__cdecl *cmpfunc_t)(const wchar_t *s1, const wchar_t *s2);
	cmpfunc_t cmpfunc=IgnoreCase ? StrCmpI : StrCmp;
	FARString strType, strCurName;

	for (int I=0; I<FrameCount; I++)
	{
		FrameList[I]->GetTypeAndName(strType, strCurName);

		if (!cmpfunc(Name, strCurName)) ++Counter;
	}

	return Counter;
}

/*!
  \return Âîçâðàùàåò nullptr åñëè íàæàò "îòêàç" èëè åñëè íàæàò òåêóùèé ôðåéì.
  Äðóãèìè ñëîâàìè, åñëè íåìîäàëüíûé ôðåéì íå ïîìåíÿëñÿ.
  Åñëè æå ôðåéì ïîìåíÿëñÿ, òî òîãäà ôóíêöèÿ äîëæíà âîçâðàòèòü
  óêàçàòåëü íà ïðåäûäóùèé ôðåéì.
*/
Frame *Manager::FrameMenu()
{
	/* $ 28.04.2002 KM
	    Ôëàã äëÿ îïðåäåëåíèÿ òîãî, ÷òî ìåíþ ïåðåêëþ÷åíèÿ
	    ýêðàíîâ óæå àêòèâèðîâàíî.
	*/
	static int AlreadyShown=FALSE;

	if (AlreadyShown)
		return nullptr;

	int ExitCode, CheckCanLoseFocus=CurrentFrame->GetCanLoseFocus();
	{
		MenuItemEx ModalMenuItem;
		VMenu ModalMenu(MSG(MScreensTitle),nullptr,0,ScrY-4);
		ModalMenu.SetHelp(L"ScrSwitch");
		ModalMenu.SetFlags(VMENU_WRAPMODE);
		ModalMenu.SetPosition(-1,-1,0,0);

		if (!CheckCanLoseFocus)
			ModalMenuItem.SetDisable(TRUE);

		for (int I=0; I<FrameCount; I++)
		{
			FARString strType, strName, strNumText;
			FrameList[I]->GetTypeAndName(strType, strName);
			ModalMenuItem.Clear();

			if (I<10)
				strNumText.Format(L"&%d. ",I);
			else if (I<36)
				strNumText.Format(L"&%lc. ",I+55);  // 55='A'-10
			else
				strNumText = L"&   ";

			//TruncPathStr(strName,ScrX-24);
			ReplaceStrings(strName,L"&",L"&&",-1);
			/*  äîáàâëÿåòñÿ "*" åñëè ôàéë èçìåíåí */
			ModalMenuItem.strName.Format(L"%ls%-10.10ls %lc %ls", strNumText.CPtr(), strType.CPtr(),(FrameList[I]->IsFileModified()?L'*':L' '), strName.CPtr());
			ModalMenuItem.SetSelect(I==FramePos);
			ModalMenu.AddItem(&ModalMenuItem);
		}

		AlreadyShown=TRUE;
		ModalMenu.Process();
		AlreadyShown=FALSE;
		ExitCode=ModalMenu.Modal::GetExitCode();
	}

	if (CheckCanLoseFocus)
	{
		if (ExitCode>=0)
		{
			ActivateFrame(ExitCode);
			return (ActivatedFrame==CurrentFrame || !CurrentFrame->GetCanLoseFocus()?nullptr:CurrentFrame);
		}

		return (ActivatedFrame==CurrentFrame?nullptr:CurrentFrame);
	}

	return nullptr;
}


int Manager::GetFrameCountByType(int Type)
{
	int ret=0;

	for (int I=0; I<FrameCount; I++)
	{
		/* $ 10.05.2001 DJ
		   íå ó÷èòûâàåì ôðåéì, êîòîðûé ñîáèðàåìñÿ óäàëÿòü
		*/
		if (FrameList[I] == DeletedFrame || (unsigned int)FrameList [I]->GetExitCode() == XC_QUIT)
			continue;

		if (FrameList[I]->GetType()==Type)
			ret++;
	}

	return ret;
}

void Manager::SetFramePos(int NewPos)
{
	_MANAGER(CleverSysLog clv(L"Manager::SetFramePos(int NewPos)"));
	_MANAGER(SysLog(L"NewPos=%i",NewPos));
	FramePos=NewPos;
}

/*$ 11.05.2001 OT Òåïåðü ìîæíî èñêàòü ôàéë íå òîëüêî ïî ïîëíîìó èìåíè, íî è îòäåëüíî - ïóòü, îòäåëüíî èìÿ */
int  Manager::FindFrameByFile(int ModalType,const wchar_t *FileName, const wchar_t *Dir)
{
	FARString strBufFileName;
	FARString strFullFileName = FileName;

	if (Dir)
	{
		strBufFileName = Dir;
		AddEndSlash(strBufFileName);
		strBufFileName += FileName;
		strFullFileName = strBufFileName;
	}

	for (int I=0; I<FrameCount; I++)
	{
		FARString strType, strName;

		// Mantis#0000469 - ïîëó÷àòü Name áóäåì òîëüêî ïðè ñîâïàäåíèè ModalType
		if (FrameList[I]->GetType()==ModalType)
		{
			FrameList[I]->GetTypeAndName(strType, strName);

			if (!StrCmpI(strName, strFullFileName))
				return(I);
		}
	}

	return -1;
}

BOOL Manager::ShowBackground()
{
	if (CtrlObject->CmdLine)
	{
		CtrlObject->CmdLine->ShowBackground();
		return TRUE;
	}
	return FALSE;
}


void Manager::ActivateFrame(Frame *Activated)
{
	_MANAGER(CleverSysLog clv(L"Manager::ActivateFrame(Frame *Activated)"));
	_MANAGER(SysLog(L"Activated=%i",Activated));

	if (IndexOf(Activated)==-1 && IndexOfStack(Activated)==-1)
		return;

	if (!ActivatedFrame)
	{
		ActivatedFrame=Activated;
	}
}

void Manager::ActivateFrame(int Index)
{
	_MANAGER(CleverSysLog clv(L"Manager::ActivateFrame(int Index)"));
	_MANAGER(SysLog(L"Index=%i",Index));
	ActivateFrame((*this)[Index]);
}

void Manager::DeactivateFrame(Frame *Deactivated,int Direction)
{
	_MANAGER(CleverSysLog clv(L"Manager::DeactivateFrame (Frame *Deactivated,int Direction)"));
	_MANAGER(SysLog(L"Deactivated=%p, Direction=%d",Deactivated,Direction));

	if (Direction)
	{
		FramePos+=Direction;

		if (Direction>0)
		{
			if (FramePos>=FrameCount)
			{
				FramePos=0;
			}
		}
		else
		{
			if (FramePos<0)
			{
				FramePos=FrameCount-1;
			}
		}

		ActivateFrame(FramePos);
	}
	else
	{
		// Direction==0
		// Direct access from menu or (in future) from plugin
	}

	DeactivatedFrame=Deactivated;
}

void Manager::SwapTwoFrame(int Direction)
{
	if (Direction)
	{
		int OldFramePos=FramePos;
		FramePos+=Direction;

		if (Direction>0)
		{
			if (FramePos>=FrameCount)
			{
				FramePos=0;
			}
		}
		else
		{
			if (FramePos<0)
			{
				FramePos=FrameCount-1;
			}
		}

		Frame *TmpFrame=FrameList[OldFramePos];
		FrameList[OldFramePos]=FrameList[FramePos];
		FrameList[FramePos]=TmpFrame;
		ActivateFrame(OldFramePos);
	}

	DeactivatedFrame=CurrentFrame;
}

void Manager::RefreshFrame(Frame *Refreshed)
{
	_MANAGER(CleverSysLog clv(L"Manager::RefreshFrame(Frame *Refreshed)"));
	_MANAGER(SysLog(L"Refreshed=%p",Refreshed));

	if (ActivatedFrame)
		return;

	if (Refreshed)
	{
		RefreshedFrame=Refreshed;
	}
	else
	{
		RefreshedFrame=CurrentFrame;
	}

	if (IndexOf(Refreshed)==-1 && IndexOfStack(Refreshed)==-1)
		return;

	/* $ 13.04.2002 KM
	  - Âûçûâàåì ïðèíóäèòåëüíûé Commit() äëÿ ôðåéìà èìåþùåãî ÷ëåíà
	    NextModal, ýòî îçíà÷àåò ÷òî àêòèâíûì ñåé÷àñ ÿâëÿåòñÿ
	    VMenu, à çíà÷èò Commit() ñàì íå áóäåò âûçâàí ïîñëå âîçâðàòà
	    èç ôóíêöèè.
	    Óñòðàíÿåò åù¸ îäèí ìîìåíò íåïåðåðèñîâêè, êîãäà îäèí íàä
	    äðóãèì íàõîäèòñÿ íåñêîëüêî îáúåêòîâ VMenu. Ïðèìåð:
	    íàñòðîéêà öâåòîâ. Òåïåðü AltF9 â äèàëîãå íàñòðîéêè
	    öâåòîâ êîððåêòíî ïåðåðèñîâûâàåò ìåíþ.
	*/
	if (RefreshedFrame && RefreshedFrame->NextModal)
		Commit();
}

void Manager::RefreshFrame(int Index)
{
	_MANAGER(CleverSysLog clv(L"Manager::RefreshFrame(int Index)"));
	_MANAGER(SysLog(L"Index=%d",Index));
	RefreshFrame((*this)[Index]);
}

void Manager::ExecuteFrame(Frame *Executed)
{
	_MANAGER(CleverSysLog clv(L"Manager::ExecuteFrame(Frame *Executed)"));
	_MANAGER(SysLog(L"Executed=%p",Executed));
	ExecutedFrame=Executed;
}


/* $ 10.05.2001 DJ
   ïåðåêëþ÷àåòñÿ íà ïàíåëè (ôðåéì ñ íîìåðîì 0)
*/

void Manager::SwitchToPanels()
{
	_MANAGER(CleverSysLog clv(L"Manager::SwitchToPanels()"));
	ActivateFrame(0);
}


int Manager::HaveAnyFrame()
{
	if (FrameCount || InsertedFrame || DeletedFrame || ActivatedFrame || RefreshedFrame ||
	        ModalizedFrame || DeactivatedFrame || ExecutedFrame || CurrentFrame)
		return 1;

	return 0;
}

void Manager::EnterMainLoop()
{
	WaitInFastFind=0;
	StartManager=TRUE;

	for (;;)
	{
		Commit();

		if (EndLoop || !HaveAnyFrame())
		{
			break;
		}

		ProcessMainLoop();
	}
}

void Manager::SetLastInputRecord(INPUT_RECORD *Rec)
{
	if (&LastInputRecord != Rec)
		LastInputRecord=*Rec;
}


void Manager::ProcessMainLoop()
{
	if ( CurrentFrame )
		CtrlObject->Macro.SetMode(CurrentFrame->GetMacroMode());

	if ( CurrentFrame && !CurrentFrame->ProcessEvents() )
	{
		ProcessKey(KEY_IDLE);
	}
	else
	{
		// Mantis#0000073: Íå ðàáîòàåò àâòîñêðîëèíã â QView
		WaitInMainLoop=IsPanelsActive() && ((FilePanels*)CurrentFrame)->ActivePanel->GetType()!=QVIEW_PANEL;
		//WaitInFastFind++;
		int Key=GetInputRecord(&LastInputRecord);
		//WaitInFastFind--;
		WaitInMainLoop=FALSE;

		if (EndLoop)
			return;

		if (LastInputRecord.EventType==MOUSE_EVENT)
		{
				// èñïîëüçóåì êîïèþ ñòðóêòóðû, ò.ê. LastInputRecord ìîæåò âíåçàïíî èçìåíèòüñÿ âî âðåìÿ âûïîëíåíèÿ ProcessMouse
				MOUSE_EVENT_RECORD mer=LastInputRecord.Event.MouseEvent;
				ProcessMouse(&mer);
		}
		else
			ProcessKey(Key);
	}
}

void Manager::ExitMainLoop(int Ask)
{
	if (CloseFAR)
	{
		CloseFAR=FALSE;
		CloseFARMenu=TRUE;
	};

	if (!Ask || !Opt.Confirm.Exit || !Message(0,2,MSG(MQuit),MSG(MAskQuit),MSG(MYes),MSG(MNo)))
	{
		/* $ 29.12.2000 IS
		   + Ïðîâåðÿåì, ñîõðàíåíû ëè âñå èçìåíåííûå ôàéëû. Åñëè íåò, òî íå âûõîäèì
		     èç ôàðà.
		*/
		if (ExitAll())
		{
			//TODO: ïðè çàêðûòèè ïî x íóæíî äåëàòü ôîðñèðîâàííûé âûõîä. Èíà÷å ìîãóò áûòü
			//      ãëþêè, íàïðèìåð, ïðè ïåðåçàãðóçêå
			FilePanels *cp;

			if (!(cp = CtrlObject->Cp())
			        || (!cp->LeftPanel->ProcessPluginEvent(FE_CLOSE,nullptr) && !cp->RightPanel->ProcessPluginEvent(FE_CLOSE,nullptr)))
				EndLoop=TRUE;
		}
		else
		{
			CloseFARMenu=FALSE;
		}
	}
}

#if defined(FAR_ALPHA_VERSION)
#include <float.h>
#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4717)
//#ifdef __cplusplus
//#if defined(_MSC_VER < 1500) // TODO: See REMINDER file, section intrin.h
#ifndef _M_IA64
extern "C" void __ud2();
#else
extern "C" void __setReg(int, uint64_t);
#endif
//#endif                       // TODO: See REMINDER file, section intrin.h
//#endif
#endif
static void Test_EXCEPTION_STACK_OVERFLOW(char* target)
{
	char Buffer[1024]; /* ÷òîáû áûñòðåå ðâàíóëî */
	strcpy(Buffer, "zzzz");
	Test_EXCEPTION_STACK_OVERFLOW(Buffer);
}
#if defined(_MSC_VER)
#pragma warning( pop )
#endif
#endif


int Manager::ProcessKey(DWORD Key)
{
	int ret=FALSE;

	if (CurrentFrame)
	{
		DWORD KeyM=(Key&(~KEY_CTRLMASK));

		if (!((KeyM >= KEY_MACRO_BASE && KeyM <= KEY_MACRO_ENDBASE) || (KeyM >= KEY_OP_BASE && KeyM <= KEY_OP_ENDBASE))) // ïðîïóñòèì ìàêðî-êîäû
		{
			switch (CurrentFrame->GetType())
			{
				case MODALTYPE_PANELS:
				{
					_ALGO(CleverSysLog clv(L"Manager::ProcessKey()"));
					_ALGO(SysLog(L"Key=%ls",_FARKEY_ToName(Key)));

					if (CtrlObject->Cp()->ActivePanel->SendKeyToPlugin(Key,TRUE))
						return TRUE;

					break;
				}
				case MODALTYPE_VIEWER:
					//if(((FileViewer*)CurrentFrame)->ProcessViewerInput(FrameManager->GetLastInputRecord()))
					//  return TRUE;
					break;
				case MODALTYPE_EDITOR:
					//if(((FileEditor*)CurrentFrame)->ProcessEditorInput(FrameManager->GetLastInputRecord()))
					//  return TRUE;
					break;
				case MODALTYPE_DIALOG:
					//((Dialog*)CurrentFrame)->CallDlgProc(DN_KEY,((Dialog*)CurrentFrame)->GetDlgFocusPos(),Key);
					break;
				case MODALTYPE_VMENU:
				case MODALTYPE_HELP:
				case MODALTYPE_COMBOBOX:
				case MODALTYPE_USER:
				case MODALTYPE_FINDFOLDER:
				default:
					break;
			}
		}

#if 0
#if defined(FAR_ALPHA_VERSION)

// ñåé êîä äëÿ ïðîâåðêè èñêëþ÷àòîð, ïðîñüáà íå òðîãàòü :-)
		if (Key == (KEY_APPS|KEY_CTRL|KEY_ALT) && GetRegKey(L"System/Exception",L"Used",0))
		{
			struct __ECODE
			{
				NTSTATUS Code;
				const wchar_t *Name;
			} ECode[]=
			{
				{EXCEPTION_ACCESS_VIOLATION,L"Access Violation (Read)"},
				{EXCEPTION_ACCESS_VIOLATION,L"Access Violation (Write)"},
				{EXCEPTION_INT_DIVIDE_BY_ZERO,L"Divide by zero"},
				{EXCEPTION_ILLEGAL_INSTRUCTION,L"Illegal instruction"},
				{EXCEPTION_STACK_OVERFLOW,L"Stack Overflow"},
				{EXCEPTION_FLT_DIVIDE_BY_ZERO,L"Floating-point divide by zero"},
				{EXCEPTION_BREAKPOINT,L"Breakpoint"},
#ifdef _M_IA64
				{EXCEPTION_DATATYPE_MISALIGNMENT,L"Alignment fault (IA64 specific)",},
#endif
				/*
				        {EXCEPTION_FLT_OVERFLOW,"EXCEPTION_FLT_OVERFLOW"},
				        {EXCEPTION_SINGLE_STEP,"EXCEPTION_SINGLE_STEP",},
				        {EXCEPTION_ARRAY_BOUNDS_EXCEEDED,"EXCEPTION_ARRAY_BOUNDS_EXCEEDED",},
				        {EXCEPTION_FLT_DENORMAL_OPERAND,"EXCEPTION_FLT_DENORMAL_OPERAND",},
				        {EXCEPTION_FLT_INEXACT_RESULT,"EXCEPTION_FLT_INEXACT_RESULT",},
				        {EXCEPTION_FLT_INVALID_OPERATION,"EXCEPTION_FLT_INVALID_OPERATION",},
				        {EXCEPTION_FLT_STACK_CHECK,"EXCEPTION_FLT_STACK_CHECK",},
				        {EXCEPTION_FLT_UNDERFLOW,"EXCEPTION_FLT_UNDERFLOW",},
				        {EXCEPTION_INT_OVERFLOW,"EXCEPTION_INT_OVERFLOW",0},
				        {EXCEPTION_PRIV_INSTRUCTION,"EXCEPTION_PRIV_INSTRUCTION",0},
				        {EXCEPTION_IN_PAGE_ERROR,"EXCEPTION_IN_PAGE_ERROR",0},
				        {EXCEPTION_NONCONTINUABLE_EXCEPTION,"EXCEPTION_NONCONTINUABLE_EXCEPTION",0},
				        {EXCEPTION_INVALID_DISPOSITION,"EXCEPTION_INVALID_DISPOSITION",0},
				        {EXCEPTION_GUARD_PAGE,"EXCEPTION_GUARD_PAGE",0},
				        {EXCEPTION_INVALID_HANDLE,"EXCEPTION_INVALID_HANDLE",0},
				*/
			};
			static union
			{
				int     i;
				int     *iptr;
				double  d;
			} zero_const, refers;
			zero_const.i=0L;
			MenuItemEx ModalMenuItem;
			ModalMenuItem.Clear();
			VMenu ModalMenu(L"Test Exceptions",nullptr,0,ScrY-4);
			ModalMenu.SetFlags(VMENU_WRAPMODE);
			ModalMenu.SetPosition(-1,-1,0,0);

			for (size_t I=0; I<ARRAYSIZE(ECode); I++)
			{
				ModalMenuItem.strName = ECode[I].Name;
				ModalMenu.AddItem(&ModalMenuItem);
			}

			ModalMenu.Process();
			int ExitCode=ModalMenu.Modal::GetExitCode();

			switch (ExitCode)
			{
				case -1:
					return TRUE;
				case 0:
					zero_const.i=*zero_const.iptr;
					break;
				case 1:
					*zero_const.iptr = 0;
					break;
				case 2:
					zero_const.i=1/zero_const.i;
					break;
				case 3:
#if defined(_MSC_VER)
#ifdef _M_IA64
#define __REG_IA64_IntR0 1024
					__setReg(__REG_IA64_IntR0, 666);
#else
					__ud2();
#endif
#elif defined(__GNUC__)
					asm("ud2");
#else
#error "Unsupported compiler"
#endif
					break;
				case 4:
					Test_EXCEPTION_STACK_OVERFLOW(nullptr);
					break;
				case 5:
					refers.d = 1.0/zero_const.d;
					break;
				case 6:
					DebugBreak();
					break;
#ifdef _M_IA64
				case 7:
				{
					BYTE temp[10];
					memset(temp, 0, 10);
					double* val;
					val = (double*)(&temp[3]);
					printf("%lf\n", *val);
				}
#endif
			}

			Message(MSG_WARNING, 1, L"Test Exceptions failed", L"", ECode[ExitCode].Name, L"", MSG(MOk));
			return TRUE;
		}

#endif
#endif
		/*** ÁËÎÊ ÏÐÈÂÅËÅÃÈÐÎÂÀÍÍÛÕ ÊËÀÂÈØ ! ***/

		/***   ÊÎÒÎÐÛÅ ÍÅËÜÇß ÍÀÌÀÊÐÎÑÈÒÜ    ***/
		switch (Key)
		{
			case KEY_ALT|KEY_NUMPAD0:
			case KEY_ALTINS:
			{
				RunGraber();
				return TRUE;
			}
			case KEY_CONSOLE_BUFFER_RESIZE:
				WINPORT(Sleep)(10);
				ResizeAllFrame();
				return TRUE;
		}

		/*** À âîò çäåñü - âñå îñòàëüíîå! ***/
		if (!IsProcessAssignMacroKey)
			// â ëþáîì ñëó÷àå åñëè êîìó-òî íåíóæíû âñå êëàâèøè èëè
		{
			switch (Key)
			{
				// <Óäàëèòü ïîñëå ïîÿâëåíèÿ ìàêðîôóíêöèè Scroll>
				case KEY_CTRLALTUP:
					if(Opt.WindowMode)
					{
						Console.ScrollWindow(-1);
						return TRUE;
					}
					break;

				case KEY_CTRLALTDOWN:
					if(Opt.WindowMode)
					{
						Console.ScrollWindow(1);
						return TRUE;
					}
					break;

				case KEY_CTRLALTPGUP:
					if(Opt.WindowMode)
					{
						Console.ScrollWindow(-ScrY);
						return TRUE;
					}
					break;

				case KEY_CTRLALTHOME:
					if(Opt.WindowMode)
					{
						while(Console.ScrollWindow(-ScrY));
						return TRUE;
					}
					break;

				case KEY_CTRLALTPGDN:
					if(Opt.WindowMode)
					{
						Console.ScrollWindow(ScrY);
						return TRUE;
					}
					break;

				case KEY_CTRLALTEND:
					if(Opt.WindowMode)
					{
						while(Console.ScrollWindow(ScrY));
						return TRUE;
					}
					break;
				// </Óäàëèòü ïîñëå ïîÿâëåíèÿ ìàêðîôóíêöèè Scroll>

				case KEY_CTRLW:
					ShowProcessList();
					return TRUE;
				case KEY_F11:
					PluginsMenu();
					FrameManager->RefreshFrame();
					//_MANAGER(SysLog(-1));
					return TRUE;
				case KEY_ALTF9:
				{
					//_MANAGER(SysLog(1,"Manager::ProcessKey, KEY_ALTF9 pressed..."));
					WINPORT(Sleep)(10);
					SetVideoMode();
					WINPORT(Sleep)(10);

					/* Â ïðîöåññå èñïîëíåíèÿ Alt-F9 (â íîðìàëüíîì ðåæèìå) â î÷åðåäü
					   êîíñîëè ïîïàäàåò WINDOW_BUFFER_SIZE_EVENT, ôîðìèðóåòñÿ â
					   ChangeVideoMode().
					   Â ðåæèìå èñïîëíåíèÿ ìàêðîñîâ ÝÒÎ íå ïðîèñõîäèò ïî âïîëíå ïîíÿòíûì
					   ïðè÷èíàì.
					*/
					if (CtrlObject->Macro.IsExecuting())
					{
						int PScrX=ScrX;
						int PScrY=ScrY;
						WINPORT(Sleep)(10);
						GetVideoMode(CurSize);

						if (PScrX+1 == CurSize.X && PScrY+1 == CurSize.Y)
						{
							//_MANAGER(SysLog(-1,"GetInputRecord(WINDOW_BUFFER_SIZE_EVENT); return KEY_NONE"));
							return TRUE;
						}
						else
						{
							PrevScrX=PScrX;
							PrevScrY=PScrY;
							//_MANAGER(SysLog(-1,"GetInputRecord(WINDOW_BUFFER_SIZE_EVENT); return KEY_CONSOLE_BUFFER_RESIZE"));
							WINPORT(Sleep)(10);
							return ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
						}
					}

					//_MANAGER(SysLog(-1));
					return TRUE;
				}
				case KEY_F12:
				{
					int TypeFrame=FrameManager->GetCurrentFrame()->GetType();

					if (TypeFrame != MODALTYPE_HELP && TypeFrame != MODALTYPE_DIALOG)
					{
						DeactivateFrame(FrameMenu(),0);
						//_MANAGER(SysLog(-1));
						return TRUE;
					}

					break; // îòäàäèì F12 äàëüøå ïî öåïî÷êå
				}

				case KEY_CTRLALTSHIFTPRESS:
				case KEY_RCTRLALTSHIFTPRESS:
				{
					if (!(Opt.CASRule&1) && Key == KEY_CTRLALTSHIFTPRESS)
						break;

					if (!(Opt.CASRule&2) && Key == KEY_RCTRLALTSHIFTPRESS)
						break;

					if (!Opt.OnlyEditorViewerUsed)
					{
						if (CurrentFrame->FastHide())
						{
							int isPanelFocus=CurrentFrame->GetType() == MODALTYPE_PANELS;

							if (isPanelFocus)
							{
								int LeftVisible=CtrlObject->Cp()->LeftPanel->IsVisible();
								int RightVisible=CtrlObject->Cp()->RightPanel->IsVisible();
								int CmdLineVisible=CtrlObject->CmdLine->IsVisible();
								int KeyBarVisible=CtrlObject->Cp()->MainKeyBar.IsVisible();
								CtrlObject->CmdLine->ShowBackground();
								CtrlObject->Cp()->LeftPanel->Hide0();
								CtrlObject->Cp()->RightPanel->Hide0();

								switch (Opt.PanelCtrlAltShiftRule)
								{
									case 0:
										CtrlObject->CmdLine->Show();
										CtrlObject->Cp()->MainKeyBar.Show();
										break;
									case 1:
										CtrlObject->Cp()->MainKeyBar.Show();
										break;
								}

								WaitKey(Key==KEY_CTRLALTSHIFTPRESS?KEY_CTRLALTSHIFTRELEASE:KEY_RCTRLALTSHIFTRELEASE);

								if (LeftVisible)      CtrlObject->Cp()->LeftPanel->Show();

								if (RightVisible)     CtrlObject->Cp()->RightPanel->Show();

								if (CmdLineVisible)   CtrlObject->CmdLine->Show();

								if (KeyBarVisible)    CtrlObject->Cp()->MainKeyBar.Show();
							}
							else
							{
								ImmediateHide();
								WaitKey(Key==KEY_CTRLALTSHIFTPRESS?KEY_CTRLALTSHIFTRELEASE:KEY_RCTRLALTSHIFTRELEASE);
							}

							FrameManager->RefreshFrame();
						}

						return TRUE;
					}

					break;
				}
				case KEY_CTRLTAB:
				case KEY_CTRLSHIFTTAB:

					if (CurrentFrame->GetCanLoseFocus())
					{
						DeactivateFrame(CurrentFrame,Key==KEY_CTRLTAB?1:-1);
					}

					_MANAGER(SysLog(-1));
					return TRUE;
			}
		}

		CurrentFrame->UpdateKeyBar();
		CurrentFrame->ProcessKey(Key);
	}

	_MANAGER(SysLog(-1));
	return ret;
}

int Manager::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	// Ïðè êàïòþðåííîé ìûøè îòäàåì óïðàâëåíèå çàäàííîìó îáúåêòó
//    if (ScreenObject::CaptureMouseObject)
//      return ScreenObject::CaptureMouseObject->ProcessMouse(MouseEvent);
	int ret=FALSE;

//    _D(SysLog(1,"Manager::ProcessMouse()"));
	if (CurrentFrame)
		ret=CurrentFrame->ProcessMouse(MouseEvent);

//    _D(SysLog(L"Manager::ProcessMouse() ret=%i",ret));
	_MANAGER(SysLog(-1));
	return ret;
}

void Manager::PluginsMenu()
{
	_MANAGER(SysLog(1));
	int curType = CurrentFrame->GetType();

	if (curType == MODALTYPE_PANELS || curType == MODALTYPE_EDITOR || curType == MODALTYPE_VIEWER || curType == MODALTYPE_DIALOG)
	{
		/* 02.01.2002 IS
		   ! Âûâîä ïðàâèëüíîé ïîìîùè ïî Shift-F1 â ìåíþ ïëàãèíîâ â ðåäàêòîðå/âüþåðå/äèàëîãå
		   ! Åñëè íà ïàíåëè QVIEW èëè INFO îòêðûò ôàéë, òî ñ÷èòàåì, ÷òî ýòî
		     ïîëíîöåííûé âüþåð è çàïóñêàåì ñ ñîîòâåòñòâóþùèì ïàðàìåòðîì ïëàãèíû
		*/
		if (curType==MODALTYPE_PANELS)
		{
			int pType=CtrlObject->Cp()->ActivePanel->GetType();

			if (pType==QVIEW_PANEL || pType==INFO_PANEL)
			{
				FARString strType, strCurFileName;
				CtrlObject->Cp()->GetTypeAndName(strType, strCurFileName);

				if (!strCurFileName.IsEmpty())
				{
					DWORD Attr=apiGetFileAttributes(strCurFileName);

					// èíòåðåñóþò òîëüêî îáû÷íûå ôàéëû
					if (Attr!=INVALID_FILE_ATTRIBUTES && !(Attr&FILE_ATTRIBUTE_DIRECTORY))
						curType=MODALTYPE_VIEWER;
				}
			}
		}

		// â ðåäàêòîðå, âüþåðå èëè äèàëîãå ïîêàæåì ñâîþ ïîìîùü ïî Shift-F1
		const wchar_t *Topic=curType==MODALTYPE_EDITOR?L"Editor":
		                     curType==MODALTYPE_VIEWER?L"Viewer":
		                     curType==MODALTYPE_DIALOG?L"Dialog":nullptr;
		CtrlObject->Plugins.CommandsMenu(curType,0,Topic);
	}

	_MANAGER(SysLog(-1));
}

BOOL Manager::IsPanelsActive()
{
	if (FramePos>=0)
	{
		return CurrentFrame?CurrentFrame->GetType() == MODALTYPE_PANELS:FALSE;
	}
	else
	{
		return FALSE;
	}
}

Frame *Manager::operator[](int Index)
{
	if (Index<0 || Index>=FrameCount || !FrameList)
	{
		return nullptr;
	}

	return FrameList[Index];
}

int Manager::IndexOfStack(Frame *Frame)
{
	int Result=-1;

	for (int i=0; i<ModalStackCount; i++)
	{
		if (Frame==ModalStack[i])
		{
			Result=i;
			break;
		}
	}

	return Result;
}

int Manager::IndexOf(Frame *Frame)
{
	int Result=-1;

	for (int i=0; i<FrameCount; i++)
	{
		if (Frame==FrameList[i])
		{
			Result=i;
			break;
		}
	}

	return Result;
}

BOOL Manager::Commit()
{
	_MANAGER(CleverSysLog clv(L"Manager::Commit()"));
	_MANAGER(ManagerClass_Dump(L"ManagerClass"));
	int Result = false;

	if (DeletedFrame && (InsertedFrame||ExecutedFrame))
	{
		UpdateCommit();
		DeletedFrame = nullptr;
		InsertedFrame = nullptr;
		ExecutedFrame=nullptr;
		Result=true;
	}
	else if (ExecutedFrame)
	{
		ExecuteCommit();
		ExecutedFrame=nullptr;
		Result=true;
	}
	else if (DeletedFrame)
	{
		DeleteCommit();
		DeletedFrame = nullptr;
		Result=true;
	}
	else if (InsertedFrame)
	{
		InsertCommit();
		InsertedFrame = nullptr;
		Result=true;
	}
	else if (DeactivatedFrame)
	{
		DeactivateCommit();
		DeactivatedFrame=nullptr;
		Result=true;
	}
	else if (ActivatedFrame)
	{
		ActivateCommit();
		ActivatedFrame=nullptr;
		Result=true;
	}
	else if (RefreshedFrame)
	{
		RefreshCommit();
		RefreshedFrame=nullptr;
		Result=true;
	}
	else if (ModalizedFrame)
	{
		ModalizeCommit();
//    ModalizedFrame=nullptr;
		Result=true;
	}
	else if (UnmodalizedFrame)
	{
		UnmodalizeCommit();
//    UnmodalizedFrame=nullptr;
		Result=true;
	}

	if (Result)
	{
		Result=Commit();
	}

	return Result;
}

void Manager::DeactivateCommit()
{
	_MANAGER(CleverSysLog clv(L"Manager::DeactivateCommit()"));
	_MANAGER(SysLog(L"DeactivatedFrame=%p",DeactivatedFrame));

	/*$ 18.04.2002 skv
	  Åñëè íå÷åãî àêòèâèðîâàòü, òî â îáùåì-òî íå íàäî è äåàêòèâèðîâàòü.
	*/
	if (!DeactivatedFrame || !ActivatedFrame)
	{
		return;
	}

	if (!ActivatedFrame)
	{
		_MANAGER("WARNING! !ActivatedFrame");
	}

	if (DeactivatedFrame)
	{
		DeactivatedFrame->OnChangeFocus(0);
	}

	int modalIndex=IndexOfStack(DeactivatedFrame);

	if (-1 != modalIndex && modalIndex== ModalStackCount-1)
	{
		/*if (IsSemiModalBackFrame(ActivatedFrame))
		{ // ßâëÿåòñÿ ëè "ðîäèòåëåì" ïîëóìîäàëüíîãî ôðýéìà?
		  ModalStackCount--;
		}
		else
		{*/
		if (IndexOfStack(ActivatedFrame)==-1)
		{
			ModalStack[ModalStackCount-1]=ActivatedFrame;
		}
		else
		{
			ModalStackCount--;
		}

//    }
	}
}


void Manager::ActivateCommit()
{
	_MANAGER(CleverSysLog clv(L"Manager::ActivateCommit()"));
	_MANAGER(SysLog(L"ActivatedFrame=%p",ActivatedFrame));

	if (CurrentFrame==ActivatedFrame)
	{
		RefreshedFrame=ActivatedFrame;
		return;
	}

	int FrameIndex=IndexOf(ActivatedFrame);

	if (-1!=FrameIndex)
	{
		FramePos=FrameIndex;
	}

	/* 14.05.2002 SKV
	  Åñëè ìû ïûòàåìñÿ àêòèâèðîâàòü ïîëóìîäàëüíûé ôðýéì,
	  òî íàäî åãî âûòàùèò íà âåðõ ñòýêà ìîäàëîâ.
	*/

	for (int I=0; I<ModalStackCount; I++)
	{
		if (ModalStack[I]==ActivatedFrame)
		{
			Frame *tmp=ModalStack[I];
			ModalStack[I]=ModalStack[ModalStackCount-1];
			ModalStack[ModalStackCount-1]=tmp;
			break;
		}
	}

	RefreshedFrame=CurrentFrame=ActivatedFrame;
}

void Manager::UpdateCommit()
{
	_MANAGER(CleverSysLog clv(L"Manager::UpdateCommit()"));
	_MANAGER(SysLog(L"DeletedFrame=%p, InsertedFrame=%p, ExecutedFrame=%p",DeletedFrame,InsertedFrame, ExecutedFrame));

	if (ExecutedFrame)
	{
		DeleteCommit();
		ExecuteCommit();
		return;
	}

	int FrameIndex=IndexOf(DeletedFrame);

	if (-1!=FrameIndex)
	{
		ActivateFrame(FrameList[FrameIndex] = InsertedFrame);
		ActivatedFrame->FrameToBack=CurrentFrame;
		DeleteCommit();
	}
	else
	{
		_MANAGER(SysLog(L"ERROR! DeletedFrame not found"));
	}
}

//! Óäàëÿåò DeletedFrame èçî âñåõ î÷åðåäåé!
//! Íàçíà÷àåò ñëåäóþùèé àêòèâíûé, (èñõîäÿ èç ñâîèõ ïðåäñòàâëåíèé)
//! Íî òîëüêî â òîì ñëó÷àå, åñëè àêòèâíûé ôðåéì åùå íå íàçíà÷åí çàðàíåå.
void Manager::DeleteCommit()
{
	_MANAGER(CleverSysLog clv(L"Manager::DeleteCommit()"));
	_MANAGER(SysLog(L"DeletedFrame=%p",DeletedFrame));

	if (!DeletedFrame)
	{
		return;
	}

	// <ifDoubleInstance>
	//BOOL ifDoubI=ifDoubleInstance(DeletedFrame);
	// </ifDoubleInstance>
	int ModalIndex=IndexOfStack(DeletedFrame);

	if (ModalIndex!=-1)
	{
		/* $ 14.05.2002 SKV
		  Íàä¸æíåå íàéòè è óäàëèòü èìåííî òî, ÷òî
		  íóæíî, à íå ïðîñòî âåðõíèé.
		*/
		for (int i=0; i<ModalStackCount; i++)
		{
			if (ModalStack[i]==DeletedFrame)
			{
				for (int j=i+1; j<ModalStackCount; j++)
				{
					ModalStack[j-1]=ModalStack[j];
				}

				ModalStackCount--;
				break;
			}
		}

		if (ModalStackCount)
		{
			ActivateFrame(ModalStack[ModalStackCount-1]);
		}
	}

	for (int i=0; i<FrameCount; i++)
	{
		if (FrameList[i]->FrameToBack==DeletedFrame)
		{
			FrameList[i]->FrameToBack=CtrlObject->Cp();
		}
	}

	int FrameIndex=IndexOf(DeletedFrame);

	if (-1!=FrameIndex)
	{
		DeletedFrame->DestroyAllModal();

		for (int j=FrameIndex; j<FrameCount-1; j++)
		{
			FrameList[j]=FrameList[j+1];
		}

		FrameCount--;

		if (FramePos >= FrameCount)
		{
			FramePos=0;
		}

		if (DeletedFrame->FrameToBack==CtrlObject->Cp())
		{
			ActivateFrame(FrameList[FramePos]);
		}
		else
		{
			ActivateFrame(DeletedFrame->FrameToBack);
		}
	}

	/* $ 14.05.2002 SKV
	  Äîëãî íå ìîã ïîíÿòü, íóæåí âñ¸ æå ýòîò êîä èëè íåò.
	  Íî âðîäå êàê íóæåí.
	  SVS> Êîãäà ïîíàäîáèòñÿ - â íåêîòîðûõ ìåñòàõ ðàññêîììåíòèòü êóñêè êîäà
	       ïîìå÷åííûå ñêîáêàìè <ifDoubleInstance>

	if (ifDoubI && IsSemiModalBackFrame(ActivatedFrame)){
	  for(int i=0;i<ModalStackCount;i++)
	  {
	    if(ModalStack[i]==ActivatedFrame)
	    {
	      break;
	    }
	  }

	  if(i==ModalStackCount)
	  {
	    if (ModalStackCount == ModalStackSize){
	      ModalStack = (Frame **) xf_realloc (ModalStack, ++ModalStackSize * sizeof (Frame *));
	    }
	    ModalStack[ModalStackCount++]=ActivatedFrame;
	  }
	}
	*/
	DeletedFrame->OnDestroy();

	if (DeletedFrame->GetDynamicallyBorn())
	{
		_MANAGER(SysLog(L"delete DeletedFrame %p, CurrentFrame=%p",DeletedFrame,CurrentFrame));

		if (CurrentFrame==DeletedFrame)
			CurrentFrame=0;

		/* $ 14.05.2002 SKV
		  Òàê êàê â äåñòðóêòîðå ôðýéìà íåÿâíî ìîæåò áûòü
		  âûçâàí commit, òî íàäî ïîäñòðàõîâàòüñÿ.
		*/
		Frame *tmp=DeletedFrame;
		DeletedFrame=nullptr;
		delete tmp;
	}

	// Ïîëàãàåìñÿ íà òî, ÷òî â ActevateFrame íå áóäåò ïåðåïèñàí óæå
	// ïðèñâîåííûé  ActivatedFrame
	if (ModalStackCount)
	{
		ActivateFrame(ModalStack[ModalStackCount-1]);
	}
	else
	{
		ActivateFrame(FramePos);
	}
}

void Manager::InsertCommit()
{
	_MANAGER(CleverSysLog clv(L"Manager::InsertCommit()"));
	_MANAGER(SysLog(L"InsertedFrame=%p",InsertedFrame));

	if (InsertedFrame)
	{
		if (FrameListSize <= FrameCount)
		{
			FrameList=(Frame **)xf_realloc(FrameList,sizeof(*FrameList)*(FrameCount+1));
			FrameListSize++;
		}

		InsertedFrame->FrameToBack=CurrentFrame;
		FrameList[FrameCount]=InsertedFrame;

		if (!ActivatedFrame)
		{
			ActivatedFrame=InsertedFrame;
		}

		FrameCount++;
	}
}

void Manager::RefreshCommit()
{
	_MANAGER(CleverSysLog clv(L"Manager::RefreshCommit()"));
	_MANAGER(SysLog(L"RefreshedFrame=%p",RefreshedFrame));

	if (!RefreshedFrame)
		return;

	if (IndexOf(RefreshedFrame)==-1 && IndexOfStack(RefreshedFrame)==-1)
		return;

	if (!RefreshedFrame->Locked())
	{
		if (!IsRedrawFramesInProcess)
			RefreshedFrame->ShowConsoleTitle();

		if (RefreshedFrame)
			RefreshedFrame->Refresh();

		if (!RefreshedFrame)
			return;

		CtrlObject->Macro.SetMode(RefreshedFrame->GetMacroMode());
	}

	if ((Opt.ViewerEditorClock &&
	        (RefreshedFrame->GetType() == MODALTYPE_EDITOR ||
	         RefreshedFrame->GetType() == MODALTYPE_VIEWER))
	        || (WaitInMainLoop && Opt.Clock))
		ShowTime(1);
}

void Manager::ExecuteCommit()
{
	_MANAGER(CleverSysLog clv(L"Manager::ExecuteCommit()"));
	_MANAGER(SysLog(L"ExecutedFrame=%p",ExecutedFrame));

	if (!ExecutedFrame)
	{
		return;
	}

	if (ModalStackCount == ModalStackSize)
	{
		ModalStack = (Frame **) xf_realloc(ModalStack, ++ModalStackSize * sizeof(Frame *));
	}

	ModalStack [ModalStackCount++] = ExecutedFrame;
	ActivatedFrame=ExecutedFrame;
}

/*$ 26.06.2001 SKV
  Äëÿ âûçîâà èç ïëàãèíîâ ïîñðåäñòâîì ACTL_COMMIT
*/
BOOL Manager::PluginCommit()
{
	return Commit();
}

/* $ Ââåäåíà äëÿ íóæä CtrlAltShift OT */
void Manager::ImmediateHide()
{
	if (FramePos<0)
		return;

	// Ñíà÷àëà ïðîâåðÿåì, åñòü ëè ó ïðÿòûâàåìîãî ôðåéìà SaveScreen
	if (CurrentFrame->HasSaveScreen())
	{
		CurrentFrame->Hide();
		return;
	}

	// Ôðåéìû ïåðåðèñîâûâàþòñÿ, çíà÷èò äëÿ íèæíèõ
	// íå âûñòàâëÿåì çàãîëîâîê êîíñîëè, ÷òîáû íå ìåëüêàë.
	if (ModalStackCount>0)
	{
		/* $ 28.04.2002 KM
		    Ïðîâåðèì, à íå ìîäàëüíûé ëè ðåäàêòîð èëè âüþâåð íà âåðøèíå
		    ìîäàëüíîãî ñòåêà? È åñëè äà, ïîêàæåì User screen.
		*/
		if (ModalStack[ModalStackCount-1]->GetType()==MODALTYPE_EDITOR ||
		        ModalStack[ModalStackCount-1]->GetType()==MODALTYPE_VIEWER)
		{
			if (CtrlObject->CmdLine)
				CtrlObject->CmdLine->ShowBackground();
		}
		else
		{
			int UnlockCount=0;
			IsRedrawFramesInProcess++;

			while ((*this)[FramePos]->Locked())
			{
				(*this)[FramePos]->Unlock();
				UnlockCount++;
			}

			RefreshFrame((*this)[FramePos]);
			Commit();

			for (int i=0; i<UnlockCount; i++)
			{
				(*this)[FramePos]->Lock();
			}

			if (ModalStackCount>1)
			{
				for (int i=0; i<ModalStackCount-1; i++)
				{
					if (!(ModalStack[i]->FastHide() & CASR_HELP))
					{
						RefreshFrame(ModalStack[i]);
						Commit();
					}
					else
					{
						break;
					}
				}
			}

			/* $ 04.04.2002 KM
			   Ïåðåðèñóåì çàãîëîâîê òîëüêî ó àêòèâíîãî ôðåéìà.
			   Ýòèì ìû ïðåäîòâðàùàåì ìåëüêàíèå çàãîëîâêà êîíñîëè
			   ïðè ïåðåðèñîâêå âñåõ ôðåéìîâ.
			*/
			IsRedrawFramesInProcess--;
			CurrentFrame->ShowConsoleTitle();
		}
	}
	else
	{
		if (CtrlObject->CmdLine)
			CtrlObject->CmdLine->ShowBackground();
	}
}

void Manager::ModalizeCommit()
{
	CurrentFrame->Push(ModalizedFrame);
	ModalizedFrame=nullptr;
}

void Manager::UnmodalizeCommit()
{
	Frame *iFrame;

	for (int i=0; i<FrameCount; i++)
	{
		iFrame=FrameList[i];

		if (iFrame->RemoveModal(UnmodalizedFrame))
		{
			break;
		}
	}

	for (int i=0; i<ModalStackCount; i++)
	{
		iFrame=ModalStack[i];

		if (iFrame->RemoveModal(UnmodalizedFrame))
		{
			break;
		}
	}

	UnmodalizedFrame=nullptr;
}

BOOL Manager::ifDoubleInstance(Frame *frame)
{
	// <ifDoubleInstance>
	/*
	  if (ModalStackCount<=0)
	    return FALSE;
	  if(IndexOfStack(frame)==-1)
	    return FALSE;
	  if(IndexOf(frame)!=-1)
	    return TRUE;
	*/
	// </ifDoubleInstance>
	return FALSE;
}

/*  Âûçîâ ResizeConsole äëÿ âñåõ NextModal ó
    ìîäàëüíîãî ôðåéìà. KM
*/
void Manager::ResizeAllModal(Frame *ModalFrame)
{
	if (!ModalFrame->NextModal)
		return;

	Frame *iModal=ModalFrame->NextModal;

	while (iModal)
	{
		iModal->ResizeConsole();
		iModal=iModal->NextModal;
	}
}

void Manager::ResizeAllFrame()
{
	ScrBuf.Lock();
	for (int i=0; i < FrameCount; i++)
	{
		FrameList[i]->ResizeConsole();
	}

	for (int i=0; i < ModalStackCount; i++)
	{
		ModalStack[i]->ResizeConsole();
		/* $ 13.04.2002 KM
		  - À òåïåðü ïðîðåñàéçèì âñå NextModal...
		*/
		ResizeAllModal(ModalStack[i]);
	}

	ImmediateHide();
	FrameManager->RefreshFrame();
	//RefreshFrame();
	ScrBuf.Unlock();
}

void Manager::InitKeyBar()
{
	for (int I=0; I < FrameCount; I++)
		FrameList[I]->InitKeyBar();
}

/*void Manager::AddSemiModalBackFrame(Frame* frame)
{
  if(SemiModalBackFramesCount>=SemiModalBackFramesSize)
  {
    SemiModalBackFramesSize+=4;
    SemiModalBackFrames=
      (Frame**)xf_realloc(SemiModalBackFrames,sizeof(Frame*)*SemiModalBackFramesSize);

  }
  SemiModalBackFrames[SemiModalBackFramesCount]=frame;
  SemiModalBackFramesCount++;
}

BOOL Manager::IsSemiModalBackFrame(Frame *frame)
{
  if(!SemiModalBackFrames)return FALSE;
  for(int i=0;i<SemiModalBackFramesCount;i++)
  {
    if(SemiModalBackFrames[i]==frame)return TRUE;
  }
  return FALSE;
}

void Manager::RemoveSemiModalBackFrame(Frame* frame)
{
  if(!SemiModalBackFrames)return;
  for(int i=0;i<SemiModalBackFramesCount;i++)
  {
    if(SemiModalBackFrames[i]==frame)
    {
      for(int j=i+1;j<SemiModalBackFramesCount;j++)
      {
        SemiModalBackFrames[j-1]=SemiModalBackFrames[j];
      }
      SemiModalBackFramesCount--;
      return;
    }
  }
}
*/

// âîçâðàùàåò top-ìîäàë èëè ñàì ôðåéì, åñëè ó ôðåéìà íåòó ìîäàëîâ
Frame* Manager::GetTopModal()
{
	Frame *f=CurrentFrame, *fo=nullptr;

	while (f)
	{
		fo=f;
		f=f->GetTopModal();
	}

	if (!f)
		f=fo;

	return f;
}
