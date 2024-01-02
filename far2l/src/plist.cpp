/*
plist.cpp

Список процессов (Ctrl-W)
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

#include "plist.hpp"
#include "keys.hpp"
#include "help.hpp"
#include "lang.hpp"
#include "vmenu.hpp"
#include "message.hpp"
#include "config.hpp"
#include "interf.hpp"
#include "execute.hpp"
#include "dirmix.hpp"
#include "manager.hpp"

void ShowProcessList()
{
	farExecuteA(GetMyScriptQuoted("ps.sh").c_str(), 0);
	if (FrameManager) {
		auto *current_frame = FrameManager->GetCurrentFrame();
		if (current_frame) {
			FrameManager->RefreshFrame(current_frame);
		}
	}
/*
	for (int i = FrameManager->GetFrameCount(); i > 0; --i) {
		FrameManager->RefreshFrame(i - 1);
	}
*/
#if 0
	VMenu ProcList(Msg::ProcessListTitle,nullptr,0,ScrY-4);
	ProcList.SetFlags(VMENU_WRAPMODE);
	ProcList.SetPosition(-1,-1,0,0);

//	if (!EnumWindows(EnumWindowsProc,(LPARAM)&ProcList))
//		return;

	ProcList.AssignHighlights(FALSE);
	ProcList.SetBottomTitle(Msg::ProcessListBottom);
	ProcList.Show();

	while (!ProcList.Done())
	{
		FarKey Key=ProcList.ReadInput();

		switch (Key)
		{
			case KEY_F1:
			{
				Help::Present(L"TaskList");
				break;
			}
			case KEY_CTRLR:
			{
				ProcList.Hide();
				ProcList.DeleteItems();
				ProcList.SetPosition(-1,-1,0,0);

//				if (!EnumWindows(EnumWindowsProc,(LPARAM)&ProcList))
//				{
//					ProcList.Modal::SetExitCode(-1);
//					break;
//				}

				ProcList.Show();
				break;
			}
			case KEY_NUMDEL:
			case KEY_DEL:
			{
			}
			break;
			default:
				ProcList.ProcessInput();
				break;
		}
	}

	if (ProcList.Modal::GetExitCode()>=0)
	{
	}
#endif
}
