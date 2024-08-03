#pragma once
#include <WinCompat.h>
#include <WinPort.h>
#include <map>
#include <StackSerializer.h>

extern long _iterm2_cmd_ts;
extern bool _iterm2_cmd_state;

struct TTYInputKey
{
	WORD vk;
	DWORD control_keys;
};

template <size_t N>
	struct NChars
{
	char chars[N];

	NChars() = default;
	NChars(const NChars&) = default;

	NChars(const char *src)
	{
		for (size_t i = 0; i < N; ++i) {
			chars[i] = src[i];
		}
	}

	bool operator <(const NChars<N> &other) const
	{
		for (size_t i = 0;; ++i) {
			if (i == N || chars[i] > other.chars[i])
				return false;
			if (chars[i] < other.chars[i])
				return true;
		}
	}
};

template <size_t N, class V>
	struct NCharsMap : std::map<NChars<N>, V>
{
	void Add(const char *s, const V &v)
	{
		NChars<N> nc(s);
		auto ir = std::map<NChars<N>, V>::emplace(nc, v);
		ASSERT_MSG(ir.second, "can't add '%s'", s);
	}

	bool Lookup(const char *s, V &out) const
	{
		NChars<N> nc(s);
		auto it = std::map<NChars<N>, V>::find(nc);
		if (it == std::map<NChars<N>, V>::end())
			return false;

		out = it->second;
		return true;
	}
};

template <size_t N> using NChars2Key = NCharsMap<N, TTYInputKey>;

struct ITTYInputSpecialSequenceHandler
{
	virtual void OnUsingExtension(char extension) = 0;
	virtual void OnInspectKeyEvent(KEY_EVENT_RECORD &event) = 0;
	virtual void OnFar2lEvent(StackSerializer &stk_ser) = 0;
	virtual void OnFar2lReply(StackSerializer &stk_ser) = 0;
	virtual void OnInputBroken() = 0;
};

//wait for more characters from input buffer
#define TTY_PARSED_WANTMORE         ((size_t)0)
//no sequence encountered, plain text recognized
#define TTY_PARSED_PLAINCHARS       ((size_t)-1)
//unrecognized sequence, skip ESC char (0x1b) and continue
#define TTY_PARSED_BADSEQUENCE      ((size_t)-2)

class TTYInputSequenceParser
{
	NChars2Key<1> _nch2key1;
	NChars2Key<2> _nch2key2;
	NChars2Key<3> _nch2key3;
	NChars2Key<4> _nch2key4;
	NChars2Key<5> _nch2key5;
	NChars2Key<6> _nch2key6;
	NChars2Key<7> _nch2key7;
	NChars2Key<8> _nch2key8;


	struct {
		//click indicators
		bool left   = false;
		bool middle = false;
		bool right  = false;
		//double click threshold
		DWORD left_ts   = 0;
		DWORD middle_ts = 0;
		DWORD right_ts  = 0;
		//indicators of pressed state for avoiding "blinking" event while button is held
		bool left_pressed   = false;
		bool middle_pressed = false;
		bool right_pressed  = false;
	} _mouse;

	ITTYInputSpecialSequenceHandler *_handler;
	StackSerializer _tmp_stk_ser;
	DWORD _extra_control_keys = 0;
	std::vector<INPUT_RECORD> _ir_pending;
	bool _kitty_right_ctrl_down = false;
	int _iterm_last_flags = 0;
	char _using_extension = 0;
	//bit indicators for modifier keys in mouse input sequence
	const unsigned int
		_shift_ind = 0x04,
		_alt_ind   = 0x08,
		_ctrl_ind  = 0x10;

	//work-around for double encoded mouse events in win32-input mode
	std::vector<char> _win_double_buffer; // buffer for accumulate unpacked chras
	bool _win32_accumulate = false;      // flag for parse win32-input sequence into _win_double_buffer

	void AssertNoConflicts();

	void AddStr(WORD vk, DWORD control_keys, const char *fmt, ...);
	void AddStr_ControlsThenCode(WORD vk, const char *fmt, const char *code);
	void AddStr_CodeThenControls(WORD vk, const char *fmt, const char *code);
	void AddStrTilde(WORD vk, int code);
	void AddStrTilde_Controls(WORD vk, int code);
	void AddStrF1F5(WORD vk, const char *code);
	void AddStrCursors(WORD vk, const char *code);

	size_t ParseNChars2Key(const char *s, size_t l);

	/**
	 * Parse X10 mouse report sequence.
	 * looks like `x1B[Mayx`
	*/
	size_t ParseX10Mouse(const char *s, size_t l);

	/**
	 * Parse mouse from SGR Extended coordinates sequence.
	 * looks like `\x1B[<a;y;xM` or `\x1B[<a;y;xm`,
	*/
	size_t ParseSGRMouse(const char *s, size_t l);

	/**
	 * Schedule mouse event.
	 * constuct INPUT_RECORD of mouse event and appending it to _ir_pending
	 * Params have to be parsed from input sequence by ether of ParseX10Mouse() or ParseSGRMouse()
	*/
	void AddPendingMouseEvent(int action, int col, int row);

	void ParseAPC(const char *s, size_t l);
	size_t TryParseAsWinTermEscapeSequence(const char *s, size_t l);
	size_t TryUnwrappWinDoubleEscapeSequence(const char *s, size_t l);
	size_t ReadUTF8InHex(const char *s, wchar_t *uni_char);
	size_t TryParseAsITerm2EscapeSequence(const char *s, size_t l);
	size_t TryParseAsKittyEscapeSequence(const char *s, size_t l);
	size_t ParseEscapeSequence(const char *s, size_t l);
	void OnBracketedPaste(bool start);

	void AddPendingKeyEvent(const TTYInputKey &k);
	size_t ParseIntoPending(const char *s, size_t l);


public:

	TTYInputSequenceParser(ITTYInputSpecialSequenceHandler *handler);

	/**
	 * parse and schedule input from recognized sequence.
	 * returns number of parsed bytes, numbers below 1 is error flags of following meaning:
	 * +  0 - (TTY_PARSED_WANTMORE) wait for more characters from input buffer
	 * + -1 - (TTY_PARSED_PLAINCHARS) no sequence encountered, plain text recognized
	 * + -2 - (TTY_PARSED_BADSEQUENCE) unrecognized sequence, skip ESC char (0x1b) and continue
	*/
	size_t Parse(const char *s, size_t l, bool idle_expired);

	/**
	 * parse and schedule mouse sequence from _win_double_buffer.
	 * executed only if _win_double_buffer contains valid number of characters
	 * for X10 mouse sequence
	*/
	void ParseWinDoubleBuffer(bool idle_expired);
	char UsingExtension() const { return _using_extension; };
};
