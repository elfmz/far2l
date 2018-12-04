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
	std::vector<char> _rawbuf;
	void WriteReally(const char *str, int len);

	void Write(const char *str, int len);
	void Format(const char *fmt, ...);
public:
	TTYOutput(int out);

	void Flush();
	void SetScreenBuffer(bool alternate);

	void ChangeCursor(bool visible, bool force = false);
	void MoveCursor(unsigned int y, unsigned int x, bool force = false);
	void WriteLine(const CHAR_INFO *ci, unsigned int cnt);
	void ChangeKeypad(bool app);
	void ChangeMouse(bool enable);
	void ChangeTitle(std::string title);

	void SendFar2lInterract(const StackSerializer &stk_ser);
};
