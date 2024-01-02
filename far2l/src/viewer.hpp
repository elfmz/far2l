#pragma once

/*
viewer.hpp

Internal viewer
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

#include "scrobj.hpp"
#include "namelist.hpp"
#include <farplug-wide.h>
#include "poscache.hpp"
#include "config.hpp"
#include "cache.hpp"
#include "fileholder.hpp"
#include "ViewerStrings.hpp"

#define VIEWER_UNDO_COUNT 64

enum
{
	VIEW_UNWRAP   = 0,
	VIEW_WRAP     = 1,
	VIEW_WORDWRAP = 2
};

class FileViewer;
class KeyBar;

struct InternalViewerBookMark
{
	DWORD64 SavePosAddr[POSCACHE_BOOKMARK_COUNT];
	DWORD64 SavePosLeft[POSCACHE_BOOKMARK_COUNT];
};

struct ViewerUndoData
{
	int64_t UndoAddr;
	int64_t UndoLeft;
};

enum SEARCH_FLAGS
{
	SEARCH_MODE2   = 0x00000001,
	REVERSE_SEARCH = 0x00000002
};

enum SHOW_MODES
{
	SHOW_RELOAD,
	SHOW_HEX,
	SHOW_UP,
	SHOW_DOWN
};

class Viewer : public ScreenObject
{
	friend class FileViewer;

private:
	unsigned iBoostPg = 0;
	BitFlags SearchFlags;

	struct ViewerOptions ViOpt;

	NamesList ViewNamesList;
	KeyBar *ViewKeyBar;

	ViewerStrings Strings;

	FARString strFullFileName;

	BufferedFileView ViewFile;

	FAR_FIND_DATA_EX ViewFindData;

	FARString strProcessedViewName;

	FARString strLastSearchStr;
	int LastSearchCase, LastSearchWholeWords, LastSearchReverse, LastSearchHex, LastSearchRegexp;

	struct ViewerMode VM;

	int64_t FilePos;
	int64_t SecondPos;
	int64_t LastScrPos;
	int64_t FileSize;
	int64_t LastSelPos;

	int64_t LeftPos;
	int64_t LastPage;
	int CRSym;
	int64_t SelectPos, SelectSize;
	DWORD SelectFlags;
	int ShowStatusLine, HideCursor;

	FARString strTitle;

	FARString strPluginData;
	int CodePageChangedByUser;
	int InternalKey;

	struct InternalViewerBookMark BMSavePos;
	struct ViewerUndoData UndoData[VIEWER_UNDO_COUNT];

	int LastKeyUndo;
	int Width, XX2;		// , используется при расчете ширины при скролбаре
	int ViewerID;
	bool OpenFailed;
	bool bVE_READ_Sent;
	FileViewer *HostFileViewer;
	bool AdjustSelPosition;

	bool m_bQuickView;

	UINT DefCodePage;

	FileHolderPtr FHP;

private:
	virtual void DisplayObject();

	void ShowPage(int nMode);

	void FilePosShiftLeft(uint64_t Offset);
	void Up();
	void ShowHex();
	void ShowStatus();
	/*
		$ 27.04.2001 DJ
		функции для рисования скроллбара, для корректировки ширины в
		зависимости от наличия скроллбара и для корректировки позиции файла
		на границу строки
	*/
	void DrawScrollbar();
	void AdjustWidth();
	void AdjustFilePos();

	void ReadString(ViewerString &rString, int MaxSize, int StrSize);
	int CalcStrSize(const wchar_t *Str, int Length);
	void ChangeViewKeyBar();
	void SetCRSym();
	void Search(int Next, int FirstChar);
	void ConvertToHex(char *SearchStr, int &SearchLength);
	int HexToNum(int Hex);

	int vread(wchar_t *Buf, int Count, bool Raw = false);
	void vseek(int64_t Offset, int Whence);

	int64_t vtell();
	bool vgetc(WCHAR &C);
	void SetFileSize();
	int GetStrBytesNum(const wchar_t *Str, int Length);

	FARString ComposeCacheName();
	void SavePosCache();

public:
	Viewer(bool bQuickView = false, UINT aCodePage = CP_AUTODETECT);
	virtual ~Viewer();

public:
	int OpenFile(FileHolderPtr NewFileHolder, int warning);
	void SetViewKeyBar(KeyBar *ViewKeyBar);

	virtual int ProcessKey(FarKey Key);
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual int64_t VMProcess(MacroOpcode OpCode, void *vParam = nullptr, int64_t iParam = 0);

	void SetStatusMode(int Mode);
	void EnableHideCursor(int HideCursor);
	int GetWrapMode();
	void SetWrapMode(int Wrap);
	int GetWrapType();
	void SetWrapType(int TypeWrap);
	void KeepInitParameters();
	void GetFileName(FARString &strName);
	void SetProcessed(bool Processed);
	bool GetProcessed() const { return VM.Processed; }
	virtual void ShowConsoleTitle();

	void SetTitle(const wchar_t *Title);
	FARString &GetTitle(FARString &Title, int SubLen = -1, int TruncSize = 0);

	FileHolderPtr &GetFileHolder() { return FHP; }

	void SetFilePos(int64_t Pos);	// $ 18.07.2000 tran - change 'long' to 'unsigned long'
	int64_t GetFilePos() const { return FilePos; };
	int64_t GetViewFilePos() const { return FilePos; };
	int64_t GetViewFileSize() const { return FileSize; };

	void SetPluginData(const wchar_t *PluginData);
	void SetNamesList(NamesList *List);

	int ViewerControl(int Command, void *Param);
	void SetHostFileViewer(FileViewer *Viewer) { HostFileViewer = Viewer; };

	void GoTo(int ShowDlg = TRUE, int64_t NewPos = 0, DWORD Flags = 0);
	void GetSelectedParam(int64_t &Pos, int64_t &Length, DWORD &Flags);
	// Функция выделения - как самостоятельная функция
	void SelectText(const int64_t &MatchPos, const int64_t &SearchLength, const DWORD Flags = 0x1);

	int GetTabSize() const { return ViOpt.TabSize; }
	void SetTabSize(int newValue) { ViOpt.TabSize = newValue; }

	int GetAutoDetectCodePage() const { return ViOpt.AutoDetectCodePage; }
	void SetAutoDetectCodePage(int newValue) { ViOpt.AutoDetectCodePage = newValue; }

	int GetShowScrollbar() const { return ViOpt.ShowScrollbar; }
	void SetShowScrollbar(int newValue) { ViOpt.ShowScrollbar = newValue; }

	int GetShowArrows() const { return ViOpt.ShowArrows; }
	void SetShowArrows(int newValue) { ViOpt.ShowArrows = newValue; }
	/* IS $ */
	int GetPersistentBlocks() const { return ViOpt.PersistentBlocks; }
	void SetPersistentBlocks(int newValue) { ViOpt.PersistentBlocks = newValue; }

	int GetHexMode() const { return VM.Hex; }

	UINT GetCodePage() const { return VM.CodePage; }

	NamesList *GetNamesList() { return &ViewNamesList; }

	int ProcessHexMode(int newMode, bool isRedraw = TRUE);
	int ProcessWrapMode(int newMode, bool isRedraw = TRUE);
	int ProcessTypeWrapMode(int newMode, bool isRedraw = TRUE);
};
