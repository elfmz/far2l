#pragma once

/*
filefilterparams.hpp

Ïàðàìåòðû Ôàéëîâîãî ôèëüòðà
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

#include "plugin.hpp"
#include "CFileMask.hpp"
#include "bitflags.hpp"
#include "hilight.hpp"

#define FILEFILTER_SIZE_SIZE 32

#define DEFAULT_SORT_GROUP 10000

enum enumFileFilterFlagsType
{
	FFFT_FIRST = 0, //îáÿçàí áûòü ïåðâûì

	FFFT_LEFTPANEL = FFFT_FIRST,
	FFFT_RIGHTPANEL,
	FFFT_FINDFILE,
	FFFT_COPY,
	FFFT_SELECT,
	FFFT_CUSTOM,

	FFFT_COUNT, //îáÿçàí áûòü ïîñëåäíèì
};

enum enumFileFilterFlags
{
	FFF_NONE    = 0x00000000,
	FFF_INCLUDE = 0x00000001,
	FFF_EXCLUDE = 0x00000002,
	FFF_STRONG  = 0x10000000
};

enum enumFDateType
{
	FDATE_MODIFIED=0,
	FDATE_CREATED,
	FDATE_OPENED,
	FDATE_CHANGED,

	FDATE_COUNT, // âñåãäà ïîñëåäíèé !!!
};

class FileFilterParams
{
	private:

		string m_strTitle;

		struct
		{
			bool Used;
			string strMask;
			CFileMask FilterMask; // Õðàíèëèùå ñêîìïèëèðîâàííîé ìàñêè.
		} FMask;

		struct
		{
			bool Used;
			enumFDateType DateType;
			ULARGE_INTEGER DateAfter;
			ULARGE_INTEGER DateBefore;
			bool bRelative;
		} FDate;

		struct
		{
			uint64_t SizeAboveReal; // Çäåñü âñåãäà áóäåò ðàçìåð â áàéòàõ
			uint64_t SizeBelowReal; // Çäåñü âñåãäà áóäåò ðàçìåð â áàéòàõ
			wchar_t SizeAbove[FILEFILTER_SIZE_SIZE]; // Çäåñü âñåãäà áóäåò ðàçìåð êàê åãî ââ¸ë þçåð
			wchar_t SizeBelow[FILEFILTER_SIZE_SIZE]; // Çäåñü âñåãäà áóäåò ðàçìåð êàê åãî ââ¸ë þçåð
			bool Used;
		} FSize;

		struct
		{
			bool Used;
			DWORD AttrSet;
			DWORD AttrClear;
		} FAttr;

		struct
		{
			HighlightDataColor Colors;
			int SortGroup;
			bool bContinueProcessing;
		} FHighlight;

		DWORD FFlags[FFFT_COUNT];

	public:

		FileFilterParams();

		const FileFilterParams &operator=(const FileFilterParams &FF);

		void SetTitle(const wchar_t *Title);
		void SetMask(bool Used, const wchar_t *Mask);
		void SetDate(bool Used, DWORD DateType, FILETIME DateAfter, FILETIME DateBefore, bool bRelative);
		void SetSize(bool Used, const wchar_t *SizeAbove, const wchar_t *SizeBelow);
		void SetAttr(bool Used, DWORD AttrSet, DWORD AttrClear);
		void SetColors(HighlightDataColor *Colors);
		void SetSortGroup(int SortGroup) { FHighlight.SortGroup = SortGroup; }
		void SetContinueProcessing(bool bContinueProcessing) { FHighlight.bContinueProcessing = bContinueProcessing; }
		void SetFlags(enumFileFilterFlagsType FType, DWORD Flags) { FFlags[FType] = Flags; }
		void ClearAllFlags() { memset(FFlags,0,sizeof(FFlags)); }

		const wchar_t *GetTitle() const;
		bool  GetMask(const wchar_t **Mask) const;
		bool  GetDate(DWORD *DateType, FILETIME *DateAfter, FILETIME *DateBefore, bool *bRelative) const;
		bool  GetSize(const wchar_t **SizeAbove, const wchar_t **SizeBelow) const;
		bool  GetAttr(DWORD *AttrSet, DWORD *AttrClear) const;
		void  GetColors(HighlightDataColor *Colors) const;
		int   GetMarkChar() const;
		int   GetSortGroup() const { return FHighlight.SortGroup; }
		bool  GetContinueProcessing() const { return FHighlight.bContinueProcessing; }
		DWORD GetFlags(enumFileFilterFlagsType FType) const { return FFlags[FType]; }

		// Äàííûé ìåòîä âûçûâàåòñÿ "ñíàðóæè" è ñëóæèò äëÿ îïðåäåëåíèÿ:
		// ïîïàäàåò ëè ôàéë fd ïîä óñëîâèå óñòàíîâëåííîãî ôèëüòðà.
		// Âîçâðàùàåò true  - ïîïàäàåò;
		//            false - íå ïîïàäàåò.
		bool FileInFilter(const FileListItem& fli, uint64_t CurrentTime);
		bool FileInFilter(const FAR_FIND_DATA_EX& fde, uint64_t CurrentTime);
		bool FileInFilter(const FAR_FIND_DATA& fd, uint64_t CurrentTime);
};

bool FileFilterConfig(FileFilterParams *FF, bool ColorConfig=false);

//Öåíòðàëèçîâàííàÿ ôóíêöèÿ äëÿ ñîçäàíèÿ ñòðîê ìåíþ ðàçëè÷íûõ ôèëüòðîâ.
void MenuString(string &strDest, FileFilterParams *FF, bool bHighlightType=false, int Hotkey=0, bool bPanelType=false, const wchar_t *FMask=nullptr, const wchar_t *Title=nullptr);
