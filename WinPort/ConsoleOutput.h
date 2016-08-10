#pragma once
#include <mutex>
#include <vector>
#include "WinCompat.h"
#include "ConsoleBuffer.h"

class ConsoleOutputListener
{
	public:
		virtual void OnConsoleOutputUpdated(const SMALL_RECT &area) = 0;
		virtual void OnConsoleOutputResized() = 0;
		virtual void OnConsoleOutputTitleChanged() = 0;
		virtual void OnConsoleOutputWindowMoved(bool absolute, COORD pos) = 0;
};

class ConsoleOutput
{
	ConsoleBuffer _buf;
	USHORT _attributes;
	std::mutex _mutex;
	struct {
		COORD pos;
		UCHAR height;
		bool visible;
	} _cursor;
	COORD _largest_window_size;
	std::wstring _title;
	DWORD _mode;
	
	ConsoleOutputListener *_listener;

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
	size_t ModifySequenceAt(SequenceModifier &sm, COORD &pos);
	void ScrollOutputOnOverflow();

public:
	ConsoleOutput();
	void SetListener(ConsoleOutputListener *listener);

	void SetAttributes(USHORT attributes);
	USHORT GetAttributes();
	void SetCursor(const COORD &pos);
	void SetCursor(UCHAR height, bool visible);
	COORD GetCursor();
	COORD GetCursor(UCHAR &height, bool &visible);

	void SetSize(unsigned int width, unsigned int height);
	void GetSize(unsigned int &width, unsigned int &height);

	void SetLargestConsoleWindowSize(COORD size);
	COORD GetLargestConsoleWindowSize();

	void SetWindowInfo(bool absolute, const SMALL_RECT &rect);
	void SetTitle(const WCHAR *title);
	std::wstring GetTitle();

	DWORD GetMode();
	void SetMode(DWORD mode);

	void Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	void Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	bool Write(const CHAR_INFO &data, COORD screen_pos);

	size_t WriteString(const WCHAR *data, size_t count);
	size_t WriteStringAt(const WCHAR *data, size_t count, COORD &pos);
	size_t FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos);
	size_t FillAttributeAt(WORD wAttribute, size_t count, COORD &pos);
	
};


