#pragma once
#include <stdexcept>
#include <vector>
#include <WinCompat.h>
#include <StackSerializer.h>

class TTYOutput
{
	enum { AUTO_FLUSH_THRESHOLD = 0x1000 };

	struct Cursor
	{
		unsigned int y = -1, x = -1;
		bool visible = false;
	} _cursor;

	struct Attributes
	{
		Attributes() = default;
		Attributes(const Attributes &) = default;
		Attributes(WORD attributes);

		bool foreground_intensive = false;
		bool background_intensive = false;
		unsigned char foreground = -1;
		unsigned char background = -1;

		bool operator ==(const Attributes &attr) const;
		bool operator !=(const Attributes &attr) const {return !(operator ==(attr)); }
	} _attr;

	int _out;
	unsigned int _pending_spaces = 0;
	std::vector<char> _rawbuf;
	void WriteReally(const char *str, int len);

	void Space();
	void FinalizeSpaces();
	void Write(const char *str, int len);
	void Format(const char *fmt, ...);
public:
	TTYOutput(int out);
	~TTYOutput();

	void Flush();

	void ChangeCursor(bool visible, bool force = false);
	inline bool ShouldMoveCursor(unsigned int y, unsigned int x) const { return x != _cursor.x || y != _cursor.y; }
	void MoveCursor(unsigned int y, unsigned int x);
	void WriteLine(const CHAR_INFO *ci, unsigned int cnt);
	void ChangeKeypad(bool app);
	void ChangeMouse(bool enable);
	void ChangeTitle(std::string title);

	void SendFar2lInterract(const StackSerializer &stk_ser);
};
