#pragma once

/*
hilight.hpp

Files highlighting
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

#include "CFileMask.hpp"
#include "array.hpp"

#define	HIGHLIGHT_MAX_MARK_LENGTH	8

class VMenu;
class FileFilterParams;
struct FileListItem;

enum enumHighlightDataColor
{
	HIGHLIGHTCOLOR_NORMAL = 0,
	HIGHLIGHTCOLOR_SELECTED,
	HIGHLIGHTCOLOR_UNDERCURSOR,
	HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR,

	HIGHLIGHTCOLORTYPE_FILE     = 0,
	HIGHLIGHTCOLORTYPE_MARKSTR  = 1,
};

struct HighlightDataColor
{
	uint64_t Color[2][4];	// [0=file, 1=mark][0=normal,1=selected,2=undercursor,3=selectedundercursor];
							// nonzero upper 3 bytes meaning foreground RGB, nonzero lower 3 bytes meaning background RGB
	uint64_t Mask[2][4];	// transparency mask, 0 = fully transparent
	wchar_t	 Mark[HIGHLIGHT_MAX_MARK_LENGTH + 1]; 	// + null terminator
	uint32_t MarkLen;
	bool	 bMarkInherit;
};

class HighlightFiles
{
private:
	TPointerArray<FileFilterParams> HiData;
	int FirstCount, UpperCount, LowerCount, LastCount;
	uint64_t CurrentTime;

private:
	void InitHighlightFiles();
	void ClearData();

	int MenuPosToRealPos(int MenuPos, int **Count, bool Insert = false);
	void FillMenu(VMenu *HiMenu, int MenuPos);
	void ProcessGroups();

public:
	HighlightFiles();
	~HighlightFiles();

public:
	void UpdateCurrentTime();
	void GetHiColor(FileListItem **FileItem, int FileCount, bool UseAttrHighlighting = false);
	int GetGroup(const FileListItem *fli);
	void HiEdit(int MenuPos);

	void SaveHiData();
};

const HighlightDataColor *PooledHighlightDataColor(const HighlightDataColor &Color);
