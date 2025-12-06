#pragma once

/*
editor.hpp

Редактор
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
#include <farplug-wide.h>
#include "poscache.hpp"
#include "bitflags.hpp"
#include "config.hpp"
#include "DList.hpp"
#include "noncopyable.hpp"
#include "FARString.hpp"

class FileEditor;
class KeyBar;

// Internal structure to hold standard bookmark data (Ctrl-0..9) for the editor
struct InternalEditorBookMark
{
	DWORD64 Line[POSCACHE_BOOKMARK_COUNT];       // Line numbers for bookmarks
	DWORD64 Cursor[POSCACHE_BOOKMARK_COUNT];     // Cursor positions (X) for bookmarks
	DWORD64 ScreenLine[POSCACHE_BOOKMARK_COUNT]; // Visual screen line offsets
	DWORD64 LeftPos[POSCACHE_BOOKMARK_COUNT];    // Horizontal scroll offsets
};

// Structure representing a node in the bookmark stack (used for push/pop positions)
struct InternalEditorStackBookMark
{
	DWORD Line;       // Line number
	DWORD Cursor;     // Cursor position
	DWORD ScreenLine; // Screen line offset
	DWORD LeftPos;    // Horizontal scroll offset
	InternalEditorStackBookMark *prev, *next; // Linked list pointers
};

// Parameters used to save/restore the editor state (cache)
struct EditorCacheParams
{
	int Line;           // Current line number
	int LinePos;        // Current cursor position within the line
	int ScreenLine;     // Current line relative to the top of the screen
	int LeftPos;        // Horizontal scroll offset
	int CodePage;       // Current code page
	int TabSize;        // Tabulator width
	int ExpandTabs;     // Flag: 0 = don't expand, 1 = expand all, 2 = expand new

	InternalEditorBookMark SavePos; // Saved bookmarks
};

// Represents a single action in the Undo/Redo buffer
struct EditorUndoData
{
	int Type {0};           // Type of undo action (UNDO_EDIT, UNDO_INSSTR, etc.)
	int StrPos {0};         // Position within the string where action occurred
	int StrNum {0};         // Line number affected
	wchar_t EOL[10]{0};     // End-of-line characters for this record
	int Length {0};         // Length of the string data
	wchar_t *Str {nullptr}; // Content of the string involved (inserted/deleted text)

	EditorUndoData() = default;
	~EditorUndoData()
	{
	    delete[] Str;
	}
	EditorUndoData(const EditorUndoData& src) : EditorUndoData()
	{
		operator=(src);
	}
	EditorUndoData& operator=(const EditorUndoData &src)
	{
		if (this != &src)
		{
			SetData(src.Type, src.Str, src.EOL, src.StrNum, src.StrPos, src.Length);
		}
		return *this;
	}
	
	// Helper to set the data for the undo record, allocating memory for the string
	void SetData(int Type, const wchar_t *Str, const wchar_t *Eol, int StrNum, int StrPos, int Length = -1)
	{
		if (Length == -1 && Str)
			Length = (int)StrLength(Str);

		this->Type = Type;
		this->StrPos = StrPos;
		this->StrNum = StrNum;
		this->Length = Length;
		far_wcsncpy(EOL, Eol ? Eol : L"", ARRAYSIZE(EOL) - 1);

	    delete[] this->Str;

		if (Str) {
			this->Str = new wchar_t[Length + 1];

			if (this->Str)
				wmemmove(this->Str, Str, Length);
		} else
			this->Str = nullptr;
	}
};

// Editor state flags
// Lower byte (0xFF mask) is used by ScreenObject base class
enum FLAGS_CLASS_EDITOR
{
	FEDITOR_MODIFIED     = 0x00000200, // The text has been modified
	FEDITOR_JUSTMODIFIED = 0x00000400, // Modified flag for redraw optimization (sends EE_REDRAW)
	// set to 1 by TextChanged, no matter what
	// is value of State.
	
	FEDITOR_MARKINGBLOCK          = 0x00000800, // Currently selecting a stream block
	FEDITOR_MARKINGVBLOCK         = 0x00001000, // Currently selecting a vertical block
	FEDITOR_WASCHANGED            = 0x00002000, // File was changed at some point (even if undone)
	FEDITOR_OVERTYPE              = 0x00004000, // Overtype mode is active (vs Insert)
	FEDITOR_NEWUNDO               = 0x00010000, // Start a new undo transaction group
	FEDITOR_UNDOSAVEPOSLOST       = 0x00020000, // The saved position for undo is invalid
	FEDITOR_DISABLEUNDO           = 0x00040000, // Undo system is currently disabled (e.g., during undo)
	FEDITOR_LOCKMODE              = 0x00080000, // Editor is locked (Read Only or similar restriction)
	FEDITOR_CURPOSCHANGEDBYPLUGIN = 0x00100000, // TRUE, если позиция в редакторе была изменена
	// плагином (ECTL_SETPOSITION)
	
	FEDITOR_ISRESIZEDCONSOLE = 0x00800000, // Console resize event occurred
	FEDITOR_PROCESSCTRLQ     = 0x02000000, // Processing Ctrl-Q (literal character input)
	FEDITOR_DIALOGMEMOEDIT   = 0x80000000, // Editor is running inside a dialog (DI_MEMOEDIT)
};

class Edit;

// Main Editor Class
class Editor : public ScreenObject
{
	friend class DlgEdit;
	friend class FileEditor;

private:
	/*
		$ 04.11.2003 SKV
		на любом выходе если была нажата кнопка выделения,
		и она его "сняла" (сделала 0-й ширины), то его надо убрать.
	*/
	// RAII helper to handle block unmarking safety logic.
	// Used in ProcessKey to ensure empty blocks are unmarked if necessary.
	class EditorBlockGuard : public NonCopyable
	{
		Editor &ed;
		void (Editor::*method)();
		bool needCheckUnmark;

	public:
		void SetNeedCheckUnmark(bool State) { needCheckUnmark = State; }
		EditorBlockGuard(Editor &ed, void (Editor::*method)())
			:
			ed(ed), method(method), needCheckUnmark(false)
		{}
		~EditorBlockGuard()
		{
			if (needCheckUnmark)
				(ed.*method)();
		}
	};

	DList<EditorUndoData> UndoData; // Double linked list of undo operations
	EditorUndoData *UndoPos;        // Current position in the undo list
	EditorUndoData *UndoSavePos;    // Saved position in undo list (matches "saved" state on disk)
	int UndoSkipLevel;              // Nesting level for undo grouping (UNDO_BEGIN/UNDO_END)

	int LastChangeStrPos; // Position of the last character change (for grouping undo edits)
	int NumLastLine;      // Total number of lines in the editor
	int NumLine;          // Current line number (0-based)
	
	/*
		$ 26.02.2001 IS
		Сюда запомним размер табуляции и в дальнейшем будем использовать его,
		а не Opt.TabSize
	*/
	EditorOptions EdOpt;  // Editor configuration options

	int Pasting;             // Counter/Flag indicating a paste operation is in progress
	wchar_t GlobalEOL[10];   // The global End-Of-Line string used for the file

	// Macro block selection variables (MCODE_F_EDITOR_SEL)
	Edit *MBlockStart; 
	int MBlockStartX;

	// Standard block selection variables
	Edit *BlockStart;   // Start line of the stream block
	int BlockStartLine; // Line number of block start
	Edit *VBlockStart;  // Start line of the vertical block

	// Vertical block coordinates
	int VBlockX;
	int VBlockSizeX;
	int VBlockY;
	int VBlockSizeY;

	int MaxRightPos; // Remembers the maximum horizontal position when moving up/down

	int XX2;	// Adjusted Right X coordinate (considering scrollbar)

	FARString strLastSearchStr; // Last search pattern
	/*
		$ 30.07.2000 KM
		Новая переменная для поиска "Whole words"
	*/
	// Last search parameters
	int LastSearchCase, LastSearchWholeWords, LastSearchReverse, LastSearchSelFound, LastSearchRegexp;
	
	int m_WordWrapMaxRightPos; // Max right position specific for word wrap mode

	UINT m_codepage;	// Current code page of the text

	int StartLine; // Initial start line (set by viewer or external call)
	int StartChar; // Initial start char

	InternalEditorBookMark SavePos; // Saved standard bookmarks (Ctrl-0..9)

	InternalEditorStackBookMark *StackPos; // Pointer to the top of the bookmark stack
	BOOL NewStackPos;                      // Flag indicating if a new stack position needs creating

	int EditorID; // Unique ID for this editor instance

	FileEditor *HostFileEditor; // Pointer to the hosting FileEditor (if applicable)
	Edit *TopList;   // Pointer to the first line (Edit object) in the list
	Edit *EndList;   // Pointer to the last line in the list
	Edit *TopScreen; // Pointer to the line currently at the top of the screen (non-wrap mode)
	
	// Word Wrap specific state variables
	int m_CurVisualLineInLogicalLine; // Current visual line index within the logical line
	Edit *m_TopScreenLogicalLine;     // Logical line at the top of the screen (wrap mode)
	int m_TopScreenVisualLine;        // Visual line offset within m_TopScreenLogicalLine
	
	Edit *CurLine; // Pointer to the current active line
	
	// Cached line pointer to speed up line lookups
	Edit *LastGetLine;
	int LastGetLineNumber;

	// Mouse selection state
	int MouseSelStartingLine{-1}, MouseSelStartingPos{-1};
	
	bool SaveTabSettings; // Whether tab settings need to be saved to cache
	bool m_bWordWrap;     // Current word wrap state
	int m_WrapMaxVisibleLineLength; // Max visible length calculated for word wrap (for plugins)
	bool m_MouseButtonIsHeld;       // Track mouse button state
	
	// Line number caching for performance
	int m_CachedTotalLines;
	int m_CachedLineNumWidth;
	bool m_LineCountDirty;

private:
	// Maps a logical line and character position to a visual line index in word wrap mode
	int FindVisualLine(Edit* line, int Pos);
	
	// Returns the total number of visual lines (considering wrapping)
	int GetTotalVisualLines();
	
	// Returns the absolute index of the top visual line
	int GetTopVisualLine();
	
	// Calculates how many visual lines exist below a specific point
	int GetVisualLinesBelow(Edit* startLine, int startVisual);
	
	// Renders the editor content
	virtual void DisplayObject();
	
	// Updates cursor position, clamping it within visual bounds
	void UpdateCursorPosition(int horizontal_cell_pos);
	
	// Main routine to draw the editor interface
	// CurLineOnly: Optimization to redraw only the current line
	void ShowEditor(int CurLineOnly);
	
	// Deletes a specific string/line
	void DeleteString(Edit *DelPtr, int LineNumber, int DeleteLast, int UndoLine);
	
	// Inserts a new line at current position (handling enter/split, auto-indent)
	void InsertString();
	
	// Moves cursor up one line (visual or logical)
	void Up();
	
	// Moves cursor down one line (visual or logical)
	void Down();
	
	// Scrolls the view down without moving cursor (if possible)
	void ScrollDown();
	
	// Scrolls the view up without moving cursor (if possible)
	void ScrollUp();
	
	// Performs search and replace. Next=1 implies "Find Next"
	BOOL Search(int Next);

	// Moves view to a specific visual line offset within a logical line
	void GoToVisualLine(int VisualLine);
	
	// Moves cursor to a specific logical line number
	void GoToLine(int Line);
	
	// Shows the "Go To Position" dialog
	void GoToPosition();

	// Updates the modified state and flags
	void TextChanged(int State);

	// Calculates the distance (number of lines) between two Edit objects
	int CalcDistance(Edit *From, Edit *To, int MaxDist);
	
	// Pastes text from clipboard or argument
	void Paste(const wchar_t *Src = nullptr);
	
	// Copies selected text to clipboard
	void Copy(int Append);
	
	// Deletes the currently selected block
	void DeleteBlock();
	
	// Marks a block (stream or vertical) based on coordinates
	bool MarkBlock(bool SelVBlock, int SelStartLine, int SelStartPos, int SelWidth, int SelHeight);
	
	// Unmarks selection. Returns true if something was unmarked.
	bool UnmarkBlock();
	
	// Unmarks block and triggers a redraw
	void UnmarkBlockAndShowIt();
	
	// Unmarks block if it has zero size
	void UnmarkEmptyBlock();
	
	// Clears macro-specific block markers
	void UnmarkMacroBlock();

	// Handles paste event logic (locking, undo grouping)
	void ProcessPasteEvent();

	// Adds an entry to the undo buffer
	void AddUndoData(int Type, const wchar_t *Str = nullptr, const wchar_t *Eol = nullptr, int StrNum = 0,
			int StrPos = 0, int Length = -1);
	
	// Adjusts screen position to ensure cursor is visible (centering if needed)
	void AdjustScreenPosition();
	
	// Performs Undo (redo=0) or Redo (redo=1)
	void Undo(int redo);
	
	// Selects all text
	void SelectAll();
	
	// Visual helper to highlight wrapped lines (shows logic continuation on right margin)
	void HighlightAsWrapped(int Y, Edit &ShowString); 
	
	// Calculates total line count (helper for line numbers)
	int CalculateTotalLines();
	
	// Calculates the width required to display line numbers
	int CalculateLineNumberWidth();
	
	// Shifts the selected block indentation left
	void BlockLeft();
	
	// Shifts the selected block indentation right
	void BlockRight();
	
	// Deletes a vertical block
	void DeleteVBlock();
	
	// Copies a vertical block
	void VCopy(int Append);
	
	// Pastes a vertical block
	void VPaste(wchar_t *ClipText);
	
	// Shifts a vertical block content horizontally
	void VBlockShift(int Left);
	
	// Retrieves an Edit object pointer by line number, using cache
	Edit *GetStringByNumber(int DestLine);
	
	// Displays a message during long operations (like search)
	static void EditorShowMsg(const wchar_t *Title, const wchar_t *Msg, const wchar_t *Name, int Percent);

	// Bookmarking functions
	int SetBookmark(DWORD Pos);
	int GotoBookmark(DWORD Pos);

	// Stack Bookmark functions
	// Clears the entire bookmark stack
	int ClearStackBookmarks();
	// Deletes a specific bookmark node from the stack
	int DeleteStackBookmark(InternalEditorStackBookMark *sb_delete);
	// Restores the editor position from the current stack bookmark
	int RestoreStackBookmark();
	// Pushes the current position onto the bookmark stack
	int AddStackBookmark(BOOL blNewPos = TRUE);
	// Helper to get the first bookmark in the list
	InternalEditorStackBookMark *PointerToFirstStackBookmark(int *piCount = nullptr);
	// Helper to get the last bookmark in the list
	InternalEditorStackBookMark *PointerToLastStackBookmark(int *piCount = nullptr);
	// Retrieves a bookmark pointer by index
	InternalEditorStackBookMark *PointerToStackBookmark(int iIdx);
	// Navigates back in the stack history (smart behavior with saving current pos)
	int BackStackBookmark();
	// Moves to the previous bookmark in the stack
	int PrevStackBookmark();
	// Moves to the next bookmark in the stack
	int NextStackBookmark();
	// Moves to the last bookmark
	int LastStackBookmark();
	// Jumps to a specific stack bookmark by index
	int GotoStackBookmark(int iIdx);
	// Explicitly pushes a bookmark (API/Macro)
	int PushStackBookMark();
	// Restores and removes the top bookmark (API/Macro)
	int PopStackBookMark();
	// Gets the index of the current bookmark
	int CurrentStackBookmarkIdx();
	// Retrieves data of a specific bookmark
	int GetStackBookmark(int iIdx, EditorBookMarks *Param);
	// Retrieves all bookmarks
	int GetStackBookmarks(EditorBookMarks *Param);

	// Helper to convert block start pointer to line number
	int BlockStart2NumLine(int *Pos);
	// Helper to convert block end pointer to line number
	int BlockEnd2NumLine(int *Pos);
	
	// Checks if an Edit pointer belongs to this editor
	bool CheckLine(Edit *line);
	
	// Extracts text from a stream block into a buffer
	wchar_t *Block2Text(wchar_t *ptrInitData);
	
	// Extracts text from a vertical block into a buffer
	wchar_t *VBlock2Text(wchar_t *ptrInitData);

public:
	// Constructor: Initialize editor state, load options
	Editor(ScreenObject *pOwner = nullptr, bool DialogUsed = false);
	// Destructor: clean up memory
	virtual ~Editor();

public:
	// Enable saving tab settings to cache
	void EnableSaveTabSettings();
	
	// Restores editor state (position, etc) from cache
	void SetCacheParams(EditorCacheParams *pp);
	
	// Saves editor state to cache structure
	void GetCacheParams(EditorCacheParams *pp);

	// Sets the current code page
	bool SetCodePage(UINT codepage);
	
	// Returns current code page
	UINT GetCodePage();

	// Loads raw string data into the editor (initialization)
	int SetRawData(const wchar_t *SrcBuf, int SizeSrcBuf, int TextFormat);
	
	// Exports editor content to a buffer
	int GetRawData(wchar_t **DestBuf, int &SizeDestBuf, int TextFormat = 0);

	// Handles keyboard input events
	virtual int ProcessKey(FarKey Key);
	
	// Handles mouse input events
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	
	// Handles macro execution queries
	virtual int64_t VMProcess(MacroOpcode OpCode, void *vParam = nullptr, int64_t iParam = 0);

	// Persists current search parameters to global configuration
	void KeepInitParameters();
	
	// Sets the initial start position (Line and Char)
	void SetStartPos(int LineNum, int CharNum);
	
	// Returns true if modified flag is set
	BOOL IsFileModified() const;
	
	// Returns true if modified or was modified flag is set
	BOOL IsFileChanged() const;
	
	// Sets the editor title (not typically used in base class directly)
	void SetTitle(const wchar_t *Title);
	
	// Returns the current cursor position (offset in bytes/chars from start)
	long GetCurPos();
	
	// Plugin API entry point: handles ECTL_* commands
	int EditorControl(int Command, void *Param);
	
	// Links this Editor instance to a host FileEditor
	void SetHostFileEditor(FileEditor *Editor) { HostFileEditor = Editor; };
	
	// Sets the global replace mode state
	static void SetReplaceMode(int Mode);
	
	// Gets the host FileEditor
	FileEditor *GetHostFileEditor() { return HostFileEditor; };
	
	// Flags that the console has been resized
	void PrepareResizedConsole() { Flags.Set(FEDITOR_ISRESIZEDCONSOLE); }

	// Tab size settings
	void SetTabSize(int NewSize);
	int GetTabSize() const { return EdOpt.TabSize; }

	// Word wrap settings
	void SetWordWrap(int NewMode);
	int GetWordWrap() const { return m_bWordWrap; }
	
	// Tab conversion settings
	void SetConvertTabs(int NewMode);
	int GetConvertTabs() const { return EdOpt.ExpandTabs; }

	// Delete behavior settings
	void SetDelRemovesBlocks(int NewMode);
	int GetDelRemovesBlocks() const { return EdOpt.DelRemovesBlocks; }

	// Persistent block selection settings
	void SetPersistentBlocks(int NewMode);
	int GetPersistentBlocks() const { return EdOpt.PersistentBlocks; }

	// Auto indent settings
	void SetAutoIndent(int NewMode) { EdOpt.AutoIndent = NewMode; }
	int GetAutoIndent() const { return EdOpt.AutoIndent; }

	// Codepage detection settings
	void SetAutoDetectCodePage(int NewMode) { EdOpt.AutoDetectCodePage = NewMode; }
	int GetAutoDetectCodePage() const { return EdOpt.AutoDetectCodePage; }

	// Cursor behavior beyond EOL
	void SetCursorBeyondEOL(int NewMode);
	int GetCursorBeyondEOL() const { return EdOpt.CursorBeyondEOL; }

	// Backspace behavior
	void SetBSLikeDel(int NewMode) { EdOpt.BSLikeDel = NewMode; }
	int GetBSLikeDel() const { return EdOpt.BSLikeDel; }

	// Char code base display setting
	void SetCharCodeBase(int NewMode) { EdOpt.CharCodeBase = NewMode % 3; }
	int GetCharCodeBase() const { return EdOpt.CharCodeBase; }

	// Read only lock setting
	void SetReadOnlyLock(int NewMode) { EdOpt.ReadOnlyLock = NewMode & 3; }
	int GetReadOnlyLock() const { return EdOpt.ReadOnlyLock; }

	// Scrollbar visibility
	void SetShowScrollBar(int NewMode) { EdOpt.ShowScrollBar = NewMode; }

	// Search pick up word setting
	void SetSearchPickUpWord(int NewMode) { EdOpt.SearchPickUpWord = NewMode; }

	// Word divider characters
	void SetWordDiv(const wchar_t *WordDiv) { EdOpt.strWordDiv = WordDiv; }
	const wchar_t *GetWordDiv() { return EdOpt.strWordDiv; }

	// White space visibility
	int GetShowWhiteSpace() const { return EdOpt.ShowWhiteSpace; }
	void SetShowWhiteSpace(int NewMode);

	// Line number visibility
	int GetShowLineNumbers() const { return EdOpt.ShowLineNumbers; }
	void SetShowLineNumbers(int NewMode);

	// Get save position mode configuration
	void GetSavePosMode(int &SavePos, int &SaveShortPos);

	// Set save position mode configuration (-1 to ignore parameter)
	void SetSavePosMode(int SavePos, int SaveShortPos);

	// Parses "row,col" string to integers
	void GetRowCol(const wchar_t *argv, int *row, int *col);

	// Starts marking a vertical block
	void BeginVBlockMarking();
	
	// Adjusts vertical block selection based on cursor movement
	void AdjustVBlock(int PrevX);

	// Performs transliteration/transformation (Xlat) on selected text or word
	void Xlat();
	
	// Static callback for displaying messages via PreRedraw
	static void PR_EditorShowMsg();

	// Frees all allocated memory (lines, undo stack)
	void FreeAllocatedData(bool FreeUndo = true);

	// Creates a new Edit object (line)
	Edit *CreateString(const wchar_t *lpwszStr, int nLength);
	
	// Inserts a new string/line into the linked list (factory + insertion)
	Edit *InsertString(const wchar_t *lpwszStr, int nLength, Edit *pAfter = nullptr, int AfterLineNumber = -1);

	// Sets the parent dialog window
	void SetDialogParent(DWORD Sets);
	
	// Controls ReadOnly state (LockMode)
	void SetReadOnly(int NewReadOnly) { Flags.Change(FEDITOR_LOCKMODE, NewReadOnly); };
	int GetReadOnly() { return Flags.Check(FEDITOR_LOCKMODE); };
	
	// Overtype mode stubs (not fully implemented in Editor class itself but delegated)
	void SetOvertypeMode(int Mode);
	int GetOvertypeMode();
	
	// Edit beyond end mode setter
	void SetEditBeyondEnd(int Mode);
	
	// Clear flag stubs
	void SetClearFlag(int Flag);
	int GetClearFlag();

	// Returns current column number
	int GetCurCol();
	
	// Returns current row number
	int GetCurRow() { return NumLine; }
	
	// Sets cursor position
	void SetCurPos(int NewCol, int NewRow = -1);
	
	// Configures cursor appearance
	void SetCursorType(bool Visible, DWORD Size);
	void GetCursorType(bool &Visible, DWORD &Size);
	
	// Sets color attributes for objects
	void SetObjectColor(uint64_t Color, uint64_t SelColor, uint64_t ColorUnChanged);
	
	// Draws the scrollbar
	void DrawScrollbar();

	// Sets the position of the editor on screen
	virtual void SetPosition(int X1, int Y1, int X2, int Y2);
};

// Macros for packing/unpacking editor parameters in PosCache
#define POSCACHE_EDIT_PARAM4_PACK(VALUE, CP, EXPAND_TABS, TAB_SIZE)                                            \
	do {                                                                                                       \
		VALUE = ((DWORD(CP) & 0xffff) | ((DWORD(EXPAND_TABS) & 0xff) << 16)                                    \
				| ((DWORD(TAB_SIZE) & 0xff) << 24));                                                           \
	} while (0)

#define POSCACHE_EDIT_PARAM4_UNPACK(VALUE, CP, EXPAND_TABS, TAB_SIZE)                                          \
	do {                                                                                                       \
		CP = (((VALUE)&0xffff) == 0xffff) ? -1 : ((VALUE)&0xffff);                                             \
		EXPAND_TABS = ((VALUE) == 0xffffffff) ? 0 : ((VALUE) >> 16) & 0xff;                                    \
		TAB_SIZE = ((VALUE) == 0xffffffff) ? 0 : ((VALUE) >> 24) & 0xff;                                       \
	} while (0)
