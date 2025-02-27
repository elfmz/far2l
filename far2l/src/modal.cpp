/*
modal.cpp

Parent class для модальных объектов
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

#include "modal.hpp"
#include "keys.hpp"
#include "help.hpp"
#include "lockscrn.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "InterThreadCall.hpp"

Modal::Modal()
	:
	ReadKey(KEY_INVALID), WriteKey(KEY_INVALID), ExitCode(-1), EndLoop(0)
{}

void Modal::Process()
{
	if (LastMessageId >= 0) {
		fprintf(stderr, "\nFirst & last msg ids for this dialog (best guess) are %s and %s\n", GetStringId(FirstMessageId), GetStringId(LastMessageId));
		FirstMessageId = -1;
		LastMessageId = -1;
	}

	Show();

	while (!Done()) {
		ReadInput();
		ProcessInput();
		DispatchInterThreadCalls();
	}

	GetDialogObjectsData();
}

FarKey Modal::ReadInput(INPUT_RECORD *GetReadRec)
{
	if (GetReadRec)
		memset(GetReadRec, 0, sizeof(INPUT_RECORD));

	if (WriteKey != KEY_INVALID) {
		ReadKey = WriteKey;
		WriteKey = KEY_INVALID;
	} else {
		ReadKey = GetInputRecord(&ReadRec);

		if (GetReadRec) {
			*GetReadRec = ReadRec;
		}
	}

	if (ReadKey == KEY_CONSOLE_BUFFER_RESIZE) {
		LockScreen LckScr;
		Hide();
		Show();
	}

	if (CloseFARMenu) {
		SetExitCode(-1);
	}

	return (ReadKey);
}

void Modal::WriteInput(FarKey Key)
{
	WriteKey = Key;
}

void Modal::ProcessInput()
{
	if (ReadRec.EventType == MOUSE_EVENT)
		ProcessMouse(&ReadRec.Event.MouseEvent);
	else
		ProcessKey(ReadKey);
}

int Modal::Done()
{
	return (EndLoop);
}

void Modal::ClearDone()
{
	EndLoop = 0;
}

int Modal::GetExitCode()
{
	return (ExitCode);
}

void Modal::SetExitCode(int Code)
{
	ExitCode = Code;
	EndLoop = TRUE;
}

void Modal::SetHelp(const wchar_t *Topic)
{
	strHelpTopic = Topic;
}

void Modal::ShowHelp()
{
	if (!strHelpTopic.IsEmpty())
		Help::Present(strHelpTopic);
}
