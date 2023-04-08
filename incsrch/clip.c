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

void PasteSearchText(void)
{
	TCHAR *p;
	size_t len = 0, i;
	TCHAR pData[MAX_STR];

	pData[0] = 0;
	if (!OpenClipboard(NULL))
		return;
#if defined(WINPORT_DIRECT)
	p = GetClipboardData(CF_UNICODETEXT);
	len = _tstrnlen(p, MAX_STR);
	memcpy(pData, p, len * sizeof(pData[0]));
#else
	for (nFmt = 0; (nFmt = EnumClipboardFormats(nFmt)) != 0;) {
		switch (nFmt) {
			case CF_TEXT:
				p = GetClipboardData(CF_TEXT);
				if (p) {
					len = _tstrnlen(p, MAX_STR);
					CharToOemBuff(p, pData, len);
					goto Ok;
				}
				continue;
			case CF_OEMTEXT:
				p = GetClipboardData(CF_OEMTEXT);
				if (p) {
				OEM:
					len = _tstrnlen(p, MAX_STR);
					memcpy(pData, p, len * sizeof(pData[0]));
					goto Ok;
				}
				continue;
			case CF_UNICODETEXT:
				p = GetClipboardData(CF_UNICODETEXT);
				if (p) {
					WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, (LPCWSTR)p, -1, pData, MAX_STR, NULL,
							NULL);
					len = _tstrnlen(pData, MAX_STR);
					goto Ok;
				}
				continue;
		}
	}
	p = GetClipboardData(CF_OEMTEXT);
	if (p)
		goto OEM;
#endif
	CloseClipboard();
	for (i = 0; i < len; i++) {
		if (nEvents == PREVIEW_EVENTS) {
#if defined(WINPORT_DIRECT)
			putwchar('\007');
#else
			MessageBeep((UINT)-1);
#endif
			break;
		}
		aEvents[nEvents].Flags = (unsigned char)(i ? KC_CHAR | KC_FAILKILL : KC_CHAR);
		aEvents[nEvents].AsciiChar = pData[i];
		nEvents++;
	}
}
