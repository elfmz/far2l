#pragma once

/*
history.hpp

История (Alt-F8, Alt-F11, Alt-F12)
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

#include "DList.hpp"

class Dialog;
class VMenu;

enum enumHISTORYTYPE
{
	HISTORYTYPE_CMD,
	HISTORYTYPE_FOLDER,
	HISTORYTYPE_VIEW,
	HISTORYTYPE_DIALOG
};

struct HistoryRecord
{
	bool   Lock;
	int    Type;
	FARString strName;
	FILETIME Timestamp;

	HistoryRecord()
	{
		Lock = false;
		Type = 0;
		strName.Clear();
		Timestamp.dwLowDateTime=0;
		Timestamp.dwHighDateTime=0;
	}

	const HistoryRecord& operator=(const HistoryRecord &rhs)
	{
		if (this != &rhs)
		{
			strName = rhs.strName;
			Type = rhs.Type;
			Lock = rhs.Lock;
			Timestamp.dwLowDateTime  = rhs.Timestamp.dwLowDateTime;
			Timestamp.dwHighDateTime = rhs.Timestamp.dwHighDateTime;
		}

		return *this;
	}
};

class History
{
	private:
		std::string strRegKey;
		bool EnableAdd, KeepSelectedPos, SaveType;
		int RemoveDups;
		enumHISTORYTYPE TypeHistory;
		size_t HistoryCount;
		const int *EnableSave;
		DList<HistoryRecord> HistoryList;
		HistoryRecord *CurrentItem;
		struct stat LoadedStat{};

	private:
		void AddToHistoryLocal(const wchar_t *Str, const wchar_t *Prefix, int Type);
		bool EqualType(int Type1, int Type2);
		const wchar_t *GetTitle(int Type);
		int ProcessMenu(FARString &strStr, const wchar_t *Title, VMenu &HistoryMenu, int Height, int &Type, Dialog *Dlg);
		bool ReadHistory(bool bOnlyLines=false);
		bool SaveHistory();
		void SyncChanges();

	public:
		History(enumHISTORYTYPE TypeHistory, size_t HistoryCount, const std::string &RegKey, const int *EnableSave, bool SaveType);
		~History();

	public:
		void AddToHistory(const wchar_t *Str, int Type=0, const wchar_t *Prefix=nullptr, bool SaveForbid=false);
		static bool ReadLastItem(const char *RegKey, FARString &strStr);
		int  Select(const wchar_t *Title, const wchar_t *HelpTopic, FARString &strStr, int &Type);
		int  Select(VMenu &HistoryMenu, int Height, Dialog *Dlg, FARString &strStr);
		void GetPrev(FARString &strStr);
		void GetNext(FARString &strStr);
		bool GetSimilar(FARString &strStr, int LastCmdPartLength, bool bAppend=false);
		bool GetAllSimilar(VMenu &HistoryMenu,const wchar_t *Str);
		void SetAddMode(bool EnableAdd, int RemoveDups, bool KeepSelectedPos);
		void ResetPosition() { CurrentItem = nullptr; }
};
