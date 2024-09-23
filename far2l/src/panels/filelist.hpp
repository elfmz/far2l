#pragma once

/*
filelist.hpp

Файловая панель - общие функции
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

#include "panel.hpp"
#include "dizlist.hpp"
#include "filefilterparams.hpp"
#include "DList.hpp"
#include "panelctype.hpp"
#include "plugins.hpp"
#include "ConfigRW.hpp"
#include "FSNotify.h"
#include <memory>
#include <map>
#include <vector>
#include <deque>

extern const HighlightDataColor ZeroColors;

struct FileListItem
{
	FileListItem(const FileListItem &)            = delete;
	FileListItem &operator=(const FileListItem &) = delete;

	FileListItem();
	~FileListItem();

	FARString strName;
	FARString strOwner, strGroup;
	FARString strCustomData;

	uint64_t FileSize{};
	uint64_t PhysicalSize{};

	FILETIME CreationTime{};
	FILETIME AccessTime{};
	FILETIME WriteTime{};
	FILETIME ChangeTime{};

	wchar_t *DizText{};
	wchar_t **CustomColumnData{};

	DWORD_PTR UserData{};

	const HighlightDataColor *ColorsPtr = &ZeroColors;

	DWORD NumberOfLinks{};
	DWORD UserFlags{};
	DWORD FileAttr{};
	DWORD FileMode{};
	DWORD CRC32{};

	unsigned int Position{};	// for unsorted sorting..
	int SortGroup{};
	int CustomColumnNumber{};

	bool Selected{};
	bool PrevSelected{};
	bool DeleteDiz{};
	uint8_t ShowFolderSize{};

	/// temporary values used to optimize sorting, they fit into
	/// 8-bytes alignment gap so there is no memory waisted
	unsigned short FileNamePos{};	// offset from beginning of StrName
	unsigned short FileExtPos{};	// offset from FileNamePos
};

template <class T>
struct StdVecWrap : protected std::vector<T>
{
	using typename std::vector<T>::iterator;
	using std::vector<T>::begin;
	using std::vector<T>::end;
	using std::vector<T>::operator[];

	void ReserveExtra(int extra) { std::vector<T>::reserve(std::vector<T>::size() + (size_t)extra); }

	void Swap(StdVecWrap<T> &other) { return std::vector<T>::swap(other); }

	inline T *Data() { return std::vector<T>::data(); }
	inline int Count() const { return (int)std::vector<T>::size(); }
	inline bool IsEmpty() const { return std::vector<T>::empty(); }
};

struct ListDataVec : StdVecWrap<FileListItem *>
{
	ListDataVec(const ListDataVec &)            = delete;
	ListDataVec &operator=(const ListDataVec &) = delete;

	ListDataVec();
	~ListDataVec();

	void Clear();

	FileListItem *Add();

	// занести предопределенные данные для каталога ".."
	FileListItem *AddParentPoint();
	FileListItem *AddParentPoint(const FILETIME *Times, FARString Owner, FARString Group);
};

struct PluginPanelItemVec : StdVecWrap<PluginPanelItem>
{
	PluginPanelItemVec(const PluginPanelItemVec &)            = delete;
	PluginPanelItemVec &operator=(const PluginPanelItemVec &) = delete;

	PluginPanelItemVec();
	~PluginPanelItemVec();

	void Add(FileListItem *CreateFrom);
	void Clear();
};

struct PluginsListItem
{
	HANDLE hPlugin;
	FARString strHostFile;
	FARString strPrevOriginalCurDir;
	std::map<FARString, FARString> Dir2CursorFile;
	int Modified;
	int PrevViewMode;
	int PrevSortMode;
	int PrevSortOrder;
	int PrevNumericSort;
	int PrevCaseSensitiveSort;
	int PrevDirectoriesFirst;
	PanelViewSettings PrevViewSettings;
};

struct PrevDataItem
{
	ListDataVec PrevListData;
	FARString strPrevName;
	int PrevTopFile;
};

class FileList : public Panel
{
private:
	class FileFilter *Filter;
	DizList Diz;
	int DizRead;
	/*
		$ 09.11.2001 IS
		Открывающий и закрывающий символ, которые используются для показа
		имени, которое не помещается в панели. По умолчанию - фигурные скобки.
	*/
	wchar_t openBracket[2], closeBracket[2];

	FARString strOriginalCurDir;
	FARString strPluginDizName;
	ListDataVec ListData;
	std::map<std::wstring, FARString> SymlinksCache;
	HANDLE hPlugin;
	DList<PrevDataItem *> PrevDataList;
	DList<PluginsListItem *> PluginsList;
	std::unique_ptr<IFSNotify> ListChange;
	long UpperFolderTopFile, LastCurFile;
	long ReturnCurrentFile;
	long SelFileCount;
	long GetSelPosition, LastSelPosition;
	long TotalFileCount;
	uint64_t SelFileSize;
	uint64_t TotalFileSize;
	uint64_t FreeDiskSize;
	size_t	MarkLM;
	clock_t LastUpdateTime;
	int Height, Columns;
	std::deque<std::string> _symlinks_backlog;

	int ColumnsInGlobal;

	int LeftPos;
	int ShiftSelection;
	int MouseSelection;
	int SelectedFirst;
	int IsEmpty;	// указывает на полностью пустую колонку
	int AccessTimeUpdateRequired;

	int UpdateRequired, UpdateRequiredMode, UpdateDisabled;
	int SortGroupsRead;
	int InternalProcessKey;

	long CacheSelIndex, CacheSelPos;
	long CacheSelClearIndex, CacheSelClearPos;

private:
	virtual void SetSelectedFirstMode(int Mode);
	virtual int GetSelectedFirstMode() { return SelectedFirst; }
	virtual void DisplayObject();
	void Up(int Count);
	void Down(int Count);
	void Scroll(int Count);
	void CorrectPosition();
	void ShowFileList(int Fast);
	void ShowList(int ShowStatus, int StartColumn);
	void SetShowColor(int Position, int ColorType = HIGHLIGHTCOLORTYPE_FILE);
	DWORD64 GetShowColor(int Position, int ColorType);
	void ShowSelectedSize();
	void ShowTotalSize(OpenPluginInfo &Info);
	bool ResolveSymlink(FARString &target_path, const wchar_t *link_name, FileListItem *fi);
	int ConvertName(FARString &strDest, const wchar_t *SrcName,
		int MaxLength, int RightAlign, int ShowStatus, DWORD dwFileAttr, FileListItem *fi);

	void Select(FileListItem *SelPtr, bool Selection);
	long SelectFiles(int Mode, const wchar_t *Mask = nullptr);
	void ProcessEnter(bool EnableExec, bool SeparateWindow, bool EnableAssoc = true, bool RunAs = false,
			OPENFILEPLUGINTYPE Type = OFP_NORMAL);
	bool ProcessEnter_ChangeDir(const wchar_t *dir, const wchar_t *select_file = nullptr);
	// ChangeDir возвращает FALSE, eсли не смогла выставить заданный путь
	BOOL ChangeDir(const wchar_t *NewDir, BOOL IsUpdated = TRUE);
	void CountDirSize(DWORD PluginFlags);
	/*
		$ 19.03.2002 DJ
		IgnoreVisible - обновить, даже если панель невидима
	*/
	void ReadFileNames(int KeepSelection, int IgnoreVisible, int DrawMessage, int CanBeAnnoying);
	void UpdatePlugin(int KeepSelection, int IgnoreVisible);

	void MoveSelection(ListDataVec &NewList, ListDataVec &OldList);
	virtual int GetSelCount();
	virtual int GetSelName(FARString *strName, DWORD &FileAttr, DWORD &FileMode, FAR_FIND_DATA_EX *fde = nullptr);
	virtual void UngetSelName();
	virtual void ClearLastGetSelection();

	virtual uint64_t GetLastSelectedSize();

	virtual int GetCurName(FARString &strName);
	virtual int GetCurBaseName(FARString &strName);

	void PushPlugin(HANDLE hPlugin, const wchar_t *HostFile);
	int PopPlugin(int EnableRestoreViewMode);
	void CopyFiles();
	void CopyNames(bool FullPathName, bool UNC);
	void SelectSortMode();
	bool ApplyCommand();
	void DescribeFiles();
	void CreatePluginItemList(PluginPanelItemVec &ItemList);
	HANDLE OpenPluginForFile(const wchar_t *FileName, DWORD FileAttr, OPENFILEPLUGINTYPE Type);
	int PreparePanelView(PanelViewSettings *PanelView);
	int PrepareColumnWidths(unsigned int *ColumnTypes, int *ColumnWidths, int *ColumnWidthsTypes,
			int &ColumnCount, int FullScreen);
	void PrepareViewSettings(int ViewMode, OpenPluginInfo *PlugInfo);

	void PluginDelete();
	void PutDizToPlugin(FileList *DestPanel, PluginPanelItem *ItemList, int ItemNumber, int Delete, int Move,
			DizList *SrcDiz, DizList *DestDiz);
	void PluginGetFiles(const wchar_t **DestPath, int Move);
	void PluginToPluginFiles(int Move);
	void PluginHostGetFiles();
	void PluginPutFilesToNew();
	// возвращает то, что возвращает PutFiles
	int PluginPutFilesToAnother(int Move, Panel *AnotherPanel);
	void ProcessPluginCommand();
	void PluginClearSelection(PluginPanelItem *ItemList, int ItemNumber);
	void ProcessCopyKeys(FarKey Key);
	void ReadSortGroups(bool UpdateFilterCurrentTime = true);
	int ProcessOneHostFile(int Idx);
	bool TrySymlinkTraverse();
	void RevertSymlinkTraverse();

protected:
	virtual void ClearAllItem();

public:
	FileList();
	virtual ~FileList();

public:
	virtual int ProcessKey(FarKey Key);
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual int64_t VMProcess(MacroOpcode OpCode, void *vParam = nullptr, int64_t iParam = 0);
	virtual void MoveToMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual void SetFocus();
	virtual void Update(int Mode);
	/*
		$ 22.06.2001 SKV
		Параметр для игнорирования времени последнего Update.
		Используется для Update после исполнения команды.
	*/
	virtual int UpdateIfChanged(int UpdateMode);

	/*
		$ 19.03.2002 DJ
		UpdateIfRequired() - обновить, если апдейт был пропущен из-за того,
		что панель невидима
	*/
	virtual void UpdateIfRequired();

	virtual int SendKeyToPlugin(DWORD Key, BOOL Pred = FALSE);
	virtual void CreateChangeNotification(int CheckTree);
	virtual void CloseChangeNotification();
	virtual void SortFileList(int KeepPosition);
	virtual void SetViewMode(int ViewMode);
	virtual void SetSortMode(int SortMode);
	void SetSortMode0(int SortMode);
	virtual void ChangeSortOrder(int NewOrder);
	virtual void ChangeNumericSort(int Mode);
	virtual void ChangeCaseSensitiveSort(int Mode);
	virtual void ChangeDirectoriesFirst(int Mode);
	virtual BOOL SetCurDir(const wchar_t *NewDir, int ClosePlugin);
	virtual int GetPrevSortMode();
	virtual int GetPrevSortOrder();
	virtual int GetPrevViewMode();
	virtual int GetPrevNumericSort();
	virtual int GetPrevCaseSensitiveSort();
	virtual int GetPrevDirectoriesFirst();

	HANDLE OpenFilePlugin(const wchar_t *FileName, int PushPrev, OPENFILEPLUGINTYPE Type);
	virtual int GetFileName(FARString &strName, int Pos, DWORD &FileAttr);
	virtual int GetCurrentPos();
	virtual bool FindPartName(const wchar_t *Name, int Next, int Direct = 1, int ExcludeSets = 0);
	long FindFile(const char *Name, BOOL OnlyPartName = FALSE);

	virtual bool GoToFile(long idxItem);
	virtual bool GoToFile(const wchar_t *Name, BOOL OnlyPartName = FALSE);
	virtual long FindFile(const wchar_t *Name, BOOL OnlyPartName = FALSE);

	virtual bool IsSelected(const wchar_t *Name);
	virtual bool IsSelected(long idxItem);

	virtual long FindFirst(const wchar_t *Name);
	virtual long FindNext(int StartPos, const wchar_t *Name);

	void ProcessHostFile();
	virtual void UpdateViewPanel();
	virtual void CompareDir();
	virtual void ClearSelection();
	virtual void SaveSelection();
	virtual void RestoreSelection();
	virtual void EditFilter();
	virtual bool FileInFilter(long idxItem);
	virtual void ReadDiz(PluginPanelItem *ItemList = nullptr, int ItemLength = 0, DWORD dwFlags = 0);
	virtual void DeleteDiz(const wchar_t *Name);
	virtual void FlushDiz();
	virtual void GetDizName(FARString &strDizName);
	virtual void CopyDiz(const wchar_t *Name, const wchar_t *DestName, DizList *DestDiz);
	virtual int IsFullScreen();
	virtual int IsDizDisplayed();
	virtual int IsColumnDisplayed(int Type);
	virtual int GetColumnsCount() { return Columns; }
	virtual void SetReturnCurrentFile(int Mode);
	virtual void GetOpenPluginInfo(OpenPluginInfo *Info);
	virtual void SetPluginMode(HANDLE hPlugin, const wchar_t *PluginFile, bool SendOnFocus = false);

	void PluginGetPanelInfo(PanelInfo &Info);
	size_t PluginGetPanelItem(int ItemNumber, PluginPanelItem *Item);
	size_t PluginGetSelectedPanelItem(int ItemNumber, PluginPanelItem *Item);
	void PluginGetColumnTypesAndWidths(FARString &strColumnTypes, FARString &strColumnWidths);

	void PluginBeginSelection();
	void PluginSetSelection(int ItemNumber, bool Selection);
	void PluginClearSelection(int SelectedItemNumber);
	void PluginEndSelection();

	virtual void SetPluginModified();
	virtual int ProcessPluginEvent(int Event, void *Param);
	virtual void SetTitle();
	// virtual FARString &GetTitle(FARString &Title,int SubLen=-1,int TruncSize=0);
	int PluginPanelHelp(HANDLE hPlugin);
	virtual long GetFileCount() { return ListData.Count(); }

	FARString &CreateFullPathName(const wchar_t *Name, DWORD FileAttr, FARString &strDest, int UNC);

	virtual const void *GetItem(int Index);
	virtual BOOL UpdateKeyBar();

	virtual void IfGoHome(wchar_t Drive);

	void ResetLastUpdateTime() { LastUpdateTime = 0; }
	virtual HANDLE GetPluginHandle();
	virtual int GetRealSelCount();
	static void SetFilePanelModes();
	static void SavePanelModes(ConfigWriter &cfg_writer);
	static void ReadPanelModes(ConfigReader &cfg_reader);
	static int FileNameToPluginItem(const wchar_t *Name, PluginPanelItem *pi);
	static void FileListToPluginItem(FileListItem *fi, PluginPanelItem *pi);
	static void FreePluginPanelItem(PluginPanelItem *pi);
	size_t FileListToPluginItem2(FileListItem *fi, PluginPanelItem *pi);
	static void PluginToFileListItem(PluginPanelItem *pi, FileListItem *fi);
	static int IsModeFullScreen(int Mode);
};
