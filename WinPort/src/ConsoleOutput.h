#pragma once
#include <mutex>
#include <vector>
#include <string>
#include <condition_variable>
#include "WinCompat.h"
#include "ConsoleBuffer.h"
#include "Backend.h"

class ConsoleOutput : public IConsoleOutput
{
	HANDLE _con_handle{NULL};
	std::mutex _mutex;
	ConsoleBuffer _buf;
	std::vector<CHAR_INFO> _temp_chars;
	std::wstring _title;
	IConsoleOutputBackend *_backend;
	DWORD _mode;	
	DWORD64 _attributes;
	COORD _prev_pos{-1, -1};
	unsigned short _repaint_defer{0}; // unlikely it will be more than 10..
	struct DeferredRepaints : std::vector<SMALL_RECT>
	{
		void Add(const SMALL_RECT &area);
		void Add(const SMALL_RECT *areas, size_t cnt);
	} _deferred_repaints;
	unsigned int _change_id{1};
	std::condition_variable _change_id_cond;
	
	struct {
		COORD pos;
		UCHAR height;
		bool visible;
	} _cursor;
	
	struct {
		PCONSOLE_SCROLL_CALLBACK pfn;
		PVOID context;
	} _scroll_callback;
	
	struct {
		USHORT top;
		USHORT bottom;
	} _scroll_region;

	struct SequenceModifier
	{
		enum {
			SM_WRITE_STR,
			SM_FILL_CHAR,
			SM_FILL_ATTR
		} kind;
		size_t count;
		union {
			const WCHAR *str;
			TCHAR chr;
			DWORD64 attr;
		};
	};
	
	void LockedChangeIdUpdate();

	SHORT ModifySequenceEntityAt(SequenceModifier &sm, COORD pos, SMALL_RECT &area);
	size_t ModifySequenceAt(SequenceModifier &sm, COORD &pos);
	void ScrollOutputOnOverflow(SMALL_RECT &area);

	virtual const WCHAR *LockedGetTitle();
	virtual CHAR_INFO *LockedDirectLineAccess(size_t line_index, unsigned int &width);
	virtual void Unlock();
	void SetUpdateCellArea(SMALL_RECT &area, COORD pos);
	void CopyFrom(const ConsoleOutput &co);

public:
	ConsoleOutput();
	virtual void SetBackend(IConsoleOutputBackend *listener);

	virtual void SetAttributes(DWORD64 attributes);
	virtual DWORD64 GetAttributes();

	virtual	void SetCursorBlinkTime(DWORD interval);
	virtual void SetCursor(COORD pos);
	virtual void SetCursor(UCHAR height, bool visible);
	virtual COORD GetCursor();
	virtual COORD GetCursor(UCHAR &height, bool &visible);

	virtual void SetSize(unsigned int width, unsigned int height);
	virtual void GetSize(unsigned int &width, unsigned int &height);

	virtual COORD GetLargestConsoleWindowSize();
	virtual void SetWindowMaximized(bool maximized);

	virtual void SetWindowInfo(bool absolute, const SMALL_RECT &rect);
	virtual void SetTitle(const WCHAR *title);

	virtual DWORD GetMode();
	virtual void SetMode(DWORD mode);

	virtual void Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	virtual void Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	virtual bool Read(CHAR_INFO &data, COORD screen_pos);
	virtual bool Write(const CHAR_INFO &data, COORD screen_pos);

	virtual size_t WriteString(const WCHAR *data, size_t count);
	virtual size_t WriteStringAt(const WCHAR *data, size_t count, COORD &pos);
	virtual size_t FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos);
	virtual size_t FillAttributeAt(DWORD64 qAttribute, size_t count, COORD &pos);
	
	virtual bool Scroll(const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, 
				COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
				
	virtual void SetScrollRegion(SHORT top, SHORT bottom);
	virtual void GetScrollRegion(SHORT &top, SHORT &bottom);
	virtual void SetScrollCallback(PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext);
	
	virtual void AdhocQuickEdit();
	virtual DWORD64 SetConsoleTweaks(DWORD64 tweaks);
	virtual void ConsoleChangeFont();
	virtual void ConsoleSaveWindowState();
	virtual bool IsActive();
	virtual void ConsoleDisplayNotification(const WCHAR *title, const WCHAR *text);
	virtual bool ConsoleBackgroundMode(bool TryEnterBackgroundMode);
	virtual bool SetFKeyTitles(const CHAR **titles);
	virtual BYTE GetColorPalette();
	virtual void GetBasePalette(void *p);
	virtual bool SetBasePalette(void *p);
	virtual void OverrideColor(DWORD Index, DWORD *ColorFG, DWORD *ColorBK);
	virtual void RepaintsDeferStart();
	virtual void RepaintsDeferFinish();

	virtual IConsoleOutput *ForkConsoleOutput(HANDLE con_handle);
	virtual void JoinConsoleOutput(IConsoleOutput *con_out);

	virtual unsigned int WaitForChange(unsigned int prev_change_id, unsigned int timeout_msec = -1);
};
