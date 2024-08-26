#pragma once
#include <stdexcept>
#include <vector>
#include <map>
#include <WinCompat.h>
#include <StackSerializer.h>
#include "../WinPortRGB.h"

extern long _iterm2_cmd_ts;
extern bool _iterm2_cmd_state;

struct TTYBasePalette
{
	TTYBasePalette();

	DWORD foreground[BASE_PALETTE_SIZE];
	DWORD background[BASE_PALETTE_SIZE];
};

class TTYOutput
{
	struct Cursor
	{
		unsigned int y = -1, x = -1;
		bool visible = false;
	} _cursor;

	std::vector<char> _rawbuf;
	struct {
		WCHAR wch = 0;
		unsigned int count = 0;
		std::string tmp;
	} _same_chars;

	struct TrueColors {
		void AppendSuffix(std::string &out, DWORD rgb);
		std::map<DWORD, BYTE> _colors256_lookup;
	} _true_colors;

	int _out;
	bool _far2l_tty, _norgb, _kernel_tty, _screen_tty, _wezterm;
	DWORD _nodetect;
	TTYBasePalette _palette;
	bool _prev_attr_valid{false};
	DWORD64 _prev_attr{};
	std::string _tmp_attrs;

	void WriteReally(const char *str, int len);
	void FinalizeSameChars();
	void WriteWChar(WCHAR wch);
	void Write(const char *str, int len);
	void Format(const char *fmt, ...);

	void AppendTrueColorSuffix(std::string &out, DWORD rgb);
	void WriteUpdatedAttributes(DWORD64 new_attr, bool is_space);

public:
	TTYOutput(int out, bool far2l_tty, bool norgb, DWORD nodetect);
	~TTYOutput();

	void Flush();

	void ChangePalette(const TTYBasePalette &palette);
	void ChangeCursorHeight(unsigned int height);
	void ChangeCursor(bool visible, bool force = false);
	int WeightOfHorizontalMoveCursor(unsigned int y, unsigned int x) const;
	void MoveCursorStrict(unsigned int y, unsigned int x);
	void MoveCursorLazy(unsigned int y, unsigned int x);
	void WriteLine(const CHAR_INFO *ci, unsigned int cnt);
	void ChangeKeypad(bool app);
	void ChangeMouse(bool enable);
	void ChangeTitle(std::string title);

	void SendFar2lInteract(const StackSerializer &stk_ser);
	void SendOSC52ClipSet(const std::string &clip_data);

	void CheckiTerm2Hack();
};
