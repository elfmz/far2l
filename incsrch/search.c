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

#ifdef __WATCOMC__
extern void GetProgress(int nNum, char *sBuff, int nLen);
#pragma aux GetProgress parm[eax][edi][ecx] modify exact[eax ebx ecx edx edi] =                                \
				"dec     edi"                                                                                  \
				"mov     ebx, 10"                                                                              \
				"Next:   test    eax, eax"                                                                     \
				"jz      Fill"                                                                                 \
				"xor     edx, edx"                                                                             \
				"div     ebx"                                                                                  \
				"add     edx, 48"                                                                              \
				"mov     [edi][ecx], dl"                                                                       \
				"loop    Next"                                                                                 \
				"jmp     short Exit"                                                                           \
				"Fill:   mov     [edi][ecx], 48"                                                               \
				"loop    Fill"                                                                                 \
				"Exit:  ";
#elif defined(WINPORT_DIRECT)
static __inline void GetProgress(int nNum, TCHAR *sBuff, int nLen)
{
	apiSnprintf(sBuff, nLen + 1, _T("%*d"), nLen, nNum);
	sBuff[nLen] = _T('/');
}
#else
static __inline void GetProgress(int nNum, char *sBuff, int nLength)
{
	_asm {
		mov     eax, nNum
		mov     edi, sBuff
		mov     ecx, nLength
		dec     edi
		mov     ebx, 10

Next:	test    eax, eax
		jz      Fill
		xor     edx, edx
		div     ebx
		add     edx, 48         ; to ascii
		mov     [edi][ecx], dl
		loop    Next
		jmp     short Exit
Fill:	mov     [edi][ecx], 48  ; fill with '0'
		loop    Fill
Exit:
	}
}
#endif

#ifdef __SW_ET
static __etGetString(void)
{
	apiEditorControl(ECTL_SETPOSITION, &esp);
	apiEditorControl(ECTL_GETSTRING, &egs);
}

static void __etCollectEvents(void)
{
	CollectEvents();
}

static void __etShowProgress(int nLine, char *sTitle, int nLineChars)
{
	GetProgress(nLine, sTitle, nLineChars);
	apiText(48, 0, 0x0E, sTitle);
	apiText(48, 0, 0x0E, NULL);
}
#endif

static void Help(void)
{
	static const DialogTemplateItem DialogTemplate[11] = {
		{DI_TEXT,   2, 2,  FALSE, DIF_CENTERGROUP, (INT_PTR) _T("FAR Incremental Search plugin")},
		{DI_TEXT,   2, 3,  FALSE, DIF_CENTERGROUP, (INT_PTR) _T("Version 2.1")},
		{DI_TEXT,   2, 4,  FALSE, DIF_CENTERGROUP | DIF_BOXCOLOR | DIF_SEPARATOR, 0},
		{DI_TEXT,   2, 5,  FALSE, DIF_CENTERGROUP, (INT_PTR) _T("copyright (c) 1999-2019, Stanislav V. Mekhanoshin")},
		{DI_TEXT,   2, 6,  FALSE, DIF_CENTERGROUP, (INT_PTR) _T("This program comes with ABSOLUTELY NO WARRANTY")},
		{DI_TEXT,   2, 7,  FALSE, DIF_CENTERGROUP, (INT_PTR) _T("This is free software, and you are welcome to redistribute it")},
		{DI_TEXT,   2, 8,  FALSE, DIF_CENTERGROUP, (INT_PTR) _T("under certain conditions.")},
		{DI_TEXT,   2, 9,  FALSE, DIF_CENTERGROUP, (INT_PTR) _T("http://rampitec.us.to")},
		{DI_TEXT,   2, 10, FALSE, DIF_CENTERGROUP | DIF_BOXCOLOR | DIF_SEPARATOR, 0},
		{DI_TEXT,   2, 11, FALSE, DIF_CENTERGROUP, (INT_PTR) _T("Press F1 for help")},
		{DI_BUTTON, 2, 12, FALSE, DIF_CENTERGROUP, (INT_PTR) _T("Ok")}
	};
	struct FarDialogItem DialogItems[12];

	DialogFromTemplate(NULL, DialogTemplate, DialogItems, sizeof(DialogItems) / sizeof(DialogItems[0]),
			_T("Iface"), 11, 11);
}

static BOOL FindString(BOOL bNext, BOOL bForward)
{
	TCHAR *p = NULL;
	TCHAR *(*SubString)(const TCHAR *text, size_t N);
	int nLineChars;
	int nStopLine;
	int nStartLine;
	int nStartPos;
	int pos;
	DWORD nMs;
	DWORD nCurTic;
	int nStatusMessage = -1;
	TCHAR sPattern[MAX_STR];
	TCHAR sTitle[22];

	memcpy(sPattern, sStr, nLen * sizeof(sStr[0]));
#if !defined(WINPORT_DIRECT)
	ect.Text = sPattern;
	ect.TextLength = nLen;
	apiEditorControl(ECTL_OEMTOEDITOR, &ect);
#endif
	if (!bCaseSensitive)
		UpperCase(sPattern, sPattern, nLen);
	if (bForward)
		SetSubstringPatternL(sPattern);
	else
		SetSubstringPatternR(sPattern);

	SubString = bForward ? SubStringL : SubStringR;
	apiEditorControl(ECTL_GETSTRING, &egs);

	pos = utoar(ei.TotalLines, sTitle, sizeof(sTitle) / sizeof(sTitle[0]));
	nLineChars = sizeof(sTitle) / sizeof(sTitle[0]) - pos - 1;
	sTitle[nLineChars] = '/';
	memmovel(&sTitle[nLineChars + 1], &sTitle[pos], (nLineChars + 1) * sizeof(sTitle[0]));

	nStartLine = ei.CurLine;
	nStartPos = ei.CurPos;
	pos = (egs.SelStart == -1) ? nStartPos : egs.SelStart;
	if (bNext && bForward)
		pos++;
	else if (!bForward) {
		pos+= nLen;
		if (bNext)
			pos--;
		if (pos > egs.StringLength)
			pos = egs.StringLength;
	}

	if (egs.StringLength >= nLen && ((pos < egs.StringLength && bForward) || (!bForward && pos > 0))) {
		/*
		GetProgress(nStartLine+1,sTitle,nLineChars);
		apiText(48,0,0x0E,sTitle);
		apiText(48,0,0x0E,NULL);
		*/
		if (bForward)
			p = SubStringL(&egs.StringText[pos], egs.StringLength - pos);
		else
			p = SubStringR(egs.StringText, pos);
		if (p) {
			pos = -1;
			goto Success;
		}
	}

	esp.CurLine = nStartLine;
	esp.CurPos = -1;
	esp.LeftPos = 0;
	nMs = GetTickCount();
	if (bRestartEOF) {
		nStopLine = nStartLine;
		goto StartLoop;
	} else
		nStopLine = bForward ? ei.TotalLines - 1 : 0;
	while (esp.CurLine != nStopLine) {
	StartLoop:
		if (bForward) {
			esp.CurLine++;
			if (bRestartEOF && esp.CurLine == ei.TotalLines) {
				nStatusMessage = MEOF;
				StatusMessage(MEOF);
				esp.CurLine = 0;
			}
		} else {
			if (bRestartEOF && esp.CurLine == 0) {
				nStatusMessage = MBOF;
				StatusMessage(MBOF);
				esp.CurLine = ei.TotalLines;
			}
			esp.CurLine--;
		}
		nCurTic = GetTickCount();
		if (nMs + 200 < nCurTic) {
#ifdef __SW_ET
			__etCollectEvents();
#else
			CollectEvents();
#endif
			if (bTermEvent)
				goto Break;
#ifdef __SW_ET
			__etShowProgress(esp.CurLine + 1, sTitle, nLineChars);
#else
			GetProgress(esp.CurLine + 1, sTitle, nLineChars);
			apiText(48, 0, 0x0E, sTitle);
			apiText(48, 0, 0x0E, NULL);
#endif
			nMs = nCurTic;
		}
#ifdef __SW_ET
		__etGetString();
#else
		apiEditorControl(ECTL_SETPOSITION, &esp);
		apiEditorControl(ECTL_GETSTRING, &egs);
#endif
		if (!egs.StringLength)
			continue;
		p = SubString(egs.StringText, egs.StringLength);
		if (p) {
			if (bRestartEOF && nStartLine == esp.CurLine && nStartPos == (p - egs.StringText) + nLen)
				p = NULL;
			pos = esp.CurLine;
			break;
		}
	}
Break:
	esp.TopScreenLine = ei.TopScreenLine;
	SetPosition(nStartLine, nStartPos, ei.LeftPos);
	esp.TopScreenLine = -1;
	if (p) {
	Success:
		PositionToView(pos, p - egs.StringText);
		SelectFound(!bStopOnFound);
		if (nStatusMessage >= 0)
			StatusMessage(nStatusMessage);
		return TRUE;
	}
	if (!bTermEvent)
		bNotFound = TRUE;
	return FALSE;
}

void SearchLoopEditor(void)
{
	/* BOOL bRedraw; */
	int aHistLines[MAX_STR + 1], aHistCols[MAX_STR + 1];
	int i;
	/*
	#if defined(_MSC_VER) && defined(_DEBUG)
		_asm{
			int 3;
		}
	#endif
	*/
	bEscape = FALSE;
	bTermEvent = FALSE;
	nEvents = 0;
#if !defined(WINPORT_DIRECT)
	hInputHandle = GetStdHandle(STD_INPUT_HANDLE);
#endif
	bNotFound = FALSE;

	apiEditorControl(ECTL_GETINFO, &ei);
	esp.CurTabPos = -1;
	esp.TopScreenLine = -1;
	esp.Overtype = -1;
	egs.StringNumber = -1;

	nLen = 0;

	/* There is no 0 lines possible in FAR!
	 * if( !ei.TotalLines )return;
	 */

	if (bThisUseSelection) {
		if (ei.BlockType != BTYPE_NONE) {
			apiEditorControl(ECTL_GETSTRING, &egs);
			if (egs.SelStart != -1 && egs.SelStart < egs.StringLength) {
				nLen = ((egs.SelEnd == -1 || egs.SelEnd > egs.StringLength) ? egs.StringLength : egs.SelEnd)
						- egs.SelStart;
				if (nLen > MAX_STR)
					nLen = MAX_STR;
				memcpy(sStr, &egs.StringText[egs.SelStart], nLen * sizeof(sStr[0]));
#if !defined(WINPORT_DIRECT)
				ect.Text = sStr;
				ect.TextLength = nLen;
				apiEditorControl(ECTL_EDITORTOOEM, &ect);
#endif
				/*
				bRedraw = egs.SelStart+nLen >= ei.WindowSizeX+ei.LeftPos ||
						  egs.SelStart < ei.LeftPos;
				*/
				PositionToView(-1, egs.SelStart);
				if (bThisAutoNext) {
					aEvents[0].Flags = KC_NEXT;
					nEvents++;
				}
			} else
				goto NoBlock;
		} else
		NoBlock:
			if (bStopOnFound)
				return;
	}

	for (i = 0; i <= nLen; i++) {
		aHistLines[i] = ei.CurLine;
		aHistCols[i] = ei.CurPos - (nLen - i);
	}

	ShowTitle(OPEN_EDITOR);
	if (!bStopOnFound || ei.BlockStartLine != ei.CurLine || ei.BlockType != BTYPE_STREAM
			|| egs.SelStart + nLen != egs.SelEnd)
		SelectFound(/*!bStopOnFound || bRedraw*/ TRUE);

#if !defined(WINPORT_DIRECT)
	InitUpcaseTable(ei.TableNum, ei.AnsiMode);
#endif

	for (;;) {
		if (!bStopOnFound)
			if (!CollectEvents())
				WaitInput(TRUE);
		while (nEvents || bTermEvent) {
			BOOL bFail = FALSE;
			if (bTermEvent && !bStopOnFound) {
				if (!bKeepSelection) {
					nLen = 0;
					SelectFound(FALSE);
				}
				if (!bEscape) {
					apiEditorControl(ECTL_REDRAW, NULL);
					apiEditorControl(ECTL_PROCESSINPUT, &Event);
				}
				apiEditorControl(ECTL_SETTITLE, NULL);
				return;
			}
			switch (aEvents[0].Flags & ~KC_FAILKILL) {
				case KC_BACK:
					if (nLen) {
						nLen--;
						ShowTitle(OPEN_EDITOR);
						if (bBSunroll)
							SetPosition(aHistLines[nLen], aHistCols[nLen], -1);
						else
							SetPosition(-1, ei.CurPos - 1, -1);
						apiEditorControl(ECTL_GETINFO, &ei);
						SelectFound(TRUE);
					}
					goto Next;
				case KC_NEXT:
				case KC_PREV:
					if (nLen) {
						ShowTitle(OPEN_EDITOR);
						/* to force FAR clean up status messages --  *
						 * redraw is too slow...                     *
						 * apiEditorControl(ECTL_REDRAW,NULL);       */
						if (!FindString(TRUE, (aEvents[0].Flags == KC_PREV) ? bReverse : !bReverse)) {
							ShowTitle(OPEN_EDITOR);
							bFail = TRUE;
						}
						if (bStopOnFound)
							return;
					}
					goto Next;
				case KC_CLEARMSG:
					ShowTitle(OPEN_EDITOR);
					goto Next;
				case KC_HELP:
					Help();
					goto Next;
				case KC_CHAR:
					if (nLen == MAX_STR) {
#if defined(WINPORT_DIRECT)
						putwchar('\007');
#else
						MessageBeep((UINT)-1);
#endif
						break;
					}
					sStr[nLen++] = aEvents[0].AsciiChar;
					ShowTitle(OPEN_EDITOR);
					if (!FindString(FALSE, !bReverse)) {
						nLen--;
						ShowTitle(OPEN_EDITOR);
						bFail = TRUE;
					} else {
						aHistLines[nLen] = ei.CurLine;
						aHistCols[nLen] = ei.CurPos;
					}
			}
		Next:
			do {
				if (--nEvents)
					memmovel(aEvents, &aEvents[1], sizeof(aEvents[0]) * nEvents);
			} while (bFail && nEvents && (aEvents[0].Flags & KC_FAILKILL));
		}
	}
}

#ifdef VIEWVER_SUPPORT
void SearchLoopViewer(void)
{
	struct ViewerInfo vi;
	bEscape = FALSE;
	bTermEvent = FALSE;
	nEvents = 0;
	hInputHandle = GetStdHandle(STD_INPUT_HANDLE);
	bNotFound = FALSE;

	vi.StructSize = sizeof(vi);
	apiViewerControl(VCTL_GETINFO, &vi);

	nLen = 0;
	ShowTitle(OPEN_VIEWER);

#if !defined(UNICODE)
	if (!vi.CurMode.Unicode)
		InitUpcaseTable(vi.CurMode.UseDecodeTable ? vi.CurMode.TableNum : -1, vi.CurMode.AnsiMode);
#endif

	for (;;) {
		if (!bStopOnFound)
			if (!CollectEvents())
				WaitForSingleObject(hInputHandle, INFINITE);
		while (nEvents || bTermEvent) {
			BOOL bFail = FALSE;
			if (bTermEvent && !bStopOnFound) {
				if (!bKeepSelection) {
					nLen = 0;
					SelectFound(FALSE);
				}
				if (!bEscape) {
					apiEditorControl(ECTL_REDRAW, NULL);
					apiEditorControl(ECTL_PROCESSINPUT, &Event);
				}
				return;
			}
			switch (aEvents[0].Flags & ~KC_FAILKILL) {
				case KC_BACK:
					if (nLen) {
						nLen--;
						ShowTitle();
						if (bBSunroll)
							SetPosition(aHistLines[nLen], aHistCols[nLen], -1);
						else
							SetPosition(-1, ei.CurPos - 1, -1);
						apiEditorControl(ECTL_GETINFO, &ei);
						SelectFound(TRUE);
					}
					goto Next;
				case KC_NEXT:
				case KC_PREV:
					if (nLen) {
						ShowTitle();
						if (!FindString(TRUE, (aEvents[0].Flags == KC_PREV) ? bReverse : !bReverse)) {
							ShowTitle();
							bFail = TRUE;
						}
						if (bStopOnFound)
							return;
					}
					goto Next;
				case KC_CLEARMSG:
					ShowTitle();
					goto Next;
				case KC_HELP:
					Help();
					goto Next;
				case KC_CHAR:
					if (nLen == MAX_STR) {
						MessageBeep((UINT)-1);
						break;
					}
					sStr[nLen++] = aEvents[0].AsciiChar;
					ShowTitle();
					if (!FindString(FALSE, !bReverse)) {
						nLen--;
						ShowTitle();
						bFail = TRUE;
					} else {
						aHistLines[nLen] = ei.CurLine;
						aHistCols[nLen] = ei.CurPos;
					}
			}
		Next:
			do {
				if (--nEvents)
					memmovel(aEvents, &aEvents[1], sizeof(aEvents[0]) * nEvents);
			} while (bFail && nEvents && (aEvents[0].Flags & KC_FAILKILL));
		}
	}
}
#endif	// VIEWVER_SUPPORT
