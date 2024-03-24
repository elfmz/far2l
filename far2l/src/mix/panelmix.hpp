#pragma once

/*
panelmix.hpp

Misc functions for processing of path names
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

class Panel;

void ShellUpdatePanels(Panel *SrcPanel, BOOL NeedSetUpADir = FALSE);
bool CheckUpdateAnotherPanel(Panel *SrcPanel, const wchar_t *SelName);

int _MakePath1(DWORD Key, FARString &strPathName, const wchar_t *Param2, int escaping = 1); // by default escaping filenames

const FARString FormatStr_Attribute(DWORD FileAttributes, DWORD UnixMode, int Width = -1);
const FARString FormatStr_DateTime(const FILETIME *FileTime, int ColumnType, DWORD Flags, int Width);
const FARString FormatStr_Size(int64_t FileSize, int64_t PhysicalSize, const FARString &strName,
		DWORD FileAttributes, uint8_t ShowFolderSize, int ColumnType, DWORD Flags, int Width);
void TextToViewSettings(const wchar_t *ColumnTitles, const wchar_t *ColumnWidths,
		unsigned int *ViewColumnTypes, int *ViewColumnWidths, int *ViewColumnWidthsTypes, int &ColumnCount);
void ViewSettingsToText(unsigned int *ViewColumnTypes, int *ViewColumnWidths, int *ViewColumnWidthsTypes,
		int ColumnCount, FARString &strColumnTitles, FARString &strColumnWidths);
