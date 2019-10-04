#pragma once
#include <mutex>
#include <vector>
#include <string>
#include "WinCompat.h"
#include "ConsoleBuffer.h"
#include "Backend.h"

class ConsoleOutput
{
	std::mutex _mutex;
	ConsoleBuffer _buf;
	std::vector<CHAR_INFO> _temp_chars;
	std::wstring _title;
	IConsoleOutputBackend *_backend;
	DWORD _mode;	
	USHORT _attributes;
	
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
			WORD attr;
		};
	};
	
	bool ModifySequenceEntityAt(SequenceModifier &sm, COORD pos);
	size_t ModifySequenceAt(SequenceModifier &sm, COORD &pos);
	void ScrollOutputOnOverflow(SMALL_RECT &area);
	
public:
	ConsoleOutput();
	void SetBackend(IConsoleOutputBackend *listener);

	void SetAttributes(USHORT attributes);
	USHORT GetAttributes();
	void SetCursor(COORD pos);
	void SetCursor(UCHAR height, bool visible);
	COORD GetCursor();
	COORD GetCursor(UCHAR &height, bool &visible);

	void SetSize(unsigned int width, unsigned int height);
	void GetSize(unsigned int &width, unsigned int &height);

	COORD GetLargestConsoleWindowSize();
	void SetWindowMaximized(bool maximized);

	void SetWindowInfo(bool absolute, const SMALL_RECT &rect);
	void SetTitle(const WCHAR *title);
	std::wstring GetTitle();

	DWORD GetMode();
	void SetMode(DWORD mode);

	void Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	void Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	bool Read(CHAR_INFO &data, COORD screen_pos);
	bool Write(const CHAR_INFO &data, COORD screen_pos);

	size_t WriteString(const WCHAR *data, size_t count);
	size_t WriteStringAt(const WCHAR *data, size_t count, COORD &pos);
	size_t FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos);
	size_t FillAttributeAt(WORD wAttribute, size_t count, COORD &pos);
	
	bool Scroll(const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, 
				COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
				
	void SetScrollRegion(SHORT top, SHORT bottom);
	void GetScrollRegion(SHORT &top, SHORT &bottom);
	void SetScrollCallback(PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext);
	
	void AdhocQuickEdit();
	DWORD SetConsoleTweaks(DWORD tweaks);
	void ConsoleChangeFont();
	bool IsActive();
	void ConsoleDisplayNotification(const WCHAR *title, const WCHAR *text);
};


