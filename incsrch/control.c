/*
FAR manager incremental search plugin, search as you type in editor.
Copyright (C) 1999-2019, Stanislav V. Mekhanoshin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "incsrch.h"

static void PutEvent(int Cmd)
{
	register int cnt = Event.Event.KeyEvent.wRepeatCount;
	if (!cnt)
		cnt++;
	do {
		aEvents[nEvents].Flags = (unsigned char)Cmd;
		aEvents[nEvents].AsciiChar = Event.Event.KeyEvent.uChar.AsciiChar;
		if (++nEvents == PREVIEW_EVENTS) {
#if defined(WINPORT_DIRECT)
			putwchar('\007');
#else
			MessageBeep((UINT)-1);
#endif
			break;
		}
		Cmd|= KC_FAILKILL;
	} while (--cnt);
}

BOOL CollectEvents(void)
{
Loop:
	while (!bTermEvent && nEvents != PREVIEW_EVENTS && WaitInput(FALSE)
			&& apiEditorControl(ECTL_READINPUT, &Event)) {
		switch (Event.EventType) {
			case 0:		// Internal FAR (bug?)
			case NOOP_EVENT:
			case FOCUS_EVENT:
			case MENU_EVENT:
			case WINDOW_BUFFER_SIZE_EVENT:
			case 0x8001:	// KEY_EVENT&0x8000
				continue;
			case MOUSE_EVENT:
				if (Event.Event.MouseEvent.dwButtonState)
					goto Quit;
				continue;
			case KEY_EVENT:
				if (!Event.Event.KeyEvent.bKeyDown)
					continue;
				else if (Event.Event.KeyEvent.wVirtualKeyCode == VK_MENU) {
					aEvents[nEvents++].Flags = KC_CLEARMSG;
					continue;
				} else if (Event.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
					goto Quit;
				} else
					switch (Event.Event.KeyEvent.wVirtualKeyCode) {
						case VK_F1:
							aEvents[nEvents++].Flags = KC_HELP;
						case VK_CONTROL:
						case VK_SHIFT:
						case VK_NUMLOCK:
						case VK_CAPITAL:
						case VK_SCROLL:
							continue;
						case VK_ESCAPE:
							bTermEvent = TRUE;
							bEscape = TRUE;
							return TRUE;
						case VK_BACK:
							PutEvent(KC_BACK);
							continue;
						case VK_RETURN:
							if (Event.Event.KeyEvent.dwControlKeyState
									& (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
								PutEvent((Event.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
												? KC_PREV
												: KC_NEXT);
								continue;
							}
							goto Quit;
						case VK_INSERT:
							if (Event.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) {
							Paste:
								PasteSearchText();
								continue;
							}
							goto Quit;
						case 'V':
						case 'v':
							if (Event.Event.KeyEvent.dwControlKeyState
									& (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
								goto Paste;
						default:
							if ((Event.Event.KeyEvent.dwControlKeyState
										& (ENHANCED_KEY | LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED
												| RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED))
									|| ((unsigned)Event.Event.KeyEvent.uChar.AsciiChar < 32
											&& Event.Event.KeyEvent.uChar.AsciiChar != '\t'))
								goto Quit;
							PutEvent(KC_CHAR);
					}
				continue;
			default:
				goto Quit;
		}
	}
	if (bTermEvent) {
	Quit:
		bTermEvent = TRUE;
		if (bStopOnFound) {
			bTermEvent = FALSE;
			goto Loop;
		}
	}
	return nEvents || bTermEvent;
}

void ShowTitle(int OpenFrom)
{
	int nRest;
	TCHAR Title[TITLE_LEN + 2];

#if defined(UNICODE)
	_tstrcpy(Title, TITLE_PREFIX_STR);
#else
	*(DWORD *)Title = TITLE_PREFIX_STR_DWORD;
#endif

	if (!bStopOnFound) {
		nRest = ((nLen >= TITLE_LEN - TITLE_PREFIX_LEN - 1) ? TITLE_LEN - TITLE_PREFIX_LEN - 1 : nLen);
		memcpy(&Title[TITLE_PREFIX_LEN], &sStr[nLen - nRest], nRest * sizeof(sStr[0]));
		if (OpenFrom == OPEN_EDITOR) {
			Title[nRest + TITLE_PREFIX_LEN] = '\0';
			apiEditorControl(ECTL_SETTITLE, Title);
		}
#ifdef VIEWVER_SUPPORT
		else {
			/* blank the next chars... */
			setmem(&Title[nRest + TITLE_PREFIX_LEN], ' ',
					sizeof(Title) / sizeof(Title[0]) - nRest - TITLE_PREFIX_LEN - 1);
			Title[sizeof(Title) / sizeof(Title[0])] = '\0';
			apiText(0, 0, iViewerStatusColor, Title);
			apiText(0, 0, iViewerStatusColor, NULL);
		}
#endif
	}
	if (bNotFound) {
		StatusMessage(MNotFound);
		if (bBeepOnMismatch)
#if defined(WINPORT_DIRECT)
			putwchar('\007');
#else
			MessageBeep((UINT)-1);
#endif
		bNotFound = FALSE;
	}
}

#ifdef __SW_ET
void __etRedraw(void)
{
	apiEditorControl(ECTL_REDRAW, NULL);
}

void __etSelect(struct EditorSelect *pSi)
{
	apiEditorControl(ECTL_SELECT, pSi);
}
#endif

void SelectFound(BOOL bRedraw)
{
	struct EditorSelect si;

	si.BlockType = (nLen ? BTYPE_STREAM : BTYPE_NONE);
	si.BlockStartLine = ei.CurLine;
	si.BlockStartPos = ei.CurPos - nLen;
	si.BlockWidth = nLen;
	si.BlockHeight = 1;
#ifdef __SW_ET
	__etSelect(&si);
#else
	apiEditorControl(ECTL_SELECT, &si);
#endif
	if (bRedraw)
#ifdef __SW_ET
		__etRedraw();
#else
		apiEditorControl(ECTL_REDRAW, NULL);
#endif
}

void SetPosition(int nLine, int nPos, int nLeftPos)
{
	esp.CurLine = nLine;
	esp.CurPos = nPos;
	esp.LeftPos = nLeftPos;
	apiEditorControl(ECTL_SETPOSITION, &esp);
}

void PositionToView(int nLine, int nStartPos)
{
	int pos = nStartPos + nLen;

	SetPosition(nLine, pos,
			(ei.LeftPos > nStartPos) ? (ei.WindowSizeX > pos) ? 0 : nStartPos
					: (ei.LeftPos + ei.WindowSizeX <= pos)
					? (pos - ei.WindowSizeX + 1)
					: ei.LeftPos);

	apiEditorControl(ECTL_GETINFO, &ei);
}

void StatusMessage(int Index)
{
	apiText(28, 0, 0x0E, GetMsg(Index));
	apiText(28, 0, 0x0E, NULL);
	if (bStopOnFound)
		CollectEvents();	// to force FAR redraw title
}

int DialogFromTemplate(const TCHAR *sTitle, const DialogTemplateItem *aTplItems,
		struct FarDialogItem *aDialogItems, int nItemsNumber, TCHAR *sHelpTopic, int nFocus,
		int nDefaultButton)
{
	int i;
	int nDialogWidth = 0;

	zeromem(aDialogItems, sizeof(struct FarDialogItem) * nItemsNumber);
	aDialogItems[0].Type = DI_DOUBLEBOX;
	for (i = 1; i < nItemsNumber; i++) {
		aDialogItems[i].Type = aTplItems[i - 1].Type;
		aDialogItems[i].X1 = aTplItems[i - 1].X1;
		aDialogItems[i].Y1 = aTplItems[i - 1].Y1;
		aDialogItems[i].Selected = aTplItems[i - 1].Selected;
		aDialogItems[i].Flags = aTplItems[i - 1].Flags & ~DIFT_MSGNUM;
		if (aTplItems[i - 1].Flags & DIFT_MSGNUM)
#if defined(WINPORT_DIRECT)
			aDialogItems[i].PtrData = GetMsg(aTplItems[i - 1].Data);
#else
			_tstrcpy((TCHAR *)aDialogItems[i].Data, GetMsg(aTplItems[i - 1].Data));
#endif
		else if (aTplItems[i - 1].Data)
#if defined(WINPORT_DIRECT)
			aDialogItems[i].PtrData = (TCHAR *)(aTplItems[i - 1].Data);
#else
			_tstrcpy((TCHAR *)aDialogItems[i].Data, (TCHAR *)(aTplItems[i - 1].Data));
#endif
	}
	aDialogItems[nFocus].Focus = TRUE;
	aDialogItems[nDefaultButton].DefaultButton = TRUE;

	if (sTitle)
#if defined(WINPORT_DIRECT)
		aDialogItems[0].PtrData = sTitle;
#else
		_tstrcpy((TCHAR *)aDialogItems[0].Data, sTitle);
#endif
	aDialogItems[0].X1 = 3;
	aDialogItems[0].Y1 = 1;
	aDialogItems[0].Y2 = aDialogItems[nItemsNumber - 1].Y1 + 1;

	for (i = 1; i < nItemsNumber; i++) {
#if defined(WINPORT_DIRECT)
		if (!aDialogItems[i].PtrData)
			continue;
		int w = _tstrlen((TCHAR *)aDialogItems[i].PtrData);
#else
		int w = _tstrlen((TCHAR *)aDialogItems[i].Data);
#endif
		if (nDialogWidth < w)
			nDialogWidth = w;
	}

	nDialogWidth+= 16;
	aDialogItems[0].X2 = nDialogWidth - 4;

#if defined(WINPORT_DIRECT)
	HANDLE hDlg = apiDialogInit(ModuleNumber, -1, -1, nDialogWidth, aDialogItems[nItemsNumber - 1].Y1 + 3,
			sHelpTopic, aDialogItems, nItemsNumber, 0, 0, NULL, 0);
	if (hDlg == INVALID_HANDLE_VALUE)
		return 0;

	int Ret = apiDialogRun(hDlg);

	for (i = 1; i < nItemsNumber; i++)
		aDialogItems[i].Selected = apiSendDlgMessage(hDlg, DM_GETCHECK, i, 0) == BSTATE_CHECKED;

	apiDialogFree(hDlg);

	return Ret;
#else
	return apiDialog(ModuleNumber, -1, -1, nDialogWidth, aDialogItems[0].Y2 + 2, sHelpTopic, aDialogItems,
			nItemsNumber);
#endif
}
